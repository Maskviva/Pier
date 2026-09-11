"""test_plot_equivalence.py: the plot fixture reproduces the 26.20.2 PlotGenerator.

The generator is ported line for line from PlotGenerator.cpp and plot_layout.h of Pier
26.20.2 into `plot_generator_chunk`; the template pack built from fixtures/plot.json
must yield the same block in every cell of several chunks, including chunks that
straddle a road and chunks at negative coordinates.
"""
import os
import tempfile
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
from pierpack.tpl_build import build_file  # noqa: E402
from pierpack.tpl_read import read_template  # noqa: E402
from pierpack import refgen_tpl as R  # noqa: E402

# Pier 的 dimension_height.h 说了算。底原本是 -512，2026-09-10 收回原版的 -64。
K_MIN_Y, K_MAX_Y, K_BEDROCK_Y = -64, 320, -64


def classify1d(offset, plot_size, border_width):
    if offset >= plot_size:
        return "road"
    if border_width > 0 and (offset < border_width or offset >= plot_size - border_width):
        return "border"
    return "plot"


def combine2d(x, z):
    if x == "road" or z == "road":
        return "road"
    if x == "plot" and z == "plot":
        return "plot"
    return "border"


def plot_generator_chunk(cx, cz, plot_size=64, road_width=7, border_width=1, floor_y=64,
                         floor_block="minecraft:grass_block", fill_block="minecraft:dirt",
                         road_block="minecraft:birch_planks", border_block="minecraft:stone_block_slab"):
    height = K_MAX_Y - K_MIN_Y
    bedrock_idx = K_BEDROCK_Y - K_MIN_Y
    floor_idx = floor_y - K_MIN_Y
    border_idx = floor_idx + 1
    cell = plot_size + road_width
    cols = []
    for x in range(16):
        ix = (cx * 16 + x) % cell
        ax = classify1d(ix, plot_size, border_width)
        for z in range(16):
            iz = (cz * 16 + z) % cell
            az = classify1d(iz, plot_size, border_width)
            col = ["minecraft:air"] * height
            col[bedrock_idx] = "minecraft:bedrock"
            for y in range(bedrock_idx + 1, floor_idx):
                col[y] = fill_block
            col[floor_idx] = floor_block
            area = combine2d(ax, az)
            if area == "road":
                col[floor_idx] = road_block
            elif area == "border":
                col[border_idx] = border_block
            cols.append(col)
    return cols


def test_plot_equivalence():
    out = os.path.join(tempfile.gettempdir(), "plot_eq.ptpl")
    build_file(os.path.join(HERE, "..", "fixtures", "plot.json"), out)
    with open(out, "rb") as f:
        pack = read_template(f.read())
    cases = [
        dict(plot_size=64, road_width=7, border_width=1, floor_y=64),
        dict(plot_size=48, road_width=5, border_width=2, floor_y=100),
        dict(plot_size=16, road_width=0, border_width=0, floor_y=4),
    ]
    chunks = [(0, 0), (3, 4), (4, 4), (-1, -1), (-5, 2), (17, -9)]
    for params in cases:
        res = R.mount(pack, dict(params), {})
        assert res.height == K_MAX_Y - K_MIN_Y
        for cx, cz in chunks:
            got = R.generate_chunk(res, cx, cz)
            want = plot_generator_chunk(cx, cz, **params)
            for i in range(256):
                if got[i] != want[i]:
                    x, z = divmod(i, 16)
                    diff = [(y + K_MIN_Y, got[i][y], want[i][y]) for y in range(len(want[i])) if got[i][y] != want[i][y]]
                    raise AssertionError(f"chunk ({cx},{cz}) column ({x},{z}) params {params}: first diffs {diff[:3]}")
    # Static layer extraction must hand refillStatic exactly the layers PlotGenerator fills once.
    res = R.mount(pack, cases[0], {})
    floor_idx = 64 - K_MIN_Y
    assert floor_idx not in res.static_layers and floor_idx + 1 not in res.static_layers
    assert all(y in res.static_layers for y in range(res.height) if y not in (floor_idx, floor_idx + 1))


def test_constraints_and_kinds():
    out = os.path.join(tempfile.gettempdir(), "plot_eq.ptpl")
    build_file(os.path.join(HERE, "..", "fixtures", "plot.json"), out)
    with open(out, "rb") as f:
        pack = read_template(f.read())
    for bad, msg in [
        (dict(plot_size=8, border_width=4), "half"),
        (dict(floor_y=-64), "outside"),
        (dict(plot_size=3), "outside"),
        (dict(plot_depth=10), "derived"),
        (dict(nope=1), "unknown parameters"),
    ]:
        try:
            R.mount(pack, bad, {})
        except R.MountError as e:
            assert msg in str(e), (bad, str(e))
        else:
            raise AssertionError("mount accepted %r" % (bad,))
    res = R.mount(pack, {}, {"floor": "minecraft:stone"})
    assert res.role_blocks[pack.roles.index(next(r for r in pack.roles if r["name"] == "floor"))] == "minecraft:stone"
    assert res.values[pack.param_index("plot_depth")] == 64


if __name__ == "__main__":
    test_plot_equivalence()
    test_constraints_and_kinds()
    print("test_plot_equivalence: ok")
