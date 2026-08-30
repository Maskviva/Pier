#pragma once

#include <optional>
#include <vector>

#include "mc/world/level/biome/source/FixedBiomeSource.h"
#include "mc/world/level/levelgen/flat/FlatWorldGenerator.h"

#include "pier/dimensions/plot/plot_layout.h"

class Block;
class Dimension;
class LevelChunk;

namespace Json
{
    class Value;
}

namespace pier::dimensions
{
    /**
     * 地皮世界的区块生成器。
     *
     * 继承 `FlatWorldGenerator` 而不是从零写一个 `WorldGenerator`：平坦生成器
     * 已经把 BlockVolume 原型、生物群系源、结构查询等全套接好了，我们只需要
     * 接管 `loadChunk` 往缓冲区里填自己的图案。
     *
     * 性能：每个区块只做 2 次层填充（256×2 次指针写）+ 256 次分类，然后一次
     * `setBlockVolume`。和原版平坦世界基本同量级 —— 这就是为什么必须放在
     * C++ 侧做，而不是让模组事后逐格 `set_block` 铺出来。
     */
    class PlotGenerator final : public FlatWorldGenerator
    {
        PlotLayout mLayout;
        Block const* mAirBlock{nullptr};
        Block const* mBedrockBlock{nullptr};
        Block const* mFloorBlock{nullptr};
        Block const* mFillBlock{nullptr};
        Block const* mRoadBlock{nullptr};
        Block const* mBorderBlock{nullptr};

        /** 缓冲区内的 y 索引（已归一化到 0..kTotalHeight）。 */
        int mFloorIndexY{0};
        int mBorderIndexY{0};

    public:
        PlotGenerator(Dimension& dimension, uint seed, Json::Value const& options, PlotLayout const& layout);

        void loadChunk(LevelChunk& lc, bool forceImmediateReplacementDataLoad) override;

        [[nodiscard]] PlotLayout const& layout() const { return mLayout; }

    private:
        /**
         * 每个线程一份的方块缓冲区 + BlockVolume 视图。
         *
         * 注意：上游那份实现用的是 `static thread_local ThreadData`，只在
         * **第一次**调用时按当时那个 generator 初始化。它只有一个地皮维度，
         * 所以没事；本宿主允许存在**多个**地皮维度，那样写会让第二个维度用上
         * 第一个维度的方块。这里用 `owner` 指针做校验，换了 generator 就整体
         * 重填。
         */
        struct ThreadBuffer
        {
            std::vector<Block const*> blocks;
            std::optional<BlockVolume> volume;
            void const* owner{nullptr};
        };

        ThreadBuffer& acquireBuffer();
        void refillStatic(ThreadBuffer& buf);
    };
} // namespace pier::dimensions
