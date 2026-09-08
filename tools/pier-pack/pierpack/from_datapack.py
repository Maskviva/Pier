"""from_datapack.py: assembles a volume pack source from a Java datapack directory.

Reads data/<ns>/worldgen/dimension/<name>.json for the noise settings id and the
multi-noise biome list, the noise_settings file, every density_function and noise file
under every namespace, and the temperature of every biome file, and writes one source
file vol_build.py can compile. A biome_source that names a preset instead of listing
its biomes is refused: presets live in the game and not in the datapack.
"""
from __future__ import annotations

import glob
import json
import os


def _resource_files(root: str, kind: str) -> dict:
    out = {}
    for ns_dir in glob.glob(os.path.join(root, "data", "*")):
        ns = os.path.basename(ns_dir)
        base = os.path.join(ns_dir, "worldgen", kind)
        for path in glob.glob(os.path.join(base, "**", "*.json"), recursive=True):
            rel = os.path.relpath(path, base).replace(os.sep, "/")[:-5]
            with open(path, encoding="utf-8") as f:
                out[f"{ns}:{rel}"] = json.load(f)
    return out


def _norm(s: str) -> str:
    return s if ":" in s else "minecraft:" + s


def assemble(root: str, dimension: str, name: str | None = None) -> dict:
    dims = _resource_files(root, "dimension")
    key = _norm(dimension)
    if key not in dims:
        raise ValueError(f"dimension '{dimension}' is not in the datapack; found: {', '.join(sorted(dims)) or 'none'}")
    dim = dims[key]
    gen = dim.get("generator", {})
    if _norm(gen.get("type", "")) != "minecraft:noise":
        raise ValueError("only a minecraft:noise generator can become a volume pack")
    settings_ref = gen.get("settings")
    settings_all = _resource_files(root, "noise_settings")
    if isinstance(settings_ref, str):
        if _norm(settings_ref) not in settings_all:
            raise ValueError(f"noise_settings '{settings_ref}' is not in the datapack")
        settings = settings_all[_norm(settings_ref)]
    elif isinstance(settings_ref, dict):
        settings = settings_ref
    else:
        raise ValueError("the dimension's generator names no settings")
    bs = gen.get("biome_source", {})
    if _norm(bs.get("type", "")) != "minecraft:multi_noise":
        raise ValueError("only a multi_noise biome source can become a volume pack")
    if "biomes" not in bs:
        raise ValueError("the biome source uses a preset; write its biome list into the dimension file first")
    biome_files = _resource_files(root, "biome")
    temperatures = {k: float(v.get("temperature", 0.5)) for k, v in biome_files.items()}
    src = dict(settings)
    src.update({
        "pier_pack": 1, "type": "volume", "name": name or key,
        "density_functions": _resource_files(root, "density_function"),
        "noises": _resource_files(root, "noise"),
        "biomes": [{"biome": _norm(b["biome"]), "parameters": b["parameters"]} for b in bs["biomes"]],
        "biome_temperatures": {_norm(b["biome"]): temperatures.get(_norm(b["biome"]), 0.5) for b in bs["biomes"]},
    })
    return src
