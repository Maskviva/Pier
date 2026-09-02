/** world/LiquidFlowEvent.cpp: water or lava spreading into one cell.
 * In a plot setting this is the only interception point for a neighbor pouring water into
 * a resident's ground: the pour happens on the neighbor's own plot where PlayerPlaceBlock
 * stop it, and the crossing is the spreading step. The event carries the target cell and
 * the source cell, and a consumer judges permission on the target.
 * A hot path: liquid spreads every tick, so the test must stay cheap and a consumer
 * throttles for itself when needed. No throttle cache is built in as it is for pressure
 * plates, because which cell changes far faster than which player and the hit rate would
 * be too low to be worth it. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/LiquidBlock.h"
#include "mc/world/level/block/BlockType.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& liquidFlowDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            LiquidTrySpreadToHook,
            ll::memory::HookPriority::High, // Outside the dimension rules
            LiquidBlock,
            &LiquidBlock::_trySpreadTo,
            void,
            ::BlockSource& region,
            ::BlockPos const& pos,
            int neighbor,
            ::BlockPos const& flowFromPos,
            uchar flowFromDirection)
        {
            auto& def = liquidFlowDef();
            if (!def.live())
            {
                return origin(region, pos, neighbor, flowFromPos, flowFromDirection);
            }

            int dim = -1;
            std::string liquid;
            try
            {
                dim = static_cast<int>(region.getDimensionId());
                liquid = this->getTypeName();
            }
            catch (...)
            {
                liquid.clear();
            }

            std::string snbt = "{\"eventId\":\"LiquidFlowEvent\""
                ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"dim\":" + snbtNum(dim)
                + ",\"fromX\":" + snbtNum(flowFromPos.x)
                + ",\"fromY\":" + snbtNum(flowFromPos.y)
                + ",\"fromZ\":" + snbtNum(flowFromPos.z)
                + ",\"direction\":" + snbtNum(static_cast<int>(flowFromDirection))
                + ",\"liquid\":\"" + snbtEscape(liquid) + "\"}";

            if (dispatchHookEventCancellable(def, snbt)) return;
            origin(region, pos, neighbor, flowFromPos, flowFromDirection);
        }

        HookEventDef gDef{
            "LiquidFlowEvent",
            []
            {
                int const r = LiquidTrySpreadToHook::hook();
                if (r != 0)
                {
                    hostLogger().error(
                        "[hooks/LiquidFlowEvent] the LiquidBlock::_trySpreadTo detour failed to install with code={}", r);
                }
                return r == 0;
            }
        };
        HookEventDef& liquidFlowDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
