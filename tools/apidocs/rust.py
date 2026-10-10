# -*- coding: utf-8 -*-
"""The Rust reference: the public API of the `levilamina` crate in bindings/rust/pier-rs.

There is no rustc here, so the crate is read as text. Strings, char literals and comments are
blanked out first, which leaves braces that can be counted; each `impl` block then says
which type its methods belong to. An item is public when it is declared `pub` and either its
module chain is `pub mod` from the crate root or a reachable module re-exports it with
`pub use`. `#[doc(hidden)]` items and names starting with `__` are left out, as rustdoc
leaves them out. The methods `accessors!` generates line by line are expanded here, since
they appear in the source as table rows and not as `pub fn`.
"""

import os
import re

from .model import Entry, Group, Page, ROOT

CRATE = os.path.join(ROOT, "bindings", "rust", "pier-rs")
SRC_REL = "bindings/rust/pier-rs/src"

# Source file or directory -> page. Methods go to the page of their type's definition.
PAGES = [
    ("lib.rs", "root"), ("context.rs", "root"), ("rt/", "root"),
    ("player/", "player"), ("sel.rs", "player"),
    ("entity/", "entity"), ("world/", "world"), ("block/", "block"),
    ("item.rs", "item"), ("container.rs", "container"),
    ("event/names.rs", "event-names"), ("event/", "event"),
    ("command.rs", "command"), ("gui.rs", "gui"), ("scoreboard.rs", "scoreboard"),
    ("packet.rs", "packet"), ("server.rs", "server"), ("host.rs", "host"),
    ("bus.rs", "bus"), ("service.rs", "service"), ("lane.rs", "lane"),
    ("kvdb.rs", "kvdb"), ("money.rs", "money"), ("nbt/", "nbt"), ("sim.rs", "sim"),
    ("dimensions.rs", "dimensions"), ("client.rs", "client"), ("registry.rs", "registry"),
    ("types.rs", "types"),
]
TITLES = {
    "root": ("levilamina: lifecycle, logging and errors", "levilamina：生命周期、日志与错误"),
    "player": ("levilamina::player", "levilamina::player · 玩家"),
    "entity": ("levilamina::entity", "levilamina::entity · 实体"),
    "world": ("levilamina::world", "levilamina::world · 世界"),
    "block": ("levilamina::block", "levilamina::block · 方块"),
    "item": ("levilamina::item", "levilamina::item · 物品"),
    "container": ("levilamina::container", "levilamina::container · 容器"),
    "event": ("levilamina::event", "levilamina::event · 事件"),
    "event-names": ("levilamina::event::names", "levilamina::event::names · 事件名"),
    "command": ("levilamina::command", "levilamina::command · 命令"),
    "gui": ("levilamina::gui", "levilamina::gui · 表单"),
    "scoreboard": ("levilamina::scoreboard", "levilamina::scoreboard · 计分板"),
    "packet": ("levilamina::packet", "levilamina::packet · 数据包"),
    "server": ("levilamina::server", "levilamina::server · 服务器"),
    "host": ("levilamina::Host", "levilamina::Host · 宿主与任务"),
    "bus": ("levilamina::bus", "levilamina::bus · 事件总线"),
    "service": ("levilamina::service", "levilamina::service · 服务"),
    "lane": ("levilamina::lane", "levilamina::lane · 快速通道"),
    "kvdb": ("levilamina::kvdb", "levilamina::kvdb · 键值数据库"),
    "money": ("levilamina::money", "levilamina::money · 经济"),
    "nbt": ("levilamina::nbt", "levilamina::nbt · NBT"),
    "sim": ("levilamina::sim", "levilamina::sim · 模拟玩家"),
    "dimensions": ("levilamina::dimensions", "levilamina::dimensions · 自定义维度"),
    "client": ("levilamina::client", "levilamina::client · 客户端"),
    "registry": ("levilamina::registry", "levilamina::registry · 注册表"),
    "types": ("levilamina::types", "levilamina::types · 通用类型"),
}
INTRO_FILE = {"root": "lib.rs", "player": "player/mod.rs", "entity": "entity/mod.rs",
              "world": "world/mod.rs", "block": "block/mod.rs", "event": "event/mod.rs",
              "nbt": "nbt/mod.rs"}

