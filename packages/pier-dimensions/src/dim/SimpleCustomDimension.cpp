/**
 * SimpleCustomDimension.cpp —— 一个原版生成器 + 自己的种子。
 *
 * 两处历史修正，都写在原地：
 *   1. `init` 里曾无条件 `mHasSkylight = false` —— 把每一个自定义维度都做成了
 *      下界的照明模型，选超平坦的人得到一张全黑的地图；
 *   2. `createGenerator` 的 default 分支曾一声不吭地建虚空世界 —— 玩家看到的是
 *      「我选了主世界，结果掉进虚空」。
 *
 * 另有三个符号解析改成懒加载，理由在下面 `overworldAddress()` 那一段。
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

        // 这三个符号**只有** Overworld / Nether / TheEnd 生成器用得到。
        //
        // 以前它们是命名空间作用域的变量，DLL 一加载就解析，于是任何一次
        // BDS 签名变更都会在启动时打三条 FATAL —— 哪怕这台服务器只用
        // Flat / Void / Plot 维度，根本不会调用它们。改成函数内 static
        // 懒解析后，只有真正用到时才解析，用不到就完全无感。
        //
        // 同时补上空指针检查：符号缺失时降级成「不生成结构」并打一条 warn，
        // 而不是拿 nullptr 去 addressCall 直接崩服。
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
                    "符号 OverworldDimensionAnon::addStructureFeatures 未找到，"
                    "自定义主世界维度将不生成结构"
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
                    "符号 NetherDimensionAnon::addStructureFeatures 未找到，"
                    "自定义下界维度将不生成结构"
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
                    "符号 StructureFeatureRegistry::addStructureFeature<EndCityFeature> 未找到，"
                    "自定义末地维度将不生成末地城"
                );
                return;
            }
            ll::memory::addressCall<EndCityFeature&, StructureFeatureRegistry*, Dimension&, uint&>(
                addr, self, dimension, seed
            );
        }
    } // namespace

    SimpleCustomDimension::SimpleCustomDimension(std::string const& name, DimensionFactoryInfo const& info)
        : // 第 5 个参数是 mTypeId，见 PlotDimension.cpp 里的说明。
          //
          // 高度范围取共享常量：这一份必须和 CustomDimensionManager 交给
          // DimensionDefinition（也就是 DimensionDataPacket 里发给客户端的那份）
          // 的值完全一致，否则客户端进维度就会因为子区块索引越界而闪退。
          Dimension(
              DimensionArguments(std::move(info.arguments), info.dimId, {kWorldMinY, kWorldMaxY}, name, name)
          )
    {
        mDefaultBrightness->sky = Brightness::MAX();
        // 这里读的是**已经存进配置的那个名字**，不是本次调用传进来的参数 ——
        // generateNewData 只在维度第一次创建时跑一次。所以一个维度建错了生成器，
        // 之后重启多少次都还是错的，改代码不会追溯修正它。
        auto const storedName = static_cast<std::string_view>(info.data["generatorType"]);
        auto generatorTypeOpt = magic_enum::enum_cast<GeneratorType>(storedName);
        if (!generatorTypeOpt)
        {
            hostLogger().error(
                "维度 '{}' 存下来的 generatorType 是 '{}'，不认识 —— 退回 Overworld。"
                "地形会和创建时选的不一样。",
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
        // 以前这里无条件 `mHasSkylight = false`，也就是不管选的是主世界、超平坦
        // 还是虚空，一律按下界的照明模型来。选超平坦的人会得到一张全黑的平坦
        // 地图 —— 方块都在、能站上去，但什么都看不见。
        //
        // 对齐原版的做法：只有下界和末地关天光，其余都开。
        // （OverworldDimension 连 init 都没重写，NetherDimension 和
        //  TheEndDimension 才重写。）
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

        // 见 utils.h 里的说明：服务端判子区块越界用的是 Dimension::mHeightRange，
        // 客户端用的是 DimensionDataPacket 里那份定义，两者是独立的两份数据。
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
            // Void 之外的任何值走到这里都是 bug（Legacy / Undefined / 越界的
            // static_cast）。以前这个分支是纯 default，什么都不说就建了个虚空
            // 世界出来 —— 玩家看到的是「我选了主世界，结果掉进虚空」。
            if (generatorType != GeneratorType::Void)
            {
                hostLogger().error(
                    "维度 '{}' 的 generatorType={}({}) 没有对应的生成器分支，只能退回虚空生成器。"
                    "这是个 bug，请把这一行发给开发者。",
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
