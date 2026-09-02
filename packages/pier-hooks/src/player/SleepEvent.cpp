/** player/SleepEvent.cpp: a player is about to sleep in a bed.
 * What it can stop: someone else's bed, where skipping the night and setting a spawn
 * point both use another person's ground; game modes such as bed wars where the night
 * must not be skipped; and the nether or the end, where a bed is an explosive trap.
 * Cancelling uses the engine's own `BedSleepingResult::NotPossibleHere`, so the client
 * shows the vanilla message that sleeping is not possible here, a mod sends no message of
 * its own, and the player is not left half asleep. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/actor/player/BedSleepingResult.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& sleepDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            PlayerStartSleepHook,
            ll::memory::HookPriority::Normal,
            Player,
            &Player::$startSleepInBed,
            ::BedSleepingResult,
            ::BlockPos const& bedBlockPos)
        {
            auto& def = sleepDef();
            if (!def.live()) return origin(bedBlockPos);

            std::string snbt = "{\"eventId\":\"PlayerSleepEvent\""
                ",\"x\":" + snbtNum(bedBlockPos.x)
                + ",\"y\":" + snbtNum(bedBlockPos.y)
                + ",\"z\":" + snbtNum(bedBlockPos.z)
                + ",\"dim\":" + snbtNum(static_cast<int>(this->getDimensionId()))
                + "," + playerRefSnbt(*this) + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return ::BedSleepingResult::NotPossibleHere;
            }
            return origin(bedBlockPos);
        }

        HookEventDef gDef{
            "PlayerSleepEvent",
            []
            {
                int const r = PlayerStartSleepHook::hook();
                if (r != 0)
                {
                    hostLogger().error(
                        "[hooks/SleepEvent] the Player::$startSleepInBed detour failed to install with code={}", r);
                }
                return r == 0;
            }
        };
        HookEventDef& sleepDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
