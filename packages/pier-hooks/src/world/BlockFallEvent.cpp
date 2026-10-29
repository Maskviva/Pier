/** hooks/world/BlockFallEvent.cpp: the synthetic, cancellable "BlockFallEvent".
 * Sand, gravel and anvils fall when the block under them goes, and the block that goes may be a
 * neighbor's, so a column on one plot can be brought down by digging on the next, or a falling
 * block can land in a plot from outside it. FallingBlock::startFalling is the engine's single
 * place where a block that has lost its support turns into a falling block, so the hook asks
 * right before. Returning without calling origin leaves the block hanging where it is.
 * Payload {eventId, x, y, z, dim, block, creative}: the block that would fall, and whether a
 * creative player's action started it.
 * A field that could not be read leaves the payload with `partial` 1. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/FallingBlock.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& fallDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            FallingBlockStartHook,
            ll::memory::HookPriority::Normal,
            FallingBlock,
            &FallingBlock::$startFalling,
            void,
            ::BlockSource& region,
            ::BlockPos const& pos,
            ::Block const& oldBlock,
            bool creative)
        {
            auto& def = fallDef();
            if (!def.live()) return origin(region, pos, oldBlock, creative);

            std::string name;
            bool readFailed = false;
            try
            {
                name = oldBlock.getTypeName();
            }
            catch (...)
            {
                name.clear();
                readFailed = true;
            }
            std::string const snbt = "{\"eventId\":\"BlockFallEvent\""
                ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"dim\":" + snbtNum(static_cast<int>(region.getDimensionId()))
                + ",\"block\":\"" + snbtEscape(name) + "\""
                + ",\"creative\":" + (creative ? "1" : "0")
                + (readFailed ? ",\"partial\":1" : "") + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return;
            }
            origin(region, pos, oldBlock, creative);
        }

        HookEventDef gDef{"BlockFallEvent", [] { return FallingBlockStartHook::hook() == 0; }};
        HookEventDef& fallDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
