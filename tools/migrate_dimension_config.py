#!/usr/bin/env python3
"""Migrate worlds/<level>/dimension_config.json from the pre-26.20.3 payload shapes to the
dimension spec shape. Run once, with the server stopped. Writes a .bak next to the file.

  {seed:N, generatorType:"Overworld"}   -> {seed:N, sky:{client:"overworld"}, terrain:{kind:"native", generator:"overworld"}}
  {seed:N, generatorType:"Void"}        -> {seed:N, sky:{client:"end", skylight:false, weather:false}, terrain:{kind:"native", generator:"void"}}
  {seed:N, layout:{plotSize..}}         -> left alone, and reported

A native terrain is migrated in place and generates what it generated before. A plot layout
has no in-file equivalent since 26.32.2: its terrain is a template pack now, which is a
binary the host verifies by hash, so the entry is left as it is and the steps are printed.
The dimension keeps its id either way, because the id lives in the same entry and is not
touched.
"""
import json, re, shutil, sys

PLOT_STEPS = """  a plot world is a template pack now. Three steps, with the server stopped:
    1. write the old layout into a layers spec and convert it:
       python3 tools/pier-pack/pierpack/cli.py from-layers <spec> -o source.json
       or start from tools/pier-pack/fixtures/plot.json, which is the same world
    2. python3 -m pierpack.cli build source.json -o <pack dir>/terrain.ptpl
       python3 -m pierpack.cli hash <pack dir>/terrain.ptpl   into the pack config
    3. register the same name once through md_add_dimension_pack; the id in this file is
       kept, so the chunks already generated stay where they are"""


def snbt_get(s, key, default=None):
    m = re.search(r'(?<![A-Za-z_])' + re.escape(key) + r':\s*("((?:[^"\\]|\\.)*)"|-?\d+)', s)
    if not m:
        return default
    return m.group(2) if m.group(2) is not None else int(m.group(1))


def migrate(snbt):
    """Returns the new payload, whether it changed, and a note for the operator."""
    if "terrain:" in snbt:
        return snbt, False, None
    seed = snbt_get(snbt, "seed", 0)
    if "layout:" in snbt:
        return snbt, False, PLOT_STEPS
    gen = (snbt_get(snbt, "generatorType", "Overworld") or "Overworld").lower().replace("theend", "end")
    if gen == "void":
        return '{seed:%d,sky:{client:"end",skylight:false,weather:false},terrain:{kind:"native",generator:"void"}}' % seed, True, None
    if gen == "flat":
        return '{seed:%d,terrain:{kind:"native",generator:"flat"}}' % seed, True, None
    timeless = gen in ("nether", "end")
    return '{seed:%d,sky:{client:"%s",skylight:%s,weather:%s},terrain:{kind:"native",generator:"%s"}}' % (
        seed, gen, "false" if timeless else "true", "false" if timeless else "true", gen), True, None


def main(path):
    with open(path, encoding="utf-8") as f:
        cfg = json.load(f)
    changed = 0
    left = 0
    for name, info in cfg.get("dimensionList", {}).items():
        new, did, note = migrate(info.get("sNbt", ""))
        if did:
            print(f"{name}: {info['sNbt']}\n   -> {new}")
            info["sNbt"] = new
            changed += 1
        elif note:
            print(f"{name}: left as it is,\n{note}")
            left += 1
    if not changed:
        print("nothing to migrate" if not left else f"{left} entry needs a terrain pack; see above")
        return
    shutil.copy(path, path + ".bak")
    with open(path, "w", encoding="utf-8") as f:
        json.dump(cfg, f, ensure_ascii=False, indent=2)
    print(f"{changed} entries migrated; backup at {path}.bak")
    if left:
        print(f"{left} entry left as it is and needs a terrain pack; see above")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("usage: migrate_dimension_config.py worlds/<level>/dimension_config.json")
    main(sys.argv[1])
