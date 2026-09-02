/** hooks/world/HopperEvents.cpp: "HopperTransferEvent", fired on every slot write of a
 * hopper through HopperBlockActor::setItem, which is every moment an item enters or
 * leaves one. The payload carries the stack before and after and a subscriber computes
 * the difference itself, where an increase means an inflow. There is no dimension field,
 * because the scope of setItem holds no BlockSource, and an observer keys on the position
 * it registered. The lifetime rules are in hook_events.h. */
#include "pier/hooks/hook_events.h"

#include <atomic>
#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/Container.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/block/actor/BlockActorType.h"
#include "mc/world/level/block/actor/HopperBlockActor.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& hopperDef(); // Forward declaration, used by the hook body

        LL_TYPE_INSTANCE_HOOK(
            HopperSetItemHook,
            ll::memory::HookPriority::Normal,
            HopperBlockActor,
            &HopperBlockActor::$setItem,
            void,
            int slot,
            ::ItemStack const& item)
        {
            auto& def = hopperDef();
            if (!def.live())
            {
                origin(slot, item); // Installed but idle
                return;
            }

            // This guard must run before any virtual dispatch through this->. The body of
            // Container::setItem is trivial and the MSVC ICF very likely folds it onto the
            // same address as the identically shaped setItem of a chest or a furnace, so
            // this detour is entered for those block entities too. this is then a
            // ChestBlockActor*, a virtual call through it reads the vptr at the hopper's
            // Container subobject offset, and jumping through that garbage vptr crashes on
            // DEP. getType() is non-virtual, reads BlockActor::mType at offset 0, means the
            // same for every block entity, and stays safe under folding.
            if (this->getType() != ::BlockActorType::Hopper)
            {
                // The discriminator. If the crash really came from ICF folding, this
                // fires for chests and furnaces, which means the guard fixed it. If the
                // this-adjustor did not match, getType() reads garbage, this fires with a
                // meaningless value while the counter receives no events, which means the
                // hook point has to change.
                static std::atomic<bool> logged{false};
                if (!logged.exchange(true))
                {
                    hostLogger().debug(
                        "[hooks/HopperEvents] the guard refused a non-hopper block entity "
                        "(getType={}); that is expected under ICF folding, and a counter that "
                        "stays empty means the hook point has to change",
                        static_cast<int>(this->getType()));
                }
                origin(slot, item);
                return;
            }
            if (slot < 0 || slot >= 5) // A hopper has exactly 5 slots; hardened anyway
            {
                origin(slot, item);
                return;
            }

            // Read the state before the write, let the write happen, then report.
            int oldCount = 0;
            std::string oldName;
            {
                // $getItem is the non-virtual implementation and skips the vtable.
                ItemStack const& prev = this->$getItem(slot);
                oldCount = prev.mCount;
                if (oldCount > 0) oldName = prev.getTypeName();
            }
            origin(slot, item);

            int newCount = item.mCount;
            std::string newName = newCount > 0 ? item.getTypeName() : std::string{};
            BlockPos const& pos = this->getPosition();

            std::string snbt = "{\"eventId\":\"HopperTransferEvent\""
                ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"slot\":" + snbtNum(slot)
                + ",\"item\":\"" + snbtEscape(newName)
                + "\",\"count\":" + snbtNum(newCount)
                + ",\"old_item\":\"" + snbtEscape(oldName)
                + "\",\"old_count\":" + snbtNum(oldCount) + "}";
            dispatchHookEvent(def, snbt);
        }

        HookEventDef gDef{"HopperTransferEvent", [] { return HopperSetItemHook::hook() == 0; }};
        HookEventDef& hopperDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
