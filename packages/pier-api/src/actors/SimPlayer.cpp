/** actors/SimPlayer.cpp —— 模拟（假）玩家。
 *
 * 只有两个主 ABI 入口：
 *   - sim_spawn(name, dim, x, y, z)：SimulatedPlayer::create（LeviLamina 的
 *     便捷封装）。产物是一个顶着这个名字的真 ServerPlayer，所以**每个**已
 *     有的按玩家 API（传送、血量、背包、踢出、发消息、位置、…）都经普通
 *     的名字选择器对它生效 —— 不需要复制一套接口面。
 *   - sim_do(sel, action, args_snbt)：simulate* 家族上的多路动词派发器。
 *     新动词加在**这里**、桥的这一侧，不涨 ABI 表 —— 动作词表是数据，不是
 *     布局。用 Actor::isSimulatedPlayer() 把门，真玩家永远不可能被操纵。
 *     args 是 SNBT（无参动词给 {} 或 ""）。
 *
 * 动词（花括号里是参数，'=' 后是默认值）：
 *   despawn | stop | jump | attack | interact | use_item | drop | respawn
 *   move_to{x,y,z,speed=1,face_target=1}      直线移动
 *   navigate_to{x,y,z,speed=4.3}              寻路移动
 *   look_at{x,y,z}
 *   destroy_block{x,y,z,face=1} | destroy_look{hand=5.5} | stop_destroy
 *   interact_block{x,y,z,face=1}
 *   sneak{on=1} | fly{on=1}
 *   chat{msg}
 */
#ifndef PIER_BUILD_CLIENT

#include <cmath>
#include <string>
#include <string_view>
#include <utility>

#include "mc/deps/core/math/Vec2.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/scripting/modules/gametest/ScriptNavigationResult.h"
#include "mc/scripting/modules/minecraft/ScriptFacing.h"
#include "mc/server/SimulatedPlayer.h"
#include "mc/server/sim/LookDuration.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/Level.h"

#include "sdk/abi.h"

