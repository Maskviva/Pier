/**
 * DimensionSpec.cpp: reading and clamping the spec. Only file that touches the SNBT keys.
 *
 * Refusal here is a returned problem list, not a log line: the caller in Slots.cpp logs it
 * once with the dimension name and returns -1. Reading nothing and building a default
 * dimension would be the silent fallback contract §5.1 forbids, and this one persists.
 */
#include "pier/dimensions/spec/dimension_spec.h"

#include <algorithm>
#include <string_view>

#include "magic_enum.hpp"

#include "mc/deps/nbt/ListTag.h"

namespace pier::dimensions::spec
{
    namespace
    {
        int num(CompoundTag const& c, char const* key, int fallback)
        {
            return c.contains(key) ? static_cast<int>(c.at(key)) : fallback;
        }

        float fnum(CompoundTag const& c, char const* key, float fallback)
        {
            return c.contains(key) ? static_cast<float>(c.at(key)) : fallback;
        }

        std::string str(CompoundTag const& c, char const* key, std::string const& fallback)
        {
            if (!c.contains(key)) return fallback;
            auto sv = static_cast<std::string_view>(c.at(key));
            return sv.empty() ? fallback : std::string{sv};
        }

        bool boolean(CompoundTag const& c, char const* key, bool fallback)
        {
            return c.contains(key) ? static_cast<bool>(c.at(key)) : fallback;
        }

        /** [lo, hi] as a ListTag of two numbers, or a single number meaning [x, x]. */
        Range range(CompoundTag const& c, char const* key, Range fallback)
        {
            if (!c.contains(key)) return fallback;
            auto const& tag = c.at(key);
            if (tag.getId() == Tag::Type::List)
            {
                auto const& ls = tag.get<ListTag>();
                if (ls.size() != 2) return fallback;
                return Range{static_cast<float>(ls[0]), static_cast<float>(ls[1])};
            }
            float one = static_cast<float>(tag);
            return Range{one, one};
        }

        std::optional<GeneratorType> generatorNamed(std::string_view name)
        {
            std::string lower{name};
            std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            if (lower == "overworld") return GeneratorType::Overworld;
            if (lower == "nether") return GeneratorType::Nether;
            if (lower == "end" || lower == "theend" || lower == "the_end") return GeneratorType::TheEnd;
            if (lower == "void") return GeneratorType::Void;
            if (lower == "flat") return GeneratorType::Flat;
            return std::nullopt;
        }

    } // namespace

    DimensionHeightRange dimensionHeightOf(CompoundTag const& stored)
    {
        auto [s, problems] = DimensionSpec::fromNbt(stored);
        int minY = s ? s->minY : kWorldMinY;
        int maxY = s ? s->maxY : kWorldMaxY;
        return DimensionHeightRange{static_cast<short>(minY), static_cast<short>(maxY)};
    }

    GeneratorType clientGeneratorOf(CompoundTag const& stored)
    {
        auto [s, problems] = DimensionSpec::fromNbt(stored);
        return s ? s->sky.client : GeneratorType::Flat;
    }

    int Layers::surfaceY() const
    {
        int total = 0;
        for (auto const& l : layers) total += l.thickness;
        return baseY + total - 1;
    }


