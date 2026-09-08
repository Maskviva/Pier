"""vol_read.py: decodes a PIERVOL file into a plain model and validates its invariants.

The same checks the C++ decoder makes: every operand index smaller than its node's,
every noise, spline, FCON, RAND and string index in range, every list inside its array,
the router channels in range, and the settings on subchunk boundaries.
"""
from __future__ import annotations

import struct
from dataclasses import dataclass, field

from . import format as F
from .container import PackError, Reader, read_array, read_u32s


@dataclass
class VolumePack:
    info: dict
    strings: list
    settings: dict
    noises: list
    amps: list
    splines: list
    spline_points: list
    fcon: list
    funcs: list
    router: dict
    surf: list
    surf_lists: list
    surf_root: int
    biomes: list
    rands: list
    file_sha256: str = ""

    def summary(self) -> dict:
        s = self.settings
        return {
            "settings": {"min_y": s["min_y"], "height": s["height"], "sea_level": s["sea_level"],
                         "default_block": self.strings[s["default_block_str"]], "default_fluid": self.strings[s["default_fluid_str"]],
                         "size_horizontal": s["size_horizontal"], "size_vertical": s["size_vertical"],
                         "legacy_random_source": bool(s["flags"] & F.SETTINGS_LEGACY_RANDOM)},
            "noises": [self.strings[n["name_str"]] for n in self.noises],
            "functions": len(self.funcs), "splines": len(self.splines),
            "router": {k: (v != F.NONE) for k, v in self.router.items() if k != "reserved"},
            "surface_rules": len(self.surf), "biomes": [self.strings[b["biome_str"]] for b in self.biomes],
        }


def _check(v, limit, what, allow_none=False):
    if v == F.NONE:
        if allow_none:
            return
        raise PackError(f"{what} is missing")
    if v >= limit:
        raise PackError(f"{what} index {v} is out of range (limit {limit})")


