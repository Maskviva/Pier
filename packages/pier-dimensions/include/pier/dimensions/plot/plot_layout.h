#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "pier/dimensions/dim/dimension_height.h"

#include "mc/deps/nbt/CompoundTag.h"

namespace pier::dimensions
{
    /**
     * 地皮世界的几何布局。
     *
     * ── 网格约定（C++ 生成器与模组侧必须完全一致）──────────────────────
     *
     *   cell = plotSize + roadWidth
     *   ix   = mod(worldX, cell),  iz = mod(worldZ, cell)
     *
     *   ix >= plotSize || iz >= plotSize          → 道路
     *   否则，距地皮边缘 < borderWidth             → 边框（地皮的一部分）
     *   否则                                       → 地皮内部
     *
     * 也就是说：**地皮占据每个单元格的低位区间 [0, plotSize)，道路占据高位
     * 区间 [plotSize, cell)；边框算在 plotSize 之内。**
     *
     * 这条约定同时被以下两处使用，改一处必须改另一处：
     *   - 本文件的 PlotGenerator::loadChunk（决定方块长什么样）
     *   - 模组侧的 plot_at / is_border（决定谁能建）
     *
     * 写「模组侧」而不是某一门语言：地皮系统是通过 md_set_plot_grid 这组槽位
     * 接进来的，实现它的模组用什么语言写与这条约定无关。
     *
     * 竖直方向：
     *   minY          基岩
     *   (minY, floorY) 填充方块
     *   floorY         地表 / 道路方块
     *   floorY + 1     边框方块（只在边框格上；相当于一圈路缘）
     *   其余           空气
     */
    struct PlotLayout
    {
        int plotSize = 64;
        int roadWidth = 7;
        int borderWidth = 1;
        int floorY = 64;

        std::string floorBlock = "minecraft:grass_block";
        std::string fillBlock = "minecraft:dirt";
        std::string roadBlock = "minecraft:birch_planks";
        std::string borderBlock = "minecraft:stone_block_slab";
        std::string biome = "minecraft:plains";

        /**
         * 世界竖直范围。**不要在这里写字面量** —— 它必须和
         * `DimensionDefinition` 发给客户端的那一份是同一个来源，
         * 见 DimensionHeight.h。
         */
        static constexpr int kMinY = kWorldMinY;
        static constexpr int kMaxY = kWorldMaxY;
        static constexpr int kBedrockY = pier::dimensions::kBedrockY;
        static constexpr int kTotalHeight = kMaxY - kMinY; // 832（底部 -512 起）
        static constexpr int kChunkWidth = 16;

        [[nodiscard]] int cellSize() const { return plotSize + roadWidth; }

        /**
         * 夹到安全范围。**永远不要相信跨 ABI 传进来的数值** —— 一个越界的
         * floorY 会让缓冲区索引越界，直接崩服。
         */
        void clamp()
        {
            if (plotSize < 4) plotSize = 4;
            if (plotSize > 512) plotSize = 512;
            if (roadWidth < 0) roadWidth = 0;
            if (roadWidth > 64) roadWidth = 64;
            if (borderWidth < 0) borderWidth = 0;
            if (borderWidth * 2 >= plotSize) borderWidth = 0;
            // floorY + 1 要能放下边框，所以上界留一格
            if (floorY <= kMinY) floorY = kMinY + 1;
            if (floorY >= kMaxY - 1) floorY = kMaxY - 2;
            if (floorBlock.empty()) floorBlock = "minecraft:grass_block";
            if (fillBlock.empty()) fillBlock = "minecraft:dirt";
            if (roadBlock.empty()) roadBlock = "minecraft:birch_planks";
            if (borderBlock.empty()) borderBlock = "minecraft:stone_block_slab";
            if (biome.empty()) biome = "minecraft:plains";
        }

        [[nodiscard]] CompoundTag toNbt() const
        {
            CompoundTag t;
            t["plotSize"] = plotSize;
            t["roadWidth"] = roadWidth;
            t["borderWidth"] = borderWidth;
            t["floorY"] = floorY;
            t["floorBlock"] = floorBlock;
            t["fillBlock"] = fillBlock;
            t["roadBlock"] = roadBlock;
            t["borderBlock"] = borderBlock;
            t["biome"] = biome;
            return t;
        }

        [[nodiscard]] static PlotLayout fromNbt(CompoundTag const& t)
        {
            PlotLayout l;
            auto num = [&](char const* key, int fallback) -> int
            {
                return t.contains(key) ? static_cast<int>(t.at(key)) : fallback;
            };
            auto str = [&](char const* key, std::string const& fallback) -> std::string
            {
                if (!t.contains(key)) return fallback;
                auto sv = static_cast<std::string_view>(t.at(key));
                return sv.empty() ? fallback : std::string{sv};
            };
            l.plotSize = num("plotSize", l.plotSize);
            l.roadWidth = num("roadWidth", l.roadWidth);
            l.borderWidth = num("borderWidth", l.borderWidth);
            l.floorY = num("floorY", l.floorY);
            l.floorBlock = str("floorBlock", l.floorBlock);
            l.fillBlock = str("fillBlock", l.fillBlock);
            l.roadBlock = str("roadBlock", l.roadBlock);
            l.borderBlock = str("borderBlock", l.borderBlock);
            l.biome = str("biome", l.biome);
            l.clamp();
            return l;
        }

        /** 从 SNBT 解析；解析失败返回全默认值（已 clamp）。 */
        /**
         * 解析布局 SNBT。空串 = 用默认布局；**非空但解析失败返回 nullopt**。
         *
         * V-40：以前解析失败静默退回默认布局，而布局会随维度写进
         * dimension_config.json 永久保存 —— 一处拼写错误就把一个世界的地皮
         * 尺寸定死在默认值上，没有任何日志，事后也改不回来（改了就和存档里
         * 已经生成的地形对不上）。宁可现在失败。
         */
        [[nodiscard]] static std::optional<PlotLayout> fromSnbt(std::string const& snbt)
        {
            if (snbt.empty())
            {
                PlotLayout l;
                l.clamp();
                return l;
            }
            auto tag = CompoundTag::fromSnbt(snbt);
            if (!tag) return std::nullopt;
            return fromNbt(*tag);
        }
    };

    /** 单元格内某一维的区域类型。 */
    enum class PlotArea1D
    {
        Plot,
        Border,
        Road
    };

    [[nodiscard]] inline int positiveMod(int value, int modulus)
    {
        int r = value % modulus;
        return r < 0 ? r + modulus : r;
    }

    /** 把单元格内偏移分类。与模组侧的 plot_at / is_border 同构。 */
    [[nodiscard]] inline PlotArea1D classify1D(int offset, PlotLayout const& l)
    {
        if (offset >= l.plotSize) return PlotArea1D::Road;
        if (l.borderWidth > 0 && (offset < l.borderWidth || offset >= l.plotSize - l.borderWidth))
            return PlotArea1D::Border;
        return PlotArea1D::Plot;
    }

    /** 二维合并：任一维是路即为路；两维都是内部才是内部；其余是边框。 */
    [[nodiscard]] inline PlotArea1D combine2D(PlotArea1D x, PlotArea1D z)
    {
        if (x == PlotArea1D::Road || z == PlotArea1D::Road) return PlotArea1D::Road;
        if (x == PlotArea1D::Plot && z == PlotArea1D::Plot) return PlotArea1D::Plot;
        return PlotArea1D::Border;
    }
} // namespace pier::dimensions
