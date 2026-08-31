/**
 * hooks/protect/TakeEntityEvent.cpp —— 合成事件 "PlayerTakeEntityEvent"，可取消。
 *
 * 补 LeviLamina PlayerPickUpItemEvent 的缺口：那个事件只在 ActorCategory::Item
 * 上发布，而落地的箭矢与三叉戟仍是投射物实体，从不进入判定。两者覆盖面互补不
 * 重叠，掉落物归那个事件，箭矢和三叉戟归本事件，都要管就都订阅。
 *
 * 钩点取各投射物自己的 playerTouch 虚函数，不是 Player::take。Arrow 与
 * ThrownTrident 各有独立 playerTouch 实现，直接把物品塞进背包，不经过
 * Player::take。新增漏网投射物加一行 PIER_PICKUP_HOOK 即可。
 *
 * 载荷 {eventId, x, y, z, dim, entity, entityId, isItemActor, item, _player:{…}}。
 * isItemActor 在当前钩点集合下恒为 false，保留是为载荷形状稳定；x/y/z 取整，
 * 因为 LL 的反射把 Vec3 序列化成 JSON 数组。
 */
#include "pier/hooks/hook_events.h"

#include <set>
#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorCategory.h"
#include "mc/world/actor/item/ItemActor.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/actor/projectile/Arrow.h"
#include "mc/world/actor/projectile/ThrownTrident.h"
#include "mc/world/item/ItemStack.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& takeDef(); // 前向；gDef 定义在本文件末尾

        /** 被捡实体的类型名。取不到就给空串而不是猜。 */
        std::string safeActorType(Actor const& a)
        {
            try
            {
                return std::string{a.getTypeName()};
            }
            catch (...)
            {
                return {};
            }
        }

        /** 掉落物里装的是什么。非掉落物返回空串。 */
        std::string carriedItemName(Actor const& a, bool isItem)
        {
            if (!isItem) return {};
            try
            {
                auto const& stack = static_cast<ItemActor const&>(a).item();
                return std::string{stack.getTypeName()};
            }
            catch (...)
            {
                return {};
            }
        }

        std::string buildSnbt(Player& p, Actor const& taken, bool isItem)
        {
            auto const pos = p.getPosition();
            return std::string{"{\"eventId\":\"PlayerTakeEntityEvent\",\"x\":"}
                + snbtNum(static_cast<int>(pos.x)) + ",\"y\":" + snbtNum(static_cast<int>(pos.y))
                + ",\"z\":" + snbtNum(static_cast<int>(pos.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(p.getDimensionId()))
                + ",\"entity\":\"" + snbtEscape(safeActorType(taken))
                + "\",\"entityId\":"
                + snbtNum(static_cast<int64_t>(taken.getOrCreateUniqueID().rawID))
                + "L,\"isItemActor\":" + (isItem ? "true" : "false")
                + ",\"item\":\"" + snbtEscape(carriedItemName(taken, isItem))
                + "\"," + playerRefSnbt(p) + "}";
        }

        /** 每种实体类型打一次到达证明。「钩子没装上」「挂错了函数」「装对了但
         *  判定放行」三种情况的现象完全一样，这条日志是区分它们的唯一凭据。 */
        void logFirstTouch(Actor const& a)
        {
            static std::set<std::string> seen;
            std::string const key = safeActorType(a);
            if (seen.insert(key).second)
            {
                hostLogger().debug("[TakeEntityEvent] 首次触碰 '{}'", key);
            }
        }

        /**
         * 拦一类投射物的拾取。
         *
         * playerTouch 返回 void，没有取消位，拦截方式是不调用 origin：这次触碰
         * 等于没发生，实体留在原地，下次走过去还能再试。
         */
#define PIER_PICKUP_HOOK(HookName, ActorClass)                                                  \
    LL_TYPE_INSTANCE_HOOK(                                                                      \
        /* 虚函数必须挂 $ 前缀那份，LeviLamina 用它绕开 vtable 派发；直接取                     \
         * &Cls::playerTouch 会被 static_assert 拦下。同 DropItemEvent。 */                     \
        HookName, ll::memory::HookPriority::Normal, ActorClass, &ActorClass::$playerTouch, void, \
        ::Player& player)                                                                       \
    {                                                                                           \
        auto& def = takeDef();                                                                  \
        if (!def.live())                                                                        \
        {                                                                                       \
            origin(player);                                                                     \
            return;                                                                             \
        }                                                                                       \
        logFirstTouch(*this);                                                                   \
        if (dispatchHookEventCancellable(def, buildSnbt(player, *this, false)))                 \
        {                                                                                       \
            /* 不调 origin：这次触碰等于没发生，实体留在原地，可以再试。 */                     \
            return;                                                                             \
        }                                                                                       \
        origin(player);                                                                         \
    }

        PIER_PICKUP_HOOK(ArrowPickupHook, Arrow)
        PIER_PICKUP_HOOK(TridentPickupHook, ThrownTrident)

#undef PIER_PICKUP_HOOK

        HookEventDef gDef{
            "PlayerTakeEntityEvent",
            []
            {
                // 显式装、显式报状态（0 == 成功）。没装上的保护和「装上了但从
                // 不拦」在行为上完全一样，装失败必须看得见。
                int ra = ArrowPickupHook::hook();
                int rt = TridentPickupHook::hook();
                auto& log = hostLogger();
                log.debug(
                    "[TakeEntityEvent] 安装 detour：Arrow::$playerTouch={} (code={})，"
                    "ThrownTrident::$playerTouch={} (code={})",
                    ra == 0 ? "成功" : "失败", ra,
                    rt == 0 ? "成功" : "失败", rt);
                if (ra != 0 || rt != 0)
                {
                    log.error(
                        "[TakeEntityEvent] 原生 detour 安装失败（非 0 状态码）。"
                        "结果：拾取保护对**箭矢、三叉戟等投射物**完全不生效"
                        "（掉落物仍由 PlayerPickUpItemEvent 覆盖）。"
                        "最常见原因是本宿主链接的 BDS/LeviLamina 版本与服务器"
                        "实际运行的版本不一致，导致 $playerTouch 的符号地址解析错误。");
                }
                return ra == 0 && rt == 0;
            }
        };

        HookEventDef& takeDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
