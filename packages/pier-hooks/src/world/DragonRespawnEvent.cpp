/** hooks/world/DragonRespawnEvent.cpp: the synthetic, cancellable "DragonRespawnEvent".
 * Four end crystals on the exit portal bring the ender dragon back after it has been killed, and
 * a server that runs the end as an event, or has retired the fight, wants that to stop or to wait.
 * EndDragonFight::_createNewDragon is the engine's single place where the fight makes the dragon,
 * so the hook asks right before. The first dragon of a fresh end is not asked about, since
 * nothing has been killed and nothing is coming back. Returning without calling origin leaves the
 * end without a dragon until the next try.
 * Payload {eventId, dim}, with dim always the end, where the fight exists. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/level/dimension/DimensionType.h"
#include "mc/world/level/dimension/end/EndDragonFight.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& respawnDef(); // Forward declaration

        /** The numbering the engine gives the end. */
        constexpr int kEndDimension = 2;

        LL_TYPE_INSTANCE_HOOK(
            DragonRespawnHook,
            ll::memory::HookPriority::Normal,
            EndDragonFight,
            &EndDragonFight::_createNewDragon,
            void)
        {
            auto& def = respawnDef();
            if (!def.live() || !this->mPreviouslyKilled) return origin();

            std::string const snbt = "{\"eventId\":\"DragonRespawnEvent\""
                ",\"dim\":" + snbtNum(kEndDimension) + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return;
            }
            origin();
        }

        HookEventDef gDef{"DragonRespawnEvent", [] { return DragonRespawnHook::hook() == 0; }};
        HookEventDef& respawnDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
