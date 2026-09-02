/** world/PistonPushEvent.cpp: a piston is about to push or pull a set of blocks.
 * A piston is the classic cross-plot griefing tool: the machine is built on the owner's own
 * ground and the arm reaches into a neighbor's to move their blocks away. The
 * PISTON_CROSS_PLOT rule in pier-dimensions decides sameness of area from the grid, and
 * this event hands the decision to the mod, because who actually owns a plot and who is
 * authorized on it is known only there.
 * The payload carries the piston position, its facing and the list of blocks attached
 * this time, at most 12, which is the vanilla cap. Cancelling means the push or pull does
 * not happen, and _checkAttachedBlocks returning false is the engine's own cannot-move
 * path, so it is safe. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/actor/PistonBlockActor.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& pistonDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            PistonCheckAttachedHook,
            ll::memory::HookPriority::High, // Outside the dimension rules
            PistonBlockActor,
            &PistonBlockActor::_checkAttachedBlocks,
            bool,
            ::BlockSource& region)
        {
            auto& def = pistonDef();
            if (!def.live()) return origin(region);

            // The engine computes which blocks are attached first: without calling origin
            // mAttachedBlocks still holds the previous set. The mod is asked afterwards.
            if (!origin(region)) return false;

            int dim = -1;
            std::string blocks = "[";
            int px = 0, py = 0, pz = 0;
            int fx = 0, fy = 0, fz = 0;
            try
            {
                dim = static_cast<int>(region.getDimensionId());
                auto const& self = this->mPosition.get();
                px = self.x;
                py = self.y;
                pz = self.z;
                auto const& facing = this->getFacingDir(region);
                fx = facing.x;
                fy = facing.y;
                fz = facing.z;
                bool first = true;
                for (auto const& b : this->mAttachedBlocks.get())
                {
                    if (!first) blocks += ",";
                    first = false;
                    blocks += "[" + snbtNum(b.x) + "," + snbtNum(b.y) + "," + snbtNum(b.z) + "]";
                }
            }
            catch (...)
            {
                // An unreadable attachment list means the pushed set is unknown. This only
                // reports, and the mod decides from the coordinates, having at least the
                // piston position.
            }
            blocks += "]";

            std::string snbt = "{\"eventId\":\"PistonPushEvent\""
                ",\"x\":" + snbtNum(px)
                + ",\"y\":" + snbtNum(py)
                + ",\"z\":" + snbtNum(pz)
                + ",\"dim\":" + snbtNum(dim)
                + ",\"facing\":[" + snbtNum(fx) + "," + snbtNum(fy) + "," + snbtNum(fz) + "]"
                + ",\"attached\":" + blocks + "}";

            if (dispatchHookEventCancellable(def, snbt)) return false;
            return true;
        }

        HookEventDef gDef{
            "PistonPushEvent",
            []
            {
                int const r = PistonCheckAttachedHook::hook();
                if (r != 0)
                {
                    hostLogger().error(
                        "[hooks/PistonPushEvent] the PistonBlockActor::_checkAttachedBlocks "
                        "detour failed to install with code={}, so cross-plot piston pushes "
                        "and pulls are unprotected", r);
                }
                return r == 0;
            }
        };
        HookEventDef& pistonDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
