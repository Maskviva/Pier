"""refgen_tpl.py: the reference generator of a template pack, in plain Python.

`mount` binds parameters and roles, evaluates the expression table, checks the
constraints, expands the spans into per-offset zone tables and the stacks into one block
column per combined zone, and computes shape bounding boxes. `generate_chunk` fills a
16 by 16 by height array of block names for one chunk: the zone columns first, then
every pick whose rotated root touches the chunk. The C++ generator implements the same
steps in the same order, and the tests compare its output against this one.
"""
from __future__ import annotations

from dataclasses import dataclass, field

from . import format as F
from .container import PackError
from .expr import evaluate, floordiv
from .tpl_read import TemplatePack

AIR = "minecraft:air"
MASK32 = 0xFFFFFFFF


def mix32(h: int) -> int:
    """MurmurHash3 fmix32; the C++ side uses the same constants."""
    h &= MASK32
    h ^= h >> 16
    h = (h * 0x85EBCA6B) & MASK32
    h ^= h >> 13
    h = (h * 0xC2B2AE35) & MASK32
    h ^= h >> 16
    return h


def cell_hash(cx: int, cz: int, salt: int) -> int:
    a = (cx & MASK32) * 0x9E3779B1 & MASK32
    b = (cz & MASK32) * 0x85EBCA77 & MASK32
    return mix32(a ^ mix32(b ^ (salt & MASK32)))


def weighted_pick(h: int, weights: list) -> int:
    total = sum(weights)
    if total <= 0:
        return 0
    r = h % total
    for i, w in enumerate(weights):
        if r < w:
            return i
        r -= w
    return len(weights) - 1


def posmod(v: int, m: int) -> int:
    return v % m


@dataclass
class Box:
    x0: int
    y0: int
    z0: int
    x1: int
    y1: int
    z1: int

    def empty(self):
        return self.x0 >= self.x1 or self.y0 >= self.y1 or self.z0 >= self.z1

    @staticmethod
    def union(a, b):
        if a is None:
            return b
        if b is None:
            return a
        return Box(min(a.x0, b.x0), min(a.y0, b.y0), min(a.z0, b.z0), max(a.x1, b.x1), max(a.y1, b.y1), max(a.z1, b.z1))

    @staticmethod
    def isect(a, b):
        if a is None or b is None:
            return None
        r = Box(max(a.x0, b.x0), max(a.y0, b.y0), max(a.z0, b.z0), min(a.x1, b.x1), min(a.y1, b.y1), min(a.z1, b.z1))
        return None if r.empty() else r


@dataclass
class Resolved:
    pack: TemplatePack
    values: list
    vals: list
    min_y: int
    max_y: int
    period_x: int
    period_z: int
    zone_of_x: list
    zone_of_z: list
    role_blocks: list
    zone_columns: list
    static_layers: set
    shape_params: list
    shape_bbox: list
    picks: list = field(default_factory=list)
    confine: tuple | None = None

    @property
    def height(self):
        return self.max_y - self.min_y


class MountError(Exception):
    pass


def bind_params(pack: TemplatePack, given: dict) -> tuple[list, list]:
    values = [0] * len(pack.params)
    for i, p in enumerate(pack.params):
        if p["kind"] == "derived":
            if p["name"] in given:
                raise MountError(f"parameter {p['name']} is derived by the pack and cannot be given")
            continue
        v = given.get(p["name"], p["default"])
        if not isinstance(v, int) or isinstance(v, bool):
            raise MountError(f"parameter {p['name']}: value {v!r} is not an integer")
        if p["kind"] == "fixed" and v != p["default"]:
            raise MountError(f"parameter {p['name']} is fixed at {p['default']} by the pack; {v} was given")
        if p["kind"] == "free":
            if not (p["min"] <= v <= p["max"]):
                raise MountError(f"parameter {p['name']}={v} lies outside [{p['min']}, {p['max']}]")
            if p["step"] > 1 and v % p["step"]:
                raise MountError(f"parameter {p['name']}={v} is not a multiple of {p['step']}")
        if p["kind"] == "choice" and v not in pack.choice_lists[p["aux"]]:
            raise MountError(f"parameter {p['name']}={v} is not one of {pack.choice_lists[p['aux']]}")
        values[i] = v
    unknown = set(given) - {p["name"] for p in pack.params}
    if unknown:
        raise MountError("unknown parameters: " + ", ".join(sorted(unknown)))
    vals = evaluate(pack.expr, values)
    for i, p in enumerate(pack.params):
        if p["kind"] == "derived":
            values[i] = vals[p["aux"]]
            vals = evaluate(pack.expr, values)
    return values, vals


