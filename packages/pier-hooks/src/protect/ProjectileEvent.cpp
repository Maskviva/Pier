/** hooks/protect/ProjectileEvent.cpp: the synthetic, cancellable "PlayerSpawnProjectileEvent".
 * A vanilla projectile is an item component and throwing goes through
 * ThrowableItemComponent::_doThrow, then ProjectileItemComponent::shootProjectile, then
 * Item::createProjectileActor. BedrockSpawner::spawnProjectile is no longer on the player path
 * and serves only as a backstop, and PlayerUseItemEvent does not cover the charge-and-release
 * path of bows, crossbows and tridents. The five hook points are ordered by coverage and do nest,
 * and gDispatching collapses one launch into one decision. Cancelling stops the projectile and
 * does not refund ammunition: by the time the hook runs the arrow has already left the inventory,
 * and the client realigns within a tick.
 * Payload {eventId, x, y, z, dim, projectile, _player:{name,xuid,uuid}}. At hook points 2 and 4
 * the entity type is not resolved yet, so projectile may be empty and is informational only. / */
#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/common/Globals.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/core/string/HashedString.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorDefinitionIdentifier.h"
#include "mc/world/actor/ActorType.h"
#include "mc/world/actor/VanillaActorRendererId.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/item/CrossbowItem.h"
#include "mc/world/item/ItemInstance.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/item/TridentItem.h"
#include "mc/world/item/components/ProjectileItemComponent.h"
#include "mc/world/item/components/ShooterItemComponent.h"
#include "mc/world/level/BedrockSpawner.h"
#include "mc/world/level/BlockSource.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& projectileDef(); // Forward declaration

        /**
         * The in-progress flag a nested hook point checks so it does not ask again.
         *
         * thread_local and not global: a global flag would let two concurrent launches
         * swallow each other's decision, and that kind of leak is silent. DispatchGuard
         * restores it as each hook exits.
         */
        thread_local bool gDispatching = false;

        struct DispatchGuard
        {
            DispatchGuard() { gDispatching = true; }
            ~DispatchGuard() { gDispatching = false; }
        };

        /** Shared payload assembly, so all five hook points report the same event
         *  shape. */
        std::string buildSnbt(Player& p, std::string const& projectile, ::Vec3 const& at, int dim)
        {
            return "{\"eventId\":\"PlayerSpawnProjectileEvent\""
                ",\"x\":" + snbtNum(static_cast<int>(at.x))
                + ",\"y\":" + snbtNum(static_cast<int>(at.y))
                + ",\"z\":" + snbtNum(static_cast<int>(at.z))
                + ",\"dim\":" + snbtNum(dim)
                + ",\"projectile\":\"" + snbtEscape(projectile)
                + "\"," + playerRefSnbt(p) + "}";
        }

        /** A name lookup throws while an item or actor is being destroyed, and an
         *  exception crossing a detour takes the whole server down, so it is caught here.
         *  A subscriber receiving an empty name falls back to a coarser decision, which is
         *  never more permissive. */
        template <class Fn>
        std::string safeName(Fn&& fn)
        {
            try
            {
                return std::string{fn()};
            }
            catch (...)
            {
                return {};
            }
        }

        /** Asks once. Returns true when this launch must be refused. */
        bool refuseLaunch(Player& p, std::string const& projectile, ::Vec3 const& at, int dim)
        {
            DispatchGuard guard;
            return dispatchHookEventCancellable(projectileDef(), buildSnbt(p, projectile, at, dim));
        }

        // 1. Component-driven projectiles: snowballs, eggs, ender pearls, potions,
        //    experience bottles, wind charges, fire charges, and the arrows a bow or
        //    crossbow shoots. It carries a Player* directly and returning nullptr
        //    cancels.

        LL_TYPE_INSTANCE_HOOK(
            ShootProjectileHook,
            ll::memory::HookPriority::Normal,
            ProjectileItemComponent,
            &ProjectileItemComponent::shootProjectile,
            ::Actor*,
            ::BlockSource& region,
            ::Vec3 const& aimPos,
            ::Vec3 const& aimDir,
            float power,
            ::Player* player)
        {
            auto& def = projectileDef();
            if (!def.live() || gDispatching || player == nullptr)
            {
                return origin(region, aimPos, aimDir, power, player);
            }

            if (refuseLaunch(*player, {}, aimPos, static_cast<int>(region.getDimensionId())))
            {
                return nullptr;
            }
            return origin(region, aimPos, aimDir, power, player);
        }

        // 2. The bow or crossbow release itself. Redundant with 1, but it fires before
        //    the per-arrow loop, so a multishot crossbow costs one decision here.

        LL_TYPE_INSTANCE_HOOK(
            ShooterReleaseHook,
            ll::memory::HookPriority::Normal,
            ShooterItemComponent,
            &ShooterItemComponent::_shootProjectiles,
            void,
            ::ItemStack& shooterStack,
            ::Player* player,
            int durationLeft)
        {
            auto& def = projectileDef();
            if (!def.live() || gDispatching || player == nullptr)
            {
                return origin(shooterStack, player, durationLeft);
            }

            std::string shooterName;
            if (!shooterStack.isNull())
            {
                shooterName = safeName([&] { return shooterStack.getTypeName(); });
            }

            if (refuseLaunch(
                    *player,
                    shooterName,
                    player->getPosition(),
                    static_cast<int>(player->getDimensionId())))
            {
                return;
            }
            origin(shooterStack, player, durationLeft);
        }

        // 3. A thrown trident. It is a bespoke item and neither hook point 1 nor 2
        //    reaches it.

        LL_TYPE_INSTANCE_HOOK(
            TridentReleaseHook,
            ll::memory::HookPriority::Normal,
            TridentItem,
            &TridentItem::$releaseUsing,
            void,
            ::ItemStack& item,
            ::Player* player,
            int durationLeft)
        {
            auto& def = projectileDef();
            if (!def.live() || gDispatching || player == nullptr)
            {
                return origin(item, player, durationLeft);
            }

            std::string projName =
                safeName([] { return ::VanillaActorRendererId::trident().getString(); });

            if (refuseLaunch(
                    *player,
                    projName,
                    player->getPosition(),
                    static_cast<int>(player->getDimensionId())))
            {
                return;
            }
            origin(item, player, durationLeft);
        }

        // 4. A crossbow loaded with a firework rocket, as in 3.

        LL_TYPE_INSTANCE_HOOK(
            CrossbowFireworkHook,
            ll::memory::HookPriority::Normal,
            CrossbowItem,
            &CrossbowItem::_shootFirework,
            void,
            ::ItemInstance const& projectileInstance,
            ::Player& player)
        {
            auto& def = projectileDef();
            if (!def.live() || gDispatching)
            {
                return origin(projectileInstance, player);
            }

            std::string projName = safeName([&] { return projectileInstance.getTypeName(); });

            if (refuseLaunch(
                    player,
                    projName,
                    player.getPosition(),
                    static_cast<int>(player.getDimensionId())))
            {
                return;
            }
            origin(projectileInstance, player);
        }

        // 5. The older spawner path, a backstop for add-on entities and the code around
        //    dispensers.

        LL_TYPE_INSTANCE_HOOK(
            SpawnProjectileHook,
            ll::memory::HookPriority::Normal,
            BedrockSpawner,
            &BedrockSpawner::$spawnProjectile,
            ::Actor*,
            ::BlockSource& region,
            ::ActorDefinitionIdentifier const& id,
            ::Actor* spawner,
            ::Vec3 const& position,
            ::Vec3 const& direction)
        {
            auto& def = projectileDef();
            if (!def.live() || gDispatching || spawner == nullptr || !spawner->isPlayer())
            {
                return origin(region, id, spawner, position, direction);
            }

            // Tridents belong to TridentReleaseHook, and reporting one here as well would
            // fire the event twice for a single throw.
            static auto& tridentName = EntityCanonicalName(::ActorType::Trident);
            if (*id.mCanonicalName == tridentName)
            {
                return origin(region, id, spawner, position, direction);
            }

            auto& p = *static_cast<Player*>(spawner);

            std::string projName = safeName([&] { return id.mCanonicalName->getString(); });

            if (refuseLaunch(p, projName, position, static_cast<int>(region.getDimensionId())))
            {
                return nullptr;
            }
            return origin(region, id, spawner, position, direction);
        }

        HookEventDef gDef{
            "PlayerSpawnProjectileEvent",
            []
            {
                // Each status is reported separately, where 0 is success. The hook points
                // cover different things, so with only 1 failing every snowball and arrow
                // gets through while tridents are still blocked, and the symptom is
                // protection that works sometimes. One combined log line could not tell
                // such a partial failure apart.
                int r1 = ShootProjectileHook::hook();
                int r2 = ShooterReleaseHook::hook();
                int r3 = TridentReleaseHook::hook();
                int r4 = CrossbowFireworkHook::hook();
                int r5 = SpawnProjectileHook::hook();
                auto& log = hostLogger();
                log.debug(
                    "[hooks/ProjectileEvent] installing detours: shootProjectile={}, _shootProjectiles={}, "
                    "trident.releaseUsing={}, _shootFirework={}, spawnProjectile={} "
                    "(codes: {} {} {} {} {})",
                    r1 == 0 ? "ok" : "failed", r2 == 0 ? "ok" : "failed",
                    r3 == 0 ? "ok" : "failed", r4 == 0 ? "ok" : "failed",
                    r5 == 0 ? "ok" : "failed",
                    r1, r2, r3, r4, r5);
                if (r1 != 0)
                {
                    log.error(
                        "[hooks/ProjectileEvent] the primary hook point "
                        "ProjectileItemComponent::shootProjectile failed to install with "
                        "code={}, so component-driven projectiles such as snowballs, eggs, "
                        "ender pearls, potions and arrows are entirely unprotected; the "
                        "remaining hook points cover only tridents, crossbow fireworks and "
                        "the older path. The usual cause is a mismatch between the BDS or "
                        "LeviLamina version this host was linked against and the one the "
                        "server runs.", r1);
                }
                if (r2 != 0 || r3 != 0 || r4 != 0 || r5 != 0)
                {
                    log.warn(
                        "[hooks/ProjectileEvent] a secondary hook point failed to install "
                        "(codes: {} {} {} {}), so interception is missing on the "
                        "corresponding paths: the whole-shot decision for bows and "
                        "crossbows, tridents, crossbow fireworks and the older spawner "
                        "path",
                        r2, r3, r4, r5);
                }
                // The primary hook point is required; a missing secondary one only
                // degrades, and that was warned about above.
                return r1 == 0;
            }
        };
        HookEventDef& projectileDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
