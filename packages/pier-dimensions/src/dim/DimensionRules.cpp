/**
 * DimensionRules.cpp: behavior rules that apply per dimension.
 * A Bedrock gamerule is one value for the whole server: keeping mobs out of a creative
 * plot world means doMobSpawning=false, which also empties the survival world on the
 * same server. The functions that do the work are hooked instead, such as
 * Spawner::spawnMob and Explosion::explode. Each reaches a BlockSource, directly or
 * through the object it works on, that yields a dimension id, so the decision really is
 * per dimension.
 * The rule table is sparse by dimension id and a miss goes straight to origin(). These
 * hooks are installed globally, by the first rule set, and never change the behavior of a
 * dimension nobody set a rule for; a rule set on a vanilla dimension is enforced too.
 * Fifteen hook points cover twelve rules. SpawnMonster, SpawnAnimal and SpawnSpawner
 * share Spawner::spawnMob, ExplodeBlocks and MobGriefing share Explosion::explode,
 * PistonPush and PistonCrossCell share PistonBlockActor::_checkAttachedBlocks, FireSpread
 * has four, and MobGriefing has four more. EntityCrossCell is in CellConfine.cpp.
 */
#include "pier/dimensions/dim/dimension_rules.h"

#if !defined(__clang__)
#error "This file hooks EventCoordinatorPimpl::_processEvent and builds with clang-cl only: cl rejects its declaration (C2491). Configure with: xmake f --toolchain=clang-cl"
#endif

#include <atomic>
#include <cstdint>
#include <mutex>
#include <type_traits>
#include <unordered_map>

#include "ll/api/base/ScopedValue.h"
#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/IRandom.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/ecs/gamerefs_entity/GameRefsEntity.h"
#include "mc/gameplayhandlers/ActorGameplayHandler.h"
#include "mc/gameplayhandlers/CoordinatorResult.h"
#include "mc/legacy/ActorUniqueID.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorCategory.h"
#include "mc/world/actor/ActorDefinitionIdentifier.h"
#include "mc/world/actor/Mob.h"
#include "mc/world/actor/ai/goal/PlaceBlockGoal.h"
#include "mc/world/actor/ai/goal/TakeBlockGoal.h"
#include "mc/world/actor/boss/WitherBoss.h"
#include "mc/world/events/ActorEventListener.h"
#include "mc/world/events/ActorGameplayEvent.h"
#include "mc/world/events/ActorGriefingBlockEvent.h"
#include "mc/world/events/EventCoordinatorPimpl.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Explosion.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/Spawner.h"
#include "mc/world/level/block/FarmBlock.h"
#include "mc/world/level/block/FireBlock.h"
#include "mc/world/level/block/LiquidBlock.h"
#include "mc/world/level/block/actor/PistonBlockActor.h"
#include "mc/world/level/block/block_events/BlockPlaceEvent.h"
#include "mc/world/level/block/block_events/BlockQueuedTickEvent.h"

#include "sdk/abi.h"

#include "pier/dimensions/gen/cell_confine.h"
#include "pier/support/log.h"
#include "pier/support/i18n.h"

namespace pier::dimensions
{
    // The numbering must match the ABI value for value. Writing it twice is
    // unavoidable, since this side needs names, but keeping the two in step must not
    // rest on a person. The symptom of a mismatch is setting mob spawning off and
    // turning off fire spread instead, which shows nothing about the numbering, so it
    // is pinned at compile time.
    static_assert(static_cast<int>(DimRule::SpawnMonster) == PIER_DIMRULE_SPAWN_MONSTER);
    static_assert(static_cast<int>(DimRule::SpawnAnimal) == PIER_DIMRULE_SPAWN_ANIMAL);
    static_assert(static_cast<int>(DimRule::SpawnSpawner) == PIER_DIMRULE_SPAWN_SPAWNER);
    static_assert(static_cast<int>(DimRule::ExplodeBlocks) == PIER_DIMRULE_EXPLODE_BLOCKS);
    static_assert(static_cast<int>(DimRule::FireSpread) == PIER_DIMRULE_FIRE_SPREAD);
    static_assert(static_cast<int>(DimRule::MobGriefing) == PIER_DIMRULE_MOB_GRIEFING);
    static_assert(static_cast<int>(DimRule::Projectile) == PIER_DIMRULE_PROJECTILE);
    static_assert(static_cast<int>(DimRule::PistonPush) == PIER_DIMRULE_PISTON_PUSH);
    static_assert(static_cast<int>(DimRule::LiquidFlow) == PIER_DIMRULE_LIQUID_FLOW);
    static_assert(static_cast<int>(DimRule::FarmlandDecay) == PIER_DIMRULE_FARMLAND_DECAY);
    static_assert(static_cast<int>(DimRule::Ride) == PIER_DIMRULE_RIDE);
    static_assert(static_cast<int>(DimRule::PistonCrossCell) == PIER_DIMRULE_PISTON_CROSS_CELL);
    static_assert(static_cast<int>(DimRule::EntityCrossCell) == PIER_DIMRULE_ENTITY_CROSS_CELL);
    static_assert(kDimRuleCount == PIER_DIMRULE_ENTITY_CROSS_CELL + 1, "a rule was appended without updating the count");

