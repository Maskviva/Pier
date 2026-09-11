/** supplied_generator.h: terrain a mod fills, one chunk at a time.
 *
 * Derives from FlatWorldGenerator for the prototype BlockVolume and the structure
 * queries, like the pack generators. What differs is where the materials come from: not
 * from anything this host can read, but from a callback the mod registered.
 *
 * The two palettes are resolved to Block and Biome pointers once, at registration, and
 * the boundary carries nothing but indices after that. A chunk is 98304 entries, and
 * looking names up per chunk is the difference between a generator and a stall.
 *
 * What this host does not know is the shape of the terrain, and that is the point. A
 * layer stack, a grid of plots, a noise field and a binary format read off disk are one
 * thing from here: something that fills a buffer when asked.
 */
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "mc/world/level/levelgen/flat/FlatWorldGenerator.h"

#include "sdk/abi.h"

class Biome;
class Block;
class Dimension;
class LevelChunk;
namespace Json { class Value; }

namespace pier::dimensions
{
    /** The resolved palettes and the callback of one generated dimension. Built once,
     *  read from every chunk worker, never written again. */
    struct SuppliedTerrain
    {
        std::vector<Block const*> materials;
        std::vector<Biome const*> biomes;
        PierGenerateChunkFn fn = nullptr;
        void* user = nullptr;
        /** The mod that registered it, for the line said when it is gone. */
        std::string owner;
    };

    /** Resolves both palettes against the live registries, or returns null and says
     *  which name failed. A name that is not there would otherwise become a hole in the
     *  terrain that shows up only once someone walks into it. */
    std::shared_ptr<SuppliedTerrain const>
    resolveSuppliedTerrain(std::string const& dimName, std::string const& materialPalette,
                           std::string const& biomePalette, PierGenerateChunkFn fn, void* user,
                           std::string const& owner, std::vector<std::string>& problems);

    /** Keeps the terrain of one dimension until the level closes. Server thread only;
     *  a generator copies the pointer once and the chunk threads work from that. */
    void rememberSuppliedTerrain(std::string const& dimName, std::shared_ptr<SuppliedTerrain const> terrain);
    std::shared_ptr<SuppliedTerrain const> suppliedTerrainOf(std::string const& dimName);
    void forgetSuppliedTerrain(std::string const& dimName);

    class SuppliedGenerator final : public FlatWorldGenerator
    {
        std::shared_ptr<SuppliedTerrain const> mTerrain;
        std::int32_t mMinY = 0;
        std::int32_t mHeight = 0;
        /** Taken at construction: WorldGenerator does not keep the Dimension, and the
         *  chunk path must not reach back into the level from a worker thread anyway. */
        std::int32_t mDimId = 0;
        std::string mDimName;

        struct ThreadBuffer
        {
            std::vector<Block const*> blocks;
            std::vector<std::uint16_t> materials;
            std::vector<std::uint16_t> biomes;
            std::optional<BlockVolume> volume;
        };
        ThreadBuffer& acquireBuffer();

    public:
        SuppliedGenerator(Dimension& dimension, uint seed, Json::Value const& options,
                          std::shared_ptr<SuppliedTerrain const> terrain, std::int32_t minY,
                          std::int32_t height);
        void loadChunk(LevelChunk& lc, bool forceImmediateReplacementDataLoad) override;
    };
} // namespace pier::dimensions