def mount(pack: TemplatePack, params: dict, roles: dict, min_y: int | None = None, max_y: int | None = None) -> Resolved:
    values, vals = bind_params(pack, params)
    for c in pack.constraints:
        if vals[c["expr"]] == 0:
            raise MountError(c["message"])
    if pack.info["height_fixed"]:
        if (min_y is not None and min_y != pack.info["height_min"]) or (max_y is not None and max_y != pack.info["height_max"]):
            raise MountError("the pack fixes its height range")
        min_y, max_y = pack.info["height_min"], pack.info["height_max"]
    else:
        min_y = pack.info["height_min"] if min_y is None else min_y
        max_y = pack.info["height_max"] if max_y is None else max_y
        if min_y > pack.info["height_min"] or max_y < pack.info["height_max"]:
            raise MountError("the dimension height does not contain the range the pack was made for")
    px, pz = vals[pack.period_x], vals[pack.period_z]
    if px <= 0 or pz <= 0:
        raise MountError("a period is not positive")
    zone_of_x = _expand(pack.spans_x, vals, px, "x")
    zone_of_z = _expand(pack.spans_z, vals, pz, "z")
    unknown = set(roles) - {r["name"] for r in pack.roles}
    if unknown:
        raise MountError("unknown roles: " + ", ".join(sorted(unknown)))
    role_blocks = [roles.get(r["name"], r["block"]) for r in pack.roles]
    n = pack.zone_count
    height = max_y - min_y
    columns = []
    for zone in range(n):
        col = [AIR] * height
        for s in pack.stacks:
            if s["zone"] != F.NONE and s["zone"] != zone:
                continue
            lo = max(vals[s["from_expr"]], min_y)
            hi = min(vals[s["to_expr"]], max_y)
            block = AIR if s["role"] == F.NONE else role_blocks[s["role"]]
            for y in range(lo, hi):
                col[y - min_y] = block
        columns.append(col)
    static_layers = {y for y in range(height) if all(c[y] == columns[0][y] for c in columns)}
    shape_params = [_shape_params(pack, s, vals) for s in pack.shapes]
    bbox = []
    for i, s in enumerate(pack.shapes):
        bbox.append(_bbox(pack, i, shape_params[i], bbox))
    res = Resolved(pack=pack, values=values, vals=vals, min_y=min_y, max_y=max_y, period_x=px, period_z=pz,
                   zone_of_x=zone_of_x, zone_of_z=zone_of_z, role_blocks=role_blocks, zone_columns=columns,
                   static_layers=static_layers, shape_params=shape_params, shape_bbox=bbox)
    if pack.confine is not None:
        cell, gap = vals[pack.confine["cell_expr"]], vals[pack.confine["gap_expr"]]
        if cell < 1 or gap < 0 or gap > 64 or cell + gap != px or cell + gap != pz:
            raise MountError("confine: cell + gap must equal both periods, with a gap of at most 64")
        res.confine = (cell, gap)
    for p in pack.picks:
        w, d = vals[p["w_expr"]], vals[p["d_expr"]]
        if w <= 0 or d <= 0:
            raise MountError("a pick rectangle is empty")
        res.picks.append({"salt": p["salt"], "x0": vals[p["x0_expr"]], "z0": vals[p["z0_expr"]], "w": w, "d": d,
                          "anchor_y": vals[p["anchor_y_expr"]], "roots": p["roots"]})
    return res


