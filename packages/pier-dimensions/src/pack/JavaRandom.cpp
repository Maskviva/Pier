/** JavaRandom.cpp: Java's worldgen random sources and noise samplers, bit for bit where
 * the arithmetic is integer and operation for operation where it is floating point. */
#include "pier/dimensions/pack/java_random.h"

#include <cmath>
#include <cstring>

namespace pier::dimensions::pack::jr
{
    namespace
    {
        constexpr std::uint64_t kMask48 = (std::uint64_t(1) << 48) - 1;

        std::uint64_t rotl(std::uint64_t v, int n) { return (v << n) | (v >> (64 - n)); }

        std::uint64_t mixStafford13(std::uint64_t z)
        {
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
            return z ^ (z >> 31);
        }

        constexpr std::uint32_t kMd5K[64] = {
            0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
            0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
            0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
            0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
            0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
            0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
            0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
            0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
        };
        constexpr int kMd5S[64] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                                   5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
                                   4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                                   6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

        std::uint32_t rotl32(std::uint32_t v, int n) { return (v << n) | (v >> (32 - n)); }

        constexpr int kGradient[16][3] = {{1, 1, 0}, {-1, 1, 0}, {1, -1, 0}, {-1, -1, 0}, {1, 0, 1}, {-1, 0, 1},
                                          {1, 0, -1}, {-1, 0, -1}, {0, 1, 1}, {0, -1, 1}, {0, 1, -1}, {0, -1, -1},
                                          {1, 1, 0}, {0, -1, 1}, {-1, 1, 0}, {0, -1, -1}};

        double gradDot(int hash, double x, double y, double z)
        {
            auto const& g = kGradient[hash & 15];
            return g[0] * x + g[1] * y + g[2] * z;
        }

        double smoothstep(double x) { return x * x * x * (x * (x * 6.0 - 15.0) + 10.0); }
    } // namespace

    std::array<std::uint8_t, 16> md5(std::uint8_t const* data, std::size_t len)
    {
        std::uint32_t a0 = 0x67452301, b0 = 0xefcdab89, c0 = 0x98badcfe, d0 = 0x10325476;
        std::vector<std::uint8_t> msg(data, data + len);
        msg.push_back(0x80);
        while (msg.size() % 64 != 56) msg.push_back(0);
        std::uint64_t bits = std::uint64_t(len) * 8;
        for (int i = 0; i < 8; ++i) msg.push_back(std::uint8_t(bits >> (8 * i)));
        for (std::size_t off = 0; off < msg.size(); off += 64)
        {
            std::uint32_t m[16];
            for (int i = 0; i < 16; ++i)
                m[i] = std::uint32_t(msg[off + i * 4]) | (std::uint32_t(msg[off + i * 4 + 1]) << 8)
                    | (std::uint32_t(msg[off + i * 4 + 2]) << 16) | (std::uint32_t(msg[off + i * 4 + 3]) << 24);
            std::uint32_t a = a0, b = b0, c = c0, d = d0;
            for (int i = 0; i < 64; ++i)
            {
                std::uint32_t f;
                int g;
                if (i < 16) { f = (b & c) | (~b & d); g = i; }
                else if (i < 32) { f = (d & b) | (~d & c); g = (5 * i + 1) % 16; }
                else if (i < 48) { f = b ^ c ^ d; g = (3 * i + 5) % 16; }
                else { f = c ^ (b | ~d); g = (7 * i) % 16; }
                std::uint32_t tmp = d;
                d = c;
                c = b;
                b = b + rotl32(a + f + kMd5K[i] + m[g], kMd5S[i]);
                a = tmp;
            }
            a0 += a; b0 += b; c0 += c; d0 += d;
        }
        std::array<std::uint8_t, 16> out{};
        std::uint32_t const words[4] = {a0, b0, c0, d0};
        for (int i = 0; i < 4; ++i)
            for (int k = 0; k < 4; ++k) out[i * 4 + k] = std::uint8_t(words[i] >> (8 * k));
        return out;
    }

