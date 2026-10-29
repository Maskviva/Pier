/** hooks/world/DragonEggEvent.cpp: the synthetic, cancellable "DragonEggTeleportEvent".
 * A dragon egg jumps to another place when it is clicked or when the block under it goes, and
 * nothing watches it: a plot's trophy leaves the plot, or one lands inside it. The egg block's
 * teleport function is the engine's single place where the jump is decided and made, so the
 * hook asks right before. Returning without calling origin leaves the egg where it is.
 * Payload {eventId, x, y, z, dim}: the block the egg is in. The cell it would jump to is not
 * known until the engine has drawn it, and drawing it here would take random numbers the
 * engine's own draw then misses. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/DragonEggBlock.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& teleportDef(); // Forward declaration

        LL_TYPE_STATIC_HOOK(
            DragonEggTeleportHook,
            ll::memory::HookPriority::Normal,
            DragonEggBlock,
            &DragonEggBlock::_attemptTeleport,
            void,
            ::BlockSource& region,
            ::Random& random,
            ::BlockPos const& pos)
        {
            auto& def = teleportDef();
            if (!def.live()) return origin(region, random, pos);

            std::string const snbt = "{\"eventId\":\"DragonEggTeleportEvent\""
                ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"dim\":" + snbtNum(static_cast<int>(region.getDimensionId())) + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return;
            }
            origin(region, random, pos);
        }

        HookEventDef gDef{"DragonEggTeleportEvent", [] { return DragonEggTeleportHook::hook() == 0; }};
        HookEventDef& teleportDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
