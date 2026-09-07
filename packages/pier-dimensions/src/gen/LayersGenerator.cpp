/**
 * LayersGenerator.cpp: fills a chunk from a layer stack and a grid.
 *
 * The buffer is one column-major array per thread, refilled whole when the generator
 * changes: the upstream generator keeps one static thread_local buffer initialized by
 * whichever generator called first, which is right with one flat dimension and wrong with
 * several, where the second would get the blocks of the first. The `owner` check is that fix.
 * The buffer height is the dimension's own range, not a constant: two dimensions of
 * different heights on the same thread refill anyway.
 */
#include "pier/dimensions/dim/complete_base_types.h"

#include "pier/dimensions/gen/layers_generator.h"

#include "mc/deps/core/string/HashedString.h"
#include "mc/world/level/ChunkPos.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/biome/registry/BiomeRegistry.h"
#include "mc/world/level/biome/source/FixedBiomeSource.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/BlockVolume.h"
#include "mc/world/level/block/VanillaBlockTypeIds.h"
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
            // BlockTypeRegistry::get() is inlined away in 26.32. The static that held
            // the registry is still exported, and Bedrock::Owner keeps the object in a
            // public mValue, which is the reference the accessor handed back.
            return &BlockTypeRegistry::mBlockTypeRegistry().mValue.getDefaultBlockState(
                HashedString{id}, true);
        }

        void fillLayer(std::vector<Block const*>& buf, int height, Block const* block, int yIdx)
        {
            for (int i = 0; i < kColumns; ++i) buf[static_cast<size_t>(yIdx + i * height)] = block;
        }
        void fillRange(std::vector<Block const*>& buf, int height, Block const* block, int from, int toExclusive)
        {
            for (int y = from; y < toExclusive; ++y) fillLayer(buf, height, block, y);
        }
    } // namespace

    LayersGenerator::LayersGenerator(Dimension& dimension, uint seed, Json::Value const& options, spec::Layers const& layers, int minY, int maxY)
        : FlatWorldGenerator(dimension, seed, options), mLayers(layers), mMinY(minY), mMaxY(maxY)
    {
        auto& level = dimension.mLevel;
        mBiome = level.getBiomeRegistry().lookupByName(mLayers.biome);
        if (!mBiome)
        {
            // The spec was validated at registration; reaching here means the pack changed
            // under a saved dimension. Say so, do not pretend the biome was plains.
            hostLogger().error("[layers] biome '{}' is not in the registry any more; using minecraft:plains for this session", mLayers.biome);
            mBiome = level.getBiomeRegistry().lookupByName("minecraft:plains");
        }
        if (mBiome) mBiomeSource = std::make_unique<FixedBiomeSource>(*mBiome);

        mAir = lookupBlock("minecraft:air");
        mBedrock = lookupBlock(std::string{VanillaBlockTypeIds::Bedrock().getString()});
        int y = mLayers.baseY;
        for (auto const& l : mLayers.layers)
        {
            mStack.push_back({lookupBlock(l.block), y - mMinY, l.thickness});
            y += l.thickness;
        }
        mSurfaceIndexY = mStack.empty() ? -1 : (y - 1) - mMinY;
        if (mLayers.grid)
        {
            mGap = lookupBlock(mLayers.grid->gapBlock);
            mEdge = lookupBlock(mLayers.grid->edgeBlock);
        }
    }

    LayersGenerator::ThreadBuffer& LayersGenerator::acquireBuffer()
    {
        static thread_local ThreadBuffer buf;
        size_t const size = static_cast<size_t>(kColumns) * static_cast<size_t>(mMaxY - mMinY);
        if (buf.blocks.size() != size)
        {
            buf.blocks.assign(size, nullptr);
            buf.owner = nullptr;
        }
        if (buf.owner != static_cast<void const*>(this))
        {
            refillStatic(buf);
            buf.owner = static_cast<void const*>(this);
        }
        return buf;
    }

    void LayersGenerator::refillStatic(ThreadBuffer& buf)
    {
        int const height = mMaxY - mMinY;
        fillRange(buf.blocks, height, mAir, 0, height);
        if (kBedrockY >= mMinY && kBedrockY < mMinY + height) fillLayer(buf.blocks, height, mBedrock, kBedrockY - mMinY);
        for (auto const& l : mStack) fillRange(buf.blocks, height, l.block, l.indexY, l.indexY + l.thickness);
        buf.volume = mPrototype;
        buf.volume->mHeight = static_cast<uint>(height);
        buf.volume->mBlocks->mBegin = buf.blocks.data();
        buf.volume->mBlocks->mEnd = buf.blocks.data() + buf.blocks.size();
    }

    void LayersGenerator::loadChunk(LevelChunk& lc, bool)
    {
        auto& buf = acquireBuffer();
        int const height = mMaxY - mMinY;
        auto const& chunkPos = lc.mPosition.get();
        if (mLayers.grid && mSurfaceIndexY >= 0)
        {
            auto const& g = *mLayers.grid;
            int const edgeIdx = mSurfaceIndexY + 1;
            // Restore the two rows the previous chunk may have painted: surface and the row above.
            fillLayer(buf.blocks, height, mStack.back().block, mSurfaceIndexY);
            if (edgeIdx < height) fillLayer(buf.blocks, height, mAir, edgeIdx);
            int const startX = chunkPos.x * kW;
            int const startZ = chunkPos.z * kW;
            for (int x = 0; x < kW; ++x)
            {
                auto const ax = spec::classify1D(spec::positiveMod(startX + x, g.period()), g);
                for (int z = 0; z < kW; ++z)
                {
                    auto const az = spec::classify1D(spec::positiveMod(startZ + z, g.period()), g);
                    size_t const column = static_cast<size_t>((x * kW + z) * height);
                    switch (spec::combine2D(ax, az))
                    {
                    case spec::CellArea::Gap: buf.blocks[column + static_cast<size_t>(mSurfaceIndexY)] = mGap; break;
                    case spec::CellArea::Edge: if (edgeIdx < height) buf.blocks[column + static_cast<size_t>(edgeIdx)] = mEdge; break;
                    case spec::CellArea::Interior: break;
                    }
                }
            }
        }
        lc.setBlockVolume(*buf.volume, 0);
        if (mBiomeSource) mBiomeSource->fillBiomes(lc, nullptr);
        lc.recomputeHeightMap(false);
        // LevelChunk::setSaved is inlined away in 26.32 and has no symbol left. It
        // stamped the chunk's dirty counters so a freshly generated chunk counted as
        // already saved, which it can be: an ungenerated chunk comes back identical from
        // the seed. Without the stamp the chunk is written on the next save pass, which
        // costs disk traffic on a large world and loses nothing. The counters are public
        // and could be stamped here, but the direction is a guess, and guessing it
        // backwards means a chunk that is never written at all.
        if (!lc.tryChangeState(ChunkState::Generating, ChunkState::Generated))
        {
            hostLogger().error("[layers] chunk ({}, {}) failed the Generating to Generated transition; it will not be sent", chunkPos.x, chunkPos.z);
        }
    }
} // namespace pier::dimensions
