/**
 * PlotConfine.cpp —— 地皮边界约束：网格表、合并图、实体拦截。
 *
 * 设计说明在 plot_confine.h，这里只补三条实现上的取舍。锁与 DimensionRules.cpp 同一个判断：写只发生在世界注册或合并变化时，读在 tick 路
 * 径上，但一次 Actor::move 本就要跑碰撞盒求交、方块查询、伤害判定，一次未争用的
 * mutex 淹没在噪音里。真正省事的是最外层那个原子计数：一个地皮世界都没有时连锁都不
 * 取。「读多写少所以上无锁结构」是感觉不是测量结果。
 *
 * 组根缓存整表清空，不做失效分析：合并表是整表替换推过来的（见 setPlotMerges），加
 * 一条边可能把两个几百块的组并成一个，没有便宜的增量答案。整表清空 O(1)。
 *
 * 组遍历上界 4096，与模组侧一致，但撞上界时行为故意不同：模组侧是给标题去重用的，
 * 走到哪算哪；这里是安全判定，走不完即不知道两块在不在一个组里，必须拒绝，否则把
 * 地皮合到 4096 块以上就能把约束整个关掉。
 */
#include "pier/dimensions/plot/plot_confine.h"

#include <atomic>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/Actor.h"

#include "pier/dimensions/dim/dimension_rules.h"
#include "pier/support/log.h"

namespace pier::dimensions
{
    namespace
    {
        using ::pier::hostLogger;

        constexpr int kGroupScanLimit = 4096;

        /** (x, z) 打包成一个键。两个 int32 拼进一个 uint64，无碰撞。 */
        constexpr uint64_t pk(int32_t x, int32_t z)
        {
            return (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32)
                | static_cast<uint64_t>(static_cast<uint32_t>(z));
        }

        constexpr PlotXZ unpk(uint64_t k)
        {
            return PlotXZ{
                static_cast<int32_t>(static_cast<uint32_t>(k >> 32)),
                static_cast<int32_t>(static_cast<uint32_t>(k & 0xFFFFFFFFull))
            };
        }

        /** 一个维度的网格 + 合并表 + 组根备忘。 */
        struct DimGrid
        {
            int plotSize{0};
            int roadWidth{0};
            /** 只存有合并标记的地皮：key = pk(x,z)，value = MergeBit 的按位或。 */
            std::unordered_map<uint64_t, uint32_t> merges;
            /** 组根备忘。合并表一变就整表清空。 */
            std::unordered_map<uint64_t, uint64_t> roots;
            /** 组遍历撞上界的地皮，记住以便一直保守拒绝而不是每次重走一遍图。 */
            std::unordered_set<uint64_t> oversized;
        };

        std::mutex& gridMutex()
        {
            static std::mutex m;
            return m;
        }

        std::unordered_map<int, DimGrid>& grids()
        {
            static std::unordered_map<int, DimGrid> m;
            return m;
        }

        /**
         * 已注册网格的维度数。最外层的无锁快速路径。
         *
         * 没有任何地皮世界时，`Actor::move` 的 hook 只读一个 relaxed 原子就
         * 返回，这正是「装了 loader 但不用地皮系统」的服务器该付的代价：零。
         */
        std::atomic<int> gGridCount{0};

        /** `Actor::move` 的 detour 是否已经装上。装上之后不再拆，见下方说明。 */
        std::atomic<bool> gMoveHookInstalled{false};

        void installMoveHookOnce();

        //  网格几何

        int positiveModLocal(int value, int modulus)
        {
            int r = value % modulus;
            return r < 0 ? r + modulus : r;
        }

        int floorDivLocal(int value, int divisor)
        {
            int q = value / divisor;
            if ((value % divisor != 0) && ((value < 0) != (divisor < 0))) --q;
            return q;
        }

        constexpr uint32_t bitOf(int dir)
        {
            switch (dir)
            {
            case 0: return kMergeNorth;
            case 1: return kMergeEast;
            case 2: return kMergeSouth;
            default: return kMergeWest;
            }
        }

