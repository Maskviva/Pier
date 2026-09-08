"""refgen_vol.py: the reference generator of a volume pack, in plain Python.

`seed_noises` builds every sampler from the world seed the way Java's RandomState does.
`Evaluator` computes a density function at a block: flat_cache samples at the quart
origin with y = 0, interpolated samples the cell corners and lerps, every other wrapper
is transparent. `generate_chunk` fills 16 by 16 by height block names: default block
where final density is positive, fluid below sea level, air above; then the biome per
column, then the surface rules in Java's column walk. The C++ generator mirrors this.
"""
from __future__ import annotations

import math
from dataclasses import dataclass, field

from . import format as F
from . import java_noise as J
from .container import PackError
from .vol_read import VolumePack

AIR = "minecraft:air"
INT_MIN = -(1 << 31)
INT_MAX = (1 << 31) - 1
WAY_BELOW_MIN_Y = -(1 << 30)  # DimensionType.WAY_BELOW_MIN_Y is Integer.MIN_VALUE >> 1


def f32(x):
    return J.f32(x)


def seed_noises(pack: VolumePack, seed: int) -> dict:
    """Every sampler keyed by NOIS index, plus the positional factories by RAND index."""
    legacy = bool(pack.settings["flags"] & F.SETTINGS_LEGACY_RANDOM)
    root = J.LegacyRandom(seed) if legacy else J.Xoroshiro.from_seed(seed)
    factory = root.fork_positional()
    out = {}
    for i, n in enumerate(pack.noises):
        name = pack.strings[n["name_str"]]
        amps = pack.amps[n["amp_first"]:n["amp_first"] + n["octave_count"]]
        if n["kind"] == F.NOISE_NORMAL:
            if n["flags"] & F.NOISE_LEGACY_BIOME:
                idx = n["flags"] >> F.NOISE_LEGACY_INDEX_SHIFT
                out[i] = J.NormalNoise.create(J.LegacyRandom(seed + idx), n["first_octave"], amps, legacy=True)
            else:
                out[i] = J.NormalNoise.create(factory.from_hash_of(name), n["first_octave"], amps)
        elif n["kind"] == F.NOISE_SIMPLEX:
            r = J.LegacyRandom(seed)
            r.consume(17292)
            out[i] = J.NoiseTable(r)
        else:
            out[i] = J.BlendedNoise.create(factory.from_hash_of("minecraft:terrain"), *amps)
    rands = {}
    for i, s in enumerate(pack.rands):
        rands[i] = factory.from_hash_of(pack.strings[s]).fork_positional()
    return {"noises": out, "rands": rands}


def rarity_2d(v: float) -> float:
    if v < -0.75:
        return 0.5
    if v < -0.5:
        return 0.75
    if v < 0.5:
        return 1.0
    return 2.0 if v < 0.75 else 3.0


def rarity_3d(v: float) -> float:
    if v < -0.5:
        return 0.75
    if v < 0.0:
        return 1.0
    return 1.5 if v < 0.5 else 2.0