def _expand(spans, vals, period, axis):
    out = []
    for s in spans:
        ln = vals[s["len_expr"]]
        if ln < 0:
            raise MountError(f"a {axis} span has negative length {ln}")
        out += [s["zone"]] * ln
    if len(out) != period:
        raise MountError(f"the {axis} spans sum to {len(out)}, the period is {period}")
    return out


def _shape_params(pack, s, vals):
    p = []
    for bit, key in ((1, "p0"), (2, "p1"), (4, "p2")):
        p.append(vals[s[key]] if s["flags"] & bit else s[key])
    return p


def _bbox(pack, i, p, boxes):
    s = pack.shapes[i]
    op = F.SHAPE_OPS[s["op"]]
    a = boxes[s["a"]] if s["a"] != F.NONE else None
    b = boxes[s["b"]] if s["b"] != F.NONE else None
    if op == "nothing":
        return None
    if op == "box" or op == "wedge":
        return None if min(p) <= 0 else Box(0, 0, 0, p[0], p[1], p[2])
    if op == "cylinder":
        r, h = p[0], p[1]
        return None if r < 0 or h <= 0 else Box(0, 0, 0, 2 * r + 1, h, 2 * r + 1)
    if op == "translate":
        return None if a is None else Box(a.x0 + p[0], a.y0 + p[1], a.z0 + p[2], a.x1 + p[0], a.y1 + p[1], a.z1 + p[2])
    if op == "rotate_y":
        if a is None:
            return None
        if p[0] % 2 == 1:
            return Box(a.x0, a.y0, a.z0, a.x0 + (a.z1 - a.z0), a.y1, a.z0 + (a.x1 - a.x0))
        return Box(a.x0, a.y0, a.z0, a.x1, a.y1, a.z1)
    if op == "mirror":
        return a
    if op == "repeat":
        if a is None or p[1] < 1:
            return None
        ext = p[0] * (p[1] - 1)
        if ext < 0:
            return None
        if p[2] == 0:
            return Box(a.x0, a.y0, a.z0, a.x1 + ext, a.y1, a.z1)
        if p[2] == 1:
            return Box(a.x0, a.y0, a.z0, a.x1, a.y1 + ext, a.z1)
        return Box(a.x0, a.y0, a.z0, a.x1, a.y1, a.z1 + ext)
    if op == "union":
        return Box.union(a, b)
    if op == "difference":
        return a
    if op == "intersect":
        return Box.isect(a, b)
    if op in ("shell", "paint"):
        return a
    if op == "choose":
        out = None
        for e in pack.choose_lists[s["aux"]]:
            out = Box.union(out, boxes[e["node"]])
        return out
    if op == "voxels":
        v = pack.voxels[s["aux"]]
        return Box(0, 0, 0, v["sx"], v["sy"], v["sz"])
    raise PackError(op)


class _Ctx:
    def __init__(self, res: Resolved, cell_x: int, cell_z: int):
        self.res = res
        self.cell_x = cell_x
        self.cell_z = cell_z


