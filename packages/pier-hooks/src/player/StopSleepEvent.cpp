/** player/StopSleepEvent.cpp: the synthetic "PlayerStopSleepEvent", observation only.
 * PlayerSleepEvent covers going to bed and nothing covered getting up, which a game that
 * counts sleepers, or a mod that restores state at wake-up, needs. Player::stopSleepInBed is
 * the engine's one exit from a bed. It cannot be cancelled: the engine has already decided
 * to leave, and refusing here would leave the player lying in a bed the engine considers
 * empty. The event dispatches before origin, so a callback still finds the player in bed.
 * Payload {eventId, x, y, z, dim, forceful, updateLevelList, _player}. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/actor/player/Player.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& stopSleepDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            PlayerStopSleepHook,
            ll::memory::HookPriority::Normal,
            Player,
            &Player::$stopSleepInBed,
            void,
            bool forcefulWakeUp,
            bool updateLevelList)
        {
            auto& def = stopSleepDef();
            if (!def.live()) return origin(forcefulWakeUp, updateLevelList);

            auto const& pos = this->getPosition();
            std::string const snbt = "{\"eventId\":\"PlayerStopSleepEvent\""
                ",\"x\":" + snbtNum(static_cast<int>(pos.x))
                + ",\"y\":" + snbtNum(static_cast<int>(pos.y))
                + ",\"z\":" + snbtNum(static_cast<int>(pos.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(this->getDimensionId()))
                + ",\"forceful\":" + (forcefulWakeUp ? "1" : "0")
                + ",\"updateLevelList\":" + (updateLevelList ? "1" : "0")
                + "," + playerRefSnbt(*this) + "}";

            dispatchHookEvent(def, snbt);
            origin(forcefulWakeUp, updateLevelList);
        }

        HookEventDef gDef{"PlayerStopSleepEvent", [] { return PlayerStopSleepHook::hook() == 0; }};
        HookEventDef& stopSleepDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
