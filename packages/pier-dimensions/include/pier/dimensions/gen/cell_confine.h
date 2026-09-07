#pragma once

#include <cstdint>

/** cell_confine.h: whether two coordinates count as the same grid cell. DimRule::PistonPush
 * applies to a whole dimension, while keeping flying machines, pistons and actors inside one cell
 * needs a decision by boundary: pushing inside a cell is fine and only crossing is blocked. A plot
 * world is one use of this and not its definition, the same way GridSpec in dimension_spec.h is: a
 * showcase world confining each exhibit and a city world confining each block want the same rule.
 * The grid convention is the one dimension_spec.h states, and the two must agree for a dimension
 * that has both, since a confinement boundary that does not fall on a visible gap is invisible.
 * The decision happens inside PistonBlockActor::_checkAttachedBlocks and Actor::move on the engine
 * tick path, so asking the mod side across the ABI each time is out of the question; the geometry
 * and the merge relations are pushed over instead.
 * Ownership must match the mod side exactly: a cell interior belongs to that cell; a seam belongs
 * to the west or north cell when that cell declares the merge; a junction to the north-west cell
 * when all four edges of the surrounding 2x2 are merged; everything else to no cell. A mismatch
 * blocks an owner on ground they merged themselves: placing by hand works, a piston cannot.
 * A dimension with no registered grid must not notice these hooks at all, and without a grid
 * sameArea always returns true. */
namespace pier::dimensions
{
    /** The grid coordinates of one cell. Isomorphic to the cell id on the mod side. */
    struct CellXZ
    {
        int32_t x{0};
        int32_t z{0};

        friend bool operator==(CellXZ const& a, CellXZ const& b) { return a.x == b.x && a.z == b.z; }
        friend bool operator!=(CellXZ const& a, CellXZ const& b) { return !(a == b); }
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
     * Registers or updates the cell grid of a dimension.
     *
     * `cellSize <= 0` means the dimension has no cell grid and is equivalent to
     * `clearCellGrid`. The incoming values are not trusted: a negative gapWidth would
     * turn the modulus into a division by zero, so they are clamped here.
     */
    void setCellGrid(int dimension, int cellSize, int gapWidth);

    /** Withdraws the grid of a dimension, called when a world is deleted or switched to
     *  a model with no cells. The merge table is cleared with it. */
    void clearCellGrid(int dimension);

    /**
     * Replaces the whole merge mark table of a dimension.
     *
     * `entries` is `count` groups of `(x, z, mask)`, so `count * 3` int32 values. Only
     * cells carrying a merge mark need to be passed, since a cell with no entry counts
     * as unmerged on all four sides, so a world with a few thousand cells and a dozen
     * merges sends a few dozen integers.
     *
     * A whole-table replacement and not an increment. An increment requires both sides
     * to agree at all times on which entries exist, while unlink clears the neighbor
     * before storing itself and can fail in between, and once they disagree there is no
     * way back. A whole-table replacement pulls the state back into agreement every
     * time.
     */
    void setCellMerges(int dimension, int32_t const* entries, int32_t count);

    /**
     * Whether the dimension currently has a cell grid. The first fast path of a hook.
     *
     * Exposed on its own because it is far cheaper than `sameArea`: with no grid, no
     * coordinate has to be computed.
     */
    bool hasCellGrid(int dimension);

    /**
     * The only question: whether `(x1,z1)` and `(x2,z2)` belong to the same area within
     * which movement is free.
     *
     * * A dimension with no grid always returns `true`, meaning no interference.
     * * Neither point belonging to any cell, both in a gap, returns `true`. A gap is
     *   public ground and moving on it is not a crossing.
     * * One point in a cell and the other not returns `false`. That covers pushing out
     *   and pushing in symmetrically: shoving a creeper into someone else's cell and
     *   pulling their chest out are the same operation.
     * * Both points in a cell returns true only when their merge group roots match.
     */
    bool sameArea(int dimension, int x1, int z1, int x2, int z2);

    /**
     * Diagnostic: which cell a coordinate belongs to. `out` is written only when this returns
     * true.
     *
     * The interception path does not use it, since `sameArea` computes both sides at
     * once and shares the memo. It is exposed so that why a move was blocked can be
     * reproduced by hand.
     */
    bool owningCell(int dimension, int x, int z, CellXZ* out);
} // namespace pier::dimensions
