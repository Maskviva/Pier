/** volume_pack.h: a PIERVOL pack decoded into owned tables, seeded for one world, and
 * generated chunk by chunk.
 * VolumePack is the file after validation. SeededVolume holds the samplers built from
 * the world seed the way Java's RandomState builds them, and generates a chunk of
 * materials with a biome per column: density first, then the surface rules in Java's
 * column walk. A material is an index into `materials`, 0 being air; the generator
 * turns them into Block pointers once. Everything here is engine-free. */
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "pier/dimensions/pack/java_random.h"
#include "pier/dimensions/pack/pack_format.h"
#include "pier/dimensions/pack/pack_reader.h"

namespace pier::dimensions::pack
{
    struct VolNoise
    {
        std::string name;
        std::int32_t firstOctave = 0;
        std::vector<double> amplitudes;
        double valueFactor = 0;
        std::uint32_t kind = 0;
        std::uint32_t flags = 0;
    };

    struct VolSpline
    {
        std::uint32_t coordNode = 0;
        std::vector<SplinePoint> points;
    };

    struct VolumePack
    {
        Sha256 fileHash{};
        std::string sourceName;
        Settings settings{};
        std::string defaultBlock;
        std::string defaultFluid;
        std::vector<VolNoise> noises;
        std::vector<VolSpline> splines;
        std::vector<float> fcon;
        std::vector<FuncNode> funcs;
        Router router{};
        std::vector<SurfNode> surf;
        std::vector<std::vector<std::uint32_t>> surfLists;
        std::uint32_t surfRoot = kNone;
        std::vector<BiomeTarget> biomes;
        std::vector<std::string> biomeNames;
        std::vector<std::string> rands;
        std::vector<std::string> strings;

        [[nodiscard]] std::int32_t minY() const { return settings.minY; }
        [[nodiscard]] std::int32_t maxY() const { return settings.minY + settings.height; }
    };

    std::optional<VolumePack> decodeVolume(PackFile const& file, std::vector<std::string>& problems);

    /** One world's instance: the samplers, the positional factories, the material table.
     *  materials[0] is air, [1] the default block, [2] the default fluid, then every
     *  block the surface rules name. */
    class SeededVolume
    {
        struct Sampler
        {
            std::optional<jr::NormalNoise> normal;
            std::unique_ptr<jr::NoiseTable> simplex;
            std::optional<jr::BlendedNoise> blended;
        };
        std::shared_ptr<VolumePack const> mPack;
        std::vector<Sampler> mSamplers;
        std::vector<jr::Positional> mRands;
        std::vector<std::string> mMaterials;
        std::vector<std::uint16_t> mSurfMaterial;
        std::int32_t mCellW = 4;
        std::int32_t mCellH = 8;

        struct Chunk;
        double eval(Chunk& c, std::uint32_t i, std::int32_t x, std::int32_t y, std::int32_t z) const;
        float spline(Chunk& c, std::uint32_t si, std::int32_t x, std::int32_t y, std::int32_t z) const;
        std::int32_t preliminarySurfaceLevel(Chunk& c, std::int32_t x, std::int32_t z) const;
        std::size_t pickBiome(Chunk& c, std::int32_t x, std::int32_t y, std::int32_t z) const;
        void applySurface(Chunk& c) const;
        std::uint32_t rule(Chunk& c, std::uint32_t i, std::size_t column, std::int32_t y) const;
        bool cond(Chunk& c, std::uint32_t i, std::size_t column, std::int32_t y) const;

    public:
        SeededVolume(std::shared_ptr<VolumePack const> pack, std::uint64_t seed);

        [[nodiscard]] VolumePack const& pack() const { return *mPack; }
        [[nodiscard]] std::vector<std::string> const& materials() const { return mMaterials; }
        [[nodiscard]] std::int32_t height() const { return mPack->settings.height; }

        /** Materials laid out (x * 16 + z) * height + y from minY, and one BIOM index
         *  per column, x major. */
        void generateChunk(std::int32_t chunkX, std::int32_t chunkZ, std::vector<std::uint16_t>& out,
                           std::vector<std::uint32_t>& biomes) const;
    };
} // namespace pier::dimensions::pack
