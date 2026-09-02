/** player/SlotChangeEvent.cpp: the player changed the slot they hold.
 * Observation only, not cancellable: setSelectedSlot returns a reference to the item in
 * the new slot, and cancelling would mean inventing an ItemStack reference out of
 * nothing, for which there is no correct answer. Pinning the held item means setting the
 * slot back afterwards through PIER_PACT_SET_SELECTED_SLOT.
 * Uses: abilities and menus triggered by a held item, anti-cheat detection of repeated
 * slot switches within one tick, and recording which tool a player actually mined
 * with. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/actor/player/Player.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/item/ItemStackBase.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& slotDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            PlayerSetSelectedSlotHook,
            ll::memory::HookPriority::Normal,
            Player,
            &Player::setSelectedSlot,
            ::ItemStack const&,
            int slot)
        {
            auto& def = slotDef();
            if (!def.live()) return origin(slot);

            int from = -1;
            try
            {
                from = this->getSelectedItemSlot();
            }
            catch (...)
            {
                from = -1;
            }

            // The engine completes the change before this reports, so the item in the
            // payload is what the player now holds and a consumer need not look it up.
            ::ItemStack const& held = origin(slot);

            if (from == slot) return held; // Nothing really changed; a client resends the same slot

            std::string item;
            try
            {
                if (!held.isNull()) item = held.getTypeName();
            }
            catch (...)
            {
                item.clear();
            }

            std::string snbt = "{\"eventId\":\"PlayerChangeSlotEvent\""
                ",\"from\":" + snbtNum(from)
                + ",\"to\":" + snbtNum(slot)
                + ",\"item\":\"" + snbtEscape(item) + "\""
                + ",\"dim\":" + snbtNum(static_cast<int>(this->getDimensionId()))
                + "," + playerRefSnbt(*this) + "}";
            dispatchHookEvent(def, snbt);

            return held;
        }

        HookEventDef gDef{
            "PlayerChangeSlotEvent",
            []
            {
                int const r = PlayerSetSelectedSlotHook::hook();
                if (r != 0)
                {
                    hostLogger().error(
                        "[hooks/SlotChangeEvent] the Player::setSelectedSlot detour failed to install with code={}", r);
                }
                return r == 0;
            }
        };
        HookEventDef& slotDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
