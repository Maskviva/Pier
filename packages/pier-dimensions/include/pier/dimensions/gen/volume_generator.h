/** volume_generator.h: the chunk generator of a volume-pack dimension.
 * Derives from FlatWorldGenerator for the prototype BlockVolume and the structure
 * queries, like the template generator. The seeded pack decides every cell and the
 * biome of every column through SeededVolume::generateChunk; this class turns materials
 * into Block pointers and BIOM entries into Biome pointers once, and writes them. */
#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "mc/world/level/levelgen/flat/FlatWorldGenerator.h"

#include "pier/dimensions/pack/volume_pack.h"

class Biome;
class Block;
class Dimension;
class LevelChunk;
namespace Json { class Value; }

namespace pier::dimensions
{
    class VolumeGenerator final : public FlatWorldGenerator
    {
        std::shared_ptr<pack::SeededVolume const> mSeeded;
        std::vector<Block const*> mBlocks;
        std::vector<Biome const*> mBiomes;

        struct ThreadBuffer
        {
            std::vector<Block const*> blocks;
            std::vector<std::uint16_t> materials;
            std::vector<std::uint32_t> biomes;
            std::optional<BlockVolume> volume;
        };
        ThreadBuffer& acquireBuffer();

    public:
        VolumeGenerator(Dimension& dimension, uint seed, Json::Value const& options,
                        std::shared_ptr<pack::SeededVolume const> seeded);
        void loadChunk(LevelChunk& lc, bool forceImmediateReplacementDataLoad) override;
    };
} // namespace pier::dimensions
