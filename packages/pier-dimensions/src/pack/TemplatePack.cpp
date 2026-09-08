/** TemplatePack.cpp: decodes the sections of a PIERTPL file and mounts it.
 * The validation mirrors tools/pier-pack/pierpack/tpl_read.py check for check; a pack
 * the tool accepts and this file refuses is a reader difference and is reported with
 * the section and index in the message, never repaired in place. */
#include "pier/dimensions/pack/template_pack.h"

#include <algorithm>
#include <cstring>
#include <set>

#include "pier/dimensions/pack/expr.h"

namespace pier::dimensions::pack
{
    namespace
    {
        std::string where(char const* sec, std::size_t i) { return std::string(sec) + " " + std::to_string(i) + ": "; }

        bool inRange(std::uint32_t v, std::size_t limit, bool allowNone = false)
        {
            if (v == kNone) return allowNone;
            return v < limit;
        }

        bool readCount(Section const& s, std::uint32_t& count, char const* sec, std::vector<std::string>& problems)
        {
            if (!s.read(0, count))
            {
                problems.push_back(std::string(sec) + " is truncated");
                return false;
            }
            return true;
        }

        bool readVarint(std::uint8_t const* p, std::size_t len, std::size_t& off, std::uint32_t& v)
        {
            v = 0;
            for (int shift = 0; shift <= 28; shift += 7)
            {
                if (off >= len) return false;
                std::uint8_t b = p[off++];
                v |= std::uint32_t(b & 0x7F) << shift;
                if (!(b & 0x80)) return true;
            }
            return false;
        }

        bool decodeCells(Section const& s, Voxel const& e, bool liquid, std::vector<std::uint16_t>& cells,
                         std::size_t paletteSize, std::size_t i, std::vector<std::string>& problems)
        {
            std::size_t n = std::size_t(e.sx) * e.sy * e.sz;
            std::uint64_t off = liquid ? e.liqOff : e.mainOff;
            std::uint64_t len = liquid ? e.liqLen : e.mainLen;
            std::uint64_t colOff = liquid ? e.liqColOff : e.colOff;
            if (off > s.size || len > s.size - off)
            {
                problems.push_back(where("VOXL", i) + "cell data runs past the section");
                return false;
            }
            cells.assign(n, 0);
            if (e.encoding == kVoxelRaw)
            {
                if (len != n * 2)
                {
                    problems.push_back(where("VOXL", i) + "raw data has the wrong length");
                    return false;
                }
                std::memcpy(cells.data(), s.data + off, n * 2);
            }
            else if (e.encoding == kVoxelRle)
            {
                std::size_t cols = std::size_t(e.sx) * e.sz;
                if (colOff > s.size || cols * 4 > s.size - colOff)
                {
                    problems.push_back(where("VOXL", i) + "column table runs past the section");
                    return false;
                }
                std::uint8_t const* data = s.data + off;
                for (std::size_t c = 0; c < cols; ++c)
                {
                    std::uint32_t start = 0;
                    std::memcpy(&start, s.data + colOff + c * 4, 4);
                    std::size_t p = start;
                    std::size_t filled = 0;
                    while (filled < e.sy)
                    {
                        std::uint32_t run = 0, val = 0;
                        if (!readVarint(data, std::size_t(len), p, run) || !readVarint(data, std::size_t(len), p, val)
                            || run == 0 || run > e.sy - filled)
                        {
                            problems.push_back(where("VOXL", i) + "column " + std::to_string(c) + " has a bad run");
                            return false;
                        }
                        std::fill_n(cells.begin() + static_cast<std::ptrdiff_t>(c * e.sy + filled), run, std::uint16_t(val));
                        filled += run;
                    }
                }
            }
            else
            {
                problems.push_back(where("VOXL", i) + "unknown encoding");
                return false;
            }
            for (auto v : cells)
            {
                if (v >= paletteSize)
                {
                    problems.push_back(where("VOXL", i) + "a cell index exceeds the palette");
                    return false;
                }
            }
            return true;
        }

