/** actors/Players.cpp —— 玩家管理、属性、动作。
 *
 * 玩家句柄就是选择器（名字 / xuid / uuid），每次调用都对着在线表重新解析
 * —— 永不缓存指针。对版本敏感的写操作走原生调用或原生数据包。
 */
#include <algorithm>
#include <cmath>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mc/deps/core/math/Vec2.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/core/string/HashedString.h"
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/network/MinecraftPacketIds.h"
#include "mc/network/MinecraftPackets.h"
#include "mc/network/NetworkPeer.h"
#include "mc/network/packet/RemoveObjectivePacket.h"
#include "mc/network/packet/ScorePacketInfo.h"
#include "mc/network/packet/ScorePacketType.h"
#include "mc/network/packet/SetDisplayObjectivePacket.h"
#include "mc/network/packet/SetScorePacket.h"
#include "mc/network/packet/SetTitlePacket.h"
#include "mc/network/packet/SetTitlePacketPayload.h"
#include "mc/network/packet/TextPacket.h"
#include "mc/network/packet/TextPacketPayload.h"
#include "mc/network/packet/TextPacketType.h"
#include "mc/network/packet/UpdateAbilitiesPacket.h"
#include "mc/platform/UUID.h"
#include "mc/server/commands/PlayerPermissionLevel.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorHurtResult.h"
#include "mc/world/actor/player/Abilities.h"
#include "mc/world/actor/player/AbilitiesIndex.h"
#include "mc/world/actor/player/LayeredAbilities.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/attribute/Attribute.h"
#include "mc/world/attribute/AttributeInstance.h"
#include "mc/world/attribute/AttributeInstanceConstRef.h"
#include "mc/world/attribute/AttributeInstanceForwarder.h"
#include "mc/world/attribute/MutableAttributeWithContext.h"
#include "mc/world/gamemode/InteractionResult.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/DimensionType.h"
#include "mc/world/scores/IdentityDefinition.h"
#include "mc/world/scores/ObjectiveSortOrder.h"
#include "mc/world/scores/ScoreboardId.h"

#include "sdk/abi.h"

