/** TemplateMount.cpp: binds one dimension's parameters and roles to a template pack.
 * The steps and their order are those of refgen_tpl.mount in the tool: bind, evaluate,
 * constraints, height, periods and spans, roles to materials, zone columns, static
 * layers, shape parameters, bounding boxes, picks. Every refusal names the parameter or
 * constraint; a value outside its range is never clamped, since the dimension would
 * then persist with terrain the caller did not choose. */
#include "pier/dimensions/pack/template_pack.h"

#include <algorithm>

#include "pier/dimensions/pack/expr.h"

namespace pier::dimensions::pack
{
    namespace
    {
        bool bindParams(TemplatePack const& pack, std::map<std::string, std::int64_t> const& given,
                        std::vector<std::int64_t>& values, std::vector<std::int64_t>& vals,
                        std::vector<std::string>& problems)
        {
            values.assign(pack.params.size(), 0);
            bool ok = true;
            for (auto const& [name, v] : given)
            {
                if (!pack.paramIndex(name))
                {
                    problems.push_back("unknown parameter '" + name + "'");
                    ok = false;
                }
            }
            for (std::size_t i = 0; i < pack.params.size(); ++i)
            {
                auto const& p = pack.params[i];
                auto it = given.find(p.name);
                if (p.kind == ParamKind::Derived)
                {
                    if (it != given.end())
                    {
                        problems.push_back("parameter " + p.name + " is derived by the pack and cannot be given");
                        ok = false;
                    }
                    continue;
                }
                std::int64_t v = it == given.end() ? p.def : it->second;
                if (p.kind == ParamKind::Fixed && v != p.def)
                {
                    problems.push_back("parameter " + p.name + " is fixed at " + std::to_string(p.def) + " by the pack; "
                                       + std::to_string(v) + " was given");
                    ok = false;
                }
                if (p.kind == ParamKind::Free)
                {
                    if (v < p.min || v > p.max)
                    {
                        problems.push_back("parameter " + p.name + "=" + std::to_string(v) + " lies outside ["
                                           + std::to_string(p.min) + ", " + std::to_string(p.max) + "]");
                        ok = false;
                    }
                    else if (p.step > 1 && v % p.step != 0)
                    {
                        problems.push_back("parameter " + p.name + "=" + std::to_string(v) + " is not a multiple of "
                                           + std::to_string(p.step));
                        ok = false;
                    }
                }
                if (p.kind == ParamKind::Choice)
                {
                    auto const& lst = pack.choiceLists[p.aux];
                    if (std::find(lst.begin(), lst.end(), static_cast<std::int32_t>(v)) == lst.end() || v != static_cast<std::int32_t>(v))
                    {
                        problems.push_back("parameter " + p.name + "=" + std::to_string(v) + " is not one of its choices");
                        ok = false;
                    }
                }
                values[i] = v;
            }
            if (!ok) return false;
            if (!evaluateExpr(pack.expr, values, vals, problems)) return false;
            for (std::size_t i = 0; i < pack.params.size(); ++i)
            {
                if (pack.params[i].kind != ParamKind::Derived) continue;
                values[i] = vals[pack.params[i].aux];
                if (!evaluateExpr(pack.expr, values, vals, problems)) return false;
            }
            return true;
        }

        std::uint16_t material(MountedTemplate& m, std::string const& block)
        {
            for (std::size_t i = 0; i < m.materials.size(); ++i)
                if (m.materials[i] == block) return static_cast<std::uint16_t>(i);
            m.materials.push_back(block);
            return static_cast<std::uint16_t>(m.materials.size() - 1);
        }

        bool expandSpans(std::vector<Span> const& spans, std::vector<std::int64_t> const& vals, std::int64_t period,
                         char const* axis, std::vector<std::uint32_t>& out, std::vector<std::string>& problems)
        {
            out.clear();
            for (auto const& s : spans)
            {
                auto len = vals[s.lenExpr];
                if (len < 0)
                {
                    problems.push_back(std::string("a ") + axis + " span has negative length " + std::to_string(len));
                    return false;
                }
                if (static_cast<std::int64_t>(out.size()) + len > period) break;
                out.insert(out.end(), static_cast<std::size_t>(len), s.zone);
            }
            if (static_cast<std::int64_t>(out.size()) != period)
            {
                problems.push_back(std::string("the ") + axis + " spans do not add up to the " + axis + " period");
                return false;
            }
            return true;
        }

