/**
 * DimensionRules.cpp —— 按维度生效的行为规则。
 *
 * # 为什么不用 gamerule
 *
 * 基岩版的 gamerule 是**全服一份**的。想让创造用的地皮世界不刷怪，只能
 * `doMobSpawning=false`，而这会连带把同一个服务器上的生存世界也变成空城。
 * 之前那一版就是这么做的（在玩家进世界时整体切 gamerule），代价是两个玩家
 * 分处不同世界时会互相干扰。
 *
 * 这里换成钩住**真正干活的那几个函数**：`Spawner::spawnMob`、`Level::explode`
 * 之类。这些函数都带着一个 `BlockSource`，从中拿得到维度 id，于是判定可以
 * 真正做到按维度隔离。参考实现：IceBlcokMC/PlotX 的 PermCore 也是这么做的 ——
 * 它把 `preCheck(BlockSource&, BlockPos const&)` 作为所有拦截的入口，第一件事
 * 就是比对维度 id。
 *
 * # 没有条目的维度完全不受影响
 *
 * 规则表按维度 id 稀疏存储。查不到就直接 `origin()`，原版维度保持原版行为，
 * 调用方不需要为它们显式「开启」任何东西。这一点很重要：这些 hook 是全局装
 * 上去的，绝不能因为装了 hook 就改变没被管理的维度的行为。
 *
 * # 八个挂载点覆盖十二条规则
 *
 * 数目对不上是正常的：`SpawnMonster` / `SpawnAnimal` / `SpawnSpawner` 共用
 * `Spawner::spawnMob` 一个点，`ExplodeBlocks` / `MobGriefing` 共用
 * `Level::explode`，`PistonPush` / `PistonCrossPlot` 共用
 * `PistonBlockActor::_checkAttachedBlocks`。`EntityCrossPlot` 不在本文件 ——
 * 它由 PlotConfine.cpp 自己的挂载点实现。
 */