    namespace
    {
        using ::pier::hostLogger;

        /**
         * (dimension id, rule) to allowed. Only entries that were set explicitly.
         * A mutex rather than a lock-free structure. Writes happen when a world is
         * created or loaded, a few dozen times per server, and reads happen on the spawn
         * path, a few hundred times per second. Reads dominate, but their absolute count
         * is small: one spawn already runs a great deal of engine logic and one mutex
         * lock disappears into that noise. Optimizing this starts with a measurement.
         */
        std::mutex& rulesMutex()
        {
            static std::mutex m;
            return m;
        }

        std::unordered_map<uint64_t, bool>& rules()
        {
            static std::unordered_map<uint64_t, bool> m;
            return m;
        }

        constexpr uint64_t key(int dimension, int rule)
        {
            return (static_cast<uint64_t>(static_cast<uint32_t>(dimension)) << 32)
                | static_cast<uint32_t>(rule);
        }

        /** Whether the dimension has any rule at all. Without one the hook takes its
         *  fast path. */
        std::unordered_map<int, int>& dimCounts()
        {
            static std::unordered_map<int, int> m;
            return m;
        }

        /*  Lock-free fast path for the hook bodies
         * Once installed the detours below are global and sit on LiquidBlock::_trySpreadTo,
         * FireBlock::checkBurn, Spawner::spawnMob and Actor::canAddPassenger, which fire
         * thousands of times per tick, so a dimension with no rule at all must answer
         * without taking rulesMutex. Dimensions 0..63 are mirrored into atomics: one bit
         * per dimension with any
         * rule, one byte per (dimension, rule) holding unset, deny or allow. Writers update
         * them under rulesMutex, readers load relaxed and never take the lock. An id of 64
         * or above keeps the locked path. */
        constexpr int kMirroredDims = 64;
        std::atomic<uint64_t> gRuleDimBits{0};
        std::atomic<bool> gRulesAbove64{false};
        // 0 unset, 1 deny, 2 allow
        std::atomic<uint8_t> gRuleMirror[kMirroredDims][kDimRuleCount]{};

        void mirrorRuleLocked(int dimension, int rule, uint8_t state)
        {
            if (dimension < 0) return;
            if (dimension >= kMirroredDims)
            {
                gRulesAbove64.store(true, std::memory_order_relaxed);
                return;
            }
            gRuleMirror[dimension][rule].store(state, std::memory_order_relaxed);
            auto it = dimCounts().find(dimension);
            bool const any = it != dimCounts().end() && it->second > 0;
            uint64_t const bit = uint64_t{1} << dimension;
            if (any) gRuleDimBits.fetch_or(bit, std::memory_order_relaxed);
            else gRuleDimBits.fetch_and(~bit, std::memory_order_relaxed);
        }

        bool anyRuleFor(int dimension)
        {
            if (dimension < 0) return false;
            if (dimension < kMirroredDims)
            {
                return (gRuleDimBits.load(std::memory_order_relaxed) >> dimension) & 1u;
            }
            if (!gRulesAbove64.load(std::memory_order_relaxed)) return false;
            std::lock_guard lock{rulesMutex()};
            auto it = dimCounts().find(dimension);
            return it != dimCounts().end() && it->second > 0;
        }
    } // namespace

