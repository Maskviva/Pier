#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""example-surrogate —— 示例模组用到的符号，`prelude` 里必须都有。

## 定位：代偿，不是规矩

和另外三个 surrogate 同族。`cargo check` 一定会报
（`cannot find type X in this scope`），所以按契约 §九 的判据它不该进那张表。

## 它为什么值得单独写

示例是**契约 §十 四步的可运行证明**。一个编不过的示例，等于那个证明没有
成立过 —— 而这件事在没有 cargo 的机器上完全看不出来：示例的源码读起来
一切正常。

上一轮已经吃过一次同族的亏：v0 的示例 manifest 依赖
`levilamina-rust-loader`，改名之后那个模组不存在了，示例装不上，
且没有任何报错。那一次的教训做成了 `manifest-matches-host`（进了 §九，
因为宿主不会报错）；这一次的教训做成这个（不进 §九，因为编译器会报）。

## 判据

示例里出现的每一个**大写开头的标识符**，要么在 `prelude` 的再导出列表里，
要么带完整路径（`levilamina::Xxx`），要么是 Rust 自带的
（`Ok` / `Err` / `Some` / `None` / `Self` / `String` / `Vec` …）。

会误报：字符串和注释里的大写词。所以扫描前先剥掉它们；剥不干净的
（比如格式串里的类型名）由人判断，措辞是「可能」。
"""

import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

# Rust 自带的、不需要 prelude 导出的名字。
BUILTIN = {
    "Ok", "Err", "Some", "None", "Self", "String", "Vec", "Box", "Option", "Result",
    "Mutex", "OnceLock", "Arc", "Rc", "HashMap", "HashSet", "BTreeMap", "Duration",
    "Instant", "Default", "Clone", "Copy", "Debug", "Display", "From", "Into",
    "Iterator", "FnOnce", "FnMut", "Fn", "Send", "Sync",
}


def strip(text):
    """剥注释和字符串字面量，保留行号。

    字符串正则必须允许**跨行**：Rust 的 `"...\\` 反斜杠续行是合法的，
    而一个只匹配单行的正则会在续行处停住，把后半截字符串当成代码。
    第一版就是这么写的，于是格式串里的「ABI」被报成了未导出的类型名。

    这已经是这一轮第三次踩同族的坑（前两次：在剥掉注释的文本里找注释、
    在剥掉字符串的文本里找 `#include "..."`）。统一的教训是：
    **先想清楚要找的东西会不会被自己剥掉，以及自己剥得干不干净。**
    """

    def keep_nl(m):
        return "\n" * m.group(0).count("\n")

    text = re.sub(r"/\*.*?\*/", keep_nl, text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    text = re.sub(r'r#*"(?:.|\n)*?"#*', '""', text)
    # `[^"\\]|\\.` 里的 `\\.` 在 DOTALL 下才吃得掉反斜杠后面的换行。
    return re.sub(r'"(?:[^"\\]|\\.)*"', keep_nl, text, flags=re.S)


def prelude_exports(lib_rs):
    with open(lib_rs, encoding="utf-8") as f:
        text = f.read()
    m = re.search(r"pub mod prelude \{(.*?)\n\}", text, re.S)
    if not m:
        return None
    body = strip(m.group(1))
    return set(re.findall(r"\b([A-Z]\w*|[a-z_]\w*!?)\b", body))


def main():
    problems = []
    examples = os.path.join(ROOT, "examples")
    lib_rs = os.path.join(ROOT, "bindings", "rust", "pier-rs", "src", "lib.rs")
    if not os.path.exists(lib_rs) or not os.path.isdir(examples):
        print("  没有 pier-rs 或 examples/ —— 无从检查")
        return 0

    exported = prelude_exports(lib_rs)
    if exported is None:
        print("  ✗ pier-rs 的 lib.rs 里找不到 `pub mod prelude`")
        return 1

    n = 0
    for dp, dirs, fs in os.walk(examples):
        dirs[:] = [d for d in dirs if d not in ("target", ".git")]
        for fn in sorted(fs):
            if not fn.endswith(".rs"):
                continue
            p = os.path.join(dp, fn)
            rel = os.path.relpath(p, ROOT)
            n += 1
            with open(p, encoding="utf-8") as f:
                raw = f.read()
            code = strip(raw)
            uses_prelude = "prelude::*" in code
            # 本文件自己定义的类型不算
            local = set(re.findall(r"\b(?:struct|enum|trait|type)\s+(\w+)", code))
            for m in re.finditer(r"\b([A-Z]\w*)\b", code):
                name = m.group(1)
                if name in BUILTIN or name in local or name in exported:
                    continue
                # 带完整路径的放行
                start = max(0, m.start() - 40)
                if "::" in code[start : m.start()][-3:] or re.search(
                    r"\b\w+::%s\b" % re.escape(name), code
                ):
                    continue
                line = code[: m.start()].count("\n") + 1
                hint = "prelude 里没有它" if uses_prelude else "这个文件没有 use prelude::*"
                problems.append("%s:%d 可能用到未导出的 %r —— %s" % (rel, line, name, hint))

            # 宏也要能找到：`register_mod!` 走 #[macro_export]，
            # 但示例若写 `levilamina::register_mod!` 就必须真的导出到 crate 根。
            for m in re.finditer(r"levilamina::(\w+)!", code):
                mac = m.group(1)
                src = open(
                    os.path.join(ROOT, "bindings", "rust", "pier-rs", "src", "rt", "runtime.rs"),
                    encoding="utf-8",
                ).read() + open(
                    os.path.join(ROOT, "bindings", "rust", "pier-rs", "src", "rt", "registration.rs"),
                    encoding="utf-8",
                ).read()
                if ("macro_rules! %s" % mac) not in src:
                    problems.append("%s 用了 `levilamina::%s!`，但找不到它的 macro_rules 定义"
                                    % (rel, mac))

    for pb in sorted(set(problems)):
        print("  ? %s" % pb)
    if problems:
        print()
        print("  %d 处可疑。逐条确认 —— 格式串里的大写词可能是误判。" % len(set(problems)))
        return 1
    print("  %d 个示例源文件用到的符号，prelude 都覆盖得到。" % n)
    print("  这**不**代表能编过 —— 真判据是 `cargo check`。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