        bool decodeVoxels(PackFile const& f, Section const& s, TemplatePack& t, std::vector<std::string>& problems)
        {
            std::uint32_t n = 0;
            if (!readCount(s, n, "VOXL", problems)) return false;
            std::vector<Voxel> entries;
            if (!s.readArray(4, n, entries))
            {
                problems.push_back("VOXL entries run past the section");
                return false;
            }
            std::size_t total = 0;
            for (auto const& e : entries) total += e.palCount;
            std::vector<std::uint32_t> refs;
            if (!s.readArray(4 + n * sizeof(Voxel), total, refs))
            {
                problems.push_back("VOXL palette references run past the section");
                return false;
            }
            for (std::size_t i = 0; i < entries.size(); ++i)
            {
                auto const& e = entries[i];
                TplVoxel v;
                v.sx = e.sx; v.sy = e.sy; v.sz = e.sz;
                if (e.sx == 0 || e.sy == 0 || e.sz == 0 || std::uint64_t(e.sx) * e.sy * e.sz > (1u << 24))
                {
                    problems.push_back(where("VOXL", i) + "size is empty or above 2^24 cells");
                    return false;
                }
                if (e.palFirst > total || e.palCount > total - e.palFirst || e.palCount < 2)
                {
                    problems.push_back(where("VOXL", i) + "palette out of range");
                    return false;
                }
                for (std::uint32_t k = 0; k < e.palCount; ++k)
                {
                    auto ref = refs[e.palFirst + k];
                    if (!f.validString(ref))
                    {
                        problems.push_back(where("VOXL", i) + "palette string out of range");
                        return false;
                    }
                    v.palette.push_back(f.str(ref));
                }
                if (!v.palette[0].empty() || v.palette[1] != "minecraft:air")
                {
                    problems.push_back(where("VOXL", i) + "palette must start with keep and air");
                    return false;
                }
                if (!decodeCells(s, e, false, v.cells, v.palette.size(), i, problems)) return false;
                if (e.hasLiquid && !decodeCells(s, e, true, v.liquid, v.palette.size(), i, problems)) return false;
                t.voxels.push_back(std::move(v));
            }
            return true;
        }