    void setDimensionRule(int dimension, int rule, bool allow)
    {
        if (rule < 0 || rule >= kDimRuleCount)
        {
            // An out-of-range rule number can only come from a caller bug or from an SDK
            // newer than the host. Ignoring it silently turns "I did set it" into a
            // report with no findable cause.
            hostLogger().error(
                "[dim] {}", pier::trf("dim.dimension_rules.1", dimension, rule, kDimRuleCount));
            return;
        }
        // Before the table lock: installing suspends the other threads, and one of them may
        // be waiting on that lock inside a hook body.
        registerDimensionRuleHooks();
        std::lock_guard lock{rulesMutex()};
        auto const k = key(dimension, rule);
        auto [it, inserted] = rules().insert_or_assign(k, allow);
        if (inserted) dimCounts()[dimension] += 1;
        mirrorRuleLocked(dimension, rule, allow ? 2 : 1);
    }

    bool getDimensionRule(int dimension, int rule, bool* outAllow)
    {
        if (rule < 0 || rule >= kDimRuleCount) return false;
        if (dimension >= 0 && dimension < kMirroredDims)
        {
            uint8_t const state = gRuleMirror[dimension][rule].load(std::memory_order_relaxed);
            if (state == 0) return false;
            if (outAllow) *outAllow = (state == 2);
            return true;
        }
        std::lock_guard lock{rulesMutex()};
        auto it = rules().find(key(dimension, rule));
        if (it == rules().end()) return false;
        if (outAllow) *outAllow = it->second;
        return true;
    }

    void clearDimensionRules(int dimension)
    {
        std::lock_guard lock{rulesMutex()};
        for (int r = 0; r < kDimRuleCount; ++r) rules().erase(key(dimension, r));
        dimCounts().erase(dimension);
        for (int r = 0; r < kDimRuleCount; ++r) mirrorRuleLocked(dimension, r, 0);
    }

    namespace
    {
        /**
         * Looks up one rule. An unset rule returns `fallback`, which is always true,
         * meaning behave as vanilla.
         * Defaulting to allow is the key constraint here: these hooks are installed
         * globally and an unmanaged dimension must not notice them at all.
         */
        bool allowed(int dimension, DimRule rule, bool fallback = true)
        {
            bool v = fallback;
            if (!getDimensionRule(dimension, static_cast<int>(rule), &v)) return fallback;
            return v;
        }

        /**
         * Reads the dimension id. -1 means it cannot be determined and is not a
         * dimension (contract §5.2), so every call site is written as
         * `dim < 0` going straight to origin(), meaning do nothing when unknown. That is
         * deliberate: a BlockSource whose dimension cannot be read means the engine state
         * is already abnormal, and enforcing a protection rule against a guessed
         * dimension is more dangerous than not enforcing it.
         */
        int dimOf(::BlockSource& region)
        {
            try
            {
                return static_cast<int>(region.getDimensionId());
            }
            catch (...)
            {
                return -1;
            }
        }

        //  Mob spawning

        /*
         * `Spawner::spawnMob` is on the path of every spawn: natural spawning, spawners,
         * spawn eggs and command summons all pass through it, so the source is told apart
         * by the arguments and a sheep a player placed with an egg is not blocked.
         *
         *   naturalSpawn == true   natural spawn -> SpawnMonster or SpawnAnimal
         *   fromSpawner  == true   a spawner     -> SpawnSpawner
         *   both false             command, egg or breeding -> always allowed
         *
         * The last line is deliberate: a player placing a mob in a creative world is
         * ordinary play and must not be blocked by a no-spawning setting.
         */
        // Spawner and not BedrockSpawner: the level owns a BedrockSpawner, which overrides
        // spawnItem but inherits this function, so the Spawner implementation is the one
        // that runs.
        LL_TYPE_INSTANCE_HOOK(
            DimRuleSpawnMobHook,
            ll::memory::HookPriority::Normal,
            Spawner,
            &Spawner::$spawnMob,
            ::Mob*,
            ::BlockSource& region,
            ::ActorDefinitionIdentifier const& id,
            ::Actor* spawner,
            ::Vec3 const& pos,
            bool naturalSpawn,
            bool surface,
            bool fromSpawner
        )
        {
            int const dim = dimOf(region);
            if (dim < 0 || !anyRuleFor(dim))
            {
                return origin(region, id, spawner, pos, naturalSpawn, surface, fromSpawner);
            }

            if (fromSpawner && !allowed(dim, DimRule::SpawnSpawner))
            {
                return nullptr;
            }

            if (naturalSpawn)
            {
                // The common setting, no natural spawning at all, is answered before the
                // engine builds the mob. Only a split setting needs the category, which
                // exists only on the built mob, and pays for a spawn it then undoes.
                if (!allowed(dim, DimRule::SpawnMonster) && !allowed(dim, DimRule::SpawnAnimal))
                {
                    return nullptr;
                }
                // Hostile or friendly is decided by category, and an undecidable one
                // counts as hostile. One mob too few in a creative world beats one too
                // many interrupting a build.
                auto* mob = origin(region, id, spawner, pos, naturalSpawn, surface, fromSpawner);
                if (!mob) return nullptr;

                bool const hostile = mob->hasCategory(::ActorCategory::Monster);
                bool const ok =
                    hostile ? allowed(dim, DimRule::SpawnMonster) : allowed(dim, DimRule::SpawnAnimal);
                if (!ok)
                {
                    // The category is only known once the mob exists, so it is removed
                    // immediately. That is more reliable than guessing beforehand:
                    // ActorDefinitionIdentifier carries only a name, and hardcoding
                    // entries such as "minecraft:zombie" into a table eventually misses
                    // one.
                    try
                    {
                        // $despawn is the virtual thunk (Actor.h:1917). A direct
                        // despawn() does not exist in these headers.
                        mob->$despawn();
                    }
                    catch (...)
                    {
                        hostLogger().warn("[dim] {}", pier::trf("dim.dimension_rules.2", dim));
                    }
                    return nullptr;
                }
                return mob;
            }

            // Command, spawn egg and breeding are not blocked.
            return origin(region, id, spawner, pos, naturalSpawn, surface, fromSpawner);
        }

