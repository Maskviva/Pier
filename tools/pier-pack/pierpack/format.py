"""format.py: the byte layout of PIERTPL and PIERVOL, mirroring pack_format.h.

Every struct here is a `struct` format string plus a tuple of field names, in the same
order as the C++ struct. tests/test_layout.py reads the static_asserts out of the header
and compares them with `struct.calcsize` of these strings, so a field added on one side
without the other fails there rather than at load time on a server.
"""
from __future__ import annotations

import struct
from dataclasses import dataclass

MAGIC_TEMPLATE = b"PIERTPL\0"
MAGIC_VOLUME = b"PIERVOL\0"
FORMAT_VERSION = 1
HEADER_SIZE = 32
SECTION_ENTRY_SIZE = 56
SECTION_ALIGN = 8
NONE = 0xFFFFFFFF


def fourcc(tag: str) -> int:
    assert len(tag) == 4
    b = tag.encode("ascii")
    return b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24)


def tag_name(value: int) -> str:
    return bytes([value & 0xFF, (value >> 8) & 0xFF, (value >> 16) & 0xFF, (value >> 24) & 0xFF]).decode(
        "ascii", "replace"
    )


@dataclass(frozen=True)
class Layout:
    name: str
    fmt: str
    fields: tuple

    @property
    def size(self) -> int:
        return struct.calcsize(self.fmt)

    def pack(self, **kw) -> bytes:
        return struct.pack(self.fmt, *[kw.get(f, 0) for f in self.fields])

    def unpack(self, buf: bytes, off: int = 0) -> dict:
        vals = struct.unpack_from(self.fmt, buf, off)
        return dict(zip(self.fields, vals))


# Container
HEADER = Layout("Header", "<8sIIIIQ", ("magic", "format_version", "flags", "section_count", "header_size", "total_size"))
SECTION = Layout("SectionEntry", "<IIQQ32s", ("type", "version", "offset", "length", "sha256"))
STRING_REF = Layout("StringRef", "<II", ("offset", "length"))
INFO = Layout(
    "Info",
    "<IIIiiIII",
    ("tool_version_str", "built_at_str", "source_name_str", "height_min", "height_max", "flags", "biome_str", "reserved"),
)
INFO_HEIGHT_FIXED = 1 << 0

SEC_STRS = fourcc("STRS")
SEC_INFO = fourcc("INFO")

# PIERTPL
SEC_PARM = fourcc("PARM")
SEC_PCHC = fourcc("PCHC")
SEC_EXPR = fourcc("EXPR")
SEC_ROLE = fourcc("ROLE")
SEC_ZONE = fourcc("ZONE")
SEC_STAK = fourcc("STAK")
SEC_CNST = fourcc("CNST")
SEC_SHAP = fourcc("SHAP")
SEC_PICK = fourcc("PICK")
SEC_VOXL = fourcc("VOXL")
SEC_CONF = fourcc("CONF")

PARAM_FREE, PARAM_FIXED, PARAM_CHOICE, PARAM_DERIVED = 0, 1, 2, 3
PARAM_KIND_NAMES = {PARAM_FREE: "free", PARAM_FIXED: "fixed", PARAM_CHOICE: "choice", PARAM_DERIVED: "derived"}

PARAM = Layout("Param", "<IHHiiiiII", ("name_str", "kind", "flags", "def", "min", "max", "step", "aux", "reserved"))
LIST_REF = Layout("ListRef", "<II", ("first", "count"))

EXPR_OPS = ("const", "param", "add", "sub", "mul", "div", "mod", "min", "max", "neg", "abs",
            "lt", "le", "eq", "and", "or", "not")
EXPR_OP = {n: i for i, n in enumerate(EXPR_OPS)}
EXPR_NODE = Layout("ExprNode", "<HHIIi", ("op", "reserved", "a", "b", "imm"))

ROLE = Layout("Role", "<II", ("name_str", "default_block_str"))
SPAN = Layout("Span", "<II", ("zone", "len_expr"))
STACK = Layout("Stack", "<IIII", ("zone", "from_expr", "to_expr", "role"))
CONSTRAINT = Layout("Constraint", "<II", ("expr", "message_str"))

SHAPE_OPS = (
    "nothing", "box", "cylinder", "wedge", "translate", "rotate_y", "mirror", "repeat",
    "union", "difference", "intersect", "shell", "paint", "choose", "voxels",
)
SHAPE_OP = {n: i for i, n in enumerate(SHAPE_OPS)}
SHAPE_P0_IS_EXPR, SHAPE_P1_IS_EXPR, SHAPE_P2_IS_EXPR = 1, 2, 4
SHAPE_NODE = Layout("ShapeNode", "<HHIIiiiHHI", ("op", "flags", "a", "b", "p0", "p1", "p2", "role", "reserved", "aux"))
CHOOSE_ENTRY = Layout("ChooseEntry", "<II", ("node", "weight"))

PICK = Layout(
    "Pick", "<IIIIIIII", ("salt", "x0_expr", "z0_expr", "w_expr", "d_expr", "anchor_y_expr", "root_first", "root_count")
)
PICK_ROOT = Layout("PickRoot", "<IIII", ("shape_node", "weight", "rot_mask", "reserved"))