        constexpr PlotXZ neighbourOf(PlotXZ id, int dir)
        {
            switch (dir)
            {
            case 0: return PlotXZ{id.x, id.z - 1};
            case 1: return PlotXZ{id.x + 1, id.z};
            case 2: return PlotXZ{id.x, id.z + 1};
            default: return PlotXZ{id.x - 1, id.z};
            }
        }

        /** 这块地皮自己声明了朝 `dir` 合并。 */
        bool claims(DimGrid const& g, PlotXZ id, int dir)
        {
            auto it = g.merges.find(pk(id.x, id.z));
            return it != g.merges.end() && (it->second & bitOf(dir)) != 0;
        }

        /**
         * 两块相邻地皮是否连通。判据是任一侧声明，不是两侧都声明。
         *
         * 和模组侧的 `connected` 逐字一致，理由也一样：unlink 是先清邻居再存
         * 自己，中间那次写盘失败会留下单边标记。用「与」的话，从两侧出发会走出
         * 两个不同的集合，组根就不唯一了 —— 而组根不唯一，「同一片区域」这个
         * 问题就没有稳定答案，同一次活塞推动会时而被拦时而放行。
         */
        bool connected(DimGrid const& g, PlotXZ a, int dir)
        {
            return claims(g, a, dir) || claims(g, neighbourOf(a, dir), (dir + 2) % 4);
        }

        /** `a` 在 `PlotId` 的 Ord 意义下更小（先比 x 再比 z）。和模组侧一致。 */
        constexpr bool lessThan(PlotXZ a, PlotXZ b)
        {
            return a.x != b.x ? a.x < b.x : a.z < b.z;
        }

        /**
         * 合并组的代表编号。走不完（撞上界）时返回 false —— 调用方必须据此拒绝。
         *
         * 绝大多数地皮没合并过：四周都不连通就直接返回自己，省掉一次容器分配。
         * 「自己没标合并」不够，邻居仍可能单边标着，所以四个方向都要按
         * `connected` 问一遍。
         */
        bool groupRootLocked(DimGrid& g, PlotXZ id, PlotXZ* out)
        {
            uint64_t const key = pk(id.x, id.z);
            if (g.oversized.count(key) != 0) return false;
            if (auto it = g.roots.find(key); it != g.roots.end())
            {
                *out = unpk(it->second);
                return true;
            }

            bool anyEdge = false;
            for (int d = 0; d < 4; ++d)
            {
                if (connected(g, id, d))
                {
                    anyEdge = true;
                    break;
                }
            }
            if (!anyEdge)
            {
                g.roots.emplace(key, key);
                *out = id;
                return true;
            }

            std::unordered_set<uint64_t> seen{key};
            std::vector<PlotXZ> stack{id};
            std::vector<uint64_t> members{key};
            PlotXZ best = id;

            while (!stack.empty())
            {
                PlotXZ cur = stack.back();
                stack.pop_back();
                for (int d = 0; d < 4; ++d)
                {
                    if (!connected(g, cur, d)) continue;
                    PlotXZ nb = neighbourOf(cur, d);
                    uint64_t nk = pk(nb.x, nb.z);
                    if (!seen.insert(nk).second) continue;
                    if (members.size() >= static_cast<size_t>(kGroupScanLimit))
                    {
                        // 走不完 = 不知道。把整个已访问集合都标成 oversized，
                        // 否则从组里另一块进来又要重走一遍这张走不完的图。
                        for (uint64_t m : seen) g.oversized.insert(m);
                        hostLogger().warn(
                            "地皮合并组超过 {} 块（从 {};{} 出发），跨界判定对这一组"
                            "一律拒绝。这不是配置问题 —— 组这么大时「同一片区域」已经没有"
                            "可负担的答案，而安全判定不知道的时候必须拒绝。",
                            kGroupScanLimit, id.x, id.z
                        );
                        return false;
                    }
                    members.push_back(nk);
                    stack.push_back(nb);
                    if (lessThan(nb, best)) best = nb;
                }
            }

            uint64_t const rootKey = pk(best.x, best.z);
            // 整组一次写完：组里任意一块之后进来都直接命中。
            for (uint64_t m : members) g.roots[m] = rootKey;
            *out = best;
            return true;
        }

