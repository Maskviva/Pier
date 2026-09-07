/**
 * NoiseGenerator.cpp: density terrain. d(x,y,z) = gradient(y) + sum(octaves) * islands(x,z);
 * solid where d > threshold; blocks by depth below the local surface; optional fluid and
 * bedrock. Every noise object is the engine's own with an exported constructor.
 *
 * Biome placement is two more Perlin fields, temperature and humidity, matched against the
 * climate boxes of the spec and written per column with LevelChunk::_setBiome. The four
 * remaining engine climate axes are not sampled: they are properties of the vanilla terrain
 * shaper, and this terrain has its own shape, so a value for them would be invented rather
 * than measured. The constructor says so once when a target constrains them.
 *
 * Per chunk: 16*16*height density samples, an order above a flat world and the same order
 * as the vanilla end. A coarser sample grid with trilinear interpolation is the optimization
 * to reach for if generation lags; the shape does not change, only the cost.
 */
#include "pier/dimensions/dim/complete_base_types.h"

#include "pier/dimensions/gen/noise_generator.h"

#include <cmath>

#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/core/string/HashedString.h"
#include "mc/world/level/ChunkBlockPos.h"
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
            // BlockTypeRegistry::get() is inlined away in 26.32. The static that held
            // the registry is still exported, and Bedrock::Owner keeps the object in a
            // public mValue, which is the reference the accessor handed back.
            return &BlockTypeRegistry::mBlockTypeRegistry().mValue.getDefaultBlockState(
                HashedString{id}, true);
        }

        /** [0,1] to [-1,1]. Every threshold in the spec is written on that scale. */
        constexpr float signed_(float v) { return v * 2.f - 1.f; }
    } // namespace

    NoiseGenerator::NoiseGenerator(Dimension& dimension, uint seed, Json::Value const& options, spec::Noise const& noise, int minY, int maxY)
        : FlatWorldGenerator(dimension, seed, options), mNoise(noise), mMinY(minY), mMaxY(maxY)
    {
        auto& level = dimension.mLevel;
        auto& registry = level.getBiomeRegistry();

        mAir = lookupBlock("minecraft:air");
        for (auto const& p : mNoise.palette) mPalette.push_back({lookupBlock(p.block), p.depthLo, p.depthHi});
        if (mNoise.fluid) mFluid = lookupBlock(mNoise.fluid->first);
        if (mNoise.bedrock) mBedrock = lookupBlock(mNoise.bedrock->first);

        uint salt = 0;
        for (auto const& o : mNoise.octaves) mOctaves.push_back(std::make_unique<PerlinNoise>(seed ^ (0x9E37u * ++salt), o.levels));
        if (mNoise.islands) mIslandNoise = std::make_unique<PerlinNoise>(seed ^ 0x51A7Du, 3);

        bool constrainsTemperature = false, constrainsHumidity = false, constrainsOthers = false;
        for (auto const& b : mNoise.biomes)
        {
            auto* biome = registry.lookupByName(b.biome);
            if (!biome)
            {
                hostLogger().error("[noise] biome '{}' is not in the registry; that target is dropped for this session", b.biome);
                continue;
            }
            mTargets.emplace_back(b, biome);
            auto narrow = [](spec::Range const& r) { return r.lo > -1.f || r.hi < 1.f; };
            constrainsTemperature |= narrow(b.temperature);
            constrainsHumidity |= narrow(b.humidity);
            constrainsOthers |= narrow(b.continentalness) || narrow(b.erosion) || narrow(b.depth) || narrow(b.weirdness);
        }
        if (constrainsOthers)
        {
            hostLogger().warn("[noise] the climate table constrains continentalness, erosion, depth or weirdness; those axes belong to the vanilla terrain shaper and are not sampled here, so those bounds are ignored and only temperature and humidity decide placement");
        }
        if (constrainsTemperature) mTemperature = std::make_unique<PerlinNoise>(seed ^ 0x7E39Fu, 2);
        if (constrainsHumidity) mHumidity = std::make_unique<PerlinNoise>(seed ^ 0x48D19u, 2);

        if (mTargets.empty())
        {
            hostLogger().error("[noise] no biome target resolved; the dimension falls back to minecraft:plains everywhere");
            mBiome = registry.lookupByName("minecraft:plains");
        }
        else
        {
            mBiome = mTargets.front().second;
        }
        if (mBiome) mBiomeSource = std::make_unique<FixedBiomeSource>(*mBiome);
    }

    Biome const* NoiseGenerator::biomeAt(int x, int z) const
    {
        if (mTargets.size() <= 1) return nullptr;
        // getValueNormalized is inlined away in 26.32; it was getValue scaled by the
        // public mNormalizationFactor, so the scaling is written out at each call.
        // The climate fields are low frequency on purpose: a biome the size of a chunk is
        // a checkerboard, not a biome.
        float const t = mTemperature ? signed_(mTemperature->getValue(Vec3{x * 0.0009f, 0.f, z * 0.0009f}) * mTemperature->mNormalizationFactor) : 0.f;
        float const h = mHumidity ? signed_(mHumidity->getValue(Vec3{x * 0.0011f, 64.f, z * 0.0011f}) * mHumidity->mNormalizationFactor) : 0.f;
        for (auto const& [target, biome] : mTargets)
        {
            if (t < target.temperature.lo || t > target.temperature.hi) continue;
            if (h < target.humidity.lo || h > target.humidity.hi) continue;
            return biome;
        }
        return nullptr;
    }

    float NoiseGenerator::density(int x, int y, int z) const
    {
        float const span = static_cast<float>(mNoise.gradientToY - mNoise.gradientFromY);
        float t = span == 0.f ? 0.f : (static_cast<float>(y - mNoise.gradientFromY) / span);
        float d = 1.f - 2.f * std::clamp(t, 0.f, 1.f);
        float n = 0.f;
        for (size_t i = 0; i < mOctaves.size(); ++i)
        {
            auto const& o = mNoise.octaves[i];
            // getValueNormalized is the only one of the two whose range is stated, [0,1];
            // getValue's depends on the octave count. Mapping to [-1,1] keeps the sum
            // centered, so the gradient alone decides where the surface sits.
            n += signed_(mOctaves[i]->getValue(Vec3{x * o.scaleXZ, y * o.scaleY, z * o.scaleXZ}) * mOctaves[i]->mNormalizationFactor) * o.amplitude;
        }
        if (mIslandNoise)
        {
            float const mask = signed_(mIslandNoise->getValue(Vec3{x * mNoise.islands->first, 0.f, z * mNoise.islands->first}) * mIslandNoise->mNormalizationFactor);
            if (mask < mNoise.islands->second) return -1.f;
            n *= (mask - mNoise.islands->second) / (1.f - mNoise.islands->second);
        }
        return d + n;
    }

    Block const* NoiseGenerator::blockAtDepth(int depth) const
    {
        for (auto const& p : mPalette)
            if (depth >= p.depthLo && depth <= p.depthHi) return p.block;
        return mPalette.empty() ? mAir : mPalette.back().block;
    }

    NoiseGenerator::ThreadBuffer& NoiseGenerator::acquireBuffer()
    {
        static thread_local ThreadBuffer buf;
        size_t const size = static_cast<size_t>(kColumns) * static_cast<size_t>(mMaxY - mMinY);
        if (buf.blocks.size() != size || buf.owner != static_cast<void const*>(this))
        {
            buf.blocks.assign(size, nullptr);
            buf.volume = mPrototype;
            buf.volume->mHeight = static_cast<uint>(mMaxY - mMinY);
            buf.volume->mBlocks->mBegin = buf.blocks.data();
            buf.volume->mBlocks->mEnd = buf.blocks.data() + buf.blocks.size();
            buf.owner = static_cast<void const*>(this);
        }
        return buf;
    }

    void NoiseGenerator::loadChunk(LevelChunk& lc, bool)
    {
        auto& buf = acquireBuffer();
        int const height = mMaxY - mMinY;
        auto const& chunkPos = lc.mPosition.get();
        int const startX = chunkPos.x * kW;
        int const startZ = chunkPos.z * kW;
        int const fluidIdx = mNoise.fluid ? mNoise.fluid->second - mMinY : -1;
        int const bedrockIdx = mNoise.bedrock ? mNoise.bedrock->second - mMinY : -1;

        for (int x = 0; x < kW; ++x)
        {
            for (int z = 0; z < kW; ++z)
            {
                size_t const column = static_cast<size_t>((x * kW + z) * height);
                // Top down, so the depth below the surface is known when a solid cell is met.
                int depth = -1;
                for (int yIdx = height - 1; yIdx >= 0; --yIdx)
                {
                    int const y = mMinY + yIdx;
                    bool solid = density(startX + x, y, startZ + z) > mNoise.threshold;
                    if (yIdx == bedrockIdx) solid = true;
                    Block const* b;
                    if (solid)
                    {
                        depth = depth < 0 ? 0 : depth + 1;
                        b = yIdx == bedrockIdx ? mBedrock : blockAtDepth(depth);
                    }
                    else
                    {
                        depth = -1;
                        b = (fluidIdx >= 0 && yIdx <= fluidIdx) ? mFluid : mAir;
                    }
                    buf.blocks[column + static_cast<size_t>(yIdx)] = b;
                }
            }
        }
        lc.setBlockVolume(*buf.volume, 0);
        if (mBiomeSource) mBiomeSource->fillBiomes(lc, nullptr);
        // Then the columns that want a different biome. fillBiomes wrote the fixed one over
        // the whole chunk, so only the differences are written here.
        for (int x = 0; x < kW; ++x)
        {
            for (int z = 0; z < kW; ++z)
            {
                if (auto const* biome = biomeAt(startX + x, startZ + z); biome && biome != mBiome)
                {
                    lc._setBiome(*biome, ChunkBlockPos{static_cast<uchar>(x), ChunkLocalHeight{0}, static_cast<uchar>(z)}, true);
                }
            }
        }
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
            hostLogger().error("[noise] chunk ({}, {}) failed the Generating to Generated transition; it will not be sent", chunkPos.x, chunkPos.z);
        }
    }
} // namespace pier::dimensions
