/** pier/dimensions/gen/layers_generator.h: the chunk generator of a layered dimension.
 * Derives from FlatWorldGenerator for the same reason the plot generator did: the flat
 * generator already wires the BlockVolume prototype, the biome source and structure queries,
 * leaving loadChunk to fill the buffer. Cost per chunk is one fill per layer plus 256 grid
 * classifications and one setBlockVolume, the same order as a vanilla flat world.
 * The stack and the grid come from spec::Layers; the layer blocks resolve once in the
 * constructor, since a lookup per block per chunk would sit on the hottest path there is. */
#pragma once

#include <optional>
#include <vector>

#include "mc/world/level/levelgen/flat/FlatWorldGenerator.h"

#include "pier/dimensions/spec/dimension_spec.h"

class Block;
class Dimension;
class LevelChunk;
namespace Json { class Value; }

namespace pier::dimensions
{
    class LayersGenerator final : public FlatWorldGenerator
    {
        struct ResolvedLayer
        {
            Block const* block{nullptr};
            int indexY{0};
            int thickness{0};
        };

        spec::Layers mLayers;
        int mMinY{0};
        int mMaxY{0};
        Block const* mAir{nullptr};
        Block const* mBedrock{nullptr};
        Block const* mGap{nullptr};
        Block const* mEdge{nullptr};
        std::vector<ResolvedLayer> mStack;
        int mSurfaceIndexY{-1};

        struct ThreadBuffer
        {
            std::vector<Block const*> blocks;
            std::optional<BlockVolume> volume;
            void const* owner{nullptr};
        };
        ThreadBuffer& acquireBuffer();
        void refillStatic(ThreadBuffer& buf);

    public:
        LayersGenerator(Dimension& dimension, uint seed, Json::Value const& options, spec::Layers const& layers, int minY, int maxY);
        void loadChunk(LevelChunk& lc, bool forceImmediateReplacementDataLoad) override;
    };
} // namespace pier::dimensions
