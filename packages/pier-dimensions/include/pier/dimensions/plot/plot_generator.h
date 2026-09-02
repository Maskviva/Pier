#pragma once

#include <optional>
#include <vector>

#include "mc/world/level/biome/source/FixedBiomeSource.h"
#include "mc/world/level/levelgen/flat/FlatWorldGenerator.h"

#include "pier/dimensions/plot/plot_layout.h"

class Block;
class Dimension;
class LevelChunk;

namespace Json
{
    class Value;
}

namespace pier::dimensions
{
    /**
     * The chunk generator of a plot world.
     *
     * It derives from `FlatWorldGenerator` rather than writing a `WorldGenerator` from
     * scratch, because the flat generator already wires up the BlockVolume prototype,
     * the biome source and structure queries, leaving only `loadChunk` to override and
     * fill the buffer with its own pattern.
     *
     * Cost per chunk: two layer fills, meaning 256 by 2 pointer writes, plus 256
     * classifications, then one `setBlockVolume`. That is the same order as a vanilla
     * flat world, which is why this belongs on the C++ side rather than having a mod lay
     * it out afterwards one `set_block` at a time.
     */
    class PlotGenerator final : public FlatWorldGenerator
    {
        PlotLayout mLayout;
        Block const* mAirBlock{nullptr};
        Block const* mBedrockBlock{nullptr};
        Block const* mFloorBlock{nullptr};
        Block const* mFillBlock{nullptr};
        Block const* mRoadBlock{nullptr};
        Block const* mBorderBlock{nullptr};

        /** The y index inside the buffer, normalized to 0..kTotalHeight. */
        int mFloorIndexY{0};
        int mBorderIndexY{0};

    public:
        PlotGenerator(Dimension& dimension, uint seed, Json::Value const& options, PlotLayout const& layout);

        void loadChunk(LevelChunk& lc, bool forceImmediateReplacementDataLoad) override;

        [[nodiscard]] PlotLayout const& layout() const { return mLayout; }

    private:
        /**
         * One block buffer and BlockVolume view per thread.
         *
         * The upstream implementation uses a `static thread_local ThreadData`
         * initialized on the first call from whichever generator called it. That is
         * safe with a single plot dimension, and this host allows several, where the
         * same code would give the second dimension the blocks of the first. The
         * `owner` pointer is checked here and the whole buffer is refilled when the
         * generator changes.
         */
        struct ThreadBuffer
        {
            std::vector<Block const*> blocks;
            std::optional<BlockVolume> volume;
            void const* owner{nullptr};
        };

        ThreadBuffer& acquireBuffer();
        void refillStatic(ThreadBuffer& buf);
    };
} // namespace pier::dimensions