def eval_shape(ctx: _Ctx, i: int, x: int, y: int, z: int):
    """Returns None when outside, else the material: ("air",), ("role", idx) or
    ("block", name)."""
    res = ctx.res
    pack = res.pack
    s = pack.shapes[i]
    p = res.shape_params[i]
    op = F.SHAPE_OPS[s["op"]]
    bb = res.shape_bbox[i]
    if bb is None:
        return None
    if not (bb.x0 <= x < bb.x1 and bb.y0 <= y < bb.y1 and bb.z0 <= z < bb.z1):
        return None
    if op == "box":
        return ("air",)
    if op == "cylinder":
        r = p[0]
        dx, dz = x - r, z - r
        return ("air",) if dx * dx + dz * dz <= r * r else None
    if op == "wedge":
        return ("air",) if y * p[0] < (x + 1) * p[1] else None
    if op == "translate":
        return eval_shape(ctx, s["a"], x - p[0], y - p[1], z - p[2])
    if op == "rotate_y":
        a = res.shape_bbox[s["a"]]
        w, d = a.x1 - a.x0, a.z1 - a.z0
        u, v = x - a.x0, z - a.z0
        n = p[0] % 4
        if n == 0:
            cx, cz = a.x0 + u, a.z0 + v
        elif n == 1:
            cx, cz = a.x0 + v, a.z0 + (d - 1 - u)
        elif n == 2:
            cx, cz = a.x0 + (w - 1 - u), a.z0 + (d - 1 - v)
        else:
            cx, cz = a.x0 + (w - 1 - v), a.z0 + u
        return eval_shape(ctx, s["a"], cx, y, cz)
    if op == "mirror":
        a = res.shape_bbox[s["a"]]
        if p[0] == 0:
            return eval_shape(ctx, s["a"], a.x0 + a.x1 - 1 - x, y, z)
        return eval_shape(ctx, s["a"], x, y, a.z0 + a.z1 - 1 - z)
    if op == "repeat":
        a = res.shape_bbox[s["a"]]
        period, count, axis = p
        if period <= 0:
            return eval_shape(ctx, s["a"], x, y, z)
        m = (a.x0, a.y0, a.z0)[axis]
        c = (x, y, z)[axis]
        k = floordiv(c - m, period)
        if k < 0 or k >= count:
            return None
        coords = [x, y, z]
        coords[axis] -= k * period
        return eval_shape(ctx, s["a"], *coords)
    if op == "union":
        r = eval_shape(ctx, s["a"], x, y, z)
        return r if r is not None else eval_shape(ctx, s["b"], x, y, z)
    if op == "difference":
        r = eval_shape(ctx, s["a"], x, y, z)
        if r is None:
            return None
        return None if eval_shape(ctx, s["b"], x, y, z) is not None else r
    if op == "intersect":
        r = eval_shape(ctx, s["a"], x, y, z)
        if r is None:
            return None
        return r if eval_shape(ctx, s["b"], x, y, z) is not None else None
    if op == "shell":
        r = eval_shape(ctx, s["a"], x, y, z)
        if r is None:
            return None
        t = max(1, p[0])
        for dist in range(1, t + 1):
            for dx, dy, dz in ((dist, 0, 0), (-dist, 0, 0), (0, dist, 0), (0, -dist, 0), (0, 0, dist), (0, 0, -dist)):
                if eval_shape(ctx, s["a"], x + dx, y + dy, z + dz) is None:
                    return r
        return None
    if op == "paint":
        r = eval_shape(ctx, s["a"], x, y, z)
        return None if r is None else ("role", s["role"])
    if op == "choose":
        entries = pack.choose_lists[s["aux"]]
        if p[0] == 1:
            pv = res.values[p[1]]
            lst = pack.choice_lists[pack.params[p[1]]["aux"]]
            idx = lst.index(pv)
        else:
            idx = weighted_pick(cell_hash(ctx.cell_x, ctx.cell_z, p[1]), [e["weight"] for e in entries])
        return eval_shape(ctx, entries[idx]["node"], x, y, z)
    if op == "voxels":
        v = pack.voxels[s["aux"]]
        c = v["cells"][(x * v["sz"] + z) * v["sy"] + y]
        if c == F.VOXEL_KEEP:
            return None
        if c == F.VOXEL_AIR:
            return ("air",)
        return ("block", v["palette"][c])
    raise PackError(op)


def material_block(res: Resolved, m) -> str:
    if m[0] == "air":
        return AIR
    if m[0] == "role":
        return res.role_blocks[m[1]]
    return m[1]


