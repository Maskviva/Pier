"""test_vol_equivalence.py: the C++ volume layer and the Python reference agree cell by cell.

Compiles the volume half of src/pack with the driver in cpp_check/vol_main.cpp under g++,
then compares blocks and the biome of every column over the islands fixture and a legacy
nether-like fixture with end islands, for two seeds and chunks on both sides of the
origin. Skipped when no g++ is on the path.
"""
import json
import os
import tempfile
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.dirname(HERE))
from pierpack.vol_build import build_file  # noqa: E402
from pierpack.vol_read import read_volume  # noqa: E402
from pierpack import refgen_vol as RV  # noqa: E402

SRC = os.path.join(ROOT, "packages", "pier-dimensions", "src", "pack")
INC = os.path.join(ROOT, "packages", "pier-dimensions", "include")
FILES = ["Sha256.cpp", "PackReader.cpp", "JavaRandom.cpp", "VolumePack.cpp", "VolumeGen.cpp"]


def compile_driver():
    exe = os.path.join(tempfile.gettempdir(), "pier_vol_check")
    cmd = ["g++", "-std=c++20", "-O2", "-I" + INC, os.path.join(HERE, "..", "cpp_check", "vol_main.cpp")]
    cmd += [os.path.join(SRC, f) for f in FILES] + ["-o", exe]
    subprocess.run(cmd, check=True)
    return exe


def cpp_chunk(exe, pack_path, seed, cx, cz):
    r = subprocess.run([exe, pack_path, str(seed), str(cx), str(cz)], capture_output=True, text=True)
    if r.returncode != 0:
        raise AssertionError("driver failed: " + r.stderr)
    cols, biomes = [], []
    for line in r.stdout.strip("\n").split("\n"):
        biome, rest = line.split(" ", 1)
        biomes.append(int(biome))
        col = []
        for run in rest.split(" "):
            name, n = run.rsplit("*", 1)
            col += [name] * int(n)
        cols.append(col)
    return cols, biomes