VOXEL_RAW, VOXEL_RLE = 0, 1
VOXEL_KEEP, VOXEL_AIR = 0, 1
VOXEL = Layout(
    "Voxel",
    "<IIIIIBB2sQQQQQQ",
    ("sx", "sy", "sz", "pal_count", "pal_first", "encoding", "has_liquid", "reserved",
     "main_off", "main_len", "col_off", "liq_off", "liq_len", "liq_col_off"),
)
CONFINE = Layout("Confine", "<II", ("cell_expr", "gap_expr"))

# PIERVOL
SEC_SETT = fourcc("SETT")
SEC_NOIS = fourcc("NOIS")
SEC_SPLN = fourcc("SPLN")
SEC_FCON = fourcc("FCON")
SEC_FUNC = fourcc("FUNC")
SEC_ROUT = fourcc("ROUT")
SEC_SURF = fourcc("SURF")
SEC_BIOM = fourcc("BIOM")
SEC_RAND = fourcc("RAND")

SETTINGS_AQUIFERS, SETTINGS_ORE_VEINS, SETTINGS_LEGACY_RANDOM = 1, 2, 4
SETTINGS = Layout(
    "Settings", "<iiiIIIIIIIII",
    ("min_y", "height", "sea_level", "default_block_str", "default_fluid_str", "size_horizontal", "size_vertical", "flags",
     "surface_rand", "surface_noise", "surface_secondary_noise", "reserved"),
)
NOISE_NORMAL, NOISE_SIMPLEX, NOISE_BLENDED = 0, 1, 2
NOISE_LEGACY_BIOME = 1
NOISE_LEGACY_INDEX_SHIFT = 8
NOISE = Layout("Noise", "<IiIIdII", ("name_str", "first_octave", "octave_count", "amp_first", "value_factor", "kind", "flags"))
SPLINE_POINT_CONST, SPLINE_POINT_SPLINE = 0, 1
SPLINE = Layout("Spline", "<IIII", ("coord_node", "point_first", "point_count", "reserved"))
SPLINE_POINT = Layout("SplinePoint", "<ffII", ("location", "derivative", "kind", "ref"))

FUNC_OPS = (
    "const", "add", "mul", "min", "max", "abs", "square", "cube", "half_negative", "quarter_negative",
    "squeeze", "clamp", "range_choice", "y_clamped_gradient", "noise", "shifted_noise", "shift",
    "shift_a", "shift_b", "spline", "end_islands", "weird_scaled_sampler", "blend_alpha",
    "blend_offset", "blend_density", "interpolated", "flat_cache", "cache_2d", "cache_once",
    "cache_all_in_cell", "old_blended_noise",
)
FUNC_OP = {n: i for i, n in enumerate(FUNC_OPS)}
FUNC_NODE = Layout("FuncNode", "<HHIIIffII", ("op", "flags", "a", "b", "c", "p0", "p1", "aux", "reserved"))
ROUTER_FIELDS = (
    "final_density", "temperature", "vegetation", "continents", "erosion", "depth", "ridges",
    "initial_density_without_jaggedness", "barrier", "fluid_level_floodedness",
    "fluid_level_spread", "lava", "vein_toggle", "vein_ridged", "vein_gap", "reserved",
)
ROUTER = Layout("Router", "<16I", ROUTER_FIELDS)

SURF_OPS = {
    "block": 0, "sequence": 1, "condition": 2, "bandlands": 3,
    "biome": 10, "noise_threshold": 11, "vertical_gradient": 12, "y_above": 13, "water": 14,
    "temperature": 15, "steep": 16, "not": 17, "hole": 18, "above_preliminary_surface": 19,
    "stone_depth": 20,
}
SURF_OP_NAMES = {v: k for k, v in SURF_OPS.items()}
ANCHOR_ABSOLUTE, ANCHOR_ABOVE_BOTTOM, ANCHOR_BELOW_TOP = 0, 1, 2
SURF_ADD_STONE_DEPTH = 1 << 4
SURF_CEILING = 1 << 5
SURF_NODE = Layout("SurfNode", "<HHIIffiiI", ("op", "flags", "a", "b", "p0", "p1", "i0", "i1", "aux"))
BIOME_TARGET = Layout(
    "BiomeTarget", "<I12fff",
    ("biome_str", "t_lo", "t_hi", "h_lo", "h_hi", "c_lo", "c_hi", "e_lo", "e_hi", "d_lo", "d_hi", "w_lo", "w_hi", "offset",
     "base_temperature"),
)

ALL_LAYOUTS = [
    HEADER, SECTION, STRING_REF, INFO, PARAM, LIST_REF, EXPR_NODE, ROLE, SPAN, STACK, CONSTRAINT,
    SHAPE_NODE, CHOOSE_ENTRY, PICK, PICK_ROOT, VOXEL, CONFINE, SETTINGS, NOISE, SPLINE,
    SPLINE_POINT, FUNC_NODE, ROUTER, SURF_NODE, BIOME_TARGET,
]


def align8(n: int) -> int:
    return (n + SECTION_ALIGN - 1) & ~(SECTION_ALIGN - 1)


def write_varint(out: bytearray, v: int) -> None:
    assert v >= 0
    while True:
        b = v & 0x7F
        v >>= 7
        if v:
            out.append(b | 0x80)
        else:
            out.append(b)
            return


def read_varint(buf: bytes, off: int) -> tuple[int, int]:
    shift = 0
    v = 0
    while True:
        b = buf[off]
        off += 1
        v |= (b & 0x7F) << shift
        if not (b & 0x80):
            return v, off
        shift += 7
        if shift > 35:
            raise ValueError("varint too long")
