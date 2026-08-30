/** net/Packets.cpp —— 按连接投递数据包（追加，struct_size 把门）。
 *
 * 两层：
 *   - api_send_packet：裸原语。任意 MinecraftPacketIds + 线格式包体，反序列
 *     化成真数据包对象、交给**一个**玩家的连接。它是逃生舱：让每个「就发个
 *     包」的需求不必再动桥。
 *   - api_spawn_particle_for / api_player_send_title：同一条投递路径的类型化
 *     派生。包在 C++ 侧构造（版本安全：没有线格式跨 FFI），复用同一个投递
 *     助手。
 *
 * api_player_send_title 存在的理由：老的标题路（player_action 的
 * PACT_SET_TITLE）曾 shell 出去跑 `/title "<name>" title <text>`，名字带引
 * 号就碎、文本里的选择器会被展开、也定不了时长。
 *
 * 刻意**不**暴露广播变体（Level 本来就会广播；模组真要「所有人」时自己循
 * 环玩家）。
 *
 * 读改既有的包是另一半故事，住在 PacketHooks —— 本文件只制造新包。
 */
#ifndef PIER_BUILD_CLIENT

#include <memory>
#include <set>
#include <string>
#include <string_view>

#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/core/utility/ReadOnlyBinaryStream.h"
#include "mc/network/MinecraftPacketIds.h"
#include "mc/network/MinecraftPackets.h"
#include "mc/network/Packet.h"
#include "mc/network/packet/SetTitlePacket.h"
#include "mc/network/packet/SetTitlePacketPayload.h"
#include "mc/network/packet/SpawnParticleEffectPacket.h"
#include "mc/world/actor/player/Player.h"

#include "sdk/abi.h"

#include "pier/api/bridge.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/log.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        /// 共享投递：解析目标、把现成的包交给那一个连接。裸入口和类型化入口
        /// 都终结在这里。
        bool sendToPlayer(PierPlayerSel sel, Packet& pkt)
        {
            Player* p = bridge::resolvePlayer(sel);
            if (!p) return false;
            p->sendNetworkPacket(pkt);
            return true;
        }

        bool api_send_packet(PierPlayerSel sel, int32_t packetId, uint8_t const* body, size_t bodyLen)
        {
            PIER_API_GUARD_BEGIN
                if (!body && bodyLen != 0) return false;

                auto pkt = MinecraftPackets::createPacket(static_cast<MinecraftPacketIds>(packetId));
                if (!pkt) return false;

                // 把调用方给的包体反序列化进包对象。流**借用**字节
                //（copyBuffer=false）—— 本帧内有效。
                std::string_view raw{reinterpret_cast<char const*>(body), bodyLen};
                ReadOnlyBinaryStream stream{raw, /*copyBuffer=*/false};
                if (!pkt->read(stream)) return false;
                // 包体必须**恰好**是一个包：有尾随垃圾意味着调用方按错的形状
                // 序列化了这个游戏版本 —— 早点拒绝，别把半解析的包发给客户端。
                if (!stream.ensureReadCompleted()) return false;

                return sendToPlayer(sel, *pkt);
            PIER_API_GUARD_END
        }

        bool api_player_send_title(
            PierPlayerSel sel, int32_t type, PierStr text, int32_t fadeInTicks, int32_t stayTicks,
            int32_t fadeOutTicks)
        {
            PIER_API_GUARD_BEGIN
                using TitleType = SetTitlePacketPayload::TitleType;

                // 6..8 是 TextObject 变体；它们的载荷构造函数要
                // ResolvedTextObject，那东西跨这道 FFI 没有意义。拒绝而不是静
                // 默降级成纯字符串变体 —— 调用方要的和它会拿到的不是一个东西。
                if (type < 0 || type > 5) return false;
                auto const kind = static_cast<TitleType>(type);

                // 三个时长要么全给要么全不给。混着给没有任何站得住的解读：
                // 「淡入 5 tick、停留时长看客户端手头是啥」是调用点的 bug，不
                // 是请求。
                int const specified =
                    (fadeInTicks >= 0 ? 1 : 0) + (stayTicks >= 0 ? 1 : 0) + (fadeOutTicks >= 0 ? 1 : 0);
                if (specified != 0 && specified != 3) return false;
                bool const withTimes = (specified == 3);

                Player* p = bridge::resolvePlayer(sel);
                if (!p) return false;

                // 时长（若要）先发、单独一个包 —— `/title <who> times a b c`
                // 发的就是它，客户端把它应用到**之后**到达的标题上。只把时长
                // 塞进内容包在有些版本上行、有些版本上不行；发 Times 包是原版
                // 自己依赖的行为。
                if (withTimes)
                {
                    SetTitlePacket times;
                    times.mType = TitleType::Times;
                    times.mFadeInTime = fadeInTicks;
                    times.mStayTime = stayTicks;
                    times.mFadeOutTime = fadeOutTicks;
                    p->sendNetworkPacket(times);
                    // type == 5 表示调用方只想改时长。
                    if (kind == TitleType::Times) return true;
                }
                else if (kind == TitleType::Times)
                {
                    // 不带时长的 Times 是个空请求，不是合法的包。
                    return false;
                }

                // ll::PayloadPacket<T> 派生自 T（mc/network/Packet.h:204），载荷
                // 字段直接躺在包上 —— 和上面 SpawnParticleEffectPacket 同一个访
                // 问模式。不涉及线格式。
                SetTitlePacket pkt;
                pkt.mType = kind;
                if (kind == TitleType::Title || kind == TitleType::Subtitle
                    || kind == TitleType::Actionbar)
                {
                    pkt.mTitleText = toString(text);
                }
                if (withTimes)
                {
                    pkt.mFadeInTime = fadeInTicks;
                    pkt.mStayTime = stayTicks;
                    pkt.mFadeOutTime = fadeOutTicks;
                }
                return sendToPlayer(sel, pkt);
            PIER_API_GUARD_END
        }

        bool api_spawn_particle_for(
            PierPlayerSel sel, int32_t dimension, PierStr effectName, double x, double y, double z)
        {
            PIER_API_GUARD_BEGIN
                // 类型化构造：MCAPI 默认构造把包初始化好（序列化模式）、载荷给
                // 默认值（mActorId = 无效，mMolangVariables = nullopt）；这里只
                // 填要紧的三个字段。不涉及线格式 —— 版本升了也不用像
                // api_send_packet 的调用方那样自己跟。
                if (dimension < 0 || dimension > 255)
                {
                    static std::set<int32_t> warned;
                    if (warned.insert(dimension).second)
                    {
                        hostLogger().warn(
                            "spawn_particle_for: 维度 {} 装不进 SpawnParticleEffectPacket 的单字节维度号，"
                            "会被截断成 {}。自定义维度里的定向粒子可能不显示，改用 spawn_particle。",
                            dimension, static_cast<int>(static_cast<uchar>(dimension)));
                    }
                }
                SpawnParticleEffectPacket pkt;
                pkt.mVanillaDimensionId = static_cast<uchar>(dimension);
                pkt.mPos = Vec3{(float)x, (float)y, (float)z};
                pkt.mEffectName = toString(effectName);
                return sendToPlayer(sel, pkt);
            PIER_API_GUARD_END
        }

        void fill(PierApi& api)
        {
            api.send_packet = &api_send_packet;
            api.player_send_title = &api_player_send_title;
            api.spawn_particle_for = &api_spawn_particle_for;
        }

        spi::SlotPackReg reg{{"packets", &fill}};
    } // namespace
} // namespace pier::api_impl

#endif // !PIER_BUILD_CLIENT
