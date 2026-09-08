"""test_layout.py: the Python layouts and pack_format.h agree on every struct size.

The header states each size in a static_assert next to the struct; this test reads them
out with a regular expression and compares with struct.calcsize on the Python side.
"""
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
from pierpack import format as F  # noqa: E402

HEADER = os.path.normpath(os.path.join(
    HERE, "..", "..", "..", "packages", "pier-dimensions", "include", "pier", "dimensions", "pack", "pack_format.h"))


def cpp_sizes():
    with open(HEADER, encoding="utf-8") as f:
        src = f.read()
    sizes = {}
    for m in re.finditer(r"static_assert\(sizeof\((\w+)\) == (\w+)\)", src):
        name, val = m.group(1), m.group(2)
        if val.startswith("k"):
            val = {"kHeaderSize": 32, "kSectionEntrySize": 56}[val]
        sizes[name] = int(val)
    return sizes


def test_sizes_match():
    sizes = cpp_sizes()
    assert sizes, "no static_asserts found in " + HEADER
    missing = []
    for lay in F.ALL_LAYOUTS:
        if lay.name not in sizes:
            missing.append(lay.name)
            continue
        assert lay.size == sizes[lay.name], f"{lay.name}: py={lay.size} cpp={sizes[lay.name]}"
    assert not missing, "structs without a static_assert in the header: %s" % missing
    assert set(sizes) == {l.name for l in F.ALL_LAYOUTS}, "header structs not mirrored: %s" % (
        set(sizes) - {l.name for l in F.ALL_LAYOUTS})


def test_opcode_tables_match_header():
    with open(HEADER, encoding="utf-8") as f:
        src = f.read()

    def enum_body(name):
        m = re.search(r"enum class %s : std::uint16_t\s*\{(.*?)\};" % name, src, re.S)
        assert m, name
        out = {}
        for line in m.group(1).splitlines():
            line = line.strip().rstrip(",")
            if "=" in line and not line.startswith("//"):
                k, v = [x.strip() for x in line.split("=")]
                out[k] = int(v)
        return out

    expr = enum_body("ExprOp")
    assert expr["Count_"] == len(F.EXPR_OPS)
    shape = enum_body("ShapeOp")
    assert shape["Count_"] == len(F.SHAPE_OPS)
    func = enum_body("FuncOp")
    assert func["Count_"] == len(F.FUNC_OPS)
    surf = enum_body("SurfOp")
    py_surf = {k: v for k, v in F.SURF_OPS.items()}
    cpp_surf = {}
    camel = {"Block": "block", "Sequence": "sequence", "Condition": "condition", "Bandlands": "bandlands",
             "Biome": "biome", "NoiseThreshold": "noise_threshold", "VerticalGradient": "vertical_gradient",
             "YAbove": "y_above", "Water": "water", "Temperature": "temperature", "Steep": "steep",
             "Not": "not", "Hole": "hole", "AbovePreliminarySurface": "above_preliminary_surface",
             "StoneDepth": "stone_depth"}
    for k, v in surf.items():
        cpp_surf[camel[k]] = v
    assert cpp_surf == py_surf


def test_varint_roundtrip():
    for v in (0, 1, 127, 128, 300, 65535, 2**31 - 1):
        b = bytearray()
        F.write_varint(b, v)
        got, off = F.read_varint(bytes(b), 0)
        assert got == v and off == len(b)


if __name__ == "__main__":
    test_sizes_match()
    test_opcode_tables_match_header()
    test_varint_roundtrip()
    print("test_layout: ok")