#include "pier/api/bridge.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/log.h"
#include "pier/support/snbt.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        /**
         * objective 名 → 一段独占的 ScoreboardId 槽位号。
         *
         * 侧边栏的每一行是一个 FakePlayer 计分条目，键是 ScoreboardId。id 段
         * 早先对所有 objective 是同一个常量，于是两个插件的第 N 行是同一个条
         * 目，互相覆盖 —— 屏幕上两套内容穿插、右侧两组分数。
         *
         * 用 FNV-1a 把名字散到 [0, 2^18) 上，乘以 4096 行的间距。同名必定同段
         * （幂等，重复设置不会漂移），不同名几乎必定不同段。
         */
        uint32_t objectiveSlotHash(std::string_view name)
        {
            uint32_t h = 2166136261u;
            for (char c : name)
            {
                h ^= static_cast<unsigned char>(c);
                h *= 16777619u;
            }
            // 2^18 段 × 4096 行 = 2^30，正好填满 0x40000000 之上的一个象限。
            return h & 0x3FFFFu;
        }

        /// 按 '\n' 切分。给侧边栏两个动作用 —— 它们的整个载荷是一条换行拼接
        /// 的字符串：一次 FFI 字符串胜过 N 次调用，侧边栏本来也是整块重建的。
        std::vector<std::string> splitLines(std::string_view text)
        {
            std::vector<std::string> out;
            while (true)
            {
                auto const nl = text.find('\n');
                if (nl == std::string_view::npos)
                {
                    out.emplace_back(text);
                    return out;
                }
                out.emplace_back(text.substr(0, nl));
                text.remove_prefix(nl + 1);
            }
        }

        void api_list_players(void* ctx, PierStrSink snbtSink)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level || !snbtSink) return;
                level->forEachPlayer([&](Player& p)
                {
                    snbtSink(ctx, ps(bridge::playerSummarySnbt(p)));
                    return true;
                });
            PIER_API_GUARD_END_VOID
        }

        bool api_player_resolve(PierPlayerSel sel, PierActorId* out)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p || !out) return false;
                *out = p->getOrCreateUniqueID().rawID;
                return true;
            PIER_API_GUARD_END
        }

        bool api_player_send_message(PierPlayerSel sel, PierStr msg)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p) return false;
                p->sendMessage(sv(msg));
                return true;
            PIER_API_GUARD_END
        }

        bool api_player_send_message_typed(PierPlayerSel sel, PierStr msg, int32_t type)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p) return false;

                // 把 ABI 的整数映射到 TextPacketType；越界的落回 Raw（客户端上
                // 一行普通文本），而不是拒绝。
                auto ptype = TextPacketType::Raw;
                if (type >= 0 && type <= 11)
                    ptype = static_cast<TextPacketType>(static_cast<uchar>(type));

                // 构一个带 MessageOnly 体的 TextPacket —— createRawMessage 用的
                // 就是这个形状，只是类型换成调用方给的。这覆盖了所有单字符串的
                // 种类（Tip、Popup、JukeboxPopup、SystemMessage、Announcement、
                // …）。带作者/参数的种类（Chat/Whisper/Translate）在这里同样按
                // 纯消息发 —— 和 LSE 的 tell(msg, type) 做的是同一个简化。
                TextPacket pkt{};
                TextPacketPayload::MessageOnly body;
                body.mType = ptype;
                body.mMessage->assign(sv(msg));
                pkt.mBody = body;

                p->sendNetworkPacket(pkt);
                return true;
            PIER_API_GUARD_END
        }

        bool api_player_disconnect(PierPlayerSel sel, PierStr reason)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p) return false;
                p->disconnect(sv(reason));
                return true;
            PIER_API_GUARD_END
        }

        void api_broadcast_message(PierStr msg)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level) return;
                std::string_view text = sv(msg);
                level->forEachPlayer([&](Player& p)
                {
                    p.sendMessage(text);
                    return true;
                });
            PIER_API_GUARD_END_VOID
        }

        bool api_player_set_gamemode(PierPlayerSel sel, int32_t mode)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p) return false;
                // mode 用的就是引擎的 GameType 判别值（0=生存 1=创造 2=冒险
                // 6=旁观），这里只做白名单校验，不再翻译成命令里的名字。
                //
                // 顺带修掉命令路径藏着的一个坑：玩家名早先直接拼进带引号的命
                // 令，名字里有引号或反斜杠就能把命令撕开。
                switch (mode)
                {
                case 0:
                case 1:
                case 2:
                case 6:
                    break;
                default:
                    return false;
                }
                p->setPlayerGameType(static_cast<::GameType>(mode));
                return true;
            PIER_API_GUARD_END
        }

        /**
         * 传送玩家，需要时跨维度。
         *
         * 早先的实现错在两处：
         *
         *  1. `if (dim < 0 || dim > 2) return false;` 把每个自定义维度都拒了
         *     （自定义维度 id 从 3 起），经这个 API 进自定义维度根本不可能。
         *     注意 Actors.cpp 里**实体**传送路径从来没有这条限制 —— 玩家这条
         *     纯粹是疏忽。
         *
         *  2. 它 shell 出去跑 `/execute in <name> run tp`。`execute in` 子命令
         *     吃的是由原版集合建出来的**命令枚举**，自定义维度名在那里未必是
         *     合法 token —— 命令可能解析失败，甚至解析进错的维度，哪怕名字已
         *     经注册。
         *
         * Actor::teleport 是 LeviLamina 自己的跨维度助手（api_actor_action 已
         * 在用），走的是引擎自己的换维度机制。客户端对自定义维度有真实认知
         * （DimensionDataPacket 已描述给它），原版路径就是全部所需 —— 没有任
         * 何包改写层要过。命令路径则整个绕开了引擎机制。
         */
        bool api_player_teleport(PierPlayerSel sel, int32_t dim, double x, double y, double z)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p) return false;

                auto const name = bridge::dimensionSelector(dim);
                if (name.empty()) return false;

                // blockSourceOf 在这里兼作「强制建出 + 放行闸」。维度桥的实现
                // 方（dimensions 包）**必须**校验建出的引擎实例 id == 请求的
                // dim（spi.h §6 写明）：两者不一致时把玩家送进 dim，引擎会在
                // 区块工作线程上抛未捕获异常，整个进程 fastfail(0xC0000409)
                // —— 不是一句「传送失败」能兜住的。校验所需的台账知识只在
                // dimensions 那一头，所以门设在桥的实现里，这里只认「非空即
                // 放行」。
                if (!bridge::blockSourceOf(dim)) return false;

                p->teleport(Vec3{(float)x, (float)y, (float)z}, DimensionType{dim}, p->getRotation());
                return true;
            PIER_API_GUARD_END
        }

        // ───────────────────────── 属性助手 ─────────────────────────

        /** 读一个 attribute 的当前值；无 NaN：缺失时 ok=false。 */
        bool readAttribute(Player& p, Attribute const& attr, double* out)
        {
            auto cref = p.getAttribute(attr);
            // mPtr 是标量 TypedStorage（裸指针），没有 .get() 包装。
            auto* inst = cref.mPtr;
            if (!inst) return false;
            *out = static_cast<double>(inst->getCurrentValue());
            return true;
        }

        /**
         * 经 AttributeInstanceForwarder 写 attribute 当前值，让监听器触发、
         * 玩家同步类属性到达客户端。
         */
        bool writeAttribute(Player& p, Attribute const& attr, float value)
        {
            // getMutableAttribute 把实例和修改上下文捆在一起，经 operator->
            // 暴露 forwarder；它的 bool 测试守住「不存在」。
            auto mut = p.getMutableAttribute(attr);
            if (!mut) return false;
            mut->setCurrentValue(value);
            return true;
        }

        // ───────────────────────── 属性 ─────────────────────────

        bool api_player_get_num(PierPlayerSel sel, int32_t prop, double* out)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p || !out) return false;
                switch (prop)
                {
                case PIER_PPROP_GAME_TYPE:
                    *out = static_cast<double>(static_cast<int>(p->getPlayerGameType()));
                    return true;
                case PIER_PPROP_LEVEL:
                    return readAttribute(*p, Player::LEVEL(), out);
                case PIER_PPROP_EXPERIENCE:
                    return readAttribute(*p, Player::EXPERIENCE(), out);
                case PIER_PPROP_HUNGER:
                    return readAttribute(*p, Player::HUNGER(), out);
                case PIER_PPROP_SATURATION:
                    return readAttribute(*p, Player::SATURATION(), out);
                case PIER_PPROP_EXHAUSTION:
                    return readAttribute(*p, Player::EXHAUSTION(), out);
                case PIER_PPROP_XP_NEEDED_NEXT_LEVEL:
                    *out = static_cast<double>(p->getXpNeededForNextLevel());
                    return true;
                case PIER_PPROP_LUCK:
                    *out = static_cast<double>(p->getLuck());
                    return true;
                case PIER_PPROP_SELECTED_SLOT:
                    *out = static_cast<double>(p->getSelectedItemSlot());
                    return true;
                case PIER_PPROP_IS_OPERATOR:
                    *out = p->isOperator() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_CAN_USE_OPERATOR_BLOCKS:
                    *out = p->canUseOperatorBlocks() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_IS_FLYING:
                    *out = p->isFlying() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_CAN_JUMP:
                    *out = p->canJump() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_IS_EMOTING:
                    *out = p->isEmoting() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_IS_IN_RAID:
                    *out = p->isInRaid() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_IS_HURT:
                    *out = p->isHurt() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_IS_SCOPING:
                    // Player::isScoping() 在生成头里被 #ifdef LL_PLAT_C 圈着，普
                    // 通构建拿不到。这一个属性报「不支持」，而不是编译失败。
                    return false;
                case PIER_PPROP_CAN_SLEEP:
                    *out = p->canSleep() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_HAS_RESPAWN_POSITION:
                    *out = p->hasRespawnPosition() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_CLIENT_SUB_ID:
                    *out = static_cast<double>(static_cast<int>(p->getClientSubId()));
                    return true;
                /* ── 追加：玩家补漏 ── */
                case PIER_PPROP_DIRECTION:
                    *out = static_cast<double>(p->getDirection());
                    return true;
                case PIER_PPROP_CHUNK_RADIUS:
                    *out = static_cast<double>(p->getChunkRadius());
                    return true;
                case PIER_PPROP_NETWORK_RTT:
                {
                    // getNetworkStatus() 返回 optional<NetworkPeer::NetworkStatus>；
                    // mCurrentPing 是包了一层的 chrono::milliseconds，用 ->count()。
                    auto opt = p->getNetworkStatus();
                    if (!opt) return false;
                    *out = static_cast<double>(opt->mCurrentPing->count());
                    return true;
                }
                case PIER_PPROP_PLATFORM:
                    *out = static_cast<double>(static_cast<int>(p->getPlatform()));
                    return true;
                case PIER_PPROP_ENCHANTMENT_SEED:
                    *out = static_cast<double>(p->getEnchantmentSeed());
                    return true;
                case PIER_PPROP_IS_USING_ITEM:
                    *out = p->isUsingItem() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_IS_BLOCKING:
                    *out = p->isBlocking() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_IS_GLIDING:
                    *out = p->isGliding() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_IS_SWIMMING:
                    *out = p->isSwimming() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_PERMISSION_LEVEL:
                    *out = static_cast<double>(static_cast<int>(p->getPlayerPermissionLevel()));
                    return true;
                case PIER_PPROP_SCORE:
                    // Player 没有 getScore()；值住在公开成员 mScore 里
                    //（TypedStorage<int> 取用时坍缩成裸 int）。
                    *out = static_cast<double>(p->mScore);
                    return true;
                case PIER_PPROP_FALL_DISTANCE:
                    *out = static_cast<double>(p->getFallDistance());
                    return true;
                case PIER_PPROP_IS_DEAD:
                    *out = p->isDead() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_HAS_DIED_BEFORE:
                    *out = p->hasDiedBefore() ? 1.0 : 0.0;
                    return true;
                // 玩家当前所在维度。自定义维度的 id >= 3，所以调用方不能假设只
                // 有 0/1/2。写法照抄 Actors.cpp 里已有的那一处，Player 继承自
                // Actor。
                case PIER_PPROP_DIMENSION:
                    *out = static_cast<double>(static_cast<int>(p->getDimensionId()));
                    return true;
                default:
                    return false;
                }
            PIER_API_GUARD_END
        }

        bool api_player_get_str(PierPlayerSel sel, int32_t prop, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p || !sink) return false;
                switch (prop)
                {
                case PIER_PSTR_REAL_NAME:
                    sink(ctx, ps(p->getRealName()));
                    return true;
                case PIER_PSTR_UUID:
                    sink(ctx, ps(p->getUuid().asString()));
                    return true;
                case PIER_PSTR_XUID:
                    sink(ctx, ps(p->getXuid()));
                    return true;
                case PIER_PSTR_IP_AND_PORT:
                    sink(ctx, ps(p->getIPAndPort()));
                    return true;
                case PIER_PSTR_LOCALE_CODE:
                    sink(ctx, ps(p->getLocaleCode()));
                    return true;
                case PIER_PSTR_NAME_TAG:
                    sink(ctx, ps(p->getNameTag()));
                    return true;
                /* ── 追加 ── */
                case PIER_PSTR_LAST_DEATH_POS:
                {
                    auto pos = p->getLastDeathPos();
                    if (!pos.has_value())
                    {
                        sink(ctx, ps(std::string_view{}));
                        return true;
                    }
                    std::string snbt = "{x:" + snbtNum(pos->x) + ",y:" + snbtNum(pos->y)
                        + ",z:" + snbtNum(pos->z) + "}";
                    sink(ctx, ps(snbt));
                    return true;
                }
                case PIER_PSTR_LAST_DEATH_DIMENSION:
                {
                    auto dim = p->getLastDeathDimension();
                    if (!dim.has_value())
                    {
                        sink(ctx, ps(std::string_view{}));
                        return true;
                    }
                    sink(ctx, ps(snbtNum(static_cast<int>(*dim))));
                    return true;
                }
                case PIER_PSTR_NETWORK_STATUS:
                {
                    // NetworkStatus 字段：mCurrentPing/mAveragePing 是包了一层的
                    // chrono::milliseconds（用 ->count()）；丢包字段是裸 float
                    //（直接用）。返回 optional，要判。
                    auto opt = p->getNetworkStatus();
                    if (!opt) return false;
                    auto const& ns = *opt;
                    std::string snbt = "{ping:" + snbtNum(ns.mCurrentPing->count());
                    snbt += ",avg_ping:" + snbtNum(ns.mAveragePing->count());
                    snbt += ",packet_loss:" + snbtDouble(ns.mCurrentPacketLoss);
                    snbt += ",avg_packet_loss:" + snbtDouble(ns.mAveragePacketLoss);
                    snbt += ",max_bps:" + snbtDouble(ns.mApproximateMaxBps) + "}";
                    sink(ctx, ps(snbt));
                    return true;
                }
                case PIER_PSTR_PLATFORM_ONLINE_ID:
                    sink(ctx, ps(p->getPlatformOnlineId()));
                    return true;
                default:
                    return false;
                }
            PIER_API_GUARD_END
        }

        bool api_player_set_num(PierPlayerSel sel, int32_t prop, double v)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p) return false;
                switch (prop)
                {
                case PIER_PPROP_LEVEL:
                    return writeAttribute(*p, Player::LEVEL(), static_cast<float>(v));
                case PIER_PPROP_EXPERIENCE:
                    return writeAttribute(*p, Player::EXPERIENCE(), static_cast<float>(v));
                case PIER_PPROP_HUNGER:
                    return writeAttribute(*p, Player::HUNGER(), static_cast<float>(v));
                case PIER_PPROP_SATURATION:
                    return writeAttribute(*p, Player::SATURATION(), static_cast<float>(v));
                case PIER_PPROP_EXHAUSTION:
                    return writeAttribute(*p, Player::EXHAUSTION(), static_cast<float>(v));
                default:
                    return false; // 只读或未知
                }
            PIER_API_GUARD_END
        }

        // ───────────────────────── 动作 ─────────────────────────

        /**
         * 设一个能力位，正确分派 bool 与 float。
         *
         * 四个独立的坑在这里汇合，所以它是助手函数而不是一行改动：
         *
         *  1. 旧分派用 `idx < 32` 区分 bool / float。这纯粹是错的 ——
         *     AbilitiesIndex 只到 0..19，float 槽在 13（FlySpeed）、14
         *     （WalkSpeed）、19（VerticalFlySpeed）。于是所有能力全走了
         *     bool 路径。
         *
         *  2. `Player::setAbility` **只有** bool 重载（Player.h:286），连
         *     "float" 分支也经隐式 float→bool 落到它上：任何非零速度都静默
         *     变成 `true`。float 能力从来没工作过。float 路径必须走
         *     LayeredAbilities::setAbility(idx, float)
         *     （LayeredAbilities.h:25），那边两个重载都有。
         *
         *  3. 服务端写层不等于告诉客户端。移动/飞行速度是客户端应用的，不
         *     发 UpdateAbilitiesPacket 玩家就按旧速度动。
         *
         * bool 路径刻意仍走 Player::setAbility：那是 LeviLamina 自己的助
         * 手、已经同步、而且布尔能力现在是好的 —— 没理由去动它们。
         *
         *  4. **写任何能力位都会把玩家的 PlayerPermissionLevel 静默降成
         *     Custom。** `LayeredAbilities::setAbility` 是引擎「切到自定义权
         *     限」的路径 —— 和按玩家的权限勾选框走的同一条 —— 它把玩家挪到
         *     AbilitiesLayer::CustomCache 上，从此报告
         *     PlayerPermissionLevel::Custom。随之而来的
         *     UpdateAbilitiesPacket 把 mPlayerPermissions 和能力层放在一起
         *     （SerializedAbilitiesData），于是客户端被告知它不再是管理员，
         *     开始按访客行事：普通方块没有选框（中继器、容器这类可交互的还
         *     有）、不能攻击、没有本地放置预测。服务端什么都没变，命令和服
         *     务端驱动的动作照常 —— 这正是它难被认出来的原因。
         *
         *     它还是持久的：PermissionsHandler::addSaveData 把这个等级写进玩
         *     家存档，重新登录也不会复原。
         *
         *     快照等级、写、写完发现被顶了就放回去。真想要 Custom 的调用方经
         *     PIER_PACT_SET_PERMISSION_LEVEL 显式要。
         */
        bool setPlayerPermissionLevel(Player& p, PlayerPermissionLevel level)
        {
            p.getAbilities().setPlayerPermissions(level);
            UpdateAbilitiesPacket pkt{p.getOrCreateUniqueID(), p.getAbilities()};
            p.sendNetworkPacket(pkt);
            return true;
        }

        bool setPlayerAbility(Player& p, int idx, double value)
        {
            if (idx < 0 || idx >= static_cast<int>(AbilitiesIndex::AbilityCount))
            {
                return false;
            }
            auto index = static_cast<AbilitiesIndex>(idx);
            auto const before = p.getPlayerPermissionLevel();

            switch (index)
            {
            case AbilitiesIndex::FlySpeed:
            case AbilitiesIndex::WalkSpeed:
            case AbilitiesIndex::VerticalFlySpeed:
            {
                if (!p.getAbilities().setAbility(index, static_cast<float>(value)))
                {
                    return false;
                }
                // 把整个分层集合推给客户端；速度在客户端应用，不推不变。
                UpdateAbilitiesPacket pkt{p.getOrCreateUniqueID(), p.getAbilities()};
                p.sendNetworkPacket(pkt);
                break;
            }
            default:
            {
                bool const want = value != 0.0;
                p.setAbility(index, want);

                if (p.canUseAbility(index) != want)
                {
                    UpdateAbilitiesPacket resync{p.getOrCreateUniqueID(), p.getAbilities()};
                    p.sendNetworkPacket(resync);

                    hostLogger().error(
                        "setPlayerAbility: 写 idx={} want={} 之后回读不一致，这一位没有落地。"
                        "最常见的原因是在 PlayerJoinEvent 里就写能力位，那时玩家的能力层还没"
                        "建好。改成进服之后延迟一两秒再写，或者干脆不写这一位。已经给客户端"
                        "补发了一份当前状态。",
                        idx,
                        want
                    );

                    if (idx <= static_cast<int>(AbilitiesIndex::AttackMobs))
                    {
                        return false;
                    }
                }
                break;
            }
            }

            if (p.getPlayerPermissionLevel() != before)
            {
                hostLogger().warn(
                    "setPlayerAbility: 写 idx={} 把权限等级从 {} 顶到了 {}，已还原。"
                    "这在实测里没有发生过，出现了说明引擎行为变了。",
                    idx,
                    static_cast<int>(before),
                    static_cast<int>(p.getPlayerPermissionLevel())
                );
                setPlayerPermissionLevel(p, before);
            }
            return true;
        }

        bool api_player_action(
            PierPlayerSel sel,
            int32_t action,
            PierStr sarg,
            double a,
            double b,
            double c,
            void* ctx,
            PierStrSink out)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p) return false;
                switch (action)
                {
                case PIER_PACT_SET_ABILITY:
                {
                    int idx = static_cast<int>(a);
                    return setPlayerAbility(*p, idx, b);
                }
                case PIER_PACT_CAN_USE_ABILITY:
                {
                    int idx = static_cast<int>(a);
                    if (idx < 0 || idx >= static_cast<int>(AbilitiesIndex::AbilityCount)) return false;
                    bool can = p->canUseAbility(static_cast<AbilitiesIndex>(idx));
                    if (out) out(ctx, ps(std::string_view{can ? "1" : "0"}));
                    return true;
                }
                case PIER_PACT_SET_SELECTED_SLOT:
                {
                    int slot = static_cast<int>(a);
                    if (slot < 0 || slot > 8) return false;
                    p->setSelectedSlot(slot);
                    return true;
                }
                case PIER_PACT_GIVE_ITEM:
                {
                    auto opt = bridge::itemFromSnbt(sv(sarg));
                    if (!opt) return false;
                    ItemStack item = std::move(*opt);
                    if (item.isNull()) return false;
                    return p->addAndRefresh(item);
                }
                case PIER_PACT_SET_SPAWN_POINT:
                {
                    std::string dimStr = toString(sarg);
                    int dim = 0;
                    if (!dimStr.empty())
                    {
                        try
                        {
                            // 早先被钳在 0..2，把指向自定义维度的重生点静默挪进
                            // 了末地。
                            dim = std::stoi(dimStr);
                        }
                        catch (...)
                        {
                            return false;
                        }
                    }
                    // 原生。同上，不再把玩家名拼进命令字符串。
                    p->setRespawnPosition(
                        BlockPos{static_cast<int>(a), static_cast<int>(b), static_cast<int>(c)},
                        static_cast<::DimensionType>(dim)
                    );
                    return true;
                }
                case PIER_PACT_CLEAR_TITLE:
                {
                    // 原生数据包。命令路径要把玩家名拼进带引号的字符串里，名字
                    // 含引号就撕开命令；而且 /title 会走一遍命令解析和权限检
                    // 查，对一个「给这个玩家发个包」的动作来说全是白付的。
                    SetTitlePacketPayload payload{SetTitlePacketPayload::TitleType::Clear};
                    SetTitlePacket{std::move(payload)}.sendTo(*p);
                    return true;
                }
                case PIER_PACT_SET_TITLE:
                {
                    auto kind = static_cast<int>(a);
                    auto type = kind == 1
                                    ? SetTitlePacketPayload::TitleType::Subtitle
                                    : kind == 2
                                    ? SetTitlePacketPayload::TitleType::Actionbar
                                    : SetTitlePacketPayload::TitleType::Title;
                    // filteredTitleText 传 nullopt：那是给聊天过滤用的备用文本，
                    // 这条链路上的文本来自模组而不是玩家输入，没有可过滤的东西。
                    SetTitlePacketPayload payload{type, toString(sarg), std::nullopt};
                    SetTitlePacket{std::move(payload)}.sendTo(*p);
                    return true;
                }
                /* ── 追加 ── */
                case PIER_PACT_ADD_EXPERIENCE:
                    p->addExperience(static_cast<int>(a));
                    return true;
                case PIER_PACT_ADD_LEVELS:
                    p->addLevels(static_cast<int>(a));
                    return true;
                case PIER_PACT_START_COOLDOWN:
                    // startItemCooldown(HashedString const&, int ticks, bool updateClient)
                    p->startItemCooldown(HashedString{toString(sarg)}, static_cast<int>(a), true);
                    return true;
                case PIER_PACT_START_RIDING:
                {
                    auto* vehicle = bridge::resolveActor(static_cast<PierActorId>(a));
                    if (!vehicle) return false;
                    // startRiding(Actor&, bool forceRiding) —— force=true 让请求
                    // 在载具已满时也成功。
                    return p->startRiding(*vehicle, true);
                }
                case PIER_PACT_STOP_RIDING:
                    // stopRiding(bool exitFromPassenger, bool actorIsBeingDestroyed,
                    //            bool switchingVehicles, bool isBeingTeleported)
                    p->stopRiding(true, false, false, false);
                    return true;
                case PIER_PACT_ATTACK:
                {
                    auto* target = bridge::resolveActor(static_cast<PierActorId>(a));
                    if (!target) return false;
                    // attack(Actor&, ActorDamageCause const&) —— 调用方没指明来
                    // 源，用 Override（通用原因）。
                    p->attack(*target, ::SharedTypes::Legacy::ActorDamageCause::Override);
                    return true;
                }
                case PIER_PACT_DROP:
                {
                    auto opt = bridge::itemFromSnbt(sv(sarg));
                    if (!opt) return false;
                    return p->drop(std::move(*opt), a != 0.0);
                }
                case PIER_PACT_INTERACT:
                {
                    auto* target = bridge::resolveActor(static_cast<PierActorId>(a));
                    if (!target) return false;
                    // interact(Actor&, Vec3 const& location) 返回 InteractionResult；
                    // 把它的 mSuccess 位当 bool 返回。
                    auto result = p->interact(*target, target->getPosition());
                    return result.mSuccess;
                }
                case PIER_PACT_START_USING_ITEM:
                {
                    auto opt = bridge::itemFromSnbt(sv(sarg));
                    if (!opt) return false;
                    p->startUsingItem(std::move(*opt), static_cast<int>(a));
                    return true;
                }
                case PIER_PACT_STOP_USING_ITEM:
                    p->stopUsingItem();
                    return true;
                case PIER_PACT_SET_CHUNK_RADIUS:
                    p->setChunkRadius(static_cast<int>(a));
                    return true;
                case PIER_PACT_SET_ENCHANTMENT_SEED:
                    p->setEnchantmentSeed(static_cast<int>(a));
                    return true;
                case PIER_PACT_REGISTER_TRACKED_BOSS:
                {
                    auto* boss = bridge::resolveActor(static_cast<PierActorId>(a));
                    if (!boss) return false;
                    // registerTrackedBoss 吃的是 ActorUniqueID，不是 Actor 引用。
                    p->registerTrackedBoss(boss->getOrCreateUniqueID());
                    return true;
                }
                case PIER_PACT_UNREGISTER_TRACKED_BOSS:
                {
                    auto* boss = bridge::resolveActor(static_cast<PierActorId>(a));
                    if (!boss) return false;
                    p->unRegisterTrackedBoss(boss->getOrCreateUniqueID());
                    return true;
                }
                case PIER_PACT_PLAY_EMOTE:
                    // playEmote(string const& pieceId, bool playChatMessage)
                    p->playEmote(toString(sarg), false);
                    return true;
                case PIER_PACT_RESEND_ALL_CHUNKS:
                    p->resendAllChunks();
                    return true;
                case PIER_PACT_OPEN_INVENTORY:
                    p->openInventory();
                    return true;
                /* ── 追加：侧边栏 ── */
                case PIER_PACT_SIDEBAR_SET:
                {
                    // sarg = "objective\ntitle\nline1\nline2…"。只作用于这个玩家
                    // 的侧边栏 —— 服务端 Scoreboard 是全局的，按玩家的板子只能
                    // 是一串客户端从不与真实计分板状态关联的数据包。
                    //
                    // 包在这里构，而不是把字节递过 FFI：
                    // SetDisplayObjective/RemoveObjective 现在是 PayloadPacket
                    //（反射序列化），线上形状不是模组能手搓并跨版本维持的。
                    // api_player_send_title 存在的理由与此相同。
                    auto const lines = splitLines(sv(sarg));
                    if (lines.size() < 2)
                    {
                        hostLogger().error("sidebar: payload 少于两行（objective + title）");
                        return false;
                    }
                    std::string const& objective = lines[0];
                    if (objective.empty())
                    {
                        hostLogger().error("sidebar: objective 是空的");
                        return false;
                    }

                    // 每次从零重建：客户端按 scoreboard id 键控条目，行集变了还
                    // 复用 id 正是陈旧行的来源。
                    if (auto gone = MinecraftPackets::createPacket(MinecraftPacketIds::RemoveObjective))
                    {
                        static_cast<RemoveObjectivePacket*>(gone.get())->mObjectiveName = objective;
                        p->sendNetworkPacket(*gone);
                    }
                    else
                    {
                        hostLogger().error("sidebar: createPacket(RemoveObjective) 返回空");
                    }

                    auto shown = MinecraftPackets::createPacket(MinecraftPacketIds::SetDisplayObjective);
                    if (!shown)
                    {
                        hostLogger().error("sidebar: createPacket(SetDisplayObjective) 返回空");
                        return false;
                    }
                    {
                        auto* d = static_cast<SetDisplayObjectivePacket*>(shown.get());
                        d->mDisplaySlotName = std::string{"sidebar"};
                        d->mObjectiveName = objective;
                        d->mObjectiveDisplayName = lines[1];
                        d->mCriteriaName = std::string{"dummy"};
                        d->mSortOrder = ObjectiveSortOrder::Descending;
                        p->sendNetworkPacket(*shown);
                    }

                    if (lines.size() == 2) return true;

                    // ScoreboardId 段按 objective 名分开。
                    //
                    // 这里早先是一个写死的常量 0x40000000，**所有插件共用**。两
                    // 个插件同时开侧边栏时，各自的第 1 行都落在 0x40000001 ——
                    // 那是同一个 scoreboard 条目，谁后发谁覆盖。屏幕上就是两套
                    // 内容穿插在一起、右侧出现两组分数，而且谁都清不掉对方的。
                    //
                    // 按名字哈希出各自的段位。段间距 4096 行，远超 MAX_ROWS，
                    // 所以不会有实际重叠；哈希冲突的概率是 1/(2^30/4096)，而且
                    // 真撞上也只影响同时开两个侧边栏的场景 —— 比现在这个必然冲
                    // 突好得多。
                    //
                    // 高位固定 0x4 是为了避开原版计分板真实用到的低位 id 段。
                    int64_t const kSidebarIdBase = INT64_C(0x40000000)
                        + (static_cast<int64_t>(objectiveSlotHash(objective)) * INT64_C(4096));
                    std::vector<ScorePacketInfo> infos;
                    infos.reserve(lines.size() - 2);
                    int score = static_cast<int>(lines.size()) - 2;
                    for (size_t i = 2; i < lines.size(); ++i, --score)
                    {
                        ScorePacketInfo info{};
                        info.mScoreboardId->mRawID = kSidebarIdBase + static_cast<int64_t>(i - 1);
                        info.mObjectiveName = objective;
                        info.mScoreValue = score;
                        info.mIdentityType = IdentityDefinition::Type::FakePlayer;
                        info.mFakePlayerName = lines[i].empty() ? std::string{" "} : lines[i];
                        infos.push_back(std::move(info));
                    }

                    auto scores = MinecraftPackets::createPacket(MinecraftPacketIds::SetScore);
                    if (!scores)
                    {
                        hostLogger().error("sidebar: createPacket(SetScore) 返回空");
                        return false;
                    }
                    auto* sp = static_cast<SetScorePacket*>(scores.get());
                    auto const rows = infos.size();
                    sp->mType = ScorePacketType::Change;
                    sp->mScoreInfo = std::move(infos);
                    p->sendNetworkPacket(*scores);

                    // 到达证明：每个 objective 打一次。
                    //
                    // 早先是全局一次（static bool），于是第二个插件的侧边栏有没
                    // 有真的发出去、用的是哪一段 id，日志里完全看不出来 —— 而那
                    // 正是两个侧边栏互相覆盖时唯一需要知道的事。
                    static std::set<std::string> announcedObjectives;
                    if (announcedObjectives.insert(objective).second)
                    {
                        hostLogger().debug(
                            "sidebar: '{}' 已发出 {} 行（FakePlayer，id 段 0x{:x}..0x{:x}）",
                            objective, rows,
                            static_cast<uint64_t>(kSidebarIdBase + 1),
                            static_cast<uint64_t>(kSidebarIdBase + static_cast<int64_t>(rows)));
                    }
                    return true;
                }
                case PIER_PACT_SIDEBAR_CLEAR:
                {
                    if (sarg.len == 0) return false;

                    // **先解绑显示槽，再删 objective。** 顺序反了等于没清。
                    //
                    // SIDEBAR_SET 是三步：RemoveObjective → 建 objective →
                    // SetDisplayObjective 把它挂到 "sidebar" 槽。而这里早先只做
                    // 了删 objective 这一步 —— 客户端会删掉计分项，但**槽位仍然
                    // 绑在这个名字上**，屏幕上的旧内容不会消失。
                    //
                    // 更麻烦的是它把槽位占着不放：另一个插件随后调 SIDEBAR_SET，
                    // 它发的 RemoveObjective 移除的是**自己的**名字，动不了这条
                    // 陈旧绑定，于是它的侧边栏也显示不出来。两个插件轮流用一个
                    // 槽位时（起床战争维度进出）表现就是「出来之后卡在旧内容，
                    // 别的插件也抢不回来」。
                    //
                    // SetDisplayObjective 带空的 mObjectiveName 就是「这个槽不显
                    // 示任何东西」—— 这是原版 `/scoreboard objectives setdisplay
                    // sidebar`（不带目标名）走的同一条线。
                    if (auto blank =
                            MinecraftPackets::createPacket(MinecraftPacketIds::SetDisplayObjective))
                    {
                        auto* d = static_cast<SetDisplayObjectivePacket*>(blank.get());
                        d->mDisplaySlotName = std::string{"sidebar"};
                        d->mObjectiveName = std::string{};
                        d->mObjectiveDisplayName = std::string{};
                        d->mCriteriaName = std::string{"dummy"};
                        d->mSortOrder = ObjectiveSortOrder::Descending;
                        p->sendNetworkPacket(*d);
                    }

                    auto gone = MinecraftPackets::createPacket(MinecraftPacketIds::RemoveObjective);
                    if (!gone) return false;
                    static_cast<RemoveObjectivePacket*>(gone.get())->mObjectiveName = toString(sarg);
                    p->sendNetworkPacket(*gone);

                    hostLogger().debug("sidebar: 已清除 '{}'（解绑槽位 + 删 objective）", sv(sarg));
                    return true;
                }
                case PIER_PACT_SET_PERMISSION_LEVEL:
                {
                    int lvl = static_cast<int>(a);
                    if (lvl < static_cast<int>(PlayerPermissionLevel::Visitor)
                        || lvl > static_cast<int>(PlayerPermissionLevel::Custom))
                    {
                        return false;
                    }
                    return setPlayerPermissionLevel(*p, static_cast<PlayerPermissionLevel>(lvl));
                }

                default:
                    return false;
                }
            PIER_API_GUARD_END
        }

        void fill(PierApi& api)
        {
            api.list_players = &api_list_players;
            api.player_resolve = &api_player_resolve;
            api.player_send_message = &api_player_send_message;
            api.player_send_message_typed = &api_player_send_message_typed;
            api.player_disconnect = &api_player_disconnect;
            api.broadcast_message = &api_broadcast_message;
            api.player_set_gamemode = &api_player_set_gamemode;
            api.player_teleport = &api_player_teleport;
            api.player_get_num = &api_player_get_num;
            api.player_get_str = &api_player_get_str;
            api.player_set_num = &api_player_set_num;
            api.player_action = &api_player_action;
        }

        spi::SlotPackReg reg{{"players", &fill}};
    } // namespace
} // namespace pier::api_impl
