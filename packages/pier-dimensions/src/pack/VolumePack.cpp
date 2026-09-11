/** VolumePack.cpp: decodes the sections of a PIERVOL file with the checks of
 * tools/pier-pack/pierpack/vol_read.py. */
#include "pier/dimensions/pack/volume_pack.h"

#include <cstring>

namespace pier::dimensions::pack
{
    namespace
    {
        bool inRange(std::uint32_t v, std::size_t limit, bool allowNone = false)
        {
            if (v == kNone) return allowNone;
            return v < limit;
        }

        std::string at(char const* sec, std::size_t i) { return std::string(sec) + " " + std::to_string(i) + ": "; }
    } // namespace

    std::optional<VolumePack> decodeVolume(PackFile const& f, std::vector<std::string>& problems)
    {
        if (f.kind() != PackKind::Volume)
        {
            problems.push_back("the file is a template pack, not a volume pack");
            return std::nullopt;
        }
        VolumePack v;
        v.fileHash = f.fileHash();
        for (std::size_t i = 0; i < f.stringCount(); ++i) v.strings.push_back(f.str(static_cast<std::uint32_t>(i)));
        auto need = [&](std::uint32_t tag) -> std::optional<Section>
        {
            auto s = f.section(tag);
            if (!s) problems.push_back("section " + tagName(tag) + " is missing");
            return s;
        };
        auto info = need(kSecInfo), sett = need(kSecSettings), nois = need(kSecNoises), spln = need(kSecSplines);
        auto fcon = need(kSecFloatConsts), func = need(kSecFunctions), rout = need(kSecRouter), surf = need(kSecSurface);
        auto biom = need(kSecBiomes), rand = need(kSecRandoms);
        if (!info || !sett || !nois || !spln || !fcon || !func || !rout || !surf || !biom || !rand) return std::nullopt;

        Info in{};
        if (!info->read(0, in) || !f.validString(in.sourceNameStr) || !sett->read(0, v.settings))
        {
            problems.push_back("INFO or SETT is truncated");
            return std::nullopt;
        }
        v.sourceName = f.str(in.sourceNameStr);
        auto const& s = v.settings;
        if (s.minY % 16 || s.height % 16 || s.height <= 0)
        {
            problems.push_back("SETT: min_y and height must be multiples of 16 with a positive height");
            return std::nullopt;
        }
        if ((s.sizeHorizontal != 1 && s.sizeHorizontal != 2 && s.sizeHorizontal != 4)
            || (s.sizeVertical != 1 && s.sizeVertical != 2 && s.sizeVertical != 4))
        {
            problems.push_back("SETT: size_horizontal and size_vertical must be 1, 2 or 4");
            return std::nullopt;
        }
        // A flag this generator does not act on is refused rather than ignored. The two
        // below have bits in the format and no implementation behind them, so a pack
        // built with them on would generate terrain without the aquifers or the ore veins
        // its author asked for, and nothing would say so.
        if ((s.flags & kSettingsAquifers) != 0)
        {
            problems.push_back("SETT: aquifers_enabled is set and this host does not generate aquifers; "
                               "build the pack without it rather than have the terrain quietly differ");
            return std::nullopt;
        }
        if ((s.flags & kSettingsOreVeins) != 0)
        {
            problems.push_back("SETT: ore_veins_enabled is set and this host does not generate ore veins; "
                               "build the pack without it rather than have the terrain quietly differ");
            return std::nullopt;
        }
        if ((s.flags & ~kSettingsKnown) != 0)
        {
            problems.push_back("SETT: the flags carry a bit this host does not know, so the pack was built "
                               "by a newer tool and what it asks for cannot be honoured");
            return std::nullopt;
        }
        if (in.heightMin != s.minY || in.heightMax != s.minY + s.height)
        {
            problems.push_back("INFO height does not match SETT");
            return std::nullopt;
        }
        if (!f.validString(s.defaultBlockStr) || !f.validString(s.defaultFluidStr))
        {
            problems.push_back("SETT names a string out of range");
            return std::nullopt;
        }
        v.defaultBlock = f.str(s.defaultBlockStr);
        v.defaultFluid = f.str(s.defaultFluidStr);

        std::uint32_t n = 0;
        std::vector<Noise> rawNoise;
        if (!nois->read(0, n) || !nois->readArray(4, n, rawNoise))
        {
            problems.push_back("NOIS entries run past the section");
            return std::nullopt;
        }
        std::size_t total = 0;
        for (auto const& x : rawNoise) total += x.octaveCount;
        std::vector<double> amps;
        if (!nois->readArray(4 + std::size_t(n) * sizeof(Noise), total, amps))
        {
            problems.push_back("NOIS amplitudes run past the section");
            return std::nullopt;
        }
        for (std::size_t i = 0; i < rawNoise.size(); ++i)
        {
            auto const& x = rawNoise[i];
            if (!f.validString(x.nameStr) || x.kind > kNoiseBlended || x.octaveCount == 0 || x.ampFirst > total
                || x.octaveCount > total - x.ampFirst || (x.kind == kNoiseBlended && x.octaveCount != 5))
            {
                problems.push_back(at("NOIS", i) + "bad entry");
                return std::nullopt;
            }
            VolNoise vn;
            vn.name = f.str(x.nameStr);
            vn.firstOctave = x.firstOctave;
            vn.amplitudes.assign(amps.begin() + x.ampFirst, amps.begin() + x.ampFirst + x.octaveCount);
            vn.valueFactor = x.valueFactor;
            vn.kind = x.kind;
            vn.flags = x.flags;
            v.noises.push_back(std::move(vn));
        }

        if (!fcon->read(0, n) || !fcon->readArray(4, n, v.fcon))
        {
            problems.push_back("FCON values run past the section");
            return std::nullopt;
        }
        if (!func->read(0, n) || !func->readArray(4, n, v.funcs))
        {
            problems.push_back("FUNC nodes run past the section");
            return std::nullopt;
        }

        std::vector<Spline> rawSpl;
        if (!spln->read(0, n) || !spln->readArray(4, n, rawSpl))
        {
            problems.push_back("SPLN entries run past the section");
            return std::nullopt;
        }
        total = 0;
        for (auto const& x : rawSpl) total += x.pointCount;
        std::vector<SplinePoint> pts;
        if (!spln->readArray(4 + std::size_t(n) * sizeof(Spline), total, pts))
        {
            problems.push_back("SPLN points run past the section");
            return std::nullopt;
        }
        for (std::size_t i = 0; i < rawSpl.size(); ++i)
        {
            auto const& x = rawSpl[i];
            if (!inRange(x.coordNode, v.funcs.size()) || x.pointCount == 0 || x.pointFirst > total || x.pointCount > total - x.pointFirst)
            {
                problems.push_back(at("SPLN", i) + "coordinate or points out of range");
                return std::nullopt;
            }
            VolSpline vs;
            vs.coordNode = x.coordNode;
            vs.points.assign(pts.begin() + x.pointFirst, pts.begin() + x.pointFirst + x.pointCount);
            for (std::size_t k = 0; k < vs.points.size(); ++k)
            {
                auto const& p = vs.points[k];
                if ((k > 0 && p.location <= vs.points[k - 1].location) || p.kind > kSplinePointSpline
                    || (p.kind == kSplinePointSpline && p.ref >= i))
                {
                    problems.push_back(at("SPLN", i) + "points are not increasing or a nested spline is not earlier");
                    return std::nullopt;
                }
            }
            v.splines.push_back(std::move(vs));
        }

        for (std::size_t i = 0; i < v.funcs.size(); ++i)
        {
            auto const& fn = v.funcs[i];
            if (fn.op >= static_cast<std::uint16_t>(FuncOp::Count_))
            {
                problems.push_back(at("FUNC", i) + "unknown op " + std::to_string(fn.op));
                return std::nullopt;
            }
            auto op = static_cast<FuncOp>(fn.op);
            if ((fn.a != kNone && fn.a >= i) || (fn.b != kNone && fn.b >= i) || (fn.c != kNone && fn.c >= i))
            {
                problems.push_back(at("FUNC", i) + "operands are not earlier nodes");
                return std::nullopt;
            }
            bool needA = op == FuncOp::Add || op == FuncOp::Mul || op == FuncOp::Min || op == FuncOp::Max || op == FuncOp::Abs
                || op == FuncOp::Square || op == FuncOp::Cube || op == FuncOp::HalfNegative || op == FuncOp::QuarterNegative
                || op == FuncOp::Squeeze || op == FuncOp::Clamp || op == FuncOp::RangeChoice || op == FuncOp::ShiftedNoise
                || op == FuncOp::WeirdScaledSampler || op == FuncOp::BlendDensity || op == FuncOp::Interpolated
                || op == FuncOp::FlatCache || op == FuncOp::Cache2D || op == FuncOp::CacheOnce || op == FuncOp::CacheAllInCell;
            bool needB = op == FuncOp::Add || op == FuncOp::Mul || op == FuncOp::Min || op == FuncOp::Max || op == FuncOp::RangeChoice || op == FuncOp::ShiftedNoise;
            bool needC = op == FuncOp::RangeChoice || op == FuncOp::ShiftedNoise;
            if ((needA && fn.a == kNone) || (needB && fn.b == kNone) || (needC && fn.c == kNone))
            {
                problems.push_back(at("FUNC", i) + "missing operand");
                return std::nullopt;
            }
            bool normalNoise = op == FuncOp::NoiseOp || op == FuncOp::ShiftedNoise || op == FuncOp::Shift || op == FuncOp::ShiftA
                || op == FuncOp::ShiftB || op == FuncOp::WeirdScaledSampler;
            if (normalNoise && (!inRange(fn.aux, v.noises.size()) || v.noises[fn.aux].kind != kNoiseNormal))
            {
                problems.push_back(at("FUNC", i) + "needs a normal noise");
                return std::nullopt;
            }
            if (op == FuncOp::EndIslands && (!inRange(fn.aux, v.noises.size()) || v.noises[fn.aux].kind != kNoiseSimplex))
            {
                problems.push_back(at("FUNC", i) + "end_islands needs the simplex noise");
                return std::nullopt;
            }
            if (op == FuncOp::OldBlendedNoise && (!inRange(fn.aux, v.noises.size()) || v.noises[fn.aux].kind != kNoiseBlended))
            {
                problems.push_back(at("FUNC", i) + "old_blended_noise needs a blended noise");
                return std::nullopt;
            }
            if (op == FuncOp::SplineOp && !inRange(fn.aux, v.splines.size()))
            {
                problems.push_back(at("FUNC", i) + "spline out of range");
                return std::nullopt;
            }
            if (op == FuncOp::YClampedGradient && (fn.aux == kNone || std::size_t(fn.aux) + 2 > v.fcon.size()))
            {
                problems.push_back(at("FUNC", i) + "y_clamped_gradient values out of range");
                return std::nullopt;
            }
        }

        if (!rout->read(0, v.router))
        {
            problems.push_back("ROUT is truncated");
            return std::nullopt;
        }
        std::uint32_t channels[16];
        std::memcpy(channels, &v.router, sizeof channels);
        for (int k = 0; k < 15; ++k)
        {
            if (!inRange(channels[k], v.funcs.size(), true))
            {
                problems.push_back("ROUT names a function out of range");
                return std::nullopt;
            }
        }
        if (v.router.finalDensity == kNone)
        {
            problems.push_back("ROUT: final_density is missing");
            return std::nullopt;
        }

        if (!surf->read(0, n) || !surf->readArray(4, n, v.surf))
        {
            problems.push_back("SURF nodes run past the section");
            return std::nullopt;
        }
        std::size_t off = 4 + std::size_t(n) * sizeof(SurfNode);
        std::uint32_t ln = 0;
        std::vector<ListRef> refs;
        if (!surf->read(off, ln) || !surf->readArray(off + 4, ln, refs))
        {
            problems.push_back("SURF lists run past the section");
            return std::nullopt;
        }
        off += 4 + std::size_t(ln) * sizeof(ListRef);
        total = 0;
        for (auto const& r : refs) total += r.count;
        std::vector<std::uint32_t> items;
        if (!surf->readArray(off, total, items))
        {
            problems.push_back("SURF items run past the section");
            return std::nullopt;
        }
        off += total * 4;
        if (!surf->read(off, v.surfRoot))
        {
            problems.push_back("SURF root is missing");
            return std::nullopt;
        }
        for (auto const& r : refs)
        {
            if (r.first > total || r.count > total - r.first)
            {
                problems.push_back("SURF: a list is out of range");
                return std::nullopt;
            }
            v.surfLists.emplace_back(items.begin() + r.first, items.begin() + r.first + r.count);
        }

        if (!rand->read(0, n) || !rand->readArray(4, n, items))
        {
            problems.push_back("RAND names run past the section");
            return std::nullopt;
        }
        for (auto sidx : items)
        {
            if (!f.validString(sidx))
            {
                problems.push_back("RAND names a string out of range");
                return std::nullopt;
            }
            v.rands.push_back(f.str(sidx));
        }

        for (std::size_t i = 0; i < v.surf.size(); ++i)
        {
            auto const& sn = v.surf[i];
            auto op = static_cast<SurfOp>(sn.op);
            bool known = sn.op <= 3 || (sn.op >= 10 && sn.op <= 20);
            if (!known || (sn.a != kNone && sn.a >= i) || (sn.b != kNone && sn.b >= i))
            {
                problems.push_back(at("SURF", i) + "unknown op or operands not earlier");
                return std::nullopt;
            }
            if (op == SurfOp::Bandlands)
            {
                problems.push_back(at("SURF", i) + "bandlands is not supported");
                return std::nullopt;
            }
            if (op == SurfOp::Block && !f.validString(sn.aux))
            {
                problems.push_back(at("SURF", i) + "block name out of range");
                return std::nullopt;
            }
            if (op == SurfOp::Sequence || op == SurfOp::Biome)
            {
                if (!inRange(sn.aux, v.surfLists.size()))
                {
                    problems.push_back(at("SURF", i) + "list out of range");
                    return std::nullopt;
                }
                for (auto it : v.surfLists[sn.aux])
                {
                    if ((op == SurfOp::Sequence && it >= i) || (op == SurfOp::Biome && !f.validString(it)))
                    {
                        problems.push_back(at("SURF", i) + "a list item is out of range");
                        return std::nullopt;
                    }
                }
            }
            if ((op == SurfOp::Condition && (sn.a == kNone || sn.b == kNone)) || (op == SurfOp::Not && sn.a == kNone))
            {
                problems.push_back(at("SURF", i) + "missing operand");
                return std::nullopt;
            }
            if (op == SurfOp::NoiseThreshold && (!inRange(sn.aux, v.noises.size()) || v.noises[sn.aux].kind != kNoiseNormal))
            {
                problems.push_back(at("SURF", i) + "noise out of range");
                return std::nullopt;
            }
            if (op == SurfOp::VerticalGradient && !inRange(sn.aux, v.rands.size()))
            {
                problems.push_back(at("SURF", i) + "random out of range");
                return std::nullopt;
            }
        }
        if (!inRange(v.surfRoot, v.surf.size(), true))
        {
            problems.push_back("SURF root out of range");
            return std::nullopt;
        }
        if (v.surfRoot != kNone)
        {
            if (!inRange(s.surfaceRand, v.rands.size()) || !inRange(s.surfaceNoise, v.noises.size())
                || !inRange(s.surfaceSecondaryNoise, v.noises.size(), true))
            {
                problems.push_back("SETT: the surface random or noises are out of range");
                return std::nullopt;
            }
        }

        if (!biom->read(0, n) || !biom->readArray(4, n, v.biomes) || v.biomes.empty())
        {
            problems.push_back("BIOM is empty or runs past the section");
            return std::nullopt;
        }
        for (auto const& b : v.biomes)
        {
            if (!f.validString(b.biomeStr))
            {
                problems.push_back("BIOM names a string out of range");
                return std::nullopt;
            }
            v.biomeNames.push_back(f.str(b.biomeStr));
        }
        return v;
    }
} // namespace pier::dimensions::pack
