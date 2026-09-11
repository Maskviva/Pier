/**
 * SuppliedGenerator.cpp: fills a chunk from whatever the owning mod returns.
 *
 * The buffer is rewritten whole each call from the indices the mod wrote, with no
 * static rows, and the prototype BlockVolume is re-pointed at it per call, which is
 * what the upstream flat generator does with its own. Biomes go in per column through
 * LevelChunk::_setBiome with the whole column filled, the shape Bedrock stores for a
 * dimension without a 3D biome source.
 *
 * This runs on chunk worker threads and calls out of the process image into a mod. The
 * contract that makes it safe is on PierGenerateChunkFn in abi.h; what is enforced here
 * is the part a caller cannot get wrong by being careful: the buffer is the host's, the
 * indices are bounds-checked before they become pointers, and a throw does not cross
 * back in.
 */
#include "pier/dimensions/dim/complete_base_types.h"

#include "pier/dimensions/gen/supplied_generator.h"

#include <algorithm>
#include <atomic>
#include <unordered_map>

#include "ll/api/service/Bedrock.h"

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
            return &BlockTypeRegistry::mBlockTypeRegistry().mValue.getDefaultBlockState(HashedString{id}, true);
        }

        /** Newline-separated, trimmed, empty lines dropped. The palettes arrive this way
         *  because a mod writes them once and the host reads them once; a structured
         *  encoding for two lists of names buys nothing and has to be parsed on both
         *  sides. */
        std::vector<std::string> lines(std::string const& text)
        {
            std::vector<std::string> out;
            std::size_t at = 0;
            while (at <= text.size())
            {
                auto end = text.find('\n', at);
                if (end == std::string::npos) end = text.size();
                auto line = text.substr(at, end - at);
                while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
                std::size_t lead = 0;
                while (lead < line.size() && line[lead] == ' ') ++lead;
                line = line.substr(lead);
                if (!line.empty()) out.push_back(std::move(line));
                if (end == text.size()) break;
                at = end + 1;
            }
            return out;
        }
    } // namespace

    std::shared_ptr<SuppliedTerrain const>
    resolveSuppliedTerrain(std::string const& dimName, std::string const& materialPalette,
                           std::string const& biomePalette, PierGenerateChunkFn fn, void* user,
                           std::string const& owner, std::vector<std::string>& problems)
    {
        if (!fn)
        {
            problems.push_back("no generator callback was given, so nothing would fill the terrain");
            return nullptr;
        }
        auto mats = lines(materialPalette);
        auto bios = lines(biomePalette);
        if (mats.empty() || bios.empty())
        {
            problems.push_back("the material and biome palettes must each name at least one entry");
            return nullptr;
        }
        // Index 0 is what an untouched column holds, and the mod is entitled to leave
        // columns untouched. Anything but air there turns "nothing here" into a solid
        // world, which is not a mistake the terrain can recover from.
        if (mats.front() != "minecraft:air")
        {
            problems.push_back("material palette entry 0 is '" + mats.front()
                               + "'; it must be minecraft:air, which is what an untouched column holds");
            return nullptr;
        }
        if (mats.size() > 0xFFFF || bios.size() > 0xFFFF)
        {
            problems.push_back("a palette has more than 65535 entries and the indices are 16 bit");
            return nullptr;
        }

        auto out = std::make_shared<SuppliedTerrain>();
        out->fn = fn;
        out->user = user;
        out->owner = owner;

        auto level = ll::service::getLevel();
        if (!level)
        {
            problems.push_back("Level is not open, so no palette can be resolved");
            return nullptr;
        }
        auto& biomeRegistry = level->getBiomeRegistry();
        for (auto const& name : mats)
        {
            // Every unknown name, not the first: a palette holds a handful and stopping
            // at one turns fixing them into a restart each.
            auto const* b = lookupBlock(name);
            if (!b)
            {
                problems.push_back("block '" + name + "' is not in the registry");
                continue;
            }
            out->materials.push_back(b);
        }
        for (auto const& name : bios)
        {
            Biome const* b = biomeRegistry.lookupByName(name);
            if (!b)
            {
                problems.push_back("biome '" + name + "' is not in the registry. Custom biomes come from a "
                                   "behavior pack loaded before mods; check the pack and the spelling");
                continue;
            }
            out->biomes.push_back(b);
        }
        if (out->materials.size() != mats.size() || out->biomes.size() != bios.size())
        {
            problems.push_back("'" + dimName + "' was not registered: a palette names something the registries "
                               "do not have, and a missing entry becomes a hole in the terrain that shows up "
                               "only once someone walks into it");
            return nullptr;
        }
        return out;
    }

    SuppliedGenerator::SuppliedGenerator(Dimension& dimension, uint seed, Json::Value const& options,
                                         std::shared_ptr<SuppliedTerrain const> terrain,
                                         std::int32_t minY, std::int32_t height)
        : FlatWorldGenerator(dimension, seed, options)
        , mTerrain(std::move(terrain))
        , mMinY(minY)
        , mHeight(height)
        , mDimId(static_cast<std::int32_t>(dimension.getDimensionId()))
        , mDimName(dimension.mName.get())
    {
        mBiome = mTerrain && !mTerrain->biomes.empty()
                     ? mTerrain->biomes.front()
                     : dimension.mLevel.getBiomeRegistry().lookupByName("minecraft:plains");
        if (mBiome) mBiomeSource = std::make_unique<FixedBiomeSource>(*mBiome);
    }

    SuppliedGenerator::ThreadBuffer& SuppliedGenerator::acquireBuffer()
    {
        static thread_local ThreadBuffer buf;
        auto const size = static_cast<std::size_t>(kColumns) * static_cast<std::size_t>(mHeight);
        if (buf.blocks.size() != size)
        {
            buf.blocks.assign(size, nullptr);
            buf.materials.assign(size, 0);
            buf.biomes.assign(static_cast<std::size_t>(kColumns), 0);
        }
        buf.volume = mPrototype;
        buf.volume->mHeight = static_cast<uint>(mHeight);
        buf.volume->mBlocks->mBegin = buf.blocks.data();
        buf.volume->mBlocks->mEnd = buf.blocks.data() + buf.blocks.size();
        return buf;
    }

    void SuppliedGenerator::loadChunk(LevelChunk& lc, bool)
    {
        auto& buf = acquireBuffer();
        auto const& chunkPos = lc.mPosition.get();
        std::fill(buf.materials.begin(), buf.materials.end(), static_cast<std::uint16_t>(0));
        std::fill(buf.biomes.begin(), buf.biomes.end(), static_cast<std::uint16_t>(0));

        PierChunkRequest req{};
        req.dim_id = mDimId;
        req.chunk_x = chunkPos.x;
        req.chunk_z = chunkPos.z;
        req.min_y = mMinY;
        req.height = mHeight;
        req.out_materials = buf.materials.data();
        req.out_biomes = buf.biomes.data();

        bool filled = false;
        try
        {
            filled = mTerrain && mTerrain->fn && mTerrain->fn(mTerrain->user, &req) != 0;
        }
        catch (...)
        {
            // A throw here would unwind through the engine's chunk pipeline. Air and a
            // line is the worst honest answer; letting it out is not an answer at all.
            filled = false;
        }
        if (!filled)
        {
            static std::atomic<bool> said{false};
            if (!said.exchange(true))
            {
                hostLogger().error(
                    "[supplied] '{}' did not fill chunk ({}, {}) and this dimension is generating air. "
                    "Said once; the mod that owns it is '{}'",
                    mDimName, chunkPos.x, chunkPos.z, mTerrain ? mTerrain->owner : "(none)"
                );
            }
        }

        // Bounds-checked here and not trusted from the callback: an index past the
        // palette is a pointer this host would then hand to the chunk pipeline.
        auto const matCount = static_cast<std::uint16_t>(mTerrain->materials.size());
        for (std::size_t i = 0; i < buf.blocks.size(); ++i)
        {
            auto idx = buf.materials[i];
            buf.blocks[i] = mTerrain->materials[idx < matCount ? idx : 0];
        }
        lc.setBlockVolume(*buf.volume, 0);

        auto const bioCount = static_cast<std::uint16_t>(mTerrain->biomes.size());
        for (int x = 0; x < kW; ++x)
        {
            for (int z = 0; z < kW; ++z)
            {
                auto idx = buf.biomes[static_cast<std::size_t>(x * kW + z)];
                auto const* biome = mTerrain->biomes[idx < bioCount ? idx : 0];
                if (biome)
                {
                    lc._setBiome(*biome,
                                 ChunkBlockPos{static_cast<uchar>(x), ChunkLocalHeight{static_cast<short>(0)},
                                               static_cast<uchar>(z)},
                                 true);
                }
            }
        }
        lc.recomputeHeightMap(false);
        if (!lc.tryChangeState(ChunkState::Generating, ChunkState::Generated))
        {
            hostLogger().error("[supplied] chunk ({}, {}) failed the Generating to Generated transition; it will not be sent", chunkPos.x, chunkPos.z);
        }
    }
} // namespace pier::dimensions

namespace pier::dimensions
{
    namespace
    {
        /** Name to terrain, for the dimensions a mod supplies.
         *
         *  Written only from the server thread, at registration. A generator takes its
         *  shared_ptr once, in createGenerator, and the chunk threads work from that
         *  copy; the map itself never crosses a thread boundary.
         */
        std::unordered_map<std::string, std::shared_ptr<SuppliedTerrain const>>& terrains()
        {
            static std::unordered_map<std::string, std::shared_ptr<SuppliedTerrain const>> map;
            return map;
        }
    } // namespace

    void rememberSuppliedTerrain(std::string const& dimName, std::shared_ptr<SuppliedTerrain const> terrain)
    {
        terrains()[dimName] = std::move(terrain);
    }

    std::shared_ptr<SuppliedTerrain const> suppliedTerrainOf(std::string const& dimName)
    {
        auto it = terrains().find(dimName);
        return it == terrains().end() ? nullptr : it->second;
    }

    void forgetSuppliedTerrain(std::string const& dimName)
    {
        terrains().erase(dimName);
    }
} // namespace pier::dimensions