        std::optional<IBox> boxOf(MountedTemplate const& m, std::size_t i)
        {
            auto const& pack = *m.pack;
            auto const& s = pack.shapes[i];
            auto const& p = m.shapeParams[i];
            auto const a = s.a == kNone ? std::nullopt : m.shapeBox[s.a];
            auto const b = s.b == kNone ? std::nullopt : m.shapeBox[s.b];
            auto i32 = [](std::int64_t v) { return static_cast<std::int32_t>(v); };
            auto join = [](std::optional<IBox> const& x, std::optional<IBox> const& y) -> std::optional<IBox>
            {
                if (!x) return y;
                if (!y) return x;
                return IBox{std::min(x->x0, y->x0), std::min(x->y0, y->y0), std::min(x->z0, y->z0),
                            std::max(x->x1, y->x1), std::max(x->y1, y->y1), std::max(x->z1, y->z1)};
            };
            switch (static_cast<ShapeOp>(s.op))
            {
            case ShapeOp::Nothing: return std::nullopt;
            case ShapeOp::Box:
            case ShapeOp::Wedge:
                if (p[0] <= 0 || p[1] <= 0 || p[2] <= 0) return std::nullopt;
                return IBox{0, 0, 0, i32(p[0]), i32(p[1]), i32(p[2])};
            case ShapeOp::Cylinder:
                if (p[0] < 0 || p[1] <= 0) return std::nullopt;
                return IBox{0, 0, 0, i32(2 * p[0] + 1), i32(p[1]), i32(2 * p[0] + 1)};
            case ShapeOp::Translate:
                if (!a) return std::nullopt;
                return IBox{a->x0 + i32(p[0]), a->y0 + i32(p[1]), a->z0 + i32(p[2]),
                            a->x1 + i32(p[0]), a->y1 + i32(p[1]), a->z1 + i32(p[2])};
            case ShapeOp::RotateY:
                if (!a) return std::nullopt;
                if (((p[0] % 4) + 4) % 4 % 2 == 1)
                    return IBox{a->x0, a->y0, a->z0, a->x0 + (a->z1 - a->z0), a->y1, a->z0 + (a->x1 - a->x0)};
                return a;
            case ShapeOp::Mirror:
            case ShapeOp::Shell:
            case ShapeOp::Paint:
            case ShapeOp::Difference:
                return a;
            case ShapeOp::Repeat:
            {
                if (!a || p[1] < 1) return std::nullopt;
                auto ext = p[0] * (p[1] - 1);
                if (ext < 0) return std::nullopt;
                IBox r = *a;
                if (p[2] == 0) r.x1 += i32(ext);
                else if (p[2] == 1) r.y1 += i32(ext);
                else r.z1 += i32(ext);
                return r;
            }
            case ShapeOp::Union: return join(a, b);
            case ShapeOp::Intersect:
            {
                if (!a || !b) return std::nullopt;
                IBox r{std::max(a->x0, b->x0), std::max(a->y0, b->y0), std::max(a->z0, b->z0),
                       std::min(a->x1, b->x1), std::min(a->y1, b->y1), std::min(a->z1, b->z1)};
                if (r.empty()) return std::nullopt;
                return r;
            }
            case ShapeOp::Choose:
            {
                std::optional<IBox> out;
                for (auto const& e : pack.chooseLists[s.aux]) out = join(out, m.shapeBox[e.node]);
                return out;
            }
            case ShapeOp::Voxels:
            {
                auto const& v = pack.voxels[s.aux];
                return IBox{0, 0, 0, i32(v.sx), i32(v.sy), i32(v.sz)};
            }
            default: return std::nullopt;
            }
        }
    } // namespace

