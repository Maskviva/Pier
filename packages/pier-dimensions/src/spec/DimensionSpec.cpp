/**
 * DimensionSpec.cpp: reading and clamping the spec. Only file that touches the SNBT keys.
 *
 * Refusal here is a returned problem list, not a log line: the caller in Slots.cpp logs it
 * once with the dimension name and returns -1. Reading nothing and building a default
 * dimension would be the silent fallback contract §5.1 forbids, and this one persists.
 */
#include "pier/dimensions/spec/dimension_spec.h"

#include <algorithm>
#include <optional>
#include <string_view>
#include <utility>

#include "magic_enum.hpp"

#include "mc/deps/nbt/CompoundTagVariant.h"

#include "pier/support/i18n.h"
#include "pier/support/log.h"

namespace pier::dimensions::spec
{
    using ::pier::hostLogger;

    namespace
    {
        /* A field of the wrong type reads as absent, and the caller is told which one.
         *
         * The conversions on CompoundTagVariant are not uniform: a number reached through
         * the wrong type throws std::runtime_error, while a string does it through
         * std::get and throws std::bad_variant_access. Either one leaves the registration
         * as a refusal naming neither the field nor the spec, because the exception
         * escapes the reader and the API guard catches it far from here. One mistyped
         * field in a recipe is a thing to report, not a thing to fail on. */
        int num(CompoundTag const& c, char const* key, int fallback, std::vector<std::string>* problems = nullptr)
        {
            if (!c.contains(key)) return fallback;
            if (!c.at(key).is_number())
            {
                if (problems) problems->push_back(std::string{key} + " is not a number; using " + std::to_string(fallback));
                return fallback;
            }
            return static_cast<int>(c.at(key));
        }

        std::string str(CompoundTag const& c, char const* key, std::string const& fallback,
                        std::vector<std::string>* problems = nullptr)
        {
            if (!c.contains(key)) return fallback;
            if (!c.at(key).is_string())
            {
                if (problems) problems->push_back(std::string{key} + " is not a string; using '" + fallback + "'");
                return fallback;
            }
            auto sv = static_cast<std::string_view>(c.at(key));
            return sv.empty() ? fallback : std::string{sv};
        }

        bool boolean(CompoundTag const& c, char const* key, bool fallback, std::vector<std::string>* problems = nullptr)
        {
            if (!c.contains(key)) return fallback;
            if (!c.at(key).is_number())
            {
                if (problems) problems->push_back(std::string{key} + " is not a true/false value; using " + (fallback ? "true" : "false"));
                return fallback;
            }
            return static_cast<bool>(c.at(key));
        }

