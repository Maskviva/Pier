# -*- coding: utf-8 -*-
"""The Go reference: the exported API of package levilamina in bindings/go/levilamina.

Pages are by subject, with the slugs of the C++ pages. A method goes to the page of its
receiver type (TYPE_PAGES), a free function to the page FUNC_PAGES gives it or else to the
page of the first slot it reaches, and a constant to the page of its type or of its family
prefix. Anything that none of these places stops the generator with its name. The Raw
methods, one per callback-free slot, have a page of their own, grouped by subject.
"""

import os
import re

from .model import Entry, Group, Page, ROOT
from . import cpp

PKG_REL = "bindings/go/levilamina"
PKG = os.path.join(ROOT, PKG_REL)

TYPE_PAGES = {
    "GamingStatus": "core", "TaskID": "core", "Mod": "core", "Loader": "core", "Unloader": "core",
    "Context": "core", "Logger": "core", "NotProvidedError": "core",
    "Priority": "events", "Event": "events", "Listener": "events", "ListenerHandle": "events",
    "CommandOutput": "commands", "Permission": "commands", "Invocation": "commands",
    "ParamType": "commands", "Overload": "commands", "CommandBuilder": "commands",
    "CommandOrigin": "commands", "CommandCall": "commands", "EnumValue": "commands",
    "SoftEnumOp": "commands",
    "EntityInfo": "world", "PaletteEntry": "world", "BlockCell": "world", "ExplodeOptions": "world",
    "Player": "player", "PlayerInfo": "player", "SelKind": "player", "PlayerSel": "player",
    "PlayerPos": "player",
    "Entity": "entity", "ActorInfo": "entity", "ActorID": "entity",
    "BlockAt": "block", "BlockInfo": "block",
    "Item": "item", "Container": "item", "SlotItem": "item", "ContainerRef": "item",
    "KvDb": "data", "KeyValue": "data", "KvDbHandle": "data",
    "MoneyKind": "money", "MoneyEvent": "money",
    "PacketVerdict": "packet", "PacketEvent": "packet", "PacketCall": "packet",
    "PacketHook": "packet", "PacketHookHandle": "packet",
    "KeyBinding": "client", "KeyHandle": "client",
    "ChunkRequest": "dimensions",
    "Subscription": "crossmod", "Vetoable": "crossmod", "ServiceRegistration": "crossmod",
    "CallErrorKind": "crossmod", "CallError": "crossmod", "ServiceInfo": "crossmod",
    "NbtKind": "nbt", "Nbt": "nbt",
    "RawAPI": "raw",
}
FUNC_PAGES = {
    "Register": "core", "IsNotProvided": "core",
    "ParseSNBT": "nbt", "NewCompound": "nbt", "NbtByteOf": "nbt", "NbtIntOf": "nbt",
    "NbtLongOf": "nbt", "NbtDoubleOf": "nbt", "NbtStringOf": "nbt",
    "ByName": "player", "ByXuid": "player", "ByUuid": "player",
    "PlayerByName": "player", "PlayerByXuid": "player", "PlayerByUuid": "player",
    "EntityByID": "entity", "Block": "block", "ItemOf": "item",
    "NewOverload": "commands", "NewCommand": "commands", "Scoreboard": "scoreboard",
}
CONST_PAGES = {"Container": "item"}
CONST_FAMILIES = [
    ("PProp", "player"), ("PStr", "player"), ("PAct", "player"),
    ("AProp", "entity"), ("AStr", "entity"), ("AAct", "entity"),
    ("BProp", "block"), ("BStr", "block"), ("BAct", "block"),
    ("IProp", "item"), ("IStr", "item"), ("Srv", "server"), ("Sys", "server"),
]
EXTRA_TOPICS = [("nbt", "SNBT and NBT values", "SNBT 与 NBT 值"),
                ("raw", "Raw: one typed method per slot", "Raw：每个槽位一个带类型的方法")]


def mask(t):
    """t with comments, strings, raw strings and runes blanked out, newlines kept."""
    out = list(t)
    n, i = len(t), 0

    def blank(a, b):
        for k in range(a, min(b, n)):
            if out[k] != "\n":
                out[k] = " "

    while i < n:
        if t.startswith("//", i):
            j = t.find("\n", i)
            j = n if j < 0 else j
            blank(i, j)
            i = j
        elif t.startswith("/*", i):
            j = t.find("*/", i + 2) + 2
            blank(i, j)
            i = j
        elif t[i] == "`":
            j = t.find("`", i + 1) + 1
            blank(i, j)
            i = j
        elif t[i] == '"':
            j = i + 1
            while t[j] != '"':
                j += 2 if t[j] == "\\" else 1
            blank(i, j + 1)
            i = j + 1
        elif t[i] == "'":
            m = re.compile(r"'(?:\\.[^']*|[^\\'])'").match(t, i)
            j = m.end() if m else i + 1
            blank(i, j)
            i = j
        else:
            i += 1
    return "".join(out)