        /**
         * 一格归哪块地皮。必须和模组侧的 `owning_plot` 逐条一致。
         *
         * 返回 false = 这一格不属于任何地皮（在道路上）。
         */
        bool owningPlotLocked(DimGrid const& g, int x, int z, PlotXZ* out)
        {
            int const cell = g.plotSize + g.roadWidth;
            if (cell <= 0) return false;
            int const px = floorDivLocal(x, cell);
            int const pz = floorDivLocal(z, cell);
            bool const onRoadX = positiveModLocal(x, cell) >= g.plotSize;
            bool const onRoadZ = positiveModLocal(z, cell) >= g.plotSize;

            PlotXZ const base{px, pz};
            if (!onRoadX && !onRoadZ)
            {
                *out = base;
                return true;
            }
            if (onRoadX && !onRoadZ)
            {
                // 南北走向的缝，分隔 base 和它东边的邻居。
                if (!connected(g, base, 1)) return false;
                *out = base;
                return true;
            }
            if (!onRoadX && onRoadZ)
            {
                // 东西走向的缝，分隔 base 和它南边的邻居。
                if (!connected(g, base, 2)) return false;
                *out = base;
                return true;
            }
            // 路口：围它的 2×2 四条边全合并才算地皮内部。
            PlotXZ const ne = neighbourOf(base, 1);
            PlotXZ const sw = neighbourOf(base, 2);
            if (!(connected(g, base, 1) && connected(g, base, 2) && connected(g, ne, 2)
                  && connected(g, sw, 1)))
            {
                return false;
            }
            *out = base;
            return true;
        }
    } // namespace

    //  对外接口

    void setPlotGrid(int dimension, int plotSize, int roadWidth)
    {
        if (plotSize <= 0)
        {
            clearPlotGrid(dimension);
            return;
        }
        // 永远不要相信调用方给的数值：负的 roadWidth 会让 cell 变成 0 甚至负数，
        // 而 cell 是取模的除数。这一条和 PlotLayout::clamp 是同一个理由。
        if (roadWidth < 0) roadWidth = 0;
        if (plotSize > 512) plotSize = 512;
        if (roadWidth > 64) roadWidth = 64;

        std::lock_guard lock{gridMutex()};
        auto [it, inserted] = grids().try_emplace(dimension);
        if (inserted) gGridCount.fetch_add(1, std::memory_order_relaxed);
        auto& g = it->second;
        if (g.plotSize != plotSize || g.roadWidth != roadWidth)
        {
            g.plotSize = plotSize;
            g.roadWidth = roadWidth;
            // 几何变了，归属判定的答案就全变了。合并表本身没变，但备忘要清。
            g.roots.clear();
            g.oversized.clear();
        }
        installMoveHookOnce();
    }

    void clearPlotGrid(int dimension)
    {
        std::lock_guard lock{gridMutex()};
        if (grids().erase(dimension) > 0)
        {
            gGridCount.fetch_sub(1, std::memory_order_relaxed);
        }
    }

    void setPlotMerges(int dimension, int32_t const* entries, int32_t count)
    {
        if (count < 0) count = 0;
        if (entries == nullptr) count = 0;
        std::lock_guard lock{gridMutex()};
        auto it = grids().find(dimension);
        if (it == grids().end())
        {
            // 网格还没注册就推合并表：丢掉并说清楚。静默接受会更糟 ——
            // 表存下来了、几何却是空的，归属判定永远返回「不在地皮上」，
            // 表现是「合并了但活塞还是推不过去」。
            hostLogger().warn(
                "维度 {} 还没有注册地皮网格就收到了合并表（{} 条），已忽略。"
                "调用顺序应该是先 set_plot_grid 再 set_plot_merges。",
                dimension, count
            );
            return;
        }
        auto& g = it->second;
        g.merges.clear();
        g.roots.clear();
        g.oversized.clear();
        for (int32_t i = 0; i < count; ++i)
        {
            int32_t const x = entries[i * 3 + 0];
            int32_t const z = entries[i * 3 + 1];
            auto const mask = static_cast<uint32_t>(entries[i * 3 + 2]) & 0xFu;
            if (mask == 0) continue; // 没有标记的条目不占地方
            g.merges[pk(x, z)] |= mask;
        }
    }