        //  Projectiles

        // Spawner and not BedrockSpawner: the level owns a BedrockSpawner, which overrides
        // spawnItem but inherits this function, so the Spawner implementation is the one
        // that runs.
        LL_TYPE_INSTANCE_HOOK(
            DimRuleSpawnProjectileHook,
            ll::memory::HookPriority::Normal,
            Spawner,
            &Spawner::$spawnProjectile,
            ::Actor*,
            ::BlockSource& region,
            ::ActorDefinitionIdentifier const& id,
            ::Actor* spawner,
            ::Vec3 const& position,
            ::Vec3 const& direction
        )
        {
            int const dim = dimOf(region);
            if (dim >= 0 && anyRuleFor(dim) && !allowed(dim, DimRule::Projectile))
            {
                return nullptr;
            }
            return origin(region, id, spawner, position, direction);
        }

        //  Explosions

        /*
         * The explosion is not cancelled, only its block-breaking bit is turned off. With
         * mBreaking false it still bangs and still hurts but leaves no crater. Cancelling
         * the whole thing would also eat the damage and the particles, and a player would
         * conclude the creeper is broken.
         * ExplodeBlocks = false stops any explosion from breaking blocks, TNT included.
         * MobGriefing = false stops mob-caused explosions, while TNT a player lit still
         * breaks blocks. It also stops what mobs do to blocks outside an explosion, which
         * is the four hooks after the ride rule. With both set, either one forbidding is
         * enough.
         * Explosion::explode is the hook point and not a Level::explode overload: every
         * explosion runs it, whichever overload or component built the Explosion, and the
         * Explosion carries its own BlockSource in mRegion and its cause in mSourceID.
         */

        /** The actor an explosion came from, or null when it has none or it is gone. The
         *  lookup includes actors marked removed, whose category MobGriefing still reads. */
        ::Actor* sourceOf(::Explosion const& explosion)
        {
            ::ActorUniqueID const& id = explosion.mSourceID.get();
            if (id == ::ActorUniqueID::INVALID_ID())
            {
                return nullptr;
            }
            return explosion.mRegion.getLevel().fetchEntity(id, true);
        }

        /** Whether a mob and not a player caused the explosion. A source that cannot be
         *  read counts as no mob, which leaves ExplodeBlocks as the only rule applied. */
        bool causedByMob(::Explosion const& explosion)
        {
            try
            {
                ::Actor* const source = sourceOf(explosion);
                return source != nullptr && source->hasCategory(::ActorCategory::Mob)
                    && !source->hasCategory(::ActorCategory::Player);
            }
            catch (...)
            {
                return false;
            }
        }

