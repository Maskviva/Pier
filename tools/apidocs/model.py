# -*- coding: utf-8 -*-
"""The page model of the generated API reference, and its Markdown renderer.

A page holds groups and a group holds entries. Each binding's parser fills them in from the
source; the renderer writes them in the layout of the LegacyScriptEngine documentation: one
heading per interface, its declaration, the description taken from the source comment, then
the parameters, the return type and the slots of abi.h the interface reaches.

Rendering happens after every binding has been parsed, so a description can link to an
interface on another page and a slot entry can list its callers in every binding.
"""

import os
import posixpath
import re

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
DOCS = os.path.join(ROOT, "docs")
LANGS = ("en", "zh")
BINDING_ORDER = ("rust", "go", "zig", "cpp")
FENCE = {"rust": "rust", "go": "go", "zig": "zig", "cpp": "c"}
BINDING_NAME = {"rust": "Rust", "go": "Go", "zig": "Zig", "cpp": "C++"}

L = {
    "zh": dict(
        params="参数", rtype="返回值类型", slots="对应槽位", deprecated="已弃用",
        deprecated_since="已弃用（自 %s 起）", name="名称", value="值", desc="说明",
        fields="字段", call="调用形式", section="所在分节", order="表内序号",
        order_fmt="第 %d 个槽位（从 0 数起）", users="各绑定里的调用方", traits="实现的 trait",
        none_in="无", more="等，共 %d 个", unsupported="这个常量在当前的引擎版本上取不到值",
        group_note="分组说明", section_notes="abi.h 里的分节说明", source="源码",
        sep="：", list_sep="、",
    ),
    "en": dict(
        params="Parameters", rtype="Return type", slots="Slots", deprecated="Deprecated",
        deprecated_since="Deprecated since %s", name="Name", value="Value", desc="Description",
        fields="Fields", call="Call", section="Section of abi.h", order="Position in the table",
        order_fmt="slot %d, counting from 0", users="Callers in each binding", traits="Implements",
        none_in="none", more="and more, %d in all", unsupported="the current engine version gives no value for this constant",
        group_note="Group note", section_notes="Section notes in abi.h", source="Source",
        sep=": ", list_sep=", ",
    ),
}


class Entry:
    """One interface: a function, method, macro, constant or slot."""

    def __init__(self, binding, kind, name, qual, sig, src, anchor=None):
        self.binding = binding
        self.kind = kind
        self.name = name
        self.qual = qual
        self.anchor = anchor or qual
        self.sig = sig
        self.src = src
        self.doc = []          # raw comment lines, converted at render time
        self.params = []       # (name, type)
        self.ret = None
        self.slots = []        # slot names of abi.h, in order of first use
        self.deprecated = None  # (since or None, note)
        self.call = None       # C++ only: the call form
        self.section = None    # C++ only: the abi.h section title
        self.order = None      # C++ only: the slot's position in PierApi
        self.note = []         # C++ only: the group note above a run of slots
        self.page = None


class Group:
    """A heading on a page: a type with its methods, or a set of free functions."""

    def __init__(self, title, anchor, kind="type"):
        self.title = title
        self.anchor = anchor
        self.kind = kind
        self.doc = []
        self.decl = None
        self.traits = []
        self.consts = []       # (name, value, doc lines, unsupported) for a table
        self.entries = []
        self.title_zh = None   # a group of free functions has a translated title


class Page:
    def __init__(self, binding, slug, title_en, title_zh, sources):
        self.binding = binding
        self.slug = slug
        self.title = {"en": title_en, "zh": title_zh}
        self.sources = sources
        self.intro = []        # raw comment lines
        self.groups = []
        self.section_notes = []  # C++ only: (section title, raw lines)

    def path(self, lang):
        return "%s/api/%s/%s.md" % (lang, self.binding, self.slug)


# Comment text into Markdown

_CODE_SPAN = re.compile(r"(`+)(.+?)\1")


_IDENT = re.compile(r"(?<![\w`*&.-])((?:[A-Za-z]\w*::)+~?\w+(?:\(\))?|[A-Za-z][A-Za-z0-9]*_\w*[A-Za-z0-9](?:\(\))?)(?![\w`])")


