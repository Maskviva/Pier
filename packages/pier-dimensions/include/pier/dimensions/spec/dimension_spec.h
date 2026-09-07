/** pier/dimensions/spec/dimension_spec.h: one declarative description of a custom dimension.
 * Seed, height range, sky, and a terrain recipe of one of three kinds: native (an engine
 * generator), layers (a stack with an optional grid) or noise (density terrain with
 * multi-biome placement). The SNBT shape is documented at md_add_dimension in abi.h and is
 * written by the RSW world manager; this file is the only reader.
 * The payload of an existing dimension is never rewritten: it persists beside chunks generated
 * from it, and a changed reading puts new chunks at odds with them along a seam nothing can
 * move. Payloads in the pre-26.20.3 shape are migrated once by tools/migrate_dimension_config.py
 * and refused here. Values crossing the ABI are clamped, never trusted: a height out of range
 * indexes a fixed chunk buffer out of bounds. */
#pragma once

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
    struct Layer
    {
        std::string block;
        int thickness = 1;
    };

    /** period = cell + gap; mod(coord, period) >= cell is gap, within edge of the cell
     *  boundary is edge, otherwise interior. The mod side shares this convention. */
    struct Grid
    {
        int cell = 64;
        int gap = 7;
        int edge = 1;
        std::string gapBlock = "minecraft:birch_planks";
        std::string edgeBlock = "minecraft:stone_block_slab";
        /** Register this grid with the cell-confinement hooks too. One definition of the
         *  grid for terrain and confinement; the two can never disagree. */
        bool confine = false;
        [[nodiscard]] int period() const { return cell + gap; }
    };

    struct Native
    {
        GeneratorType generator = GeneratorType::Overworld;
    };

    struct Layers
    {
        int baseY = 63;
        std::string biome = "minecraft:plains";
        std::vector<Layer> layers;
        std::optional<Grid> grid;
        [[nodiscard]] bool isVoid() const { return layers.empty() && !grid; }
        [[nodiscard]] int surfaceY() const;
    };

    struct Range
    {
        float lo = -1.f;
        float hi = 1.f;
    };

    struct BiomeTarget
    {
        std::string biome;
        Range temperature, humidity, continentalness, erosion, depth, weirdness;
        float offset = 0.f;
    };

    struct Octave
    {
        float scaleXZ = 0.01f;
        float scaleY = 0.02f;
        float amplitude = 1.f;
        int levels = 4;
    };

    struct PaletteEntry
    {
        std::string block;
        int depthLo = 0;
        int depthHi = 0;
    };

    struct Noise
    {
        std::vector<BiomeTarget> biomes;
        int gradientFromY = 0;
        int gradientToY = 128;
        float threshold = 0.f;
        std::vector<Octave> octaves;
        std::optional<std::pair<float, float>> islands; // scale, floor
        std::vector<PaletteEntry> palette;
        std::optional<std::pair<std::string, int>> fluid;   // block, level
        std::optional<std::pair<std::string, int>> bedrock; // block, y
    };

    struct Sky
    {
        /** What the client is told in DimensionDefinition. Nether and End skies have no
         *  day/night; this is how those dimensions lock time, and it is independent of the
         *  server-side generator. */
        GeneratorType client = GeneratorType::Overworld;
        bool skylight = true;
        bool weather = true;
        /** The tick of day this dimension is held at, 0..23999, or empty to follow the
         *  level clock. A nether or end client sky has no day cycle to begin with, so
         *  this only changes what an overworld sky shows. */
        std::optional<int> time;
        [[nodiscard]] bool timeless() const { return client == GeneratorType::Nether || client == GeneratorType::TheEnd; }
    };

    struct DimensionSpec
    {
        uint seed = 0;
        int minY = kWorldMinY;
        int maxY = kWorldMaxY;
        Sky sky;
        std::variant<Native, Layers, Noise> terrain;

        [[nodiscard]] bool isNative() const { return std::holds_alternative<Native>(terrain); }
        [[nodiscard]] bool isLayers() const { return std::holds_alternative<Layers>(terrain); }
        [[nodiscard]] bool isNoise() const { return std::holds_alternative<Noise>(terrain); }

        /** Clamps into the safe range and reports what it changed. Height stays on
         *  subchunk boundaries; a stack that does not fit is cut, never silently grown. */
        std::vector<std::string> clamp();


        /** Reads the spec shape. Returns the problems found; `nullopt` means refused.
         *  A payload in the pre-26.20.3 shape ({seed,generatorType} or {seed,layout}) is
         *  refused with a line naming tools/migrate_dimension_config.py: that shape is
         *  migrated once in the file, not read forever in code. */
        static std::pair<std::optional<DimensionSpec>, std::vector<std::string>> fromNbt(CompoundTag const& t);
        static std::pair<std::optional<DimensionSpec>, std::vector<std::string>> fromSnbt(std::string const& snbt);

    };

    /** Height range read from a stored payload, for the Dimension base constructor, which
     *  runs before the spec member is built. The same two numbers go to the client through
     *  CustomDimensionManager; both must come from here. */
    [[nodiscard]] DimensionHeightRange dimensionHeightOf(CompoundTag const& stored);

    /** What the client is told in DimensionDefinition; see spec::Sky. */
    [[nodiscard]] GeneratorType clientGeneratorOf(CompoundTag const& stored);

    [[nodiscard]] inline int positiveMod(int value, int modulus)
    {
        int r = value % modulus;
        return r < 0 ? r + modulus : r;
    }

    enum class CellArea
    {
        Interior,
        Edge,
        Gap
    };

    [[nodiscard]] inline CellArea classify1D(int offset, Grid const& g)
    {
        if (offset >= g.cell) return CellArea::Gap;
        if (g.edge > 0 && (offset < g.edge || offset >= g.cell - g.edge)) return CellArea::Edge;
        return CellArea::Interior;
    }

    [[nodiscard]] inline CellArea combine2D(CellArea x, CellArea z)
    {
        if (x == CellArea::Gap || z == CellArea::Gap) return CellArea::Gap;
        if (x == CellArea::Interior && z == CellArea::Interior) return CellArea::Interior;
        return CellArea::Edge;
    }
} // namespace pier::dimensions::spec
