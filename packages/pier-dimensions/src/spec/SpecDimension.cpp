/**
 * SpecDimension.cpp: sky, height and generator choice, all read from the stored spec.
 *
 * What is read here is the payload already stored in dimension_config.json and not the
 * argument of the call, because generateNewData runs once when the dimension is first
 * created. A dimension built from a spec stays that spec across restarts; a pack edit on
 * the mod side does not reach it, by design: the stored hash pins the binary.
 *
 * The three structure symbols in the native branch are resolved lazily and not at load:
 * resolving them eagerly pulls in the structure registry before the engine has built it.
 */
#include "pier/dimensions/dim/complete_base_types.h"

#include "pier/dimensions/spec/spec_dimension.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "magic_enum.hpp"

#include "ll/api/memory/Memory.h"

#include "mc/common/Brightness.h"
#include "mc/deps/core/math/Color.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/DimensionConversionData.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/LevelSeed64.h"
#include "mc/world/level/biome/registry/BiomeRegistry.h"
#include "mc/world/level/biome/source/FixedBiomeSource.h"
#include "mc/world/level/dimension/DimensionArguments.h"
#include "mc/world/level/dimension/NetherBrightnessRamp.h"
#include "mc/world/level/dimension/OverworldBrightnessRamp.h"
#include "mc/world/level/dimension/OverworldDimension.h"
#include "mc/world/level/dimension/VanillaDimensions.h"
#include "mc/world/level/levelgen/VoidGenerator.h"
#include "mc/world/level/levelgen/flat/FlatWorldGenerator.h"
#include "mc/world/level/levelgen/structure/EndCityFeature.h"
#include "mc/world/level/levelgen/structure/StructureFeatureRegistry.h"
#include "mc/world/level/levelgen/v1/NetherGenerator.h"
#include "mc/world/level/levelgen/v1/OverworldGeneratorMultinoise.h"
#include "mc/world/level/levelgen/v1/TheEndGenerator.h"
#include "mc/world/level/levelgen/v2/ChunkGeneratorStructureState.h"
#include "mc/world/level/storage/Experiments.h"
#include "mc/world/level/storage/LevelData.h"

#include "pier/dimensions/base/utils.h"
#include "pier/dimensions/dim/custom_dimension_manager.h"
#include "pier/dimensions/gen/template_generator.h"
#include "pier/dimensions/gen/volume_generator.h"
#include "pier/dimensions/pack/pack_locate.h"
#include "pier/support/log.h"

namespace pier::dimensions
{
    namespace
    {
        using ::pier::hostLogger;
        using namespace ll::memory_literals;

        void* overworldAddress()
        {
            static void* p = "`anonymous namespace'::OverworldDimensionAnon::addStructureFeatures"_sym.resolve(true);
            return p;
        }
        void* netherAddress()
        {
            static void* p = "`anonymous namespace'::NetherDimensionAnon::addStructureFeatures"_sym.resolve(true);
            return p;
        }
        void* endcityAddress()
        {
            static void* p =
                "??$addStructureFeature@VEndCityFeature@@AEAVDimension@@AEAI@StructureFeatureRegistry@@QEAAAEAVEndCityFeature@@AEAVDimension@@AEAI@Z"_sym
                    .resolve(true);
            return p;
        }

        void overworldAddStructureFeatures(StructureFeatureRegistry& registry, uint seed, bool isLegacy, BaseGameVersion const& v)
        {
            auto* addr = overworldAddress();
            if (!addr) { hostLogger().warn("[dim] OverworldDimensionAnon::addStructureFeatures not found; custom overworld dimensions generate no structures"); return; }
            ll::memory::addressCall<void*, StructureFeatureRegistry&, uint, bool, BaseGameVersion const&>(addr, registry, seed, isLegacy, v);
        }
        void netherAddStructureFeatures(StructureFeatureRegistry& registry, uint seed, BaseGameVersion const& v, Experiments const& e)
        {
            auto* addr = netherAddress();
            if (!addr) { hostLogger().warn("[dim] NetherDimensionAnon::addStructureFeatures not found; custom nether dimensions generate no structures"); return; }
            ll::memory::addressCall<void*, StructureFeatureRegistry&, uint, BaseGameVersion const&, Experiments const&>(addr, registry, seed, v, e);
        }
        void createEndCityFeature(StructureFeatureRegistry* self, Dimension& dimension, uint& seed)
        {
            auto* addr = endcityAddress();
            if (!addr) { hostLogger().warn("[dim] StructureFeatureRegistry::addStructureFeature<EndCityFeature> not found; custom end dimensions generate no end cities"); return; }
            ll::memory::addressCall<EndCityFeature&, StructureFeatureRegistry*, Dimension&, uint&>(addr, self, dimension, seed);
        }

