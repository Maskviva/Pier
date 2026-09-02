/**
 * SimpleCustomDimension.cpp: a vanilla generator plus its own seed.
 *
 * Two constraints are enforced in place. `init` must not set `mHasSkylight = false`
 * unconditionally, which would give every custom dimension the nether lighting model and
 * hand anyone choosing superflat a completely dark map. The default branch of
 * `createGenerator` must not build a void world silently, which a player experiences as
 * choosing the overworld and falling into the void.
 *
 * Three symbol resolutions are lazy; the reason is in `overworldAddress()` below.
 */
#include "pier/dimensions/dim/complete_base_types.h"

#include "pier/dimensions/base/simple_custom_dimension.h"

#include <algorithm>
#include <memory>
#include <string_view>

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
#include "mc/world/level/chunk/vanilla_level_chunk_upgrade/VanillaLevelChunkUpgrade.h"
#include "mc/world/level/dimension/DimensionArguments.h"
#include "mc/world/level/dimension/IClientDimensionExtensions.h"
#include "mc/world/level/dimension/NetherBrightnessRamp.h"
#include "mc/world/level/dimension/OverworldBrightnessRamp.h"
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
#include "pier/dimensions/dim/dimension_height.h"
#include "pier/support/log.h"

namespace pier::dimensions
{
    namespace
    {
        using ::pier::hostLogger;

        // These three symbols are needed only by the Overworld, Nether and TheEnd
        // generators, so they resolve lazily as function-local statics. At namespace
        // scope they would resolve as soon as the DLL loads and any BDS signature change
        // would print three FATAL lines at startup, even on a server that only uses Flat,
        // Void or Plot. A missing symbol degrades to generating no structures and warns,
        // rather than passing nullptr to addressCall.
        using namespace ll::memory_literals;

        void* overworldAddress()
        {
            static void* p =
                "`anonymous namespace'::OverworldDimensionAnon::addStructureFeatures"_sym.resolve(true);
            return p;
        }

        void* netherAddress()
        {
            static void* p =
                "`anonymous namespace'::NetherDimensionAnon::addStructureFeatures"_sym.resolve(true);
            return p;
        }

        void* endcityAddress()
        {
            static void* p =
                "??$addStructureFeature@VEndCityFeature@@AEAVDimension@@AEAI@StructureFeatureRegistry@@QEAAAEAVEndCityFeature@@AEAVDimension@@AEAI@Z"_sym
                    .resolve(true);
            return p;
        }

        void overworldAddStructureFeatures(
            StructureFeatureRegistry& registry, uint seed, bool isLegacy, BaseGameVersion const& baseGameVersion
        )
        {
            auto* addr = overworldAddress();
            if (!addr)
            {
                hostLogger().warn(
                    "[dim] symbol OverworldDimensionAnon::addStructureFeatures not found; "
                    "custom overworld dimensions will generate no structures"
                );
                return;
            }
            ll::memory::addressCall<void*, StructureFeatureRegistry&, uint, bool, BaseGameVersion const&>(
                addr, registry, seed, isLegacy, baseGameVersion
            );
        }

        void netherAddStructureFeatures(
            StructureFeatureRegistry& registry,
            uint seed,
            BaseGameVersion const& baseGameVersion,
            Experiments const& experiments
        )
        {
            auto* addr = netherAddress();
            if (!addr)
            {
                hostLogger().warn(
                    "[dim] symbol NetherDimensionAnon::addStructureFeatures not found; "
                    "custom nether dimensions will generate no structures"
                );
                return;
            }
            ll::memory::addressCall<
                void*, StructureFeatureRegistry&, uint, BaseGameVersion const&, Experiments const&>(
                addr, registry, seed, baseGameVersion, experiments
            );
        }

        void createEndCityFeature(StructureFeatureRegistry* self, Dimension& dimension, uint& seed)
        {
            auto* addr = endcityAddress();
            if (!addr)
            {
                hostLogger().warn(
                    "[dim] symbol StructureFeatureRegistry::addStructureFeature<EndCityFeature> "
                    "not found; custom end dimensions will generate no end cities"
                );
                return;
            }
            ll::memory::addressCall<EndCityFeature&, StructureFeatureRegistry*, Dimension&, uint&>(
                addr, self, dimension, seed
            );
        }
    } // namespace