        LL_TYPE_INSTANCE_HOOK(
            DimRuleExplodeHook,
            ll::memory::HookPriority::Normal,
            Explosion,
            &Explosion::explode,
            bool,
            ::IRandom& random
        )
        {
            int const dim = this->mBreaking ? dimOf(this->mRegion) : -1;
            if (dim < 0 || !anyRuleFor(dim))
            {
                return origin(random);
            }

            bool allowBreak = allowed(dim, DimRule::ExplodeBlocks);
            if (allowBreak && !allowed(dim, DimRule::MobGriefing) && causedByMob(*this))
            {
                allowBreak = false;
            }
            if (!allowBreak)
            {
                this->mBreaking = false;
            }
            return origin(random);
        }

        //  Fire spread

        /*
         * Fire spreads in two steps. `FireBlock::checkBurn` is the step where fire burns
         * or ignites a neighbor. Blocking it leaves an already burning fire burning, still
         * consuming its own cell, while it no longer creeps sideways. The second step is
         * the new fire a ticking fire block places in the cells around it. That placement
         * runs `FireBlock::onPlace`, which asks `FireBlock::mayPlace`, the sequence
         * LeviLamina's FireSpreadEvent is built on. Refusing the answer inside a tick
         * removes the new fire, and the cell the tick belongs to is exempt so the fire
         * keeps aging in place. A fire placed outside a tick, with flint and steel for
         * one, is never refused.
         */

        /** The fire block that is ticking in a dimension forbidding FireSpread, and
         *  whether an onPlace is running inside that tick. Server thread only. */
        struct FireWindow
        {
            bool ticking = false;
            bool placing = false;
            int x = 0;
            int y = 0;
            int z = 0;
        };

        thread_local FireWindow gFireWindow;

        // This is a const member function. A const hook is known to work in this project;
        // the packet hooks in ChunkTrace are const too.
        LL_TYPE_INSTANCE_HOOK(
            DimRuleFireSpreadHook,
            ll::memory::HookPriority::Normal,
            FireBlock,
            &FireBlock::checkBurn,
            void,
            ::BlockSource& region,
            ::BlockPos const& pos,
            int chance,
            ::IRandom& random,
            int age,
            ::BlockPos const& firePos
        )
        {
            int const dim = dimOf(region);
            if (dim >= 0 && anyRuleFor(dim) && !allowed(dim, DimRule::FireSpread))
            {
                return;
            }
            origin(region, pos, chance, random, age, firePos);
        }

        LL_TYPE_INSTANCE_HOOK(
            DimRuleFireTickHook,
            ll::memory::HookPriority::Normal,
            FireBlock,
            &FireBlock::tick,
            void,
            ::BlockEvents::BlockQueuedTickEvent& eventData
        )
        {
            int const dim = dimOf(eventData.mRegion);
            if (dim < 0 || !anyRuleFor(dim) || allowed(dim, DimRule::FireSpread))
            {
                return origin(eventData);
            }
            ::BlockPos const& at = eventData.mPos.get();
            ll::ScopedValue window{gFireWindow, FireWindow{true, false, at.x, at.y, at.z}};
            origin(eventData);
        }

        LL_TYPE_INSTANCE_HOOK(
            DimRuleFirePlaceHook,
            ll::memory::HookPriority::Normal,
            FireBlock,
            &FireBlock::onPlace,
            void,
            ::BlockEvents::BlockPlaceEvent& eventData
        )
        {
            if (!gFireWindow.ticking)
            {
                return origin(eventData);
            }
            FireWindow inside = gFireWindow;
            inside.placing = true;
            ll::ScopedValue window{gFireWindow, inside};
            origin(eventData);
        }

        LL_TYPE_INSTANCE_HOOK(
            DimRuleFireMayPlaceHook,
            ll::memory::HookPriority::Normal,
            FireBlock,
            &FireBlock::$mayPlace,
            bool,
            ::BlockSource& region,
            ::BlockPos const& pos
        )
        {
            bool const placeable = origin(region, pos);
            if (!placeable || !gFireWindow.placing)
            {
                return placeable;
            }
            return pos.x == gFireWindow.x && pos.y == gFireWindow.y && pos.z == gFireWindow.z;
        }

        //  Liquid spread

        /*
         * `LiquidBlock::_trySpreadTo` is the step where water or lava spreads into one
         * cell. Blocking it leaves a placed liquid source in place while it no longer
         * creeps outward, which is what the `LiquidFlow` flag in PlotSquared means.
         *
         * The hook point comes from `onLiquidFlow` in LegacyScriptEngine.
         */
        LL_TYPE_INSTANCE_HOOK(
            DimRuleLiquidFlowHook,
            ll::memory::HookPriority::Normal,
            LiquidBlock,
            &LiquidBlock::_trySpreadTo,
            void,
            ::BlockSource& region,
            ::BlockPos const& pos,
            int neighbor,
            ::BlockPos const& flowFromPos,
            uchar flowFromDirection
        )
        {
            int const dim = dimOf(region);
            if (dim >= 0 && anyRuleFor(dim) && !allowed(dim, DimRule::LiquidFlow))
            {
                return;
            }
            origin(region, pos, neighbor, flowFromPos, flowFromDirection);
        }