#include "pier/api/bridge.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        double argD(CompoundTag const& t, std::string_view key, double def)
        {
            if (!t.contains(key)) return def;
            return bridge::nbtToDouble(t.at(key), def);
        }

        bool argB(CompoundTag const& t, std::string_view key, bool def)
        {
            if (!t.contains(key)) return def;
            return bridge::nbtToDouble(t.at(key), def != 0.0) != 0.0;
        }

        std::string argS(CompoundTag const& t, std::string_view key)
        {
            if (!t.contains(key)) return {};
            return std::string{static_cast<std::string_view>(t.at(key))};
        }

        BlockPos argBlockPos(CompoundTag const& t)
        {
            return BlockPos{
                static_cast<int>(std::floor(argD(t, "x", 0))),
                static_cast<int>(std::floor(argD(t, "y", 0))),
                static_cast<int>(std::floor(argD(t, "z", 0))),
            };
        }

        ::ScriptModuleMinecraft::ScriptFacing argFace(CompoundTag const& t)
        {
            int f = static_cast<int>(argD(t, "face", 1)); // 默认 Up
            if (f < 0 || f > 5) f = 1;
            return static_cast<::ScriptModuleMinecraft::ScriptFacing>(f);
        }

        bool api_sim_spawn(PierStr name, int32_t dimension, double x, double y, double z)
        {
            PIER_API_GUARD_BEGIN
                if (!bridge::levelReady()) return false;
                // V-15：目标维度必须能经维度桥建出（同 player_teleport）。
                if (!bridge::blockSourceOf(dimension)) return false;
                // V-30：不允许和在线玩家同名 —— 所有按名字寻址的槽都会在两者之间
                // 随机命中，`chat` 动词还能以真人的名字发言。
                if (bridge::resolvePlayer(PierPlayerSel{0, name}) != nullptr) return false;
                auto sp = SimulatedPlayer::create(
                    toString(name),
                    Vec3{(float)x, (float)y, (float)z},
                    DimensionType{dimension}
                );
                return static_cast<bool>(sp);
            PIER_API_GUARD_END
        }

        bool api_sim_is(PierPlayerSel sel)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                return p && p->isSimulatedPlayer();
            PIER_API_GUARD_END
        }

        void api_sim_list(void* ctx, PierStrSink nameSink)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level || !nameSink) return;
                // 复用既有的玩家枚举原语，过滤出机器人。只吐名字（不吐完整摘
                // 要）：调用方拿名字重建一个 SimPlayer 句柄，再经它够到完整的
                // 玩家接口面。
                level->forEachPlayer([&](Player& p)
                {
                    if (p.isSimulatedPlayer()) nameSink(ctx, ps(p.getRealName()));
                    return true;
                });
            PIER_API_GUARD_END_VOID
        }

        bool api_sim_do(PierPlayerSel sel, PierStr action, PierStr argsSnbt)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p || !p->isSimulatedPlayer()) return false; // 真玩家永不被操纵
                auto* sim = static_cast<SimulatedPlayer*>(p);

                // 参数只解析一次；"" 和 "{}" 都表示「无参」。
                CompoundTag args;
                std::string_view raw = sv(argsSnbt);
                if (!raw.empty())
                {
                    auto parsed = CompoundTag::fromSnbt(raw);
                    if (!parsed) return false; // 畸形参数：拒绝，不猜
                    args = std::move(*parsed);
                }

                std::string_view verb = sv(action);

                if (verb == "despawn")
                {
                    sim->simulateDisconnect();
                    return true;
                }
                if (verb == "stop")
                {
                    sim->simulateStopMoving();
                    sim->simulateStopUsingItem();
                    sim->simulateStopBuild();
                    sim->simulateStopInteracting();
                    sim->simulateStopDestroyingBlock();
                    return true;
                }
                if (verb == "jump") return sim->simulateJump();
                if (verb == "attack") return sim->simulateAttack();
                if (verb == "interact") return sim->simulateInteract();
                if (verb == "use_item") return sim->simulateUseItem();
                if (verb == "drop") return sim->simulateDropSelectedItem();
                if (verb == "respawn") return sim->simulateRespawn();
                if (verb == "move_to")
                {
                    Vec3 pos{
                        (float)argD(args, "x", 0),
                        (float)argD(args, "y", 0),
                        (float)argD(args, "z", 0)
                    };
                    sim->simulateMoveToLocation(
                        pos, (float)argD(args, "speed", 1.0), argB(args, "face_target", true));
                    return true;
                }
                if (verb == "navigate_to")
                {
                    Vec3 pos{
                        (float)argD(args, "x", 0),
                        (float)argD(args, "y", 0),
                        (float)argD(args, "z", 0)
                    };
                    static_cast<void>(
                        sim->simulateNavigateToLocation(pos, (float)argD(args, "speed", 4.3)));
                    return true;
                }
                if (verb == "look_at")
                {
                    // 存在三个重载（(Vec3&)、(Vec3&, LookDuration)、还有一个
                    // BlockPos 的）；裸 Vec3 在前两个之间有歧义。显式传
                    // LookDuration 钉住 Vec3 重载 —— Instant 对应无时长版本
                    // 「瞬间转头」的语义。
                    sim->simulateLookAt(
                        Vec3{
                            (float)argD(args, "x", 0),
                            (float)argD(args, "y", 0),
                            (float)argD(args, "z", 0)
                        },
                        ::sim::LookDuration::Instant);
                    return true;
                }
                if (verb == "destroy_block")
                    return sim->simulateDestroyBlock(argBlockPos(args), argFace(args));
                if (verb == "destroy_look")
                    return sim->simulateDestroyLookAt((float)argD(args, "hand", 5.5));
                if (verb == "stop_destroy")
                {
                    sim->simulateStopDestroyingBlock();
                    return true;
                }
                if (verb == "interact_block")
                    return sim->simulateInteract(argBlockPos(args), argFace(args));
                if (verb == "sneak")
                {
                    return argB(args, "on", true) ? sim->simulateSneaking()
                                                  : sim->simulateStopSneaking();
                }
                if (verb == "fly")
                {
                    if (argB(args, "on", true))
                        sim->simulateFly();
                    else
                        sim->simulateStopFlying();
                    return true;
                }
                if (verb == "chat")
                {
                    auto msg = argS(args, "msg");
                    if (msg.empty()) return false;
                    sim->simulateChat(msg);
                    return true;
                }

                return false; // 不认识的动词
            PIER_API_GUARD_END
        }

        void fill(PierApi& api)
        {
            api.sim_spawn = &api_sim_spawn;
            api.sim_is = &api_sim_is;
            api.sim_list = &api_sim_list;
            api.sim_do = &api_sim_do;
        }

        spi::SlotPackReg reg{{"sim-player", &fill}};
    } // namespace
} // namespace pier::api_impl

#endif // !PIER_BUILD_CLIENT
