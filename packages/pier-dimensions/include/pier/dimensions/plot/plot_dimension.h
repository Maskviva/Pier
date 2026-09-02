#pragma once

#include <memory>
#include <string>

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/level/dimension/Dimension.h"

#include "pier/dimensions/plot/plot_layout.h"

namespace pier::dimensions
{
    struct DimensionFactoryInfo;

    /**
     * A custom dimension that uses PlotGenerator.
     *
     * Structurally identical to SimpleCustomDimension, with the same override set. It differs in
     * two places: createGenerator returns a PlotGenerator, and the PlotLayout is stored into
     * dimension_config.json alongside the rest. The layout is an intrinsic property of the
     * dimension and not hot-editable config, so once stored it survives a restart and plots players
     * already built cannot shift because an administrator edited a config. The layout in the mod-
     * side config is used only to create a new dimension and for geometry, while what the terrain
     * actually looks like is decided by the stored copy.
     *
     * This package is an object package compiled into the host, its capability reaches the ABI
     * table through a SlotPack, and it exports no symbol, so there is no export macro. / */
    class PlotDimension final : public Dimension
    {
        uint mSeed{0};
        PlotLayout mLayout{};

    public:
        PlotDimension(std::string const& name, DimensionFactoryInfo const& info);

        /** Called at first registration. Produces the data stored into
         *  `dimension_config.json`. */
        static CompoundTag generateNewData(uint seed, PlotLayout const& layout);

        [[nodiscard]] PlotLayout const& layout() const { return mLayout; }

        void init(br::worldgen::StructureSetRegistry const&) override;
        std::unique_ptr<WorldGenerator> createGenerator(br::worldgen::StructureSetRegistry const&) override;
        void upgradeLevelChunk(ChunkSource& chunkSource, LevelChunk& oldLc, LevelChunk& newLc) override;
        void fixWallChunk(ChunkSource& cs, LevelChunk& lc) override;
        bool levelChunkNeedsUpgrade(LevelChunk const& lc) const override;
        void _upgradeOldLimboEntity(CompoundTag& tag, ::LimboEntitiesVersion vers) override;
        Vec3 translatePosAcrossDimension(Vec3 const& pos, DimensionType did) const override;
        std::unique_ptr<ChunkSource>
        _wrapStorageForVersionCompatibility(std::unique_ptr<ChunkSource> cs, ::StorageVersion ver) override;
        mce::Color getBrightnessDependentFogColor(mce::Color const& color, float brightness) const override;
        short getCloudHeight() const override;
    };
} // namespace pier::dimensions
