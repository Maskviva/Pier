/** TemplateGen.cpp: evaluates the shape DAG and fills one chunk of materials.
 * A material is a small integer into MountedTemplate::materials; a shape evaluation
 * returns kOutside or a material. The recursion follows refgen_tpl.eval_shape: box
 * pruning first, then the operator, with RotateY and Mirror mapping the point back into
 * the child's frame around the child's own bounding box. Cell selection uses cellHash
 * and weighted_pick from the tool with the same constants. */
#include "pier/dimensions/pack/template_pack.h"

#include <algorithm>

namespace pier::dimensions::pack
{
    namespace
    {
        constexpr std::int32_t kOutside = -1;

        std::uint32_t mix32(std::uint32_t h)
        {
            h ^= h >> 16;
            h *= 0x85EBCA6Bu;
            h ^= h >> 13;
            h *= 0xC2B2AE35u;
            h ^= h >> 16;
            return h;
        }

        std::size_t weightedPick(std::uint32_t h, std::vector<PickRoot> const& roots)
        {
            std::uint64_t total = 0;
            for (auto const& r : roots) total += r.weight;
            if (total == 0) return 0;
            std::uint64_t v = h % total;
            for (std::size_t i = 0; i < roots.size(); ++i)
            {
                if (v < roots[i].weight) return i;
                v -= roots[i].weight;
            }
            return roots.size() - 1;
        }

        std::size_t weightedPick(std::uint32_t h, std::vector<ChooseEntry> const& entries)
        {
            std::uint64_t total = 0;
            for (auto const& e : entries) total += e.weight;
            if (total == 0) return 0;
            std::uint64_t v = h % total;
            for (std::size_t i = 0; i < entries.size(); ++i)
            {
                if (v < entries[i].weight) return i;
                v -= entries[i].weight;
            }
            return entries.size() - 1;
        }

        std::int32_t floorDiv(std::int32_t a, std::int32_t b)
        {
            std::int32_t q = a / b;
            if ((a % b != 0) && ((a < 0) != (b < 0))) --q;
            return q;
        }

        std::int32_t posMod(std::int32_t a, std::int32_t m)
        {
            std::int32_t r = a % m;
            return r < 0 ? r + m : r;
        }

        struct Ctx
        {
            MountedTemplate const& m;
            std::int32_t cellX;
            std::int32_t cellZ;
        };