        //  Farmland trampling

        /*
         * `FarmBlock::$transformOnFall` turns farmland back into dirt when stepped on.
         * In a plot world someone running across a farm ruins a stretch of it.
         */
        LL_TYPE_INSTANCE_HOOK(
            DimRuleFarmlandHook,
            ll::memory::HookPriority::Normal,
            FarmBlock,
            &FarmBlock::$transformOnFall,
            void,
            ::BlockSource& region,
            ::BlockPos const& pos,
            ::Actor* actor,
            float fallDistance
        )
        {
            int const dim = dimOf(region);
            if (dim >= 0 && anyRuleFor(dim) && !allowed(dim, DimRule::FarmlandDecay))
            {
                return;
            }
            origin(region, pos, actor, fallDistance);
        }

        //  Piston movement

        /* PistonBlockActor::_checkAttachedBlocks decides whether this extension can carry
         * the attached blocks; false means it cannot, so the piston jams rather than moving
         * them. Blocking here and not the piston itself keeps the piston moving and
         * redstone working, where disabling pistons breaks many contraptions.
         * PistonPush and PistonCrossCell share this hook, the first applying to the whole
         * dimension and the second deciding by boundary; two patches on one symbol would
         * add an indirect jump and an ordering question for nothing.
         * origin(region) must run first: it computes which blocks move, with
         * _attachedBlockWalker filling mAttachedBlocks, and without it there is no list to
         * inspect. Once it returns true, each block is checked so that its current cell and
         * its destination cell are both in the same area as the piston, and one block out
         * of bounds refuses the whole move, because a partial allowance tears a flying
         * machine in half. A refusal leaves mAttachedBlocks alone: returning false is the
         * engine's own cannot-move exit, taken on bedrock or beyond 12 blocks. */
        LL_TYPE_INSTANCE_HOOK(
            DimRulePistonHook,
            ll::memory::HookPriority::Normal,
            PistonBlockActor,
            &PistonBlockActor::_checkAttachedBlocks,
            bool,
            ::BlockSource& region
        )
        {
            int const dim = dimOf(region);
            bool const managed = dim >= 0 && anyRuleFor(dim);
            if (managed && !allowed(dim, DimRule::PistonPush))
            {
                return false;
            }
            if (!origin(region))
            {
                return false;
            }
            // Nothing further is computed when crossing is allowed, or the dimension is
            // unmanaged, or there is no grid.
            if (!managed || allowed(dim, DimRule::PistonCrossCell) || !hasCellGrid(dim))
            {
                return true;
            }

            try
            {
                // The position and the list are read as members rather than through
                // `getPosition()` and `getAttachedBlocks()`. Both of those are MCFOLD and
                // need runtime symbol resolution, which adds a step that can fail on
                // version drift, while all they do is read these two members.
                // `getFacingDir` is different: it computes the facing from the block
                // state and has to be called.
                auto const& self = this->mPosition.get();
                auto const& facing = this->getFacingDir(region);
                for (auto const& b : this->mAttachedBlocks.get())
                {
                    // Both the origin cell and the destination cell are checked. Checking
                    // only the destination would allow pulling a block out of someone
                    // else's plot with a sticky piston, which is the same violation as
                    // pushing one in.
                    if (!sameArea(dim, self.x, self.z, b.x, b.z)
                        || !sameArea(dim, self.x, self.z, b.x + facing.x, b.z + facing.z))
                    {
                        return false;
                    }
                }
            }
            catch (...)
            {
                // An unreadable list refuses conservatively. This is a protection
                // decision, and allowing when unknown is the same as not installing it.
                return false;
            }
            return true;
        }

        //  Riding

