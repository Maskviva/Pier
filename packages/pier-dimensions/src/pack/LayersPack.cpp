/**
 * LayersPack.cpp: the mapping in pier-pack's from_layers, performed in memory.
 *
 * Zones and spans are the only part that is not a transcription. A stack with no grid is
 * one zone whose span covers the single-column period; a grid is three zones, interior,
 * edge and gap, laid out along both axes as edge / interior / edge / gap, and combined so
 * that gap beats edge beats interior. That combination is what makes the road continuous
 * across a corner: a column that is gap on either axis is gap.
 */
#include "pier/dimensions/pack/layers_pack.h"

#include <algorithm>
#include <cstdint>

#include "pier/dimensions/dim/dimension_height.h"

namespace pier::dimensions::pack
{
    namespace
    {
        /** Every expression the inline spec produces is a constant, so the table is just
         *  a list of numbers and an index into it is the expression id. */
        struct Constants
        {
            std::vector<ExprNode> nodes;

            std::uint32_t of(std::int64_t v)
            {
                auto const imm = static_cast<std::int32_t>(v);
                for (std::uint32_t i = 0; i < nodes.size(); ++i)
                {
                    if (nodes[i].imm == imm) return i;
                }
                nodes.push_back(ExprNode{static_cast<std::uint16_t>(ExprOp::Const), 0, 0, 0, imm});
                return static_cast<std::uint32_t>(nodes.size() - 1);
            }
        };

        /** Zone ids. The order is the one the combine table below is written against. */
        enum Zone : std::uint32_t
        {
            ZInterior = 0,
            ZEdge = 1,
            ZGap = 2,
        };

        /** A role index for a block name, adding it the first time it is seen. Two
         *  layers of the same block share one role, which is what the tool does. Air is
         *  kNone and not a role, so index 0 is an ordinary entry. */
        std::uint32_t roleOf(TemplatePack& p, std::string const& block)
        {
            for (std::uint32_t i = 0; i < p.roles.size(); ++i)
            {
                if (p.roles[i].defaultBlock == block) return i;
            }
            p.roles.push_back(TplRole{"layer_" + std::to_string(p.roles.size()), block});
            return static_cast<std::uint32_t>(p.roles.size() - 1);
        }
    } // namespace

