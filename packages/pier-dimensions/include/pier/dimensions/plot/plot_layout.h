#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "pier/dimensions/dim/dimension_height.h"

#include "mc/deps/nbt/CompoundTag.h"

namespace pier::dimensions
{
    /**
     * The geometric layout of a plot world.
     * The grid convention, which the C++ generator and the mod side must share exactly, with cell =
     * plotSize + roadWidth, ix = mod(worldX, cell), iz = mod(worldZ, cell):
     *   ix >= plotSize || iz >= plotSize    -> road
     *   otherwise, distance to the plot edge below borderWidth -> border, part of the plot
     *   otherwise                           -> plot interior
     * A plot occupies the low range [0, plotSize) of a cell and a road the high range [plotSize,
     * cell). Both PlotGenerator::loadChunk and plot_at and is_border on the mod side use this
     * convention, so changing one means changing the other.
     * Vertically: bedrock at minY; fill blocks over (minY, floorY); the surface or road block at
     * floorY; the border block at floorY + 1, only on border cells, forming a curb; air everywhere
     * else. / */
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
         * The vertical range of the world. No literal belongs here: it must come from
         * the same source as the copy `DimensionDefinition` sends to the client. See
         * dimension_height.h.
         */
        static constexpr int kMinY = kWorldMinY;
        static constexpr int kMaxY = kWorldMaxY;
        static constexpr int kBedrockY = pier::dimensions::kBedrockY;
        static constexpr int kTotalHeight = kMaxY - kMinY; // 832, from a bottom of -512
        static constexpr int kChunkWidth = 16;

        [[nodiscard]] int cellSize() const { return plotSize + roadWidth; }

        /**
         * Clamps into the safe range. A value arriving across the ABI is never trusted:
         * an out-of-range floorY makes a buffer index go out of bounds and crashes the
         * server.
         */
        void clamp()
        {
            if (plotSize < 4) plotSize = 4;
            if (plotSize > 512) plotSize = 512;
            if (roadWidth < 0) roadWidth = 0;
            if (roadWidth > 64) roadWidth = 64;
            if (borderWidth < 0) borderWidth = 0;
            if (borderWidth * 2 >= plotSize) borderWidth = 0;
            // floorY + 1 has to hold the border, so one cell of headroom is kept
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

        /**
         * Parses a layout SNBT. An empty string means the default layout, while a
         * non-empty string that fails to parse returns nullopt.
         *
         * A silent fallback to the default layout would be wrong here, because the
         * layout is written into dimension_config.json with the dimension and persists.
         * One typo would fix the plot size of a world at the default with no log line,
         * and it could not be corrected afterwards, since a change would no longer match
         * the terrain already generated in the save. Failing now is better.
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

    /** The area type of one axis within a cell. */
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

    /** Classifies an offset within a cell. Isomorphic to plot_at and is_border on the
     *  mod side. */
    [[nodiscard]] inline PlotArea1D classify1D(int offset, PlotLayout const& l)
    {
        if (offset >= l.plotSize) return PlotArea1D::Road;
        if (l.borderWidth > 0 && (offset < l.borderWidth || offset >= l.plotSize - l.borderWidth))
            return PlotArea1D::Border;
        return PlotArea1D::Plot;
    }

    /** Combining two axes: either axis being road makes it road, both being interior
     *  makes it interior, and everything else is border. */
    [[nodiscard]] inline PlotArea1D combine2D(PlotArea1D x, PlotArea1D z)
    {
        if (x == PlotArea1D::Road || z == PlotArea1D::Road) return PlotArea1D::Road;
        if (x == PlotArea1D::Plot && z == PlotArea1D::Plot) return PlotArea1D::Plot;
        return PlotArea1D::Border;
    }
} // namespace pier::dimensions
