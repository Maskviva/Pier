/** player/InventoryChangeEvent.cpp: the synthetic "PlayerInventoryChangeEvent", observation only.
 * PlayerChangeSlotEvent says the held slot moved, which is a different thing: this one says a
 * stack in one of the player's containers changed, whatever caused it, a pick-up, a craft, a
 * command or another mod. Player::inventoryChanged is the engine's notification for it, and it
 * runs after the slot already holds the new stack, so the event cannot be cancelled. Block the
 * cause instead, with PlayerPickUpItemEvent, PlayerDropItemEvent or PlayerRequestItemActionEvent.
 * Payload {eventId, slot, oldItem, oldCount, newItem, newCount, forceBalanced, x, y, z, dim,
 * _player}: the slot number inside the container that changed, and the stacks before and after.
 * `partial` is 1 when an item could not be read. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/Container.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& inventoryChangeDef(); // Forward declaration

        void readStack(::ItemStack const& stack, std::string& name, int& count, bool& readFailed)
        {
            try
            {
                if (!stack.isNull())
                {
                    name = stack.getTypeName();
                    count = stack.mCount;
                }
            }
            catch (...)
            {
                name.clear();
                count = 0;
                readFailed = true;
            }
        }

        LL_TYPE_INSTANCE_HOOK(
            PlayerInventoryChangedHook,
            ll::memory::HookPriority::Normal,
            Player,
            &Player::inventoryChanged,
            void,
            ::Container& container,
            int slot,
            ::ItemStack const& oldItem,
            ::ItemStack const& newItem,
            bool forceBalanced)
        {
            auto& def = inventoryChangeDef();
            if (!def.live()) return origin(container, slot, oldItem, newItem, forceBalanced);

            std::string oldName;
            std::string newName;
            int oldCount = 0;
            int newCount = 0;
            bool readFailed = false;
            readStack(oldItem, oldName, oldCount, readFailed);
            readStack(newItem, newName, newCount, readFailed);
            auto const& at = this->getPosition();
            std::string const snbt = "{\"eventId\":\"PlayerInventoryChangeEvent\""
                ",\"slot\":" + snbtNum(slot)
                + ",\"oldItem\":\"" + snbtEscape(oldName) + "\""
                + ",\"oldCount\":" + snbtNum(oldCount)
                + ",\"newItem\":\"" + snbtEscape(newName) + "\""
                + ",\"newCount\":" + snbtNum(newCount)
                + ",\"forceBalanced\":" + (forceBalanced ? "1" : "0")
                + ",\"x\":" + snbtNum(static_cast<int>(at.x))
                + ",\"y\":" + snbtNum(static_cast<int>(at.y))
                + ",\"z\":" + snbtNum(static_cast<int>(at.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(this->getDimensionId()))
                + (readFailed ? ",\"partial\":1" : "")
                + "," + playerRefSnbt(*this) + "}";

            dispatchHookEvent(def, snbt);
            origin(container, slot, oldItem, newItem, forceBalanced);
        }

        HookEventDef gDef{
            "PlayerInventoryChangeEvent", [] { return PlayerInventoryChangedHook::hook() == 0; }};
        HookEventDef& inventoryChangeDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
