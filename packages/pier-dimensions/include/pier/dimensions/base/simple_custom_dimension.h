#pragma once

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/level/GeneratorType.h"
#include "mc/world/level/dimension/Dimension.h"

#include <memory>
#include <string>

namespace pier::dimensions
{
    struct DimensionFactoryInfo;

    /**
     * The plainest custom dimension: a vanilla generator plus its own seed.
     *
     * The declaration carries no `MORE_DIMENSIONS_API`, meaning `__declspec(dllexport)`.
     * This package is an object package compiled into the host, its capability reaches
     * the ABI table through a SlotPack, and it exports no symbol. An export macro would
     * only make the linker emit an export table nobody uses.
     */
    class SimpleCustomDimension : public Dimension
    {
        uint seed;
        GeneratorType generatorType;

    public:
        SimpleCustomDimension(std::string const& name, DimensionFactoryInfo const& info);

        static CompoundTag
        generateNewData(uint seed = 123, GeneratorType generatorType = GeneratorType::Overworld);

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