ITEM = re.compile(
    r"^[ \t]*((?:pub(?:\([^)]*\))?[ \t]+)?)((?:(?:unsafe|const|async|extern|default)\s+)*)"
    r"(fn|struct|enum|trait|type|const|static|mod|impl|union|use|macro_rules!)(?![\w!])", re.M)


def mask(t):
    """t with every comment, string and char literal replaced by spaces, newlines kept, so
    positions in it are positions in t."""
    out = list(t)
    n = len(t)

    def blank(a, b):
        for k in range(a, min(b, n)):
            if out[k] != "\n":
                out[k] = " "

    i = 0
    raw = re.compile(r"b?r(#*)\"")
    char = re.compile(r"b?'(?:\\(?:x[0-9a-fA-F]{2}|u\{[0-9a-fA-F]+\}|.)|[^\\'\n])'")
    while i < n:
        if t.startswith("//", i):
            j = t.find("\n", i)
            j = n if j < 0 else j
            blank(i, j)
            i = j
            continue
        if t.startswith("/*", i):
            depth, j = 0, i
            while j < n:
                if t.startswith("/*", j):
                    depth += 1
                    j += 2
                elif t.startswith("*/", j):
                    depth -= 1
                    j += 2
                    if depth == 0:
                        break
                else:
                    j += 1
            blank(i, j)
            i = j
            continue
        prev_ident = i > 0 and (t[i - 1].isalnum() or t[i - 1] == "_")
        m = raw.match(t, i)
        if m and not prev_ident:
            end = t.find('"' + m.group(1), m.end())
            blank(i, end + 1 + len(m.group(1)))
            i = end + 1 + len(m.group(1))
            continue
        if t[i] == '"' or (t.startswith('b"', i) and not prev_ident):
            j = i + (2 if t[i] == "b" else 1)
            while t[j] != '"':
                j += 2 if t[j] == "\\" else 1
            blank(i, j + 1)
            i = j + 1
            continue
        if t[i] in "'b" and not prev_ident:
            m = char.match(t, i)
            if m:
                blank(i, m.end())
                i = m.end()
                continue
        i += 1
    return "".join(out)


def braces(masked):
    """Position of each opening brace -> position of its closing brace."""
    match, stack = {}, []
    for i, ch in enumerate(masked):
        if ch == "{":
            stack.append(i)
        elif ch == "}":
            match[stack.pop()] = i
    return match


def header_end(masked, p, until_semicolon):
    """The end of an item header from p: the `{` opening its body or the `;` closing it, at
    parenthesis and bracket depth 0. Constants, statics and aliases end at the `;`."""
    depth = 0
    for i in range(p, len(masked)):
        ch = masked[i]
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        elif depth == 0 and ch == ";":
            return i, ";"
        elif depth == 0 and ch == "{" and not until_semicolon:
            return i, "{"
        elif ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
    raise SystemExit("gen-api-docs: an item header from offset %d never ends" % p)


def doc_and_attrs(lines, line_no):
    """The `///` lines and attributes directly above line `line_no` (0-based)."""
    docs, attrs = [], []
    k = line_no - 1
    while k >= 0:
        s = lines[k].strip()
        if s.startswith("///"):
            docs.insert(0, lines[k].strip()[3:])
        elif s.startswith("#["):
            attrs.insert(0, s)
        elif s.endswith("]") and not s.startswith("//"):
            j = k
            while j >= 0 and not lines[j].strip().startswith("#["):
                j -= 1
            attrs.insert(0, " ".join(x.strip() for x in lines[j:k + 1]))
            k = j
        else:
            break
        k -= 1
    return docs, attrs


