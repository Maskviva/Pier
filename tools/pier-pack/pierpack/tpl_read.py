"""tpl_read.py: decodes a PIERTPL file into a plain model and validates its invariants.

Everything checked here is checked again by the C++ reader: operand indices smaller than
the node's own index, zone and role and string indices in range, list references inside
their arrays, voxel data and column tables inside the section. A pack that passes here
and fails there points at a reader difference, not at the pack.
"""
from __future__ import annotations

import struct
from dataclasses import dataclass, field

from . import format as F
from .container import PackError, Reader, read_array, read_u32s


@dataclass
class TemplatePack:
    info: dict
    strings: list
    params: list
    choice_lists: list
    expr: list
    roles: list
    zone_names: list
    period_x: int
    period_z: int
    spans_x: list
    spans_z: list
    combine: list
    stacks: list
    constraints: list
    shapes: list = field(default_factory=list)
    choose_lists: list = field(default_factory=list)
    picks: list = field(default_factory=list)
    voxels: list = field(default_factory=list)
    confine: dict | None = None
    file_sha256: str = ""

    @property
    def zone_count(self):
        return len(self.zone_names)

    def param_index(self, name: str) -> int:
        for i, p in enumerate(self.params):
            if p["name"] == name:
                return i
        raise KeyError(name)


def _check_index(v, limit, what):
    if v != F.NONE and v >= limit:
        raise PackError(f"{what} index {v} is out of range (limit {limit})")