def _code_idents(text):
    """Puts the identifiers of a plain-text comment in code spans: a name with an underscore,
    such as PIER_PPROP_LEVEL or player_get_num, and a path such as Player::getLuck()."""
    out, pos = [], 0
    for m in _CODE_SPAN.finditer(text):
        out.append(_IDENT.sub(r"`\1`", text[pos:m.start()]))
        out.append(m.group(0))
        pos = m.end()
    out.append(_IDENT.sub(r"`\1`", text[pos:]))
    return "".join(out)


def _escape_prose(text, idents=False):
    """Escapes the characters Markdown would act on, outside code spans. Plain-text comments
    write `*out` and `Vec<T>` meaning the characters themselves."""
    if idents:
        text = _code_idents(text)
    out, pos = [], 0
    for m in _CODE_SPAN.finditer(text):
        out.append(_escape_plain(text[pos:m.start()]))
        out.append(m.group(0))
        pos = m.end()
    out.append(_escape_plain(text[pos:]))
    return "".join(out)


def _escape_plain(s):
    s = s.replace("\\", "\\\\")
    for ch in "*_[]":
        s = s.replace(ch, "\\" + ch)
    return s.replace("<", "&lt;").replace(">", "&gt;")


def plain_to_md(lines):
    """A plain-text comment (Go, Zig, C) as Markdown. A run of lines indented past the
    comment's own margin is laid out as written, in a text block; the rest are paragraphs,
    joined line by line."""
    lines = [l.rstrip() for l in lines]
    while lines and not lines[0].strip():
        lines.pop(0)
    while lines and not lines[-1].strip():
        lines.pop()
    if not lines:
        return ""
    margin = min(len(l) - len(l.lstrip()) for l in lines if l.strip())
    lines = [l[margin:] if l.strip() else "" for l in lines]
    blocks, para, pre = [], [], []

    def flush_para():
        if para:
            blocks.append(_escape_prose(" ".join(p.strip() for p in para), idents=True))
            para.clear()

    def flush_pre():
        while pre and not pre[-1].strip():
            pre.pop()
        if pre:
            m = min(len(l) - len(l.lstrip()) for l in pre if l.strip())
            body = "\n".join(l[m:] if l.strip() else "" for l in pre)
            blocks.append("```text\n%s\n```" % body)
            pre.clear()

    for l in lines:
        indented = l.startswith("  ") or l.startswith("\t")
        if not l.strip():
            if pre:
                pre.append("")
            else:
                flush_para()
        elif indented or (pre and l.lstrip().startswith(("-", "*")) and l.startswith(" ")):
            flush_para()
            pre.append(l.replace("\t", "    "))
        else:
            flush_pre()
            para.append(l)
    flush_para()
    flush_pre()
    return "\n\n".join(blocks)


_RUST_FENCE = re.compile(r"^```\s*(\w[\w,]*)?\s*$")


def rust_to_md(lines, resolve):
    """A Rust doc comment, which is Markdown already. Code fences become rust blocks with the
    hidden `# ` lines dropped, a heading becomes a bold line so it does not enter the page's
    table of contents, and an intra-doc link resolves through `resolve`."""
    out, in_code, keep = [], False, True
    for raw in lines:
        l = raw[1:] if raw.startswith(" ") else raw
        m = _RUST_FENCE.match(l.strip())
        if m:
            if not in_code:
                attrs = (m.group(1) or "").split(",")
                keep = not any(a in ("text",) for a in attrs)
                out.append("```rust" if keep else "```text")
                in_code = True
            else:
                out.append("```")
                in_code = False
            continue
        if in_code:
            if keep and (l.strip() == "#" or l.lstrip().startswith("# ")):
                continue
            out.append(l)
            continue
        h = re.match(r"^#{1,6}\s+(.*)$", l)
        if h:
            out.append("**%s**" % h.group(1).strip())
            continue
        out.append(_rust_inline(l, resolve))
    while out and not out[-1].strip():
        out.pop()
    while out and not out[0].strip():
        out.pop(0)
    return "\n".join(out)