def split_top(text, sep=","):
    out, depth, cur = [], 0, []
    for ch in text:
        if ch in "(<[{":
            depth += 1
        elif ch in ")>]}":
            depth -= 1
        if ch == sep and depth == 0:
            out.append("".join(cur))
            cur = []
        else:
            cur.append(ch)
    if "".join(cur).strip():
        out.append("".join(cur))
    return [x.strip() for x in out if x.strip()]


def fn_parts(sig):
    """(parameters, return type) of a fn signature."""
    flat = " ".join(sig.split()).replace("->", "\x00")
    m = re.search(r"\bfn\s+\w+\s*(<.*?>)?\s*\(", flat)
    start = m.end()
    depth, i = 1, start
    while depth:
        depth += {"(": 1, ")": -1}.get(flat[i], 0)
        i += 1
    params = []
    for p in split_top(flat[start:i - 1].replace("\x00", "->")):
        if re.fullmatch(r"(&\s*('\w+\s+)?(mut\s+)?)?(mut\s+)?self(\s*:.*)?", p):
            continue
        name, _, ty = p.partition(":")
        params.append((name.strip(), ty.strip()))
    rest = flat[i:]
    ret = None
    if rest.lstrip().startswith("\x00"):
        ret = re.split(r"\bwhere\b", rest.lstrip()[1:])[0].strip().replace("\x00", "->")
    return params, ret


def dedent(text):
    lines = text.split("\n")
    pad = min((len(l) - len(l.lstrip()) for l in lines[1:] if l.strip()), default=0)
    first_pad = len(lines[0]) - len(lines[0].lstrip())
    pad = min(pad, first_pad) if len(lines) > 1 else first_pad
    return "\n".join(l[pad:] if len(l) >= pad else l.lstrip() for l in lines).strip("\n")


def deprecation(attrs):
    for a in attrs:
        if a.startswith("#[deprecated"):
            since = re.search(r'since\s*=\s*"([^"]*)"', a)
            note = re.search(r'note\s*=\s*"((?:\\.|[^"\\])*)"', a) or re.search(r'deprecated\s*=\s*"((?:\\.|[^"\\])*)"', a)
            return (since.group(1) if since else None, note.group(1) if note else "")
    return None


def hidden(attrs, name):
    return name.startswith("__") or any("doc(hidden)" in a.replace(" ", "") for a in attrs)


class Item:
    def __init__(self, kind, name, vis, file, line, sig, docs, attrs):
        self.kind, self.name, self.vis = kind, name, vis
        self.file, self.line, self.sig = file, line, sig
        self.docs, self.attrs = docs, attrs
        self.owner = None      # impl type for a method
        self.trait = None      # trait name for a trait method
        self.body = ""         # masked body text, for the slot analysis
        self.decl = None
        self.entry = None