def braces(masked):
    match, stack = {}, []
    for i, ch in enumerate(masked):
        if ch == "{":
            stack.append(i)
        elif ch == "}":
            match[stack.pop()] = i
    return match


def doc_above(lines, k):
    out = []
    k -= 1
    while k >= 0 and lines[k].startswith("//"):
        s = lines[k]
        if s.startswith("//go:") or s.startswith("//export"):
            k -= 1
            continue
        out.insert(0, s[3:] if s.startswith("// ") else s[2:])
        k -= 1
    return out


def split_top(text):
    out, depth, cur = [], 0, []
    for ch in text:
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
        if ch == "," and depth == 0:
            out.append("".join(cur).strip())
            cur = []
        else:
            cur.append(ch)
    if "".join(cur).strip():
        out.append("".join(cur).strip())
    return out


def go_params(text):
    """Go parameters, a grouped `a, b, c float64` expanded to one row per name."""
    parts = split_top(text)
    out, pending = [], []
    for p in parts:
        m = re.match(r"^(\w+)\s+(.+)$", p)
        if m and not re.match(r"^(func|map|chan|struct|interface)\b", p):
            for n in pending:
                out.append((n, m.group(2)))
            pending = []
            out.append((m.group(1), m.group(2)))
        elif re.fullmatch(r"\w+", p):
            pending.append(p)
        else:
            out.append(("", p))
    for n in pending:
        out.append(("", n))
    return out


class Decl:
    pass


def read_package():
    funcs, types, consts, varz = [], [], [], []
    for fn in sorted(os.listdir(PKG)):
        if not fn.endswith(".go") or fn.endswith("_test.go"):
            continue
        with open(os.path.join(PKG, fn), encoding="utf-8") as f:
            text = f.read()
        masked = mask(text)
        match = braces(masked)
        lines = text.split("\n")
        offsets = [0]
        for l in lines:
            offsets.append(offsets[-1] + len(l) + 1)
        for k, line in enumerate(lines):
            if line.startswith("func "):
                d = Decl()
                d.file, d.line, d.doc = fn, k + 1, doc_above(lines, k)
                start = offsets[k]
                p, depth = start, 0
                while True:
                    ch = masked[p]
                    if ch == "(":
                        depth += 1
                    elif ch == ")":
                        depth -= 1
                    elif ch == "{" and depth == 0:
                        if re.search(r"\b(struct|interface)\s*$", masked[start:p]):
                            p = match[p]
                        else:
                            break
                    p += 1
                d.sig = " ".join(text[start:p].split())
                d.body = masked[p:match[p] + 1]
                m = re.match(r"^func\s+(?:\((\w+\s+)?\*?(\w+)\)\s+)?(\w+)\s*\(", d.sig)
                d.recv_var = (m.group(1) or "").strip() or None
                d.recv, d.name = m.group(2), m.group(3)
                inner_start = m.end()
                depth, q = 1, inner_start
                while depth:
                    depth += {"(": 1, ")": -1}.get(d.sig[q], 0)
                    q += 1
                d.params = go_params(d.sig[inner_start:q - 1])
                d.ret = d.sig[q:].strip() or None
                funcs.append(d)
            elif line.startswith("type ") and not line.startswith("type ("):
                d = Decl()
                d.file, d.line, d.doc = fn, k + 1, doc_above(lines, k)
                d.name = line.split()[1]
                start = offsets[k]
                brace = masked.find("{", start, offsets[k + 1])
                if brace >= 0 and re.search(r"\b(struct|interface)\s*\{", masked[start:brace + 1]):
                    end = match[brace] + 1
                    d.decl = struct_decl(text[start:end])
                else:
                    d.decl = line.strip()
                types.append(d)
            elif re.match(r"^(const|var) \($", line):
                kind = line.split()[0]
                j = k + 1
                block_doc = doc_above(lines, k)
                iota, last_type, last_expr = 0, None, None
                while lines[j] != ")":
                    s = lines[j].strip()
                    m = re.match(r"^(\w+)(?:\s+([\w.*\[\]]+))?(?:\s*=\s*(.*?))?\s*(?://\s*(.*))?$", s)
                    if s and not s.startswith("//") and m:
                        name, ty, expr, trail = m.groups()
                        if expr is None and kind == "const":
                            ty, expr = last_type, last_expr
                        else:
                            last_type, last_expr = ty, expr
                        d = Decl()
                        d.file, d.line, d.kind = fn, j + 1, kind
                        d.name, d.type = name, ty
                        d.doc = doc_above(lines, j) + ([trail] if trail else [])
                        d.block_doc = block_doc
                        d.value = const_value(expr, iota)
                        (consts if kind == "const" else varz).append(d)
                        iota += 1
                    j += 1
            elif re.match(r"^(const|var) [A-Za-z_]", line):
                m = re.match(r"^(const|var) (\w+)(?:\s+([\w.*\[\]]+))?(?:\s*=\s*(.*))?$", line)
                d = Decl()
                d.file, d.line, d.kind = fn, k + 1, m.group(1)
                d.name, d.type = m.group(2), m.group(3)
                d.doc, d.block_doc = doc_above(lines, k), []
                d.value = (m.group(4) or "").strip()
                (consts if d.kind == "const" else varz).append(d)
    return funcs, types, consts, varz