def _rust_inline(line, resolve):
    def link(m):
        text, target = m.group(1), m.group(2)
        url = resolve(target)
        return "[%s](%s)" % (text, url) if url else text

    # [`Path`] and [text](crate::path), the two intra-doc forms this crate writes.
    line = re.sub(r"\[(`[^`\]]+`)\](?!\()", lambda m: link(_Pair(m.group(1), m.group(1).strip("`"))), line)
    line = re.sub(r"\[([^\]]+)\]\(((?:crate|self|super)::[^)\s]+|[A-Z]\w*(?:::\w+)*)\)",
                  lambda m: link(_Pair(m.group(1), m.group(2))), line)
    out, pos = [], 0
    for m in re.finditer(r"`[^`]*`|\[[^\]]*\]\([^)]*\)", line):
        out.append(re.sub(r"<(?=[A-Za-z/])", "&lt;", line[pos:m.start()]))
        out.append(m.group(0))
        pos = m.end()
    out.append(re.sub(r"<(?=[A-Za-z/])", "&lt;", line[pos:]))
    return "".join(out)


class _Pair:
    def __init__(self, a, b):
        self.a, self.b = a, b

    def group(self, i):
        return self.a if i == 1 else self.b


def one_line(md):
    """A description squeezed into one table cell."""
    md = re.sub(r"```\w*\n(.*?)\n```", lambda m: "`%s`" % " ".join(m.group(1).split()), md, flags=re.S)
    return " ".join(md.split()).replace("|", "\\|")


# Rendering

def _indent(text, n):
    pad = " " * n
    return "\n".join(pad + l if l.strip() else "" for l in text.split("\n"))


def rel(from_page_path, to_path):
    return posixpath.relpath(to_path, posixpath.dirname(from_page_path))