    std::optional<MountedTemplate> mountTemplate(TemplatePack const& pack,
                                                 std::map<std::string, std::int64_t> const& params,
                                                 std::map<std::string, std::string> const& roles,
                                                 std::int32_t minY, std::int32_t maxY,
                                                 std::vector<std::string>& problems,
                                                 MountFailure* failure)
    {
        MountFailure local = MountFailure::None;
        MountFailure& why = failure ? *failure : local;
        why = MountFailure::None;
        MountedTemplate m;
        m.pack = &pack;
        if (!bindParams(pack, params, m.values, m.vals, problems))
        {
            why = MountFailure::Params;
            return std::nullopt;
        }
        bool ok = true;
        for (std::size_t i = 0; i < pack.constraints.size(); ++i)
        {
            if (m.vals[pack.constraints[i].expr] == 0)
            {
                problems.push_back(pack.constraintMessages[i]);
                ok = false;
            }
        }
        if (!ok)
        {
            why = MountFailure::Constraint;
            return std::nullopt;
        }
        why = MountFailure::Height;
        if (pack.heightFixed)
        {
            if (minY != pack.heightMin || maxY != pack.heightMax)
            {
                problems.push_back("the pack fixes its height range to [" + std::to_string(pack.heightMin) + ", "
                                   + std::to_string(pack.heightMax) + ")");
                return std::nullopt;
            }
        }
        else if (minY > pack.heightMin || maxY < pack.heightMax)
        {
            problems.push_back("the dimension height does not contain the range the pack was made for");
            return std::nullopt;
        }
        why = MountFailure::Constraint;
        m.minY = minY;
        m.maxY = maxY;
        auto px = m.vals[pack.periodXExpr], pz = m.vals[pack.periodZExpr];
        if (px <= 0 || pz <= 0 || px > (1 << 20) || pz > (1 << 20))
        {
            problems.push_back("a period is not positive or exceeds 2^20");
            return std::nullopt;
        }
        m.periodX = static_cast<std::int32_t>(px);
        m.periodZ = static_cast<std::int32_t>(pz);
        if (!expandSpans(pack.spansX, m.vals, px, "x", m.zoneOfX, problems)) return std::nullopt;
        if (!expandSpans(pack.spansZ, m.vals, pz, "z", m.zoneOfZ, problems)) return std::nullopt;
        for (auto const& [name, block] : roles)
        {
            bool known = false;
            for (auto const& r : pack.roles) known = known || r.name == name;
            if (!known)
            {
                problems.push_back("unknown role '" + name + "'");
                ok = false;
            }
        }
        if (!ok)
        {
            why = MountFailure::Params;
            return std::nullopt;
        }
        m.materials.push_back("minecraft:air");
        for (auto const& r : pack.roles)
        {
            auto it = roles.find(r.name);
            m.roleMaterial.push_back(material(m, it == roles.end() ? r.defaultBlock : it->second));
        }
        for (auto const& v : pack.voxels)
        {
            std::vector<std::uint16_t> pal;
            pal.push_back(0);
            pal.push_back(0);
            for (std::size_t i = 2; i < v.palette.size(); ++i) pal.push_back(material(m, v.palette[i]));
            m.voxelMaterial.push_back(std::move(pal));
        }
        auto const h = static_cast<std::size_t>(maxY - minY);
        std::size_t const zn = pack.zoneCount();
        m.zoneColumns.assign(zn, std::vector<std::uint16_t>(h, 0));
        for (std::size_t zone = 0; zone < zn; ++zone)
        {
            auto& col = m.zoneColumns[zone];
            for (auto const& s : pack.stacks)
            {
                if (s.zone != kNone && s.zone != zone) continue;
                auto lo = std::max<std::int64_t>(m.vals[s.fromExpr], minY);
                auto hi = std::min<std::int64_t>(m.vals[s.toExpr], maxY);
                std::uint16_t mat = s.role == kNone ? 0 : m.roleMaterial[s.role];
                for (auto y = lo; y < hi; ++y) col[static_cast<std::size_t>(y - minY)] = mat;
            }
        }
        m.staticLayer.assign(h, 1);
        for (std::size_t y = 0; y < h; ++y)
        {
            for (std::size_t zone = 1; zone < zn; ++zone)
            {
                if (m.zoneColumns[zone][y] != m.zoneColumns[0][y])
                {
                    m.staticLayer[y] = 0;
                    break;
                }
            }
        }
        m.shapeParams.resize(pack.shapes.size());
        m.shapeBox.resize(pack.shapes.size());
        for (std::size_t i = 0; i < pack.shapes.size(); ++i)
        {
            auto const& s = pack.shapes[i];
            m.shapeParams[i] = {
                (s.flags & kShapeP0IsExpr) ? m.vals[std::uint32_t(s.p0)] : s.p0,
                (s.flags & kShapeP1IsExpr) ? m.vals[std::uint32_t(s.p1)] : s.p1,
                (s.flags & kShapeP2IsExpr) ? m.vals[std::uint32_t(s.p2)] : s.p2,
            };
            m.shapeBox[i] = boxOf(m, i);
        }
        for (auto const& p : pack.picks)
        {
            MountedPick mp;
            mp.salt = p.pick.salt;
            mp.x0 = static_cast<std::int32_t>(m.vals[p.pick.x0Expr]);
            mp.z0 = static_cast<std::int32_t>(m.vals[p.pick.z0Expr]);
            mp.w = static_cast<std::int32_t>(m.vals[p.pick.wExpr]);
            mp.d = static_cast<std::int32_t>(m.vals[p.pick.dExpr]);
            mp.anchorY = static_cast<std::int32_t>(m.vals[p.pick.anchorYExpr]);
            if (mp.w <= 0 || mp.d <= 0)
            {
                problems.push_back("a pick rectangle is empty with these parameters");
                return std::nullopt;
            }
            mp.roots = p.roots;
            m.picks.push_back(std::move(mp));
        }
        if (pack.confine)
        {
            auto cell = m.vals[pack.confine->cellExpr];
            auto gap = m.vals[pack.confine->gapExpr];
            if (cell < 1 || gap < 0 || gap > 64 || cell + gap != px || cell + gap != pz)
            {
                problems.push_back("confine: cell + gap must equal both periods, with a gap of at most 64");
                return std::nullopt;
            }
            m.confine = std::make_pair(static_cast<std::int32_t>(cell), static_cast<std::int32_t>(gap));
        }
        why = MountFailure::None;
        return m;
    }
} // namespace pier::dimensions::pack
