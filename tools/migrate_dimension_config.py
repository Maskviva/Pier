#!/usr/bin/env python3
"""Migrate worlds/<level>/dimension_config.json from the pre-26.20.3 payload shapes to the
dimension spec shape. Run once, with the server stopped. Writes a .bak next to the file.

  {seed:N, generatorType:"Overworld"}   -> {seed:N, sky:{client:"overworld"}, terrain:{kind:"native", generator:"overworld"}}
  {seed:N, generatorType:"Void"}        -> {seed:N, sky:{client:"end", skylight:false, weather:false}, terrain:{kind:"layers", layers:[]}}
  {seed:N, layout:{plotSize..}}         -> {seed:N, terrain:{kind:"layers", base_y:floorY-1, layers:[fill,floor], grid:{..., confine:true}}}

The terrain generated from the migrated spec is the same as before: the mapping is the one
the old readers used, written into the file once instead of kept in code forever.
"""
import json, re, shutil, sys

def snbt_get(s, key, default=None):
    m = re.search(r'(?<![A-Za-z_])' + re.escape(key) + r':\s*("((?:[^"\\]|\\.)*)"|-?\d+)', s)
    if not m:
        return default
    return m.group(2) if m.group(2) is not None else int(m.group(1))

def migrate(snbt):
    if "terrain:" in snbt:
        return snbt, False
    seed = snbt_get(snbt, "seed", 0)
    if "layout:" in snbt:
        lay = snbt[snbt.index("layout:"):]
        g = lambda k, d: snbt_get(lay, k, d)
        floor_y = g("floorY", 64)
        new = ('{seed:%d,terrain:{kind:"layers",base_y:%d,biome:"%s",layers:[{block:"%s",thickness:1},{block:"%s",thickness:1}],'
               'grid:{cell:%d,gap:%d,edge:%d,gap_block:"%s",edge_block:"%s",confine:true}}}') % (
            seed, floor_y - 1, g("biome", "minecraft:plains"), g("fillBlock", "minecraft:dirt"), g("floorBlock", "minecraft:grass_block"),
            g("plotSize", 64), g("roadWidth", 7), g("borderWidth", 1), g("roadBlock", "minecraft:birch_planks"), g("borderBlock", "minecraft:stone_block_slab"))
        return new, True
    gen = (snbt_get(snbt, "generatorType", "Overworld") or "Overworld").lower().replace("theend", "end")
    if gen == "void":
        return '{seed:%d,sky:{client:"end",skylight:false,weather:false},terrain:{kind:"layers",base_y:63,biome:"minecraft:plains",layers:[]}}' % seed, True
    if gen == "flat":
        return '{seed:%d,terrain:{kind:"native",generator:"flat"}}' % seed, True
    timeless = gen in ("nether", "end")
    return '{seed:%d,sky:{client:"%s",skylight:%s,weather:%s},terrain:{kind:"native",generator:"%s"}}' % (
        seed, gen, "false" if timeless else "true", "false" if timeless else "true", gen), True

def main(path):
    with open(path, encoding="utf-8") as f:
        cfg = json.load(f)
    changed = 0
    for name, info in cfg.get("dimensionList", {}).items():
        new, did = migrate(info.get("sNbt", ""))
        if did:
            print(f"{name}: {info['sNbt']}\n   -> {new}")
            info["sNbt"] = new
            changed += 1
    if not changed:
        print("nothing to migrate")
        return
    shutil.copy(path, path + ".bak")
    with open(path, "w", encoding="utf-8") as f:
        json.dump(cfg, f, ensure_ascii=False, indent=2)
    print(f"{changed} entries migrated; backup at {path}.bak")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("usage: migrate_dimension_config.py worlds/<level>/dimension_config.json")
    main(sys.argv[1])