def legacy_fixture(path):
    """A nether-like world: legacy random source, blended noise, end islands as a
    decoration channel, so the legacy paths and the two extra samplers run."""
    src = {
        "pier_pack": 1, "type": "volume", "name": "legacy_test", "legacy_random_source": True,
        "sea_level": 32, "default_block": {"Name": "minecraft:netherrack"}, "default_fluid": {"Name": "minecraft:lava"},
        "noise": {"min_y": 0, "height": 128, "size_horizontal": 1, "size_vertical": 2},
        "noises": {
            "minecraft:temperature": {"firstOctave": -10, "amplitudes": [1.5, 0, 1, 0, 0, 0]},
            "minecraft:vegetation": {"firstOctave": -8, "amplitudes": [1, 1, 0, 0, 0, 0]},
            "minecraft:shift": {"firstOctave": -3, "amplitudes": [1, 1, 1, 0]},
            "minecraft:surface": {"firstOctave": -6, "amplitudes": [1, 1, 1]},
            "minecraft:cave_layer": {"firstOctave": -8, "amplitudes": [1]},
        },
        "density_functions": {
            "t:base": {"type": "minecraft:old_blended_noise", "xz_scale": 0.25, "y_scale": 0.375, "xz_factor": 80, "y_factor": 60, "smear_scale_multiplier": 8},
            "t:gradient": {"type": "minecraft:y_clamped_gradient", "from_y": 0, "to_y": 128, "from_value": 1.2, "to_value": -1.2},
            "t:islands": {"type": "minecraft:end_islands"},
            "t:cave": {"type": "minecraft:weird_scaled_sampler", "input": {"type": "minecraft:noise", "noise": "minecraft:cave_layer", "xz_scale": 1, "y_scale": 1}, "noise": "minecraft:cave_layer", "rarity_value_mapper": "type_2"},
            "t:final": {"type": "minecraft:interpolated", "argument": {"type": "minecraft:min", "argument1": {"type": "minecraft:add", "argument1": "t:gradient", "argument2": {"type": "minecraft:mul", "argument1": 0.5, "argument2": "t:base"}}, "argument2": {"type": "minecraft:add", "argument1": {"type": "minecraft:cube", "argument": "t:cave"}, "argument2": {"type": "minecraft:range_choice", "input": "t:islands", "min_inclusive": -0.1, "max_exclusive": 0.6, "when_in_range": 0.1, "when_out_of_range": 0.5}}}},
        },
        "noise_router": {
            "final_density": "t:final", "initial_density_without_jaggedness": "t:gradient",
            "temperature": {"type": "minecraft:noise", "noise": "minecraft:temperature", "xz_scale": 0.25, "y_scale": 0},
            "vegetation": {"type": "minecraft:noise", "noise": "minecraft:vegetation", "xz_scale": 0.25, "y_scale": 0},
            "continents": 0, "erosion": 0, "depth": "t:gradient", "ridges": {"type": "minecraft:shift_a", "argument": "minecraft:shift"},
        },
        "surface_rule": {"type": "minecraft:sequence", "sequence": [
            {"type": "minecraft:condition", "if_true": {"type": "minecraft:vertical_gradient", "random_name": "minecraft:bedrock_floor", "true_at_and_below": {"above_bottom": 0}, "false_at_and_above": {"above_bottom": 5}}, "then_run": {"type": "minecraft:block", "result_state": {"Name": "minecraft:bedrock"}}},
            {"type": "minecraft:condition", "if_true": {"type": "minecraft:not", "invert": {"type": "minecraft:vertical_gradient", "random_name": "minecraft:bedrock_roof", "true_at_and_below": {"below_top": 5}, "false_at_and_above": {"below_top": 0}}}, "then_run": {"type": "minecraft:block", "result_state": {"Name": "minecraft:bedrock"}}},
            {"type": "minecraft:condition", "if_true": {"type": "minecraft:stone_depth", "offset": 0, "surface_type": "ceiling", "add_surface_depth": True, "secondary_depth_range": 0}, "then_run": {"type": "minecraft:condition", "if_true": {"type": "minecraft:biome", "biome_is": ["minecraft:soul_sand_valley"]}, "then_run": {"type": "minecraft:block", "result_state": {"Name": "minecraft:soul_soil"}}}},
            {"type": "minecraft:condition", "if_true": {"type": "minecraft:stone_depth", "offset": 0, "surface_type": "floor", "add_surface_depth": True, "secondary_depth_range": 0}, "then_run": {"type": "minecraft:condition", "if_true": {"type": "minecraft:water", "offset": 0, "surface_depth_multiplier": 0, "add_stone_depth": False}, "then_run": {"type": "minecraft:condition", "if_true": {"type": "minecraft:not", "invert": {"type": "minecraft:hole"}}, "then_run": {"type": "minecraft:block", "result_state": {"Name": "minecraft:gravel"}}}}},
        ]},
        "biomes": [
            {"biome": "minecraft:nether_wastes", "parameters": {"temperature": 0, "humidity": 0, "continentalness": 0, "erosion": 0, "depth": 0, "weirdness": 0, "offset": 0}},
            {"biome": "minecraft:soul_sand_valley", "parameters": {"temperature": 0, "humidity": -0.15, "continentalness": 0, "erosion": 0, "depth": 0, "weirdness": 0, "offset": 0}},
            {"biome": "minecraft:crimson_forest", "parameters": {"temperature": 0.15, "humidity": 0, "continentalness": 0, "erosion": 0, "depth": 0, "weirdness": 0, "offset": 0}},
        ],
    }
    with open(path, "w", encoding="utf-8") as f:
        json.dump(src, f)


def test_cpp_matches_reference():
    if not shutil.which("g++"):
        print("test_vol_equivalence: skipped, no g++")
        return
    exe = compile_driver()
    legacy_fixture(os.path.join(tempfile.gettempdir(), "legacy_test.json"))
    cases = [(os.path.join(HERE, "..", "fixtures", "islands.json"), os.path.join(tempfile.gettempdir(), "eq_islands.pvol")), (os.path.join(tempfile.gettempdir(), "legacy_test.json"), os.path.join(tempfile.gettempdir(), "eq_legacy.pvol"))]
    chunks = [(0, 0), (3, -2), (-7, 11)]
    for src, out in cases:
        build_file(src, out)
        with open(out, "rb") as f:
            pack = read_volume(f.read())
        for seed in (12345, 9007199254740993):
            gen = RV.Generator(pack, seed)
            for cx, cz in chunks:
                want_cols, want_biomes = gen.generate_chunk(cx, cz)
                got_cols, got_biomes = cpp_chunk(exe, out, seed, cx, cz)
                assert got_biomes == want_biomes, (src, seed, cx, cz, got_biomes[:8], want_biomes[:8])
                for i in range(256):
                    if got_cols[i] != want_cols[i]:
                        x, z = divmod(i, 16)
                        diffs = [(y + pack.settings["min_y"], got_cols[i][y], want_cols[i][y]) for y in range(len(want_cols[i])) if got_cols[i][y] != want_cols[i][y]]
                        raise AssertionError(f"{src} seed {seed} chunk ({cx},{cz}) column ({x},{z}): {diffs[:4]}")


if __name__ == "__main__":
    test_cpp_matches_reference()
    print("test_vol_equivalence: ok")