class Evaluator:
    def __init__(self, pack: VolumePack, seeded: dict):
        self.pack = pack
        self.noises = seeded["noises"]
        self.cell_w = 4 * pack.settings["size_horizontal"]
        self.cell_h = 4 * pack.settings["size_vertical"]
        self.corner_memo: dict = {}
        self.flat_memo: dict = {}

    def spline(self, si: int, x: int, y: int, z: int) -> float:
        sp = self.pack.splines[si]
        pts = self.pack.spline_points[sp["point_first"]:sp["point_first"] + sp["point_count"]]
        f = f32(self.eval(sp["coord_node"], x, y, z))
        locs = [f32(p["location"]) for p in pts]
        # Mth.binarySearch(0, n, i -> f < locs[i]) - 1
        lo, hi = 0, len(locs)
        while lo < hi:
            mid = (lo + hi) >> 1
            if f < locs[mid]:
                hi = mid
            else:
                lo = mid + 1
        i = lo - 1
        j = len(locs) - 1

        def value(k):
            p = pts[k]
            if p["kind"] == F.SPLINE_POINT_CONST:
                import struct
                return struct.unpack("<f", struct.pack("<I", p["ref"]))[0]
            return self.spline(p["ref"], x, y, z)

        def extend(k):
            g = f32(pts[k]["derivative"])
            v = value(k)
            return v if g == 0.0 else f32(v + f32(g * f32(f - locs[k])))

        if i < 0:
            return extend(0)
        if i == j:
            return extend(j)
        g, h = locs[i], locs[i + 1]
        k = f32(f32(f - g) / f32(h - g))
        n, o = value(i), value(i + 1)
        l, m = f32(pts[i]["derivative"]), f32(pts[i + 1]["derivative"])
        p = f32(f32(l * f32(h - g)) - f32(o - n))
        q = f32(f32(-m * f32(h - g)) + f32(o - n))
        return f32(f32(n + f32(k * f32(o - n))) + f32(f32(k * f32(1.0 - k)) * f32(p + f32(k * f32(q - p)))))

    def eval(self, i: int, x: int, y: int, z: int) -> float:
        fn = self.pack.funcs[i]
        op = F.FUNC_OPS[fn["op"]]
        if op == "const":
            return fn["p0"]
        if op == "add":
            return self.eval(fn["a"], x, y, z) + self.eval(fn["b"], x, y, z)
        if op == "mul":
            a = self.eval(fn["a"], x, y, z)
            return 0.0 if a == 0.0 else a * self.eval(fn["b"], x, y, z)
        if op == "min":
            a = self.eval(fn["a"], x, y, z)
            return min(a, self.eval(fn["b"], x, y, z))
        if op == "max":
            a = self.eval(fn["a"], x, y, z)
            return max(a, self.eval(fn["b"], x, y, z))
        if op == "abs":
            return abs(self.eval(fn["a"], x, y, z))
        if op == "square":
            v = self.eval(fn["a"], x, y, z)
            return v * v
        if op == "cube":
            v = self.eval(fn["a"], x, y, z)
            return v * v * v
        if op == "half_negative":
            v = self.eval(fn["a"], x, y, z)
            return v if v > 0.0 else v * 0.5
        if op == "quarter_negative":
            v = self.eval(fn["a"], x, y, z)
            return v if v > 0.0 else v * 0.25
        if op == "squeeze":
            v = self.eval(fn["a"], x, y, z)
            c = max(-1.0, min(1.0, v))
            return c / 2.0 - c * c * c / 24.0
        if op == "clamp":
            v = self.eval(fn["a"], x, y, z)
            return max(fn["p0"], min(fn["p1"], v))
        if op == "range_choice":
            v = self.eval(fn["a"], x, y, z)
            return self.eval(fn["b"], x, y, z) if (v >= fn["p0"] and v < fn["p1"]) else self.eval(fn["c"], x, y, z)
        if op == "y_clamped_gradient":
            fy, ty = fn["p0"], fn["p1"]
            fv, tv = self.pack.fcon[fn["aux"]], self.pack.fcon[fn["aux"] + 1]
            t = (y - fy) / (ty - fy)
            if t < 0.0:
                return fv
            if t > 1.0:
                return tv
            return fv + t * (tv - fv)
        if op == "noise":
            return self.noises[fn["aux"]].value(x * fn["p0"], y * fn["p1"], z * fn["p0"])
        if op == "shifted_noise":
            sx = self.eval(fn["a"], x, y, z)
            sy = self.eval(fn["b"], x, y, z)
            sz = self.eval(fn["c"], x, y, z)
            return self.noises[fn["aux"]].value(x * fn["p0"] + sx, y * fn["p1"] + sy, z * fn["p0"] + sz)
        if op == "shift":
            return self.noises[fn["aux"]].value(x * 0.25, y * 0.25, z * 0.25) * 4.0
        if op == "shift_a":
            return self.noises[fn["aux"]].value(x * 0.25, 0.0, z * 0.25) * 4.0
        if op == "shift_b":
            return self.noises[fn["aux"]].value(z * 0.25, x * 0.25, 0.0) * 4.0
        if op == "spline":
            return float(self.spline(fn["aux"], x, y, z))
        if op == "end_islands":
            t = self.noises[fn["aux"]]
            return (f32(J.end_island_height(t, int(x / 8), int(z / 8))) - 8.0) / 128.0
        if op == "weird_scaled_sampler":
            d = self.eval(fn["a"], x, y, z)
            e = rarity_3d(d) if fn["p0"] == 0.0 else rarity_2d(d)
            return e * abs(self.noises[fn["aux"]].value(x / e, y / e, z / e))
        if op == "blend_alpha":
            return 1.0
        if op == "blend_offset":
            return 0.0
        if op in ("blend_density", "cache_2d", "cache_once", "cache_all_in_cell"):
            return self.eval(fn["a"], x, y, z)
        if op == "flat_cache":
            key = (i, x & ~3, z & ~3)
            v = self.flat_memo.get(key)
            if v is None:
                v = self.eval(fn["a"], x & ~3, 0, z & ~3)
                self.flat_memo[key] = v
            return v
        if op == "interpolated":
            return self.interpolate(i, fn["a"], x, y, z)
        if op == "old_blended_noise":
            return self.noises[fn["aux"]].value(x, y, z)
        raise PackError(op)

    def corner(self, i: int, child: int, cx: int, cy: int, cz: int) -> float:
        key = (i, cx, cy, cz)
        v = self.corner_memo.get(key)
        if v is None:
            v = self.eval(child, cx * self.cell_w, cy * self.cell_h, cz * self.cell_w)
            self.corner_memo[key] = v
        return v

    def interpolate(self, i: int, child: int, x: int, y: int, z: int) -> float:
        cw, ch = self.cell_w, self.cell_h
        cx, cy, cz = x // cw, y // ch, z // cw
        tx = (x - cx * cw) / cw
        ty = (y - cy * ch) / ch
        tz = (z - cz * cw) / cw
        c = self.corner
        v000 = c(i, child, cx, cy, cz)
        v100 = c(i, child, cx + 1, cy, cz)
        v010 = c(i, child, cx, cy + 1, cz)
        v110 = c(i, child, cx + 1, cy + 1, cz)
        v001 = c(i, child, cx, cy, cz + 1)
        v101 = c(i, child, cx + 1, cy, cz + 1)
        v011 = c(i, child, cx, cy + 1, cz + 1)
        v111 = c(i, child, cx + 1, cy + 1, cz + 1)
        # Java's NoiseInterpolator: lerp along y first, then x, then z.
        return J.lerp(tz, J.lerp(tx, J.lerp(ty, v000, v010), J.lerp(ty, v100, v110)),
                      J.lerp(tx, J.lerp(ty, v001, v011), J.lerp(ty, v101, v111)))


