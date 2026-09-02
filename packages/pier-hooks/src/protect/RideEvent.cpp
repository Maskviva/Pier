/** hooks/protect/RideEvent.cpp: the synthetic, cancellable "PlayerRideEvent".
 * The vanilla ActorStartRidingEvent is a notification fired after mounting and cannot
 * refuse. The hook point is Actor::canAddPassenger and not Actor::startRiding: the former
 * is the vehicle's own veto and answering no is an outcome every caller already handles,
 * while refusing inside startRiding lies to a function that has already decided the mount
 * happens.
 * The roles are reversed accordingly: this is the vehicle and the argument is the rider.
 * Swapping them checks the boat's permission instead of the player's and fails only at a
 * plot boundary. x, y and z are the vehicle position, because the question is whether
 * this person may use that vehicle.
 * Payload {eventId, x, y, z, dim, vehicle, _player:{...}}, where vehicle is the actor
 * type name. */
#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/player/Player.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& rideDef();      // Forward: a player mounts
        HookEventDef& actorRideDef(); // Forward: a non-player actor mounts

        LL_TYPE_INSTANCE_HOOK(
            PlayerRideHook,
            ll::memory::HookPriority::Normal,
            Actor,
            &Actor::$canAddPassenger,
            bool,
            ::Actor& passenger)
        {
            auto& playerDef = rideDef();
            auto& actorDef = actorRideDef();

            bool const passengerIsPlayer = passenger.isPlayer();
            auto& def = passengerIsPlayer ? playerDef : actorDef;
            if (!def.live())
            {
                return origin(passenger);
            }

            std::string vehicleName;
            std::string passengerName;
            try
            {
                vehicleName = this->getTypeName();
                passengerName = passenger.getTypeName();
            }
            catch (...)
            {
                vehicleName.clear();
                passengerName.clear();
            }

            auto const& pos = this->getPosition();
            std::string snbt = passengerIsPlayer
                ? std::string{"{\"eventId\":\"PlayerRideEvent\""}
                : std::string{"{\"eventId\":\"ActorRideEvent\""};
            snbt += ",\"x\":" + snbtNum(static_cast<int>(pos.x))
                + ",\"y\":" + snbtNum(static_cast<int>(pos.y))
                + ",\"z\":" + snbtNum(static_cast<int>(pos.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(this->getDimensionId()))
                + ",\"vehicle\":\"" + snbtEscape(vehicleName) + "\""
                + ",\"vehicleId\":" + snbtNum(static_cast<int64_t>(this->getOrCreateUniqueID().rawID)) + "L";
            if (passengerIsPlayer)
            {
                snbt += "," + playerRefSnbt(*static_cast<Player*>(&passenger));
            }
            else
            {
                // A non-player passenger, such as a villager in a boat or a pig in a
                // minecart. The payload carries the type and the id and fabricates no
                // _player, so a consumer tells the two paths apart by its presence.
                snbt += ",\"passenger\":\"" + snbtEscape(passengerName) + "\""
                    + ",\"passengerId\":"
                    + snbtNum(static_cast<int64_t>(passenger.getOrCreateUniqueID().rawID)) + "L";
            }
            snbt += "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return false; // The vehicle declines this passenger
            }
            return origin(passenger);
        }

        HookEventDef gDef{"PlayerRideEvent", [] { return PlayerRideHook::hook() == 0; }};
        HookEventDef& rideDef() { return gDef; }

        // One detour serves two event ids: whichever is subscribed first installs the
        // hook, and hook() in LL is idempotent so the second call returns 0.
        HookEventDef gActorDef{"ActorRideEvent", [] { return PlayerRideHook::hook() == 0; }};
        HookEventDef& actorRideDef() { return gActorDef; }

        HookEventRegistrar gReg{gDef};
        HookEventRegistrar gActorReg{gActorDef};
    } // namespace
} // namespace pier::hooks
