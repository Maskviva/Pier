/** actors/SimPlayer.cpp: simulated players.
 * Two main ABI entry points. sim_spawn(name, dim, x, y, z) goes through
 * SimulatedPlayer::create and produces a real ServerPlayer under that name, so every
 * existing per-player API reaches it through the ordinary name selector. sim_do(sel,
 * action, args_snbt) dispatches verbs over the simulate* family. A new verb is added
 * on this side and does not grow the ABI table, since the verb list is data and not
 * layout. Actor::isSimulatedPlayer() gates it, a real player is never driven, and
 * args is SNBT.
 *
 * Verbs, with parameters in braces and defaults after '=':
 *   despawn | stop | jump | attack | interact | use_item | drop | respawn
 *   move_to{x,y,z,speed=1,face_target=1}   navigate_to{x,y,z,speed=4.3}
 *   look_at{x,y,z}   interact_block{x,y,z,face=1}   chat{msg}
 *   destroy_block{x,y,z,face=1} | destroy_look{hand=5.5} | stop_destroy
 *   sneak{on=1} | fly{on=1}
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
            int f = static_cast<int>(argD(t, "face", 1)); // Defaults to Up
            if (f < 0 || f > 5) f = 1;
            return static_cast<::ScriptModuleMinecraft::ScriptFacing>(f);
        }

        bool api_sim_spawn(PierStr name, int32_t dimension, double x, double y, double z)
        {
            PIER_API_GUARD_BEGIN
                if (!bridge::levelReady()) return false;
                // The target dimension must be buildable through the dimension
                // bridge, as in player_teleport.
                if (!bridge::blockSourceOf(dimension)) return false;
                // A name already held by an online player is refused. Every slot that
                // addresses by name would hit one or the other at random, and the
                // `chat` verb could speak under a real person's name.
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
                // Reuses the existing player enumeration primitive and filters for
                // bots. Only names are emitted rather than full summaries, because the
                // caller rebuilds a SimPlayer handle from a name and reaches the whole
                // player interface through it.
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
                if (!p || !p->isSimulatedPlayer()) return false; // A real player is never driven
                auto* sim = static_cast<SimulatedPlayer*>(p);

                // Arguments are parsed once. Both "" and "{}" mean no arguments.
                CompoundTag args;
                std::string_view raw = sv(argsSnbt);
                if (!raw.empty())
                {
                    auto parsed = CompoundTag::fromSnbt(raw);
                    if (!parsed) return false; // Malformed arguments are refused, not guessed
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
                    // Three overloads exist: (Vec3&), (Vec3&, LookDuration) and one
                    // taking a BlockPos. A bare Vec3 is ambiguous between the first
                    // two. Passing LookDuration explicitly pins the Vec3 overload, and
                    // Instant carries the same turn-immediately meaning as the version
                    // without a duration.
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

                return false; // Unknown verb
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