class Crate:
    def __init__(self):
        self.items = []        # every fn, type, const, macro found
        self.mods = {}         # module path tuple -> is the `mod` declaration pub
        self.reexports = set()  # names some reachable module re-exports with pub use
        self.trait_impls = {}  # type -> [trait names]
        self.intros = {}
        self._read()

    def _read(self):
        root = os.path.join(CRATE, "src")
        for dp, _, fs in os.walk(root):
            for fn in sorted(fs):
                if fn.endswith(".rs"):
                    p = os.path.join(dp, fn)
                    rel = os.path.relpath(p, root).replace(os.sep, "/")
                    with open(p, encoding="utf-8") as f:
                        self._file(rel, f.read())

    def modpath(self, rel):
        parts = rel[:-3].split("/")
        if parts[-1] in ("mod", "lib"):
            parts = parts[:-1]
        return tuple(parts)

    def _file(self, rel, text):
        masked = mask(text)
        match = braces(masked)
        lines = text.split("\n")
        here = self.modpath(rel)
        # The module documentation is the run of `//!` lines the file opens with; an inline
        # module further down, such as `pub mod prelude`, carries its own.
        intro = []
        for l in lines:
            if l.strip().startswith("//!"):
                intro.append(l.strip()[3:])
            elif l.strip() or intro:
                break
        self.intros[rel] = intro
        self._walk(rel, text, masked, match, lines, 0, len(masked), here, None)
        for m in re.finditer(r"^accessors!\s*\{\s*(\w+)\s*;(.*?)^\}", text, re.M | re.S):
            self._accessors(rel, text, m)

    def _walk(self, rel, text, masked, match, lines, a, b, mod, ctx):
        pos = a
        while True:
            m = ITEM.search(masked, pos, b)
            if not m:
                return
            vis, kw = m.group(1).strip(), m.group(3)
            start = m.start() + len(m.group(0)) - len(m.group(0).lstrip())
            line_no = text.count("\n", 0, start)
            end, opener = header_end(masked, m.end(), kw in ("const", "static", "type", "use"))
            head = text[start:end].rstrip()
            docs, attrs = doc_and_attrs(lines, line_no)
            close = match.get(end) if opener == "{" else end
            nm = re.match(r"\s*(?:r#)?(\w+)", masked[m.end():end])
            name = nm.group(1) if nm else ""
            if kw == "impl":
                hm = re.search(r"\bimpl\s*(?:<.*?>)?\s*(.*)$", " ".join(head.split()))
                target = hm.group(1)
                trait = None
                if " for " in " %s " % target:
                    trait, target = re.split(r"\s+for\s+", target, 1)
                    trait = re.sub(r"<.*$", "", trait).split("::")[-1].strip()
                target = re.sub(r"\bwhere\b.*$", "", target).strip()
                tname = re.sub(r"<.*$", "", target).split("::")[-1].strip().lstrip("&").strip()
                if trait:
                    self.trait_impls.setdefault(tname, [])
                    if trait not in self.trait_impls[tname]:
                        self.trait_impls[tname].append(trait)
                else:
                    self._walk(rel, text, masked, match, lines, end + 1, close, mod, ("impl", tname))
                pos = close + 1
                continue
            if kw == "mod":
                if opener == "{":
                    if not hidden(attrs, name):
                        self.mods[mod + (name,)] = vis == "pub"
                        self._walk(rel, text, masked, match, lines, end + 1, close, mod + (name,), None)
                else:
                    self.mods[mod + (name,)] = vis == "pub"
                pos = close + 1
                continue
            if kw == "use":
                if vis == "pub":
                    body = " ".join(head.split())
                    for n in re.findall(r"(\w+)(?:\s+as\s+(\w+))?\s*(?=[,}]|$)", body.split("use", 1)[1]):
                        self.reexports.add(n[1] or n[0])
                pos = end + 1
                continue
            if kw == "macro_rules!":
                if any(x.startswith("#[macro_export]") for x in attrs) and not hidden(attrs, name):
                    it = Item("macro", name, "pub", rel, line_no + 1, "macro_rules! %s" % name, docs, attrs)
                    it.mod = ()
                    it.decl = self._macro_arms(masked[end + 1:close], text[end + 1:close])
                    self.items.append(it)
                pos = close + 1
                continue
            it = Item(kw, name, vis, rel, line_no + 1, dedent(head), docs, attrs)
            it.mod = mod
            if ctx and ctx[0] == "impl":
                it.owner = ctx[1]
            if ctx and ctx[0] == "trait":
                it.trait = ctx[1]
                it.vis = "pub"
            if kw == "fn":
                it.body = masked[end:close + 1] if opener == "{" else ""
            elif kw in ("struct", "enum", "union"):
                it.decl = self._type_decl(kw, text, masked, start, end, close, opener)
            elif kw == "trait":
                it.decl = dedent(head) + " { /* ... */ }"
                if opener == "{":
                    self._walk(rel, text, masked, match, lines, end + 1, close, mod, ("trait", name))
            elif kw in ("type", "const", "static"):
                it.decl = dedent(text[start:end + 1])
            if not (ctx and ctx[0] == "fn"):
                self.items.append(it)
            pos = (close + 1) if opener == "{" else end + 1

    def _macro_arms(self, masked_body, body):
        arms = []
        for m in re.finditer(r"\(([^()]*(?:\([^()]*\)[^()]*)*)\)\s*=>", masked_body):
            pat = " ".join(body[m.start():m.end() - 2].split())
            if not pat.startswith("(@"):
                arms.append(pat)
        return arms

    def _type_decl(self, kw, text, masked, start, end, close, opener):
        head = dedent(text[start:end]).rstrip()
        if opener == ";":
            inner = re.search(r"\((.*)\)", masked[start:end + 1], re.S)
            if inner and inner.group(1).strip() and not inner.group(1).strip().startswith("pub"):
                return re.sub(r"\(.*\)", "(/* private */)", head, flags=re.S) + ";"
            return dedent(text[start:end + 1])
        body = text[end + 1:close]
        mbody = masked[end + 1:close]
        if kw == "enum":
            return head + " {\n" + "\n".join("    " + l for l in dedent(body).split("\n") if l.strip()) + "\n}"
        kept, private, pending = [], False, []
        depth = 0
        for line, mline in zip(body.split("\n"), mbody.split("\n")):
            s = line.strip()
            if depth == 0 and s.startswith("///"):
                pending.append(s)
            elif depth == 0 and s.startswith("#["):
                pending.append(s)
            elif depth == 0 and mline.strip():
                if s.startswith("pub "):
                    kept += pending + [s]
                else:
                    private = True
                pending = []
            depth += mline.count("{") + mline.count("(") - mline.count("}") - mline.count(")")
        if private:
            kept.append("// private fields")
        if not kept:
            return head + " {}"
        return head + " {\n" + "\n".join("    " + l for l in kept) + "\n}"

    def _accessors(self, rel, text, m):
        tname = m.group(1)
        base = text.count("\n", 0, m.start(2))
        docs = []
        for k, line in enumerate(m.group(2).split("\n")):
            s = line.strip()
            if s.startswith("///"):
                docs.append(s[3:])
                continue
            am = re.match(r"^(f64|i32|bool|str)\s+(\w+)\s*=\s*(\w+)\s*;", s)
            if am:
                kind, name, konst = am.groups()
                ret = {"str": "String"}.get(kind, kind)
                it = Item("fn", name, "pub", rel, base + k + 1,
                          "pub fn %s(&self) -> Result<%s>" % (name, ret), docs, [])
                it.mod = self.modpath(rel)
                it.owner = tname
                it.body = "self.%s(%s)" % ("text" if kind == "str" else "num", konst)
                it.konst = konst
                self.items.append(it)
                docs = []
            elif s:
                docs = []

    def reachable(self, mod):
        for k in range(1, len(mod) + 1):
            if not self.mods.get(mod[:k], False):
                return False
        return True


