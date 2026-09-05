/**
 * DimensionRules.cpp: behavior rules that apply per dimension.
 * A Bedrock gamerule is one value for the whole server: keeping mobs out of a creative
 * plot world means doMobSpawning=false, which also empties the survival world on the
 * same server. The functions that do the work are hooked instead, such as
 * Spawner::spawnMob and Level::explode. Each carries a BlockSource that yields a
 * dimension id, so the decision really is per dimension.
 * The rule table is sparse by dimension id and a miss goes straight to origin(). These
 * hooks are installed globally and must never change the behavior of an unmanaged
 * dimension, so a caller enables nothing for the vanilla dimensions.
 * Eight hook points cover twelve rules. SpawnMonster, SpawnAnimal and SpawnSpawner
 * share Spawner::spawnMob, ExplodeBlocks and MobGriefing share Level::explode, and
 * PistonPush and PistonCrossPlot share PistonBlockActor::_checkAttachedBlocks.
 * EntityCrossPlot is not in this file; PlotConfine.cpp implements it on its own hook.
 */
#include "pier/dimensions/dim/dimension_rules.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <unordered_map>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/IRandom.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorCategory.h"
#include "mc/world/actor/ActorDefinitionIdentifier.h"
#include "mc/world/actor/Mob.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/Spawner.h"
#include "mc/world/level/block/FarmBlock.h"
#include "mc/world/level/block/FireBlock.h"
#include "mc/world/level/block/LiquidBlock.h"
#include "mc/world/level/block/actor/PistonBlockActor.h"

#include "sdk/abi.h"

#include "pier/dimensions/plot/plot_confine.h"
#include "pier/support/log.h"

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
    static_assert(static_cast<int>(DimRule::PistonCrossPlot) == PIER_DIMRULE_PISTON_CROSS_PLOT);
    static_assert(static_cast<int>(DimRule::EntityCrossPlot) == PIER_DIMRULE_ENTITY_CROSS_PLOT);
    static_assert(kDimRuleCount == PIER_DIMRULE_ENTITY_CROSS_PLOT + 1, "a rule was appended without updating the count");

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
                "[dim] set_dimension_rule(dim={}, rule={}): the rule number is outside the "
                "range [0,{}) this host supports and is ignored; if the rule is new, the mod "
                "was built against a newer ABI and the pier host needs upgrading",
                dimension, rule, kDimRuleCount
            );
            return;
        }
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
                        hostLogger().warn("[dim] removing a spawned mob for a dimension rule threw, dim {}", dim);
                    }
                    return nullptr;
                }
                return mob;
            }

            // Command, spawn egg and breeding are not blocked.
            return origin(region, id, spawner, pos, naturalSpawn, surface, fromSpawner);
        }

        //  Projectiles

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
         * The key trade: the explosion is not cancelled, only its block-breaking bit is
         * turned off. Level::explode takes a breaksBlocks parameter, and false means it
         * still bangs and still hurts but leaves no crater. Cancelling the whole thing
         * would also eat the damage and the particles, and a player would conclude the
         * creeper is broken.
         * ExplodeBlocks = false stops any explosion from breaking blocks, TNT included.
         * MobGriefing = false stops only mob-caused explosions, while TNT a player lit
         * still breaks blocks. With both set, either one forbidding is enough.
         * Only the overload carrying a BlockSource is hooked. The Explosion passed to
         * $explode(Explosion&) has no BlockSource member, only mPos and mSourceID, so the
         * dimension cannot be obtained there.
         */
        LL_TYPE_INSTANCE_HOOK(
            DimRuleExplodeHook,
            ll::memory::HookPriority::Normal,
            Level,
            &Level::$explode,
            bool,
            ::BlockSource& region,
            ::Actor* source,
            ::Vec3 const& pos,
            float explosionRadius,
            bool fire,
            bool breaksBlocks,
            float maxResistance,
            bool allowUnderwater
        )
        {
            int const dim = dimOf(region);
            if (dim < 0 || !anyRuleFor(dim) || !breaksBlocks)
            {
                return origin(
                    region, source, pos, explosionRadius, fire, breaksBlocks, maxResistance, allowUnderwater
                );
            }

            bool allowBreak = allowed(dim, DimRule::ExplodeBlocks);

            // A mob-caused explosion is additionally subject to MobGriefing. TNT lit by
            // a player is not: with source null, meaning the TNT block itself, or a
            // player, only ExplodeBlocks applies.
            if (allowBreak && source != nullptr)
            {
                bool isMob = false;
                try
                {
                    isMob = source->hasCategory(::ActorCategory::Mob)
                        && !source->hasCategory(::ActorCategory::Player);
                }
                catch (...)
                {
                    // An undecidable category counts as not a mob. That path only lets
                    // the explosion break blocks as it otherwise would, applying no extra
                    // MobGriefing, which is the more conservative side of the two rules
                    // since ExplodeBlocks still governs it.
                    isMob = false;
                }
                if (isMob && !allowed(dim, DimRule::MobGriefing))
                {
                    allowBreak = false;
                }
            }

            return origin(
                region, source, pos, explosionRadius, fire, allowBreak, maxResistance, allowUnderwater
            );
        }

        //  Fire spread

        /*
         * `FireBlock::checkBurn` is the step where fire spreads to a neighbor. Blocking
         * it leaves an already burning fire burning, still consuming its own cell, while
         * it no longer creeps sideways. That is what fire spread should mean, rather than
         * fire not existing.
         *
         * This is a const member function. A const hook is known to work in this project;
         * the packet hooks in ChunkTrace are const too.
         */
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
         * PistonPush and PistonCrossPlot share this hook, the first applying to the whole
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
            if (!managed || allowed(dim, DimRule::PistonCrossPlot) || !hasPlotGrid(dim))
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

        using DimRuleHookReg = ll::memory::HookRegistrar<
            DimRuleSpawnMobHook,
            DimRuleSpawnProjectileHook,
            DimRuleExplodeHook,
            DimRuleFireSpreadHook,
            DimRuleLiquidFlowHook,
            DimRuleFarmlandHook,
            DimRulePistonHook,
            DimRuleRideHook>;

        bool gInstalled = false;
    } // namespace

    void registerDimensionRuleHooks()
    {
        if (gInstalled) return;
        DimRuleHookReg::hook();
        gInstalled = true;
        // The counts are stated exactly: 8 hook points covering 12 rules, with the 13th,
        // EntityCrossPlot, in PlotConfine.cpp. A log line whose numbers do not add up
        // sends whoever reads it counting in the wrong place.
        hostLogger().debug("[dim] per-dimension behavior rules enabled: 8 hook points covering 12 rules");
    }

    void unregisterDimensionRuleHooks()
    {
        if (!gInstalled) return;
        DimRuleHookReg::unhook();
        gInstalled = false;
    }
} // namespace pier::dimensions
