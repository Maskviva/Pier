/** hooks/world/PortalEvents.cpp: the synthetic, cancellable "PortalCreateEvent".
 * A world manager offering per-dimension "portals may be lit here" has no vanilla event
 * to hang it on, and the obvious substitute, watching flint and steel, is wrong: whether
 * a frame becomes a portal is decided by PortalShape, which measures the frame, checks
 * the axis and requires fire inside. Reimplementing that geometry drifts from the engine
 * the first time upstream changes a limit, and it drifts silently.
 * PortalBlock::trySpawnPortal is the engine's own answer after that whole check, so the
 * hook needs no geometry at all. Returning false without calling origin means no portal
 * blocks are placed; the fire stays and burns out on its own, which is what a player sees
 * when a frame is the wrong size.
 * Payload {eventId, dim, x, y, z}. The frame width and height are not included: they live
 * in the PortalShape that trySpawnPortal builds internally, and constructing a second one
 * here to read them would run the scan twice.
 * There is no player in the payload. Fire reaches a frame from a dispenser, from lightning
 * and from spreading, so a consumer keying on a player would miss those and see a portal
 * appear where its own rule said no. */
#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/PortalBlock.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& portalCreateDef(); // Forward declaration

        std::string buildSnbt(::BlockSource& region, ::BlockPos const& pos)
        {
            return "{\"eventId\":\"PortalCreateEvent\""
                ",\"dim\":" + snbtNum(static_cast<int>(region.getDimensionId()))
                + ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + "}";
        }

        LL_TYPE_STATIC_HOOK(
            PortalCreateHook,
            ll::memory::HookPriority::Normal,
            PortalBlock,
            PortalBlock::trySpawnPortal,
            bool,
            ::BlockSource& region,
            ::BlockPos const& pos)
        {
            auto& def = portalCreateDef();
            if (!def.live()) return origin(region, pos);

            if (dispatchHookEventCancellable(def, buildSnbt(region, pos)))
            {
                // False is what the engine itself returns for a frame that does not
                // form a portal, so every caller upstream already handles it.
                return false;
            }
            return origin(region, pos);
        }

        HookEventDef gDef{"PortalCreateEvent", [] { return PortalCreateHook::hook() == 0; }};
        HookEventDef& portalCreateDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