    std::vector<std::string> DimensionSpec::clamp()
    {
        std::vector<std::string> changed;
        auto clampInt = [&](int& v, int lo, int hi, char const* what)
        {
            if (v < lo || v > hi)
            {
                changed.push_back(std::string{what} + " clamped");
                v = std::clamp(v, lo, hi);
            }
        };
        // Height stays on subchunk boundaries. Rounding outward would silently make the
        // dimension taller than asked; rounding inward keeps every asked-for block.
        if (minY % 16 != 0) { minY += (minY % 16 + 16) % 16; changed.push_back("height.min rounded up to a subchunk boundary"); }
        if (maxY % 16 != 0) { maxY -= (maxY % 16 + 16) % 16; changed.push_back("height.max rounded down to a subchunk boundary"); }
        clampInt(minY, kWorldMinY, kWorldMaxY - 16, "height.min");
        clampInt(maxY, minY + 16, kWorldMaxY, "height.max");

        if (auto* l = std::get_if<Layers>(&terrain))
        {
            if (l->biome.empty()) l->biome = "minecraft:plains";
            for (auto& layer : l->layers)
            {
                if (layer.block.empty()) layer.block = "minecraft:stone";
                if (layer.thickness < 1) layer.thickness = 1;
            }
            if (l->grid)
            {
                clampInt(l->grid->cell, 4, 512, "grid.cell");
                clampInt(l->grid->gap, 0, 64, "grid.gap");
                if (l->grid->edge < 0 || l->grid->edge * 2 >= l->grid->cell) { l->grid->edge = 0; changed.push_back("grid.edge dropped"); }
                if (l->grid->gapBlock.empty()) l->grid->gapBlock = "minecraft:birch_planks";
                if (l->grid->edgeBlock.empty()) l->grid->edgeBlock = "minecraft:stone_block_slab";
                if (l->layers.empty()) { l->grid.reset(); changed.push_back("grid dropped: no surface to draw it on"); }
            }
            // The stack has to fit under the ceiling with one block of headroom for edge blocks.
            if (l->baseY <= kBedrockY) { l->baseY = kBedrockY + 1; changed.push_back("base_y raised above bedrock"); }
            int budget = maxY - 2 - l->baseY;
            int total = 0;
            for (auto it = l->layers.begin(); it != l->layers.end();)
            {
                if (total >= budget) { it = l->layers.erase(it); changed.push_back("a layer above the ceiling dropped"); continue; }
                if (total + it->thickness > budget) { it->thickness = budget - total; changed.push_back("a layer cut at the ceiling"); }
                total += it->thickness;
                ++it;
            }
        }
        if (auto* n = std::get_if<Noise>(&terrain))
        {
            clampInt(n->gradientFromY, minY, maxY - 1, "shape.gradient.from_y");
            clampInt(n->gradientToY, minY, maxY - 1, "shape.gradient.to_y");
            for (auto& o : n->octaves)
            {
                if (o.levels < 1) o.levels = 1;
                if (o.levels > 8) o.levels = 8;
                if (o.scaleXZ <= 0.f) o.scaleXZ = 0.01f;
                if (o.scaleY <= 0.f) o.scaleY = o.scaleXZ;
            }
            if (n->bedrock && (n->bedrock->second < minY || n->bedrock->second >= maxY)) { n->bedrock.reset(); changed.push_back("bedrock outside the height range dropped"); }
            if (n->fluid && (n->fluid->second < minY || n->fluid->second >= maxY)) { n->fluid.reset(); changed.push_back("fluid level outside the height range dropped"); }
        }
        return changed;
    }

