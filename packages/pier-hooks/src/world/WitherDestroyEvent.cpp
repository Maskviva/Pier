/** hooks/world/WitherDestroyEvent.cpp: the synthetic, cancellable "WitherDestroyEvent".
 * A wither breaks the blocks around its body when it spawns, when it is hurt and while it
 * charges. That is neither an explosion nor a player, so ExplosionEvent and the player
 * events never see it, and BlockDestroyEvent sees each block without knowing why.
 * WitherBoss::_destroyBlocks is the engine's one place for it, so the hook asks right
 * before the blocks go. Returning without calling origin leaves them standing.
 * Payload {eventId, x, y, z, dim, range, attackType, witherId}: the block coordinates of
 * the wither, how far the engine reaches, the engine's attack type number, and the actor
 * id of the wither. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <cmath>
#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/boss/WitherBoss.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& witherDef(); // Forward declaration

        int cell(float v) { return static_cast<int>(std::floor(v)); }

        LL_TYPE_INSTANCE_HOOK(
            WitherDestroyHook,
            ll::memory::HookPriority::Normal,
            WitherBoss,
            &WitherBoss::_destroyBlocks,
            void,
            ::Level& level,
            ::AABB const& bb,
            ::BlockSource& region,
            int range,
            ::WitherBoss::WitherAttackType attackType)
        {
            auto& def = witherDef();
            if (!def.live()) return origin(level, bb, region, range, attackType);

            auto const& at = this->getPosition();
            std::string const snbt = "{\"eventId\":\"WitherDestroyEvent\""
                ",\"x\":" + snbtNum(cell(at.x))
                + ",\"y\":" + snbtNum(cell(at.y))
                + ",\"z\":" + snbtNum(cell(at.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(region.getDimensionId()))
                + ",\"range\":" + snbtNum(range)
                + ",\"attackType\":" + snbtNum(static_cast<int>(attackType))
                + ",\"witherId\":" + snbtNum(static_cast<int64_t>(this->getOrCreateUniqueID().rawID))
                + "L}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return;
            }
            origin(level, bb, region, range, attackType);
        }

        HookEventDef gDef{"WitherDestroyEvent", [] { return WitherDestroyHook::hook() == 0; }};
        HookEventDef& witherDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
