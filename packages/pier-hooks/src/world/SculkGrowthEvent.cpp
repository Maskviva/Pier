/** hooks/world/SculkGrowthEvent.cpp: the synthetic, cancellable "SculkBlockGrowthEvent".
 * Sculk spreads by charge, one block at a time, and it does not stop at a plot's edge, so a
 * patch next door grows into a plot that never asked for it. SculkBlockBehavior::attemptUseCharge
 * is the engine's single place where a charge is spent turning a cell into sculk, so the hook asks
 * right before. Returning 0 is the engine's own answer of a charge that was not used, which
 * leaves the cell as it is. A call with no block source belongs to world generation, and is
 * left to the engine.
 * Payload {eventId, x, y, z, dim}: the cell the charge is about to be spent on. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/SculkBlockBehavior.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& sculkDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            SculkChargeHook,
            ll::memory::HookPriority::Normal,
            SculkBlockBehavior,
            &SculkBlockBehavior::$attemptUseCharge,
            int,
            ::IBlockWorldGenAPI& target,
            ::BlockSource* region,
            ::BlockPos const& originPos,
            ::BlockPos const& pos,
            int charge,
            int unknown,
            ::Random& random,
            ::SculkSpreader& spreader,
            bool const flag)
        {
            auto& def = sculkDef();
            if (!def.live() || region == nullptr)
            {
                return origin(target, region, originPos, pos, charge, unknown, random, spreader, flag);
            }
            std::string const snbt = "{\"eventId\":\"SculkBlockGrowthEvent\""
                ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"dim\":" + snbtNum(static_cast<int>(region->getDimensionId())) + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return 0;
            }
            return origin(target, region, originPos, pos, charge, unknown, random, spreader, flag);
        }

        HookEventDef gDef{"SculkBlockGrowthEvent", [] { return SculkChargeHook::hook() == 0; }};
        HookEventDef& sculkDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