    std::pair<std::optional<DimensionSpec>, std::vector<std::string>> DimensionSpec::fromNbt(CompoundTag const& t)
    {
        std::vector<std::string> problems;
        DimensionSpec s;
        s.seed = static_cast<uint>(num(t, "seed", 0));

        if (!t.contains("terrain"))
        {
            if (t.contains("layout") || t.contains("generatorType"))
                problems.push_back("the payload is in the pre-26.20.3 shape; run tools/migrate_dimension_config.py once on worlds/<level>/dimension_config.json, then restart");
            else
                problems.push_back("the payload has no terrain section");
            return {std::nullopt, problems};
        }

        if (t.contains("height"))
        {
            auto const& h = t.at("height").get<CompoundTag>();
            s.minY = num(h, "min", s.minY);
            s.maxY = num(h, "max", s.maxY);
        }
        if (t.contains("sky"))
        {
            auto const& sky = t.at("sky").get<CompoundTag>();
            if (auto g = generatorNamed(str(sky, "client", "overworld"))) s.sky.client = *g;
            else problems.push_back("sky.client must be overworld, nether or end");
            s.sky.skylight = boolean(sky, "skylight", !s.sky.timeless());
            s.sky.weather = boolean(sky, "weather", !s.sky.timeless());
            if (sky.contains("time"))
            {
                auto const t = num(sky, "time", 0);
                if (t < 0 || t > 23999) problems.push_back("sky.time must be 0..23999");
                else s.sky.time = t;
            }
        }

        auto const& terrain = t.at("terrain").get<CompoundTag>();
        auto const kind = str(terrain, "kind", "");
        if (kind == "native")
        {
            auto g = generatorNamed(str(terrain, "generator", ""));
            if (!g) { problems.push_back("terrain.generator must be overworld, nether, end or void"); return {std::nullopt, problems}; }
            if (*g == GeneratorType::Void) s.terrain = Layers{};
            else s.terrain = Native{*g};
        }
        else if (kind == "layers")
        {
            Layers l;
            l.baseY = num(terrain, "base_y", l.baseY);
            l.biome = str(terrain, "biome", l.biome);
            if (terrain.contains("layers"))
            {
                for (auto const& item : terrain.at("layers").get<ListTag>())
                {
                    auto const& one = item->as<CompoundTag>();
                    l.layers.push_back({str(one, "block", "minecraft:stone"), num(one, "thickness", 1)});
                }
            }
            if (terrain.contains("grid"))
            {
                auto const& g = terrain.at("grid").get<CompoundTag>();
                Grid grid;
                grid.cell = num(g, "cell", grid.cell);
                grid.gap = num(g, "gap", grid.gap);
                grid.edge = num(g, "edge", grid.edge);
                grid.gapBlock = str(g, "gap_block", grid.gapBlock);
                grid.edgeBlock = str(g, "edge_block", grid.edgeBlock);
                grid.confine = boolean(g, "confine", false);
                l.grid = grid;
            }
            s.terrain = l;
        }
        else if (kind == "noise")
        {
            Noise n;
            if (terrain.contains("height"))
            {
                auto const& h = terrain.at("height").get<CompoundTag>();
                s.minY = num(h, "min", s.minY);
                s.maxY = num(h, "max", s.maxY);
            }
            if (terrain.contains("biomes"))
            {
                for (auto const& item : terrain.at("biomes").get<ListTag>())
                {
                    auto const& b = item->as<CompoundTag>();
                    BiomeTarget bt;
                    bt.biome = str(b, "biome", "");
                    if (bt.biome.empty()) { problems.push_back("a biomes entry has no biome"); continue; }
                    bt.temperature = range(b, "temperature", bt.temperature);
                    bt.humidity = range(b, "humidity", bt.humidity);
                    bt.continentalness = range(b, "continentalness", bt.continentalness);
                    bt.erosion = range(b, "erosion", bt.erosion);
                    bt.depth = range(b, "depth", bt.depth);
                    bt.weirdness = range(b, "weirdness", bt.weirdness);
                    bt.offset = fnum(b, "offset", 0.f);
                    n.biomes.push_back(bt);
                }
            }
            if (n.biomes.empty()) problems.push_back("terrain.biomes is empty: multi-biome placement needs at least one target");
            if (terrain.contains("shape"))
            {
                auto const& sh = terrain.at("shape").get<CompoundTag>();
                if (sh.contains("gradient"))
                {
                    auto const& g = sh.at("gradient").get<CompoundTag>();
                    n.gradientFromY = num(g, "from_y", n.gradientFromY);
                    n.gradientToY = num(g, "to_y", n.gradientToY);
                }
                n.threshold = fnum(sh, "threshold", 0.f);
                if (sh.contains("octaves"))
                {
                    for (auto const& item : sh.at("octaves").get<ListTag>())
                    {
                        auto const& o = item->as<CompoundTag>();
                        n.octaves.push_back({fnum(o, "scale_xz", 0.01f), fnum(o, "scale_y", 0.02f), fnum(o, "amplitude", 1.f), num(o, "levels", 4)});
                    }
                }
                if (sh.contains("islands"))
                {
                    auto const& is = sh.at("islands").get<CompoundTag>();
                    n.islands = std::make_pair(fnum(is, "scale", 0.003f), fnum(is, "floor", -0.4f));
                }
            }
            if (terrain.contains("palette"))
            {
                for (auto const& item : terrain.at("palette").get<ListTag>())
                {
                    auto const& p = item->as<CompoundTag>();
                    Range d = range(p, "depth", Range{0.f, 0.f});
                    n.palette.push_back({str(p, "block", "minecraft:stone"), static_cast<int>(d.lo), static_cast<int>(d.hi)});
                }
            }
            if (n.palette.empty()) problems.push_back("terrain.palette is empty: solid cells have no block");
            if (terrain.contains("fluid") && terrain.at("fluid").getId() == Tag::Type::Compound)
            {
                auto const& f = terrain.at("fluid").get<CompoundTag>();
                n.fluid = std::make_pair(str(f, "block", "minecraft:water"), num(f, "level", 63));
            }
            if (terrain.contains("bedrock") && terrain.at("bedrock").getId() == Tag::Type::Compound)
            {
                auto const& b = terrain.at("bedrock").get<CompoundTag>();
                n.bedrock = std::make_pair(str(b, "block", "minecraft:bedrock"), num(b, "y", s.minY));
            }
            s.terrain = n;
        }
        else
        {
            problems.push_back("terrain.kind must be native, layers or noise");
            return {std::nullopt, problems};
        }

        bool fatal = std::any_of(problems.begin(), problems.end(), [](auto const& p) { return p.find("empty") != std::string::npos; });
        if (fatal) return {std::nullopt, problems};
        for (auto const& c : s.clamp()) problems.push_back(c);
        return {s, problems};
    }

    std::pair<std::optional<DimensionSpec>, std::vector<std::string>> DimensionSpec::fromSnbt(std::string const& snbt)
    {
        if (snbt.empty()) return {std::nullopt, {"the spec is empty"}};
        auto tag = CompoundTag::fromSnbt(snbt);
        if (!tag) return {std::nullopt, {"the spec is not valid SNBT"}};
        return fromNbt(*tag);
    }

} // namespace pier::dimensions::spec
