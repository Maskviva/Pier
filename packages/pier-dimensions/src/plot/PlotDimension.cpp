/**
 * PlotDimension.cpp: the plot dimension itself.
 *
 * The override set is identical to SimpleCustomDimension and differs in two places:
 * `createGenerator` returns a PlotGenerator, and the constructor reads the PlotLayout
 * back out of the payload.
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
    // DimensionArguments has 5 members on 26.20 and the last is mTypeId, the dimension
    // type identifier a data-driven dimension uses to match its entry in
    // DimensionDefinitionGroup. Writing only 4 leaves mTypeId defaulted to an empty
    // string, which aggregate initialization accepts without complaint while the engine
    // receives a dimension with no type. The type identifier of a custom dimension is
    // its name.
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
            // layout is a nested CompoundTag
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
        // Skylight is kept. In vanilla only NetherDimension and TheEndDimension override
        // init to turn it off, OverworldDimension does not override it at all, so turning
        // it off in both custom dimension classes would make every custom dimension a
        // nether. A plot world is open-air and the sky is its only light source, so with
        // skylight off the whole map has light level 0: blocks, collision and chunk
        // delivery behave normally, the server log shows nothing, and the player sees
        // black and reads it as chunks failing to load. Standing on the ground without
        // falling means the blocks are there and the problem is lighting.
        Dimension::init(structureSetRegistry);

        // Every subchunk request coming back IndexOutOfBounds means the height range the
        // server judges against disagrees with the copy sent to the client. The engine
        // judges through `Dimension::isSubChunkHeightWithinRange`, which reads
        // mHeightRange, so this prints what it actually holds and corrects it when it is
        // wrong. The correction is safe, because those two numbers are the definition
        // sent to the client.
        verifyHeightRange(*this, PlotLayout::kMinY, PlotLayout::kMaxY, "PlotDimension");
    }

    std::unique_ptr<WorldGenerator> PlotDimension::createGenerator(br::worldgen::StructureSetRegistry const&)
    {
        auto& level = mLevel;
        auto& levelData = level.getLevelData();
        // The flat generator options only initialize the prototype volume of the base
        // class. The actual pattern is decided entirely by PlotGenerator::loadChunk.
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
