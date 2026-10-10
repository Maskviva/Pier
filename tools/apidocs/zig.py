# -*- coding: utf-8 -*-
"""The Zig reference: what `@import("levilamina")` exposes from bindings/zig/src.

levilamina.zig decides the public surface: a declaration of core.zig is public when the root
file re-exports it, and props_gen.zig and nbt.zig are public as a whole through
`levilamina.props` and `levilamina.nbt`. Inside a public container every `pub fn` and every
field is public. `slot("name")` reaches every slot of abi.h, so the C++ pages are the
reference for calling a slot from Zig directly.
"""

import os
import re

from .model import Entry, Group, Page, ROOT
from . import cpp

SRC_REL = "bindings/zig/src"
SRC = os.path.join(ROOT, SRC_REL)

PAGES = {
    "c": "core", "Error": "core", "slot": "core", "handle": "core", "str": "core", "view": "core",
    "Collector": "core", "Level": "core", "log": "core", "logf": "core", "Context": "core",
    "exportMod": "core", "TaskId": "core", "schedule": "core", "scheduleAfter": "core",
    "cancel": "core", "pendingTasks": "core",
    "Priority": "events", "Event": "events", "Listener": "events", "subscribe": "events",
    "Permission": "commands", "Invocation": "commands", "registerCommand": "commands",
    "executeCommand": "commands",
    "Subscription": "crossmod", "busSubscribe": "crossmod", "busPublish": "crossmod",
    "Vetoable": "crossmod", "busPublishVetoable": "crossmod", "busSubscriberCount": "crossmod",
    "Reply": "crossmod", "ServiceRegistration": "crossmod", "registerService": "crossmod",
    "CallCode": "crossmod", "CallOutcome": "crossmod", "callService": "crossmod",
    "listServicesJson": "crossmod", "serviceCaller": "crossmod",
    "SelKind": "player", "Player": "player", "Entity": "entity", "BlockAt": "block", "Item": "item",
}
# The server and system information readers props_gen.zig writes, one per constant.
PREFIX_PAGES = [("srv", "server"), ("sys", "server")]
EXTRA_TOPICS = [("nbt", "SNBT values", "SNBT 值")]

DECL = re.compile(r"^([ \t]*)(pub\s+)?(?:inline\s+|export\s+)?(fn|const|var)\s+(@\"[^\"]*\"|\w+)", re.M)


def mask(t):
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
        elif t.startswith("\\\\", i):
            j = t.find("\n", i)
            j = n if j < 0 else j
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
    while k >= 0 and lines[k].strip().startswith("///"):
        s = lines[k].strip()[3:]
        out.insert(0, s[1:] if s.startswith(" ") else s)
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


class ZDecl:
    pass


