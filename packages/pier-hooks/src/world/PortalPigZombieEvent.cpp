/** hooks/world/PortalPigZombieEvent.cpp: the synthetic, cancellable "PortalSpawnPigZombieEvent".
 * A nether portal now and then spawns a zombified piglin, which is a mob entering a plot through
 * a block nobody placed on purpose. PortalCreateEvent is about a frame becoming a portal, and
 * this is about the frame already there. SpawnChecks::canSpawnPigZombieFromPortal is the
 * engine's question of whether this tick spawns one, so the hook asks right before it answers,
 * and false is the engine's own answer of no.
 * The check does not know the frame, so PortalShape::evaluate is hooked to remember the last
 * frame the engine measured on this thread, which it does in the same random tick just before
 * it asks. Payload {eventId, x, y, z, dim, axis}: the cell the frame was measured from and its
 * axis as the engine numbers it. Nothing is asked until a frame has been measured. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/actor/SpawnChecks.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/PortalShape.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/i18n.h"
#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& pigZombieDef(); // Forward declaration

        /** The last frame measured on this thread. Left as it is after the measurement, since
         *  the question comes after the call that measured it has returned. */
        thread_local bool tlKnown = false;
        thread_local int tlX = 0;
        thread_local int tlY = 0;
        thread_local int tlZ = 0;
        thread_local int tlAxis = 0;

        LL_TYPE_INSTANCE_HOOK(
            PortalEvaluateHook,
            ll::memory::HookPriority::Normal,
            PortalShape,
            &PortalShape::evaluate,
            void,
            ::BlockPos const& originalPosition,
            ::BlockSource const& source)
        {
            origin(originalPosition, source);
            tlKnown = true;
            tlX = originalPosition.x;
            tlY = originalPosition.y;
            tlZ = originalPosition.z;
            tlAxis = static_cast<int>(this->mAxis);
        }

        LL_TYPE_STATIC_HOOK(
            PigZombieSpawnHook,
            ll::memory::HookPriority::Normal,
            SpawnChecks,
            &SpawnChecks::canSpawnPigZombieFromPortal,
            bool,
            ::Dimension const& dimension,
            ::IRandom& random)
        {
            auto& def = pigZombieDef();
            if (!def.live() || !tlKnown) return origin(dimension, random);

            std::string const snbt = "{\"eventId\":\"PortalSpawnPigZombieEvent\""
                ",\"x\":" + snbtNum(tlX)
                + ",\"y\":" + snbtNum(tlY)
                + ",\"z\":" + snbtNum(tlZ)
                + ",\"dim\":" + snbtNum(static_cast<int>(dimension.getDimensionId()))
                + ",\"axis\":" + snbtNum(tlAxis) + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return false;
            }
            return origin(dimension, random);
        }

        HookEventDef gDef{
            "PortalSpawnPigZombieEvent",
            []
            {
                int const r1 = PortalEvaluateHook::hook();
                int const r2 = PigZombieSpawnHook::hook();
                if (r1 != 0 || r2 != 0)
                {
                    hostLogger().error("{}", pier::trf("hooks.portal_pig_zombie_event.1", r1, r2));
                }
                return r1 == 0 && r2 == 0;
            }
        };
        HookEventDef& pigZombieDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