def rotated_bbox(bb: Box, n: int, w: int, d: int) -> Box:
    """The bounding box of a root after turning it n times inside the w by d pick
    rectangle; the inverse of the map eval uses."""
    n %= 4
    if n == 0:
        return bb
    pts = []
    for cx, cz in ((bb.x0, bb.z0), (bb.x1 - 1, bb.z0), (bb.x0, bb.z1 - 1), (bb.x1 - 1, bb.z1 - 1)):
        if n == 1:
            u, v = d - 1 - cz, cx
        elif n == 2:
            u, v = w - 1 - cx, d - 1 - cz
        else:
            u, v = cz, w - 1 - cx
        pts.append((u, v))
    us = [q[0] for q in pts]
    vs = [q[1] for q in pts]
    return Box(min(us), bb.y0, min(vs), max(us) + 1, bb.y1, max(vs) + 1)


def choose_turn(mask: int, h: int, w: int, d: int) -> int:
    allowed = [t for t in range(4) if mask & (1 << t)]
    if w != d:
        allowed = [t for t in allowed if t % 2 == 0] or [0]
    return allowed[h % len(allowed)]


def generate_chunk(res: Resolved, cx: int, cz: int) -> list:
    """A list of 256 columns, x major then z, each a list of `height` block names."""
    pack = res.pack
    n = pack.zone_count
    cols = []
    for x in range(16):
        wx = cx * 16 + x
        zx = res.zone_of_x[posmod(wx, res.period_x)]
        for z in range(16):
            wz = cz * 16 + z
            zz = res.zone_of_z[posmod(wz, res.period_z)]
            cols.append(list(res.zone_columns[pack.combine[zx * n + zz]]))
    chunk = Box(cx * 16, res.min_y, cz * 16, cx * 16 + 16, res.max_y, cz * 16 + 16)
    for pick in res.picks:
        gx0 = floordiv(chunk.x0 - pick["x0"] - 512, res.period_x)
        gx1 = floordiv(chunk.x1 - pick["x0"] + 512, res.period_x)
        gz0 = floordiv(chunk.z0 - pick["z0"] - 512, res.period_z)
        gz1 = floordiv(chunk.z1 - pick["z0"] + 512, res.period_z)
        for gx in range(gx0, gx1 + 1):
            for gz in range(gz0, gz1 + 1):
                _stamp(res, pick, gx, gz, chunk, cols)
    return cols


def _stamp(res: Resolved, pick: dict, gx: int, gz: int, chunk: Box, cols: list):
    roots = pick["roots"]
    h = cell_hash(gx, gz, pick["salt"])
    ri = weighted_pick(h, [r["weight"] for r in roots])
    root = roots[ri]
    bb = res.shape_bbox[root["shape_node"]]
    if bb is None:
        return
    w, d = pick["w"], pick["d"]
    turn = choose_turn(root["rot_mask"], mix32(h ^ 0x5BD1E995), w, d)
    rb = rotated_bbox(bb, turn, w, d)
    ox = gx * res.period_x + pick["x0"]
    oz = gz * res.period_z + pick["z0"]
    oy = pick["anchor_y"]
    world = Box(rb.x0 + ox, rb.y0 + oy, rb.z0 + oz, rb.x1 + ox, rb.y1 + oy, rb.z1 + oz)
    hit = Box.isect(world, chunk)
    if hit is None:
        return
    ctx = _Ctx(res, gx, gz)
    for wx in range(hit.x0, hit.x1):
        for wz in range(hit.z0, hit.z1):
            u, v = wx - ox, wz - oz
            if turn == 0:
                cu, cv = u, v
            elif turn == 1:
                cu, cv = v, d - 1 - u
            elif turn == 2:
                cu, cv = w - 1 - u, d - 1 - v
            else:
                cu, cv = w - 1 - v, u
            col = cols[(wx - chunk.x0) * 16 + (wz - chunk.z0)]
            for wy in range(hit.y0, hit.y1):
                m = eval_shape(ctx, root["shape_node"], cu, wy - oy, cv)
                if m is not None:
                    col[wy - res.min_y] = material_block(res, m)
