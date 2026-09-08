/**
 * TemplateGenerator.cpp: fills a chunk from a mounted template pack.
 *
 * The buffer is one column-major array per thread, refilled whole when the generator
 * changes: the upstream generator keeps one static thread_local buffer initialized by
 * whichever generator called first, which is right with one flat dimension and wrong
 * with several. The `owner` check is that fix. Rows that are the same in every zone
 * and out of reach of every pick are written once in refillStatic; every other row is
 * rewritten per chunk from the materials generateTemplateChunk produced, so a
 * structure painted by the previous chunk never leaks into the next.
 */
#include "pier/dimensions/dim/complete_base_types.h"

#include "pier/dimensions/gen/template_generator.h"

#include <algorithm>

#include "mc/deps/core/string/HashedString.h"
#include "mc/world/level/ChunkPos.h"
#include "mc/world/level/Level.h"
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
            // BlockTypeRegistry::get() is inlined away in 26.32. The static that held
            // the registry is still exported, and Bedrock::Owner keeps the object in a
            // public mValue, which is the reference the accessor handed back.
            return &BlockTypeRegistry::mBlockTypeRegistry().mValue.getDefaultBlockState(HashedString{id}, true);
        }
    } // namespace

    TemplateGenerator::TemplateGenerator(Dimension& dimension, uint seed, Json::Value const& options,
                                         std::shared_ptr<pack::TemplatePack const> pack, pack::MountedTemplate mounted)
        : FlatWorldGenerator(dimension, seed, options), mPack(std::move(pack)), mMounted(std::move(mounted))
    {
        // The mount refers to the pack by pointer; the pointer must be to the object this
        // generator keeps alive, not to whatever the caller mounted from.
        mMounted.pack = mPack.get();
        auto& level = dimension.mLevel;
        mBiome = level.getBiomeRegistry().lookupByName(mPack->biome);
        if (!mBiome)
        {
            // The biome was checked at registration; reaching here means the registry
            // changed under a saved dimension. Say so, do not pretend it was plains.
            hostLogger().error("[template] biome '{}' is not in the registry any more; using minecraft:plains for this session", mPack->biome);
            mBiome = level.getBiomeRegistry().lookupByName("minecraft:plains");
        }
        if (mBiome) mBiomeSource = std::make_unique<FixedBiomeSource>(*mBiome);

        mBlocks.reserve(mMounted.materials.size());
        for (auto const& name : mMounted.materials) mBlocks.push_back(lookupBlock(name));

        int const height = mMounted.height();
        mDirtyFrom = height;
        mDirtyTo = 0;
        for (int y = 0; y < height; ++y)
        {
            if (mMounted.staticLayer[static_cast<std::size_t>(y)]) continue;
            mDirtyFrom = std::min(mDirtyFrom, y);
            mDirtyTo = std::max(mDirtyTo, y + 1);
        }
        for (auto const& p : mMounted.picks)
        {
            for (auto const& r : p.roots)
            {
                auto const& bb = mMounted.shapeBox[r.shapeNode];
                if (!bb) continue;
                int lo = std::clamp(bb->y0 + p.anchorY - mMounted.minY, 0, height);
                int hi = std::clamp(bb->y1 + p.anchorY - mMounted.minY, 0, height);
                if (lo >= hi) continue;
                mDirtyFrom = std::min(mDirtyFrom, lo);
                mDirtyTo = std::max(mDirtyTo, hi);
            }
        }
        if (mDirtyFrom > mDirtyTo) mDirtyFrom = mDirtyTo = 0;
    }

    TemplateGenerator::ThreadBuffer& TemplateGenerator::acquireBuffer()
    {
        static thread_local ThreadBuffer buf;
        size_t const size = static_cast<size_t>(kColumns) * static_cast<size_t>(mMounted.height());
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

    void TemplateGenerator::refillStatic(ThreadBuffer& buf)
    {
        auto const height = static_cast<size_t>(mMounted.height());
        auto const& column = mMounted.zoneColumns[0];
        for (size_t c = 0; c < static_cast<size_t>(kColumns); ++c)
        {
            for (size_t y = 0; y < height; ++y) buf.blocks[c * height + y] = mBlocks[column[y]];
        }
        buf.volume = mPrototype;
        buf.volume->mHeight = static_cast<uint>(height);
        buf.volume->mBlocks->mBegin = buf.blocks.data();
        buf.volume->mBlocks->mEnd = buf.blocks.data() + buf.blocks.size();
    }

    void TemplateGenerator::loadChunk(LevelChunk& lc, bool)
    {
        auto& buf = acquireBuffer();
        auto const height = static_cast<size_t>(mMounted.height());
        auto const& chunkPos = lc.mPosition.get();
        if (mDirtyTo > mDirtyFrom)
        {
            pack::generateTemplateChunk(mMounted, chunkPos.x, chunkPos.z, buf.materials);
            auto const from = static_cast<size_t>(mDirtyFrom);
            auto const to = static_cast<size_t>(mDirtyTo);
            for (size_t c = 0; c < static_cast<size_t>(kColumns); ++c)
            {
                auto const* src = buf.materials.data() + c * height;
                auto* dst = buf.blocks.data() + c * height;
                for (size_t y = from; y < to; ++y) dst[y] = mBlocks[src[y]];
            }
        }
        lc.setBlockVolume(*buf.volume, 0);
        if (mBiomeSource) mBiomeSource->fillBiomes(lc, nullptr);
        lc.recomputeHeightMap(false);
        // LevelChunk::setSaved is inlined away in 26.32 and has no symbol left; without
        // the stamp a fresh chunk is written on the next save pass, which loses nothing.
        if (!lc.tryChangeState(ChunkState::Generating, ChunkState::Generated))
        {
            hostLogger().error("[template] chunk ({}, {}) failed the Generating to Generated transition; it will not be sent", chunkPos.x, chunkPos.z);
        }
    }
} // namespace pier::dimensions
