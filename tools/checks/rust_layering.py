# -*- coding: utf-8 -*-
"""rust-layering —— 绑定内部的模块依赖图（契约 §一 的同一条规矩，下沉一层）。

## 为什么需要它

`pkg_layering.py` 守的是 C++ 那八个包之间的边。Rust 侧只有两个 crate，所以
那条检查在这一侧几乎什么都没查 —— 而真正会腐烂的地方在 `pier-rs` **内部**:
二十几个域模块，谁能用谁全靠自觉。

一次讨论里提过「要不要把每个域拆成独立 crate」。结论是不拆:crate 是编译与
发版的单位，module 才是模块化的单位，拆了只多出二十几份 Cargo.toml 和一份
版本错配的新失败模式，换不来任何现在没有的隔离。但那个讨论想要的**纪律**
是对的 —— 于是把纪律放在这里，用检查而不是用目录结构去守。

副作用是退路也留住了:边一直是干净的，将来真要拆某个域，module → crate
就是机械操作。

## 判据

1. `ALLOWED` 是那张图的机器可读副本。用了没声明的边 → 红。
2. 声明了却没用的边 → 红。陈旧的许可比缺失的许可更危险:它让下一个人以为
   这条边是有意为之。
3. 任何环 → 红。环意味着两个模块以后只能一起动，也意味着讲不清谁建在谁上。

**只看真代码**，文档注释里的 `[`crate::x`]` 交叉引用不算边 —— 那正是文档
该做的事，把它算成依赖会逼着人删掉有用的链接。
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

SRC = os.path.join(ROOT, "bindings", "rust", "pier-rs", "src")

# 那张图。**这里是规矩，不是现状的描述** —— 图变了要先改这里。
#
#   rt      运行时地基:握手、两道闸、字符串收口、日志。谁都能用它。
#   types   共享值类型。零依赖，故意的:它是域之间传值的公共货币，
#           一旦它开始依赖某个域，那个域就成了所有人的依赖。
#   nbt     SNBT 树与解析。只用 rt（二进制互转要走宿主的解析器）。
ALLOWED = {
    "rt": set(),
    "types": set(),
    "nbt": {"rt"},
    # ── 叶子域:只认识地基 ──────────────────────────────────
    "service": {"rt"},
    "packet": {"rt"},
    "kvdb": {"rt"},
    "money": {"rt"},
    "client": {"rt"},
    "lane": {"rt"},
    "bus": {"rt"},
    "scoreboard": {"rt", "nbt"},
    "dimensions": {"rt", "nbt"},
    "command": {"rt", "nbt"},
    "server": {"rt", "nbt"},
    "item": {"rt", "nbt"},
    # ── 选择器:身份纪律的落点。自定义维度的命令名要问 dimensions ──
    "sel": {"rt", "dimensions"},
    # ── 组合域 ────────────────────────────────────────────
    "event": {"rt", "nbt", "sel"},
    "container": {"rt", "sel", "item"},
    "entity": {"rt", "nbt", "types", "item"},
    "player": {"rt", "nbt", "types", "sel", "item", "container", "entity"},
    "gui": {"rt", "nbt", "player"},
    "sim": {"rt", "nbt", "sel", "player"},
    "block": {"rt", "nbt", "types", "item", "container"},
    # host 在 world 下面，不在上面:`Host::world()` 那种便利访问器会造环，
    # 而 world 要用 `execute_command` 拼 /fill 是真依赖。见 host.rs 末尾。
    "host": {"rt", "nbt", "types", "packet"},
    "world": {"rt", "nbt", "types", "sel", "block", "entity", "host"},
}


def _modules():
    out = {}
    for name in os.listdir(SRC):
        p = os.path.join(SRC, name)
        if name.endswith(".rs") and name != "lib.rs":
            out[name[:-3]] = [p]
        elif os.path.isdir(p):
            files = []
            for dp, _, fs in os.walk(p):
                files += [os.path.join(dp, f) for f in fs if f.endswith(".rs")]
            if files:
                out[name] = sorted(files)
    return out


def _reexport_owner():
    """`crate::Player` 这样的名字属于哪个模块 —— lib.rs 的 re-export 表。"""
    lib = open(os.path.join(SRC, "lib.rs"), encoding="utf-8").read()
    owner = {}
    for m in re.finditer(r"^pub use (\w+)::\{?([^;]+)\};", lib, re.M):
        for n in re.split(r"[,{}\s]+", m.group(2)):
            n = n.strip()
            if n and n[0].isupper():
                owner[n] = m.group(1)
    return owner


def _strip_comments(text):
    return "\n".join(
        l for l in text.split("\n") if not l.lstrip().startswith(("///", "//!", "//"))
    )


def run():
    r = Result("rust-layering")
    if not os.path.isdir(SRC):
        r.fail("bindings/rust/pier-rs/src 不存在")
        return r

    mods = _modules()
    owner = _reexport_owner()

    unknown = sorted(set(mods) - set(ALLOWED))
    if unknown:
        r.fail(
            "这些模块不在那张图里:%s —— 新加一个域就要先在 ALLOWED 里给它一行，"
            "说清楚它建在谁上面" % "、".join(unknown)
        )
    stale = sorted(set(ALLOWED) - set(mods))
    if stale:
        r.fail("图里有已经不存在的模块:%s" % "、".join(stale))

    actual = {}
    for name, files in mods.items():
        deps = set()
        for f in files:
            code = _strip_comments(open(f, encoding="utf-8").read())
            for other in mods:
                if other != name and re.search(r"crate::" + other + r"\b", code):
                    deps.add(other)
            for sym, om in owner.items():
                if om != name and re.search(r"crate::" + sym + r"\b", code):
                    deps.add(om)
        actual[name] = deps

    for name in sorted(set(mods) & set(ALLOWED)):
        extra = actual[name] - ALLOWED[name]
        if extra:
            r.fail(
                "`%s` 用了没声明的边:%s。要么这条依赖不该有，要么图变了 —— "
                "两种都得先改 ALLOWED 再说" % (name, "、".join(sorted(extra)))
            )
        dead = ALLOWED[name] - actual[name]
        if dead:
            r.fail(
                "`%s` 声明了却没用的边:%s。陈旧的许可比缺失的更危险，"
                "它让下一个人以为这条边是有意为之" % (name, "、".join(sorted(dead)))
            )

    cycles = []
    for a in sorted(actual):
        for b in sorted(actual[a]):
            if a in actual.get(b, set()) and (b, a) not in cycles:
                cycles.append((a, b))
    for a, b in cycles:
        r.fail(
            "`%s` 与 `%s` 互相依赖。环意味着这两个模块以后只能一起动，"
            "也意味着讲不清谁建在谁上面 —— 砍掉可以不要的那一边" % (a, b)
        )

    if not r.failures:
        r.note(
            "%d 个模块，%d 条边，无环。判据只覆盖 `crate::` 路径引用；"
            "通过 `use` 引进来再裸用的名字由 lib.rs 的 re-export 表解析，"
            "表外的名字看不见。" % (len(mods), sum(len(v) for v in actual.values()))
        )
    return r


if __name__ == "__main__":
    sys.exit(run().report())
