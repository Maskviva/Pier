/** world/ChestPairEvent.cpp: two chests are about to pair into a double chest.
 * Pairing across a boundary is a real bypass: place a chest against the plot edge, let it
 * pair with the neighbor's chest, and opening the near half shows everything in theirs.
 * Container protection decides on the cell that was clicked, and that cell really is
 * the placer's own.
 * The event carries the coordinates of both chests. Cancelling means no pairing and each
 * chest stays independent, which is vanilla behavior. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/actor/ChestBlockActor.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& chestPairDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            ChestTryPairHook,
            ll::memory::HookPriority::Normal,
            ChestBlockActor,
            &ChestBlockActor::_tryToPairWith,
            void,
            ::BlockSource& region,
            ::BlockPos const& position)
        {
            auto& def = chestPairDef();
            if (!def.live()) return origin(region, position);

            int dim = -1;
            int sx = 0, sy = 0, sz = 0;
            try
            {
                dim = static_cast<int>(region.getDimensionId());
                auto const& self = this->getPosition();
                sx = self.x;
                sy = self.y;
                sz = self.z;
            }
            catch (...)
            {
                // Unreadable coordinates do not block: this event hardens, it is not the
                // last safety gate.
            }

            std::string snbt = "{\"eventId\":\"ChestPairEvent\""
                ",\"x\":" + snbtNum(sx)
                + ",\"y\":" + snbtNum(sy)
                + ",\"z\":" + snbtNum(sz)
                + ",\"dim\":" + snbtNum(dim)
                + ",\"otherX\":" + snbtNum(position.x)
                + ",\"otherY\":" + snbtNum(position.y)
                + ",\"otherZ\":" + snbtNum(position.z) + "}";

            if (dispatchHookEventCancellable(def, snbt)) return;
            origin(region, position);
        }

        HookEventDef gDef{
            "ChestPairEvent",
            []
            {
                int const r = ChestTryPairHook::hook();
                if (r != 0)
                {
                    hostLogger().error(
                        "[hooks/ChestPairEvent] the ChestBlockActor::_tryToPairWith detour failed to install with code={}", r);
                }
                return r == 0;
            }
        };
        HookEventDef& chestPairDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
