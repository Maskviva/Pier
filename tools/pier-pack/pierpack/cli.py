"""cli.py: the pier-pack command line.

    pier-pack build   <source.json> -o <out.ptpl|out.pvol>
    pier-pack inspect <pack>              the same JSON md_pack_inspect returns
    pier-pack hash    <pack>              the sha256 for the config file
    pier-pack from-layers <spec.snbt|json> -o <source.json>
                                          an old {kind:"layers"} spec as a template source
    pier-pack from-datapack <dir> <dimension id> -o <source.json>
                                          a Java datapack's noise dimension as a volume source
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys

from . import format as F
from .container import PackError, Reader


def inspect_json(data: bytes) -> dict:
    r = Reader(data)
    out = {"kind": r.kind, "format_version": F.FORMAT_VERSION, "sha256": r.file_sha256(),
           "sections": [{"type": F.tag_name(t), "bytes": len(b)} for t, (v, b) in r.sections.items()]}
    info = F.INFO.unpack(r.section(F.SEC_INFO), 0)
    out["info"] = {"tool": r.str(info["tool_version_str"]), "built_at": r.str(info["built_at_str"]),
                   "name": r.str(info["source_name_str"]), "biome": r.str(info["biome_str"])}
    out["height"] = {"min": info["height_min"], "max": info["height_max"],
                     "fixed": bool(info["flags"] & F.INFO_HEIGHT_FIXED)}
    if r.kind == "template":
        from .tpl_read import read_template
        pack = read_template(data)
        params = []
        for p in pack.params:
            e = {"name": p["name"], "kind": p["kind"]}
            if p["kind"] == "free":
                e.update(default=p["default"], min=p["min"], max=p["max"], step=p["step"])
            elif p["kind"] == "fixed":
                e["value"] = p["default"]
            elif p["kind"] == "choice":
                e.update(default=p["default"], choices=pack.choice_lists[p["aux"]])
            params.append(e)
        out["params"] = params
        out["roles"] = [{"name": r_["name"], "default": r_["block"]} for r_ in pack.roles]
        out["zones"] = pack.zone_names
        out["constraints"] = [c["message"] for c in pack.constraints]
        out["shapes"] = len(pack.shapes)
        out["picks"] = len(pack.picks)
        out["voxels"] = [{"size": [v["sx"], v["sy"], v["sz"]], "palette": len(v["palette"])} for v in pack.voxels]
        out["confine"] = pack.confine is not None
    else:
        from .vol_read import read_volume
        pack = read_volume(data)
        out.update(pack.summary())
    return out


def cmd_build(args):
    with open(args.source, encoding="utf-8") as f:
        src = json.load(f)
    kind = src.get("type", "template")
    if kind == "template":
        from .tpl_build import build_file
    elif kind == "volume":
        from .vol_build import build_file
    else:
        print(f"unknown pack type '{kind}'", file=sys.stderr)
        return 2
    data = build_file(args.source, args.output)
    print(f"{args.output}: {len(data)} bytes, sha256 {hashlib.sha256(data).hexdigest()}")
    return 0


def cmd_inspect(args):
    with open(args.pack, "rb") as f:
        data = f.read()
    print(json.dumps(inspect_json(data), indent=2, ensure_ascii=False))
    return 0


def cmd_hash(args):
    with open(args.pack, "rb") as f:
        print(hashlib.sha256(f.read()).hexdigest())
    return 0


def cmd_from_layers(args):
    from .from_layers import convert
    with open(args.spec, encoding="utf-8") as f:
        text = f.read()
    src = convert(text, name=args.name)
    with open(args.output, "w", encoding="utf-8") as f:
        json.dump(src, f, indent=1, ensure_ascii=False)
    print(f"{args.output}: template source written")
    return 0


def cmd_from_datapack(args):
    from .from_datapack import assemble
    src = assemble(args.datapack, args.dimension, name=args.name)
    with open(args.output, "w", encoding="utf-8") as f:
        json.dump(src, f, indent=1, ensure_ascii=False)
    print(f"{args.output}: volume source written ({len(src['density_functions'])} density functions, {len(src['noises'])} noises, {len(src['biomes'])} biomes)")
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="pier-pack")
    sub = ap.add_subparsers(dest="cmd", required=True)
    b = sub.add_parser("build")
    b.add_argument("source")
    b.add_argument("-o", "--output", required=True)
    b.set_defaults(fn=cmd_build)
    i = sub.add_parser("inspect")
    i.add_argument("pack")
    i.set_defaults(fn=cmd_inspect)
    h = sub.add_parser("hash")
    h.add_argument("pack")
    h.set_defaults(fn=cmd_hash)
    fl = sub.add_parser("from-layers")
    fl.add_argument("spec")
    fl.add_argument("-o", "--output", required=True)
    fl.add_argument("--name", default="layers")
    fl.set_defaults(fn=cmd_from_layers)
    fd = sub.add_parser("from-datapack")
    fd.add_argument("datapack")
    fd.add_argument("dimension")
    fd.add_argument("-o", "--output", required=True)
    fd.add_argument("--name")
    fd.set_defaults(fn=cmd_from_datapack)
    args = ap.parse_args(argv)
    try:
        return args.fn(args)
    except (PackError, OSError, ValueError) as e:
        print(f"pier-pack: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