        std::int32_t eval(Ctx const& c, std::uint32_t i, std::int32_t x, std::int32_t y, std::int32_t z)
        {
            auto const& pack = *c.m.pack;
            auto const& s = pack.shapes[i];
            auto const& p = c.m.shapeParams[i];
            auto const& bb = c.m.shapeBox[i];
            if (!bb) return kOutside;
            if (x < bb->x0 || x >= bb->x1 || y < bb->y0 || y >= bb->y1 || z < bb->z0 || z >= bb->z1) return kOutside;
            switch (static_cast<ShapeOp>(s.op))
            {
            case ShapeOp::Box: return 0;
            case ShapeOp::Cylinder:
            {
                std::int64_t r = p[0];
                std::int64_t dx = x - r, dz = z - r;
                return dx * dx + dz * dz <= r * r ? 0 : kOutside;
            }
            case ShapeOp::Wedge: return std::int64_t(y) * p[0] < std::int64_t(x + 1) * p[1] ? 0 : kOutside;
            case ShapeOp::Translate:
                return eval(c, s.a, x - std::int32_t(p[0]), y - std::int32_t(p[1]), z - std::int32_t(p[2]));
            case ShapeOp::RotateY:
            {
                auto const& a = *c.m.shapeBox[s.a];
                std::int32_t w = a.x1 - a.x0, d = a.z1 - a.z0;
                std::int32_t u = x - a.x0, v = z - a.z0;
                std::int32_t n = posMod(std::int32_t(p[0]), 4);
                std::int32_t cx, cz;
                if (n == 0) { cx = a.x0 + u; cz = a.z0 + v; }
                else if (n == 1) { cx = a.x0 + v; cz = a.z0 + (d - 1 - u); }
                else if (n == 2) { cx = a.x0 + (w - 1 - u); cz = a.z0 + (d - 1 - v); }
                else { cx = a.x0 + (w - 1 - v); cz = a.z0 + u; }
                return eval(c, s.a, cx, y, cz);
            }
            case ShapeOp::Mirror:
            {
                auto const& a = *c.m.shapeBox[s.a];
                if (p[0] == 0) return eval(c, s.a, a.x0 + a.x1 - 1 - x, y, z);
                return eval(c, s.a, x, y, a.z0 + a.z1 - 1 - z);
            }
            case ShapeOp::Repeat:
            {
                auto const& a = *c.m.shapeBox[s.a];
                auto period = std::int32_t(p[0]);
                auto count = std::int32_t(p[1]);
                auto axis = std::int32_t(p[2]);
                if (period <= 0) return eval(c, s.a, x, y, z);
                std::int32_t coord = axis == 0 ? x : axis == 1 ? y : z;
                std::int32_t base = axis == 0 ? a.x0 : axis == 1 ? a.y0 : a.z0;
                std::int32_t k = floorDiv(coord - base, period);
                if (k < 0 || k >= count) return kOutside;
                if (axis == 0) x -= k * period;
                else if (axis == 1) y -= k * period;
                else z -= k * period;
                return eval(c, s.a, x, y, z);
            }
            case ShapeOp::Union:
            {
                auto r = eval(c, s.a, x, y, z);
                return r != kOutside ? r : eval(c, s.b, x, y, z);
            }
            case ShapeOp::Difference:
            {
                auto r = eval(c, s.a, x, y, z);
                if (r == kOutside) return kOutside;
                return eval(c, s.b, x, y, z) != kOutside ? kOutside : r;
            }
            case ShapeOp::Intersect:
            {
                auto r = eval(c, s.a, x, y, z);
                if (r == kOutside) return kOutside;
                return eval(c, s.b, x, y, z) != kOutside ? r : kOutside;
            }
            case ShapeOp::Shell:
            {
                auto r = eval(c, s.a, x, y, z);
                if (r == kOutside) return kOutside;
                std::int32_t t = std::max<std::int32_t>(1, std::int32_t(p[0]));
                for (std::int32_t dist = 1; dist <= t; ++dist)
                {
                    if (eval(c, s.a, x + dist, y, z) == kOutside || eval(c, s.a, x - dist, y, z) == kOutside
                        || eval(c, s.a, x, y + dist, z) == kOutside || eval(c, s.a, x, y - dist, z) == kOutside
                        || eval(c, s.a, x, y, z + dist) == kOutside || eval(c, s.a, x, y, z - dist) == kOutside)
                        return r;
                }
                return kOutside;
            }
            case ShapeOp::Paint:
                return eval(c, s.a, x, y, z) == kOutside ? kOutside : c.m.roleMaterial[s.role];
            case ShapeOp::Choose:
            {
                auto const& entries = pack.chooseLists[s.aux];
                std::size_t idx;
                if (p[0] == 1)
                {
                    auto const& lst = pack.choiceLists[pack.params[std::size_t(p[1])].aux];
                    auto pv = c.m.values[std::size_t(p[1])];
                    idx = 0;
                    for (std::size_t k = 0; k < lst.size(); ++k)
                        if (lst[k] == pv) { idx = k; break; }
                }
                else idx = weightedPick(cellHash(c.cellX, c.cellZ, std::uint32_t(p[1])), entries);
                return eval(c, entries[idx].node, x, y, z);
            }
            case ShapeOp::Voxels:
            {
                auto const& v = pack.voxels[s.aux];
                auto cell = v.cells[(std::size_t(x) * v.sz + std::size_t(z)) * v.sy + std::size_t(y)];
                if (cell == kVoxelKeep) return kOutside;
                if (cell == kVoxelAir) return 0;
                return c.m.voxelMaterial[s.aux][cell];
            }
            default: return kOutside;
            }
        }

        IBox rotatedBox(IBox const& bb, std::int32_t n, std::int32_t w, std::int32_t d)
        {
            n = posMod(n, 4);
            if (n == 0) return bb;
            std::int32_t const xs[4] = {bb.x0, bb.x1 - 1, bb.x0, bb.x1 - 1};
            std::int32_t const zs[4] = {bb.z0, bb.z0, bb.z1 - 1, bb.z1 - 1};
            std::int32_t u0 = 0, u1 = 0, v0 = 0, v1 = 0;
            for (int k = 0; k < 4; ++k)
            {
                std::int32_t u, v;
                if (n == 1) { u = d - 1 - zs[k]; v = xs[k]; }
                else if (n == 2) { u = w - 1 - xs[k]; v = d - 1 - zs[k]; }
                else { u = zs[k]; v = w - 1 - xs[k]; }
                if (k == 0) { u0 = u1 = u; v0 = v1 = v; }
                u0 = std::min(u0, u); u1 = std::max(u1, u);
                v0 = std::min(v0, v); v1 = std::max(v1, v);
            }
            return IBox{u0, bb.y0, v0, u1 + 1, bb.y1, v1 + 1};
        }

        std::int32_t chooseTurn(std::uint32_t mask, std::uint32_t h, std::int32_t w, std::int32_t d)
        {
            std::int32_t allowed[4];
            int n = 0;
            for (int t = 0; t < 4; ++t)
            {
                if (!(mask & (1u << t))) continue;
                if (w != d && (t % 2) == 1) continue;
                allowed[n++] = t;
            }
            if (n == 0) return 0;
            return allowed[h % static_cast<std::uint32_t>(n)];
        }

