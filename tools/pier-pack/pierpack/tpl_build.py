"""tpl_build.py: compiles a template pack source (JSON) into a PIERTPL file.

The source names parameters, roles, zones, spans, stacks, constraints, shapes, picks and
voxel blobs by name; this module resolves every name to an index, compiles every
expression through expr.ExprTable, emits shape nodes in post-order so operands precede
their users, and hands finished section bodies to container.Writer. Constraints that
follow from the data, such as a voxel blob fitting into the rectangle it is picked
into, are added here so an author does not have to state them.
"""
from __future__ import annotations

import base64
import datetime as _dt
import os as _os


def _built_at() -> str:
    """The build stamp, honouring SOURCE_DATE_EPOCH.

    It is inside the hashed part of the file, so a wall-clock reading here makes every
    build of one source a different binary. That defeats the sha256 a pack config pins:
    an operator who rebuilds from the same source gets a hash the config no longer names,
    and two people building the same source can never compare results. Setting
    SOURCE_DATE_EPOCH makes the build reproducible, which is what the variable is for.
    """
    epoch = _os.environ.get("SOURCE_DATE_EPOCH")
    if epoch:
        try:
            return _dt.datetime.fromtimestamp(int(epoch), _dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
        except (ValueError, OverflowError, OSError):
            pass
    return _dt.datetime.now(_dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
import json
import os
import struct

from . import format as F
from .container import Strings, Writer
from .expr import ExprTable

TOOL_VERSION = "pier-pack 0.1"


class SourceError(Exception):
    pass


def _need(d: dict, key: str, where: str):
    if key not in d:
        raise SourceError(f"{where}: missing '{key}'")
    return d[key]


class TemplateBuilder:
    def __init__(self, src: dict, base_dir: str = "."):
        self.src = src
        self.base_dir = base_dir
        self.strings = Strings()
        self.params: list = []
        self.param_index: dict = {}
        self.choice_lists: list = []
        self.roles: list = []
        self.role_index: dict = {"air": F.NONE}
        self.zone_names: list = []
        self.zone_index: dict = {}
        self.expr: ExprTable | None = None
        self.shape_nodes: list = []
        self.shape_memo: dict = {}
        self.choose_lists: list = []
        self.voxels: list = []
        self.voxel_index: dict = {}
        self.constraints: list = []
        self.auto_constraints: list = []

    # Parameters

    def _params(self):
        params = _need(self.src, "params", "source")
        for name, p in params.items():
            kind = p.get("kind", "free")
            entry = {"name": name, "kind": kind, "def": int(p.get("default", 0)),
                     "min": int(p.get("min", -2**31)), "max": int(p.get("max", 2**31 - 1)),
                     "step": int(p.get("step", 1)), "aux": F.NONE, "expr": p.get("expr"), "choices": p.get("choices")}
            if kind == "free":
                if entry["step"] < 1:
                    raise SourceError(f"param {name}: step must be at least 1")
                if not (entry["min"] <= entry["def"] <= entry["max"]):
                    raise SourceError(f"param {name}: default {entry['def']} lies outside [{entry['min']}, {entry['max']}]")
            elif kind == "fixed":
                entry["min"] = entry["max"] = entry["def"]
            elif kind == "choice":
                choices = p.get("choices")
                if not choices:
                    raise SourceError(f"param {name}: a choice parameter needs a non-empty 'choices' list")
                choices = [int(c) for c in choices]
                if entry["def"] not in choices:
                    raise SourceError(f"param {name}: default {entry['def']} is not one of the choices")
                entry["aux"] = len(self.choice_lists)
                self.choice_lists.append(choices)
                entry["min"], entry["max"] = min(choices), max(choices)
            elif kind == "derived":
                if not p.get("expr"):
                    raise SourceError(f"param {name}: a derived parameter needs 'expr'")
            else:
                raise SourceError(f"param {name}: unknown kind '{kind}'")
            self.param_index[name] = len(self.params)
            self.params.append(entry)
        self.expr = ExprTable(params=dict(self.param_index))
        for entry in self.params:
            if entry["kind"] == "derived":
                entry["aux"] = self.expr.compile(entry["expr"])

    def _roles(self):
        for name, block in _need(self.src, "roles", "source").items():
            if name == "air":
                raise SourceError("role name 'air' is reserved")
            self.role_index[name] = len(self.roles)
            self.roles.append({"name": name, "block": str(block)})

    def role(self, name: str, where: str) -> int:
        if name not in self.role_index:
            raise SourceError(f"{where}: unknown role '{name}'")
        return self.role_index[name]

    # Zones, spans, stacks

    def _zones(self):
        self.zone_names = list(_need(self.src, "zones", "source"))
        if not self.zone_names:
            raise SourceError("zones: at least one zone is needed")
        self.zone_index = {n: i for i, n in enumerate(self.zone_names)}
        period = _need(self.src, "period", "source")
        self.period_x = self.expr.compile(_need(period, "x", "period"))
        self.period_z = self.expr.compile(_need(period, "z", "period"))
        spans = _need(self.src, "spans", "source")
        self.spans_x = [self._span(s, "spans.x") for s in _need(spans, "x", "spans")]
        self.spans_z = [self._span(s, "spans.z") for s in _need(spans, "z", "spans")]
        if not self.spans_x or not self.spans_z:
            raise SourceError("spans: each axis needs at least one span")
        # The lengths must sum to the period. Both sides are expressions, so the check is
        # emitted as a constraint the host evaluates once the parameters are bound.
        for axis, spans, period in (("x", self.spans_x, self.period_x), ("z", self.spans_z, self.period_z)):
            total = None
            for _, e in spans:
                total = e if total is None else self.expr.binop("add", total, e)
            self.auto_constraints.append((self.expr.binop("eq", total, period),
                                          f"the {axis} spans do not add up to the {axis} period"))
        combine = _need(self.src, "combine", "source")
        n = len(self.zone_names)
        self.combine = [F.NONE] * (n * n)
        for zx in self.zone_names:
            row = combine.get(zx)
            if row is None:
                raise SourceError(f"combine: no row for zone '{zx}'")
            for zz in self.zone_names:
                if zz not in row:
                    raise SourceError(f"combine: row '{zx}' has no entry for '{zz}'")
                self.combine[self.zone_index[zx] * n + self.zone_index[zz]] = self.zone(row[zz], "combine")
        self.stacks = []
        for s in _need(self.src, "stacks", "source"):
            zone = F.NONE if s.get("zone", "*") == "*" else self.zone(s["zone"], "stacks")
            self.stacks.append({
                "zone": zone,
                "from": self.expr.compile(_need(s, "from", "stacks")),
                "to": self.expr.compile(_need(s, "to", "stacks")),
                "role": self.role(s.get("role", "air"), "stacks"),
            })

    def zone(self, name: str, where: str) -> int:
        if name not in self.zone_index:
            raise SourceError(f"{where}: unknown zone '{name}'")
        return self.zone_index[name]

    def _span(self, s, where):
        if isinstance(s, dict):
            zone, length = s.get("zone"), s.get("len")
        else:
            zone, length = s[0], s[1]
        return (self.zone(zone, where), self.expr.compile(length))

    def _constraints(self):
        for c in self.src.get("constraints", []):
            self.constraints.append((self.expr.compile(_need(c, "expr", "constraints")),
                                     str(c.get("message", "a constraint of the pack failed"))))

    # Voxels

    def _voxels(self):
        for name, v in self.src.get("voxels", {}).items():
            data = v
            if "file" in v:
                path = os.path.join(self.base_dir, v["file"])
                with open(path, encoding="utf-8") as f:
                    data = json.load(f)
            sx, sy, sz = [int(x) for x in _need(data, "size", f"voxels.{name}")]
            palette = list(_need(data, "palette", f"voxels.{name}"))
            if len(palette) < 2 or palette[0] != "" or palette[1] != "minecraft:air":
                raise SourceError(f"voxels.{name}: palette[0] must be \"\" (keep) and palette[1] \"minecraft:air\"")
            cells = self._cells(data, "cells", sx * sy * sz, name)
            liquid = self._cells(data, "liquid", sx * sy * sz, name) if "liquid" in data else None
            for c in cells:
                if c >= len(palette):
                    raise SourceError(f"voxels.{name}: a cell index {c} exceeds the palette")
            self.voxel_index[name] = len(self.voxels)
            self.voxels.append({"name": name, "sx": sx, "sy": sy, "sz": sz, "palette": palette,
                                "cells": cells, "liquid": liquid,
                                "encoding": F.VOXEL_RLE if data.get("encoding", "rle") == "rle" else F.VOXEL_RAW})

    @staticmethod
    def _cells(data, key, n, name):
        raw = data[key]
        if isinstance(raw, str):
            buf = base64.b64decode(raw)
            if len(buf) != 2 * n:
                raise SourceError(f"voxels.{name}: {key} holds {len(buf)//2} cells, expected {n}")
            return list(struct.unpack("<%dH" % n, buf))
        cells = [int(x) for x in raw]
        if len(cells) != n:
            raise SourceError(f"voxels.{name}: {key} holds {len(cells)} cells, expected {n}")
        return cells

    # Shapes

    def _shapes(self):
        self.named_shapes = self.src.get("shapes", {})
        self._resolving = set()
        self.shape_by_name = {}
        for name in self.named_shapes:
            self.shape(name, "shapes")

    def _p(self, node: dict, where: str):
        """Three operands, each an int or an expression string; returns values and flags."""
        p = list(node.get("p", []))
        p += [0] * (3 - len(p))
        vals, flags = [], 0
        for i, v in enumerate(p[:3]):
            if isinstance(v, bool):
                raise SourceError(f"{where}: p[{i}] is a boolean")
            if isinstance(v, int):
                vals.append(v)
            else:
                idx = self.expr.compile(v)
                if self.expr.is_const(idx):
                    vals.append(self.expr.const_value(idx))
                else:
                    vals.append(idx)
                    flags |= 1 << i
        return vals, flags

    def shape(self, ref, where: str) -> int:
        if isinstance(ref, str):
            if ref == "nothing":
                return self._shape_node({"op": "nothing"}, where)
            if ref in self.shape_by_name:
                return self.shape_by_name[ref]
            if ref not in self.named_shapes:
                raise SourceError(f"{where}: unknown shape '{ref}'")
            if ref in self._resolving:
                raise SourceError(f"shapes: '{ref}' refers to itself")
            self._resolving.add(ref)
            idx = self._shape_node(self.named_shapes[ref], f"shapes.{ref}")
            self._resolving.discard(ref)
            self.shape_by_name[ref] = idx
            return idx
        return self._shape_node(ref, where)

    def _emit_shape(self, **kw) -> int:
        key = tuple(sorted(kw.items()))
        if key in self.shape_memo:
            return self.shape_memo[key]
        node = {"op": 0, "flags": 0, "a": F.NONE, "b": F.NONE, "p0": 0, "p1": 0, "p2": 0,
                "role": 0, "reserved": 0, "aux": 0}
        node.update(kw)
        idx = len(self.shape_nodes)
        self.shape_nodes.append(node)
        self.shape_memo[key] = idx
        return idx

    def _shape_node(self, node: dict, where: str) -> int:
        op = _need(node, "op", where)
        if op not in F.SHAPE_OP:
            raise SourceError(f"{where}: unknown shape op '{op}'")
        code = F.SHAPE_OP[op]
        kw = {"op": code}
        a = node.get("a")
        b = node.get("b")
        if op in ("translate", "rotate_y", "mirror", "repeat", "shell", "paint", "union", "difference", "intersect"):
            if a is None:
                raise SourceError(f"{where}: '{op}' needs 'a'")
            kw["a"] = self.shape(a, where + ".a")
        if op in ("union", "difference", "intersect"):
            if b is None:
                raise SourceError(f"{where}: '{op}' needs 'b'")
            kw["b"] = self.shape(b, where + ".b")
        if op in ("box", "cylinder", "wedge", "translate", "rotate_y", "mirror", "repeat", "shell"):
            vals, flags = self._p(node, where)
            kw.update(p0=vals[0], p1=vals[1], p2=vals[2], flags=flags)
            if op == "rotate_y" and not (flags & 1) and not (0 <= vals[0] <= 3):
                raise SourceError(f"{where}: rotate_y turns must be 0..3")
            if op == "mirror" and vals[0] not in (0, 2):
                raise SourceError(f"{where}: mirror axis must be 0 (x) or 2 (z)")
            if op == "repeat" and vals[2] not in (0, 1, 2):
                raise SourceError(f"{where}: repeat axis must be 0, 1 or 2")
        if op == "paint":
            kw["role"] = self.role(_need(node, "role", where), where)
        if op == "choose":
            ch = _need(node, "choose", where)
            entries = []
            for e in _need(ch, "entries", where + ".choose"):
                entries.append((self.shape(_need(e, "shape", where), where + ".choose"), int(e.get("weight", 1))))
            if not entries:
                raise SourceError(f"{where}: choose needs entries")
            by = ch.get("by", "hash")
            if by == "hash":
                kw.update(p0=0, p1=int(ch.get("salt", 0)) & 0xFFFFFFFF)
            elif by == "param":
                pname = _need(ch, "param", where + ".choose")
                if pname not in self.param_index:
                    raise SourceError(f"{where}: choose refers to unknown parameter '{pname}'")
                p = self.params[self.param_index[pname]]
                if p["kind"] != "choice":
                    raise SourceError(f"{where}: choose by param needs a choice parameter, '{pname}' is {p['kind']}")
                if len(p["choices"]) != len(entries):
                    raise SourceError(f"{where}: '{pname}' has {len(p['choices'])} choices but {len(entries)} entries")
                kw.update(p0=1, p1=self.param_index[pname])
            else:
                raise SourceError(f"{where}: choose.by must be hash or param")
            kw["aux"] = len(self.choose_lists)
            self.choose_lists.append(entries)
        if op == "voxels":
            vname = _need(node, "voxels", where)
            if vname not in self.voxel_index:
                raise SourceError(f"{where}: unknown voxel blob '{vname}'")
            kw["aux"] = self.voxel_index[vname]
        return self._emit_shape(**kw)

    # Picks

    def _picks(self):
        self.picks = []
        for i, p in enumerate(self.src.get("pick", [])):
            where = f"pick[{i}]"
            roots = []
            for r in _need(p, "roots", where):
                rot = r.get("rot", 0)
                if rot == "any":
                    mask = 0b1111
                elif isinstance(rot, list):
                    mask = 0
                    for t in rot:
                        mask |= 1 << int(t)
                else:
                    mask = 1 << int(rot)
                roots.append({"shape_node": self.shape(_need(r, "shape", where), where),
                              "weight": int(r.get("weight", 1)), "rot_mask": mask, "reserved": 0})
            if not roots:
                raise SourceError(f"{where}: roots is empty")
            self.picks.append({
                "salt": int(p.get("salt", 0)) & 0xFFFFFFFF,
                "x0": self.expr.compile(p.get("x0", 0)), "z0": self.expr.compile(p.get("z0", 0)),
                "w": self.expr.compile(_need(p, "w", where)), "d": self.expr.compile(_need(p, "d", where)),
                "anchor_y": self.expr.compile(_need(p, "anchor_y", where)),
                "roots": roots,
            })

    # Sections

    def _confine(self):
        c = self.src.get("confine")
        if not c or not c.get("enabled", False):
            self.confine = None
            return
        self.confine = {"cell_expr": self.expr.compile(_need(c, "cell", "confine")),
                        "gap_expr": self.expr.compile(_need(c, "gap", "confine"))}
        # Confinement is square: cell + gap must equal both periods.
        total = self.expr.binop("add", self.confine["cell_expr"], self.confine["gap_expr"])
        self.auto_constraints.append((self.expr.binop("and", self.expr.binop("eq", total, self.period_x),
                                                       self.expr.binop("eq", total, self.period_z)),
                                      "confine.cell + confine.gap must equal both periods"))

    def build(self) -> bytes:
        self._params()
        self._roles()
        self._zones()
        self._constraints()
        self._voxels()
        self._shapes()
        self._picks()
        self._confine()
        height = self.src.get("height", {})
        w = Writer(F.MAGIC_TEMPLATE)
        # Interning strings happens while the bodies are built, so STRS is emitted last
        # and inserted at the front of the table.
        bodies = []
        bodies.append((F.SEC_INFO, F.INFO.pack(
            tool_version_str=self.strings.add(TOOL_VERSION),
            built_at_str=self.strings.add(_built_at()),
            source_name_str=self.strings.add(str(self.src.get("name", ""))),
            height_min=int(height.get("min", -512)), height_max=int(height.get("max", 320)),
            flags=F.INFO_HEIGHT_FIXED if height.get("fixed", False) else 0,
            biome_str=self.strings.add(str(self.src.get("biome", "minecraft:plains"))))))
        body = struct.pack("<I", len(self.params))
        for p in self.params:
            body += F.PARAM.pack(name_str=self.strings.add(p["name"]), kind={"free": 0, "fixed": 1, "choice": 2, "derived": 3}[p["kind"]],
                                 flags=0, **{"def": p["def"]}, min=p["min"], max=p["max"], step=p["step"], aux=p["aux"], reserved=0)
        bodies.append((F.SEC_PARM, body))
        body = struct.pack("<I", len(self.choice_lists))
        vals = []
        for lst in self.choice_lists:
            body += F.LIST_REF.pack(first=len(vals), count=len(lst))
            vals += lst
        body += struct.pack("<%di" % len(vals), *vals)
        bodies.append((F.SEC_PCHC, body))
        bodies.append((F.SEC_ROLE, struct.pack("<I", len(self.roles)) + b"".join(
            F.ROLE.pack(name_str=self.strings.add(r["name"]), default_block_str=self.strings.add(r["block"])) for r in self.roles)))
        n = len(self.zone_names)
        body = struct.pack("<I", n) + struct.pack("<%dI" % n, *[self.strings.add(z) for z in self.zone_names])
        body += struct.pack("<II", self.period_x, self.period_z)
        body += struct.pack("<I", len(self.spans_x)) + b"".join(F.SPAN.pack(zone=z, len_expr=e) for z, e in self.spans_x)
        body += struct.pack("<I", len(self.spans_z)) + b"".join(F.SPAN.pack(zone=z, len_expr=e) for z, e in self.spans_z)
        body += struct.pack("<%dI" % (n * n), *self.combine)
        bodies.append((F.SEC_ZONE, body))
        bodies.append((F.SEC_STAK, struct.pack("<I", len(self.stacks)) + b"".join(
            F.STACK.pack(zone=s["zone"], from_expr=s["from"], to_expr=s["to"], role=s["role"]) for s in self.stacks)))
        allc = self.constraints + self.auto_constraints
        bodies.append((F.SEC_CNST, struct.pack("<I", len(allc)) + b"".join(
            F.CONSTRAINT.pack(expr=e, message_str=self.strings.add(m)) for e, m in allc)))
        if self.shape_nodes:
            body = struct.pack("<I", len(self.shape_nodes)) + b"".join(F.SHAPE_NODE.pack(**nd) for nd in self.shape_nodes)
            body += struct.pack("<I", len(self.choose_lists))
            entries = []
            for lst in self.choose_lists:
                body += F.LIST_REF.pack(first=len(entries), count=len(lst))
                entries += lst
            body += b"".join(F.CHOOSE_ENTRY.pack(node=nd, weight=wt) for nd, wt in entries)
            bodies.append((F.SEC_SHAP, body))
        if self.picks:
            body = struct.pack("<I", len(self.picks))
            roots = []
            for p in self.picks:
                body += F.PICK.pack(salt=p["salt"], x0_expr=p["x0"], z0_expr=p["z0"], w_expr=p["w"], d_expr=p["d"],
                                    anchor_y_expr=p["anchor_y"], root_first=len(roots), root_count=len(p["roots"]))
                roots += p["roots"]
            body += b"".join(F.PICK_ROOT.pack(**r) for r in roots)
            bodies.append((F.SEC_PICK, body))
        if self.voxels:
            bodies.append((F.SEC_VOXL, self._voxl_body()))
        if self.confine:
            bodies.append((F.SEC_CONF, F.CONFINE.pack(**self.confine)))
        # The EXPR table is complete only now: shape and pick compilation added nodes.
        bodies.insert(1, (F.SEC_EXPR, struct.pack("<I", len(self.expr.nodes)) + b"".join(
            F.EXPR_NODE.pack(**nd) for nd in self.expr.nodes)))
        w.add(F.SEC_STRS, self.strings.body())
        for tag, body in bodies:
            w.add(tag, body)
        return w.build()

    def _voxl_body(self) -> bytes:
        entries = []
        pal_refs = []
        data = bytearray()
        header_size = 4 + len(self.voxels) * F.VOXEL.size
        pal_total = sum(len(v["palette"]) for v in self.voxels)
        data_base = F.align8(header_size + 4 * pal_total)
        for v in self.voxels:
            pal_first = len(pal_refs)
            pal_refs += [self.strings.add(s) for s in v["palette"]]
            entry = {"sx": v["sx"], "sy": v["sy"], "sz": v["sz"], "pal_count": len(v["palette"]), "pal_first": pal_first,
                     "encoding": v["encoding"], "has_liquid": 1 if v["liquid"] is not None else 0, "reserved": b"\0\0",
                     "main_off": 0, "main_len": 0, "col_off": 0, "liq_off": 0, "liq_len": 0, "liq_col_off": 0}
            for key, cells in (("main", v["cells"]), ("liq", v["liquid"])):
                if cells is None:
                    continue
                if v["encoding"] == F.VOXEL_RAW:
                    blob = struct.pack("<%dH" % len(cells), *cells)
                    col = b""
                else:
                    blob, col = encode_rle(cells, v["sx"], v["sy"], v["sz"])
                while (data_base + len(data)) % 8:
                    data.append(0)
                entry[key + "_off"] = data_base + len(data)
                entry[key + "_len"] = len(blob)
                data += blob
                while (data_base + len(data)) % 8:
                    data.append(0)
                if col:
                    entry["col_off" if key == "main" else "liq_col_off"] = data_base + len(data)
                    data += col
            entries.append(entry)
        body = struct.pack("<I", len(entries)) + b"".join(F.VOXEL.pack(**e) for e in entries)
        body += struct.pack("<%dI" % len(pal_refs), *pal_refs)
        body += b"\0" * (data_base - len(body))
        return bytes(body) + bytes(data)


def encode_rle(cells: list, sx: int, sy: int, sz: int) -> tuple[bytes, bytes]:
    """Per-column varint runs; returns (data, column offset table)."""
    out = bytearray()
    cols = []
    for x in range(sx):
        for z in range(sz):
            cols.append(len(out))
            base = (x * sz + z) * sy
            run_val = cells[base]
            run_len = 1
            for y in range(1, sy):
                v = cells[base + y]
                if v == run_val:
                    run_len += 1
                else:
                    F.write_varint(out, run_len)
                    F.write_varint(out, run_val)
                    run_val, run_len = v, 1
            F.write_varint(out, run_len)
            F.write_varint(out, run_val)
    return bytes(out), struct.pack("<%dI" % len(cols), *cols)



#: 顶层允许出现的键。不认识的键是错误而不是忽略：一个拼错的名字、或者从 Java 数据包
#: 抄来的一项这边没有的设置，静静地不生效，出来的地形和作者写的不是一回事，而没有任何
#: 一侧会说话。列表随实现走——加了一项支持才往里加一个名字。
TEMPLATE_KEYS = {
    "pier_pack",
    "type",
    "name",
    "height",
    "params",
    "roles",
    "zones",
    "period",
    "spans",
    "combine",
    "stacks",
    "constraints",
    "voxels",
    "shapes",
    "pick",
    "confine",
    "biome",
}


def _reject_unknown_keys(src, allowed, what):
    unknown = sorted(k for k in src if k not in allowed)
    if unknown:
        raise SourceError(
            f"{what} 源码里有这边不认识的键：{', '.join(unknown)}。"
            f"拼错的名字和不支持的设置都会静静失效，所以这里拒绝而不是忽略。"
        )


def build_file(source_path: str, out_path: str) -> bytes:
    with open(source_path, encoding="utf-8") as f:
        src = json.load(f)
    if src.get("type", "template") != "template":
        raise SourceError("this source is not a template pack")
    _reject_unknown_keys(src, TEMPLATE_KEYS, "模板包")
    data = TemplateBuilder(src, base_dir=os.path.dirname(os.path.abspath(source_path))).build()
    with open(out_path, "wb") as f:
        f.write(data)
    return data