def read(fn):
    with open(os.path.join(SRC, fn), encoding="utf-8") as f:
        text = f.read()
    masked = mask(text)
    match = braces(masked)
    lines = text.split("\n")
    intro = [l.strip()[3:].strip() if l.strip()[3:4] == " " else l.strip()[3:] for l in lines if l.strip().startswith("//!")]
    decls = []

    def line_of(pos):
        return text.count("\n", 0, pos)

    def walk(a, b, owner, public_owner):
        pos = a
        while True:
            m = DECL.search(masked, pos, b)
            if not m:
                break
            start = m.start(2) if m.group(2) else m.start(3)
            k = line_of(start)
            d = ZDecl()
            d.file, d.line, d.doc = fn, k + 1, doc_above(lines, k)
            d.pub = bool(m.group(2)) and public_owner
            d.kind, d.name, d.owner = m.group(3), m.group(4), owner
            if d.kind == "fn":
                p, depth = m.end(), 0
                while True:
                    ch = masked[p]
                    if ch == "(":
                        depth += 1
                    elif ch == ")":
                        depth -= 1
                    elif ch == ";" and depth == 0:
                        break
                    elif ch == "{" and depth == 0:
                        if re.search(r"(\berror|\bstruct|\benum(\s*\([^)]*\))?|\bunion(\s*\([^)]*\))?)\s*$", masked[m.end():p]):
                            p = match[p]
                        else:
                            break
                    p += 1
                d.sig = " ".join(text[start:p].split())
                d.body = text[p:match[p] + 1] if masked[p] == "{" else ""
                d.mbody = masked[p:match[p] + 1] if masked[p] == "{" else ""
                pm = re.match(r"^(?:pub\s+)?fn\s+\w+\s*\(", d.sig)
                depth, q = 1, pm.end()
                while depth:
                    depth += {"(": 1, ")": -1}.get(d.sig[q], 0)
                    q += 1
                d.params = []
                for prm in split_top(d.sig[pm.end():q - 1]):
                    name, _, ty = prm.partition(":")
                    quals = [q for q in ("comptime", "noalias") if re.search(r"\b%s\b" % q, name)]
                    name = re.sub(r"\b(comptime|noalias)\b", "", name).strip()
                    if name in ("self",):
                        continue
                    d.params.append((name, " ".join(quals + [ty.strip()])))
                d.ret = d.sig[q:].strip() or None
                decls.append(d)
                pos = (match[p] + 1) if masked[p] == "{" else p + 1
                continue
            # const or var: up to the `;` that ends it, at brace depth 0.
            p, depth = m.end(), 0
            while True:
                ch = masked[p]
                if ch in "({[":
                    depth += 1
                elif ch in ")}]":
                    depth -= 1
                elif ch == ";" and depth == 0:
                    break
                p += 1
            d.text = text[start:p + 1]
            d.value = " ".join(text[m.end():p].split()).lstrip(":").strip()
            cm = re.search(r"=\s*(?:extern\s+|packed\s+)?(struct|enum|union|error)\b[^{]*\{", masked[m.end():p + 1])
            d.container = cm.group(1) if cm else None
            if d.container in ("struct", "union", "enum"):
                ob = m.end() + cm.end() - 1
                d.fields = fields_of(text[ob + 1:match[ob]], masked[ob + 1:match[ob]], d.container)
                d.decl_head = " ".join(text[start:ob].split())
                decls.append(d)
                walk(ob + 1, match[ob], d.name if owner is None else owner + "." + d.name, d.pub)
            else:
                d.fields = None
                decls.append(d)
            pos = p + 1

    walk(0, len(masked), None, True)
    return decls, intro


def fields_of(body, mbody, kind):
    """The field lines of a container: everything at depth 0 that is not a declaration."""
    out, depth, doc = [], 0, []
    for line, mline in zip(body.split("\n"), mbody.split("\n")):
        s = line.strip()
        if depth == 0:
            if s.startswith("///"):
                doc.append(s)
            elif mline.strip() and not re.match(r"^(pub\s+)?(inline\s+)?(fn|const|var|test|comptime)\b", mline.strip()):
                out += doc + [s]
                doc = []
            elif mline.strip():
                doc = []
        depth += mline.count("{") + mline.count("(") - mline.count("}") - mline.count(")")
    return out