    SimpleCustomDimension::SimpleCustomDimension(std::string const& name, DimensionFactoryInfo const& info)
        : // The fifth argument is mTypeId, as explained in PlotDimension.cpp.
          //
          // The height range comes from the shared constant. It must match exactly the
          // pair CustomDimensionManager hands to the DimensionDefinition, which is what
          // DimensionDataPacket sends to the client, otherwise the client crashes on a
          // subchunk index out of range as soon as it enters the dimension.
          Dimension(
              DimensionArguments(std::move(info.arguments), info.dimId, {kWorldMinY, kWorldMaxY}, name, name)
          )
    {
        mDefaultBrightness->sky = Brightness::MAX();
        // What is read here is the name already stored in the config and not the
        // argument of this call, because generateNewData runs once when the dimension is
        // first created. A dimension built with the wrong generator stays wrong across
        // any number of restarts and a code change does not correct it retroactively.
        auto const storedName = static_cast<std::string_view>(info.data["generatorType"]);
        auto generatorTypeOpt = magic_enum::enum_cast<GeneratorType>(storedName);
        if (!generatorTypeOpt)
        {
            hostLogger().error(
                "[dim] the stored generatorType of '{}' is '{}', which is unrecognized, so "
                "it falls back to Overworld; the terrain will differ from what was chosen "
                "at creation",
                name, std::string{storedName}
            );
        }
        generatorType = generatorTypeOpt.value_or(GeneratorType::Overworld);
        seed = info.data["seed"];
        switch (generatorType)
        {
        case GeneratorType::TheEnd:
            mSeaLevel = 63;
            mHasWeather = false;
            mDimensionBrightnessRamp = std::make_unique<OverworldBrightnessRamp>();
            break;
        case GeneratorType::Nether:
            mSeaLevel = 32;
            mHasWeather = false;
            mDimensionBrightnessRamp = std::make_unique<NetherBrightnessRamp>();
            break;
        default:
            mSeaLevel = 63;
            mHasWeather = true;
            mDimensionBrightnessRamp = std::make_unique<OverworldBrightnessRamp>();
        }
        mDimensionBrightnessRamp->buildBrightnessRamp();
    }

    CompoundTag SimpleCustomDimension::generateNewData(uint seed, GeneratorType generatorType)
    {
        CompoundTag result;
        result["seed"] = seed;
        result["generatorType"] = magic_enum::enum_name(generatorType);
        return result;
    }

    void SimpleCustomDimension::init(br::worldgen::StructureSetRegistry const& structureSetRegistry)
    {
        // Skylight follows what vanilla does: only the nether and the end turn it off
        // and everything else keeps it. OverworldDimension does not even override init,
        // while NetherDimension and TheEndDimension do.
        //
        // Setting `mHasSkylight = false` unconditionally would apply the nether lighting
        // model whether the choice was overworld, superflat or void, and anyone choosing
        // superflat would get a completely dark flat map where the blocks are all there
        // and standable while nothing is visible.
        switch (generatorType)
        {
        case GeneratorType::Nether:
        case GeneratorType::TheEnd:
            mHasSkylight = false;
            break;
        default:
            mHasSkylight = true;
            break;
        }
        Dimension::init(structureSetRegistry);

        // As utils.h explains, the server judges a subchunk out of range against
        // Dimension::mHeightRange while the client uses the definition inside
        // DimensionDataPacket, and the two are independent copies.
        verifyHeightRange(*this, kWorldMinY, kWorldMaxY, "SimpleCustomDimension");
    }