def page_of(rel):
    for prefix, slug in PAGES:
        if rel == prefix or (prefix.endswith("/") and rel.startswith(prefix)):
            return slug
    raise SystemExit("gen-api-docs: %s/%s has no page; add it to PAGES in tools/apidocs/rust.py"
                     % (SRC_REL, rel))


def build(slot_names):
    c = Crate()
    types = {}
    for it in c.items:
        if it.kind in ("struct", "enum", "trait", "type", "union") and it.owner is None and it.trait is None:
            public = it.vis == "pub" and (c.reachable(it.mod) or it.name in c.reexports)
            if public and not hidden(it.attrs, it.name):
                types.setdefault(it.name, it)
    pages = {}
    for slug, (en, zh) in TITLES.items():
        pages[slug] = Page("rust", slug, en, zh, [])
    for slug, f in INTRO_FILE.items():
        pages[slug].intro = c.intros.get(f, [])
    for rel, intro in c.intros.items():
        slug = page_of(rel)
        if slug not in INTRO_FILE and intro and not pages[slug].intro:
            pages[slug].intro = intro

    groups = {}

    def type_group(name):
        t = types[name]
        slug = page_of(t.file)
        key = (slug, name)
        if key not in groups:
            g = Group("`%s`" % name, name, "type")
            g.doc = t.docs
            g.decl = t.decl or t.sig
            derived = []
            for a in t.attrs:
                dm = re.match(r"#\[derive\((.*)\)\]", a)
                if dm:
                    derived += [x.strip() for x in dm.group(1).split(",") if x.strip()]
            g.traits = derived + [x for x in c.trait_impls.get(name, []) if x not in derived]
            groups[key] = g
            pages[slug].groups.append(g)
            src = "%s/%s" % (SRC_REL, t.file)
            if src not in pages[slug].sources:
                pages[slug].sources.append(src)
        return groups[key]

    def free_group(slug, key, title, title_zh, kind):
        if (slug, key) not in groups:
            g = Group(title, key, kind)
            g.title_zh = title_zh
            groups[(slug, key)] = g
            pages[slug].groups.append(g)
        return groups[(slug, key)]

    # Types in the order they are defined, so a page reads in the order of its source.
    ordered = sorted(c.items, key=lambda it: (page_of(it.file), it.file.count("/") and not it.file.endswith("mod.rs"), it.file, it.line))
    for it in ordered:
        if it.kind in ("struct", "enum", "trait", "type", "union") and it.name in types and types[it.name] is it:
            type_group(it.name)

    entries = []
    for it in ordered:
        public = it.vis == "pub" and not hidden(it.attrs, it.name)
        if it.kind == "fn" and it.owner:
            if it.owner not in types or not public:
                continue
            e = Entry("rust", "method", it.name, "%s::%s" % (it.owner, it.name), it.sig,
                      "%s/%s:%d" % (SRC_REL, it.file, it.line), anchor="%s.%s" % (it.owner, it.name))
            g = type_group(it.owner)
        elif it.kind == "fn" and it.trait:
            if it.trait not in types:
                continue
            e = Entry("rust", "method", it.name, "%s::%s" % (it.trait, it.name), it.sig,
                      "%s/%s:%d" % (SRC_REL, it.file, it.line), anchor="%s.%s" % (it.trait, it.name))
            g = type_group(it.trait)
        elif it.kind == "fn":
            if not public or not (c.reachable(it.mod) or it.name in c.reexports):
                continue
            slug = page_of(it.file)
            path = "::".join(it.mod)
            e = Entry("rust", "fn", it.name, "%s::%s" % (path, it.name) if path else it.name, it.sig,
                      "%s/%s:%d" % (SRC_REL, it.file, it.line), anchor="fn.%s" % it.name)
            g = free_group(slug, "functions", "Functions", "函数", "functions")
        elif it.kind == "macro":
            slug = page_of(it.file)
            e = Entry("rust", "macro", it.name, "%s!" % it.name, "\n".join("%s! %s" % (it.name, a) for a in it.decl) or it.sig,
                      "%s/%s:%d" % (SRC_REL, it.file, it.line), anchor="macro.%s" % it.name)
            g = free_group(slug, "macros", "Macros", "宏", "functions")
        elif it.kind in ("const", "static") and it.owner is None and it.trait is None:
            if not public or not (c.reachable(it.mod) or it.name in c.reexports):
                continue
            slug = page_of(it.file)
            g = free_group(slug, "constants", "Constants", "常量", "consts")
            vm = re.search(r"=\s*(.*?);\s*$", " ".join(it.decl.split()))
            tm = re.search(r":\s*(.*?)\s*=", " ".join(it.decl.split()))
            g.consts.append((it.name, vm.group(1) if vm else "", it.docs, False))
            g.const_types = getattr(g, "const_types", {})
            g.const_types[it.name] = tm.group(1) if tm else ""
            continue
        else:
            continue
        e.doc = it.docs
        e.deprecated = deprecation(it.attrs)
        if it.kind == "fn":
            e.params, e.ret = fn_parts(it.sig)
        e.item = it
        it.entry = e
        g.entries.append(e)
        src = "%s/%s" % (SRC_REL, it.file)
        slug = page_of(it.file) if it.kind != "fn" or not (it.owner or it.trait) else page_of(types[it.owner or it.trait].file)
        if src not in pages[slug].sources:
            pages[slug].sources.append(src)
        entries.append(e)

    with open(os.path.join(CRATE, "src", "lib.rs"), encoding="utf-8") as f:
        lib = f.read()
    pm = re.search(r"^pub mod prelude \{\n(.*?)^\}", lib, re.M | re.S)
    if pm:
        lines = pm.group(1).split("\n")
        g = Group("`prelude`", "prelude", "module")
        g.doc = [l.strip()[3:].lstrip() for l in lines if l.strip().startswith("//!")]
        g.decl = "pub mod prelude {\n" + "\n".join(l for l in lines if l.strip() and not l.strip().startswith("//")) + "\n}"
        pages["root"].groups.append(g)
    _slots(c, entries, slot_names)
    for p in pages.values():
        p.sources.sort(key=lambda s: (not s.endswith(("mod.rs", "lib.rs")), s))
    return [pages[s] for s in TITLES if pages[s].groups], entries, types


