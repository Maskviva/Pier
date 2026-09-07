/** hooks/world/DimensionEvents.cpp: the synthetic "PlayerChangeDimensionEvent".
 * Without it a mod can only poll the dimension and compare, which misses every transfer it
 * did not start itself: a portal, another mod's teleport, /execute in. Features conditioned
 * on a player changing world then fail silently in exactly those cases.
 * Level::requestPlayerChangeDimension is the single funnel for every transfer and the
 * ChangeDimensionRequest carries source, target and whether a portal caused it, so one hook
 * covers all of it. The event dispatches before origin, while the player is still in the old
 * dimension, which is what makes saving the inventory of the world being left work.
 * Cancellable and redirectable. Cancelling skips origin and drops the request, safe because
 * nothing is mutated yet. Redirecting rewrites mToDimensionId and mToLocation before origin,
 * so the engine runs the transfer itself; a subscriber must not teleport from its callback
 * instead, which re-enters here while the player still stands in the portal.
 * Payload {eventId, from, to, to_x, to_y, to_z, use_portal, respawn, _player:{...}}.
 * ll::TypedStorage of a scalar is that scalar, through the partial specialization in
 * ll/api/base/Alias.h, so mUsePortal is a plain bool while mToLocation is wrapped and needs
 * get(). Reaching for get() uniformly does not compile. */
#include "pier/hooks/hook_events.h"

#include <string>
#include <utility>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/ChangeDimensionRequest.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& changeDimDef(); // Forward declaration

        /** A subscriber may still teleport in response, and that funnels back through
         *  requestPlayerChangeDimension. Re-entry passes straight through. */
        bool gDispatching = false;

        std::string buildSnbt(::Player& player, ::ChangeDimensionRequest const& req)
        {
            ::Vec3 const& to = req.mToLocation.get();
            return "{\"eventId\":\"PlayerChangeDimensionEvent\""
                ",\"from\":" + snbtNum(req.mFromDimensionId->mValue)
                + ",\"to\":" + snbtNum(req.mToDimensionId->mValue)
                + ",\"to_x\":" + snbtDouble(to.x)
                + ",\"to_y\":" + snbtDouble(to.y)
                + ",\"to_z\":" + snbtDouble(to.z)
                + ",\"use_portal\":" + snbtNum(req.mUsePortal ? 1 : 0)
                + ",\"respawn\":" + snbtNum(req.mRespawn ? 1 : 0)
                + "," + playerRefSnbt(player) + "}";
        }

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
            if (!def.live() || gDispatching)
            {
                return origin(player, std::move(changeRequest));
            }

            // The request is read before forwarding: origin() takes it by rvalue
            // reference and is free to empty it.
            std::string const snbt = buildSnbt(player, changeRequest);

            Decision decision;
            {
                gDispatching = true;
                decision = dispatchHookEventDecided(def, snbt);
                gDispatching = false;
            }

            if (decision.cancelled) return;

            if (decision.redirect.hasDimension)
            {
                changeRequest.mToDimensionId.get() = ::DimensionType{decision.redirect.dimension};
            }
            if (decision.redirect.hasPosition)
            {
                changeRequest.mToLocation.get() =
                    ::Vec3{decision.redirect.x, decision.redirect.y, decision.redirect.z};
            }
            return origin(player, std::move(changeRequest));
        }

        HookEventDef gDef{"PlayerChangeDimensionEvent", [] { return PlayerChangeDimensionHook::hook() == 0; }};
        HookEventDef& changeDimDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