def read_template(data: bytes) -> TemplatePack:
    r = Reader(data)
    if r.kind != "template":
        raise PackError("this file is a volume pack, not a template pack")
    strings = r.strings
    info = F.INFO.unpack(r.section(F.SEC_INFO), 0)
    info = {"tool_version": r.str(info["tool_version_str"]), "built_at": r.str(info["built_at_str"]),
            "source_name": r.str(info["source_name_str"]), "height_min": info["height_min"],
            "height_max": info["height_max"], "height_fixed": bool(info["flags"] & F.INFO_HEIGHT_FIXED),
            "biome": r.str(info["biome_str"])}
    if info["height_min"] % 16 or info["height_max"] % 16 or info["height_min"] >= info["height_max"]:
        raise PackError("INFO height is not a non-empty range on subchunk boundaries")

    body = r.section(F.SEC_EXPR)
    (n,) = struct.unpack_from("<I", body, 0)
    expr, _ = read_array(body, 4, F.EXPR_NODE, n)
    for i, e in enumerate(expr):
        if e["op"] >= len(F.EXPR_OPS):
            raise PackError(f"EXPR node {i}: unknown op {e['op']}")
        for k in ("a", "b"):
            if e[k] != F.NONE and e[k] >= i:
                raise PackError(f"EXPR node {i}: operand {k}={e[k]} is not smaller than the node index")

    body = r.section(F.SEC_PCHC)
    (n,) = struct.unpack_from("<I", body, 0)
    refs, off = read_array(body, 4, F.LIST_REF, n)
    total = sum(x["count"] for x in refs)
    if off + 4 * total > len(body):
        raise PackError("PCHC values run past the section")
    vals = list(struct.unpack_from("<%di" % total, body, off))
    choice_lists = [vals[x["first"]:x["first"] + x["count"]] for x in refs]

    body = r.section(F.SEC_PARM)
    (n,) = struct.unpack_from("<I", body, 0)
    raw, _ = read_array(body, 4, F.PARAM, n)
    params = []
    for i, p in enumerate(raw):
        kind = F.PARAM_KIND_NAMES.get(p["kind"])
        if kind is None:
            raise PackError(f"PARM {i}: unknown kind {p['kind']}")
        if kind == "choice":
            _check_index(p["aux"], len(choice_lists), f"PARM {i} choice list")
        if kind == "derived":
            _check_index(p["aux"], len(expr), f"PARM {i} derived expression")
            if p["aux"] == F.NONE:
                raise PackError(f"PARM {i}: derived without an expression")
        if kind == "free" and p["step"] < 1:
            raise PackError(f"PARM {i}: step {p['step']} is below 1")
        params.append({"name": r.str(p["name_str"]), "kind": kind, "default": p["def"], "min": p["min"],
                       "max": p["max"], "step": p["step"], "aux": p["aux"]})
    for i, p in enumerate(params):
        if p["kind"] == "derived":
            # A derived parameter must not depend, directly or through another derived
            # parameter, on itself. The order is fixed: an expression may only use
            # parameters declared before the derived one.
            for e in expr[:p["aux"] + 1]:
                if F.EXPR_OPS[e["op"]] == "param" and e["imm"] >= i and _reaches(expr, p["aux"], e):
                    raise PackError(f"PARM {p['name']}: a derived parameter uses a parameter declared at or after it")

    body = r.section(F.SEC_ROLE)
    (n,) = struct.unpack_from("<I", body, 0)
    raw, _ = read_array(body, 4, F.ROLE, n)
    roles = [{"name": r.str(x["name_str"]), "block": r.str(x["default_block_str"])} for x in raw]

    body = r.section(F.SEC_ZONE)
    (zn,) = struct.unpack_from("<I", body, 0)
    if zn == 0:
        raise PackError("ZONE: no zones")
    names, off = read_u32s(body, 4, zn)
    zone_names = [r.str(s) for s in names]
    (px, pz) = struct.unpack_from("<II", body, off)
    off += 8
    _check_index(px, len(expr), "ZONE period x")
    _check_index(pz, len(expr), "ZONE period z")
    (sx,) = struct.unpack_from("<I", body, off)
    spans_x, off = read_array(body, off + 4, F.SPAN, sx)
    (sz,) = struct.unpack_from("<I", body, off)
    spans_z, off = read_array(body, off + 4, F.SPAN, sz)
    if not spans_x or not spans_z:
        raise PackError("ZONE: an axis has no spans")
    for s in spans_x + spans_z:
        _check_index(s["zone"], zn, "ZONE span zone")
        _check_index(s["len_expr"], len(expr), "ZONE span length")
    combine, off = read_u32s(body, off, zn * zn)
    for c in combine:
        _check_index(c, zn, "ZONE combine")
        if c == F.NONE:
            raise PackError("ZONE: combine has an empty cell")

    body = r.section(F.SEC_STAK)
    (n,) = struct.unpack_from("<I", body, 0)
    stacks, _ = read_array(body, 4, F.STACK, n)
    for i, s in enumerate(stacks):
        _check_index(s["zone"], zn, f"STAK {i} zone")
        _check_index(s["from_expr"], len(expr), f"STAK {i} from")
        _check_index(s["to_expr"], len(expr), f"STAK {i} to")
        _check_index(s["role"], len(roles), f"STAK {i} role")

    body = r.section(F.SEC_CNST)
    (n,) = struct.unpack_from("<I", body, 0)
    raw, _ = read_array(body, 4, F.CONSTRAINT, n)
    constraints = []
    for c in raw:
        _check_index(c["expr"], len(expr), "CNST expression")
        constraints.append({"expr": c["expr"], "message": r.str(c["message_str"])})

    pack = TemplatePack(info=info, strings=strings, params=params, choice_lists=choice_lists, expr=expr, roles=roles,
                        zone_names=zone_names, period_x=px, period_z=pz, spans_x=spans_x, spans_z=spans_z,
                        combine=combine, stacks=stacks, constraints=constraints, file_sha256=r.file_sha256())

    if r.has(F.SEC_VOXL):
        pack.voxels = _read_voxels(r, r.section(F.SEC_VOXL))

    if r.has(F.SEC_SHAP):
        body = r.section(F.SEC_SHAP)
        (n,) = struct.unpack_from("<I", body, 0)
        shapes, off = read_array(body, 4, F.SHAPE_NODE, n)
        (ln,) = struct.unpack_from("<I", body, off)
        refs, off = read_array(body, off + 4, F.LIST_REF, ln)
        total = sum(x["count"] for x in refs)
        entries, off = read_array(body, off, F.CHOOSE_ENTRY, total)
        choose_lists = [entries[x["first"]:x["first"] + x["count"]] for x in refs]
        for i, s in enumerate(shapes):
            if s["op"] >= len(F.SHAPE_OPS):
                raise PackError(f"SHAP node {i}: unknown op {s['op']}")
            op = F.SHAPE_OPS[s["op"]]
            for k in ("a", "b"):
                if s[k] != F.NONE and s[k] >= i:
                    raise PackError(f"SHAP node {i}: operand {k}={s[k]} is not smaller than the node index")
            for bit, k in ((1, "p0"), (2, "p1"), (4, "p2")):
                if s["flags"] & bit:
                    _check_index(s[k], len(expr), f"SHAP node {i} {k} expression")
            if op == "paint":
                _check_index(s["role"], len(roles), f"SHAP node {i} role")
            if op == "choose":
                _check_index(s["aux"], len(choose_lists), f"SHAP node {i} choose list")
                if not choose_lists[s["aux"]]:
                    raise PackError(f"SHAP node {i}: empty choose list")
                for e in choose_lists[s["aux"]]:
                    if e["node"] >= i:
                        raise PackError(f"SHAP node {i}: choose entry {e['node']} is not smaller than the node index")
                if s["p0"] == 1:
                    _check_index(s["p1"], len(params), f"SHAP node {i} choose parameter")
                    if params[s["p1"]]["kind"] != "choice":
                        raise PackError(f"SHAP node {i}: choose by a parameter that is not a choice")
            if op == "voxels":
                _check_index(s["aux"], len(pack.voxels), f"SHAP node {i} voxel blob")
            if op in ("translate", "rotate_y", "mirror", "repeat", "shell", "paint", "union", "difference", "intersect") and s["a"] == F.NONE:
                raise PackError(f"SHAP node {i}: {op} has no operand a")
            if op in ("union", "difference", "intersect") and s["b"] == F.NONE:
                raise PackError(f"SHAP node {i}: {op} has no operand b")
        pack.shapes = shapes
        pack.choose_lists = choose_lists

    if r.has(F.SEC_PICK):
        body = r.section(F.SEC_PICK)
        (n,) = struct.unpack_from("<I", body, 0)
        picks, off = read_array(body, 4, F.PICK, n)
        total = sum(p["root_count"] for p in picks)
        roots, off = read_array(body, off, F.PICK_ROOT, total)
        for i, p in enumerate(picks):
            for k in ("x0_expr", "z0_expr", "w_expr", "d_expr", "anchor_y_expr"):
                _check_index(p[k], len(expr), f"PICK {i} {k}")
            if p["root_count"] == 0 or p["root_first"] + p["root_count"] > len(roots):
                raise PackError(f"PICK {i}: roots out of range")
            p["roots"] = roots[p["root_first"]:p["root_first"] + p["root_count"]]
            for rt in p["roots"]:
                _check_index(rt["shape_node"], len(pack.shapes), f"PICK {i} root shape")
                if rt["rot_mask"] & ~0b1111 or rt["rot_mask"] == 0:
                    raise PackError(f"PICK {i}: rot mask {rt['rot_mask']} is not a non-empty subset of the four turns")
        pack.picks = picks

    if r.has(F.SEC_CONF):
        c = F.CONFINE.unpack(r.section(F.SEC_CONF), 0)
        _check_index(c["cell_expr"], len(expr), "CONF cell")
        _check_index(c["gap_expr"], len(expr), "CONF gap")
        pack.confine = c
    return pack