    std::shared_ptr<TemplatePack const> buildFromLayers(spec::Layers const& spec, int minY, int maxY,
                                                        std::vector<std::string>& problems)
    {
        auto const before = problems.size();

        if (spec.baseY <= kBedrockY)
        {
            problems.push_back("terrain.base_y is " + std::to_string(spec.baseY) + ", at or below the bedrock at "
                               + std::to_string(kBedrockY) + "; the stack has nowhere to stand");
        }
        std::int64_t top = spec.baseY;
        for (std::size_t i = 0; i < spec.layers.size(); ++i)
        {
            auto const& l = spec.layers[i];
            if (l.block.empty())
            {
                problems.push_back("terrain.layers[" + std::to_string(i) + "] has no block");
            }
            if (l.thickness < 1)
            {
                problems.push_back("terrain.layers[" + std::to_string(i) + "].thickness is "
                                   + std::to_string(l.thickness) + ", which places nothing");
            }
            top += std::max(1, l.thickness);
        }
        if (top > maxY)
        {
            problems.push_back("the layer stack reaches y " + std::to_string(top) + ", past the top of this dimension at "
                               + std::to_string(maxY));
        }
        if (spec.grid)
        {
            auto const& g = *spec.grid;
            if (g.cell < 4) problems.push_back("terrain.grid.cell is " + std::to_string(g.cell) + "; a cell smaller than 4 is not a plot");
            if (g.gap < 0) problems.push_back("terrain.grid.gap is negative");
            if (g.edge < 0) problems.push_back("terrain.grid.edge is negative");
            if (g.edge * 2 >= g.cell)
            {
                problems.push_back("terrain.grid.edge is " + std::to_string(g.edge) + " on a cell of "
                                   + std::to_string(g.cell) + "; the border has to fit inside the cell twice over");
            }
        }
        if (problems.size() != before) return nullptr;

        auto pack = std::make_shared<TemplatePack>();
        TemplatePack& p = *pack;
        p.sourceName = "layers";
        p.biome = spec.biome;
        p.heightMin = minY;
        p.heightMax = maxY;
        p.heightFixed = false;

        Constants k;

        // The stack, from the bottom of the world up. Bedrock is a floor and not a
        // layer: the recipe never mentions it, and a world whose lowest block can be
        // mined through is one people fall out of.
        auto const bedrock = static_cast<std::uint32_t>(p.roles.size());
        p.roles.push_back(TplRole{"bedrock", "minecraft:bedrock"});

        auto const everyZone = kNone;
        p.stacks.push_back(Stack{everyZone, k.of(kBedrockY), k.of(kBedrockY + 1), bedrock});

        std::int64_t y = spec.baseY;
        for (auto const& l : spec.layers)
        {
            auto const thickness = static_cast<std::int64_t>(std::max(1, l.thickness));
            p.stacks.push_back(Stack{everyZone, k.of(y), k.of(y + thickness), roleOf(p, l.block)});
            y += thickness;
        }
        // The surface is the last block placed, so the road and the border sit on it.
        auto const surface = spec.layers.empty() ? spec.baseY - 1 : static_cast<int>(y - 1);

        if (!spec.grid)
        {
            p.zoneNames = {"all"};
            p.periodXExpr = k.of(1);
            p.periodZExpr = k.of(1);
            p.spansX = {Span{0, k.of(1)}};
            p.spansZ = {Span{0, k.of(1)}};
            p.combine = {0};
        }
        else
        {
            auto const& g = *spec.grid;
            p.zoneNames = {"interior", "edge", "gap"};
            auto const period = static_cast<std::int64_t>(g.cell) + g.gap;
            p.periodXExpr = k.of(period);
            p.periodZExpr = k.of(period);

            // edge / interior / edge / gap along each axis. A zero-width piece is left
            // out rather than emitted empty: a zero-length span is a layout the mount
            // has no meaning for, and edge 0 and gap 0 are both things a recipe asks for.
            std::vector<Span> axis;
            if (g.edge > 0) axis.push_back(Span{ZEdge, k.of(g.edge)});
            axis.push_back(Span{ZInterior, k.of(g.cell - 2 * g.edge)});
            if (g.edge > 0) axis.push_back(Span{ZEdge, k.of(g.edge)});
            if (g.gap > 0) axis.push_back(Span{ZGap, k.of(g.gap)});
            p.spansX = axis;
            p.spansZ = axis;

            // gap beats edge beats interior, indexed zoneX * zoneCount + zoneZ. This is
            // what carries the road through a crossing instead of leaving a plot corner
            // in the middle of it.
            auto const beats = [](std::uint32_t a, std::uint32_t b) -> std::uint32_t
            { return std::max(a, b); };
            p.combine.resize(9);
            for (std::uint32_t zx = 0; zx < 3; ++zx)
            {
                for (std::uint32_t zz = 0; zz < 3; ++zz) p.combine[zx * 3 + zz] = beats(zx, zz);
            }

            auto const gapRole = static_cast<std::uint32_t>(p.roles.size());
            p.roles.push_back(TplRole{"gap", g.gapBlock});
            auto const edgeRole = static_cast<std::uint32_t>(p.roles.size());
            p.roles.push_back(TplRole{"edge", g.edgeBlock});

            // The road replaces the surface block; the border sits one above it, so a
            // plot reads as a bordered square from inside it.
            p.stacks.push_back(Stack{ZGap, k.of(surface), k.of(surface + 1), gapRole});
            if (g.edge > 0)
            {
                p.stacks.push_back(Stack{ZEdge, k.of(surface + 1), k.of(surface + 2), edgeRole});
            }

            if (g.confine)
            {
                p.confine = Confine{k.of(g.cell), k.of(g.gap)};
            }
        }

        p.expr = std::move(k.nodes);
        return pack;
    }
} // namespace pier::dimensions::pack
