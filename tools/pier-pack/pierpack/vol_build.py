"""vol_build.py: compiles a volume pack source into a PIERVOL file.

The source has the shape of a Java noise_settings file with the referenced pieces
gathered into it: `density_functions` by id, `noises` by id, `biomes` as the multi-noise
parameter list, and `surface_rule`. Density functions are written as Java writes them: a
number is a constant, a string is a reference, an object has a type and arguments. The
compiler resolves references, folds identical subtrees, and emits FUNC nodes in
post-order. Nothing here depends on the world seed; the host seeds the noises.
"""
from __future__ import annotations

import datetime as _dt
import json
import os
import struct

from . import format as F
from .container import Strings, Writer

TOOL_VERSION = "pier-pack 0.1"


class SourceError(Exception):
    pass


def norm_id(s: str) -> str:
    return s if ":" in s else "minecraft:" + s


def _type(node: dict) -> str:
    t = node.get("type")
    if not isinstance(t, str):
        raise SourceError("a density function object has no type")
    t = norm_id(t)
    return t[len("minecraft:"):] if t.startswith("minecraft:") else t


def _f32bits(v: float) -> int:
    return struct.unpack("<I", struct.pack("<f", float(v)))[0]


class VolumeBuilder:
    def __init__(self, src: dict):
        self.src = src
        self.strings = Strings()
        self.noises: list = []
        self.noise_index: dict = {}
        self.amps: list = []
        self.func_nodes: list = []
        self.func_memo: dict = {}
        self.fcon_values: list = []
        self.splines: list = []
        self.spline_points: list = []
        self.spline_memo: dict = {}
        self.surf_nodes: list = []
        self.surf_lists: list = []
        self.surf_items: list = []
        self.rands: list = []
        self.rand_index: dict = {}
        self.defs = {norm_id(k): v for k, v in src.get("density_functions", {}).items()}
        self.noise_defs = {norm_id(k): v for k, v in src.get("noises", {}).items()}
        self._resolving: set = set()
        self.ref_memo: dict = {}

    # Noises

    def noise(self, ref, where: str) -> int:
        if isinstance(ref, str):
            key = norm_id(ref)
            if key in self.noise_index:
                return self.noise_index[key]
            if key not in self.noise_defs:
                raise SourceError(f"{where}: unknown noise '{ref}'")
            params = self.noise_defs[key]
            amps = [float(a) for a in params["amplitudes"]]
            # Under legacy_random_source Java silences the shift noise.
            if self.src.get("legacy_random_source", False) and key == "minecraft:shift":
                amps = [0.0]
                params = {"firstOctave": 0}
            idx = self._add_noise(key, int(params["firstOctave"]), amps, F.NOISE_NORMAL, 0)
            self.noise_index[key] = idx
            return idx
        raise SourceError(f"{where}: a noise must be referenced by id")

    def _add_noise(self, name: str, first_octave: int, amplitudes: list, kind: int, flags: int) -> int:
        nz = [k for k, a in enumerate(amplitudes) if a != 0.0]
        # Java's span for an all-zero list overflows to 1; the value is 0 either way.
        span = (max(nz) - min(nz)) if nz else 1
        value_factor = 0.16666666666666666 / (0.1 * (1.0 + 1.0 / (span + 1))) if kind == F.NOISE_NORMAL else 0.0
        idx = len(self.noises)
        self.noises.append({"name_str": self.strings.add(name), "first_octave": first_octave, "octave_count": len(amplitudes),
                            "amp_first": len(self.amps), "value_factor": value_factor, "kind": kind, "flags": flags})
        self.amps += amplitudes
        return idx

    def legacy_biome_noise(self, key: str, index: int) -> int:
        """The nether's temperature and vegetation under legacy_random_source."""
        memo = "legacy:" + key
        if memo in self.noise_index:
            return self.noise_index[memo]
        idx = self._add_noise(key, -7, [1.0, 1.0], F.NOISE_NORMAL, F.NOISE_LEGACY_BIOME | (index << F.NOISE_LEGACY_INDEX_SHIFT))
        self.noise_index[memo] = idx
        return idx

    # Density functions

    def _emit(self, **kw) -> int:
        node = {"op": 0, "flags": 0, "a": F.NONE, "b": F.NONE, "c": F.NONE, "p0": 0.0, "p1": 0.0, "aux": F.NONE, "reserved": 0}
        node.update(kw)
        key = tuple(sorted(node.items()))
        if key in self.func_memo:
            return self.func_memo[key]
        idx = len(self.func_nodes)
        self.func_nodes.append(node)
        self.func_memo[key] = idx
        return idx

    def fcon(self, values: list) -> int:
        first = len(self.fcon_values)
        self.fcon_values += [float(v) for v in values]
        return first

    def df(self, node, where: str) -> int:
        if isinstance(node, bool):
            raise SourceError(f"{where}: a boolean is not a density function")
        if isinstance(node, (int, float)):
            return self._emit(op=F.FUNC_OP["const"], p0=float(node))
        if isinstance(node, str):
            key = norm_id(node)
            if key in self.ref_memo:
                return self.ref_memo[key]
            if key not in self.defs:
                raise SourceError(f"{where}: unknown density function '{node}'")
            if key in self._resolving:
                raise SourceError(f"density function '{node}' refers to itself")
            self._resolving.add(key)
            idx = self.df(self.defs[key], key)
            self._resolving.discard(key)
            self.ref_memo[key] = idx
            return idx
        if not isinstance(node, dict):
            raise SourceError(f"{where}: cannot read a density function of this shape")
        t = _type(node)
        op = F.FUNC_OP.get(t)
        arg = lambda k: self.df(node[k], where + "." + k) if k in node else self._missing(where, k)
        if t == "constant":
            return self._emit(op=F.FUNC_OP["const"], p0=float(node.get("argument", 0.0)))
        if t in ("add", "mul", "min", "max"):
            return self._emit(op=op, a=arg("argument1"), b=arg("argument2"))
        if t in ("abs", "square", "cube", "half_negative", "quarter_negative", "squeeze", "blend_density",
                 "interpolated", "flat_cache", "cache_2d", "cache_once", "cache_all_in_cell"):
            return self._emit(op=op, a=arg("argument"))
        if t == "clamp":
            return self._emit(op=op, a=arg("input"), p0=float(node["min"]), p1=float(node["max"]))
        if t == "range_choice":
            return self._emit(op=op, a=arg("input"), b=arg("when_in_range"), c=arg("when_out_of_range"),
                              p0=float(node["min_inclusive"]), p1=float(node["max_exclusive"]))
        if t == "y_clamped_gradient":
            return self._emit(op=op, p0=float(node["from_y"]), p1=float(node["to_y"]),
                              aux=self.fcon([node["from_value"], node["to_value"]]))
        if t == "noise":
            return self._emit(op=op, aux=self.noise(node["noise"], where), p0=float(node.get("xz_scale", 1.0)), p1=float(node.get("y_scale", 1.0)))
        if t == "shifted_noise":
            return self._emit(op=op, aux=self.noise(node["noise"], where), a=arg("shift_x"), b=arg("shift_y"), c=arg("shift_z"),
                              p0=float(node.get("xz_scale", 1.0)), p1=float(node.get("y_scale", 1.0)))
        if t in ("shift", "shift_a", "shift_b"):
            return self._emit(op=op, aux=self.noise(node["argument"], where))
        if t == "spline":
            return self._emit(op=op, aux=self.spline(node["spline"], where + ".spline"))
        if t == "end_islands":
            return self._emit(op=op, aux=self._add_noise("minecraft:end_islands", 0, [1.0], F.NOISE_SIMPLEX, 0))
        if t == "weird_scaled_sampler":
            mapper = str(node.get("rarity_value_mapper", "type_1")).lower()
            if mapper not in ("type_1", "type_2"):
                raise SourceError(f"{where}: rarity_value_mapper must be type_1 or type_2")
            return self._emit(op=op, a=arg("input"), aux=self.noise(node["noise"], where), p0=0.0 if mapper == "type_1" else 1.0)
        if t in ("blend_alpha", "blend_offset"):
            return self._emit(op=op)
        if t == "old_blended_noise":
            params = [float(node.get(k, d)) for k, d in (("xz_scale", 0.25), ("y_scale", 0.125), ("xz_factor", 80.0), ("y_factor", 160.0), ("smear_scale_multiplier", 8.0))]
            return self._emit(op=op, aux=self._add_noise("minecraft:terrain", 0, params, F.NOISE_BLENDED, 0))
        raise SourceError(f"{where}: density function type '{t}' is not supported")

    @staticmethod
    def _missing(where, k):
        raise SourceError(f"{where}: missing '{k}'")

    def spline(self, sp, where: str) -> int:
        if isinstance(sp, (int, float)):
            raise SourceError(f"{where}: a bare number is a point value, not a spline")
        coord = self.df(sp["coordinate"], where + ".coordinate")
        points = []
        last = None
        for i, p in enumerate(sp.get("points", [])):
            loc = float(p["location"])
            if last is not None and loc <= last:
                raise SourceError(f"{where}: point locations must strictly increase")
            last = loc
            val = p.get("value", 0.0)
            if isinstance(val, (int, float)) and not isinstance(val, bool):
                points.append((loc, float(p.get("derivative", 0.0)), F.SPLINE_POINT_CONST, _f32bits(val)))
            else:
                points.append((loc, float(p.get("derivative", 0.0)), F.SPLINE_POINT_SPLINE, self.spline(val, f"{where}.points[{i}]")))
        if not points:
            raise SourceError(f"{where}: a spline needs at least one point")
        key = (coord, tuple(points))
        if key in self.spline_memo:
            return self.spline_memo[key]
        idx = len(self.splines)
        self.splines.append({"coord_node": coord, "point_first": len(self.spline_points), "point_count": len(points), "reserved": 0})
        self.spline_points += points
        self.spline_memo[key] = idx
        return idx

    # Surface rules

    def rand(self, name: str) -> int:
        key = norm_id(name)
        if key not in self.rand_index:
            self.rand_index[key] = len(self.rands)
            self.rands.append(self.strings.add(key))
        return self.rand_index[key]

    def _anchor(self, a, where: str) -> tuple[int, int]:
        if isinstance(a, dict):
            if "absolute" in a:
                return int(a["absolute"]), F.ANCHOR_ABSOLUTE
            if "above_bottom" in a:
                return int(a["above_bottom"]), F.ANCHOR_ABOVE_BOTTOM
            if "below_top" in a:
                return int(a["below_top"]), F.ANCHOR_BELOW_TOP
        raise SourceError(f"{where}: a vertical anchor is {{absolute|above_bottom|below_top: n}}")

    def _emit_surf(self, **kw) -> int:
        node = {"op": 0, "flags": 0, "a": F.NONE, "b": F.NONE, "p0": 0.0, "p1": 0.0, "i0": 0, "i1": 0, "aux": F.NONE}
        node.update(kw)
        idx = len(self.surf_nodes)
        self.surf_nodes.append(node)
        return idx

    def _surf_list(self, items: list) -> int:
        idx = len(self.surf_lists)
        self.surf_lists.append((len(self.surf_items), len(items)))
        self.surf_items += items
        return idx

    def rule(self, r: dict, where: str) -> int:
        t = _type(r)
        if t == "block":
            state = r.get("result_state", {})
            name = state.get("Name") if isinstance(state, dict) else state
            if not isinstance(name, str):
                raise SourceError(f"{where}: block needs result_state.Name")
            return self._emit_surf(op=F.SURF_OPS["block"], aux=self.strings.add(norm_id(name)))
        if t == "sequence":
            nodes = [self.rule(s, f"{where}.sequence[{i}]") for i, s in enumerate(r.get("sequence", []))]
            return self._emit_surf(op=F.SURF_OPS["sequence"], aux=self._surf_list(nodes))
        if t == "condition":
            cond = self.cond(r["if_true"], where + ".if_true")
            then = self.rule(r["then_run"], where + ".then_run")
            return self._emit_surf(op=F.SURF_OPS["condition"], a=cond, b=then)
        if t == "bandlands":
            raise SourceError(f"{where}: bandlands is not supported by this version")
        raise SourceError(f"{where}: surface rule type '{t}' is not supported")

    def cond(self, c: dict, where: str) -> int:
        t = _type(c)
        if t == "biome":
            names = [self.strings.add(norm_id(b)) for b in c.get("biome_is", [])]
            return self._emit_surf(op=F.SURF_OPS["biome"], aux=self._surf_list(names))
        if t == "noise_threshold":
            return self._emit_surf(op=F.SURF_OPS["noise_threshold"], aux=self.noise(c["noise"], where),
                                   p0=float(c["min_threshold"]), p1=float(c["max_threshold"]))
        if t == "vertical_gradient":
            i0, k0 = self._anchor(c["true_at_and_below"], where)
            i1, k1 = self._anchor(c["false_at_and_above"], where)
            return self._emit_surf(op=F.SURF_OPS["vertical_gradient"], aux=self.rand(c["random_name"]), i0=i0, i1=i1, flags=k0 | (k1 << 2))
        if t == "y_above":
            i0, k0 = self._anchor(c["anchor"], where)
            flags = k0 | (F.SURF_ADD_STONE_DEPTH if c.get("add_stone_depth", False) else 0)
            return self._emit_surf(op=F.SURF_OPS["y_above"], i0=i0, i1=int(c.get("surface_depth_multiplier", 0)), flags=flags)
        if t == "water":
            flags = F.SURF_ADD_STONE_DEPTH if c.get("add_stone_depth", False) else 0
            return self._emit_surf(op=F.SURF_OPS["water"], i0=int(c.get("offset", 0)), i1=int(c.get("surface_depth_multiplier", 0)), flags=flags)
        if t in ("temperature", "steep", "hole", "above_preliminary_surface"):
            return self._emit_surf(op=F.SURF_OPS[t])
        if t == "not":
            return self._emit_surf(op=F.SURF_OPS["not"], a=self.cond(c["invert"], where + ".invert"))
        if t == "stone_depth":
            flags = (F.SURF_ADD_STONE_DEPTH if c.get("add_surface_depth", False) else 0)
            if str(c.get("surface_type", "floor")) == "ceiling":
                flags |= F.SURF_CEILING
            return self._emit_surf(op=F.SURF_OPS["stone_depth"], i0=int(c.get("offset", 0)), i1=int(c.get("secondary_depth_range", 0)), flags=flags)
        raise SourceError(f"{where}: surface condition type '{t}' is not supported")

    # Sections

    def build(self) -> bytes:
        src = self.src
        noise = src.get("noise", {})
        min_y = int(noise.get("min_y", -64))
        height = int(noise.get("height", 384))
        if min_y % 16 or height % 16 or height <= 0:
            raise SourceError("noise.min_y and noise.height must be multiples of 16 with a positive height")
        router = src.get("noise_router", {})
        if "final_density" not in router:
            raise SourceError("noise_router.final_density is required")
        legacy = bool(src.get("legacy_random_source", False))
        rout = {}
        for k in F.ROUTER_FIELDS:
            if k == "reserved":
                rout[k] = 0
                continue
            v = router.get(k)
            if v is None:
                rout[k] = F.NONE
            elif legacy and k in ("temperature", "vegetation") and isinstance(v, dict) and _type(v) == "noise":
                # Java swaps these two under legacy_random_source for the nether's
                # own noise; the density function around them is kept.
                idx = 0 if k == "temperature" else 1
                rout[k] = self._emit(op=F.FUNC_OP["noise"], aux=self.legacy_biome_noise(norm_id(v["noise"]), idx),
                                     p0=float(v.get("xz_scale", 1.0)), p1=float(v.get("y_scale", 1.0)))
            else:
                rout[k] = self.df(v, "noise_router." + k)
        surface_root = F.NONE
        if "surface_rule" in src:
            surface_root = self.rule(src["surface_rule"], "surface_rule")
        biomes = []
        for i, b in enumerate(src.get("biomes", [])):
            p = b.get("parameters", b)
            def rng(key):
                v = p.get(key, [0.0, 0.0])
                if isinstance(v, (int, float)):
                    return float(v), float(v)
                return float(v[0]), float(v[1])
            t = rng("temperature"); h = rng("humidity"); c = rng("continentalness"); e = rng("erosion"); d = rng("depth"); w = rng("weirdness")
            biomes.append({"biome_str": self.strings.add(norm_id(b["biome"])), "t_lo": t[0], "t_hi": t[1], "h_lo": h[0], "h_hi": h[1],
                           "c_lo": c[0], "c_hi": c[1], "e_lo": e[0], "e_hi": e[1], "d_lo": d[0], "d_hi": d[1], "w_lo": w[0], "w_hi": w[1],
                           "offset": float(p.get("offset", 0.0)),
                           "base_temperature": float(b.get("base_temperature", src.get("biome_temperatures", {}).get(norm_id(b["biome"]), 0.5)))})
        if not biomes:
            raise SourceError("biomes: at least one biome target is needed")
        surface_rand = self.rand("minecraft:surface") if surface_root != F.NONE else F.NONE
        surface_noise = self.noise("minecraft:surface", "surface") if surface_root != F.NONE and "minecraft:surface" in self.noise_defs else F.NONE
        surface_secondary = self.noise("minecraft:surface_secondary", "surface") if surface_root != F.NONE and "minecraft:surface_secondary" in self.noise_defs else F.NONE
        if surface_root != F.NONE and surface_noise == F.NONE:
            raise SourceError("surface_rule needs the noise minecraft:surface in noises")

        def block_name(v, default):
            if isinstance(v, dict):
                return norm_id(v.get("Name", default))
            return norm_id(v) if isinstance(v, str) else default

        flags = (F.SETTINGS_LEGACY_RANDOM if legacy else 0)
        if src.get("aquifers_enabled", False):
            flags |= F.SETTINGS_AQUIFERS
        if src.get("ore_veins_enabled", False):
            flags |= F.SETTINGS_ORE_VEINS
        sett = F.SETTINGS.pack(min_y=min_y, height=height, sea_level=int(src.get("sea_level", 63)),
                               default_block_str=self.strings.add(block_name(src.get("default_block"), "minecraft:stone")),
                               default_fluid_str=self.strings.add(block_name(src.get("default_fluid"), "minecraft:water")),
                               size_horizontal=int(noise.get("size_horizontal", 1)), size_vertical=int(noise.get("size_vertical", 2)),
                               flags=flags, surface_rand=surface_rand, surface_noise=surface_noise,
                               surface_secondary_noise=surface_secondary, reserved=0)
        w = Writer(F.MAGIC_VOLUME)
        bodies = []
        bodies.append((F.SEC_INFO, F.INFO.pack(
            tool_version_str=self.strings.add(TOOL_VERSION),
            built_at_str=self.strings.add(_dt.datetime.now(_dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")),
            source_name_str=self.strings.add(str(src.get("name", ""))),
            height_min=min_y, height_max=min_y + height, flags=F.INFO_HEIGHT_FIXED, biome_str=0)))
        bodies.append((F.SEC_SETT, sett))
        body = struct.pack("<I", len(self.noises)) + b"".join(F.NOISE.pack(**n) for n in self.noises)
        body += struct.pack("<%dd" % len(self.amps), *self.amps)
        bodies.append((F.SEC_NOIS, body))
        body = struct.pack("<I", len(self.splines)) + b"".join(F.SPLINE.pack(**s) for s in self.splines)
        body += b"".join(F.SPLINE_POINT.pack(location=l, derivative=d, kind=k, ref=r) for l, d, k, r in self.spline_points)
        bodies.append((F.SEC_SPLN, body))
        bodies.append((F.SEC_FCON, struct.pack("<I", len(self.fcon_values)) + struct.pack("<%df" % len(self.fcon_values), *self.fcon_values)))
        bodies.append((F.SEC_FUNC, struct.pack("<I", len(self.func_nodes)) + b"".join(F.FUNC_NODE.pack(**n) for n in self.func_nodes)))
        bodies.append((F.SEC_ROUT, F.ROUTER.pack(**rout)))
        body = struct.pack("<I", len(self.surf_nodes)) + b"".join(F.SURF_NODE.pack(**n) for n in self.surf_nodes)
        body += struct.pack("<I", len(self.surf_lists)) + b"".join(F.LIST_REF.pack(first=f, count=c) for f, c in self.surf_lists)
        body += struct.pack("<%dI" % len(self.surf_items), *self.surf_items)
        body += struct.pack("<I", surface_root)
        bodies.append((F.SEC_SURF, body))
        bodies.append((F.SEC_BIOM, struct.pack("<I", len(biomes)) + b"".join(F.BIOME_TARGET.pack(**b) for b in biomes)))
        bodies.append((F.SEC_RAND, struct.pack("<I", len(self.rands)) + struct.pack("<%dI" % len(self.rands), *self.rands)))
        w.add(F.SEC_STRS, self.strings.body())
        for tag, body in bodies:
            w.add(tag, body)
        return w.build()


def build_file(source_path: str, out_path: str) -> bytes:
    with open(source_path, encoding="utf-8") as f:
        src = json.load(f)
    if src.get("type") != "volume":
        raise SourceError("this source is not a volume pack")
    data = VolumeBuilder(src).build()
    with open(out_path, "wb") as f:
        f.write(data)
    return data
