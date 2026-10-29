/** hooks/world/ContainerChangeEvent.cpp: the synthetic "ContainerChangeEvent", observation only.
 * PlayerOpenContainerEvent is the gate on looking into a chest, and what a shop or a vault needs
 * once the player is looking is each stack that moves in or out of it. LevelContainerModel::
 * onItemChanged is the callback the engine runs for a slot of a container a player has open in
 * the world, so it is the point that sees a change together with the player who made it.
 * The function is virtual, and other models may share its code, so the hook acts only when
 * the object really is a LevelContainerModel, by comparing the vtable. It cannot be cancelled:
 * it is a notification that the slot already holds the new stack.
 * Payload {eventId, x, y, z, dim, slot, oldItem, oldCount, newItem, newCount, _player}: the
 * container block, the slot as the player's screen counts it, and the stacks before and after.
 * `partial` is 1 when an item could not be read. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/actor/player/Player.h"
#include "mc/world/containers/models/LevelContainerModel.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& changeDef(); // Forward declaration

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
            ContainerChangeHook,
            ll::memory::HookPriority::Normal,
            LevelContainerModel,
            &LevelContainerModel::$_onItemChanged,
            void,
            int modelSlot,
            ::ItemStack const& oldItem,
            ::ItemStack const& newItem)
        {
            auto& def = changeDef();
            if (!def.live() || *reinterpret_cast<void***>(this) != LevelContainerModel::$vftable())
            {
                return origin(modelSlot, oldItem, newItem);
            }
            if (this->mPlayer.mContainerManager)
            {
                std::string oldName;
                std::string newName;
                int oldCount = 0;
                int newCount = 0;
                bool readFailed = false;
                readStack(oldItem, oldName, oldCount, readFailed);
                readStack(newItem, newName, newCount, readFailed);
                auto const& pos = this->mBlockPos.get();
                std::string const snbt = "{\"eventId\":\"ContainerChangeEvent\""
                    ",\"x\":" + snbtNum(pos.x)
                    + ",\"y\":" + snbtNum(pos.y)
                    + ",\"z\":" + snbtNum(pos.z)
                    + ",\"dim\":" + snbtNum(static_cast<int>(this->mPlayer.getDimensionId()))
                    + ",\"slot\":" + snbtNum(modelSlot + this->_getContainerOffset())
                    + ",\"oldItem\":\"" + snbtEscape(oldName) + "\""
                    + ",\"oldCount\":" + snbtNum(oldCount)
                    + ",\"newItem\":\"" + snbtEscape(newName) + "\""
                    + ",\"newCount\":" + snbtNum(newCount)
                    + (readFailed ? ",\"partial\":1" : "")
                    + "," + playerRefSnbt(this->mPlayer) + "}";
                dispatchHookEvent(def, snbt);
            }
            origin(modelSlot, oldItem, newItem);
        }

        HookEventDef gDef{"ContainerChangeEvent", [] { return ContainerChangeHook::hook() == 0; }};
        HookEventDef& changeDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