        bool decodeShapes(Section const& s, TemplatePack& t, std::vector<std::string>& problems)
        {
            std::uint32_t n = 0;
            if (!readCount(s, n, "SHAP", problems)) return false;
            if (!s.readArray(4, n, t.shapes))
            {
                problems.push_back("SHAP nodes run past the section");
                return false;
            }
            std::size_t off = 4 + std::size_t(n) * sizeof(ShapeNode);
            std::uint32_t ln = 0;
            std::vector<ListRef> refs;
            if (!s.read(off, ln) || !s.readArray(off + 4, ln, refs))
            {
                problems.push_back("SHAP choose lists run past the section");
                return false;
            }
            off += 4 + std::size_t(ln) * sizeof(ListRef);
            std::size_t total = 0;
            for (auto const& r : refs) total += r.count;
            std::vector<ChooseEntry> entries;
            if (!s.readArray(off, total, entries))
            {
                problems.push_back("SHAP choose entries run past the section");
                return false;
            }
            for (auto const& r : refs)
            {
                if (r.first > total || r.count > total - r.first || r.count == 0)
                {
                    problems.push_back("SHAP: a choose list is empty or out of range");
                    return false;
                }
                t.chooseLists.emplace_back(entries.begin() + r.first, entries.begin() + r.first + r.count);
            }
            for (std::size_t i = 0; i < t.shapes.size(); ++i)
            {
                auto const& sh = t.shapes[i];
                if (sh.op >= static_cast<std::uint16_t>(ShapeOp::Count_))
                {
                    problems.push_back(where("SHAP", i) + "unknown op " + std::to_string(sh.op));
                    return false;
                }
                auto op = static_cast<ShapeOp>(sh.op);
                bool needA = op == ShapeOp::Translate || op == ShapeOp::RotateY || op == ShapeOp::Mirror
                    || op == ShapeOp::Repeat || op == ShapeOp::Shell || op == ShapeOp::Paint
                    || op == ShapeOp::Union || op == ShapeOp::Difference || op == ShapeOp::Intersect;
                bool needB = op == ShapeOp::Union || op == ShapeOp::Difference || op == ShapeOp::Intersect;
                if ((sh.a != kNone && sh.a >= i) || (sh.b != kNone && sh.b >= i) || (needA && sh.a == kNone) || (needB && sh.b == kNone))
                {
                    problems.push_back(where("SHAP", i) + "operands are not earlier nodes");
                    return false;
                }
                for (int bit = 0; bit < 3; ++bit)
                {
                    if (sh.flags & (1u << bit))
                    {
                        std::uint32_t p = bit == 0 ? std::uint32_t(sh.p0) : bit == 1 ? std::uint32_t(sh.p1) : std::uint32_t(sh.p2);
                        if (!inRange(p, t.expr.size()))
                        {
                            problems.push_back(where("SHAP", i) + "parameter expression out of range");
                            return false;
                        }
                    }
                }
                if (op == ShapeOp::Paint && sh.role >= t.roles.size())
                {
                    problems.push_back(where("SHAP", i) + "role out of range");
                    return false;
                }
                if (op == ShapeOp::Choose)
                {
                    if (!inRange(sh.aux, t.chooseLists.size()))
                    {
                        problems.push_back(where("SHAP", i) + "choose list out of range");
                        return false;
                    }
                    for (auto const& e : t.chooseLists[sh.aux])
                    {
                        if (e.node >= i)
                        {
                            problems.push_back(where("SHAP", i) + "a choose entry is not an earlier node");
                            return false;
                        }
                    }
                    if (sh.p0 == 1)
                    {
                        if (!inRange(std::uint32_t(sh.p1), t.params.size()) || t.params[sh.p1].kind != ParamKind::Choice)
                        {
                            problems.push_back(where("SHAP", i) + "choose by a parameter that is not a choice");
                            return false;
                        }
                        if (t.choiceLists[t.params[sh.p1].aux].size() != t.chooseLists[sh.aux].size())
                        {
                            problems.push_back(where("SHAP", i) + "choose entries do not match the parameter's choices");
                            return false;
                        }
                    }
                    else if (sh.p0 != 0)
                    {
                        problems.push_back(where("SHAP", i) + "unknown choose selector");
                        return false;
                    }
                }
                if (op == ShapeOp::Voxels && !inRange(sh.aux, t.voxels.size()))
                {
                    problems.push_back(where("SHAP", i) + "voxel blob out of range");
                    return false;
                }
            }
            return true;
        }

        bool decodePicks(Section const& s, TemplatePack& t, std::vector<std::string>& problems)
        {
            std::uint32_t n = 0;
            if (!readCount(s, n, "PICK", problems)) return false;
            std::vector<Pick> picks;
            if (!s.readArray(4, n, picks))
            {
                problems.push_back("PICK entries run past the section");
                return false;
            }
            std::size_t total = 0;
            for (auto const& p : picks) total += p.rootCount;
            std::vector<PickRoot> roots;
            if (!s.readArray(4 + std::size_t(n) * sizeof(Pick), total, roots))
            {
                problems.push_back("PICK roots run past the section");
                return false;
            }
            for (std::size_t i = 0; i < picks.size(); ++i)
            {
                auto const& p = picks[i];
                for (auto e : {p.x0Expr, p.z0Expr, p.wExpr, p.dExpr, p.anchorYExpr})
                {
                    if (!inRange(e, t.expr.size()))
                    {
                        problems.push_back(where("PICK", i) + "expression out of range");
                        return false;
                    }
                }
                if (p.rootCount == 0 || p.rootFirst > total || p.rootCount > total - p.rootFirst)
                {
                    problems.push_back(where("PICK", i) + "roots out of range");
                    return false;
                }
                TplPick tp;
                tp.pick = p;
                tp.roots.assign(roots.begin() + p.rootFirst, roots.begin() + p.rootFirst + p.rootCount);
                for (auto const& r : tp.roots)
                {
                    if (!inRange(r.shapeNode, t.shapes.size()) || r.rotMask == 0 || (r.rotMask & ~0xFu))
                    {
                        problems.push_back(where("PICK", i) + "a root has a bad shape or turn mask");
                        return false;
                    }
                }
                t.picks.push_back(std::move(tp));
            }
            return true;
        }
    } // namespace

