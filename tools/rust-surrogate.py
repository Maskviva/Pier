#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""rust-surrogate —— 没有 cargo 时对 `cargo check` 的**代偿**。

和 `link-surrogate.py` 同一个定位，理由也一样：这个仓库有一段时间是在
**没有 rustc 的环境**里写出来的。在那里，一个漏掉的逗号可以一路混到交付。

**它不是契约 §九 的检查**，有工具链之后就是冗余的。它能查的东西也少得可怜：

  * 括号 / 花括号 / 方括号配平（剥掉注释和字符串之后）
  * `pub const` / `pub type` 语句以分号收尾
  * `#[repr(C)]` 结构体的字段以逗号分隔
  * `mod` 声明指向真实存在的文件
  * **悬空的 `///`**（后面没有任何项）—— 手抄 FFI 镜像时最容易出的一种，
    因为 C 头文件里跨行的**尾注**属于上一项，原地搬过来就悬空了
  * **C 类型名残留**（`int` / `short` / `unsigned` / `long` / `size_t` …）——
    同样是手抄镜像的产物；`cargo check` 会报，但要等到有 rustc 的机器上

查不到的：类型是否存在、生命周期、trait 约束、借用检查 —— 也就是 Rust
真正会帮你挡住的绝大部分东西。**PASS 只意味着「没有显然的形状错误」。**

用法：
    python3 tools/rust-surrogate.py [crate 目录]
