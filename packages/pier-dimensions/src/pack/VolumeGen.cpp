/** VolumeGen.cpp: one world's instance of a volume pack and its chunk generation.
 * Seeding follows Java's RandomState: one root random from the seed, a positional
 * factory, fromHashOf(name) per noise. Evaluation follows refgen_vol.py operation for
 * operation, with the same caches: flat_cache at the quart origin with y = 0,
 * interpolated at the cell corners. The surface walk is Java's per-column loop over
 * stone depth above and below and the water height. Every value the tool computes in
 * float is computed in float here, in the same order. */
#include "pier/dimensions/pack/volume_pack.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace pier::dimensions::pack
{
    namespace
    {
        constexpr std::uint16_t kMatAir = 0;
        constexpr std::uint16_t kMatDefault = 1;
        constexpr std::uint16_t kMatFluid = 2;
        constexpr std::int64_t kIntMin = std::numeric_limits<std::int32_t>::min();
        constexpr std::int64_t kIntMax = std::numeric_limits<std::int32_t>::max();
        constexpr std::int64_t kWayBelowMinY = -(std::int64_t(1) << 30);

        std::int32_t floorDiv(std::int32_t a, std::int32_t b)
        {
            std::int32_t q = a / b;
            if ((a % b != 0) && ((a < 0) != (b < 0))) --q;
            return q;
        }

        struct Key
        {
            std::uint32_t node;
            std::int32_t a, b, c;
            bool operator==(Key const& o) const { return node == o.node && a == o.a && b == o.b && c == o.c; }
        };
        struct KeyHash
        {
            std::size_t operator()(Key const& k) const
            {
                std::uint64_t h = k.node;
                h = h * 0x9E3779B97F4A7C15ull ^ static_cast<std::uint32_t>(k.a);
                h = h * 0x9E3779B97F4A7C15ull ^ static_cast<std::uint32_t>(k.b);
                h = h * 0x9E3779B97F4A7C15ull ^ static_cast<std::uint32_t>(k.c);
                return static_cast<std::size_t>(h ^ (h >> 29));
            }
        };

        std::int64_t quantize(double v) { return static_cast<std::int64_t>(double(static_cast<float>(v)) * 10000.0); }

        std::int64_t intervalDistance(std::int64_t lo, std::int64_t hi, std::int64_t v)
        {
            if (v > hi) return v - hi;
            if (v < lo) return lo - v;
            return 0;
        }

        float bitsToFloat(std::uint32_t bits)
        {
            float f;
            std::memcpy(&f, &bits, 4);
            return f;
        }
    } // namespace

    struct SeededVolume::Chunk
    {
        std::int32_t cx = 0, cz = 0;
        std::vector<std::uint16_t> blocks;
        std::vector<std::uint32_t> biome;
        std::vector<std::int32_t> surfaceDepth;
        std::vector<double> secondary;
        std::vector<std::int64_t> minSurfaceLevel;
        std::vector<bool> minSurfaceKnown;
        std::vector<std::int32_t> heights;
        std::unordered_map<Key, double, KeyHash> flat;
        std::unordered_map<Key, double, KeyHash> corner;
        std::unordered_map<Key, std::int64_t, KeyHash> prelim;
        std::int32_t above = 0, below = 0;
        std::int64_t water = kIntMin;
    };

    SeededVolume::SeededVolume(std::shared_ptr<VolumePack const> pack, std::uint64_t seed) : mPack(std::move(pack))
    {
        auto const& p = *mPack;
        bool legacy = (p.settings.flags & kSettingsLegacyRandom) != 0;
        auto root = legacy ? jr::RandomSource::legacy(seed) : jr::RandomSource::xoroshiroFromSeed(seed);
        auto factory = jr::Positional::fork(root);
        for (auto const& n : p.noises)
        {
            Sampler s;
            if (n.kind == kNoiseNormal)
            {
                if (n.flags & kNoiseLegacyBiome)
                {
                    auto idx = n.flags >> kNoiseLegacyIndexShift;
                    auto r = jr::RandomSource::legacy(seed + idx);
                    s.normal = jr::NormalNoise::create(r, n.firstOctave, n.amplitudes, true);
                }
                else
                {
                    auto r = factory.fromHashOf(n.name);
                    s.normal = jr::NormalNoise::create(r, n.firstOctave, n.amplitudes, false);
                }
            }
            else if (n.kind == kNoiseSimplex)
            {
                auto r = jr::RandomSource::legacy(seed);
                r.consume(17292);
                s.simplex = std::make_unique<jr::NoiseTable>(r);
            }
            else
            {
                auto r = factory.fromHashOf("minecraft:terrain");
                auto const& a = n.amplitudes;
                s.blended = jr::BlendedNoise::create(r, a[0], a[1], a[2], a[3], a[4]);
            }
            mSamplers.push_back(std::move(s));
        }
        for (auto const& name : p.rands)
        {
            auto r = factory.fromHashOf(name);
            mRands.push_back(jr::Positional::fork(r));
        }
        mMaterials = {"minecraft:air", p.defaultBlock, p.defaultFluid};
        mSurfMaterial.assign(p.surf.size(), kMatAir);
        for (std::size_t i = 0; i < p.surf.size(); ++i)
        {
            if (static_cast<SurfOp>(p.surf[i].op) != SurfOp::Block) continue;
            auto const& name = p.strings[p.surf[i].aux];
            std::uint16_t m = 0;
            for (; m < mMaterials.size(); ++m)
                if (mMaterials[m] == name) break;
            if (m == mMaterials.size()) mMaterials.push_back(name);
            mSurfMaterial[i] = m;
        }
        mCellW = 4 * static_cast<std::int32_t>(p.settings.sizeHorizontal);
        mCellH = 4 * static_cast<std::int32_t>(p.settings.sizeVertical);
    }

    float SeededVolume::spline(Chunk& c, std::uint32_t si, std::int32_t x, std::int32_t y, std::int32_t z) const
    {
        auto const& sp = mPack->splines[si];
        float f = static_cast<float>(eval(c, sp.coordNode, x, y, z));
        auto const& pts = sp.points;
        std::size_t lo = 0, hi = pts.size();
        while (lo < hi)
        {
            std::size_t mid = (lo + hi) >> 1;
            if (f < pts[mid].location) hi = mid;
            else lo = mid + 1;
        }
        auto value = [&](std::size_t k) -> float
        {
            auto const& p = pts[k];
            if (p.kind == kSplinePointConst) return bitsToFloat(p.ref);
            return spline(c, p.ref, x, y, z);
        };
        auto extend = [&](std::size_t k) -> float
        {
            float g = pts[k].derivative;
            float v = value(k);
            return g == 0.0f ? v : v + g * (f - pts[k].location);
        };
        std::int64_t i = static_cast<std::int64_t>(lo) - 1;
        std::int64_t j = static_cast<std::int64_t>(pts.size()) - 1;
        if (i < 0) return extend(0);
        if (i == j) return extend(static_cast<std::size_t>(j));
        auto ii = static_cast<std::size_t>(i);
        float g = pts[ii].location, h = pts[ii + 1].location;
        float k = (f - g) / (h - g);
        float n = value(ii), o = value(ii + 1);
        float l = pts[ii].derivative, m = pts[ii + 1].derivative;
        float p = l * (h - g) - (o - n);
        float q = -m * (h - g) + (o - n);
        return (n + k * (o - n)) + (k * (1.0f - k)) * (p + k * (q - p));
    }

    double SeededVolume::eval(Chunk& c, std::uint32_t i, std::int32_t x, std::int32_t y, std::int32_t z) const
    {
        auto const& fn = mPack->funcs[i];
        double const p0 = fn.p0, p1 = fn.p1;
        switch (static_cast<FuncOp>(fn.op))
        {
        case FuncOp::Const: return p0;
        case FuncOp::Add: return eval(c, fn.a, x, y, z) + eval(c, fn.b, x, y, z);
        case FuncOp::Mul:
        {
            double a = eval(c, fn.a, x, y, z);
            return a == 0.0 ? 0.0 : a * eval(c, fn.b, x, y, z);
        }
        case FuncOp::Min:
        {
            double a = eval(c, fn.a, x, y, z);
            return std::min(a, eval(c, fn.b, x, y, z));
        }
        case FuncOp::Max:
        {
            double a = eval(c, fn.a, x, y, z);
            return std::max(a, eval(c, fn.b, x, y, z));
        }
        case FuncOp::Abs: return std::fabs(eval(c, fn.a, x, y, z));
        case FuncOp::Square:
        {
            double v = eval(c, fn.a, x, y, z);
            return v * v;
        }
        case FuncOp::Cube:
        {
            double v = eval(c, fn.a, x, y, z);
            return v * v * v;
        }
        case FuncOp::HalfNegative:
        {
            double v = eval(c, fn.a, x, y, z);
            return v > 0.0 ? v : v * 0.5;
        }
        case FuncOp::QuarterNegative:
        {
            double v = eval(c, fn.a, x, y, z);
            return v > 0.0 ? v : v * 0.25;
        }
        case FuncOp::Squeeze:
        {
            double v = eval(c, fn.a, x, y, z);
            double cl = std::max(-1.0, std::min(1.0, v));
            return cl / 2.0 - cl * cl * cl / 24.0;
        }
        case FuncOp::Clamp:
        {
            double v = eval(c, fn.a, x, y, z);
            return std::max(p0, std::min(p1, v));
        }
        case FuncOp::RangeChoice:
        {
            double v = eval(c, fn.a, x, y, z);
            return (v >= p0 && v < p1) ? eval(c, fn.b, x, y, z) : eval(c, fn.c, x, y, z);
        }
        case FuncOp::YClampedGradient:
        {
            double fv = mPack->fcon[fn.aux], tv = mPack->fcon[fn.aux + 1];
            double t = (y - p0) / (p1 - p0);
            if (t < 0.0) return fv;
            if (t > 1.0) return tv;
            return fv + t * (tv - fv);
        }
        case FuncOp::NoiseOp: return mSamplers[fn.aux].normal->value(x * p0, y * p1, z * p0);
        case FuncOp::ShiftedNoise:
        {
            double sx = eval(c, fn.a, x, y, z);
            double sy = eval(c, fn.b, x, y, z);
            double sz = eval(c, fn.c, x, y, z);
            return mSamplers[fn.aux].normal->value(x * p0 + sx, y * p1 + sy, z * p0 + sz);
        }
        case FuncOp::Shift: return mSamplers[fn.aux].normal->value(x * 0.25, y * 0.25, z * 0.25) * 4.0;
        case FuncOp::ShiftA: return mSamplers[fn.aux].normal->value(x * 0.25, 0.0, z * 0.25) * 4.0;
        case FuncOp::ShiftB: return mSamplers[fn.aux].normal->value(z * 0.25, x * 0.25, 0.0) * 4.0;
        case FuncOp::SplineOp: return static_cast<double>(spline(c, fn.aux, x, y, z));
        case FuncOp::EndIslands:
            return (static_cast<double>(jr::endIslandHeight(*mSamplers[fn.aux].simplex, x / 8, z / 8)) - 8.0) / 128.0;
        case FuncOp::WeirdScaledSampler:
        {
            double d = eval(c, fn.a, x, y, z);
            double e;
            if (p0 == 0.0) e = d < -0.5 ? 0.75 : d < 0.0 ? 1.0 : d < 0.5 ? 1.5 : 2.0;
            else e = d < -0.75 ? 0.5 : d < -0.5 ? 0.75 : d < 0.5 ? 1.0 : d < 0.75 ? 2.0 : 3.0;
            return e * std::fabs(mSamplers[fn.aux].normal->value(x / e, y / e, z / e));
        }
        case FuncOp::BlendAlpha: return 1.0;
        case FuncOp::BlendOffset: return 0.0;
        case FuncOp::BlendDensity:
        case FuncOp::Cache2D:
        case FuncOp::CacheOnce:
        case FuncOp::CacheAllInCell: return eval(c, fn.a, x, y, z);
        case FuncOp::FlatCache:
        {
            Key k{i, x & ~3, 0, z & ~3};
            auto it = c.flat.find(k);
            if (it != c.flat.end()) return it->second;
            double v = eval(c, fn.a, x & ~3, 0, z & ~3);
            c.flat.emplace(k, v);
            return v;
        }
        case FuncOp::Interpolated:
        {
            std::int32_t cw = mCellW, ch = mCellH;
            std::int32_t cx = floorDiv(x, cw), cy = floorDiv(y, ch), cz = floorDiv(z, cw);
            double tx = double(x - cx * cw) / cw;
            double ty = double(y - cy * ch) / ch;
            double tz = double(z - cz * cw) / cw;
            auto cornerAt = [&](std::int32_t kx, std::int32_t ky, std::int32_t kz) -> double
            {
                Key k{i, kx, ky, kz};
                auto it = c.corner.find(k);
                if (it != c.corner.end()) return it->second;
                double v = eval(c, fn.a, kx * cw, ky * ch, kz * cw);
                c.corner.emplace(k, v);
                return v;
            };
            double v000 = cornerAt(cx, cy, cz), v100 = cornerAt(cx + 1, cy, cz);
            double v010 = cornerAt(cx, cy + 1, cz), v110 = cornerAt(cx + 1, cy + 1, cz);
            double v001 = cornerAt(cx, cy, cz + 1), v101 = cornerAt(cx + 1, cy, cz + 1);
            double v011 = cornerAt(cx, cy + 1, cz + 1), v111 = cornerAt(cx + 1, cy + 1, cz + 1);
            return jr::lerp(tz, jr::lerp(tx, jr::lerp(ty, v000, v010), jr::lerp(ty, v100, v110)),
                            jr::lerp(tx, jr::lerp(ty, v001, v011), jr::lerp(ty, v101, v111)));
        }
        case FuncOp::OldBlendedNoise: return mSamplers[fn.aux].blended->value(x, y, z);
        default: return 0.0;
        }
    }

    std::size_t SeededVolume::pickBiome(Chunk& c, std::int32_t x, std::int32_t y, std::int32_t z) const
    {
        auto const& r = mPack->router;
        std::uint32_t const channels[6] = {r.temperature, r.vegetation, r.continents, r.erosion, r.depth, r.ridges};
        std::int64_t target[6];
        for (int k = 0; k < 6; ++k) target[k] = channels[k] == kNone ? 0 : quantize(eval(c, channels[k], x, y, z));
        std::int64_t best = -1;
        std::size_t bestIdx = 0;
        for (std::size_t i = 0; i < mPack->biomes.size(); ++i)
        {
            auto const& b = mPack->biomes[i];
            float const* iv[6] = {b.temperature, b.humidity, b.continentalness, b.erosion, b.depth, b.weirdness};
            std::int64_t fit = 0;
            for (int k = 0; k < 6; ++k)
            {
                auto d = intervalDistance(quantize(iv[k][0]), quantize(iv[k][1]), target[k]);
                fit += d * d;
            }
            auto o = quantize(b.offset);
            fit += o * o;
            if (best < 0 || fit < best)
            {
                best = fit;
                bestIdx = i;
            }
        }
        return bestIdx;
    }

    std::int32_t SeededVolume::preliminarySurfaceLevel(Chunk& c, std::int32_t x, std::int32_t z) const
    {
        Key k{0, x, 0, z};
        auto it = c.prelim.find(k);
        if (it != c.prelim.end()) return static_cast<std::int32_t>(it->second);
        auto r = mPack->router.initialDensityWithoutJaggedness;
        std::int64_t v = kIntMax;
        if (r != kNone)
        {
            std::int32_t minY = mPack->minY();
            for (std::int32_t y = minY + (mPack->settings.height / mCellH) * mCellH; y >= minY; y -= mCellH)
            {
                if (eval(c, r, x, y, z) > 0.390625)
                {
                    v = y;
                    break;
                }
            }
        }
        c.prelim.emplace(k, v);
        return static_cast<std::int32_t>(v);
    }

    void SeededVolume::generateChunk(std::int32_t chunkX, std::int32_t chunkZ, std::vector<std::uint16_t>& out,
                                     std::vector<std::uint32_t>& biomes) const
    {
        auto const& p = *mPack;
        auto const h = static_cast<std::size_t>(p.settings.height);
        std::int32_t const minY = p.minY();
        Chunk c;
        c.cx = chunkX;
        c.cz = chunkZ;
        c.blocks.assign(256 * h, kMatAir);
        c.biome.assign(256, 0);
        c.surfaceDepth.assign(256, 0);
        c.secondary.assign(256, 0.0);
        c.minSurfaceLevel.assign(256, 0);
        c.minSurfaceKnown.assign(256, false);
        c.heights.assign(256, 0);
        auto fd = p.router.finalDensity;
        for (std::int32_t x = 0; x < 16; ++x)
        {
            std::int32_t wx = chunkX * 16 + x;
            for (std::int32_t z = 0; z < 16; ++z)
            {
                std::int32_t wz = chunkZ * 16 + z;
                auto* col = c.blocks.data() + (std::size_t(x) * 16 + std::size_t(z)) * h;
                for (std::size_t y = 0; y < h; ++y)
                {
                    std::int32_t wy = minY + static_cast<std::int32_t>(y);
                    double d = eval(c, fd, wx, wy, wz);
                    if (d > 0.0) col[y] = kMatDefault;
                    else if (wy < p.settings.seaLevel) col[y] = kMatFluid;
                }
            }
        }
        for (std::size_t i = 0; i < 256; ++i)
        {
            auto const* col = c.blocks.data() + i * h;
            std::int32_t top = 0;
            for (std::int32_t y = static_cast<std::int32_t>(h) - 1; y >= 0; --y)
            {
                if (col[y] != kMatAir)
                {
                    top = y;
                    break;
                }
            }
            c.heights[i] = top;
            std::int32_t wx = chunkX * 16 + static_cast<std::int32_t>(i / 16);
            std::int32_t wz = chunkZ * 16 + static_cast<std::int32_t>(i % 16);
            std::int32_t qy = ((minY + top) >> 2) << 2;
            c.biome[i] = static_cast<std::uint32_t>(pickBiome(c, (wx >> 2) << 2, qy, (wz >> 2) << 2));
        }
        if (p.surfRoot != kNone) applySurface(c);
        out = std::move(c.blocks);
        biomes = std::move(c.biome);
    }

    void SeededVolume::applySurface(Chunk& c) const
    {
        auto const& p = *mPack;
        auto const& s = p.settings;
        auto const h = static_cast<std::int32_t>(s.height);
        std::int32_t const minY = p.minY();
        auto const& rand = mRands[s.surfaceRand];
        auto const& surfaceNoise = *mSamplers[s.surfaceNoise].normal;
        jr::NormalNoise const* secondary = s.surfaceSecondaryNoise == kNone ? nullptr : &*mSamplers[s.surfaceSecondaryNoise].normal;
        for (std::size_t i = 0; i < 256; ++i)
        {
            std::int32_t wx = c.cx * 16 + static_cast<std::int32_t>(i / 16);
            std::int32_t wz = c.cz * 16 + static_cast<std::int32_t>(i % 16);
            c.surfaceDepth[i] = static_cast<std::int32_t>(surfaceNoise.value(wx, 0.0, wz) * 2.75 + 3.0 + rand.at(wx, 0, wz).nextDouble() * 0.25);
            c.secondary[i] = secondary ? secondary->value(wx, 0.0, wz) : 0.0;
            auto* col = c.blocks.data() + i * static_cast<std::size_t>(h);
            std::int32_t n = 0;
            std::int64_t o = kIntMin;
            std::int64_t pp = kIntMax;
            std::int32_t top = minY + c.heights[i] + 1;
            for (std::int32_t wy = top; wy >= minY; --wy)
            {
                std::int32_t y = wy - minY;
                if (y >= h) continue;
                auto block = col[y];
                if (block == kMatAir)
                {
                    n = 0;
                    o = kIntMin;
                }
                else if (block == kMatFluid)
                {
                    if (o == kIntMin) o = wy + 1;
                }
                else
                {
                    if (pp >= wy)
                    {
                        pp = kWayBelowMinY;
                        for (std::int32_t u = wy - 1; u >= minY - 1; --u)
                        {
                            std::int32_t uy = u - minY;
                            auto below = uy >= 0 ? col[uy] : kMatAir;
                            if (below == kMatAir || below == kMatFluid)
                            {
                                pp = u + 1;
                                break;
                            }
                        }
                    }
                    ++n;
                    c.above = n;
                    c.below = static_cast<std::int32_t>(wy - pp + 1);
                    c.water = o;
                    auto r = rule(c, p.surfRoot, i, wy);
                    if (r != kNone) col[y] = static_cast<std::uint16_t>(r);
                }
            }
        }
    }

    std::uint32_t SeededVolume::rule(Chunk& c, std::uint32_t i, std::size_t column, std::int32_t y) const
    {
        auto const& sn = mPack->surf[i];
        switch (static_cast<SurfOp>(sn.op))
        {
        case SurfOp::Block: return mSurfMaterial[i];
        case SurfOp::Sequence:
            for (auto item : mPack->surfLists[sn.aux])
            {
                auto r = rule(c, item, column, y);
                if (r != kNone) return r;
            }
            return kNone;
        case SurfOp::Condition: return cond(c, sn.a, column, y) ? rule(c, sn.b, column, y) : kNone;
        default: return kNone;
        }
    }

    bool SeededVolume::cond(Chunk& c, std::uint32_t i, std::size_t column, std::int32_t y) const
    {
        auto const& p = *mPack;
        auto const& sn = p.surf[i];
        std::int32_t const minY = p.minY();
        auto anchor = [&](std::int32_t v, std::uint16_t kind) -> std::int32_t
        {
            if (kind == kAnchorAbsolute) return v;
            if (kind == kAnchorAboveBottom) return minY + v;
            return minY + p.settings.height - 1 - v;
        };
        std::int32_t wx = c.cx * 16 + static_cast<std::int32_t>(column / 16);
        std::int32_t wz = c.cz * 16 + static_cast<std::int32_t>(column % 16);
        switch (static_cast<SurfOp>(sn.op))
        {
        case SurfOp::Biome:
        {
            auto name = p.biomes[c.biome[column]].biomeStr;
            for (auto it : p.surfLists[sn.aux])
                if (it == name) return true;
            return false;
        }
        case SurfOp::NoiseThreshold:
        {
            double d = mSamplers[sn.aux].normal->value(wx, 0.0, wz);
            return double(sn.p0) <= d && d <= double(sn.p1);
        }
        case SurfOp::VerticalGradient:
        {
            std::int32_t lo = anchor(sn.i0, sn.flags & 3);
            std::int32_t hi = anchor(sn.i1, (sn.flags >> 2) & 3);
            if (y <= lo) return true;
            if (y >= hi) return false;
            double d = double(y - lo) / double(hi - lo);
            d = 1.0 + d * (0.0 - 1.0);
            auto r = mRands[sn.aux].at(wx, y, wz);
            return double(r.nextFloat()) < d;
        }
        case SurfOp::YAbove:
        {
            std::int32_t add = (sn.flags & kSurfAddStoneDepth) ? c.above : 0;
            return std::int64_t(y) + add >= std::int64_t(anchor(sn.i0, sn.flags & 3)) + std::int64_t(c.surfaceDepth[column]) * sn.i1;
        }
        case SurfOp::Water:
        {
            if (c.water == kIntMin) return true;
            std::int32_t add = (sn.flags & kSurfAddStoneDepth) ? c.above : 0;
            return std::int64_t(y) + add >= c.water + sn.i0 + std::int64_t(c.surfaceDepth[column]) * sn.i1;
        }
        case SurfOp::Temperature:
        {
            double t = p.biomes[c.biome[column]].baseTemperature;
            if (y > 80) t = t - (y - 80) * 0.05 / 40.0;
            return t < 0.15;
        }
        case SurfOp::Steep:
        {
            auto lx = static_cast<std::int32_t>(column / 16), lz = static_cast<std::int32_t>(column % 16);
            std::int32_t k = std::max(lz - 1, 0), l = std::min(lz + 1, 15);
            auto hm = [&](std::int32_t ax, std::int32_t az) { return c.heights[static_cast<std::size_t>(ax * 16 + az)]; };
            if (hm(lx, l) >= hm(lx, k) + 4) return true;
            std::int32_t o = std::max(lx - 1, 0), q = std::min(lx + 1, 15);
            return hm(o, lz) >= hm(q, lz) + 4;
        }
        case SurfOp::Not: return !cond(c, sn.a, column, y);
        case SurfOp::Hole: return c.surfaceDepth[column] <= 0;
        case SurfOp::AbovePreliminarySurface:
        {
            if (!c.minSurfaceKnown[column])
            {
                std::int32_t i0 = wx >> 4, j0 = wz >> 4;
                double p00 = preliminarySurfaceLevel(c, i0 << 4, j0 << 4);
                double p10 = preliminarySurfaceLevel(c, (i0 + 1) << 4, j0 << 4);
                double p01 = preliminarySurfaceLevel(c, i0 << 4, (j0 + 1) << 4);
                double p11 = preliminarySurfaceLevel(c, (i0 + 1) << 4, (j0 + 1) << 4);
                double tx = static_cast<float>((wx & 15) / 16.0), tz = static_cast<float>((wz & 15) / 16.0);
                double v = jr::lerp(tz, jr::lerp(tx, p00, p10), jr::lerp(tx, p01, p11));
                c.minSurfaceLevel[column] = static_cast<std::int64_t>(std::floor(double(static_cast<float>(v)))) + c.surfaceDepth[column] - 8;
                c.minSurfaceKnown[column] = true;
            }
            return std::int64_t(y) >= c.minSurfaceLevel[column];
        }
        case SurfOp::StoneDepth:
        {
            std::int32_t depth = (sn.flags & kSurfCeiling) ? c.below : c.above;
            std::int32_t j = (sn.flags & kSurfAddStoneDepth) ? c.surfaceDepth[column] : 0;
            std::int32_t k = sn.i1 == 0 ? 0 : static_cast<std::int32_t>(jr::lerp((c.secondary[column] - -1.0) / 2.0, 0.0, double(sn.i1)));
            return depth <= 1 + sn.i0 + j + k;
        }
        default: return false;
        }
    }
} // namespace pier::dimensions::pack