class Renderer:
    def __init__(self, site, lang):
        self.site = site
        self.lang = lang
        self.t = L[lang]

    def doc(self, binding, lines, page, context=""):
        if self.lang == "zh" and any(l.strip() for l in lines):
            zh = self.site.zh.text(binding, lines, "%s/%s %s" % (page.binding, page.slug, context))
            if zh is not None:
                return self.zh_md(binding, zh, page)
        if binding == "rust":
            return rust_to_md(lines, lambda target: self.site.resolve_rust(target, page, self.lang))
        return plain_to_md(lines)

    def zh_md(self, binding, text, page):
        """A Chinese unit is Markdown already; on a Rust page its intra-doc links resolve as the
        English ones do."""
        if binding != "rust":
            return text
        out, fence = [], False
        for l in text.split("\n"):
            if l.lstrip().startswith("```"):
                fence = not fence
                out.append(l)
            elif fence:
                out.append(l)
            else:
                out.append(_rust_inline(l, lambda target: self.site.resolve_rust(target, page, self.lang)))
        return "\n".join(out)

    def label(self, page, text, context):
        """A short English label, such as an abi.h section title, with its Chinese beside it."""
        if self.lang == "zh":
            zh = self.site.zh.text("cpp", [text], "%s/%s %s" % (page.binding, page.slug, context))
            if zh is not None:
                return "%s（%s）" % (zh, self.code(text))
        return _escape_prose(text)

    def code(self, s):
        if "`" in s:
            return "`` %s ``" % s
        return "`%s`" % s

    def slot_link(self, page, slot):
        target = self.site.slot_target(slot)
        if not target:
            return self.code(slot)
        return "[%s](%s#%s)" % (self.code(slot), rel(page.path(self.lang), target.path(self.lang)), slot)

    def entry_link(self, page, e):
        return "[%s](%s#%s)" % (self.code(e.qual), rel(page.path(self.lang), e.page.path(self.lang)), e.anchor)

    def entry(self, page, e):
        t, sep = self.t, self.t["sep"]
        out = ["### %s {#%s}" % (self.code(e.qual), e.anchor), ""]
        if e.deprecated:
            since, note = e.deprecated
            title = t["deprecated_since"] % since if since else t["deprecated"]
            body = self.doc(e.binding, [note], page, e.qual + " (deprecated)") if note else title
            out += ['!!! warning "%s"' % title, "", _indent(body, 4), ""]
        out += ["```%s" % FENCE[e.binding], e.sig, "```", ""]
        if e.note:
            out += ['!!! note "%s"' % t["group_note"], "", _indent(self.doc("cpp", e.note, page, e.qual + " (group note)"), 4), ""]
        body = self.doc(e.binding, e.doc, page, e.qual)
        if body:
            out += [body, ""]
        bullets = []
        if e.call:
            bullets.append("- %s%s%s" % (t["call"], sep, self.code(e.call)))
        if e.params:
            bullets.append("- %s%s" % (t["params"], sep.rstrip()))
            for n, ty in e.params:
                bullets.append("    - %s : %s" % (n, self.code(ty)))
        if e.ret:
            bullets.append("- %s%s%s" % (t["rtype"], sep, self.code(e.ret)))
        if e.slots and e.binding != "cpp":
            bullets.append("- %s%s%s" % (t["slots"], sep, t["list_sep"].join(self.slot_link(page, s) for s in e.slots)))
        if e.section:
            bullets.append("- %s%s%s" % (t["section"], sep, self.label(page, e.section, "section title")))
        if e.order is not None:
            bullets.append("- %s%s%s" % (t["order"], sep, t["order_fmt"] % e.order))
        if e.binding == "cpp" and e.kind == "slot":
            users = self.site.users_of(e.name)
            if users:
                bullets.append("- %s%s" % (t["users"], sep.rstrip()))
                for b in ("rust", "go", "zig"):
                    lst = users.get(b, [])
                    if not lst:
                        continue
                    shown = t["list_sep"].join(self.entry_link(page, u) for u in lst[:6])
                    if len(lst) > 6:
                        shown += " " + t["more"] % len(lst)
                    bullets.append("    - %s%s%s" % (BINDING_NAME[b], sep, shown))
        out += bullets
        out.append("")
        return "\n".join(out)

    def group(self, page, g):
        t = self.t
        title = g.title_zh if (self.lang == "zh" and g.title_zh) else g.title
        out = ["## %s {#%s}" % (title, g.anchor), ""]
        if g.decl:
            out += ["```%s" % FENCE[page.binding], g.decl, "```", ""]
        body = self.doc(page.binding, g.doc, page, "group " + g.anchor)
        if body:
            out += [body, ""]
        if g.traits:
            out += ["- %s%s%s" % (t["traits"], t["sep"], t["list_sep"].join(self.code(x) for x in g.traits)), ""]
        if g.consts:
            out += ["| %s | %s | %s |" % (t["name"], t["value"], t["desc"]), "|---|---|---|"]
            for name, value, doc, unsupported in g.consts:
                d = one_line(self.doc(page.binding, doc, page, name)) if doc else ""
                if unsupported:
                    d = ("**%s**" % t["unsupported"]) + ("：" if self.lang == "zh" else ": ") + d
                out.append('| <span id="%s"></span>%s | %s | %s |' % (name, self.code(name), self.code(value), d))
            out.append("")
        for e in g.entries:
            out.append(self.entry(page, e))
        return "\n".join(out)

    def page(self, page):
        t = self.t
        body = []
        intro = self.doc(page.binding, page.intro, page, "intro")
        if intro:
            body += [intro, ""]
        if page.section_notes:
            body += ['??? note "%s"' % t["section_notes"], ""]
            for title, lines in page.section_notes:
                body += [_indent("**%s**" % self.label(page, title, "section title"), 4), ""]
                text = self.doc("cpp", lines, page, "section " + title)
                if text:
                    body += [_indent(text, 4), ""]
        for g in page.groups:
            body.append(self.group(page, g))
        out = ["# %s" % page.title[self.lang], ""] + body
        text = "\n".join(out)
        return re.sub(r"\n{3,}", "\n\n", text).rstrip("\n") + "\n"