        /** The engine's void generator with a fixed biome, plains when the named one is
         *  missing so that a void never fails to build. */
        std::unique_ptr<WorldGenerator> voidWith(Dimension& dim, std::string const& biome)
        {
            auto v = std::make_unique<VoidGenerator>(dim);
            v->mBiome = dim.mLevel.getBiomeRegistry().lookupByName(biome);
            if (!v->mBiome) v->mBiome = dim.mLevel.getBiomeRegistry().lookupByName("minecraft:plains");
            if (v->mBiome) v->mBiomeSource = std::make_unique<FixedBiomeSource>(*v->mBiome);
            return v;
        }

        spec::DimensionSpec specOf(std::string const& name, CompoundTag const& stored)
        {
            auto [s, problems] = spec::DimensionSpec::fromNbt(stored);
            for (auto const& p : problems) hostLogger().warn("[dim] '{}': {}", name, p);
            if (!s)
            {
                // Refusing here would fastfail on a chunk thread. The dimension is already
                // registered by id; the least harmful shape is a void with the stored seed.
                hostLogger().error("[dim] '{}': the stored spec could not be read, generating a void. The terrain differs from what was chosen at creation; fix dimension_config.json and restart", name);
                spec::DimensionSpec v;
                v.terrain = spec::Native{GeneratorType::Void};
                return v;
            }
            return *s;
        }
    } // namespace

    SpecDimension::SpecDimension(std::string const& name, DimensionFactoryInfo const& info)
        : Dimension(DimensionArguments(std::move(info.arguments), info.dimId, spec::dimensionHeightOf(info.data), name, name))
        , mSpec(specOf(name, info.data))
    {
        mDefaultBrightness->sky = Brightness::MAX();
        mHasWeather = mSpec.sky.weather;
        if (mSpec.sky.client == GeneratorType::Nether)
        {
            mSeaLevel = 32;
            mDimensionBrightnessRamp = std::make_unique<NetherBrightnessRamp>();
        }
        else
        {
            mSeaLevel = 63;
            mDimensionBrightnessRamp = std::make_unique<OverworldBrightnessRamp>();
        }
        mDimensionBrightnessRamp->buildBrightnessRamp();
    }

    CompoundTag SpecDimension::generateNewData(std::string const& verbatimSnbt)
    {
        // The parsed tag, not the caller's bytes. CustomDimensionManager writes it back
        // out with SnbtFormat::Minimize, so dimension_config.json holds the normalized
        // form and the equality it compares is structural, not byte for byte. A mod-side
        // record that keeps the caller's own spacing will not match it as bytes.
        // Slots.cpp parsed it already; a parse failure here cannot happen and is not
        // papered over with a default.
        auto tag = CompoundTag::fromSnbt(verbatimSnbt);
        if (!tag) throw std::runtime_error("the dimension spec that parsed a moment ago does not parse now");
        return *tag;
    }

    /*
     * The sky angle this dimension shows, from its own clock rather than the level's.
     *
     * sky.time belongs beside skylight and weather: all three describe what the sky does
     * and none of them is a property of the terrain. Holding the tick here rather than in
     * a mod keeps a lock on one dimension from moving anyone else's, which is what
     * set_time would do, and needs no packet hook to undo per player.
     *
     * The base implementation is what an unlocked dimension does, so a spec without the
     * field behaves exactly as before.
     */
    float SpecDimension::getTimeOfDay(int time, float a) const
    {
        return Dimension::getTimeOfDay(mSpec.sky.time ? *mSpec.sky.time : time, a);
    }