def build(slot_names, slot_page):
    root, _ = read("levilamina.zig")
    exported = {}
    for d in root:
        if d.kind == "const" and d.pub:
            m = re.match(r"^(core|props)\.(\w+)$", d.value.lstrip("= ").strip())
            if m:
                exported[(m.group(1), m.group(2))] = d.name
    titles = {s: (en, zh) for s, en, zh in cpp.TOPICS if s != "types"}
    titles.update({s: (en, zh) for s, en, zh in EXTRA_TOPICS})
    pages = {s: Page("zig", s, "Zig: " + en, "Zig：" + zh, []) for s, (en, zh) in titles.items()}
    missing, entries = [], []
    groups = {}
    all_decls = {}
    for fn, module in (("core.zig", "core"), ("props_gen.zig", "props"), ("nbt.zig", "nbt")):
        decls, intro = read(fn)
        if fn == "core.zig":
            pages["core"].intro = intro
        if fn == "nbt.zig":
            pages["nbt"].intro = intro
        all_decls[fn] = decls
        for d in decls:
            top = d.owner.split(".")[0] if d.owner else d.name
            if module == "nbt":
                public = d.pub
                prefix = "nbt."
                page = "nbt"
            else:
                if d.owner is None:
                    public = d.pub and ((module, d.name) in exported or module == "props")
                else:
                    public = d.pub and ((module, top) in exported or module == "props")
                prefix = "" if (module, top) in exported else "props."
                page = PAGES.get(top) or next((pg for pre, pg in PREFIX_PAGES
                                               if top.startswith(pre) and top[len(pre):][:1].isupper()), None)
                if public and page is None:
                    missing.append("%s %s" % (fn, top))
                    continue
            if not public:
                continue
            d.page = page
            path = "%s/%s" % (SRC_REL, fn)
            if path not in pages[page].sources:
                pages[page].sources.append(path)
            if d.kind == "fn":
                qual = prefix + ("%s.%s" % (d.owner, d.name) if d.owner else d.name)
                e = Entry("zig", "method" if d.owner else "fn", d.name, qual, d.sig,
                          "%s/%s:%d" % (SRC_REL, fn, d.line))
                e.doc, e.params, e.ret = d.doc, d.params, d.ret
                e.zdecl = d
                key = (page, prefix + d.owner) if d.owner else (page, "functions")
                if key not in groups:
                    if d.owner:
                        missing.append("container %s" % d.owner)
                        continue
                    g = Group("Functions", "functions", "functions")
                    g.title_zh = "函数"
                    groups[key] = g
                    pages[page].groups.insert(0, g)
                groups[key].entries.append(e)
                entries.append(e)
            elif d.fields is not None or d.container:
                name = prefix + (d.owner + "." + d.name if d.owner else d.name)
                g = Group("`%s`" % name, name, "type")
                g.doc = d.doc
                if d.container == "error":
                    lines = d.text.split("\n")
                    pad = min((len(l) - len(l.lstrip()) for l in lines[1:] if l.strip()), default=0)
                    g.decl = "\n".join([lines[0].strip()] + ["    " + l[pad:].strip() if not l.strip().startswith("}") else l.strip() for l in lines[1:]])
                else:
                    body = "\n".join("    " + l for l in (d.fields or []))
                    g.decl = d.decl_head + " {\n" + body + ("\n" if body else "") + "    // ...\n};" if d.fields else d.decl_head + " { ... };"
                    if d.container == "enum" and d.fields:
                        g.decl = d.decl_head + " {\n" + body + "\n};"
                groups[(page, name)] = g
                pages[page].groups.append(g)
            else:
                e = Entry("zig", "const", d.name, prefix + (d.owner + "." + d.name if d.owner else d.name),
                          " ".join(d.text.split()), "%s/%s:%d" % (SRC_REL, fn, d.line))
                e.doc = d.doc
                g = groups.get((page, "constants"))
                if not g:
                    g = Group("Declarations", "declarations", "functions")
                    g.title_zh = "声明"
                    groups[(page, "constants")] = g
                    pages[page].groups.insert(0, g)
                g.entries.append(e)
                entries.append(e)
    if missing:
        raise SystemExit("gen-api-docs: these Zig declarations have no page; add them to PAGES in "
                         "tools/apidocs/zig.py:\n  " + "\n  ".join(missing))
    _slots(entries, all_decls, slot_names)
    return [pages[s] for s in titles if pages[s].groups], entries


def _slots(entries, all_decls, slot_names):
    fns = {}
    for fn, decls in all_decls.items():
        for d in decls:
            if d.kind == "fn":
                fns[(fn, d.owner, d.name)] = d
    memo = {}

    def walk(d, depth, seen):
        key = (d.file, d.owner, d.name)
        if key in memo:
            return memo[key]
        res = []
        for m in re.finditer(r"\bslot\(\s*\"(\w+)\"\s*\)", d.body):
            if m.group(1) in slot_names and m.group(1) not in res:
                res.append(m.group(1))
        if depth < 4:
            callees = []
            for m in re.finditer(r"\bself\.(\w+)\(", d.mbody):
                c = fns.get((d.file, d.owner, m.group(1)))
                callees += [c] if c else []
            for m in re.finditer(r"\bcore\.(\w+)\(", d.mbody):
                c = fns.get(("core.zig", None, m.group(1)))
                callees += [c] if c else []
            for m in re.finditer(r"(?<![\w.])(\w+)\(", d.mbody):
                c = fns.get((d.file, None, m.group(1)))
                callees += [c] if c and c is not d else []
            for c in callees:
                ck = (c.file, c.owner, c.name)
                if ck in seen:
                    continue
                for s in walk(c, depth + 1, seen | {key}):
                    if s not in res:
                        res.append(s)
        if depth == 0:
            memo[key] = res
        return res

    for e in entries:
        if e.kind in ("fn", "method"):
            e.slots = walk(e.zdecl, 0, frozenset())