    bool hasPlotGrid(int dimension)
    {
        if (gGridCount.load(std::memory_order_relaxed) == 0) return false;
        std::lock_guard lock{gridMutex()};
        auto it = grids().find(dimension);
        return it != grids().end() && it->second.plotSize > 0;
    }

    bool owningPlot(int dimension, int x, int z, PlotXZ* out)
    {
        if (gGridCount.load(std::memory_order_relaxed) == 0) return false;
        std::lock_guard lock{gridMutex()};
        auto it = grids().find(dimension);
        if (it == grids().end()) return false;
        PlotXZ id{};
        if (!owningPlotLocked(it->second, x, z, &id)) return false;
        if (out) *out = id;
        return true;
    }

    bool sameArea(int dimension, int x1, int z1, int x2, int z2)
    {
        if (gGridCount.load(std::memory_order_relaxed) == 0) return true;
        std::lock_guard lock{gridMutex()};
        auto it = grids().find(dimension);
        if (it == grids().end()) return true;
        auto& g = it->second;

        PlotXZ a{}, b{};
        bool const inA = owningPlotLocked(g, x1, z1, &a);
        bool const inB = owningPlotLocked(g, x2, z2, &b);
        // 都在道路上：道路是公共区域，在上面移动不算越界。
        if (!inA && !inB) return true;
        // 一侧在地皮上、一侧不在：这就是边界。方向对称 —— 推出去和推进来
        // 是同一类操作，只拦一个方向等于没拦。
        if (inA != inB) return false;
        // 同一块，最常见的一路，不必走图。
        if (a == b) return true;

        PlotXZ ra{}, rb{};
        if (!groupRootLocked(g, a, &ra)) return false;
        if (!groupRootLocked(g, b, &rb)) return false;
        return ra == rb;
    }