    void SpecDimension::init(br::worldgen::StructureSetRegistry const& structureSetRegistry)
    {
        // Skylight follows the sky, not the generator: layered terrain under an end sky is a
        // legitimate combination and it must be dark the way the end is.
        mHasSkylight = mSpec.sky.skylight;
        Dimension::init(structureSetRegistry);
        verifyHeightRange(*this, mSpec.minY, mSpec.maxY, "SpecDimension");
    }

    std::unique_ptr<WorldGenerator> SpecDimension::createGenerator(br::worldgen::StructureSetRegistry const& structureSetRegistry)
    {
        auto& level = mLevel;
        auto& levelData = level.getLevelData();
        auto const seed = mSpec.seed;
        std::unique_ptr<WorldGenerator> gen;

        if (auto const* n = std::get_if<spec::Native>(&mSpec.terrain))
        {
            auto biome = level.getBiomeRegistry().lookupByName(levelData.mBiomeOverride);
            switch (n->generator)
            {
            case GeneratorType::Overworld:
                gen = std::make_unique<OverworldGeneratorMultinoise>(*this, LevelSeed64{seed}, biome);
                gen->mStructureFeatureRegistry->mGeneratorState = br::worldgen::ChunkGeneratorStructureState::createNormal(seed, gen->getBiomeSource(), structureSetRegistry);
                overworldAddStructureFeatures(*gen->mStructureFeatureRegistry, seed, false, levelData.getBaseGameVersion());
                break;
            case GeneratorType::Nether:
                gen = std::make_unique<NetherGenerator>(*this, seed, biome);
                gen->mStructureFeatureRegistry->mGeneratorState = br::worldgen::ChunkGeneratorStructureState::createNormal(seed, gen->getBiomeSource(), structureSetRegistry);
                netherAddStructureFeatures(*gen->mStructureFeatureRegistry, seed, levelData.getBaseGameVersion(), static_cast<Experiments&>(levelData.mExperiments.get()));
                break;
            case GeneratorType::TheEnd:
            {
                uint s = seed;
                gen = std::make_unique<TheEndGenerator>(*this, seed, biome);
                gen->mStructureFeatureRegistry->mGeneratorState = br::worldgen::ChunkGeneratorStructureState::createNormal(seed, gen->getBiomeSource(), structureSetRegistry);
                createEndCityFeature(gen->mStructureFeatureRegistry.get(), *this, s);
                break;
            }
            case GeneratorType::Flat:
                gen = std::make_unique<FlatWorldGenerator>(*this, seed, levelData.mFlatWorldOptions);
                gen->mStructureFeatureRegistry->mGeneratorState = br::worldgen::ChunkGeneratorStructureState::createFlat(seed, gen->getBiomeSource(), {});
                break;
            case GeneratorType::Void:
            default:
            {
                if (n->generator != GeneratorType::Void)
                    hostLogger().error("[dim] '{}' has a native generator {} with no branch; this is a bug", mName.get(), magic_enum::enum_name(n->generator));
                gen = voidWith(*this, n->biome);
                gen->mStructureFeatureRegistry->mGeneratorState = br::worldgen::ChunkGeneratorStructureState::createFlat(seed, gen->getBiomeSource(), {});
            }
            }
            return gen;
        }

        auto const& p = std::get<spec::Pack>(mSpec.terrain);
        std::vector<std::string> problems;
        pack::PackStatus status = pack::PackStatus::Ok;
        if (p.isTemplate())
        {
            auto tpl = pack::PackCache::instance().templateAt(p.path, p.sha256, "", status, problems);
            std::optional<pack::MountedTemplate> mounted;
            if (tpl) mounted = pack::mountTemplate(*tpl, p.params, p.roles, mSpec.minY, mSpec.maxY, problems);
            if (tpl && mounted)
            {
                gen = std::make_unique<TemplateGenerator>(*this, seed, levelData.mFlatWorldOptions, tpl, std::move(*mounted));
                gen->mStructureFeatureRegistry->mGeneratorState = br::worldgen::ChunkGeneratorStructureState::createFlat(seed, gen->getBiomeSource(), {});
                return gen;
            }
        }
        else
        {
            auto vol = pack::PackCache::instance().volumeAt(p.path, p.sha256, "", status, problems);
            if (vol)
            {
                if (mSpec.minY != vol->minY() || mSpec.maxY != vol->maxY())
                    problems.push_back("the stored height does not match the pack's fixed range");
                else
                {
                    auto seeded = std::make_shared<pack::SeededVolume const>(vol, seed);
                    gen = std::make_unique<VolumeGenerator>(*this, seed, levelData.mFlatWorldOptions, std::move(seeded));
                    gen->mStructureFeatureRegistry->mGeneratorState = br::worldgen::ChunkGeneratorStructureState::createFlat(seed, gen->getBiomeSource(), {});
                    return gen;
                }
            }
        }
        // Refusing here would fastfail on a chunk thread. The dimension is already
        // registered by id; a void with an error line is the least harmful shape, and
        // the stored spec stays so the terrain returns once the pack is back in place.
        for (auto const& msg : problems) hostLogger().error("[dim] '{}': pack '{}': {}", mName.get(), p.path, msg);
        hostLogger().error("[dim] '{}': the terrain pack cannot be mounted (status {}); generating a void for this session", mName.get(), static_cast<int>(status));
        gen = voidWith(*this, "minecraft:plains");
        gen->mStructureFeatureRegistry->mGeneratorState = br::worldgen::ChunkGeneratorStructureState::createFlat(seed, gen->getBiomeSource(), {});
        return gen;
    }