        void stamp(MountedTemplate const& m, MountedPick const& pick, std::int32_t gx, std::int32_t gz, IBox const& chunk,
                   std::vector<std::uint16_t>& out)
        {
            auto h = cellHash(gx, gz, pick.salt);
            auto const& root = pick.roots[weightedPick(h, pick.roots)];
            auto const& bb = m.shapeBox[root.shapeNode];
            if (!bb) return;
            auto turn = chooseTurn(root.rotMask, mix32(h ^ 0x5BD1E995u), pick.w, pick.d);
            IBox rb = rotatedBox(*bb, turn, pick.w, pick.d);
            std::int32_t ox = gx * m.periodX + pick.x0;
            std::int32_t oz = gz * m.periodZ + pick.z0;
            std::int32_t oy = pick.anchorY;
            IBox hit{std::max(rb.x0 + ox, chunk.x0), std::max(rb.y0 + oy, chunk.y0), std::max(rb.z0 + oz, chunk.z0),
                     std::min(rb.x1 + ox, chunk.x1), std::min(rb.y1 + oy, chunk.y1), std::min(rb.z1 + oz, chunk.z1)};
            if (hit.empty()) return;
            Ctx ctx{m, gx, gz};
            auto const height = static_cast<std::size_t>(m.height());
            for (std::int32_t wx = hit.x0; wx < hit.x1; ++wx)
            {
                for (std::int32_t wz = hit.z0; wz < hit.z1; ++wz)
                {
                    std::int32_t u = wx - ox, v = wz - oz, cu, cv;
                    if (turn == 0) { cu = u; cv = v; }
                    else if (turn == 1) { cu = v; cv = pick.d - 1 - u; }
                    else if (turn == 2) { cu = pick.w - 1 - u; cv = pick.d - 1 - v; }
                    else { cu = pick.w - 1 - v; cv = u; }
                    auto* col = out.data() + (static_cast<std::size_t>(wx - chunk.x0) * 16 + static_cast<std::size_t>(wz - chunk.z0)) * height;
                    for (std::int32_t wy = hit.y0; wy < hit.y1; ++wy)
                    {
                        auto r = eval(ctx, root.shapeNode, cu, wy - oy, cv);
                        if (r != kOutside) col[wy - m.minY] = static_cast<std::uint16_t>(r);
                    }
                }
            }
        }
    } // namespace

    std::uint32_t cellHash(std::int32_t cx, std::int32_t cz, std::uint32_t salt)
    {
        std::uint32_t a = static_cast<std::uint32_t>(cx) * 0x9E3779B1u;
        std::uint32_t b = static_cast<std::uint32_t>(cz) * 0x85EBCA77u;
        return mix32(a ^ mix32(b ^ salt));
    }

    void generateTemplateChunk(MountedTemplate const& m, std::int32_t chunkX, std::int32_t chunkZ,
                               std::vector<std::uint16_t>& out)
    {
        auto const height = static_cast<std::size_t>(m.height());
        auto const zn = m.pack->zoneCount();
        out.resize(256 * height);
        for (std::int32_t x = 0; x < 16; ++x)
        {
            std::int32_t wx = chunkX * 16 + x;
            auto zx = m.zoneOfX[static_cast<std::size_t>(posMod(wx, m.periodX))];
            for (std::int32_t z = 0; z < 16; ++z)
            {
                std::int32_t wz = chunkZ * 16 + z;
                auto zz = m.zoneOfZ[static_cast<std::size_t>(posMod(wz, m.periodZ))];
                auto const& col = m.zoneColumns[m.pack->combine[zx * zn + zz]];
                std::copy(col.begin(), col.end(), out.begin() + static_cast<std::ptrdiff_t>((std::size_t(x) * 16 + std::size_t(z)) * height));
            }
        }
        IBox chunk{chunkX * 16, m.minY, chunkZ * 16, chunkX * 16 + 16, m.maxY, chunkZ * 16 + 16};
        for (auto const& pick : m.picks)
        {
            std::int32_t gx0 = floorDiv(chunk.x0 - pick.x0 - 512, m.periodX);
            std::int32_t gx1 = floorDiv(chunk.x1 - pick.x0 + 512, m.periodX);
            std::int32_t gz0 = floorDiv(chunk.z0 - pick.z0 - 512, m.periodZ);
            std::int32_t gz1 = floorDiv(chunk.z1 - pick.z0 + 512, m.periodZ);
            for (std::int32_t gx = gx0; gx <= gx1; ++gx)
                for (std::int32_t gz = gz0; gz <= gz1; ++gz) stamp(m, pick, gx, gz, chunk, out);
        }
    }
} // namespace pier::dimensions::pack
