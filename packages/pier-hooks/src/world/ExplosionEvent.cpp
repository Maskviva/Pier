/** world/ExplosionEvent.cpp: the cancellable explosion event.
 * pier-dimensions hooks Level::$explode as well, but that layer answers the coarse
 * per-dimension question of whether explosions may break blocks there. A plot-level
 * decision needs the coordinates and the radius and can only be made by a mod, so a
 * second hook sits further out at HookPriority::High: the mod sees every explosion first
 * and, without a cancel, the dimension rules apply as usual.
 * Cancelling means the explosion does not happen at all, damage and blocks alike. Keeping
 * the damage while saving the blocks uses the dimension rule
 * PIER_DIMRULE_EXPLODE_BLOCKS, or a replay on the mod side with
 * explode(..., breaks_blocks=false). */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& explosionDef(); // Forward declaration

        /** Level::$explode has two overloads, the eight-argument one and the Explosion&
         *  one. Resolving the identifier against the target type inside the macro would
         *  disambiguate on its own, but an explicit cast holds either way and states which
         *  one is hooked. Same as AttackEvent. */
        using ExplodeFn = bool (Level::*)(
            ::BlockSource&, ::Actor*, ::Vec3 const&, float, bool, bool, float, bool);

        LL_TYPE_INSTANCE_HOOK(
            LevelExplodeHook,
            ll::memory::HookPriority::High, // Outside the dimension rules: the mod decides first
            Level,
            static_cast<ExplodeFn>(&Level::$explode),
            bool,
            ::BlockSource& region,
            ::Actor* source,
            ::Vec3 const& pos,
            float explosionRadius,
            bool fire,
            bool breaksBlocks,
            float maxResistance,
            bool allowUnderwater)
        {
            auto& def = explosionDef();
            if (!def.live())
            {
                return origin(region, source, pos, explosionRadius, fire, breaksBlocks, maxResistance,
                              allowUnderwater);
            }

            int dim = -1;
            std::string sourceType;
            bool sourceIsPlayer = false;
            int64_t sourceId = 0;
            try
            {
                dim = static_cast<int>(region.getDimensionId());
                if (source)
                {
                    sourceType = source->getTypeName();
                    sourceIsPlayer = source->isPlayer();
                    sourceId = source->getOrCreateUniqueID().rawID;
                }
            }
            catch (...)
            {
                sourceType.clear();
            }

            std::string snbt = "{\"eventId\":\"ExplosionEvent\""
                ",\"x\":" + snbtDouble(pos.x)
                + ",\"y\":" + snbtDouble(pos.y)
                + ",\"z\":" + snbtDouble(pos.z)
                + ",\"dim\":" + snbtNum(dim)
                + ",\"radius\":" + snbtDouble(explosionRadius)
                + ",\"maxResistance\":" + snbtDouble(maxResistance)
                + ",\"fire\":" + (fire ? "1" : "0")
                + ",\"breaksBlocks\":" + (breaksBlocks ? "1" : "0")
                + ",\"underwater\":" + (allowUnderwater ? "1" : "0")
                + ",\"sourceIsPlayer\":" + (sourceIsPlayer ? "1" : "0")
                + ",\"sourceId\":" + snbtNum(sourceId) + "L"
                + ",\"source\":\"" + snbtEscape(sourceType) + "\"}";

            if (dispatchHookEventCancellable(def, snbt)) return false;
            return origin(region, source, pos, explosionRadius, fire, breaksBlocks, maxResistance,
                          allowUnderwater);
        }

        HookEventDef gDef{
            "ExplosionEvent",
            []
            {
                int const r = LevelExplodeHook::hook();
                if (r != 0)
                {
                    hostLogger().error(
                        "[hooks/ExplosionEvent] the Level::$explode detour failed to install "
                        "with code={}, so explosion protection is inactive", r);
                }
                return r == 0;
            }
        };
        HookEventDef& explosionDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