    /*
     * The four chunk-upgrade overrides route to the overworld's own bodies. The
     * VanillaLevelChunkUpgrade functions they called are inlined away and have no symbol
     * left, while the overworld overrides calling the same code are still exported as
     * thunks. OverworldDimension declares no data member of its own, so its layout is the
     * Dimension layout this class also begins with and the cast reaches only fields both
     * share. That is the whole of what makes this sound: the moment OverworldDimension
     * gains a member, the cast reads this object's own fields as that member.
     *
     * Doing nothing instead is not an option. A world carried over from an older BDS
     * holds chunks at an older storage version, and skipping the upgrade hands the
     * client chunks it cannot parse.
     */
    OverworldDimension& SpecDimension::asOverworldForUpgrade()
    {
        return *reinterpret_cast<OverworldDimension*>(static_cast<Dimension*>(this));
    }

    void SpecDimension::upgradeLevelChunk(ChunkSource& cs, LevelChunk& lc, LevelChunk& generatedChunk)
    {
        asOverworldForUpgrade().$upgradeLevelChunk(cs, lc, generatedChunk);
    }

    void SpecDimension::fixWallChunk(ChunkSource& cs, LevelChunk& lc)
    {
        asOverworldForUpgrade().$fixWallChunk(cs, lc);
    }

    bool SpecDimension::levelChunkNeedsUpgrade(LevelChunk const& lc) const
    {
        return const_cast<SpecDimension*>(this)->asOverworldForUpgrade().$levelChunkNeedsUpgrade(lc);
    }

    void SpecDimension::_upgradeOldLimboEntity(CompoundTag& tag, ::LimboEntitiesVersion vers)
    {
        asOverworldForUpgrade().$_upgradeOldLimboEntity(tag, vers);
    }

    Vec3 SpecDimension::translatePosAcrossDimension(Vec3 const& fromPos, DimensionType fromId) const
    {
        Vec3 topos;
        VanillaDimensions::convertPointBetweenDimensions(fromPos, topos, fromId, mId, mLevel.getDimensionConversionData());
        constexpr auto clampVal = 32000000.0f - 128.0f;
        topos.x = std::clamp(topos.x, -clampVal, clampVal);
        topos.z = std::clamp(topos.z, -clampVal, clampVal);
        return topos;
    }

    short SpecDimension::getCloudHeight() const { return static_cast<short>(std::min(192, mSpec.maxY - 64)); }

    std::unique_ptr<ChunkSource> SpecDimension::_wrapStorageForVersionCompatibility(std::unique_ptr<ChunkSource> cs, ::StorageVersion) { return cs; }

    mce::Color SpecDimension::getBrightnessDependentFogColor(mce::Color const& color, float brightness) const
    {
        float temp = (brightness * 0.94f) + 0.06f;
        float temp2 = (brightness * 0.91f) + 0.09f;
        auto result = color;
        result.r = color.r * temp;
        result.g = color.g * temp;
        result.b = color.b * temp2;
        return result;
    }
} // namespace pier::dimensions
