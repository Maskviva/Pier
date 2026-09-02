/**
 * PlotGenerator.cpp: chunk generation for a plot world.
 *
 * Each chunk refills the two layers that vary and classifies 256 columns, then makes one
 * setBlockVolume call. The column-independent parts, bedrock, fill and air, are laid down
 * once by `refillStatic`, after which each chunk overwrites only the surface and the curb
 * layer. That is why this generator stays in the same order as a vanilla flat world, and
 * why the work belongs on the C++ side.
 */
#include "pier/dimensions/dim/complete_base_types.h"

#include "pier/dimensions/plot/plot_generator.h"

#include <memory>
#include <string>

#include "mc/deps/core/string/HashedString.h"
#include "mc/world/level/BlockPos.h"
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

#include "pier/dimensions/dim/chunk_trace.h"
#include "pier/support/log.h"

namespace pier::dimensions
{
    namespace
    {
        using ::pier::hostLogger;

        constexpr int kChunkWidth = PlotLayout::kChunkWidth;
        constexpr int kTotalHeight = PlotLayout::kTotalHeight;
        constexpr int kColumns = kChunkWidth * kChunkWidth;
        constexpr int kBufferSize = kColumns * kTotalHeight;

        // The chunk memory layout is XZY: index = (x*16 + z)*totalHeight + y
        [[nodiscard]] constexpr int bufferIndex(int x, int yIdx, int z)
        {
            return (x * kChunkWidth + z) * kTotalHeight + yIdx;
        }

        /** Fills an entire y layer with one block. */
        void fillLayer(std::vector<Block const*>& buf, Block const* block, int yIdx)
        {
            for (int i = 0; i < kColumns; ++i) buf[static_cast<size_t>(yIdx + i * kTotalHeight)] = block;
        }

        void fillRange(std::vector<Block const*>& buf, Block const* block, int fromIdx, int toIdxExclusive)
        {
            for (int y = fromIdx; y < toIdxExclusive; ++y) fillLayer(buf, block, y);
        }

        // getDefaultBlockState returns air rather than null when it finds nothing, so
        // logNotFound=true is passed and a misconfigured block id leaves a trace in the
        // log instead of quietly becoming air.
        Block const* lookupBlock(HashedString const& id)
        {
            return &BlockTypeRegistry::get().getDefaultBlockState(id, true);
        }
    } // namespace

    PlotGenerator::PlotGenerator(
        Dimension& dimension, uint seed, Json::Value const& options, PlotLayout const& layout
    )
        : FlatWorldGenerator(dimension, seed, options), mLayout(layout)
    {
        mLayout.clamp();

        auto& level = dimension.mLevel;
        mBiome = level.getBiomeRegistry().lookupByName(mLayout.biome);
        if (!mBiome)
        {
            // A misspelled biome name must not stop the server from starting, so it
            // falls back to plains, but it has to say so, otherwise configuring a cherry
            // grove and walking into plains becomes a report with no findable cause.
            hostLogger().warn("[plot] unknown biome '{}', falling back to minecraft:plains", mLayout.biome);
            mBiome = level.getBiomeRegistry().lookupByName("minecraft:plains");
        }
        if (mBiome) mBiomeSource = std::make_unique<FixedBiomeSource>(*mBiome);

        mAirBlock = lookupBlock(HashedString{"minecraft:air"});
        mBedrockBlock = lookupBlock(VanillaBlockTypeIds::Bedrock());
        mFloorBlock = lookupBlock(HashedString{mLayout.floorBlock});
        mFillBlock = lookupBlock(HashedString{mLayout.fillBlock});
        mRoadBlock = lookupBlock(HashedString{mLayout.roadBlock});
        mBorderBlock = lookupBlock(HashedString{mLayout.borderBlock});

        mFloorIndexY = mLayout.floorY - PlotLayout::kMinY;
        mBorderIndexY = mFloorIndexY + 1;
    }

    PlotGenerator::ThreadBuffer& PlotGenerator::acquireBuffer()
    {
        static thread_local ThreadBuffer buf;
        if (buf.blocks.size() != static_cast<size_t>(kBufferSize))
        {
            buf.blocks.assign(static_cast<size_t>(kBufferSize), nullptr);
            buf.owner = nullptr;
        }
        if (buf.owner != static_cast<void const*>(this))
        {
            refillStatic(buf);
            buf.owner = static_cast<void const*>(this);
        }
        return buf;
    }