def quantize(f: float) -> int:
    return int(f * 10000.0)


def interval_distance(lo: int, hi: int, v: int) -> int:
    if v > hi:
        return v - hi
    if v < lo:
        return lo - v
    return 0


def pick_biome(pack: VolumePack, target: tuple) -> int:
    """Index of the BIOM entry with the least fitness, Java's brute-force findValue."""
    best, best_i = None, 0
    for i, b in enumerate(pack.biomes):
        fit = 0
        for k, (lo, hi) in enumerate((("t_lo", "t_hi"), ("h_lo", "h_hi"), ("c_lo", "c_hi"), ("e_lo", "e_hi"), ("d_lo", "d_hi"), ("w_lo", "w_hi"))):
            d = interval_distance(quantize(b[lo]), quantize(b[hi]), target[k])
            fit += d * d
        o = quantize(b["offset"])
        fit += o * o
        if best is None or fit < best:
            best, best_i = fit, i
    return best_i


@dataclass
class Column:
    blocks: list
    biome: int = 0
    surface_depth: int = 0
    secondary: float = 0.0
    min_surface_level: int | None = None


class Generator:
    def __init__(self, pack: VolumePack, seed: int):
        self.pack = pack
        self.seed = seed
        self.seeded = seed_noises(pack, seed)
        self.ev = Evaluator(pack, self.seeded)
        s = pack.settings
        self.min_y = s["min_y"]
        self.height = s["height"]
        self.sea_level = s["sea_level"]
        self.default_block = pack.strings[s["default_block_str"]]
        self.default_fluid = pack.strings[s["default_fluid_str"]]
        self.prelim_memo: dict = {}

    def climate(self, x: int, y: int, z: int) -> tuple:
        r = self.pack.router
        out = []
        for k in ("temperature", "vegetation", "continents", "erosion", "depth", "ridges"):
            out.append(quantize(f32(self.ev.eval(r[k], x, y, z))) if r[k] != F.NONE else 0)
        return tuple(out)

    def preliminary_surface_level(self, x: int, z: int) -> int:
        key = (x, z)
        v = self.prelim_memo.get(key)
        if v is not None:
            return v
        r = self.pack.router["initial_density_without_jaggedness"]
        ch = self.ev.cell_h
        v = INT_MAX
        if r != F.NONE:
            y = self.min_y + (self.height // ch) * ch
            while y >= self.min_y:
                if self.ev.eval(r, x, y, z) > 0.390625:
                    v = y
                    break
                y -= ch
        self.prelim_memo[key] = v
        return v

    def generate_chunk(self, cx: int, cz: int) -> list:
        """256 columns, x major then z, each `height` block names from min_y."""
        h = self.height
        cols = []
        fd = self.pack.router["final_density"]
        for x in range(16):
            wx = cx * 16 + x
            for z in range(16):
                wz = cz * 16 + z
                col = [AIR] * h
                for y in range(h):
                    wy = self.min_y + y
                    d = self.ev.eval(fd, wx, wy, wz)
                    if d > 0.0:
                        col[y] = self.default_block
                    elif wy < self.sea_level:
                        col[y] = self.default_fluid
                cols.append(Column(col))
        # Biome per column at its surface, quart aligned like Java's sampler.
        for i, c in enumerate(cols):
            x, z = divmod(i, 16)
            wx, wz = cx * 16 + x, cz * 16 + z
            top = self.top_of(c.blocks)
            qy = ((self.min_y + top) >> 2) << 2
            c.biome = pick_biome(self.pack, self.climate((wx >> 2) << 2, qy, (wz >> 2) << 2))
        if self.pack.surf_root != F.NONE:
            self.apply_surface(cx, cz, cols)
        return [c.blocks for c in cols], [c.biome for c in cols]

    def top_of(self, blocks: list) -> int:
        """Index of the highest non-air block, or 0."""
        for y in range(len(blocks) - 1, -1, -1):
            if blocks[y] != AIR:
                return y
        return 0

    def heightmap(self, cols: list) -> list:
        return [self.top_of(c.blocks) for c in cols]

    # Surface rules

    def apply_surface(self, cx: int, cz: int, cols: list):
        pack = self.pack
        s = pack.settings
        rand = self.seeded["rands"][s["surface_rand"]]
        surface_noise = self.seeded["noises"][s["surface_noise"]]
        secondary = self.seeded["noises"][s["surface_secondary_noise"]] if s["surface_secondary_noise"] != F.NONE else None
        heights = self.heightmap(cols)
        for i, c in enumerate(cols):
            x, z = divmod(i, 16)
            wx, wz = cx * 16 + x, cz * 16 + z
            c.surface_depth = int(surface_noise.value(wx, 0.0, wz) * 2.75 + 3.0 + rand.at(wx, 0, wz).next_double() * 0.25)
            c.secondary = secondary.value(wx, 0.0, wz) if secondary is not None else 0.0
            ctx = {"x": wx, "z": wz, "col": c, "cols": cols, "heights": heights, "lx": x, "lz": z}
            n = 0  # stone depth above
            o = INT_MIN  # water height
            p = INT_MAX
            top = self.min_y + heights[i] + 1
            for wy in range(top, self.min_y - 1, -1):
                y = wy - self.min_y
                if y >= len(c.blocks):
                    continue
                block = c.blocks[y]
                if block == AIR:
                    n = 0
                    o = INT_MIN
                elif block == self.default_fluid:
                    if o == INT_MIN:
                        o = wy + 1
                else:
                    if p >= wy:
                        p = WAY_BELOW_MIN_Y
                        for u in range(wy - 1, self.min_y - 2, -1):
                            uy = u - self.min_y
                            below = c.blocks[uy] if uy >= 0 else AIR
                            if below == AIR or below == self.default_fluid:
                                p = u + 1
                                break
                    n += 1
                    depth_below = wy - p + 1
                    ctx.update(y=wy, above=n, below=depth_below, water=o)
                    r = self.rule(pack.surf_root, ctx)
                    if r is not None:
                        c.blocks[y] = r

    def anchor(self, v: int, kind: int) -> int:
        if kind == F.ANCHOR_ABSOLUTE:
            return v
        if kind == F.ANCHOR_ABOVE_BOTTOM:
            return self.min_y + v
        return self.min_y + self.height - 1 - v

    def min_surface_level(self, ctx) -> int:
        c = ctx["col"]
        if c.min_surface_level is None:
            x, z = ctx["x"], ctx["z"]
            i, j = x >> 4, z >> 4
            p00 = self.preliminary_surface_level(i << 4, j << 4)
            p10 = self.preliminary_surface_level((i + 1) << 4, j << 4)
            p01 = self.preliminary_surface_level(i << 4, (j + 1) << 4)
            p11 = self.preliminary_surface_level((i + 1) << 4, (j + 1) << 4)
            tx, tz = f32((x & 15) / 16.0), f32((z & 15) / 16.0)
            v = J.lerp(tz, J.lerp(tx, float(p00), float(p10)), J.lerp(tx, float(p01), float(p11)))
            c.min_surface_level = math.floor(f32(v)) + c.surface_depth - 8
        return c.min_surface_level

    def rule(self, i: int, ctx):
        pack = self.pack
        sn = pack.surf[i]
        op = F.SURF_OP_NAMES[sn["op"]]
        if op == "block":
            return pack.strings[sn["aux"]]
        if op == "sequence":
            for item in pack.surf_lists[sn["aux"]]:
                r = self.rule(item, ctx)
                if r is not None:
                    return r
            return None
        if op == "condition":
            return self.rule(sn["b"], ctx) if self.cond(sn["a"], ctx) else None
        raise PackError("surface rule " + op)

    def cond(self, i: int, ctx) -> bool:
        pack = self.pack
        sn = pack.surf[i]
        op = F.SURF_OP_NAMES[sn["op"]]
        c = ctx["col"]
        if op == "biome":
            return pack.biomes[c.biome]["biome_str"] in pack.surf_lists[sn["aux"]]
        if op == "noise_threshold":
            d = self.seeded["noises"][sn["aux"]].value(ctx["x"], 0.0, ctx["z"])
            return sn["p0"] <= d <= sn["p1"]
        if op == "vertical_gradient":
            lo = self.anchor(sn["i0"], sn["flags"] & 3)
            hi = self.anchor(sn["i1"], (sn["flags"] >> 2) & 3)
            y = ctx["y"]
            if y <= lo:
                return True
            if y >= hi:
                return False
            d = (y - lo) / (hi - lo)
            d = 1.0 + d * (0.0 - 1.0)
            r = self.seeded["rands"][sn["aux"]].at(ctx["x"], y, ctx["z"])
            return r.next_float() < d
        if op == "y_above":
            add = ctx["above"] if sn["flags"] & F.SURF_ADD_STONE_DEPTH else 0
            return ctx["y"] + add >= self.anchor(sn["i0"], sn["flags"] & 3) + c.surface_depth * sn["i1"]
        if op == "water":
            if ctx["water"] == INT_MIN:
                return True
            add = ctx["above"] if sn["flags"] & F.SURF_ADD_STONE_DEPTH else 0
            return ctx["y"] + add >= ctx["water"] + sn["i0"] + c.surface_depth * sn["i1"]
        if op == "temperature":
            t = pack.biomes[c.biome]["base_temperature"]
            y = ctx["y"]
            if y > 80:
                t = t - (y - 80) * 0.05 / 40.0
            return t < 0.15
        if op == "steep":
            lx, lz, hm = ctx["lx"], ctx["lz"], ctx["heights"]
            k, l = max(lz - 1, 0), min(lz + 1, 15)
            if hm[lx * 16 + l] >= hm[lx * 16 + k] + 4:
                return True
            o, p = max(lx - 1, 0), min(lx + 1, 15)
            return hm[o * 16 + lz] >= hm[p * 16 + lz] + 4
        if op == "not":
            return not self.cond(sn["a"], ctx)
        if op == "hole":
            return c.surface_depth <= 0
        if op == "above_preliminary_surface":
            return ctx["y"] >= self.min_surface_level(ctx)
        if op == "stone_depth":
            depth = ctx["below"] if sn["flags"] & F.SURF_CEILING else ctx["above"]
            j = c.surface_depth if sn["flags"] & F.SURF_ADD_STONE_DEPTH else 0
            k = 0 if sn["i1"] == 0 else int(J.lerp((c.secondary - -1.0) / 2.0, 0.0, float(sn["i1"])))
            return depth <= 1 + sn["i0"] + j + k
        raise PackError("surface condition " + op)
