/** hooks/protect/InteractEntityEvent.cpp: the synthetic, cancellable
 * "PlayerInteractEntityEvent".
 * PlayerUseItemOnEvent covers only using an item on a block and there is no counterpart
 * on the actor side, so everything right-clicking a mob can do is unprotected: shearing,
 * dyeing, milking, leashing, name tags, saddling, opening a horse or llama inventory,
 * villager trading, feeding and breeding. The hook point is
 * Player::interact(Actor&, Vec3 const&), which sits above
 * Actor::interactPreventDefault and therefore also catches interactions handled entirely
 * inside mob component code, where shearing lives. Cancelling returns
 * InteractionResult{false, true}: the interaction did not succeed while the arm still
 * swings, because a refusal that eats the animation too looks like packet loss to a
 * player and they keep clicking.
 * Payload {eventId, x, y, z, dim, target, targetIsPlayer, item, _player:{...}}. x, y and
 * z are the target actor's position, as in RideEvent, and item is the held item type name
 * so a subscriber can separate shears, a lead, dye and food into different actions. */
#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/gamemode/InteractionResult.h"
#include "mc/world/item/ItemStack.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& interactEntityDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            PlayerInteractEntityHook,
            ll::memory::HookPriority::Normal,
            Player,
            &Player::interact,
            ::InteractionResult,
            ::Actor& actor,
            ::Vec3 const& location)
        {
            auto& def = interactEntityDef();
            if (!def.live())
            {
                return origin(actor, location);
            }

            // These two name lookups throw while an actor is being destroyed, and an
            // exception crossing a detour takes the whole server down, so they are caught
            // here.
            std::string targetName;
            bool targetIsPlayer = false;
            std::string itemName;
            try
            {
                targetName = actor.getTypeName();
                targetIsPlayer = actor.isPlayer();
                ::ItemStack const& held = this->getSelectedItem();
                if (!held.isNull()) itemName = held.getTypeName();
            }
            catch (...)
            {
                // A partial failure also counts as unreadable: a subscriber seeing an
                // empty string falls back to a coarser action.
            }

            auto const& pos = actor.getPosition();
            std::string snbt = "{\"eventId\":\"PlayerInteractEntityEvent\""
                ",\"x\":" + snbtNum(static_cast<int>(pos.x))
                + ",\"y\":" + snbtNum(static_cast<int>(pos.y))
                + ",\"z\":" + snbtNum(static_cast<int>(pos.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(actor.getDimensionId()))
                + ",\"targetIsPlayer\":" + (targetIsPlayer ? "1" : "0")
                + ",\"target\":\"" + snbtEscape(targetName)
                + "\",\"item\":\"" + snbtEscape(itemName)
                + "\"," + playerRefSnbt(*this) + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                ::InteractionResult refused{};
                refused.mSuccess = false;
                refused.mSwing = true; // Keep the swing animation, see the file header
                return refused;
            }
            return origin(actor, location);
        }

        HookEventDef gDef{"PlayerInteractEntityEvent", [] { return PlayerInteractEntityHook::hook() == 0; }};
        HookEventDef& interactEntityDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
