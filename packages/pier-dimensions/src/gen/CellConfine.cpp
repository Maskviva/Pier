/**
 * CellConfine.cpp: grid cell boundary confinement. Grid table, merge graph, actor interception.
 * The design is in cell_confine.h; three implementation trades are added here. The lock follows the
 * judgement DimensionRules.cpp makes: writes happen on registration or a merge change, reads on the
 * tick path, and one Actor::move already runs bounding box intersection, block queries and damage
 * decisions, so an uncontended mutex vanishes into that noise. What saves work is the outermost
 * atomic count: with no dimension registering a grid, no lock is taken. Reaching for a lock-free
 * structure because reads dominate is a feeling, not a measurement. The group root cache is
 * cleared whole with no invalidation analysis: the merge table arrives as a whole-table
 * replacement (see setCellMerges) and one edge can fuse two groups of hundreds of cells, so
 * no cheap incremental answer exists.
 * Clearing is O(1). The group walk is bounded at 4096 like the mod side, but hitting the bound
 * behaves differently on purpose: the mod side deduplicates a title and takes what it reached,
 * while this is a protection decision, and an unfinished walk means not knowing whether two cells
 * share a group, which must be refused. Otherwise merging past 4096 cells would switch the
 * confinement off entirely. / */
#include "pier/dimensions/gen/cell_confine.h"

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

        /** Packs (x, z) into one key. Two int32 into one uint64, without collision. */
        constexpr uint64_t pk(int32_t x, int32_t z)
        {
            return (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32)
                | static_cast<uint64_t>(static_cast<uint32_t>(z));
        }

        constexpr CellXZ unpk(uint64_t k)
        {
            return CellXZ{
                static_cast<int32_t>(static_cast<uint32_t>(k >> 32)),
                static_cast<int32_t>(static_cast<uint32_t>(k & 0xFFFFFFFFull))
            };
        }

        /** The grid, merge table and group root memo of one dimension. */
        struct DimGrid
        {
            int cellSize{0};
            int gapWidth{0};
            /** Only cells carrying a merge mark. key = pk(x,z), value = the bitwise or
             *  of MergeBit. */
            std::unordered_map<uint64_t, uint32_t> merges;
            /** The group root memo, cleared whole whenever the merge table changes. */
            std::unordered_map<uint64_t, uint64_t> roots;
            /** Cells whose group walk hit the bound, remembered so the refusal stays
             *  conservative instead of rewalking the graph every time. */
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
         * The number of dimensions with a registered grid. The outermost lock-free fast
         * path.
         *
         * With no dimension registering a grid, the `Actor::move` hook reads one relaxed atomic and
         * returns, which is what a server that installed the loader without using the
         * confinement system should pay: nothing.
         */
        std::atomic<int> gGridCount{0};

        /** One bit per dimension 0..63 that has a grid with cellSize > 0, the second
         *  lock-free gate. With one grid registered, every non-player actor move in
         *  every other dimension still reaches hasCellGrid, on the hottest function of the
         *  engine, and answers from this word without a mutex or a map find. Ids of 64 or
         *  above set the overflow flag and keep the locked path. */
        std::atomic<uint64_t> gGridDimBits{0};
        std::atomic<bool> gGridAbove64{false};

        void mirrorGridLocked(int dimension, bool present)
        {
            if (dimension < 0) return;
            if (dimension >= 64)
            {
                if (present) gGridAbove64.store(true, std::memory_order_relaxed);
                return;
            }
            uint64_t const bit = uint64_t{1} << dimension;
            if (present) gGridDimBits.fetch_or(bit, std::memory_order_relaxed);
            else gGridDimBits.fetch_and(~bit, std::memory_order_relaxed);
        }

        /** Whether the `Actor::move` detour is installed. It is never removed once
         *  installed, as explained below. */
        std::atomic<bool> gMoveHookInstalled{false};

        void installMoveHookOnce();

        //  Grid geometry

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

        constexpr CellXZ neighbourOf(CellXZ id, int dir)
        {
            switch (dir)
            {
            case 0: return CellXZ{id.x, id.z - 1};
            case 1: return CellXZ{id.x + 1, id.z};
            case 2: return CellXZ{id.x, id.z + 1};
            default: return CellXZ{id.x - 1, id.z};
            }
        }

        /** This cell itself declares a merge toward `dir`. */
        bool claims(DimGrid const& g, CellXZ id, int dir)
        {
            auto it = g.merges.find(pk(id.x, id.z));
            return it != g.merges.end() && (it->second & bitOf(dir)) != 0;
        }

        /**
         * Whether two adjacent cells are connected. The test is either side declaring
         * it, not both.
         *
         * Word for word the same as `connected` on the mod side, for the same reason:
         * unlink clears the neighbor before storing itself and a failed write in between
         * leaves a one-sided mark. Requiring both would make a walk from either side
         * produce a different set, so the group root would not be unique, and without a
         * unique root the question of one area has no stable answer and the same piston
         * push is blocked sometimes and allowed at others.
         */
        bool connected(DimGrid const& g, CellXZ a, int dir)
        {
            return claims(g, a, dir) || claims(g, neighbourOf(a, dir), (dir + 2) % 4);
        }

        /** `a` is smaller under the Ord of the cell id, comparing x first and then z.
         *  Matches the mod side. */
        constexpr bool lessThan(CellXZ a, CellXZ b)
        {
            return a.x != b.x ? a.x < b.x : a.z < b.z;
        }

        /**
         * The representative number of a merge group. Returns false when the walk did
         * not finish because it hit the bound, and the caller must refuse on that.
         *
         * The vast majority of cells were never merged, so a cell with no connected
         * neighbor returns itself and saves a container allocation. Carrying no merge
         * mark of its own is not enough, since a neighbor may still hold a one-sided
         * mark, so all four directions are asked through `connected`.
         */
        bool groupRootLocked(DimGrid& g, CellXZ id, CellXZ* out)
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
            std::vector<CellXZ> stack{id};
            std::vector<uint64_t> members{key};
            CellXZ best = id;

            while (!stack.empty())
            {
                CellXZ cur = stack.back();
                stack.pop_back();
                for (int d = 0; d < 4; ++d)
                {
                    if (!connected(g, cur, d)) continue;
                    CellXZ nb = neighbourOf(cur, d);
                    uint64_t nk = pk(nb.x, nb.z);
                    if (!seen.insert(nk).second) continue;
                    if (members.size() >= static_cast<size_t>(kGroupScanLimit))
                    {
                        // An unfinished walk means unknown. The entire visited set is
                        // marked oversized, otherwise entering from another cell of the
                        // group rewalks the same unfinishable graph.
                        for (uint64_t m : seen) g.oversized.insert(m);
                        hostLogger().warn(
                            "[cell] the merge group exceeds {} cells, walked from {};{}, so "
                            "the crossing decision refuses everything in this group; this is "
                            "not a configuration problem, since at that size the question of "
                            "one area has no affordable answer and a protection decision "
                            "must refuse when it does not know",
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
            // The whole group is written at once, so any cell of it hits the memo
            // afterwards.
            for (uint64_t m : members) g.roots[m] = rootKey;
            *out = best;
            return true;
        }

        /**
         * Which cell a coordinate belongs to. Must match the mod side
         * exactly.
         *
         * Returns false when it belongs to no cell, meaning it is in a gap.
         */
        bool owningCellLocked(DimGrid const& g, int x, int z, CellXZ* out)
        {
            int const cell = g.cellSize + g.gapWidth;
            if (cell <= 0) return false;
            int const px = floorDivLocal(x, cell);
            int const pz = floorDivLocal(z, cell);
            bool const onGapX = positiveModLocal(x, cell) >= g.cellSize;
            bool const onGapZ = positiveModLocal(z, cell) >= g.cellSize;

            CellXZ const base{px, pz};
            if (!onGapX && !onGapZ)
            {
                *out = base;
                return true;
            }
            if (onGapX && !onGapZ)
            {
                // A north-south seam, separating base from its eastern neighbor.
                if (!connected(g, base, 1)) return false;
                *out = base;
                return true;
            }
            if (!onGapX && onGapZ)
            {
                // An east-west seam, separating base from its southern neighbor.
                if (!connected(g, base, 2)) return false;
                *out = base;
                return true;
            }
            // A junction counts as cell interior only when all four edges of the
            // surrounding 2x2 are merged.
            CellXZ const ne = neighbourOf(base, 1);
            CellXZ const sw = neighbourOf(base, 2);
            if (!(connected(g, base, 1) && connected(g, base, 2) && connected(g, ne, 2)
                  && connected(g, sw, 1)))
            {
                return false;
            }
            *out = base;
            return true;
        }
    } // namespace

    //  Public interface

    void setCellGrid(int dimension, int cellSize, int gapWidth)
    {
        if (cellSize <= 0)
        {
            clearCellGrid(dimension);
            return;
        }
        // A value from a caller is never trusted: a negative gapWidth makes cell zero
        // or negative, and cell is the divisor of the modulus. The same reason
        // TerrainSpec::clamp gives.
        if (gapWidth < 0) gapWidth = 0;
        if (cellSize > 512) cellSize = 512;
        if (gapWidth > 64) gapWidth = 64;

        std::lock_guard lock{gridMutex()};
        auto [it, inserted] = grids().try_emplace(dimension);
        if (inserted) gGridCount.fetch_add(1, std::memory_order_relaxed);
        auto& g = it->second;
        if (g.cellSize != cellSize || g.gapWidth != gapWidth)
        {
            g.cellSize = cellSize;
            g.gapWidth = gapWidth;
            // Changed geometry changes every ownership answer. The merge table itself is
            // unchanged, but the memo must be cleared.
            g.roots.clear();
            g.oversized.clear();
        }
        mirrorGridLocked(dimension, g.cellSize > 0);
        installMoveHookOnce();
    }

    void clearCellGrid(int dimension)
    {
        std::lock_guard lock{gridMutex()};
        if (grids().erase(dimension) > 0)
        {
            gGridCount.fetch_sub(1, std::memory_order_relaxed);
        }
        mirrorGridLocked(dimension, false);
    }

    void setCellMerges(int dimension, int32_t const* entries, int32_t count)
    {
        if (count < 0) count = 0;
        if (entries == nullptr) count = 0;
        std::lock_guard lock{gridMutex()};
        auto it = grids().find(dimension);
        if (it == grids().end())
        {
            // A merge table pushed before the grid is registered is dropped, and said
            // so. Accepting it silently is worse: the table is stored while the geometry
            // is empty, ownership always answers not in a cell, and the symptom is that
            // cells are merged and a piston still cannot push through.
            hostLogger().warn(
                "[cell] dimension {} received a merge table of {} entries before a cell "
                "grid was registered and it was ignored; the order is the grid first, "
                "then the merge table",
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
            if (mask == 0) continue; // An entry without a mark takes no space
            g.merges[pk(x, z)] |= mask;
        }
    }

    bool hasCellGrid(int dimension)
    {
        if (gGridCount.load(std::memory_order_relaxed) == 0) return false;
        if (dimension >= 0 && dimension < 64)
        {
            return (gGridDimBits.load(std::memory_order_relaxed) >> dimension) & 1u;
        }
        if (dimension >= 64 && !gGridAbove64.load(std::memory_order_relaxed)) return false;
        std::lock_guard lock{gridMutex()};
        auto it = grids().find(dimension);
        return it != grids().end() && it->second.cellSize > 0;
    }

    bool owningCell(int dimension, int x, int z, CellXZ* out)
    {
        if (gGridCount.load(std::memory_order_relaxed) == 0) return false;
        std::lock_guard lock{gridMutex()};
        auto it = grids().find(dimension);
        if (it == grids().end()) return false;
        CellXZ id{};
        if (!owningCellLocked(it->second, x, z, &id)) return false;
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

        CellXZ a{}, b{};
        bool const inA = owningCellLocked(g, x1, z1, &a);
        bool const inB = owningCellLocked(g, x2, z2, &b);
        // Both on a road. A road is public ground and moving on it is not a crossing.
        if (!inA && !inB) return true;
        // One side in a cell and the other not is the boundary. The direction is
        // symmetric: pushing out and pushing in are the same operation, and blocking one
        // direction only is the same as blocking neither.
        if (inA != inB) return false;
        // The same cell, the most common case, needs no graph walk.
        if (a == b) return true;

        CellXZ ra{}, rb{};
        if (!groupRootLocked(g, a, &ra)) return false;
        if (!groupRootLocked(g, b, &rb)) return false;
        return ra == rb;
    }

    namespace
    {
        /** Actor crossing interception.
         * Hooked on Actor::move(Vec3 const& posDelta) and not on tick, because tick is
         * implemented per actor subclass and cannot be hooked completely, while move is
         * the step they all take. That is a capability statement, not a guarantee: an actor type
         * writing mPos itself without going through move is unconfined, which is a gap and not a
         * crash. Three kinds are left alone: players, since confining a player is a different
         * matter and on by default would be a disaster; ridden vehicles, since a player rowing to
         * the edge and being welded in place is worse than a crossing, at the cost of an empty
         * boat being blocked, though a vehicle moves no blocks; and actors already removed. A
         * blocked move clears only the horizontal delta and keeps the vertical one. Clearing
         * everything leaves an actor hovering at the boundary without falling, which looks like a
         * frozen server, while keeping y makes it hit an invisible wall, which a player
         * recognizes. */
        LL_TYPE_INSTANCE_HOOK(
            CellConfineActorMoveHook,
            ll::memory::HookPriority::Normal,
            Actor,
            &Actor::move,
            void,
            ::Vec3 const& posDelta
        )
        {
            // The outermost path: with no grid registered at all, one relaxed read and
            // return.
            if (gGridCount.load(std::memory_order_relaxed) == 0)
            {
                return origin(posDelta);
            }
            // Without a horizontal delta there is no crossing to make. Dropped items on
            // the ground, armor stands and item frames take this path, and they usually
            // make up most of the actor count.
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
                    ridden = this->hasPassenger() || this->mRemoved;
                    from = this->getPosition();
                }
            }
            catch (...)
            {
                // An unreadable state does not block. Blocking wrongly here costs a
                // permanently stuck actor, which is worse than one crossing getting
                // through. The initial values follow the same direction: isPlayerActor
                // and ridden both start as true, so an early throw lands on leave alone.
                return origin(posDelta);
            }
            if (isPlayerActor || ridden || dim < 0)
            {
                return origin(posDelta);
            }

            bool allowCross = true;
            bool const hasRule =
                getDimensionRule(dim, static_cast<int>(DimRule::EntityCrossCell), &allowCross);
            bool const hasGrid = hasCellGrid(dim);

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
            // The velocity is cleared too, otherwise the next tick drives the same
            // horizontal velocity into the boundary and the actor jitters instead of
            // stopping. The vertical velocity is kept, since it still has to fall.
            // A const_cast on `getPosDelta()` is used rather than `setPosDelta` or
            // `getPosDeltaNonConst`: the former is an inline accessor defined in Actor.h,
            // reading mBuiltInComponents->mStateVectorComponent->mPosDelta, and expands at
            // compile time, while the other two are MCFOLD and need runtime symbol
            // resolution, a step that can fail on version drift for the same effect.
            try
            {
                auto& delta = const_cast<::Vec3&>(this->getPosDelta());
                delta.x = 0.0f;
                delta.z = 0.0f;
            }
            catch (...)
            {
                // A failure to clear the velocity only affects feel, since the actor
                // jitters at the boundary instead of stopping, while the move itself was
                // already blocked by the origin(clamped) above, so the protection does
                // not depend on this step. It is deliberately not raised to error: one
                // line per tick would bury the real problem, and this path runs every
                // tick.
            }
        }

        /**
         * Installed when the first cell grid is registered and never removed.
         *
         * The same discipline every file under hooks/ follows: an unsubscribe can arrive
         * from inside the hooked function and removing the patch there is unsafe. An idle
         * hook costs one atomic read.
         *
         * The call site already holds gridMutex, so no lock is taken here.
         */
        void installMoveHookOnce()
        {
            if (gMoveHookInstalled.load(std::memory_order_relaxed)) return;
            int const r = CellConfineActorMoveHook::hook();
            gMoveHookInstalled.store(true, std::memory_order_relaxed);
            if (r == 0)
            {
                hostLogger().debug("[cell] cell boundary confinement enabled");
            }
            else
            {
                hostLogger().error(
                    "[cell] installing the Actor::move detour failed with status {}; the "
                    "usual cause is a mismatch between the BDS or LeviLamina version this "
                    "loader was linked against and the one the server runs, so a symbol "
                    "address resolved wrongly. Actor crossing is now entirely unconfined; "
                    "the piston path is unaffected, since it hooks a different symbol",
                    r
                );
            }
        }
    } // namespace
} // namespace pier::dimensions