def _slots(c, entries, slot_names):
    """Fills in each entry's slots: the ones its body names, and the ones reached through
    `self.f(`, `Self::f(`, `Type::f(`, `module::f(` and a call to a free function of the same
    file, followed four calls deep. A call on any other receiver needs the receiver's type,
    which a text reader does not have, so those slots are not listed."""
    methods, frees = {}, {}
    for it in c.items:
        if it.kind != "fn":
            continue
        if it.owner or it.trait:
            methods.setdefault((it.owner or it.trait, it.name), []).append(it)
        else:
            frees.setdefault((it.file, it.name), []).append(it)
            frees.setdefault((it.mod[-1] if it.mod else "", it.name), []).append(it)
    memo = {}

    def direct(it):
        found = []
        for rx in (r"require_slot!\(\s*(\w+)", r"has_slot!\(\s*(\w+)", r"opt_slot!\(\s*(\w+)", r"api\(\)\s*\.\s*(\w+)"):
            for m in re.finditer(rx, it.body):
                found.append((m.start(), m.group(1)))
        return [s for _, s in sorted(found) if s in slot_names]

    def callees(it):
        out = []
        owner = it.owner or it.trait
        for m in re.finditer(r"\bself\s*\.\s*(\w+)\s*\(|\bSelf::(\w+)\s*\(", it.body):
            out += methods.get((owner, m.group(1) or m.group(2)), [])
        for m in re.finditer(r"\b([A-Z]\w*)::(\w+)\s*\(", it.body):
            out += methods.get((m.group(1), m.group(2)), [])
        for m in re.finditer(r"\b([a-z_]\w*)::(\w+)\s*\(", it.body):
            out += frees.get((m.group(1), m.group(2)), [])
        for m in re.finditer(r"(?<![\w.:!])([a-z_]\w*)\s*\(", it.body):
            out += frees.get((it.file, m.group(1)), [])
        return out

    def walk(it, depth, seen):
        if id(it) in memo:
            return memo[id(it)]
        res = list(direct(it))
        if depth < 4:
            for callee in callees(it):
                if id(callee) in seen:
                    continue
                for s in walk(callee, depth + 1, seen | {id(it)}):
                    if s not in res:
                        res.append(s)
        if depth == 0:
            memo[id(it)] = res
        return res

    for e in entries:
        if e.kind in ("fn", "method"):
            e.slots = walk(e.item, 0, frozenset())
