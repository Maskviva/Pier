/** hooks/world/DimensionEvents.cpp: the synthetic "PlayerChangeDimensionEvent".
 * Without it a mod can only poll the dimension and compare, which misses every transfer
 * it did not start itself: a portal, another mod's teleport, /execute in. Features
 * conditioned on a player changing world, such as per-world inventories or per-world game
 * modes, then fail silently in exactly those cases.
 * Level::requestPlayerChangeDimension is the single funnel for every transfer and the
 * ChangeDimensionRequest carries both the source and the target dimension, so one hook
 * covers all of it. The event dispatches before origin, while the player is still in the
 * old dimension, which is what makes saving the inventory of the world being left work;
 * after origin the engine may already have swapped it. Observation only, the transfer is
 * not cancelled. */
#include "pier/hooks/hook_events.h"

#include <string>
#include <utility>

#include "ll/api/memory/Hook.h"

#include "mc/world/actor/player/Player.h"
#include "mc/world/level/ChangeDimensionRequest.h"
#include "mc/world/level/Level.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& changeDimDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            PlayerChangeDimensionHook,
            ll::memory::HookPriority::Normal,
            Level,
            &Level::$requestPlayerChangeDimension,
            void,
            ::Player& player,
            ::ChangeDimensionRequest&& changeRequest)
        {
            auto& def = changeDimDef();
            if (!def.live())
            {
                return origin(player, std::move(changeRequest));
            }

            // The request is read before forwarding: origin() takes it by rvalue reference
            // and is free to empty it.
            int const from = changeRequest.mFromDimensionId->value();
            int const to = changeRequest.mToDimensionId->value();

            std::string snbt = "{\"eventId\":\"PlayerChangeDimensionEvent\""
                ",\"from\":" + snbtNum(from)
                + ",\"to\":" + snbtNum(to)
                + "," + playerRefSnbt(player) + "}";
            dispatchHookEvent(def, snbt); // Before origin, see the file header

            return origin(player, std::move(changeRequest));
        }

        HookEventDef gDef{"PlayerChangeDimensionEvent", [] { return PlayerChangeDimensionHook::hook() == 0; }};
        HookEventDef& changeDimDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