    std::optional<std::size_t> TemplatePack::paramIndex(std::string const& name) const
    {
        for (std::size_t i = 0; i < params.size(); ++i)
            if (params[i].name == name) return i;
        return std::nullopt;
    }

    std::optional<TemplatePack> decodeTemplate(PackFile const& f, std::vector<std::string>& problems)
    {
        if (f.kind() != PackKind::Template)
        {
            problems.push_back("the file is a volume pack, not a template pack");
            return std::nullopt;
        }
        TemplatePack t;
        t.fileHash = f.fileHash();
        auto need = [&](std::uint32_t tag) -> std::optional<Section>
        {
            auto s = f.section(tag);
            if (!s) problems.push_back("section " + tagName(tag) + " is missing");
            return s;
        };
        auto info = need(kSecInfo);
        auto exprS = need(kSecExpr);
        auto pchc = need(kSecChoices);
        auto parm = need(kSecParams);
        auto role = need(kSecRoles);
        auto zone = need(kSecZones);
        auto stak = need(kSecStacks);
        auto cnst = need(kSecConstraints);
        if (!info || !exprS || !pchc || !parm || !role || !zone || !stak || !cnst) return std::nullopt;

        Info in{};
        if (!info->read(0, in) || !f.validString(in.sourceNameStr) || !f.validString(in.biomeStr))
        {
            problems.push_back("INFO is truncated or names a string out of range");
            return std::nullopt;
        }
        t.sourceName = f.str(in.sourceNameStr);
        t.biome = f.str(in.biomeStr);
        t.heightMin = in.heightMin;
        t.heightMax = in.heightMax;
        t.heightFixed = (in.flags & kInfoHeightFixed) != 0;
        if (t.heightMin % 16 || t.heightMax % 16 || t.heightMin >= t.heightMax)
        {
            problems.push_back("INFO height is not a non-empty range on subchunk boundaries");
            return std::nullopt;
        }

        std::uint32_t n = 0;
        if (!readCount(*exprS, n, "EXPR", problems) || !exprS->readArray(4, n, t.expr))
        {
            problems.push_back("EXPR nodes run past the section");
            return std::nullopt;
        }

        if (!readCount(*pchc, n, "PCHC", problems)) return std::nullopt;
        {
            std::vector<ListRef> refs;
            if (!pchc->readArray(4, n, refs))
            {
                problems.push_back("PCHC lists run past the section");
                return std::nullopt;
            }
            std::size_t total = 0;
            for (auto const& r : refs) total += r.count;
            std::vector<std::int32_t> vals;
            if (!pchc->readArray(4 + std::size_t(n) * sizeof(ListRef), total, vals))
            {
                problems.push_back("PCHC values run past the section");
                return std::nullopt;
            }
            for (auto const& r : refs)
            {
                if (r.first > total || r.count > total - r.first || r.count == 0)
                {
                    problems.push_back("PCHC: a choice list is empty or out of range");
                    return std::nullopt;
                }
                t.choiceLists.emplace_back(vals.begin() + r.first, vals.begin() + r.first + r.count);
            }
        }

        if (!readCount(*parm, n, "PARM", problems)) return std::nullopt;
        {
            std::vector<Param> raw;
            if (!parm->readArray(4, n, raw))
            {
                problems.push_back("PARM entries run past the section");
                return std::nullopt;
            }
            for (std::size_t i = 0; i < raw.size(); ++i)
            {
                auto const& p = raw[i];
                TplParam tp;
                if (!f.validString(p.nameStr) || p.kind > 3)
                {
                    problems.push_back(where("PARM", i) + "bad name or kind");
                    return std::nullopt;
                }
                tp.name = f.str(p.nameStr);
                tp.kind = static_cast<ParamKind>(p.kind);
                tp.def = p.def; tp.min = p.min; tp.max = p.max; tp.step = p.step; tp.aux = p.aux;
                if (tp.kind == ParamKind::Choice && !inRange(p.aux, t.choiceLists.size()))
                {
                    problems.push_back(where("PARM", i) + "choice list out of range");
                    return std::nullopt;
                }
                if (tp.kind == ParamKind::Derived && !inRange(p.aux, t.expr.size()))
                {
                    problems.push_back(where("PARM", i) + "derived expression out of range");
                    return std::nullopt;
                }
                if (tp.kind == ParamKind::Free && p.step < 1)
                {
                    problems.push_back(where("PARM", i) + "step below 1");
                    return std::nullopt;
                }
                for (auto const& o : t.params)
                {
                    if (o.name == tp.name)
                    {
                        problems.push_back(where("PARM", i) + "duplicate name " + tp.name);
                        return std::nullopt;
                    }
                }
                t.params.push_back(std::move(tp));
            }
        }
        if (!validateExpr(t.expr, t.params.size(), problems)) return std::nullopt;
        // A derived parameter may only use parameters declared before it, so that the
        // ordered binding pass is enough; the reachable Param nodes are checked here.
        for (std::size_t i = 0; i < t.params.size(); ++i)
        {
            if (t.params[i].kind != ParamKind::Derived) continue;
            std::vector<std::uint32_t> stack{t.params[i].aux};
            std::set<std::uint32_t> seen;
            while (!stack.empty())
            {
                auto k = stack.back();
                stack.pop_back();
                if (k == kNone || !seen.insert(k).second) continue;
                auto const& e = t.expr[k];
                if (e.op == static_cast<std::uint16_t>(ExprOp::Param) && static_cast<std::size_t>(e.imm) >= i)
                {
                    problems.push_back("PARM " + t.params[i].name + ": a derived parameter uses a parameter declared at or after it");
                    return std::nullopt;
                }
                stack.push_back(e.a);
                stack.push_back(e.b);
            }
        }

        if (!readCount(*role, n, "ROLE", problems)) return std::nullopt;
        {
            std::vector<Role> raw;
            if (!role->readArray(4, n, raw))
            {
                problems.push_back("ROLE entries run past the section");
                return std::nullopt;
            }
            for (std::size_t i = 0; i < raw.size(); ++i)
            {
                if (!f.validString(raw[i].nameStr) || !f.validString(raw[i].defaultBlockStr))
                {
                    problems.push_back(where("ROLE", i) + "string out of range");
                    return std::nullopt;
                }
                t.roles.push_back({f.str(raw[i].nameStr), f.str(raw[i].defaultBlockStr)});
            }
        }

        std::uint32_t zn = 0;
        if (!readCount(*zone, zn, "ZONE", problems)) return std::nullopt;
        {
            if (zn == 0)
            {
                problems.push_back("ZONE: no zones");
                return std::nullopt;
            }
            std::vector<std::uint32_t> names;
            if (!zone->readArray(4, zn, names))
            {
                problems.push_back("ZONE names run past the section");
                return std::nullopt;
            }
            for (auto s : names)
            {
                if (!f.validString(s))
                {
                    problems.push_back("ZONE: a name is out of range");
                    return std::nullopt;
                }
                t.zoneNames.push_back(f.str(s));
            }
            std::size_t off = 4 + std::size_t(zn) * 4;
            std::uint32_t sx = 0, sz = 0;
            if (!zone->read(off, t.periodXExpr) || !zone->read(off + 4, t.periodZExpr)
                || !inRange(t.periodXExpr, t.expr.size()) || !inRange(t.periodZExpr, t.expr.size()))
            {
                problems.push_back("ZONE periods are truncated or out of range");
                return std::nullopt;
            }
            off += 8;
            if (!zone->read(off, sx) || !zone->readArray(off + 4, sx, t.spansX))
            {
                problems.push_back("ZONE x spans run past the section");
                return std::nullopt;
            }
            off += 4 + std::size_t(sx) * sizeof(Span);
            if (!zone->read(off, sz) || !zone->readArray(off + 4, sz, t.spansZ))
            {
                problems.push_back("ZONE z spans run past the section");
                return std::nullopt;
            }
            off += 4 + std::size_t(sz) * sizeof(Span);
            if (t.spansX.empty() || t.spansZ.empty())
            {
                problems.push_back("ZONE: an axis has no spans");
                return std::nullopt;
            }
            for (auto const& s : t.spansX)
            {
                if (!inRange(s.zone, zn) || !inRange(s.lenExpr, t.expr.size()))
                {
                    problems.push_back("ZONE: an x span is out of range");
                    return std::nullopt;
                }
            }
            for (auto const& s : t.spansZ)
            {
                if (!inRange(s.zone, zn) || !inRange(s.lenExpr, t.expr.size()))
                {
                    problems.push_back("ZONE: a z span is out of range");
                    return std::nullopt;
                }
            }
            if (!zone->readArray(off, std::size_t(zn) * zn, t.combine))
            {
                problems.push_back("ZONE combine table runs past the section");
                return std::nullopt;
            }
            for (auto c : t.combine)
            {
                if (!inRange(c, zn))
                {
                    problems.push_back("ZONE: combine names a zone out of range");
                    return std::nullopt;
                }
            }
        }

        if (!readCount(*stak, n, "STAK", problems) || !stak->readArray(4, n, t.stacks))
        {
            problems.push_back("STAK entries run past the section");
            return std::nullopt;
        }
        for (std::size_t i = 0; i < t.stacks.size(); ++i)
        {
            auto const& s = t.stacks[i];
            if (!inRange(s.zone, zn, true) || !inRange(s.fromExpr, t.expr.size()) || !inRange(s.toExpr, t.expr.size())
                || !inRange(s.role, t.roles.size(), true))
            {
                problems.push_back(where("STAK", i) + "index out of range");
                return std::nullopt;
            }
        }

        if (!readCount(*cnst, n, "CNST", problems) || !cnst->readArray(4, n, t.constraints))
        {
            problems.push_back("CNST entries run past the section");
            return std::nullopt;
        }
        for (std::size_t i = 0; i < t.constraints.size(); ++i)
        {
            auto const& c = t.constraints[i];
            if (!inRange(c.expr, t.expr.size()) || !f.validString(c.messageStr))
            {
                problems.push_back(where("CNST", i) + "index out of range");
                return std::nullopt;
            }
            t.constraintMessages.push_back(f.str(c.messageStr));
        }

        if (auto s = f.section(kSecVoxels); s && !decodeVoxels(f, *s, t, problems)) return std::nullopt;
        if (auto s = f.section(kSecShapes); s && !decodeShapes(*s, t, problems)) return std::nullopt;
        if (auto s = f.section(kSecPick); s && !decodePicks(*s, t, problems)) return std::nullopt;
        if (auto s = f.section(kSecConfine))
        {
            Confine c{};
            if (!s->read(0, c) || !inRange(c.cellExpr, t.expr.size()) || !inRange(c.gapExpr, t.expr.size()))
            {
                problems.push_back("CONF is truncated or names an expression out of range");
                return std::nullopt;
            }
            t.confine = c;
        }
        return t;
    }
} // namespace pier::dimensions::pack