def const_value(expr, iota):
    if expr is None:
        return ""
    e = expr.strip()
    m = re.fullmatch(r"iota(?:\s*([+-])\s*(\d+))?", e)
    if m:
        v = iota + (int(m.group(2)) * (1 if m.group(1) == "+" else -1) if m.group(1) else 0)
        return str(v)
    return e


def struct_decl(src):
    """A struct or interface with its unexported fields folded into one comment."""
    lines = src.split("\n")
    kept, hidden_fields = [lines[0]], False
    for l in lines[1:-1]:
        s = l.strip()
        if not s or s.startswith("//"):
            kept.append(l) if s.startswith("//") else None
            continue
        if re.match(r"^[A-Z]", s) or re.match(r"^\*?[A-Z][\w.]*$", s):
            kept.append(l)
        else:
            hidden_fields = True
            while kept and kept[-1].strip().startswith("//"):
                kept.pop()
    if hidden_fields:
        kept.append("\t// unexported fields")
    kept.append(lines[-1])
    return "\n".join(l.replace("\t", "    ") for l in kept)


def exported(name):
    return bool(name) and name[0].isupper()


def build(slot_names, slot_page):
    funcs, types, consts, varz = read_package()
    titles = {s: (en, zh) for s, en, zh in cpp.TOPICS if s != "types"}
    titles.update({s: (en, zh) for s, en, zh in EXTRA_TOPICS})
    pages = {s: Page("go", s, "Go: " + en, "Go：" + zh, []) for s, (en, zh) in titles.items()}
    pages["core"].intro = doc_package()

    def src(page, fn):
        path = "%s/%s" % (PKG_REL, fn)
        if path not in pages[page].sources:
            pages[page].sources.append(path)

    missing = []

    def where(kind, name):
        missing.append("%s %s" % (kind, name))
        return "core"

    groups = {}
    tmap = {}
    for d in types:
        if not exported(d.name):
            continue
        page = TYPE_PAGES.get(d.name) or where("type", d.name)
        g = Group("`%s`" % d.name, d.name, "type")
        g.doc, g.decl = d.doc, d.decl
        groups[d.name] = g
        tmap[d.name] = page
        pages[page].groups.append(g)
        src(page, d.file)

    by_name = {d.name: d for d in funcs if not d.recv}
    raw = {d.name: d for d in funcs if d.recv == "RawAPI"}
    meth = {}
    for d in funcs:
        if d.recv:
            meth.setdefault((d.recv, d.name), d)
    memo = {}

    def slots_of(d, depth=0, seen=frozenset()):
        key = (d.recv, d.name)
        if key in memo:
            return memo[key]
        res = []
        for m in re.finditer(r"C\.piergo_(?:call|has)_(\w+)\(", d.body):
            if m.group(1) in slot_names and m.group(1) not in res:
                res.append(m.group(1))
        if depth < 4:
            callees = [raw[m.group(1)] for m in re.finditer(r"\bRaw\.(\w+)\(", d.body) if m.group(1) in raw]
            if d.recv_var:
                callees += [meth[(d.recv, m.group(1))] for m in
                            re.finditer(r"\b%s\.(\w+)\(" % re.escape(d.recv_var), d.body) if (d.recv, m.group(1)) in meth]
            callees += [by_name[m.group(1)] for m in re.finditer(r"(?<![\w.])(\w+)\(", d.body) if m.group(1) in by_name]
            for c in callees:
                if (c.recv, c.name) in seen:
                    continue
                for s in slots_of(c, depth + 1, seen | {key}):
                    if s not in res:
                        res.append(s)
        if depth == 0:
            memo[key] = res
        return res

    entries = []
    raw_groups = {}
    for d in funcs:
        if not exported(d.name) or (d.recv and not exported(d.recv)):
            continue
        sig = d.sig
        if d.recv == "RawAPI":
            qual = "Raw." + d.name
            e = Entry("go", "method", d.name, qual, sig, "%s/%s:%d" % (PKG_REL, d.file, d.line))
            e.slots = slots_of(d)
            topic = slot_page(e.slots[0]) if e.slots else where("Raw method", d.name)
            if topic not in raw_groups:
                g = Group(titles[topic][0], "raw-" + topic, "functions")
                g.title_zh = titles[topic][1]
                raw_groups[topic] = g
            raw_groups[topic].entries.append(e)
            src("raw", d.file)
        elif d.recv:
            qual = "%s.%s" % (d.recv, d.name)
            e = Entry("go", "method", d.name, qual, sig, "%s/%s:%d" % (PKG_REL, d.file, d.line))
            e.slots = slots_of(d)
            if d.recv not in groups:
                where("receiver type", d.recv)
            groups[d.recv].entries.append(e)
            src(tmap[d.recv], d.file)
        else:
            e = Entry("go", "fn", d.name, d.name, sig, "%s/%s:%d" % (PKG_REL, d.file, d.line))
            e.slots = slots_of(d)
            page = FUNC_PAGES.get(d.name) or (slot_page(e.slots[0]) if e.slots else None) or where("function", d.name)
            key = ("functions", page)
            if key not in groups:
                g = Group("Functions", "functions", "functions")
                g.title_zh = "函数"
                groups[key] = g
                pages[page].groups.insert(0, g)
            groups[key].entries.append(e)
            src(page, d.file)
        e.doc = d.doc
        e.params = [(n or "_", t) for n, t in d.params]
        e.ret = d.ret
        dep = [l for l in d.doc if l.startswith("Deprecated:")]
        if dep:
            e.deprecated = (None, dep[0][len("Deprecated:"):].strip())
        entries.append(e)
    for topic, _, _ in cpp.TOPICS:
        if topic in raw_groups:
            pages["raw"].groups.append(raw_groups[topic])

    for d in consts + varz:
        if not exported(d.name):
            continue
        page = None
        if d.type in tmap and d.kind == "const":
            groups[d.type].consts.append((d.name, d.value, d.doc, False))
            src(tmap[d.type], d.file)
            continue
        else:
            fam = next((f for f, _ in CONST_FAMILIES if d.name.startswith(f) and d.name[len(f):][:1].isupper()), None)
            if fam:
                page = dict(CONST_FAMILIES)[fam]
                gkey, gtitle = fam, "`%s*`" % fam
            else:
                pre = next((c for c in CONST_PAGES if d.name.startswith(c)), None)
                if pre:
                    page, gkey, gtitle = CONST_PAGES[pre], pre + "-values", "`%s*`" % pre
        if d.kind == "var":
            page = TYPE_PAGES.get(d.type) or where("variable", d.name)
            e = Entry("go", "var", d.name, d.name, "var %s %s" % (d.name, d.type or ""), "%s/%s:%d" % (PKG_REL, d.file, d.line))
            e.doc = d.doc
            g = groups.get(("vars", page))
            if not g:
                g = Group("Variables", "variables", "functions")
                g.title_zh = "变量"
                groups[("vars", page)] = g
                pages[page].groups.insert(0, g)
            g.entries.append(e)
            src(page, d.file)
            continue
        if not page:
            where("constant", d.name)
        g = groups.get(("consts", gkey))
        if not g:
            g = Group(gtitle, gkey, "consts")
            g.doc = d.block_doc
            groups[("consts", gkey)] = g
            pages[page].groups.append(g)
        g.consts.append((d.name, d.value, d.doc, "unsupported since" in " ".join(d.doc)))
        src(page, d.file)
    if missing:
        raise SystemExit("gen-api-docs: these Go declarations have no page; add them to the maps "
                         "in tools/apidocs/golang.py:\n  " + "\n  ".join(missing))
    order = list(titles)
    return [pages[s] for s in order if pages[s].groups], entries


def doc_package():
    with open(os.path.join(PKG, "levilamina.go"), encoding="utf-8") as f:
        lines = f.read().split("\n")
    k = next(i for i, l in enumerate(lines) if l.startswith("package "))
    return doc_above(lines, k)