"""

import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))


def strip(text):
    """剥注释与字符串字面量，保留换行以便报行号。"""

    def keep_nl(m):
        return "\n" * m.group(0).count("\n")

    text = re.sub(r"/\*.*?\*/", keep_nl, text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    text = re.sub(r'r#*"(?:.|\n)*?"#*', '""', text)
    text = re.sub(r'"(?:[^"\\\n]|\\.)*"', '""', text)
    text = re.sub(r"'(?:[^'\\]|\\.)'", "' '", text)
    return text


def check_file(path, rel, problems):
    raw = open(path, encoding="utf-8", errors="replace").read()
    code = strip(raw)

    for o, c in (("(", ")"), ("{", "}"), ("[", "]")):
        if code.count(o) != code.count(c):
            problems.append("%s：%s%s 不配平（%d vs %d）" % (rel, o, c, code.count(o), code.count(c)))

    # `pub const X: T = <expr>;` —— 表达式可以跨行，所以按分号切而不是按行。
    #
    # `const fn` / `const unsafe fn` 是**函数**，不是常量项，不适用这条。
    # 第一版没排除它，于是 `pub const fn mod_flags()` 被报成「缺分号」——
    # 一条会误报的检查最终会被人加进忽略列表，所以这里宁可漏也不能瞎报。
    for m in re.finditer(r"\bpub\s+(const|static|type)\b(?!\s+(unsafe\s+)?(fn|extern)\b)", code):
        rest = code[m.start():]
        semi = rest.find(";")
        brace = rest.find("{")
        if semi < 0:
            line = code[: m.start()].count("\n") + 1
            problems.append("%s:%d %s 语句没有分号收尾" % (rel, line, m.group(1)))
            continue
        # 结构体字面量里的 `{}` 合法，只要分号在它之后
        if 0 <= brace < semi:
            depth = 0
            i = brace
            while i < len(rest):
                if rest[i] == "{":
                    depth += 1
                elif rest[i] == "}":
                    depth -= 1
                    if depth == 0:
                        break
                i += 1
            if rest[i + 1 : i + 3].strip()[:1] != ";":
                line = code[: m.start()].count("\n") + 1
                problems.append("%s:%d %s 的初始化块之后缺分号" % (rel, line, m.group(1)))

    # #[repr(C)] 结构体的字段以逗号分隔
    for m in re.finditer(r"#\[repr\(C\)\][^\n]*\n(?:#\[[^\n]*\]\n)*\s*pub struct (\w+)\s*\{", raw):
        name = m.group(1)
        body_start = raw.index("{", m.start()) + 1
        depth = 1
        i = body_start
        while i < len(raw) and depth:
            if raw[i] == "{":
                depth += 1
            elif raw[i] == "}":
                depth -= 1
            i += 1
        body = strip(raw[body_start : i - 1])
        for j, line in enumerate(body.splitlines(), 1):
            t = line.strip()
            if not t or t.startswith("//"):
                continue
            if t.endswith(",") or t.endswith("{") or t.endswith("}"):
                continue
            problems.append("%s：结构体 %s 的字段行没有以逗号结尾 -> %r" % (rel, name, t[:60]))

    # 悬空的 `///`：后面找不到任何项。
    #
    # 这一条是被真机编译逼出来的。C 头里 `FOO = 26,` 后面跟一段跨行块注释，
    # 那段注释属于 **FOO**；手抄时原地搬过来就变成了下一项的前置注释，
    # 而最后一项后面没有下一项 —— 于是 `expected item after doc comment`。
    #
    # 更值得记的是：那个编译错误只是**运气**。前面每一条跨行尾注都同样挂错
    # 了对象，只是它们后面碰巧有项，编译器一声不吭。
    # 必须在**原文**里找 —— `code` 是剥掉注释之后的文本，在那里面找注释
    # 永远找不到。第一版就是这么写的，于是这条检查恒绿：一条永远不响的
    # 检查比没有检查更糟，因为它会让人以为这件事有人管。
    lines = raw.splitlines()
    for i, line in enumerate(lines):
        if not line.strip().startswith("///"):
            continue
        j = i + 1
        while j < len(lines) and (not lines[j].strip() or lines[j].strip().startswith("//")):
            j += 1
        if j >= len(lines):
            problems.append("%s:%d 悬空的 `///`（后面没有任何项）：%s"
                            % (rel, i + 1, line.strip()[:60]))
            break

    # C 类型名残留。手抄 FFI 镜像的典型产物 —— 它们不是 Rust 类型，
    # rustc 会报 `cannot find type`，但那要等到有 rustc 的机器上。
    CTOK = ("int", "short", "long", "unsigned", "signed", "size_t", "float", "double", "char")
    for k, line in enumerate(code.splitlines(), 1):
        if ":" not in line:
            continue
        after = line.split(":", 1)[1]
        for tok in CTOK:
            if not re.search(r"\b%s\b" % tok, after):
                continue
            # `c_char` / `c_void` 等 Rust 侧的正确拼写不算；
            # `usize` / `isize` 里也不含这些词（有词边界），不会误伤。
            if re.search(r"\bc_%s\b" % tok, after):
                continue
            problems.append("%s:%d 类型位置出现 C 类型名 %r（不是 Rust 类型）：%s"
                            % (rel, k, tok, line.strip()[:70]))
            break

    # mod 声明指向真实文件
    d = os.path.dirname(path)
    stem = os.path.splitext(os.path.basename(path))[0]
    subdir = d if stem in ("lib", "main", "mod") else os.path.join(d, stem)
    for m in re.finditer(r"^\s*(?:pub\s+)?mod\s+(\w+)\s*;", code, re.M):
        name = m.group(1)
        cands = [
            os.path.join(subdir, name + ".rs"),
            os.path.join(subdir, name, "mod.rs"),
        ]
        if not any(os.path.exists(c) for c in cands):
            line = code[: m.start()].count("\n") + 1
            problems.append("%s:%d `mod %s;` 找不到对应文件（找过 %s）"
                            % (rel, line, name, " / ".join(os.path.relpath(c, ROOT) for c in cands)))


def main():
    target = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "packages")
    problems = []
    n = 0
    for dp, dirs, fs in os.walk(target):
        dirs[:] = [d for d in dirs if d not in ("target", ".git")]
        for fn in sorted(fs):
            if not fn.endswith(".rs"):
                continue
            p = os.path.join(dp, fn)
            n += 1
            check_file(p, os.path.relpath(p, ROOT), problems)

    for pb in problems:
        print("  ✗ %s" % pb)
    if problems:
        print()
        print("  %d 处形状问题。真判据仍是 `cargo check`。" % len(problems))
        return 1
    print("  %d 个 .rs 文件无显然的形状错误。" % n)
    print("  这**不**代表能编过 —— 类型、生命周期、借用检查都不在覆盖内，")
    print("  真判据是 `cargo check`。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
