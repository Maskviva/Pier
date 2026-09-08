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

#include "mc/deps/nbt/CompoundTagVariant.h"

namespace pier::dimensions::spec
{
    namespace
    {
        int num(CompoundTag const& c, char const* key, int fallback)
        {
            return c.contains(key) ? static_cast<int>(c.at(key)) : fallback;
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

        if (!t.contains("terrain"))
        {
            if (t.contains("layout") || t.contains("generatorType"))
                problems.push_back("the payload is in the pre-26.20.3 shape; run tools/migrate_dimension_config.py once on worlds/<level>/dimension_config.json, then restart. A plot entry is not migrated in place: the tool prints the steps for building its terrain as a template pack");
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
                auto const tick = num(sky, "time", 0);
                if (tick < 0 || tick > 23999) problems.push_back("sky.time must be 0..23999");
                else s.sky.time = tick;
            }
        }

        auto const& terrain = t.at("terrain").get<CompoundTag>();
        auto const kind = str(terrain, "kind", "");
        if (kind == "native")
        {
            auto g = generatorNamed(str(terrain, "generator", ""));
            if (!g) { problems.push_back("terrain.generator must be overworld, nether, end, flat or void"); return {std::nullopt, problems}; }
            s.terrain = Native{*g, str(terrain, "biome", "minecraft:plains")};
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
                for (auto const& [k, v] : terrain.at("params").get<CompoundTag>())
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
                for (auto const& [k, v] : terrain.at("roles").get<CompoundTag>())
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
        else if (kind == "layers" || kind == "noise")
        {
            problems.push_back("terrain.kind " + kind + " is no longer served; build a terrain pack with tools/pier-pack (from-layers converts an old layers spec) and register it with md_add_dimension_pack");
            return {std::nullopt, problems};
        }
        else
        {
            problems.push_back("terrain.kind must be native, template or volume");
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