    /** Refills the column-independent parts, bedrock, fill and air, along with the
     *  BlockVolume view. */
    void PlotGenerator::refillStatic(ThreadBuffer& buf)
    {
        // With the dimension bottom moved to y=-512, bedrock no longer sits at buffer
        // index 0 and stays at y=-64. Everything below it is air, so the world a player
        // sees is exactly as it was, with 28 extra empty subchunks underneath to align
        // with the bottom the client hardcodes, subchunk -32.
        int const bedrockIdx = PlotLayout::kBedrockY - PlotLayout::kMinY;

        fillRange(buf.blocks, mAirBlock, 0, bedrockIdx);
        fillLayer(buf.blocks, mBedrockBlock, bedrockIdx);
        fillRange(buf.blocks, mFillBlock, bedrockIdx + 1, mFloorIndexY);
        fillRange(buf.blocks, mAirBlock, mFloorIndexY, kTotalHeight);

        buf.volume = mPrototype;
        buf.volume->mHeight = static_cast<uint>(kTotalHeight);
        buf.volume->mBlocks->mBegin = buf.blocks.data();
        buf.volume->mBlocks->mEnd = buf.blocks.data() + buf.blocks.size(); // &*end() is UB
    }

    void PlotGenerator::loadChunk(LevelChunk& lc, bool)
    {
        // The tracing switch comes from the one in chunk_trace.h and the env is not read
        // again here. Two copies of the test drifting apart gives the symptom of tracing
        // being on while the generation stage prints nothing, which looks like the
        // generator was never called.
        bool const trace = chunkTraceEnabled();
        if (trace)
        {
            auto const& cp = lc.mPosition.get();
            hostLogger().info(
                "[gen>  ] chunk ({}, {}) entering PlotGenerator::loadChunk, state number {}",
                cp.x, cp.z, static_cast<int>(lc.mLoadState->load())
            );
        }

        auto& buf = acquireBuffer();

        // The two varying layers are reset before each chunk, so the curb and road
        // blocks the previous chunk left behind cannot bleed into this one.
        fillLayer(buf.blocks, mFloorBlock, mFloorIndexY);
        fillLayer(buf.blocks, mAirBlock, mBorderIndexY);

        auto const& chunkPos = lc.mPosition.get();
        int const startX = chunkPos.x * kChunkWidth;
        int const startZ = chunkPos.z * kChunkWidth;
        int const cell = mLayout.cellSize();

        for (int x = 0; x < kChunkWidth; ++x)
        {
            int const ix = positiveMod(startX + x, cell);
            auto const ax = classify1D(ix, mLayout);

            for (int z = 0; z < kChunkWidth; ++z)
            {
                int const iz = positiveMod(startZ + z, cell);
                auto const az = classify1D(iz, mLayout);
                auto const area = combine2D(ax, az);

                switch (area)
                {
                case PlotArea1D::Road:
                    buf.blocks[static_cast<size_t>(bufferIndex(x, mFloorIndexY, z))] = mRoadBlock;
                    break;
                case PlotArea1D::Border:
                    // The surface keeps the plot block and a curb is added above it
                    buf.blocks[static_cast<size_t>(bufferIndex(x, mBorderIndexY, z))] = mBorderBlock;
                    break;
                case PlotArea1D::Plot:
                    break; // Already the surface block with air above
                }
            }
        }

        lc.setBlockVolume(*buf.volume, 0);
        if (mBiomeSource) mBiomeSource->fillBiomes(lc, nullptr);
        lc.recomputeHeightMap(false);
        lc.setSaved();

        // LevelChunk::tryChangeState is used and no CAS is written by hand.
        // tryChangeState is an exported function that also wakes the chain waiting on
        // this chunk, while a hand-written CAS only swaps the value and is entirely
        // silent on failure: with the state anything other than exactly Generating, the
        // step does nothing, prints nothing, and the chunk stays there forever.
        if (!lc.tryChangeState(ChunkState::Generating, ChunkState::Generated))
        {
            hostLogger().error(
                "[plot] chunk ({}, {}) failed the Generating to Generated transition, state "
                "number {}; this chunk will not be sent to the client",
                chunkPos.x, chunkPos.z, static_cast<int>(lc.mLoadState->load())
            );
        }
        else if (trace)
        {
            hostLogger().info("[gen   ] chunk ({}, {}) generated", chunkPos.x, chunkPos.z);
        }
    }
} // namespace pier::dimensions
