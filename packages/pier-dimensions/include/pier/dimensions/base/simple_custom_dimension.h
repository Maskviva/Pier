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
     * 最朴素的自定义维度：一个原版生成器 + 自己的种子。
     *
     * 声明上不再带 `MORE_DIMENSIONS_API`（`__declspec(dllexport)`）—— 本包在
     * 新架构里是编进宿主的 object 包，能力经 SlotPack 装进 ABI 表，不导出任何符号。导出宏留
     * 着只会让链接器多生成一份没人用的导出表。
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