    std::unique_ptr<WorldGenerator>
    SimpleCustomDimension::createGenerator(br::worldgen::StructureSetRegistry const& structureSetRegistry)
    {
        auto& level = mLevel;
        auto& levelData = level.getLevelData();
        auto biome = level.getBiomeRegistry().lookupByName(levelData.mBiomeOverride);
        std::unique_ptr<WorldGenerator> worldGenerator;
        switch (generatorType)
        {
        case GeneratorType::Overworld:
            worldGenerator = std::make_unique<OverworldGeneratorMultinoise>(*this, LevelSeed64{seed}, biome);
            worldGenerator->mStructureFeatureRegistry->mGeneratorState =
                br::worldgen::ChunkGeneratorStructureState::createNormal(
                    seed, worldGenerator->getBiomeSource(), structureSetRegistry
                );
            overworldAddStructureFeatures(
                *worldGenerator->mStructureFeatureRegistry, seed, false, levelData.getBaseGameVersion()
            );
            break;
        case GeneratorType::Nether:
            worldGenerator = std::make_unique<NetherGenerator>(*this, seed, biome);
            worldGenerator->mStructureFeatureRegistry->mGeneratorState =
                br::worldgen::ChunkGeneratorStructureState::createNormal(
                    seed, worldGenerator->getBiomeSource(), structureSetRegistry
                );
            netherAddStructureFeatures(
                *worldGenerator->mStructureFeatureRegistry,
                seed,
                levelData.getBaseGameVersion(),
                static_cast<Experiments&>(levelData.mExperiments.get())
            );
            break;
        case GeneratorType::TheEnd:
            worldGenerator = std::make_unique<TheEndGenerator>(*this, seed, biome);
            worldGenerator->mStructureFeatureRegistry->mGeneratorState =
                br::worldgen::ChunkGeneratorStructureState::createNormal(
                    seed, worldGenerator->getBiomeSource(), structureSetRegistry
                );
            createEndCityFeature(worldGenerator->mStructureFeatureRegistry.get(), *this, seed);
            break;
        case GeneratorType::Flat:
            worldGenerator = std::make_unique<FlatWorldGenerator>(*this, seed, levelData.mFlatWorldOptions);
            worldGenerator->mStructureFeatureRegistry->mGeneratorState =
                br::worldgen::ChunkGeneratorStructureState::createFlat(
                    seed, worldGenerator->getBiomeSource(), {}
                );
            break;
        default:
            // Any value other than Void reaching here is a bug, whether Legacy,
            // Undefined or an out-of-range static_cast. A plain default branch would
            // build a void world without saying anything, which a player experiences as
            // choosing the overworld and falling into the void.
            if (generatorType != GeneratorType::Void)
            {
                hostLogger().error(
                    "[dim] '{}' has generatorType={}({}) with no matching generator branch "
                    "and falls back to the void generator; this is a bug, report this line "
                    "to the developers",
                    mName.get(), magic_enum::enum_name(generatorType), static_cast<int>(generatorType)
                );
            }
            auto generator = std::make_unique<VoidGenerator>(*this);
            generator->mBiome = level.getBiomeRegistry().lookupByName("minecraft:ocean");
            generator->mBiomeSource = std::make_unique<FixedBiomeSource>(*generator->mBiome);
            worldGenerator = std::move(generator);
            worldGenerator->mStructureFeatureRegistry->mGeneratorState =
                br::worldgen::ChunkGeneratorStructureState::createFlat(
                    seed, worldGenerator->getBiomeSource(), {}
                );
        }
        return worldGenerator;
    }

    void SimpleCustomDimension::upgradeLevelChunk(ChunkSource& cs, LevelChunk& lc, LevelChunk& generatedChunk)
    {
        auto blockSource = BlockSource(static_cast<Level&>(mLevel), *this, cs, false, true, false);
        VanillaLevelChunkUpgrade::_upgradeLevelChunkViaMetaData(lc, generatedChunk, blockSource);
        VanillaLevelChunkUpgrade::_upgradeLevelChunkLegacy(lc, blockSource);
    }

    void SimpleCustomDimension::fixWallChunk(ChunkSource& cs, LevelChunk& lc)
    {
        auto blockSource = BlockSource(static_cast<Level&>(mLevel), *this, cs, false, true, false);
        VanillaLevelChunkUpgrade::fixWallChunk(lc, blockSource);
    }

    bool SimpleCustomDimension::levelChunkNeedsUpgrade(LevelChunk const& lc) const
    {
        return VanillaLevelChunkUpgrade::levelChunkNeedsUpgrade(lc);
    }

    void SimpleCustomDimension::_upgradeOldLimboEntity(CompoundTag& tag, ::LimboEntitiesVersion vers)
    {
        auto isTemplate = mLevel.getLevelData().mIsFromLockedTemplate;
        VanillaLevelChunkUpgrade::upgradeOldLimboEntity(tag, vers, isTemplate);
    }

    Vec3 SimpleCustomDimension::translatePosAcrossDimension(Vec3 const& fromPos, DimensionType fromId) const
    {
        Vec3 topos;
        VanillaDimensions::convertPointBetweenDimensions(
            fromPos, topos, fromId, mId, mLevel.getDimensionConversionData()
        );
        constexpr auto clampVal = 32000000.0f - 128.0f;
        topos.x = std::clamp(topos.x, -clampVal, clampVal);
        topos.z = std::clamp(topos.z, -clampVal, clampVal);
        return topos;
    }

    short SimpleCustomDimension::getCloudHeight() const { return 192; }

    std::unique_ptr<ChunkSource>
    SimpleCustomDimension::_wrapStorageForVersionCompatibility(std::unique_ptr<ChunkSource> cs, ::StorageVersion)
    {
        return cs;
    }

    mce::Color
    SimpleCustomDimension::getBrightnessDependentFogColor(mce::Color const& color, float brightness) const
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