        /*
         * `Actor::$canAddPassenger` decides whether something can be ridden. It hangs off
         * Actor, so boats, minecarts, horses and pigs are all covered. The hook point
         * comes from `onRide` in LSE.
         *
         * The dimension is taken from the ridden entity and not from the passenger. Both
         * are certainly in the same dimension, so either works, and this saves one
         * dereference.
         */
        LL_TYPE_INSTANCE_HOOK(
            DimRuleRideHook,
            ll::memory::HookPriority::Normal,
            Actor,
            &Actor::$canAddPassenger,
            bool,
            ::Actor& passenger
        )
        {
            int dim = -1;
            try
            {
                dim = static_cast<int>(this->getDimensionId());
            }
            catch (...)
            {
                // As in dimOf, -1 means it cannot be determined, and the `dim >= 0` on
                // the following line disables the rule and goes to origin(). An entity
                // whose dimension cannot be read is most likely being destroyed, and
                // blocking one ride against a guessed dimension is worth nothing.
                dim = -1;
            }
            if (dim >= 0 && anyRuleFor(dim) && !allowed(dim, DimRule::Ride))
            {
                return false;
            }
            return origin(passenger);
        }

        //  Mobs changing blocks without an explosion

        /*
         * MobGriefing also covers what mobs do to blocks outside an explosion. The engine
         * asks its own listener before a mob changes a block, whichever mob it is, and an
         * answer of Cancel stops it, so the last hook below covers every mob that asks. The
         * two goals of an enderman picking a block up and putting one down are stopped at
         * `canUse` as well, so a refused goal never starts and the enderman carries on with
         * its others, where skipping `tick` would leave a goal that started with nothing to
         * do. The wither breaks its walls through a function of its own, which is the third
         * hook. The hook points are the ones iListenAttentively raises its mob events at.
         */

        /** Whether MobGriefing is off for the dimension this mob stands in. A mob whose
         *  dimension cannot be read is not refused, as in the ride rule. */
        bool griefingRefused(::Actor& mob)
        {
            int dim = -1;
            try
            {
                dim = static_cast<int>(mob.getDimensionId());
            }
            catch (...)
            {
                dim = -1;
            }
            return dim >= 0 && anyRuleFor(dim) && !allowed(dim, DimRule::MobGriefing);
        }

        LL_TYPE_INSTANCE_HOOK(
            DimRuleTakeBlockHook,
            ll::memory::HookPriority::Normal,
            TakeBlockGoal,
            &TakeBlockGoal::$canUse,
            bool
        )
        {
            ::Mob& mob = this->mMob;
            if (griefingRefused(mob)) return false;
            return origin();
        }

        LL_TYPE_INSTANCE_HOOK(
            DimRulePlaceBlockHook,
            ll::memory::HookPriority::Normal,
            PlaceBlockGoal,
            &PlaceBlockGoal::$canUse,
            bool
        )
        {
            ::Mob& mob = this->mMob;
            if (griefingRefused(mob)) return false;
            return origin();
        }

        LL_TYPE_INSTANCE_HOOK(
            DimRuleWitherDestroyHook,
            ll::memory::HookPriority::Normal,
            WitherBoss,
            &WitherBoss::_destroyBlocks,
            void,
            ::Level& level,
            ::AABB const& bb,
            ::BlockSource& region,
            int range,
            ::WitherBoss::WitherAttackType attackType
        )
        {
            if (griefingRefused(*this)) return;
            origin(level, bb, region, range, attackType);
        }

        /** Whether the actor behind a griefing event stands in a dimension that forbids it.
         *  An actor that is already gone is not refused. */
        bool griefingEventRefused(::ActorGriefingBlockEvent const& griefing)
        {
            auto actor = griefing.mActorContext->tryUnwrap<::Actor>();
            return actor && griefingRefused(*actor);
        }

        // _processEvent is a template that the engine instantiates once per kind of event
        // list. The parameter types below pick the instantiation that carries the events a
        // listener may answer with Cancel, which holds the griefing event. Refusing calls
        // origin first, so the listener still hears of the event, and then answers Cancel.
        LL_TYPE_INSTANCE_HOOK(
            DimRuleGriefingEventHook,
            ll::memory::HookPriority::Normal,
            EventCoordinatorPimpl<ActorEventListener>,
            &EventCoordinatorPimpl<ActorEventListener>::_processEvent,
            ::CoordinatorResult,
            ::ActorGameplayHandler* handler,
            ::ActorGameplayEvent<::CoordinatorResult> const& event
        )
        {
            bool refuse = false;
            try
            {
                event.visit(
                    [&](auto&& arg)
                    {
                        using Wrapped = std::decay_t<decltype(arg)>;
                        if constexpr (std::is_same_v<
                                          Wrapped,
                                          ::Details::ValueOrRef<::ActorGriefingBlockEvent const>>)
                        {
                            refuse = griefingEventRefused(arg.value());
                        }
                    });
            }
            catch (...)
            {
                // An event that cannot be read is not refused. The rule is about a dimension
                // that is known, and a mob that is gone has none.
                refuse = false;
            }
            if (refuse)
            {
                origin(handler, event);
                return ::CoordinatorResult::Cancel;
            }
            return origin(handler, event);
        }