def read_volume(data: bytes) -> VolumePack:
    r = Reader(data)
    if r.kind != "volume":
        raise PackError("this file is a template pack, not a volume pack")
    info = F.INFO.unpack(r.section(F.SEC_INFO), 0)
    info = {"tool_version": r.str(info["tool_version_str"]), "built_at": r.str(info["built_at_str"]),
            "source_name": r.str(info["source_name_str"]), "height_min": info["height_min"], "height_max": info["height_max"],
            "height_fixed": bool(info["flags"] & F.INFO_HEIGHT_FIXED)}
    sett = F.SETTINGS.unpack(r.section(F.SEC_SETT), 0)
    if sett["min_y"] % 16 or sett["height"] % 16 or sett["height"] <= 0:
        raise PackError("SETT: min_y and height must be multiples of 16 with a positive height")
    if sett["size_horizontal"] not in (1, 2, 4) or sett["size_vertical"] not in (1, 2, 4):
        raise PackError("SETT: size_horizontal and size_vertical must be 1, 2 or 4")
    if info["height_min"] != sett["min_y"] or info["height_max"] != sett["min_y"] + sett["height"]:
        raise PackError("INFO height does not match SETT")
    for k in ("default_block_str", "default_fluid_str"):
        _check(sett[k], len(r.strings), "SETT " + k)

    body = r.section(F.SEC_NOIS)
    (n,) = struct.unpack_from("<I", body, 0)
    noises, off = read_array(body, 4, F.NOISE, n)
    total = sum(x["octave_count"] for x in noises)
    if off + 8 * total > len(body):
        raise PackError("NOIS amplitudes run past the section")
    amps = list(struct.unpack_from("<%dd" % total, body, off))
    for i, x in enumerate(noises):
        _check(x["name_str"], len(r.strings), f"NOIS {i} name")
        if x["kind"] not in (F.NOISE_NORMAL, F.NOISE_SIMPLEX, F.NOISE_BLENDED):
            raise PackError(f"NOIS {i}: unknown kind {x['kind']}")
        if x["amp_first"] + x["octave_count"] > total or x["octave_count"] == 0:
            raise PackError(f"NOIS {i}: amplitudes out of range")
        if x["kind"] == F.NOISE_BLENDED and x["octave_count"] != 5:
            raise PackError(f"NOIS {i}: a blended noise carries five parameters")

    body = r.section(F.SEC_FCON)
    (n,) = struct.unpack_from("<I", body, 0)
    if 4 + 4 * n > len(body):
        raise PackError("FCON values run past the section")
    fcon = list(struct.unpack_from("<%df" % n, body, 4))

    body = r.section(F.SEC_FUNC)
    (n,) = struct.unpack_from("<I", body, 0)
    funcs, _ = read_array(body, 4, F.FUNC_NODE, n)

    body = r.section(F.SEC_SPLN)
    (n,) = struct.unpack_from("<I", body, 0)
    splines, off = read_array(body, 4, F.SPLINE, n)
    total = sum(x["point_count"] for x in splines)
    points, _ = read_array(body, off, F.SPLINE_POINT, total)
    for i, sp in enumerate(splines):
        _check(sp["coord_node"], len(funcs), f"SPLN {i} coordinate")
        if sp["point_count"] == 0 or sp["point_first"] + sp["point_count"] > total:
            raise PackError(f"SPLN {i}: points out of range")
        last = None
        for j in range(sp["point_first"], sp["point_first"] + sp["point_count"]):
            p = points[j]
            if last is not None and p["location"] <= last:
                raise PackError(f"SPLN {i}: point locations must strictly increase")
            last = p["location"]
            if p["kind"] == F.SPLINE_POINT_SPLINE:
                if p["ref"] >= i:
                    raise PackError(f"SPLN {i}: a nested spline must have a smaller index")
            elif p["kind"] != F.SPLINE_POINT_CONST:
                raise PackError(f"SPLN {i}: unknown point kind")

    for i, fn in enumerate(funcs):
        if fn["op"] >= len(F.FUNC_OPS):
            raise PackError(f"FUNC {i}: unknown op {fn['op']}")
        op = F.FUNC_OPS[fn["op"]]
        for k in ("a", "b", "c"):
            if fn[k] != F.NONE and fn[k] >= i:
                raise PackError(f"FUNC {i}: operand {k} is not an earlier node")
        needs = {"add": "ab", "mul": "ab", "min": "ab", "max": "ab", "abs": "a", "square": "a", "cube": "a", "half_negative": "a",
                 "quarter_negative": "a", "squeeze": "a", "clamp": "a", "range_choice": "abc", "shifted_noise": "abc",
                 "weird_scaled_sampler": "a", "blend_density": "a", "interpolated": "a", "flat_cache": "a", "cache_2d": "a",
                 "cache_once": "a", "cache_all_in_cell": "a"}.get(op, "")
        for k in needs:
            if fn[k] == F.NONE:
                raise PackError(f"FUNC {i}: {op} has no operand {k}")
        if op in ("noise", "shifted_noise", "shift", "shift_a", "shift_b", "weird_scaled_sampler"):
            _check(fn["aux"], len(noises), f"FUNC {i} noise")
            if noises[fn["aux"]]["kind"] != F.NOISE_NORMAL:
                raise PackError(f"FUNC {i}: {op} needs a normal noise")
        if op == "end_islands":
            _check(fn["aux"], len(noises), f"FUNC {i} noise")
            if noises[fn["aux"]]["kind"] != F.NOISE_SIMPLEX:
                raise PackError(f"FUNC {i}: end_islands needs the simplex noise")
        if op == "old_blended_noise":
            _check(fn["aux"], len(noises), f"FUNC {i} noise")
            if noises[fn["aux"]]["kind"] != F.NOISE_BLENDED:
                raise PackError(f"FUNC {i}: old_blended_noise needs a blended noise")
        if op == "spline":
            _check(fn["aux"], len(splines), f"FUNC {i} spline")
        if op == "y_clamped_gradient":
            if fn["aux"] == F.NONE or fn["aux"] + 2 > len(fcon):
                raise PackError(f"FUNC {i}: y_clamped_gradient values out of range")

    router = F.ROUTER.unpack(r.section(F.SEC_ROUT), 0)
    for k, v in router.items():
        if k == "reserved":
            continue
        _check(v, len(funcs), "ROUT " + k, allow_none=True)
    if router["final_density"] == F.NONE:
        raise PackError("ROUT: final_density is missing")

    body = r.section(F.SEC_SURF)
    (n,) = struct.unpack_from("<I", body, 0)
    surf, off = read_array(body, 4, F.SURF_NODE, n)
    (ln,) = struct.unpack_from("<I", body, off)
    refs, off = read_array(body, off + 4, F.LIST_REF, ln)
    total = sum(x["count"] for x in refs)
    items, off = read_u32s(body, off, total)
    (root,) = struct.unpack_from("<I", body, off)
    surf_lists = [items[x["first"]:x["first"] + x["count"]] for x in refs]

    body = r.section(F.SEC_RAND)
    (n,) = struct.unpack_from("<I", body, 0)
    rands, _ = read_u32s(body, 4, n)
    for s_ in rands:
        _check(s_, len(r.strings), "RAND name")

    for i, sn in enumerate(surf):
        if sn["op"] not in F.SURF_OP_NAMES:
            raise PackError(f"SURF {i}: unknown op {sn['op']}")
        op = F.SURF_OP_NAMES[sn["op"]]
        for k in ("a", "b"):
            if sn[k] != F.NONE and sn[k] >= i:
                raise PackError(f"SURF {i}: operand {k} is not an earlier node")
        if op == "block":
            _check(sn["aux"], len(r.strings), f"SURF {i} block")
        if op in ("sequence", "biome"):
            _check(sn["aux"], len(surf_lists), f"SURF {i} list")
            for it in surf_lists[sn["aux"]]:
                if op == "sequence" and it >= i:
                    raise PackError(f"SURF {i}: a sequence item is not an earlier node")
                if op == "biome" and it >= len(r.strings):
                    raise PackError(f"SURF {i}: a biome name is out of range")
        if op == "condition" and (sn["a"] == F.NONE or sn["b"] == F.NONE):
            raise PackError(f"SURF {i}: condition needs a condition and a rule")
        if op == "not" and sn["a"] == F.NONE:
            raise PackError(f"SURF {i}: not needs an operand")
        if op == "noise_threshold":
            _check(sn["aux"], len(noises), f"SURF {i} noise")
        if op == "vertical_gradient":
            _check(sn["aux"], len(rands), f"SURF {i} random")
    _check(root, len(surf), "SURF root", allow_none=True)
    if root != F.NONE:
        _check(sett["surface_rand"], len(rands), "SETT surface random")
        _check(sett["surface_noise"], len(noises), "SETT surface noise")
        _check(sett["surface_secondary_noise"], len(noises), "SETT surface secondary noise", allow_none=True)

    body = r.section(F.SEC_BIOM)
    (n,) = struct.unpack_from("<I", body, 0)
    biomes, _ = read_array(body, 4, F.BIOME_TARGET, n)
    if not biomes:
        raise PackError("BIOM: no biome targets")
    for b in biomes:
        _check(b["biome_str"], len(r.strings), "BIOM name")

    return VolumePack(info=info, strings=r.strings, settings=sett, noises=noises, amps=amps, splines=splines,
                      spline_points=points, fcon=fcon, funcs=funcs, router=router, surf=surf, surf_lists=surf_lists,
                      surf_root=root, biomes=biomes, rands=rands, file_sha256=r.file_sha256())
