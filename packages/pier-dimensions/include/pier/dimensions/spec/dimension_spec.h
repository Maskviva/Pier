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
        /** Whether the engine's own terrain generator is used for overworld, nether and
         *  end, rather than a void with the same sky.
         *
         *  On unless the stored spec says `engine_terrain: 0b`. The switch exists because
         *  these three are engine code this host cannot repair, and an operator who meets
         *  a fault in one still needs a dimension that opens: the sky is a separate
         *  setting, so a void keeps the look the generator was chosen for and only the
         *  blocks are lost. Void and the terrain packs never read it, being this host's
         *  own code. */
        bool engineTerrain = true;
    };

    /** A terrain pack as mounted. path is the pack config as given to the slot, relative
     *  to the server root, and may be empty in a caller's spec since the slot names it;
     *  sha256 is the hex hash of the binary at first registration, empty in a caller's spec;
     *  params holds every non-derived parameter with its bound value; roles holds only
     *  the overrides. kind is template or volume and must equal the binary's magic. */
    /** The terrain belongs to the mod that registered the dimension: a payload with no
     *  terrain section at all. This host stores the spec and asks that mod for every
     *  chunk; what shape the terrain has is not something it knows or needs to.
     *
     *  A saved dimension whose mod is gone loads as a void and says so once, and its
     *  chunks stay where they are: the spec is what the mod will re-register against. */
    struct Supplied
    {
    };

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

    /** A stack of blocks, and optionally a grid cut into it. The plot world and the
     *  superflat world are this and nothing else.
     *
     *  It is declarative and carries no file, so unlike a Pack it needs no hash: the
     *  spec is the terrain, and the same spec generates the same blocks. That is why it
     *  is a terrain kind of its own rather than a pack the caller has to build first.
     *  What it becomes is a template pack assembled in memory, so the generator, the
     *  chunk fill and the confinement hook underneath are the ones the pack path uses
     *  and the ones the tests cover. */
    struct Layers
    {
        struct Layer
        {
            std::string block;
            int thickness = 1;
        };

        /** The grid the stack is cut into. `cell` is the plot, `gap` the road between
         *  them, `edge` the border ring drawn inside the plot. `confine` registers the
         *  same geometry with the confinement hook, which is what stops a piston or a
         *  mob from crossing a plot boundary; the terrain alone does not do that. */
        struct Grid
        {
            int cell = 64;
            int gap = 7;
            int edge = 1;
            std::string gapBlock = "minecraft:birch_planks";
            std::string edgeBlock = "minecraft:stone_block_slab";
            bool confine = false;
        };

        int baseY = 63;
        std::string biome = "minecraft:plains";
        std::vector<Layer> layers;
        std::optional<Grid> grid;
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
        std::variant<Native, Pack, Layers, Supplied> terrain;

        [[nodiscard]] bool isNative() const { return std::holds_alternative<Native>(terrain); }
        [[nodiscard]] bool isPack() const { return std::holds_alternative<Pack>(terrain); }
        [[nodiscard]] bool isSupplied() const { return std::holds_alternative<Supplied>(terrain); }
        [[nodiscard]] bool isLayers() const { return std::holds_alternative<Layers>(terrain); }

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

    /** The height the dimension is built with: the engine generator's own when the terrain
     *  is one of them, the spec's otherwise. `nameForLog` names the dimension in the line
     *  reporting an override; passing nothing keeps it quiet, for the callers that ask on
     *  every construction. */
    [[nodiscard]] DimensionHeightRange dimensionHeightOf(CompoundTag const& stored, char const* nameForLog = nullptr);

    [[nodiscard]] GeneratorType clientGeneratorOf(CompoundTag const& stored);
} // namespace pier::dimensions::spec
