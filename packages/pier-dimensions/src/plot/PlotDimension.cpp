/**
 * PlotDimension.cpp —— 地皮维度本体。
 *
 * 与 SimpleCustomDimension 的 override 集合完全相同，只在两处分岔：
 * `createGenerator` 返回 PlotGenerator，构造时把 PlotLayout 从载荷里读回来。
 */
#include "pier/dimensions/dim/complete_base_types.h"

#include "pier/dimensions/plot/plot_dimension.h"

#include <algorithm>
#include <memory>

#include "mc/common/Brightness.h"
#include "mc/deps/core/math/Color.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/DimensionConversionData.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/chunk/vanilla_level_chunk_upgrade/VanillaLevelChunkUpgrade.h"
#include "mc/world/level/dimension/DimensionArguments.h"
#include "mc/world/level/dimension/OverworldBrightnessRamp.h"
#include "mc/world/level/dimension/VanillaDimensions.h"
#include "mc/world/level/storage/LevelData.h"

#include "pier/dimensions/base/utils.h"
#include "pier/dimensions/dim/custom_dimension_manager.h"
#include "pier/dimensions/plot/plot_generator.h"

namespace pier::dimensions
{
    PlotDimension::PlotDimension(std::string const& name, DimensionFactoryInfo const& info)
    // DimensionArguments 在 26.20 有 5 个成员，最后一个是 mTypeId（维度类型
    // 标识，数据驱动维度用它对应 DimensionDefinitionGroup 里的那条定义）。
    // 之前这里只写了 4 个，mTypeId 被默认成空串 —— 聚合初始化不会报错，但
    // 引擎侧拿到的是一个没有类型的维度。自定义维度的类型标识就用维度名。
        : Dimension(DimensionArguments(
            std::move(info.arguments), info.dimId, {PlotLayout::kMinY, PlotLayout::kMaxY}, name, name
        ))
    {
        mDefaultBrightness->sky = Brightness::MAX();
        mSeaLevel = 63;
        mHasWeather = true;

        mSeed = info.data.contains("seed") ? static_cast<uint>(info.data.at("seed")) : 0u;
        if (info.data.contains("layout"))
        {
            // layout 是一个内嵌的 CompoundTag
            mLayout = PlotLayout::fromNbt(info.data.at("layout").get<CompoundTag>());
        }
        else
        {
            mLayout.clamp();
        }

        mDimensionBrightnessRamp = std::make_unique<OverworldBrightnessRamp>();
        mDimensionBrightnessRamp->buildBrightnessRamp();
    }

    CompoundTag PlotDimension::generateNewData(uint seed, PlotLayout const& layout)
    {
        CompoundTag result;
        result["seed"] = seed;
        result["layout"] = layout.toNbt();
        return result;
    }

    void PlotDimension::init(br::worldgen::StructureSetRegistry const& structureSetRegistry)
    {
        // 保留天光。原版只有 NetherDimension 和 TheEndDimension 重写 init 关掉天
        // 光，OverworldDimension 根本没重写；本包两个自定义维度类都关掉的话，等于
        // 把每一个自定义维度都做成下界。
        //
        // 地皮世界是露天的、光源只有天空，关掉之后整张地图光照全是 0。方块、碰撞、
        // 区块下发都正常，服务端日志看不出异常，玩家看到的却是全黑，容易被当成区块
        // 没加载。判断方法：进去后能站在地面上不往下掉，就说明方块在，是照明问题。
        Dimension::init(structureSetRegistry);

        // 子区块请求全部被回 IndexOutOfBounds，说明服务端判越界用的高度范围和
        // 发给客户端的那一份对不上。引擎判越界走的是
        // `Dimension::isSubChunkHeightWithinRange`，它读的就是 mHeightRange。
        // 所以这里把它实际是什么打出来，并且在不对的时候纠正回来 —— 纠正是
        // 安全的：这两个数就是发给客户端的那一份定义。
        verifyHeightRange(*this, PlotLayout::kMinY, PlotLayout::kMaxY, "PlotDimension");
    }

    std::unique_ptr<WorldGenerator> PlotDimension::createGenerator(br::worldgen::StructureSetRegistry const&)
    {
        auto& level = mLevel;
        auto& levelData = level.getLevelData();
        // 平坦生成器的 options 只用来初始化基类的原型体积，
        // 真正的图案完全由 PlotGenerator::loadChunk 决定。
        return std::make_unique<PlotGenerator>(*this, mSeed, levelData.mFlatWorldOptions, mLayout);
    }

    void PlotDimension::upgradeLevelChunk(ChunkSource& cs, LevelChunk& lc, LevelChunk& generatedChunk)
    {
        auto blockSource = BlockSource(static_cast<Level&>(mLevel), *this, cs, false, true, false);
        VanillaLevelChunkUpgrade::_upgradeLevelChunkViaMetaData(lc, generatedChunk, blockSource);
        VanillaLevelChunkUpgrade::_upgradeLevelChunkLegacy(lc, blockSource);
    }

    void PlotDimension::fixWallChunk(ChunkSource& cs, LevelChunk& lc)
    {
        auto blockSource = BlockSource(static_cast<Level&>(mLevel), *this, cs, false, true, false);
        VanillaLevelChunkUpgrade::fixWallChunk(lc, blockSource);
    }

    bool PlotDimension::levelChunkNeedsUpgrade(LevelChunk const& lc) const
    {
        return VanillaLevelChunkUpgrade::levelChunkNeedsUpgrade(lc);
    }

    void PlotDimension::_upgradeOldLimboEntity(CompoundTag& tag, ::LimboEntitiesVersion vers)
    {
        auto isTemplate = mLevel.getLevelData().mIsFromLockedTemplate;
        VanillaLevelChunkUpgrade::upgradeOldLimboEntity(tag, vers, isTemplate);
    }

    Vec3 PlotDimension::translatePosAcrossDimension(Vec3 const& fromPos, DimensionType fromId) const
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

    short PlotDimension::getCloudHeight() const { return 192; }

    std::unique_ptr<ChunkSource>
    PlotDimension::_wrapStorageForVersionCompatibility(std::unique_ptr<ChunkSource> cs, ::StorageVersion)
    {
        return cs;
    }

    mce::Color PlotDimension::getBrightnessDependentFogColor(mce::Color const& color, float brightness) const
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
