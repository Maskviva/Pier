"""test_town.py: the 3D path of the reference generator on the town fixture.

Covers voxel blobs with keep and air cells, shell, cylinder, union and difference,
choose by parameter and by cell hash, and the four turns of a pick, which must keep the
number of solid cells of a blob.
"""
import os
import tempfile
import sys
from collections import Counter

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
from pierpack.tpl_build import build_file  # noqa: E402
from pierpack.tpl_read import read_template  # noqa: E402
from pierpack import refgen_tpl as R  # noqa: E402
from pierpack import format as F  # noqa: E402


def load():
    out = os.path.join(tempfile.gettempdir(), "town_test.ptpl")
    build_file(os.path.join(HERE, "..", "fixtures", "town.json"), out)
    with open(out, "rb") as f:
        return read_template(f.read())


def test_town_materials():
    pack = load()
    res = R.mount(pack, {"road_width": 5}, {})
    cnt = Counter(b for cx in range(8) for cz in range(8) for col in R.generate_chunk(res, cx, cz) for b in col)
    for b in ("minecraft:oak_planks", "minecraft:oak_slab", "minecraft:glass", "minecraft:cobblestone",
              "minecraft:stone_bricks", "minecraft:water", "minecraft:birch_planks", "minecraft:stone_block_slab"):
        assert cnt.get(b, 0) > 0, b
    assert cnt.get("minecraft:oak_fence", 0) == 0
    res2 = R.mount(pack, {"road_width": 5, "wall_style": 1}, {})
    cnt2 = Counter(b for cx in range(4) for cz in range(4) for col in R.generate_chunk(res2, cx, cz) for b in col)
    assert cnt2.get("minecraft:cobblestone", 0) == 0 and cnt2.get("minecraft:oak_fence", 0) > 0


def test_turns_keep_solid_count():
    pack = load()
    res = R.mount(pack, {"road_width": 5}, {})
    hut = pack.voxels[0]
    solid = sum(1 for c in hut["cells"] if c != F.VOXEL_KEEP)
    idx = [i for i, s in enumerate(pack.shapes) if F.SHAPE_OPS[s["op"]] == "voxels"][0]
    ctx = R._Ctx(res, 0, 0)
    for turn in range(4):
        rb = R.rotated_bbox(R.Box(0, 0, 0, hut["sx"], hut["sy"], hut["sz"]), turn, 20, 20)
        n = 0
        for u in range(rb.x0, rb.x1):
            for v in range(rb.z0, rb.z1):
                cu, cv = {0: (u, v), 1: (v, 19 - u), 2: (19 - u, 19 - v), 3: (19 - v, u)}[turn]
                for y in range(hut["sy"]):
                    if R.eval_shape(ctx, idx, cu, y, cv) is not None:
                        n += 1
        assert n == solid, (turn, n, solid)


def test_fixed_param_refused():
    pack = load()
    try:
        R.mount(pack, {"plot_size": 32}, {})
    except R.MountError as e:
        assert "fixed" in str(e)
    else:
        raise AssertionError("a fixed parameter accepted another value")


if __name__ == "__main__":
    test_town_materials()
    test_turns_keep_solid_count()
    test_fixed_param_refused()
    print("test_town: ok")
