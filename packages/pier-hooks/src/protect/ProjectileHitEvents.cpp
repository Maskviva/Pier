/** protect/ProjectileHitEvents.cpp: "ProjectileHitEntityEvent" and "ProjectileHitBlockEvent",
 * both observation only.
 * An arrow that lands, a trident that sticks and a thrown egg all report their hit through
 * ProjectileComponent::onHit, and that is the one place that tells a mod what a projectile
 * touched: the hit on a target board, on a pressure plate, on the player a duel is about. One
 * detour serves both events and picks by the kind of hit. They dispatch before origin and cannot
 * be cancelled: the projectile has already hit, and to stop one, refuse it being launched with
 * PlayerSpawnProjectileEvent.
 * Payload of an entity hit {eventId, x, y, z, dim, projectile, projectileId, target, targetId,
 * targetIsPlayer}, with the position of the projectile. Payload of a block hit {eventId, x, y,
 * z, dim, block, projectile, projectileId}, with the block that was hit. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/entity/components_json_legacy/ProjectileComponent.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/dimension/DimensionType.h"
#include "mc/world/phys/HitResult.h"

#include "pier/support/i18n.h"
#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& hitEntityDef(); // Forward declarations
        HookEventDef& hitBlockDef();

        std::string projectilePart(::Actor& projectile)
        {
            std::string type;
            int64_t id = 0;
            try
            {
                type = projectile.getTypeName();
                id = projectile.getOrCreateUniqueID().rawID;
            }
            catch (...)
            {
                type.clear();
            }
            return ",\"projectile\":\"" + snbtEscape(type) + "\",\"projectileId\":" + snbtNum(id) + "L";
        }

        LL_TYPE_INSTANCE_HOOK(
            ProjectileHitHook,
            ll::memory::HookPriority::Normal,
            ProjectileComponent,
            &ProjectileComponent::onHit,
            void,
            ::Actor& owner,
            ::HitResult const& res)
        {
            auto& entityDef = hitEntityDef();
            auto& blockDef = hitBlockDef();
            if (!entityDef.live() && !blockDef.live()) return origin(owner, res);

            try
            {
                auto const& at = owner.getPosition();
                int const dim = static_cast<int>(owner.getDimensionId());
                if (::Actor* target = res.getEntity())
                {
                    if (entityDef.live())
                    {
                        std::string const snbt = "{\"eventId\":\"ProjectileHitEntityEvent\""
                            ",\"x\":" + snbtNum(static_cast<int>(at.x))
                            + ",\"y\":" + snbtNum(static_cast<int>(at.y))
                            + ",\"z\":" + snbtNum(static_cast<int>(at.z))
                            + ",\"dim\":" + snbtNum(dim)
                            + projectilePart(owner)
                            + ",\"target\":\"" + snbtEscape(target->getTypeName()) + "\""
                            + ",\"targetId\":" + snbtNum(static_cast<int64_t>(target->getOrCreateUniqueID().rawID)) + "L"
                            + ",\"targetIsPlayer\":" + (target->isPlayer() ? "1" : "0") + "}";
                        dispatchHookEvent(entityDef, snbt);
                    }
                }
                else if (blockDef.live() && res.mType == ::HitResultType::Tile && res.mBlock != ::BlockPos::ZERO())
                {
                    ::Block const& block = owner.getDimensionBlockSource().getBlock(res.mBlock);
                    if (!block.isAir())
                    {
                        std::string const snbt = "{\"eventId\":\"ProjectileHitBlockEvent\""
                            ",\"x\":" + snbtNum(res.mBlock.x)
                            + ",\"y\":" + snbtNum(res.mBlock.y)
                            + ",\"z\":" + snbtNum(res.mBlock.z)
                            + ",\"dim\":" + snbtNum(dim)
                            + ",\"block\":\"" + snbtEscape(block.getTypeName()) + "\""
                            + projectilePart(owner) + "}";
                        dispatchHookEvent(blockDef, snbt);
                    }
                }
            }
            catch (...)
            {
                // A hit that cannot be read is not reported. The projectile still lands.
            }
            origin(owner, res);
        }

        /**
         * Installs the one detour both events share, once. A second `hook()` call is a second
         * registration, so both events call this and the later subscription reuses the result.
         */
        bool installProjectileHitHook()
        {
            static bool const ok = []
            {
                int const r = ProjectileHitHook::hook();
                if (r != 0)
                {
                    hostLogger().error("{}", pier::trf("hooks.projectile_hit_events.1", r));
                }
                return r == 0;
            }();
            return ok;
        }

        HookEventDef gEntityDef{"ProjectileHitEntityEvent", [] { return installProjectileHitHook(); }};
        HookEventDef& hitEntityDef() { return gEntityDef; }

        HookEventDef gBlockDef{"ProjectileHitBlockEvent", [] { return installProjectileHitHook(); }};
        HookEventDef& hitBlockDef() { return gBlockDef; }

        HookEventRegistrar gEntityReg{gEntityDef};
        HookEventRegistrar gBlockReg{gBlockDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