        /** A compound field, or null with a line. `.get<CompoundTag>()` is a std::get and
         *  throws for anything else, which is the same unattributable refusal. */
        CompoundTag const* compound(CompoundTag const& c, char const* key, std::vector<std::string>* problems)
        {
            if (!c.contains(key)) return nullptr;
            if (!c.at(key).is_object())
            {
                if (problems) problems->push_back(std::string{key} + " is not a section; ignored");
                return nullptr;
            }
            return &c.at(key).get<CompoundTag>();
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

        bool hexDigits(std::string const& s)
        {
            if (s.size() != 64) return false;
            return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isxdigit(c) != 0; });
        }
    } // namespace

    namespace
    {
    /**
     * The height an engine generator is written against, when the terrain is one of them.
     *
     * These generators are not parameterised by the dimension's height: TheEndGenerator
     * carries a fixed 16x16x128 block buffer and each of them indexes columns from the
     * bottom of the dimension it was written for. Handing one a taller or lower dimension
     * has it write past what it allocated, and a server whose end dimension ran from -64
     * died on the tick after a player entered, with the sky already drawn and no blocks.
     *
     * nullopt for Void, which generates nothing and so fits any height.
     */
    std::optional<std::pair<int, int>> nativeHeightOf(GeneratorType gen)
    {
        switch (gen)
        {
        case GeneratorType::Overworld:
        case GeneratorType::Flat:
            return std::pair<int, int>{-64, 320};
        case GeneratorType::Nether:
            return std::pair<int, int>{0, 128};
        case GeneratorType::TheEnd:
            return std::pair<int, int>{0, 256};
        default:
            return std::nullopt;
        }
    }

    } // namespace

    DimensionHeightRange dimensionHeightOf(CompoundTag const& stored, char const* nameForLog)
    {
        auto [s, problems] = DimensionSpec::fromNbt(stored);
        int minY = s ? s->minY : kWorldMinY;
        int maxY = s ? s->maxY : kWorldMaxY;

        // An engine generator decides the height; the spec's own is used only where the
        // terrain comes from this host. Both readers of this function get the same answer,
        // which is what keeps the Dimension and the definition one shape.
        if (s)
        {
            if (auto const* n = std::get_if<Native>(&s->terrain))
            {
                if (auto const fixed = nativeHeightOf(n->generator))
                {
                    // Only where the caller named the dimension: this function answers the
                    // constructor and the registration both, and the same line four times
                    // says nothing the first one did not.
                    if (nameForLog && (fixed->first != minY || fixed->second != maxY))
                    {
                        hostLogger().warn(
                            "[dim] {}",
                            pier::trf("dim.height.generator_mismatch", nameForLog, minY, maxY,
                                      magic_enum::enum_name(n->generator), fixed->first, fixed->second)
                        );
                    }
                    minY = fixed->first;
                    maxY = fixed->second;
                }
            }
        }
        return DimensionHeightRange{static_cast<short>(minY), static_cast<short>(maxY)};
    }

    GeneratorType clientGeneratorOf(CompoundTag const& stored)
    {
        auto [s, problems] = DimensionSpec::fromNbt(stored);
        return s ? s->sky.client : GeneratorType::Flat;
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
        if (auto* n = std::get_if<Native>(&terrain); n && n->biome.empty()) n->biome = "minecraft:plains";
        return changed;
    }

    CompoundTag packTerrainTag(Pack const& p)
    {
        CompoundTag t;
        t.putString("kind", p.kind);
        t.putString("pack", p.path);
        t.putString("sha256", p.sha256);
        CompoundTag params;
        for (auto const& [k, v] : p.params) params.putInt64(k, v);
        t.putCompound("params", std::move(params));
        CompoundTag roles;
        for (auto const& [k, v] : p.roles) roles.putString(k, v);
        t.putCompound("roles", std::move(roles));
        return t;
    }

    std::pair<std::optional<DimensionSpec>, std::vector<std::string>> DimensionSpec::fromNbt(CompoundTag const& t)
    {
        std::vector<std::string> problems;
        DimensionSpec s;
        s.seed = static_cast<uint>(num(t, "seed", 0));

        // No terrain section is not an incomplete payload any more: it is how a mod
        // that supplies its own terrain describes a dimension. The pre-26.20.3 shapes
        // still have to be told apart, since they carry their terrain under other keys
        // and reading them as supplied would silently produce an empty world.
        bool const supplied = !t.contains("terrain");
        if (supplied && (t.contains("layout") || t.contains("generatorType")))
        {
            problems.push_back("the payload is in the pre-26.20.3 shape; run tools/migrate_dimension_config.py once on worlds/<level>/dimension_config.json, then restart. A plot entry is not migrated in place: the tool prints the steps for building its terrain as a template pack");
            return {std::nullopt, problems};
        }

        if (t.contains("height"))
        {
            auto const* hp = compound(t, "height", &problems);
            if (!hp) return {std::nullopt, problems};
            auto const& h = *hp;
            s.minY = num(h, "min", s.minY);
            s.maxY = num(h, "max", s.maxY);
        }
        if (t.contains("sky"))
        {
            auto const* skyp = compound(t, "sky", &problems);
            if (!skyp) return {std::nullopt, problems};
            auto const& sky = *skyp;
            if (auto g = generatorNamed(str(sky, "client", "overworld"))) s.sky.client = *g;
            else problems.push_back("sky.client must be overworld, nether or end");
            s.sky.skylight = boolean(sky, "skylight", !s.sky.timeless());
            s.sky.weather = boolean(sky, "weather", !s.sky.timeless());
            if (sky.contains("time"))
            {
                auto const tick = num(sky, "time", 0);
                if (tick < 0 || tick > 23999) problems.push_back("sky.time must be 0..23999");
                else s.sky.time = tick;
            }
        }

        if (supplied)
        {
            s.terrain = Supplied{};
            return {s, problems};
        }
        auto const* terrainp = compound(t, "terrain", &problems);
        if (!terrainp)
        {
            problems.push_back("the payload's terrain is not a section");
            return {std::nullopt, problems};
        }
        auto const& terrain = *terrainp;
        auto const kind = str(terrain, "kind", "");
        if (kind == "native")
        {
            auto g = generatorNamed(str(terrain, "generator", ""));
            if (!g) { problems.push_back("terrain.generator must be overworld, nether, end, flat or void"); return {std::nullopt, problems}; }
            s.terrain = Native{*g, str(terrain, "biome", "minecraft:plains"), num(terrain, "engine_terrain", 1) != 0};
        }
        else if (kind == "template" || kind == "volume")
        {
            Pack p;
            p.kind = kind;
            p.path = str(terrain, "pack", "");
            p.sha256 = str(terrain, "sha256", "");
            // The caller's spec names no hash yet; the stored one always does, because
            // Slots.cpp writes it. A hash that is present and malformed is refused.
            if (!p.sha256.empty() && !hexDigits(p.sha256)) problems.push_back("terrain.sha256 is not 64 hex digits");
            if (terrain.contains("params"))
            {
                auto const* paramsp = compound(terrain, "params", &problems);
                for (auto const& [k, v] : paramsp ? *paramsp : CompoundTag{})
                {
                    auto id = v.getId();
                    if (id != Tag::Type::Int64 && id != Tag::Type::Int && id != Tag::Type::Short && id != Tag::Type::Byte)
                    {
                        problems.push_back("terrain.params." + k + " is not an integer");
                        continue;
                    }
                    p.params[k] = static_cast<std::int64_t>(v);
                }
            }
            if (terrain.contains("roles"))
            {
                auto const* rolesp = compound(terrain, "roles", &problems);
                for (auto const& [k, v] : rolesp ? *rolesp : CompoundTag{})
                {
                    if (v.getId() != Tag::Type::String)
                    {
                        problems.push_back("terrain.roles." + k + " is not a string");
                        continue;
                    }
                    p.roles[k] = std::string{static_cast<std::string_view>(v)};
                }
            }
            s.terrain = std::move(p);
        }
        else if (kind == "layers")
        {
            Layers l;
            l.baseY = static_cast<int>(num(terrain, "base_y", 63));
            l.biome = str(terrain, "biome", "minecraft:plains");
            if (terrain.contains("layers") && terrain.at("layers").is_array())
            {
                for (auto const& ePtr : terrain.at("layers").get<ListTag>())
                {
                    if (!ePtr || ePtr->getId() != Tag::Type::Compound) continue;
                    auto const& c = static_cast<CompoundTag const&>(*ePtr);
                    l.layers.push_back(Layers::Layer{str(c, "block", ""), static_cast<int>(num(c, "thickness", 1))});
                }
            }
            if (l.layers.empty() && !terrain.contains("grid"))
            {
                // A stack of nothing with no grid is a void, and native/void is the one
                // that says so. Reading it as terrain would give a dimension whose only
                // block is the bedrock floor, which nobody writes down on purpose.
                problems.push_back("terrain.layers is empty and there is no grid; use terrain.kind native with generator void for an empty world");
                return {std::nullopt, problems};
            }
            if (auto const* gp = compound(terrain, "grid", &problems))
            {
                auto const& g = *gp;
                Layers::Grid grid;
                grid.cell = static_cast<int>(num(g, "cell", 64));
                grid.gap = static_cast<int>(num(g, "gap", 7));
                grid.edge = static_cast<int>(num(g, "edge", 1));
                grid.gapBlock = str(g, "gap_block", "minecraft:birch_planks");
                grid.edgeBlock = str(g, "edge_block", "minecraft:stone_block_slab");
                grid.confine = num(g, "confine", 0) != 0;
                l.grid = grid;
            }
            s.terrain = l;
        }
        else if (kind == "noise")
        {
            problems.push_back("terrain.kind noise is not served inline; build a terrain pack with tools/pier-pack and register it with md_add_dimension_pack. The layers kind is served again and needs no tool");
            return {std::nullopt, problems};
        }
        else
        {
            problems.push_back("terrain.kind must be native, layers, template or volume");
            return {std::nullopt, problems};
        }

        bool fatal = std::any_of(problems.begin(), problems.end(), [](auto const& p)
        {
            return p.find("empty") != std::string::npos || p.find("hex") != std::string::npos || p.find("not an integer") != std::string::npos;
        });
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
