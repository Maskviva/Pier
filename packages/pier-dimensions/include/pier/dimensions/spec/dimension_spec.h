/** pier/dimensions/spec/dimension_spec.h: one declarative description of a dimension.
 * A spec is seed, height range, sky, and a terrain of one of two kinds: native, a vanilla
 * generator by name, or pack, a terrain pack file with the parameters and roles it was
 * mounted with. The SNBT shape is what md_add_dimension and md_add_dimension_pack
 * document, and DimensionSpec.cpp is the one place that reads it. A pack spec always
 * holds the file's sha256 and the bound parameter values, because the spec is persisted
 * with the dimension and terrain generated from it has to be regenerable from the spec
 * alone. */
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/level/GeneratorType.h"
#include "mc/world/level/dimension/DimensionHeightRange.h"

#include "pier/dimensions/dim/dimension_height.h"

namespace pier::dimensions::spec
{
    /** A vanilla generator. biome is read for Void only, where the engine's void
     *  generator needs a fixed biome source. */
    struct Native
    {
        GeneratorType generator = GeneratorType::Overworld;
        std::string biome = "minecraft:plains";
    };

    /** A terrain pack as mounted. path is the pack config as given to the slot, relative
     *  to the server root, and may be empty in a caller's spec since the slot names it;
     *  sha256 is the hex hash of the binary at first registration, empty in a caller's spec;
     *  params holds every non-derived parameter with its bound value; roles holds only
     *  the overrides. kind is template or volume and must equal the binary's magic. */
    struct Pack
    {
        std::string kind;
        std::string path;
        std::string sha256;
        std::map<std::string, std::int64_t> params;
        std::map<std::string, std::string> roles;

        [[nodiscard]] bool isTemplate() const { return kind == "template"; }
        [[nodiscard]] bool isVolume() const { return kind == "volume"; }
    };

    struct Sky
    {
        GeneratorType client = GeneratorType::Overworld;
        bool skylight = true;
        bool weather = true;
        std::optional<int> time;
        [[nodiscard]] bool timeless() const { return client == GeneratorType::Nether || client == GeneratorType::TheEnd; }
    };

    struct DimensionSpec
    {
        uint seed = 0;
        int minY = kWorldMinY;
        int maxY = kWorldMaxY;
        Sky sky;
        std::variant<Native, Pack> terrain;

        [[nodiscard]] bool isNative() const { return std::holds_alternative<Native>(terrain); }
        [[nodiscard]] bool isPack() const { return std::holds_alternative<Pack>(terrain); }

        /** Rounds the height onto subchunk boundaries and into the world range; returns
         *  one line per change so the caller can log them. */
        std::vector<std::string> clamp();

        /** Reads the stored payload or a caller's SNBT. The first of the pair is empty
         *  on refusal, and the second holds every problem found, refusals and warnings
         *  alike. A refusal never yields a default spec. */
        static std::pair<std::optional<DimensionSpec>, std::vector<std::string>> fromNbt(CompoundTag const& t);
        static std::pair<std::optional<DimensionSpec>, std::vector<std::string>> fromSnbt(std::string const& snbt);
    };

    /** The terrain compound a Pack serializes to; Slots.cpp replaces the caller's
     *  terrain with it before the spec is stored, so the persisted form is complete. */
    [[nodiscard]] CompoundTag packTerrainTag(Pack const& p);

    [[nodiscard]] DimensionHeightRange dimensionHeightOf(CompoundTag const& stored);

    [[nodiscard]] GeneratorType clientGeneratorOf(CompoundTag const& stored);
} // namespace pier::dimensions::spec
