# -*- coding: utf-8 -*-
"""Puts the four references together: the cross-binding index, the per-binding index pages,
the overview, the navigation of both mkdocs files, and writing or comparing the result."""

import os
import re

from .model import BINDING_NAME, BINDING_ORDER, DOCS, LANGS, Page, Renderer, rel
from . import cpp, golang, rust, zig
from .zh import Catalog

NAV_BEGIN = "# BEGIN gen-api-docs"
NAV_END = "# END gen-api-docs"
MKDOCS = {"en": os.path.join(DOCS, "mkdocs.yml"), "zh": os.path.join(DOCS, "mkdocs.zh.yml")}

RUST_NAV = {
    "root": ("crate root", "crate 根"), "player": ("player", "玩家"), "entity": ("entity", "实体"),
    "world": ("world", "世界"), "block": ("block", "方块"), "item": ("item", "物品"),
    "container": ("container", "容器"), "event": ("event", "事件"), "event-names": ("event::names", "事件名"),
    "command": ("command", "命令"), "gui": ("gui", "表单"), "scoreboard": ("scoreboard", "计分板"),
    "packet": ("packet", "数据包"), "server": ("server", "服务器"), "host": ("Host", "宿主与任务"),
    "bus": ("bus", "事件总线"), "service": ("service", "服务"), "lane": ("lane", "快速通道"),
    "kvdb": ("kvdb", "键值数据库"), "money": ("money", "经济"), "nbt": ("nbt", "NBT"),
    "sim": ("sim", "模拟玩家"), "dimensions": ("dimensions", "自定义维度"), "client": ("client", "客户端"),
    "registry": ("registry", "注册表"), "types": ("types", "通用类型"),
}
SHORT = {s: (en.split(":")[0], zh.split("：")[0]) for s, en, zh in cpp.TOPICS}
SHORT.update({"nbt": ("SNBT", "SNBT"), "raw": ("Raw", "Raw")})