    namespace
    {
        /**
         * 实体越界拦截。
         *
         * 挂 Actor::move(Vec3 const& posDelta)，不挂 tick：tick 是每个实体子类各自
         * 实现的，挂不全，而 move 是它们最后都要走的那一步。这是能力声明不是保证，
         * 某个实体类型若自己改 mPos 而不走 move 就不受约束，那是缺口不是崩溃。
         *
         * 三类实体不管：玩家（约束玩家是另一件事，默认打开就是灾难）、载人的载具
         * （玩家划船走到边被焊在原地比越界严重得多；代价是空船会被拦，但载具搬不动
         * 方块）、已被移除的实体。
         *
         * 拦下时只清水平位移、保留竖直分量。全清会让卡在边界上的实体悬空不落，看起
         * 来像服务器卡了；保留 y 之后它是撞在一堵看不见的墙上，那是玩家认得的行为。
         */
        LL_TYPE_INSTANCE_HOOK(
            PlotConfineActorMoveHook,
            ll::memory::HookPriority::Normal,
            Actor,
            &Actor::move,
            void,
            ::Vec3 const& posDelta
        )
        {
            // 最外层：一个地皮世界都没有 → 一次 relaxed 读，走人。
            if (gGridCount.load(std::memory_order_relaxed) == 0)
            {
                return origin(posDelta);
            }
            // 没有水平位移就没有跨界的可能。地上的掉落物、盔甲架、画框走这条，
            // 而它们通常占实体总数的大头。
            if (posDelta.x == 0.0f && posDelta.z == 0.0f)
            {
                return origin(posDelta);
            }

            int dim = -1;
            bool isPlayerActor = true;
            bool ridden = true;
            ::Vec3 from{};
            try
            {
                isPlayerActor = this->isPlayer();
                if (!isPlayerActor)
                {
                    dim = static_cast<int>(this->getDimensionId());
                    ridden = this->hasPassenger() || this->isRemoved();
                    from = this->getPosition();
                }
            }
            catch (...)
            {
                // 读不到状态就别拦。这里拦错的代价（实体永久卡住）大于放行的
                // 代价（一次越界）。初值也是按这个方向设的：isPlayerActor 和
                // ridden 都初始化成 true，任何一条提前抛出都会落到「不管」。
                return origin(posDelta);
            }
            if (isPlayerActor || ridden || dim < 0)
            {
                return origin(posDelta);
            }

            bool allowCross = true;
            bool const hasRule =
                getDimensionRule(dim, static_cast<int>(DimRule::EntityCrossPlot), &allowCross);
            bool const hasGrid = hasPlotGrid(dim);

            if (!hasRule || allowCross || !hasGrid)
            {
                return origin(posDelta);
            }

            auto blockOf = [](float v) { return static_cast<int>(std::floor(v)); };
            int const fx = blockOf(from.x);
            int const fz = blockOf(from.z);
            int const tx = blockOf(from.x + posDelta.x);
            int const tz = blockOf(from.z + posDelta.z);
            if ((fx == tx && fz == tz) || sameArea(dim, fx, fz, tx, tz))
            {
                return origin(posDelta);
            }

            ::Vec3 clamped = posDelta;
            clamped.x = 0.0f;
            clamped.z = 0.0f;
            origin(clamped);
            // 速度也要清，否则下一拍会带着同样的水平速度再撞一次，实体在边界上
            // 抖动而不是停住。竖直速度保留 —— 它还得往下掉。
            //
            // 走 `getPosDelta()` 的 const_cast 而不是 `setPosDelta` /
            // `getPosDeltaNonConst`：前者是 Actor.h 里直接给出定义的内联
            // 访问器（读 mBuiltInComponents->mStateVectorComponent->mPosDelta），
            // 编译期就能展开；后两个是 MCFOLD，要在运行时解析符号，多一个会因
            // 版本漂移而失败的环节，而它们做的事完全一样。
            try
            {
                auto& delta = const_cast<::Vec3&>(this->getPosDelta());
                delta.x = 0.0f;
                delta.z = 0.0f;
            }
            catch (...)
            {
                // 清速度失败只影响手感（实体会在边界上抖一下而不是停住），
                // 位移本身已经被上面那次 origin(clamped) 拦住了 —— 安全性质
                // 不依赖这一步。所以这里故意不升级成 error：每 tick 一条
                // 日志会把真问题淹掉，而这条路径每 tick 都跑。
            }
        }

        /**
         * 第一次注册地皮网格时装上，之后永不拆。
         *
         * 和 hooks/ 目录下每个文件同一条纪律：退订可能来自被 hook 的函数内部，
         * 在那里拆补丁是不安全的。闲置的 hook 只多一次原子读。
         *
         * 调用点已经持有 gridMutex，所以这里不再取锁。
         */
        void installMoveHookOnce()
        {
            if (gMoveHookInstalled.load(std::memory_order_relaxed)) return;
            int const r = PlotConfineActorMoveHook::hook();
            gMoveHookInstalled.store(true, std::memory_order_relaxed);
            if (r == 0)
            {
                hostLogger().debug("地皮边界约束已启用");
            }
            else
            {
                hostLogger().error(
                    "地皮边界约束：Actor::move 的 detour 安装失败（状态码 {}）。"
                    "最常见原因是本 loader 链接的 BDS/LeviLamina 版本和服务器实际运行的"
                    "不一致，符号地址解析错误。结果：**实体越界完全不受约束**"
                    "（活塞那条路不受影响，它挂在另一个符号上）。",
                    r
                );
            }
        }
    } // namespace
} // namespace pier::dimensions
