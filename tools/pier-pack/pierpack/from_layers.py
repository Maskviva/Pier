"""from_layers.py: converts a pre-pack {kind:"layers"} spec into a template pack source.

The old spec held base_y, a biome, a stack of {block, thickness} and an optional grid of
{cell, gap, edge, gap_block, edge_block, confine}. The same terrain as a template source:
one role per distinct block, spans of edge / interior / edge / gap, and a stack per zone.
Only the terrain moves; seed, height and sky stay in the dimension spec the caller passes
with the pack.
"""
from __future__ import annotations

import json
import re

# 和 Pier 的 dimension_height.h 同源。底原本是 -512，2026-09-10 收回原版的 -64：
# 一个 52 子区块高的维度超出引擎发布过的任何形状。这里跟着改，否则转出来的包
# 声明的范围装不进任何维度，宿主以 Height 拒绝。
WORLD_MIN_Y, WORLD_MAX_Y, BEDROCK_Y = -64, 320, -64


def _snbt_to_json(text: str) -> dict:
    """Enough SNBT for the old spec shape: quoted keys and strings, ints, floats, lists,
    compounds, booleans, and unquoted keys."""
    t = text.strip()
    if t.startswith("{") and '"' in t.split("{", 1)[1][:2]:
        try:
            return json.loads(t)
        except json.JSONDecodeError:
            pass
    t = re.sub(r"([{\[,]\s*)([A-Za-z_][A-Za-z0-9_]*)\s*:", r'\1"\2":', t)
    t = re.sub(r"(\d)[bBsSlLfFdD]\b", r"\1", t)
    t = t.replace("'", '"')
    return json.loads(t)


def convert(text: str, name: str = "layers") -> dict:
    spec = _snbt_to_json(text)
    terrain = spec.get("terrain", spec)
    if terrain.get("kind", "layers") != "layers":
        raise ValueError("this spec is not a layers spec")
    base_y = int(terrain.get("base_y", 63))
    biome = terrain.get("biome", "minecraft:plains")
    layers = [(str(l.get("block", "minecraft:stone")), int(l.get("thickness", 1))) for l in terrain.get("layers", [])]
    grid = terrain.get("grid")
    roles = {"bedrock": "minecraft:bedrock"}
    stack_names = []
    for i, (block, _) in enumerate(layers):
        rn = "layer_%d" % i
        roles[rn] = block
        stack_names.append(rn)
    params = {"base_y": {"kind": "free", "default": base_y, "min": BEDROCK_Y + 1, "max": WORLD_MAX_Y - 2}}
    stacks = [
        {"zone": "*", "from": BEDROCK_Y, "to": BEDROCK_Y + 1, "role": "bedrock"},
        {"zone": "*", "from": BEDROCK_Y + 1, "to": "base_y", "role": "air"},
    ]
    y = "base_y"
    for i, (_, thickness) in enumerate(layers):
        stacks.append({"zone": "*", "from": y, "to": f"{y} + {thickness}", "role": stack_names[i]})
        y = f"{y} + {thickness}"
    surface = f"{y} - 1" if layers else "base_y - 1"
    src = {"pier_pack": 1, "type": "template", "name": name,
           "height": {"min": WORLD_MIN_Y, "max": WORLD_MAX_Y, "fixed": False},
           "params": params, "roles": roles, "biome": biome}
    if grid:
        cell, gap, edge = int(grid.get("cell", 64)), int(grid.get("gap", 7)), int(grid.get("edge", 1))
        roles["gap"] = grid.get("gap_block", "minecraft:birch_planks")
        roles["edge"] = grid.get("edge_block", "minecraft:stone_block_slab")
        params["cell"] = {"kind": "free", "default": cell, "min": 4, "max": 512}
        params["gap"] = {"kind": "free", "default": gap, "min": 0, "max": 64}
        params["edge"] = {"kind": "free", "default": edge, "min": 0, "max": 256}
        src.update({
            "zones": ["interior", "edge", "gap"],
            "period": {"x": "cell + gap", "z": "cell + gap"},
            "spans": {a: [["edge", "edge"], ["interior", "cell - 2*edge"], ["edge", "edge"], ["gap", "gap"]] for a in ("x", "z")},
            "combine": {"interior": {"interior": "interior", "edge": "edge", "gap": "gap"},
                        "edge": {"interior": "edge", "edge": "edge", "gap": "gap"},
                        "gap": {"interior": "gap", "edge": "gap", "gap": "gap"}},
            "constraints": [{"expr": "edge*2 < cell", "message": "edge must be less than half of cell"}],
            "confine": {"enabled": bool(grid.get("confine", False)), "cell": "cell", "gap": "gap"},
        })
        stacks.append({"zone": "gap", "from": surface, "to": f"{surface} + 1", "role": "gap"})
        stacks.append({"zone": "edge", "from": f"{surface} + 1", "to": f"{surface} + 2", "role": "edge"})
    else:
        src.update({"zones": ["all"], "period": {"x": 1, "z": 1},
                    "spans": {"x": [["all", 1]], "z": [["all", 1]]},
                    "combine": {"all": {"all": "all"}}})
    src["stacks"] = stacks
    return src