def _reaches(expr, root, target_node):
    """Whether the expression subtree at root uses target_node."""
    stack = [root]
    seen = set()
    while stack:
        i = stack.pop()
        if i == F.NONE or i in seen:
            continue
        seen.add(i)
        if expr[i] is target_node:
            return True
        stack.append(expr[i]["a"])
        stack.append(expr[i]["b"])
    return False


def _read_voxels(r: Reader, body: bytes) -> list:
    (n,) = struct.unpack_from("<I", body, 0)
    entries, off = read_array(body, 4, F.VOXEL, n)
    total = sum(e["pal_count"] for e in entries)
    refs, off = read_u32s(body, off, total)
    out = []
    for i, e in enumerate(entries):
        pal = [r.str(s) for s in refs[e["pal_first"]:e["pal_first"] + e["pal_count"]]]
        if len(pal) < 2 or pal[0] != "" or pal[1] != "minecraft:air":
            raise PackError(f"VOXL {i}: palette must start with keep and air")
        if e["sx"] == 0 or e["sy"] == 0 or e["sz"] == 0:
            raise PackError(f"VOXL {i}: empty size")
        cells = _decode(body, e, "main", pal, i)
        liquid = _decode(body, e, "liq", pal, i) if e["has_liquid"] else None
        out.append({"sx": e["sx"], "sy": e["sy"], "sz": e["sz"], "palette": pal, "cells": cells, "liquid": liquid})
    return out


def _decode(body: bytes, e: dict, key: str, pal: list, i: int) -> list:
    sx, sy, sz = e["sx"], e["sy"], e["sz"]
    n = sx * sy * sz
    off, ln = e[key + "_off"], e[key + "_len"]
    if off + ln > len(body):
        raise PackError(f"VOXL {i}: {key} data runs past the section")
    blob = body[off:off + ln]
    if e["encoding"] == F.VOXEL_RAW:
        if ln != 2 * n:
            raise PackError(f"VOXL {i}: raw {key} data has the wrong length")
        cells = list(struct.unpack("<%dH" % n, blob))
    elif e["encoding"] == F.VOXEL_RLE:
        col_off = e["col_off"] if key == "main" else e["liq_col_off"]
        if col_off + 4 * sx * sz > len(body):
            raise PackError(f"VOXL {i}: {key} column table runs past the section")
        cols = struct.unpack_from("<%dI" % (sx * sz), body, col_off)
        cells = [0] * n
        for x in range(sx):
            for z in range(sz):
                p = cols[x * sz + z]
                base = (x * sz + z) * sy
                filled = 0
                while filled < sy:
                    if p >= ln:
                        raise PackError(f"VOXL {i}: column ({x},{z}) runs past its data")
                    run, p = F.read_varint(blob, p)
                    val, p = F.read_varint(blob, p)
                    if run == 0 or filled + run > sy:
                        raise PackError(f"VOXL {i}: column ({x},{z}) has a bad run")
                    for y in range(filled, filled + run):
                        cells[base + y] = val
                    filled += run
    else:
        raise PackError(f"VOXL {i}: unknown encoding {e['encoding']}")
    for c in cells:
        if c >= len(pal):
            raise PackError(f"VOXL {i}: {key} cell index {c} exceeds the palette")
    return cells