        /**
         * Installs one rule hook and logs a nonzero answer with the engine function it was
         * for. The hooks are installed one by one and not through `HookRegistrar`, whose
         * `hook()` returns nothing: a hook that did not take then left no line anywhere, and
         * the switches it serves did nothing with no sign of why.
         */
        template <class Hook>
        bool installRuleHook(char const* engineFunction)
        {
            int const code = Hook::hook();
            if (code == 0) return true;
            hostLogger().error(
                "[dim] {}", pier::trf("dim.dimension_rules.3", engineFunction, code));
            return false;
        }

        std::mutex& installMutex()
        {
            static std::mutex m;
            return m;
        }

        /** Whether the hooks have been installed. Read and written under installMutex. */
        bool gInstalled = false;
    } // namespace

    void registerDimensionRuleHooks()
    {
        std::lock_guard lock{installMutex()};
        if (gInstalled) return;
        gInstalled = true;
        int failed = 0;
        auto const count = [&failed](bool installed) { failed += installed ? 0 : 1; };
        count(installRuleHook<DimRuleSpawnMobHook>("Spawner::spawnMob"));
        count(installRuleHook<DimRuleSpawnProjectileHook>("Spawner::spawnProjectile"));
        count(installRuleHook<DimRuleExplodeHook>("Explosion::explode"));
        count(installRuleHook<DimRuleFireSpreadHook>("FireBlock::checkBurn"));
        count(installRuleHook<DimRuleFireTickHook>("FireBlock::tick"));
        count(installRuleHook<DimRuleFirePlaceHook>("FireBlock::onPlace"));
        count(installRuleHook<DimRuleFireMayPlaceHook>("FireBlock::mayPlace"));
        count(installRuleHook<DimRuleLiquidFlowHook>("LiquidBlock::_trySpreadTo"));
        count(installRuleHook<DimRuleFarmlandHook>("FarmBlock::transformOnFall"));
        count(installRuleHook<DimRulePistonHook>("PistonBlockActor::_checkAttachedBlocks"));
        count(installRuleHook<DimRuleRideHook>("Actor::canAddPassenger"));
        count(installRuleHook<DimRuleTakeBlockHook>("TakeBlockGoal::canUse"));
        count(installRuleHook<DimRulePlaceBlockHook>("PlaceBlockGoal::canUse"));
        count(installRuleHook<DimRuleWitherDestroyHook>("WitherBoss::_destroyBlocks"));
        count(installRuleHook<DimRuleGriefingEventHook>("EventCoordinatorPimpl<ActorEventListener>::_processEvent"));
        if (failed != 0) return;
        // The counts are stated exactly: 15 hook points covering 12 rules, with the 13th,
        // EntityCrossCell, in CellConfine.cpp. A log line whose numbers do not add up
        // sends whoever reads it counting in the wrong place.
        hostLogger().debug("[dim] per-dimension behavior rules enabled: 15 hook points covering 12 rules");
    }

    void unregisterDimensionRuleHooks()
    {
        std::lock_guard lock{installMutex()};
        if (!gInstalled) return;
        DimRuleSpawnMobHook::unhook();
        DimRuleSpawnProjectileHook::unhook();
        DimRuleExplodeHook::unhook();
        DimRuleFireSpreadHook::unhook();
        DimRuleFireTickHook::unhook();
        DimRuleFirePlaceHook::unhook();
        DimRuleFireMayPlaceHook::unhook();
        DimRuleLiquidFlowHook::unhook();
        DimRuleFarmlandHook::unhook();
        DimRulePistonHook::unhook();
        DimRuleRideHook::unhook();
        DimRuleTakeBlockHook::unhook();
        DimRulePlaceBlockHook::unhook();
        DimRuleWitherDestroyHook::unhook();
        DimRuleGriefingEventHook::unhook();
        gInstalled = false;
    }
} // namespace pier::dimensions
