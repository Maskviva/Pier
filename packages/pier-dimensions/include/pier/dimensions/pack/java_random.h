/** java_random.h: the random sources and noise samplers of Java Edition worldgen,
 * as the volume generator needs them. The tool's pierpack/java_noise.py is the
 * reference; the equivalence test compares the two over the same seeds and inputs. */
#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace pier::dimensions::pack::jr
{
    std::array<std::uint8_t, 16> md5(std::uint8_t const* data, std::size_t len);

    std::int32_t javaStringHash(std::string const& s);

    std::uint64_t mthGetSeed(std::int32_t x, std::int32_t y, std::int32_t z);

    float toF32(double v);

    /** One of the two random sources; the flag decides which arithmetic runs. */
    class RandomSource
    {
        bool mLegacy;
        std::uint64_t mLo = 0;
        std::uint64_t mHi = 0;
        std::uint64_t mSeed48 = 0;
        std::int32_t next(int bits);

    public:
        static RandomSource xoroshiro(std::uint64_t lo, std::uint64_t hi);
        static RandomSource xoroshiroFromSeed(std::uint64_t seed);
        static RandomSource legacy(std::uint64_t seed);

        std::uint64_t nextLong();
        std::int32_t nextInt(std::int32_t bound);
        double nextDouble();
        float nextFloat();
        void consume(int n);
        [[nodiscard]] bool isLegacy() const { return mLegacy; }

    private:
        explicit RandomSource(bool legacy) : mLegacy(legacy) {}
    };

    /** A positional factory after forkPositional: fromHashOf(name) and at(x, y, z). */
    class Positional
    {
        bool mLegacy;
        std::uint64_t mLo;
        std::uint64_t mHi;

    public:
        Positional(bool legacy, std::uint64_t lo, std::uint64_t hi) : mLegacy(legacy), mLo(lo), mHi(hi) {}
        static Positional fork(RandomSource& r);
        [[nodiscard]] RandomSource fromHashOf(std::string const& name) const;
        [[nodiscard]] RandomSource at(std::int32_t x, std::int32_t y, std::int32_t z) const;
    };

    struct NoiseTable
    {
        std::uint8_t perm[256];
        double xo = 0, yo = 0, zo = 0;
        explicit NoiseTable(RandomSource& r);
        [[nodiscard]] int p(int i) const { return perm[i & 0xFF]; }
    };

    double improvedNoise(NoiseTable const& t, double x, double y, double z, double yScale, double yMax);
    double wrap(double v);

    struct PerlinNoise
    {
        std::int32_t firstOctave = 0;
        std::vector<double> amplitudes;
        std::vector<std::unique_ptr<NoiseTable>> tables;
        double lowestFreqInputFactor = 1;
        double lowestFreqValueFactor = 1;

        static PerlinNoise create(RandomSource& r, std::int32_t firstOctave, std::vector<double> const& amps);
        static PerlinNoise createLegacy(RandomSource& r, std::int32_t firstOctave, std::vector<double> const& amps);
        [[nodiscard]] NoiseTable const* octave(int i) const { return tables[tables.size() - 1 - static_cast<std::size_t>(i)].get(); }
        [[nodiscard]] double value(double x, double y, double z, double yScale = 0, double yMax = 0) const;
    };

    struct NormalNoise
    {
        PerlinNoise first;
        PerlinNoise second;
        double valueFactor = 1;
        static NormalNoise create(RandomSource& r, std::int32_t firstOctave, std::vector<double> const& amps, bool legacy);
        [[nodiscard]] double value(double x, double y, double z) const;
    };

    double simplex2D(NoiseTable const& t, double x, double z);
    float endIslandHeight(NoiseTable const& t, std::int32_t i, std::int32_t j);

    struct BlendedNoise
    {
        PerlinNoise minLimit;
        PerlinNoise maxLimit;
        PerlinNoise main;
        double xzMultiplier = 0, yMultiplier = 0, xzFactor = 1, yFactor = 1, smear = 1;
        static BlendedNoise create(RandomSource& r, double xzScale, double yScale, double xzFactor, double yFactor, double smear);
        [[nodiscard]] double value(std::int32_t x, std::int32_t y, std::int32_t z) const;
    };

    inline double lerp(double t, double a, double b) { return a + t * (b - a); }
    double clampedLerp(double a, double b, double t);
} // namespace pier::dimensions::pack::jr
