/** pier/dimensions/gen/noise_generator.h: density terrain with multi-biome placement.
 * Density and climate are both PerlinNoise, the engine's own with an exported constructor.
 * Placement writes the chosen biome per column with LevelChunk::_setBiome after the fixed
 * source filled the chunk, rather than through BiomeSource3d: that path needs a
 * XoroshiroPositionalRandomFactory, which exports no constructor and only a $vftable, so
 * building one means writing a vptr by hand. What is lost is the engine's climate noise
 * shape, not multi-biome placement itself.
 * Whether biome decoration (trees, ores from behavior-pack feature_rules) runs on this
 * terrain depends on decorationPostProcessChunk of the base, which a header cannot answer;
 * the terrain shape does not depend on it. */
#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "mc/world/level/levelgen/flat/FlatWorldGenerator.h"
#include "mc/world/level/levelgen/synth/PerlinNoise.h"

#include "pier/dimensions/spec/dimension_spec.h"

class Block;
class Dimension;
class LevelChunk;
namespace Json { class Value; }

namespace pier::dimensions
{
    class NoiseGenerator final : public FlatWorldGenerator
    {
        struct Palette
        {
            Block const* block{nullptr};
            int depthLo{0};
            int depthHi{0};
        };

        spec::Noise mNoise;
        int mMinY{0};
        int mMaxY{0};
        Block const* mAir{nullptr};
        Block const* mFluid{nullptr};
        Block const* mBedrock{nullptr};
        std::vector<Palette> mPalette;
        std::vector<std::unique_ptr<PerlinNoise>> mOctaves;
        std::unique_ptr<PerlinNoise> mIslandNoise;

        /** One low-frequency field per climate axis that any target actually constrains.
         *  Unconstrained axes are not sampled at all: a full-range box matches every value,
         *  so the noise for it would cost a lookup per column and decide nothing. */
        std::unique_ptr<PerlinNoise> mTemperature;
        std::unique_ptr<PerlinNoise> mHumidity;
        std::vector<std::pair<spec::BiomeTarget, Biome const*>> mTargets;

        struct ThreadBuffer
        {
            std::vector<Block const*> blocks;
            std::optional<BlockVolume> volume;
            void const* owner{nullptr};
        };
        ThreadBuffer& acquireBuffer();

        [[nodiscard]] float density(int worldX, int worldY, int worldZ) const;
        [[nodiscard]] Block const* blockAtDepth(int depth) const;
        /** The first target whose climate box contains this column, or nullptr when the
         *  table leaves a gap; the caller then keeps the chunk's fixed biome. */
        [[nodiscard]] Biome const* biomeAt(int worldX, int worldZ) const;

    public:
        NoiseGenerator(Dimension& dimension, uint seed, Json::Value const& options, spec::Noise const& noise, int minY, int maxY);
        void loadChunk(LevelChunk& lc, bool forceImmediateReplacementDataLoad) override;
    };
} // namespace pier::dimensions
