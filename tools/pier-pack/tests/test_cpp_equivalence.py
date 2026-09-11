"""test_cpp_equivalence.py: the C++ pack layer and the Python reference agree chunk for chunk.

Compiles packages/pier-dimensions/src/pack/*.cpp (engine-free) with the driver in
cpp_check/ under g++, then runs both sides over the plot and town fixtures, several
parameter sets and chunks at positive and negative coordinates, comparing every cell.
Skipped when no g++ is on the path, as on the Windows build machine.
"""
import os
import tempfile
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.dirname(HERE))
from pierpack.tpl_build import build_file  # noqa: E402
from pierpack.tpl_read import read_template  # noqa: E402
from pierpack import refgen_tpl as R  # noqa: E402

SRC = os.path.join(ROOT, "packages", "pier-dimensions", "src", "pack")
INC = os.path.join(ROOT, "packages", "pier-dimensions", "include")
FILES = ["Sha256.cpp", "PackReader.cpp", "Expr.cpp", "TemplatePack.cpp", "TemplateMount.cpp", "TemplateGen.cpp"]


def compile_driver():
    exe = os.path.join(tempfile.gettempdir(), "pier_pack_check")
    cmd = ["g++", "-std=c++20", "-O2", "-Wall", "-I" + INC, os.path.join(HERE, "..", "cpp_check", "main.cpp")]
    cmd += [os.path.join(SRC, f) for f in FILES] + ["-o", exe]
    subprocess.run(cmd, check=True)
    return exe


def cpp_chunk(exe, pack_path, min_y, max_y, cx, cz, params, roles):
    args = [exe, pack_path, str(min_y), str(max_y), str(cx), str(cz)]
    args += ["%s=%d" % kv for kv in params.items()] + ["role:%s=%s" % kv for kv in roles.items()]
    r = subprocess.run(args, capture_output=True, text=True)
    if r.returncode != 0:
        raise AssertionError("driver failed: " + r.stderr)
    cols = []
    for line in r.stdout.strip("\n").split("\n"):
        col = []
        for run in line.split(" "):
            name, n = run.rsplit("*", 1)
            col += [name] * int(n)
        cols.append(col)
    return cols


def test_cpp_matches_reference():
    if not shutil.which("g++"):
        print("test_cpp_equivalence: skipped, no g++")
        return
    exe = compile_driver()
    cases = [
        ("plot.json", os.path.join(tempfile.gettempdir(), "eq_plot.ptpl"), [{}, {"plot_size": 48, "road_width": 5, "border_width": 2, "floor_y": 100},
                                             {"plot_size": 16, "road_width": 0, "border_width": 0, "floor_y": 4}], {}),
        ("town.json", os.path.join(tempfile.gettempdir(), "eq_town.ptpl"), [{"road_width": 5}, {"road_width": 9, "wall_style": 1, "border_width": 2}],
         {"floor": "minecraft:stone"}),
    ]
    chunks = [(0, 0), (1, 0), (3, 4), (-1, -1), (-5, 2), (17, -9), (2, 2)]
    for src, out, param_sets, roles in cases:
        build_file(os.path.join(HERE, "..", "fixtures", src), out)
        with open(out, "rb") as f:
            pack = read_template(f.read())
        for params in param_sets:
            res = R.mount(pack, dict(params), dict(roles))
            for cx, cz in chunks:
                want = R.generate_chunk(res, cx, cz)
                got = cpp_chunk(exe, out, res.min_y, res.max_y, cx, cz, params, roles)
                assert len(got) == 256
                for i in range(256):
                    if got[i] != want[i]:
                        x, z = divmod(i, 16)
                        diffs = [(y + res.min_y, got[i][y], want[i][y]) for y in range(len(want[i])) if got[i][y] != want[i][y]]
                        raise AssertionError(f"{src} chunk ({cx},{cz}) column ({x},{z}) params {params}: {diffs[:4]}")
    # Refusals must agree too: a fixed parameter given another value, a bad role.
    r = subprocess.run([exe, os.path.join(tempfile.gettempdir(), "eq_town.ptpl"), "-64", "320", "0", "0", "plot_size=32"], capture_output=True, text=True)
    assert r.returncode == 3 and "fixed" in r.stderr, r.stderr
    r = subprocess.run([exe, os.path.join(tempfile.gettempdir(), "eq_town.ptpl"), "-64", "320", "0", "0", "role:nope=minecraft:stone"], capture_output=True, text=True)
    assert r.returncode == 3 and "unknown role" in r.stderr, r.stderr
    # A dimension that does not contain the pack's range. -64..320 is the pack's own
    # range now, so the probe has to be a range that really excludes it.
    r = subprocess.run([exe, os.path.join(tempfile.gettempdir(), "eq_town.ptpl"), "0", "320", "0", "0"], capture_output=True, text=True)
    assert r.returncode == 3 and "height" in r.stderr, r.stderr


if __name__ == "__main__":
    test_cpp_matches_reference()
    print("test_cpp_equivalence: ok")
