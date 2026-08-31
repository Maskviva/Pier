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
     * 一个使用 PlotGenerator 的自定义维度。
     *
     * 结构与 SimpleCustomDimension 一致（同样的 override 集合），区别只在
     * createGenerator 返回 PlotGenerator，以及把 PlotLayout 一起存进
     * dimension_config.json：布局是维度的固有属性而不是可热改的配置，存下来之后重启
     * 布局不变，玩家已建好的地皮不会因为管理员改了配置而错位。模组侧配置里的 layout
     * 只用于新建维度和几何计算，真正决定地形长什么样的是这里存下来的这一份。
     *
     * 本包在新架构里是编进宿主的 object 包，能力经 SlotPack 装进 ABI 表，不导出任何
     * 符号，所以没有导出宏。
     */
    class PlotDimension final : public Dimension
    {
        uint mSeed{0};
        PlotLayout mLayout{};

    public:
        PlotDimension(std::string const& name, DimensionFactoryInfo const& info);

        /** 首次注册时调用，产出存进 `dimension_config.json` 的那份数据。 */
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
