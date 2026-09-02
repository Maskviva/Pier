#pragma once

#include <cstdint>

/** plot_confine.h: whether two coordinates count as the same plot. DimRule::PistonPush applies to
 * a whole dimension, while keeping flying machines, pistons and actors inside a plot needs a
 * decision by boundary: pushing inside a plot is fine and only crossing is blocked. The decision
 * happens inside PistonBlockActor::_checkAttachedBlocks and Actor::move, on the engine tick path,
 * hundreds to thousands of times per second, so asking the mod side across the ABI each time is
 * out of the question. The grid geometry and the merge relations are therefore pushed over from
 * the mod side through md_set_plot_grid and md_set_plot_merges, and this only stores and queries
 * them. The ownership decision must match owning_plot on the mod side exactly: the interior of a
 * plot belongs to that plot; a north-south seam belongs to the western plot when that plot
 * declares a merge to the east; an east-west seam belongs to the northern plot when that plot
 * declares a merge to the south; a junction belongs to the north-west plot when all four edges of
 * the surrounding 2x2 are merged; everything else belongs to no plot. A disagreement between the
 * two sides blocks an owner on a plot they merged themselves, with the symptom that placing by
 * hand works while a piston cannot push through. The same hard constraint dimension_rules.h
 * states: a dimension with no registered grid must not notice these hooks at all, and without a
 * grid sameArea always returns true. / */
namespace pier::dimensions
{
    /** The grid number of one plot. Isomorphic to `PlotId` on the mod side. */
    struct PlotXZ
    {
        int32_t x{0};
        int32_t z{0};

        friend bool operator==(PlotXZ const& a, PlotXZ const& b) { return a.x == b.x && a.z == b.z; }
        friend bool operator!=(PlotXZ const& a, PlotXZ const& b) { return !(a == b); }
    };

    /** Direction bits of a merge mark, matching the indices of the `merged` array on
     *  the mod side: 0 north, 1 east, 2 south, 3 west. */
    enum MergeBit : uint32_t
    {
        kMergeNorth = 1u << 0,
        kMergeEast = 1u << 1,
        kMergeSouth = 1u << 2,
        kMergeWest = 1u << 3,
    };

    /**
     * Registers or updates the plot grid of a dimension.
     *
     * `plotSize <= 0` means the dimension has no plot grid and is equivalent to
     * `clearPlotGrid`. The incoming values are not trusted: a negative roadWidth would
     * turn the modulus into a division by zero, so they are clamped here.
     */
    void setPlotGrid(int dimension, int plotSize, int roadWidth);

    /** Withdraws the grid of a dimension, called when a world is deleted or switched to
     *  a non-plot model. The merge table is cleared with it. */
    void clearPlotGrid(int dimension);

    /**
     * Replaces the whole merge mark table of a dimension.
     *
     * `entries` is `count` groups of `(x, z, mask)`, so `count * 3` int32 values. Only
     * plots carrying a merge mark need to be passed, since a plot with no entry counts
     * as unmerged on all four sides, so a server with a few thousand plots and a dozen
     * merges sends a few dozen integers.
     *
     * A whole-table replacement and not an increment. An increment requires both sides
     * to agree at all times on which entries exist, while unlink clears the neighbor
     * before storing itself and can fail in between, and once they disagree there is no
     * way back. A whole-table replacement pulls the state back into agreement every
     * time.
     */
    void setPlotMerges(int dimension, int32_t const* entries, int32_t count);

    /**
     * Whether the dimension currently has a plot grid. The first fast path of a hook.
     *
     * Exposed on its own because it is far cheaper than `sameArea`: with no grid, no
     * coordinate has to be computed.
     */
    bool hasPlotGrid(int dimension);

    /**
     * The only question: whether `(x1,z1)` and `(x2,z2)` belong to the same area within
     * which movement is free.
     *
     * * A dimension with no grid always returns `true`, meaning no interference.
     * * Neither point belonging to any plot, both on a road, returns `true`. A road is
     *   public ground and moving on it is not a crossing.
     * * One point on a plot and the other not returns `false`. That covers pushing out
     *   and pushing in symmetrically: shoving a creeper into someone else's plot and
     *   pulling their chest out are the same operation.
     * * Both points on a plot returns true only when their merge group roots match.
     */
    bool sameArea(int dimension, int x1, int z1, int x2, int z2);

    /**
     * Diagnostic: which plot a cell belongs to. `out` is written only when this returns
     * true.
     *
     * The interception path does not use it, since `sameArea` computes both sides at
     * once and shares the memo. It is exposed so that why a move was blocked can be
     * reproduced by hand.
     */
    bool owningPlot(int dimension, int x, int z, PlotXZ* out);
} // namespace pier::dimensions
