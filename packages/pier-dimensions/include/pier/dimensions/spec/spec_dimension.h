/** pier/dimensions/spec/spec_dimension.h: the one custom Dimension, driven by a DimensionSpec.
 * One class serves every terrain kind: the spec decides createGenerator and what is stored,
 * and the retired ABI names route here as well. Height range and
 * sky come from the spec, so a dimension can be 256 tall with an end sky over layered terrain.
 * The height in the DimensionArguments here and the one CustomDimensionManager hands to
 * DimensionDefinition must be the same two numbers, both read from this spec: when they
 * disagree the client requests subchunks outside its buffer and crashes on entering. */
#pragma once

#include <memory>
#include <string>

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/dimension/OverworldDimension.h"

#include "pier/dimensions/spec/dimension_spec.h"

namespace pier::dimensions
{
    struct DimensionFactoryInfo;

    class SpecDimension final : public Dimension
    {
        spec::DimensionSpec mSpec;

        /** This object seen as the overworld, for the four chunk-upgrade overrides only.
         *  The bodies they used to call are inlined away and the overworld's are not;
         *  OverworldDimension adds no data member over Dimension, which is the only
         *  reason the view is safe. SpecDimension.cpp carries the full reasoning. */
        OverworldDimension& asOverworldForUpgrade();

    public:
        SpecDimension(std::string const& name, DimensionFactoryInfo const& info);

        /** The payload stored at first registration. The tag parsed from the caller's SNBT,
         *  not the caller's bytes: CustomDimensionManager serializes it back out with
         *  SnbtFormat::Minimize before writing dimension_config.json, so a byte comparison
         *  against the caller's own record does not hold and only structural equality does. */
        static CompoundTag generateNewData(std::string const& verbatimSnbt);

        [[nodiscard]] spec::DimensionSpec const& spec() const { return mSpec; }

        void init(br::worldgen::StructureSetRegistry const&) override;
        std::unique_ptr<WorldGenerator> createGenerator(br::worldgen::StructureSetRegistry const&) override;
        void upgradeLevelChunk(ChunkSource& chunkSource, LevelChunk& oldLc, LevelChunk& newLc) override;
        void fixWallChunk(ChunkSource& cs, LevelChunk& lc) override;
        bool levelChunkNeedsUpgrade(LevelChunk const& lc) const override;
        void _upgradeOldLimboEntity(CompoundTag& tag, ::LimboEntitiesVersion vers) override;
        Vec3 translatePosAcrossDimension(Vec3 const& pos, DimensionType did) const override;
        float getTimeOfDay(int time, float a) const override;
        std::unique_ptr<ChunkSource>
        _wrapStorageForVersionCompatibility(std::unique_ptr<ChunkSource> cs, ::StorageVersion ver) override;
        mce::Color getBrightnessDependentFogColor(mce::Color const& color, float brightness) const override;
        short getCloudHeight() const override;
    };
} // namespace pier::dimensions
