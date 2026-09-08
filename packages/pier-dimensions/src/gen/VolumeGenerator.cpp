/**
 * VolumeGenerator.cpp: fills a chunk from a seeded volume pack.
 *
 * Every cell changes from chunk to chunk, so the per-thread buffer is rewritten whole
 * each time from the materials the pack layer produced, with no static rows; the
 * prototype BlockVolume is re-pointed at the buffer per call, which is what the
 * upstream flat generator does with its own static buffer. Biomes are written per
 * column through LevelChunk::_setBiome with the whole column filled, which is the
 * shape Bedrock stores for a dimension without a 3D biome source.
 */
#include "pier/dimensions/dim/complete_base_types.h"

#include "pier/dimensions/gen/volume_generator.h"

#include "mc/deps/core/string/HashedString.h"
#include "mc/world/level/ChunkBlockPos.h"
#include "mc/world/level/ChunkLocalHeight.h"
#include "mc/world/level/ChunkPos.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/biome/Biome.h"
#include "mc/world/level/biome/registry/BiomeRegistry.h"
#include "mc/world/level/biome/source/FixedBiomeSource.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/BlockVolume.h"
#include "mc/world/level/block/registry/BlockTypeRegistry.h"
#include "mc/world/level/chunk/ChunkState.h"
#include "mc/world/level/chunk/LevelChunk.h"
#include "mc/world/level/dimension/Dimension.h"

#include "pier/support/log.h"

namespace pier::dimensions
{
    namespace
    {
        using ::pier::hostLogger;
        constexpr int kW = 16;
        constexpr int kColumns = kW * kW;

        Block const* lookupBlock(std::string const& id)
        {
            // BlockTypeRegistry::get() is inlined away in 26.32; the static behind it is
            // still exported and its public mValue is the registry the accessor returned.
            return &BlockTypeRegistry::mBlockTypeRegistry().mValue.getDefaultBlockState(HashedString{id}, true);
        }
    } // namespace

    VolumeGenerator::VolumeGenerator(Dimension& dimension, uint seed, Json::Value const& options,
                                     std::shared_ptr<pack::SeededVolume const> seeded)
        : FlatWorldGenerator(dimension, seed, options), mSeeded(std::move(seeded))
    {
        auto& registry = dimension.mLevel.getBiomeRegistry();
        auto const& p = mSeeded->pack();
        Biome const* fallback = registry.lookupByName("minecraft:plains");
        for (auto const& name : p.biomeNames)
        {
            // The non-const overload of lookupByName returns a mutable pointer; the
            // member and the fallback are both const, so the type is written out.
            Biome const* b = registry.lookupByName(name);
            if (!b)
            {
                // The names were checked at registration; the registry changed under a
                // saved dimension. Said once per biome, then plains stands in.
                hostLogger().error("[volume] biome '{}' is not in the registry any more; using minecraft:plains for this session", name);
                b = fallback;
            }
            mBiomes.push_back(b);
        }
        mBiome = fallback;
        if (mBiome) mBiomeSource = std::make_unique<FixedBiomeSource>(*mBiome);
        for (auto const& name : mSeeded->materials()) mBlocks.push_back(lookupBlock(name));
    }

    VolumeGenerator::ThreadBuffer& VolumeGenerator::acquireBuffer()
    {
        static thread_local ThreadBuffer buf;
        size_t const size = static_cast<size_t>(kColumns) * static_cast<size_t>(mSeeded->height());
        if (buf.blocks.size() != size) buf.blocks.assign(size, nullptr);
        buf.volume = mPrototype;
        buf.volume->mHeight = static_cast<uint>(mSeeded->height());
        buf.volume->mBlocks->mBegin = buf.blocks.data();
        buf.volume->mBlocks->mEnd = buf.blocks.data() + buf.blocks.size();
        return buf;
    }

    void VolumeGenerator::loadChunk(LevelChunk& lc, bool)
    {
        auto& buf = acquireBuffer();
        auto const& chunkPos = lc.mPosition.get();
        mSeeded->generateChunk(chunkPos.x, chunkPos.z, buf.materials, buf.biomes);
        for (size_t i = 0; i < buf.blocks.size(); ++i) buf.blocks[i] = mBlocks[buf.materials[i]];
        lc.setBlockVolume(*buf.volume, 0);
        for (int x = 0; x < kW; ++x)
        {
            for (int z = 0; z < kW; ++z)
            {
                auto const* biome = mBiomes[buf.biomes[static_cast<size_t>(x * kW + z)]];
                if (biome) lc._setBiome(*biome, ChunkBlockPos{static_cast<uchar>(x), ChunkLocalHeight{static_cast<short>(0)}, static_cast<uchar>(z)}, true);
            }
        }
        lc.recomputeHeightMap(false);
        if (!lc.tryChangeState(ChunkState::Generating, ChunkState::Generated))
        {
            hostLogger().error("[volume] chunk ({}, {}) failed the Generating to Generated transition; it will not be sent", chunkPos.x, chunkPos.z);
        }
    }
} // namespace pier::dimensions