    std::int32_t javaStringHash(std::string const& s)
    {
        // UTF-8 to UTF-16 code units, then Java's polynomial hash.
        std::uint32_t h = 0;
        std::size_t i = 0;
        while (i < s.size())
        {
            unsigned char c = static_cast<unsigned char>(s[i]);
            std::uint32_t cp;
            int extra;
            if (c < 0x80) { cp = c; extra = 0; }
            else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; extra = 1; }
            else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; extra = 2; }
            else { cp = c & 0x07; extra = 3; }
            ++i;
            for (int k = 0; k < extra && i < s.size(); ++k, ++i) cp = (cp << 6) | (static_cast<unsigned char>(s[i]) & 0x3F);
            if (cp >= 0x10000)
            {
                cp -= 0x10000;
                h = 31 * h + (0xD800 + (cp >> 10));
                h = 31 * h + (0xDC00 + (cp & 0x3FF));
            }
            else h = 31 * h + cp;
        }
        return static_cast<std::int32_t>(h);
    }

    std::uint64_t mthGetSeed(std::int32_t x, std::int32_t y, std::int32_t z)
    {
        std::int64_t l = std::int64_t(static_cast<std::int32_t>(static_cast<std::uint32_t>(x) * 3129871u))
            ^ (std::int64_t(z) * 116129781LL) ^ std::int64_t(y);
        std::uint64_t u = static_cast<std::uint64_t>(l);
        u = u * u * 42317861ull + u * 11ull;
        return static_cast<std::uint64_t>(static_cast<std::int64_t>(u) >> 16);
    }

    float toF32(double v) { return static_cast<float>(v); }

    RandomSource RandomSource::xoroshiro(std::uint64_t lo, std::uint64_t hi)
    {
        RandomSource r(false);
        if ((lo | hi) == 0)
        {
            lo = 0x9E3779B97F4A7C15ull;
            hi = 0x6A09E667F3BCC909ull;
        }
        r.mLo = lo;
        r.mHi = hi;
        return r;
    }

    RandomSource RandomSource::xoroshiroFromSeed(std::uint64_t seed)
    {
        std::uint64_t lo = seed ^ 0x6A09E667F3BCC909ull;
        std::uint64_t hi = lo + 0x9E3779B97F4A7C15ull;
        return xoroshiro(mixStafford13(lo), mixStafford13(hi));
    }

    RandomSource RandomSource::legacy(std::uint64_t seed)
    {
        RandomSource r(true);
        r.mSeed48 = (seed ^ 0x5DEECE66Dull) & kMask48;
        return r;
    }

    std::int32_t RandomSource::next(int bits)
    {
        mSeed48 = (mSeed48 * 0x5DEECE66Dull + 0xBull) & kMask48;
        return static_cast<std::int32_t>(static_cast<std::uint32_t>(mSeed48 >> (48 - bits)));
    }

    std::uint64_t RandomSource::nextLong()
    {
        if (mLegacy)
        {
            std::int64_t hi = next(32);
            std::int64_t lo = next(32);
            return static_cast<std::uint64_t>((hi << 32) + lo);
        }
        std::uint64_t l = mLo, m = mHi;
        std::uint64_t n = rotl(l + m, 17) + l;
        m ^= l;
        mLo = rotl(l, 49) ^ m ^ (m << 21);
        mHi = rotl(m, 28);
        return n;
    }

    std::int32_t RandomSource::nextInt(std::int32_t bound)
    {
        if (mLegacy)
        {
            if ((bound & -bound) == bound) return static_cast<std::int32_t>((std::int64_t(bound) * std::int64_t(next(31))) >> 31);
            for (;;)
            {
                std::int32_t bits = next(31);
                std::int32_t val = bits % bound;
                if (bits - val + (bound - 1) >= 0) return val;
            }
        }
        std::uint64_t r = (nextLong() & 0xFFFFFFFFull) * static_cast<std::uint64_t>(bound);
        std::uint64_t lo = r & 0xFFFFFFFFull;
        if (lo < static_cast<std::uint64_t>(bound))
        {
            std::uint32_t j = static_cast<std::uint32_t>(~static_cast<std::uint32_t>(bound) + 1u) % static_cast<std::uint32_t>(bound);
            while (lo < j)
            {
                r = (nextLong() & 0xFFFFFFFFull) * static_cast<std::uint64_t>(bound);
                lo = r & 0xFFFFFFFFull;
            }
        }
        return static_cast<std::int32_t>(r >> 32);
    }

    double RandomSource::nextDouble()
    {
        if (mLegacy)
        {
            std::int64_t hi = next(26);
            std::int64_t lo = next(27);
            return static_cast<double>((hi << 27) + lo) * 1.1102230246251565e-16;
        }
        return static_cast<double>(nextLong() >> 11) * 1.1102230246251565e-16;
    }

    float RandomSource::nextFloat()
    {
        if (mLegacy) return static_cast<float>(next(24)) / static_cast<float>(1 << 24);
        return static_cast<float>(nextLong() >> 40) * 5.9604645e-8f;
    }

    void RandomSource::consume(int n)
    {
        for (int i = 0; i < n; ++i)
        {
            if (mLegacy) next(32);
            else nextLong();
        }
    }

    Positional Positional::fork(RandomSource& r)
    {
        if (r.isLegacy()) return Positional(true, r.nextLong(), 0);
        std::uint64_t lo = r.nextLong();
        std::uint64_t hi = r.nextLong();
        return Positional(false, lo, hi);
    }

    RandomSource Positional::fromHashOf(std::string const& name) const
    {
        if (mLegacy) return RandomSource::legacy(mLo ^ static_cast<std::uint64_t>(static_cast<std::int64_t>(javaStringHash(name))));
        auto d = md5(reinterpret_cast<std::uint8_t const*>(name.data()), name.size());
        std::uint64_t lo = 0, hi = 0;
        for (int i = 0; i < 8; ++i)
        {
            lo = (lo << 8) | d[i];
            hi = (hi << 8) | d[8 + i];
        }
        return RandomSource::xoroshiro(lo ^ mLo, hi ^ mHi);
    }

    RandomSource Positional::at(std::int32_t x, std::int32_t y, std::int32_t z) const
    {
        if (mLegacy) return RandomSource::legacy(mthGetSeed(x, y, z) ^ mLo);
        return RandomSource::xoroshiro(mthGetSeed(x, y, z) ^ mLo, mHi);
    }

    NoiseTable::NoiseTable(RandomSource& r)
    {
        xo = r.nextDouble() * 256.0;
        yo = r.nextDouble() * 256.0;
        zo = r.nextDouble() * 256.0;
        for (int i = 0; i < 256; ++i) perm[i] = static_cast<std::uint8_t>(i);
        for (int i = 0; i < 256; ++i)
        {
            int j = r.nextInt(256 - i);
            std::uint8_t b = perm[i];
            perm[i] = perm[i + j];
            perm[i + j] = b;
        }
    }

    double wrap(double v) { return v - std::floor(v / 3.3554432e7 + 0.5) * 3.3554432e7; }

    double improvedNoise(NoiseTable const& t, double x, double y, double z, double yScale, double yMax)
    {
        double d = x + t.xo, e = y + t.yo, f = z + t.zo;
        int i = static_cast<int>(std::floor(d)), j = static_cast<int>(std::floor(e)), k = static_cast<int>(std::floor(f));
        double g = d - i, h = e - j, l = f - k;
        double m = 0.0;
        if (yScale != 0.0)
        {
            double n = (yMax >= 0.0 && yMax < h) ? yMax : h;
            m = std::floor(n / yScale + 1.0000000116860974e-07) * yScale;
        }
        double yy = h - m;
        int pl = t.p(i), pm = t.p(i + 1);
        int n = t.p(pl + j), o = t.p(pl + j + 1), q = t.p(pm + j), r = t.p(pm + j + 1);
        double d0 = gradDot(t.p(n + k), g, yy, l);
        double e0 = gradDot(t.p(q + k), g - 1.0, yy, l);
        double f0 = gradDot(t.p(o + k), g, yy - 1.0, l);
        double g0 = gradDot(t.p(r + k), g - 1.0, yy - 1.0, l);
        double h0 = gradDot(t.p(n + k + 1), g, yy, l - 1.0);
        double s0 = gradDot(t.p(q + k + 1), g - 1.0, yy, l - 1.0);
        double t0 = gradDot(t.p(o + k + 1), g, yy - 1.0, l - 1.0);
        double u0 = gradDot(t.p(r + k + 1), g - 1.0, yy - 1.0, l - 1.0);
        double v = smoothstep(g), w = smoothstep(h), aa = smoothstep(l);
        return lerp(aa, lerp(w, lerp(v, d0, e0), lerp(v, f0, g0)), lerp(w, lerp(v, h0, s0), lerp(v, t0, u0)));
    }

    PerlinNoise PerlinNoise::create(RandomSource& r, std::int32_t firstOctave, std::vector<double> const& amps)
    {
        PerlinNoise p;
        p.firstOctave = firstOctave;
        p.amplitudes = amps;
        auto factory = Positional::fork(r);
        for (std::size_t k = 0; k < amps.size(); ++k)
        {
            if (amps[k] != 0.0)
            {
                auto child = factory.fromHashOf("octave_" + std::to_string(firstOctave + static_cast<std::int32_t>(k)));
                p.tables.push_back(std::make_unique<NoiseTable>(child));
            }
            else p.tables.push_back(nullptr);
        }
        auto n = static_cast<int>(amps.size());
        p.lowestFreqInputFactor = std::pow(2.0, firstOctave);
        p.lowestFreqValueFactor = std::pow(2.0, n - 1) / (std::pow(2.0, n) - 1.0);
        return p;
    }

    PerlinNoise PerlinNoise::createLegacy(RandomSource& r, std::int32_t firstOctave, std::vector<double> const& amps)
    {
        PerlinNoise p;
        p.firstOctave = firstOctave;
        p.amplitudes = amps;
        auto n = static_cast<int>(amps.size());
        int j = -firstOctave;
        p.tables.resize(amps.size());
        auto first = std::make_unique<NoiseTable>(r);
        if (j >= 0 && j < n && amps[static_cast<std::size_t>(j)] != 0.0) p.tables[static_cast<std::size_t>(j)] = std::move(first);
        for (int k = j - 1; k >= 0; --k)
        {
            if (k < n)
            {
                if (amps[static_cast<std::size_t>(k)] != 0.0) p.tables[static_cast<std::size_t>(k)] = std::make_unique<NoiseTable>(r);
                else r.consume(262);
            }
            else r.consume(262);
        }
        p.lowestFreqInputFactor = std::pow(2.0, firstOctave);
        p.lowestFreqValueFactor = std::pow(2.0, n - 1) / (std::pow(2.0, n) - 1.0);
        return p;
    }

    double PerlinNoise::value(double x, double y, double z, double yScale, double yMax) const
    {
        double result = 0.0;
        double freq = lowestFreqInputFactor;
        double amp = lowestFreqValueFactor;
        for (std::size_t i = 0; i < tables.size(); ++i)
        {
            if (tables[i])
            {
                double v = improvedNoise(*tables[i], wrap(x * freq), wrap(y * freq), wrap(z * freq), yScale * freq, yMax * freq);
                result += amplitudes[i] * v * amp;
            }
            freq *= 2.0;
            amp /= 2.0;
        }
        return result;
    }

    NormalNoise NormalNoise::create(RandomSource& r, std::int32_t firstOctave, std::vector<double> const& amps, bool legacy)
    {
        NormalNoise n;
        n.first = legacy ? PerlinNoise::createLegacy(r, firstOctave, amps) : PerlinNoise::create(r, firstOctave, amps);
        n.second = legacy ? PerlinNoise::createLegacy(r, firstOctave, amps) : PerlinNoise::create(r, firstOctave, amps);
        int lo = -1, hi = -1;
        for (std::size_t k = 0; k < amps.size(); ++k)
        {
            if (amps[k] != 0.0)
            {
                if (lo < 0) lo = static_cast<int>(k);
                hi = static_cast<int>(k);
            }
        }
        int span = lo < 0 ? 1 : hi - lo;
        n.valueFactor = 0.16666666666666666 / (0.1 * (1.0 + 1.0 / (span + 1)));
        return n;
    }

    double NormalNoise::value(double x, double y, double z) const
    {
        constexpr double k = 1.0181268882301;
        return (first.value(x, y, z) + second.value(x * k, y * k, z * k)) * valueFactor;
    }

    double simplex2D(NoiseTable const& t, double x, double z)
    {
        static double const F2 = 0.5 * (std::sqrt(3.0) - 1.0);
        static double const G2 = (3.0 - std::sqrt(3.0)) / 6.0;
        double d = (x + z) * F2;
        int i = static_cast<int>(std::floor(x + d)), j = static_cast<int>(std::floor(z + d));
        double e = (i + j) * G2;
        double f = i - e, g = j - e;
        double h = x - f, k = z - g;
        int l, m;
        if (h > k) { l = 1; m = 0; }
        else { l = 0; m = 1; }
        double n = h - l + G2, o = k - m + G2;
        double p = h - 1.0 + 2.0 * G2, q = k - 1.0 + 2.0 * G2;
        int r = i & 0xFF, s = j & 0xFF;
        int tt = t.p(r + t.p(s)) % 12;
        int u = t.p(r + l + t.p(s + m)) % 12;
        int v = t.p(r + 1 + t.p(s + 1)) % 12;
        auto corner = [](int grad, double cx, double cy)
        {
            double ee = 0.5 - cx * cx - cy * cy;
            if (ee < 0.0) return 0.0;
            ee *= ee;
            return ee * ee * (kGradient[grad][0] * cx + kGradient[grad][1] * cy);
        };
        return 70.0 * (corner(tt, h, k) + corner(u, n, o) + corner(v, p, q));
    }

    float endIslandHeight(NoiseTable const& t, std::int32_t i, std::int32_t j)
    {
        int k = i / 2, l = j / 2;
        int m = i % 2, n = j % 2;
        float f = toF32(100.0 - toF32(std::sqrt(toF32(double(i) * i + double(j) * j))) * 8.0);
        f = std::max(-100.0f, std::min(80.0f, f));
        for (int o = -12; o <= 12; ++o)
        {
            for (int p = -12; p <= 12; ++p)
            {
                std::int64_t q = k + o, r = l + p;
                if (q * q + r * r > 4096 && simplex2D(t, double(q), double(r)) < -0.8999999761581421)
                {
                    float g = toF32(std::fmod(toF32(std::abs(double(q)) * 3439.0 + std::abs(double(r)) * 147.0), 13.0) + 9.0);
                    double h = double(m - o * 2), s = double(n - p * 2);
                    float tt = toF32(100.0 - toF32(std::sqrt(toF32(h * h + s * s))) * g);
                    tt = std::max(-100.0f, std::min(80.0f, tt));
                    f = std::max(f, tt);
                }
            }
        }
        return f;
    }

    BlendedNoise BlendedNoise::create(RandomSource& r, double xzScale, double yScale, double xzFactor, double yFactor, double smear)
    {
        BlendedNoise b;
        std::vector<double> lim(16, 1.0), mainAmps(8, 1.0);
        b.minLimit = PerlinNoise::createLegacy(r, -15, lim);
        b.maxLimit = PerlinNoise::createLegacy(r, -15, lim);
        b.main = PerlinNoise::createLegacy(r, -7, mainAmps);
        b.xzMultiplier = 684.412 * xzScale;
        b.yMultiplier = 684.412 * yScale;
        b.xzFactor = xzFactor;
        b.yFactor = yFactor;
        b.smear = smear;
        return b;
    }

    double clampedLerp(double a, double b, double t)
    {
        if (t < 0.0) return a;
        if (t > 1.0) return b;
        return lerp(t, a, b);
    }

    double BlendedNoise::value(std::int32_t x, std::int32_t y, std::int32_t z) const
    {
        double d = x * xzMultiplier, e = y * yMultiplier, f = z * xzMultiplier;
        double g = d / xzFactor, h = e / yFactor, i = f / xzFactor;
        double j = yMultiplier * smear;
        double k = j / yFactor;
        double l = 0, m = 0, n = 0;
        double o = 1.0;
        for (int p = 0; p < 8; ++p)
        {
            if (auto const* t = main.octave(p)) n += improvedNoise(*t, wrap(g * o), wrap(h * o), wrap(i * o), k * o, h * o) / o;
            o /= 2.0;
        }
        double q = (n / 10.0 + 1.0) / 2.0;
        bool bl2 = q >= 1.0, bl3 = q <= 0.0;
        o = 1.0;
        for (int r = 0; r < 16; ++r)
        {
            double s = wrap(d * o), tt = wrap(e * o), u = wrap(f * o);
            double v = j * o;
            if (!bl2)
                if (auto const* t2 = minLimit.octave(r)) l += improvedNoise(*t2, s, tt, u, v, e * o) / o;
            if (!bl3)
                if (auto const* t3 = maxLimit.octave(r)) m += improvedNoise(*t3, s, tt, u, v, e * o) / o;
            o /= 2.0;
        }
        return clampedLerp(l / 512.0, m / 512.0, q) / 128.0;
    }
} // namespace pier::dimensions::pack::jr
