/** hooks/world/DispenseItemEvent.cpp: the synthetic, cancellable "DispenseItemEvent".
 * A dispenser ejecting an item is nobody's action, so no player event sees it, and it is how
 * a machine built on one plot reaches the next: a dispenser facing a plot boundary can fill
 * or empty a neighbor's container or pen. DispenserBlock::ejectItem is the engine's single
 * place where an item leaves a dispenser for the world, so the hook asks right before.
 * Returning without calling origin leaves the item in the dispenser, which is what a
 * dispenser does when it cannot eject.
 * Payload {eventId, x, y, z, dim, item, count, slot, face}: the block coordinates of the
 * dispenser, the item and its count, the slot it comes from, and the face the dispenser
 * points to as the engine numbers it. There is no player. A field that could not be read
 * leaves the payload with `partial` 1. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <cmath>
#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/Container.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/DispenserBlock.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& dispenseDef(); // Forward declaration

        int cell(float v) { return static_cast<int>(std::floor(v)); }

        // ejectItem has two overloads. The argument list below names the one with a container,
        // which is the one the dispenser calls.
        LL_TYPE_STATIC_HOOK(
            DispenserEjectHook,
            ll::memory::HookPriority::Normal,
            DispenserBlock,
            &DispenserBlock::ejectItem,
            void,
            ::BlockSource& region,
            ::Vec3 const& pos,
            uchar face,
            ::ItemStack const& item,
            ::Container& container,
            int slot,
            int countLimit)
        {
            auto& def = dispenseDef();
            if (!def.live()) return origin(region, pos, face, item, container, slot, countLimit);

            std::string name;
            int count = 0;
            bool readFailed = false;
            try
            {
                if (!item.isNull())
                {
                    name = item.getTypeName();
                    count = item.mCount;
                }
            }
            catch (...)
            {
                name.clear();
                readFailed = true;
            }
            std::string const snbt = "{\"eventId\":\"DispenseItemEvent\""
                ",\"x\":" + snbtNum(cell(pos.x))
                + ",\"y\":" + snbtNum(cell(pos.y))
                + ",\"z\":" + snbtNum(cell(pos.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(region.getDimensionId()))
                + ",\"item\":\"" + snbtEscape(name) + "\""
                + ",\"count\":" + snbtNum(count)
                + ",\"slot\":" + snbtNum(slot)
                + ",\"face\":" + snbtNum(static_cast<int>(face))
                + (readFailed ? ",\"partial\":1" : "") + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return;
            }
            origin(region, pos, face, item, container, slot, countLimit);
        }

        HookEventDef gDef{"DispenseItemEvent", [] { return DispenserEjectHook::hook() == 0; }};
        HookEventDef& dispenseDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