class Site:
    def __init__(self):
        self.zh = Catalog()
        self.header, cpages = cpp.build()
        slots = {e.name for e in self.header.slots}
        self.slot_page = lambda s: cpp.place(cpp.SLOT_RULES, s, "slot")
        rpages, rentries, self.rust_types = rust.build(slots)
        gpages, gentries = golang.build(slots, self.slot_page)
        zpages, zentries = zig.build(slots, self.slot_page)
        self.pages = {"rust": rpages, "go": gpages, "zig": zpages, "cpp": cpages}
        self.entries = {"rust": rentries, "go": gentries, "zig": zentries,
                        "cpp": [e for p in cpages for g in p.groups for e in g.entries]}
        for b, pages in self.pages.items():
            for p in pages:
                for g in p.groups:
                    g.page = p
                    for e in g.entries:
                        e.page = p
        for b in ("rust", "go", "zig"):
            for p in self.pages[b]:
                if not (b == "go" and p.slug == "raw"):
                    p.groups = order_groups(p.groups)
        self._slot_pages = {e.name: e.page for e in self.header.slots}
        self._users = {}
        for b in ("rust", "go", "zig"):
            for e in self.entries[b]:
                for s in e.slots:
                    self._users.setdefault(s, {}).setdefault(b, []).append(e)
        self._rust_index()

    # Lookups the renderer makes

    def slot_target(self, slot):
        return self._slot_pages.get(slot)

    def users_of(self, slot):
        return self._users.get(slot, {})

    def _rust_index(self):
        self.rust_quals = {}
        for e in self.entries["rust"]:
            self.rust_quals.setdefault(e.qual.rstrip("!"), (e.page, e.anchor))
            if e.kind == "fn":
                self.rust_quals.setdefault(e.qual.split("::")[-1], (e.page, e.anchor))
        for p in self.pages["rust"]:
            for g in p.groups:
                if g.kind == "type":
                    self.rust_quals.setdefault(g.anchor, (p, g.anchor))
                for name, *_ in g.consts:
                    self.rust_quals.setdefault(name, (p, name))
        self.rust_mods = {p.slug.replace("-", "::").replace("event::names", "names"): p for p in self.pages["rust"]}
        self.rust_quals["sel"] = (next(p for p in self.pages["rust"] if p.slug == "player"), "PlayerSel")
        self.rust_mods["event::names"] = next(p for p in self.pages["rust"] if p.slug == "event-names")

    def resolve_rust(self, target, page, lang):
        t = target.strip().strip("`")
        t = re.sub(r"^(?:crate|self|super|levilamina)::", "", t)
        t = re.sub(r"\(\)$", "", t).rstrip("!")
        hit = self.rust_quals.get(t)
        segs = t.split("::")
        if not hit and len(segs) >= 2:
            hit = self.rust_quals.get("::".join(segs[-2:])) or self.rust_quals.get(segs[-1])
        if hit:
            p, anchor = hit
            return "%s#%s" % (rel(page.path(lang), p.path(lang)), anchor)
        mod = self.rust_mods.get(t)
        if mod:
            return rel(page.path(lang), mod.path(lang))
        return None

    # Pages

    def counts(self, b):
        fns = sum(1 for e in self.entries[b] if e.kind in ("fn", "method", "macro", "slot"))
        types = sum(1 for p in self.pages[b] for g in p.groups if g.kind == "type") + \
            sum(1 for e in self.entries[b] if e.kind == "type")
        consts = sum(len(g.consts) for p in self.pages[b] for g in p.groups) + \
            sum(1 for e in self.entries[b] if e.kind == "const")
        return fns, types, consts

    def cpp_split(self):
        return len(self.header.slots), sum(1 for e in self.entries["cpp"] if e.kind == "macro")

    def page_counts(self, p):
        fns = sum(1 for g in p.groups for e in g.entries if e.kind in ("fn", "method", "macro", "slot"))
        types = sum(1 for g in p.groups if g.kind == "type") + sum(1 for g in p.groups for e in g.entries if e.kind == "type")
        consts = sum(len(g.consts) for g in p.groups) + sum(1 for g in p.groups for e in g.entries if e.kind == "const")
        return fns, types, consts

    def nav_label(self, b, slug, lang):
        if b == "rust":
            return RUST_NAV[slug][0 if lang == "en" else 1]
        return SHORT[slug][0 if lang == "en" else 1]

    def binding_index(self, b, lang):
        zh = lang == "zh"
        nslots = len(self.header.slots)
        head = {
            "rust": (["# Rust 接口", "",
                      "`pier-rs` 包的 crate 名是 `levilamina`。每个模块一页；`Player`、`Entity`、`World` 这类类型的方法分在好几个源文件里，都合到它定义所在的那一页。",
                      "",
                      "`levilamina::sys` 是 `abi.h` 的逐格镜像（`pier-sys-rs` 包），其中的槽位、常量和结构体见 [C++ 接口](../cpp/index.md)。"],
                     ["# Rust API", "",
                      "The crate of the `pier-rs` package is named `levilamina`. Each module has a page; the methods of a type such as `Player`, `Entity` or `World` are spread over several source files and are gathered on the page of the type's definition.",
                      "",
                      "`levilamina::sys` mirrors `abi.h` cell for cell (the `pier-sys-rs` package); its slots, constants and structs are on the [C++ pages](../cpp/index.md)."]),
            "go": (["# Go 接口", "",
                    "模块 `github.com/Maskviva/pier/bindings/go`，包 `levilamina`。页面按主题分，文件名和 C++ 页一一对应：方法在它接收者类型所在的页，函数在它调用的第一个槽位所属的主题页。",
                    "",
                    "`levilamina.Raw` 给每个不带回调的槽位各提供一个带类型的方法，全部列在 [Raw](raw.md) 页。带回调的槽位由 `Subscribe`、`RegisterCommand`、`BusSubscribe` 这类手写函数提供。"],
                   ["# Go API", "",
                    "Module `github.com/Maskviva/pier/bindings/go`, package `levilamina`. The pages are by subject and share their file names with the C++ pages: a method is on the page of its receiver type, and a function on the subject page of the first slot it calls.",
                    "",
                    "`levilamina.Raw` has one typed method for every slot without a callback, all listed on the [Raw](raw.md) page. The slots with a callback are reached through hand-written functions such as `Subscribe`, `RegisterCommand` and `BusSubscribe`."]),
            "zig": (["# Zig 接口", "",
                     "包名 `.pier`，导出模块 `levilamina`。`levilamina.slot(\"名字\")` 能取到 abi.h 的全部 %d 个槽位" % nslots + "，每个槽位的参数和返回值见 [C++ 接口](../cpp/index.md)；下面各页列的是建立在 `slot` 之上的函数和方法。",
                     "",
                     "`props_gen.zig` 和 `nbt.zig` 分别以 `levilamina.props`、`levilamina.nbt` 公开；其中 `Player`、`Entity`、`BlockAt`、`Item` 和 `SelKind` 在根上也有同名的再导出。"],
                    ["# Zig API", "",
                     "Package `.pier`, exported module `levilamina`. `levilamina.slot(\"name\")` reaches all %d slots of abi.h" % nslots + ", whose parameters and results are on the [C++ pages](../cpp/index.md); the pages below list the functions and methods built on `slot`.",
                     "",
                     "`props_gen.zig` and `nbt.zig` are public as `levilamina.props` and `levilamina.nbt`; `Player`, `Entity`, `BlockAt`, `Item` and `SelKind` are re-exported at the root under the same names."]),
            "cpp": (["# C++ 接口", "",
                     "用 Pier SDK 编译的 C++ 模组直接包含 `sdk/abi.h`，在 `pier_main` 里拿到函数表 `PierApi`，之后每次调用都写成 `api->槽位名(...)`。表里的 %d 个槽位" % nslots + "按主题分在下面各页，每个槽位注明了它在 abi.h 里所在的分节和在表里的序号，还列出了 Rust、Go、Zig 里调用它的接口。",
                     "",
                     "调用一个槽位前要做两项检查：`struct_size` 够不够得着这个槽位，槽位是不是 NULL。写法和完整的例子见 [C++ 绑定](../../cpp/index.md)。",
                     "",
                     "`bindings/bridge/pier-bridge.h` 是给 LeviLamina 原生插件调用 Pier 服务用的，不在这一部分，见 [bridge](../../guide/bridge.md)。"],
                    ["# C++ API", "",
                     "A C++ mod compiled with the Pier SDK includes `sdk/abi.h` directly, receives the function table `PierApi` in `pier_main`, and calls `api->slot_name(...)` from then on. The %d slots" % nslots + " of the table are spread over the pages below by subject. Each slot names its section in abi.h and its position in the table, and lists the interfaces of Rust, Go and Zig that call it.",
                     "",
                     "Before a slot is called, two things are checked: that `struct_size` reaches the slot, and that the slot is not NULL. The [C++ binding](../../cpp/index.md) page shows how, with a complete mod.",
                     "",
                     "`bindings/bridge/pier-bridge.h`, for LeviLamina native plugins calling Pier services, is not part of this reference; see [bridge](../../guide/bridge.md)."]),
        }[b][0 if zh else 1]
        out = list(head) + [""]
        cols = ("页面", "函数和方法", "类型", "常量") if zh else ("Page", "Functions and methods", "Types", "Constants")
        if b == "cpp":
            cols = ("页面", "槽位和宏", "类型", "常量") if zh else ("Page", "Slots and macros", "Types", "Constants")
        out += ["| %s | %s | %s | %s |" % cols, "|---|---|---|---|"]
        for p in self.pages[b]:
            f, t, c = self.page_counts(p)
            out.append("| [%s](%s.md) | %d | %d | %d |" % (self.nav_label(b, p.slug, lang), p.slug, f, t, c))
        if b == "cpp":
            out += ["", "## abi.h 文件头" if zh else "## The header of abi.h", ""]
            out += ["文件头写着整份 ABI 的约定，原文如下：" if zh else "The file header states the conventions of the whole ABI:", ""]
            for n, block in enumerate([self.header.file_header] + self.header.notes):
                block = list(block)
                while block and not block[0].strip():
                    block.pop(0)
                while block and not block[-1].strip():
                    block.pop()
                if zh:
                    text = self.zh.text("cpp", block, "cpp/index abi.h header, block %d" % n)
                    if text is not None:
                        out += [text, ""]
                        continue
                out += ["```text"] + block + ["```", ""]
        return "\n".join(out).rstrip() + "\n"

    def overview(self, lang):
        zh = lang == "zh"
        if zh:
            out = ["# 接口参考", "",
                   "Pier 的接口按四个绑定分别列出。",
                   "", "| 绑定 | 引用方式 | 函数和方法 | 类型 | 常量 |", "|---|---|---|---|---|"]
            how = {"rust": "`use levilamina::...`", "go": "`import \"github.com/Maskviva/pier/bindings/go/levilamina\"`",
                   "zig": "`@import(\"levilamina\")`", "cpp": "`#include \"sdk/abi.h\"`"}
        else:
            out = ["# API reference", "",
                   "Pier's API is listed binding by binding.",
                   "", "| Binding | Brought in with | Functions and methods | Types | Constants |", "|---|---|---|---|---|"]
            how = {"rust": "`use levilamina::...`", "go": "`import \"github.com/Maskviva/pier/bindings/go/levilamina\"`",
                   "zig": "`@import(\"levilamina\")`", "cpp": "`#include \"sdk/abi.h\"`"}
        for b in BINDING_ORDER:
            f, t, c = self.counts(b)
            out.append("| [%s](%s/index.md) | %s | %d | %d | %d |" % (BINDING_NAME[b], b, how[b], f, t, c))
        if zh:
            out += ["", "C++ 一行的函数数是 abi.h 的 %d 个槽位加上 %d 个宏。" % self.cpp_split(), "",
                    "## 一个条目怎么读", "",
                    "- **标题**：接口的名字。方法写成 `类型::方法`（Rust）或 `类型.方法`（Go、Zig）。",
                    "- **声明**：源码里的签名，原样照抄。",
                    "- **说明**：源码里写在它上面的文档注释。",
                    "- **参数**、**返回值类型**：从签名里拆出来。",
                    "- **对应槽位**：这个接口最终调用的 abi.h 槽位，链接到 C++ 页上那个槽位的条目，那里写着宿主怎么处理这次调用。",
                    "- **已弃用**：标着弃用的接口在标题下面有一个提示框，写着从哪个版本起弃用、改用什么。", "",
                    "「对应槽位」列的是这个接口在源码里追得到的 abi.h 槽位：函数体里直接写到的，加上经同一类型的其他方法、带类型名的调用和同一文件里的函数间接调到的，最多四层。经未标注类型的变量调用的槽位列不出来；列出的每个槽位在源码里都有调用路径。",
                    "## 按场景找", "",
                    "想先看一件事怎么做，比如订阅事件、注册命令、读写玩家属性，请看教程里的[常见任务](../tasks/index.md)，每个场景都有三种语言的示例。"]
        else:
            out += ["", "The C++ count is the %d slots of abi.h plus %d macros." % self.cpp_split(), "",
                    "## Reading an entry", "",
                    "- **Heading**: the interface's name. A method is written `Type::method` (Rust) or `Type.method` (Go, Zig).",
                    "- **Declaration**: the signature as the source writes it.",
                    "- **Description**: the doc comment above it in the source.",
                    "- **Parameters** and **return type**: taken apart from the signature.",
                    "- **Slots**: the slots of abi.h the interface ends up calling, each linking to the slot's entry on the C++ pages, which says what the host does with the call.",
                    "- **Deprecated**: a deprecated interface has a box under its heading saying since which version and what to use instead.", "",
                    "The Slots column lists the abi.h slots this interface reaches in the source: the ones the body names, and the ones reached through another method of the same type, a call qualified by a type name or a function of the same file, up to four calls deep. A method called on a variable of unknown type is not listed, so the slots behind it are missing from the column. Each slot that is listed has a call path in the source.",
                    "## By task", "",
                    "To see how something is done first, such as subscribing to an event, registering a command or reading a player property, see [Common tasks](../tasks/index.md) under Tutorials, where each task has an example in three languages."]
        return "\n".join(out).rstrip() + "\n"

    def files(self):
        """Every generated file: path relative to docs/ -> text."""
        out = {}
        for lang in LANGS:
            r = Renderer(self, lang)
            out["%s/api/index.md" % lang] = self.overview(lang)
            for b in BINDING_ORDER:
                out["%s/api/%s/index.md" % (lang, b)] = self.binding_index(b, lang)
                for p in self.pages[b]:
                    out[p.path(lang)] = r.page(p)
        return out

    def nav(self, lang):
        zh = lang == "zh"
        lines = ["      %s" % NAV_BEGIN,
                 "      - api/index.md"]
        for b in BINDING_ORDER:
            lines.append('      - "%s":' % BINDING_NAME[b])
            lines.append("          - api/%s/index.md" % b)
            for p in self.pages[b]:
                lines.append('          - "%s": api/%s/%s.md' % (self.nav_label(b, p.slug, lang).replace('"', '\\"'), b, p.slug))
        lines.append("      %s" % NAV_END)
        return "\n".join(lines)

    def mkdocs_texts(self):
        out = {}
        for lang, path in MKDOCS.items():
            with open(path, encoding="utf-8") as f:
                text = f.read()
            a, b = text.find(NAV_BEGIN), text.find(NAV_END)
            if a < 0 or b < 0:
                raise SystemExit("gen-api-docs: %s has no %r ... %r lines to write the navigation "
                                 "between" % (os.path.relpath(path, DOCS), NAV_BEGIN, NAV_END))
            start = text.rfind("\n", 0, a) + 1
            end = text.find("\n", b)
            out[path] = text[:start] + self.nav(lang) + text[end:]
        return out


def order_groups(groups):
    """Free functions first, then the types with the most methods, then constants and
    macros; within a rank the source order holds."""
    def rank(ig):
        i, g = ig
        if g.kind == "module":
            return (0, -1, i)
        if g.kind == "functions" and g.anchor != "macros":
            return (0, 0, i)
        if g.kind == "type":
            return (1, -len(g.entries), i)
        if g.kind == "consts":
            return (2, 0, i)
        return (3, 0, i)
    return [g for _, g in sorted(enumerate(groups), key=rank)]


def generated_dirs():
    return [os.path.join(DOCS, lang, "api") for lang in LANGS]