#include "pier/dimensions/dim/dimension_rules.h"

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
    // 编号必须和 ABI 逐值一致。写两遍是不得已（这一侧要有名字），但**不能**
    // 只靠人来保持同步 —— 错位之后的症状是「设了不刷怪、关掉的却是火焰蔓延」，
    // 从现象完全看不出根因在编号上。所以钉在编译期。
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
    static_assert(kDimRuleCount == PIER_DIMRULE_ENTITY_CROSS_PLOT + 1, "追加了规则却没改计数");

    namespace
    {
        using ::pier::hostLogger;

        /**
         * (维度 id, 规则) -> 允许与否。**只存显式设过的项**。
         *
         * 用 mutex 而不是无锁结构：写入只发生在世界创建/加载时（每服几十次），
         * 读取在 spawn 路径上（每秒几百次）。读多写少，但读的绝对量也不大 ——
         * 一次 spawn 已经要走一大堆引擎逻辑，一次 mutex lock 淹没在噪音里。
         * 真要优化也该先测，别凭感觉上无锁。
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

        /** 该维度是否有**任何**规则。没有就整个 hook 走快速路径。 */
        std::unordered_map<int, int>& dimCounts()
        {
            static std::unordered_map<int, int> m;
            return m;
        }

        bool anyRuleFor(int dimension)
        {
            std::lock_guard lock{rulesMutex()};
            auto it = dimCounts().find(dimension);
            return it != dimCounts().end() && it->second > 0;
        }
    } // namespace

    void setDimensionRule(int dimension, int rule, bool allow)
    {
        if (rule < 0 || rule >= kDimRuleCount)
        {
            // 越界的规则号只可能来自调用方的 bug 或者一个比宿主更新的 SDK。
            // 静默忽略会让「我明明设了」变成一个查不到根因的报障。
            hostLogger().error(
                "set_dimension_rule(dim={}, rule={})：规则号超出本宿主支持的范围 [0,{})，忽略。"
                "如果这个规则是新加的，说明模组是按更新的 ABI 编的，请升级 pier。",
                dimension, rule, kDimRuleCount
            );
            return;
        }
        std::lock_guard lock{rulesMutex()};
        auto const k = key(dimension, rule);
        auto [it, inserted] = rules().insert_or_assign(k, allow);
        if (inserted) dimCounts()[dimension] += 1;
    }

    bool getDimensionRule(int dimension, int rule, bool* outAllow)
    {
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
    }

    namespace
    {
        /**
         * 查一条规则。没设过就返回 `fallback`（一律是 true = 按原版走）。
         *
         * **默认放行是这里的关键约束**：这些 hook 装在全局，任何一个没被管理
         * 的维度都必须完全感觉不到它们的存在。
         */
        bool allowed(int dimension, DimRule rule, bool fallback = true)
        {
            bool v = fallback;
            if (!getDimensionRule(dimension, static_cast<int>(rule), &v)) return fallback;
            return v;
        }

        /**
         * 取维度 id。**-1 是「问不出来」，不是某个维度**（契约 §5.2）——
         * 所有调用点都写成 `dim < 0 → 直接 origin()`，也就是「不知道就别管」。
         * 这是刻意的：一个读不出维度的 BlockSource 说明引擎状态已经不正常，
         * 这时候按某个猜出来的维度去执行保护规则，比不执行更危险。
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

        // ───────────────────── 生物生成 ─────────────────────

        /*
         * `Spawner::spawnMob` 是**所有**生成的必经之路：自然刷怪、刷怪笼、
         * 刷怪蛋、指令召唤都从这里过。所以要靠参数区分来源，别把玩家用刷怪蛋
         * 放的羊也拦掉。
         *
         *   naturalSpawn == true   自然生成 -> 按 SpawnMonster / SpawnAnimal
         *   fromSpawner  == true   刷怪笼   -> 按 SpawnSpawner
         *   两者都 false           指令/刷怪蛋/繁殖 -> **永远放行**
         *
         * 最后那条是有意的：玩家在创造世界里主动放生物是正常玩法，不该被
         * 「这个世界不刷怪」的设置拦住。
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
                // 敌对还是友好：先按类别判，判不出来就当敌对处理。
                // 宁可在创造世界少刷一只，也不要多刷一只打断建筑。
                auto* mob = origin(region, id, spawner, pos, naturalSpawn, surface, fromSpawner);
                if (!mob) return nullptr;

                bool const hostile = mob->hasCategory(::ActorCategory::Monster);
                bool const ok =
                    hostile ? allowed(dim, DimRule::SpawnMonster) : allowed(dim, DimRule::SpawnAnimal);
                if (!ok)
                {
                    // 已经生成出来了才知道类别，只能立刻移除。比在生成前猜类别
                    // 可靠 —— ActorDefinitionIdentifier 只有名字，把
                    // "minecraft:zombie" 之类硬编码成一张表迟早会漏。
                    try
                    {
                        // $despawn 是虚函数 thunk（Actor.h:1917）；
                        // 直接调 despawn() 在这套头文件里不存在。
                        mob->$despawn();
                    }
                    catch (...)
                    {
                        hostLogger().warn("按维度规则移除生成的生物时抛异常（dim {}）", dim);
                    }
                    return nullptr;
                }
                return mob;
            }

            // 指令 / 刷怪蛋 / 繁殖：不拦。
            return origin(region, id, spawner, pos, naturalSpawn, surface, fromSpawner);
        }

        // ───────────────────── 弹射物 ─────────────────────

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

        // ───────────────────── 爆炸 ─────────────────────

        /*
         * 关键取舍：**不取消爆炸，只把「破坏方块」这一位关掉**。
         *
         * `Level::explode` 有一个 `breaksBlocks` 参数，直接传 false 就是
         * 「炸得响、有伤害、但不留坑」。整个取消爆炸会连带吃掉伤害和粒子，
         * 玩家会觉得苦力怕失灵了；只关方块破坏才是「保护地形」该有的样子，
         * 也正是原版 mobGriefing 的语义。
         *
         * mobGriefing 和 explodeBlocks 的分工：
         *   ExplodeBlocks = false  -> 任何爆炸都不破坏方块（含 TNT）
         *   MobGriefing   = false  -> **只有生物引发的**爆炸不破坏方块，
         *                             玩家点的 TNT 照炸
         * 两个都设时，任意一个禁止就不破坏。
         *
         * 只钩了带 BlockSource 的那个重载。另一个 `$explode(Explosion&)` 的
         * Explosion 对象里没有 BlockSource 成员（只有 mPos / mSourceID），
         * 拿不到维度，所以钩它没有意义 —— 好在带参数的这个重载是主路径。
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

            // 生物引发的爆炸额外受 MobGriefing 约束。玩家点的 TNT 不算 ——
            // source 为空（TNT 方块自己）或者是玩家时，只看 ExplodeBlocks。
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
                    // 问不出类别就当它不是生物 —— 这条路只会让爆炸**照原样**
                    // 破坏方块（也就是不额外施加 MobGriefing），是这两条规则
                    // 里更保守的那一侧：ExplodeBlocks 仍然管着它。
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

        // ───────────────────── 火焰蔓延 ─────────────────────

        /*
         * `FireBlock::checkBurn` 是火向邻居扩散的那一步。拦住它，已经点着的火
         * 还会烧、还会烧掉自己那一格，但不会往旁边爬 —— 这正是「火焰蔓延」该有
         * 的语义，而不是「火焰不存在」。
         *
         * 注意这是 const 成员函数。本工程里 const hook 已经证实可用
         * （ChunkTrace 里那几个包 hook 就是）。
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

        // ───────────────────── 液体蔓延 ─────────────────────

        /*
         * `LiquidBlock::_trySpreadTo` 是水/岩浆向某一格扩散的那一步。拦住它，
         * 已经放下的液体源还在，但不会往外爬 —— 这正是 PlotSquared 里
         * `LiquidFlow` 那个 flag 的语义。
         *
         * 挂载点取自 LegacyScriptEngine 的 `onLiquidFlow`。
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

        // ───────────────────── 耕地被踩坏 ─────────────────────

        /*
         * `FarmBlock::$transformOnFall` 是「踩上去变回泥土」。地皮世界里这个
         * 很烦人 —— 别人从你的农场上跑过就毁一片。
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

        // ───────────────────── 活塞推动 ─────────────────────

        /*
         * `PistonBlockActor::_checkAttachedBlocks` 决定这次伸缩能不能带动附着
         * 的方块。返回 false = 推不动，活塞会卡住而不是把方块搬走。
         *
         * 为什么拦这里而不是拦活塞本身：活塞照常动、红石照常工作，只是搬不动
         * 方块。整个禁用活塞会把很多红石装置直接弄坏。
         *
         * 这一条对应 PlotSquared 的 `DisablePhysics` —— 防的是「用活塞把方块
         * 推过地皮边界」这种越界建造。
         *
         * ── PistonCrossPlot：同一个 hook 点，另一个问题 ──
         *
         * `PistonPush` 是整维度一刀切。「不出地皮」要的是按边界判：地皮内部照常
         * 推，跨界才拦。两者挂的是同一个函数，所以合在一个 hook 里而不是再装一
         * 个 detour —— 同一个符号上叠两层补丁没有任何好处，只是多一次间接跳转和
         * 一处「谁先跑」的不确定性。
         *
         * **必须先 `origin(region)`。** 要移动哪些方块是这个函数**算出来**的
         * （`_attachedBlockWalker` 往 `mAttachedBlocks` 里填），不跑就没有清单
         * 可查。跑完拿到 true 之后再逐块检查「现在这一格」和「落点那一格」是不
         * 是都和活塞自己在同一片区域，任何一块出界就整次拒绝 —— 部分放行会把一
         * 台飞行器撕成两半，那比推过去还糟。
         *
         * 拒绝时**不清 `mAttachedBlocks`**：返回 false 正是引擎自己「推不动」的
         * 出口（撞到基岩、超过 12 块都走这条），清单留在那里是它本来就有的状态，
         * 下一拍 walker 会重填。为了「看起来干净」去动引擎的成员，风险大于收益。
         */
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
            // 允许跨界（或这个维度没被管、没有网格）时到此为止，一格坐标都不算。
            if (!managed || allowed(dim, DimRule::PistonCrossPlot) || !hasPlotGrid(dim))
            {
                return true;
            }

            try
            {
                // 位置和清单走**成员**而不是 `getPosition()` / `getAttachedBlocks()`：
                // 那两个都是 MCFOLD，要在运行时解析符号，多一个会因版本漂移而失败
                // 的环节，而它们只是把这两个成员读出来。`getFacingDir` 不同 ——
                // 它要从方块状态算朝向，只能调。
                auto const& self = this->mPosition.get();
                auto const& facing = this->getFacingDir(region);
                for (auto const& b : this->mAttachedBlocks.get())
                {
                    // 起点和落点都要查。只查落点的话，把方块从别人地里**拉出来**
                    // （粘性活塞回缩）会被放行，而那和推进去是同一类越界。
                    if (!sameArea(dim, self.x, self.z, b.x, b.z)
                        || !sameArea(dim, self.x, self.z, b.x + facing.x, b.z + facing.z))
                    {
                        return false;
                    }
                }
            }
            catch (...)
            {
                // 读不到清单就保守拒绝。这是安全判定，不知道的时候放行等于没装。
                return false;
            }
            return true;
        }

        // ───────────────────── 乘坐载具 ─────────────────────

        /*
         * `Actor::$canAddPassenger` 是「这个东西能不能被骑」。挂在 Actor 上，
         * 所以船、矿车、马、猪一起覆盖。挂载点取自 LSE 的 `onRide`。
         *
         * 注意这里的维度是从**被骑的那个实体**取的，不是乘客 —— 两者一定在同
         * 一个维度，取哪个都行，取 this 少一次解引用。
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
                // 同 dimOf：-1 是「问不出来」，下面那行的 `dim >= 0` 会让整条
                // 规则失效、直接 origin()。读不出维度的实体多半正在被销毁，
                // 这时候按猜出来的维度去拦一次骑乘毫无价值。
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
        // 说数目就要说准：**8 个挂载点**，覆盖 12 条规则（第 13 条
        // EntityCrossPlot 在 PlotConfine.cpp）。旧版这里写的是「9 类」——
        // 两个数都对不上，而一条对不上号的日志会让人去数错的地方。
        hostLogger().debug("按维度生效的行为规则已启用：8 个挂载点，覆盖 12 条规则");
    }

    void unregisterDimensionRuleHooks()
    {
        if (!gInstalled) return;
        DimRuleHookReg::unhook();
        gInstalled = false;
    }
} // namespace pier::dimensions
