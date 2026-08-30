#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""include-surrogate —— 「用了本仓的某个符号，却没 include 定义它的头」。

## 定位：代偿，不是规矩

和 `link-surrogate.py` / `rust-surrogate.py` 同族。编译器一定会报
（`error C2065: "ModHostName": 未声明的标识符`），所以按契约 §九 的判据
它不该进那张表。它存在只因为这个仓库有一段时间是在**没有 MSVC** 的环境里
写出来的。

## 它逮的是什么

`pier-host/src/Entry.cpp` 用了 `ModHostName`，那个常量定义在
`pier/host/hosted_mod.h`，而 Entry.cpp 只 include 了 `mod_host.h`。
`mod_host.h` 恰好不传递包含它 —— 于是真机第一次编译直接红。

这类缺口在有传递包含的代码库里特别阴：同一个符号在别的 TU 里能用，
是因为那个 TU 碰巧 include 了别的东西。改一次 include 顺序就炸。

## 判据（保守，只报高置信）

只看**本仓自己定义**的符号（从 `packages/*/include/**` 里抽出来的
`namespace pier` 下的 `inline constexpr` / `class` / `struct` / 自由函数），
在每个 .cpp 里检查：用到了它，但那个 TU 的 include 闭包（含头文件的
`#include` 一层展开）里没有定义它的头。

会漏（宏、模板、隐式实例化），也可能误报（同名局部变量）。所以措辞是
「可能缺 include」，真判据永远是一次真正的编译。
"""

import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
PKGS = os.path.join(ROOT, "packages")


def strip_comments(text):
    """只剥注释，**保留字符串**。"""

    def keep_nl(m):
        return "\n" * m.group(0).count("\n")

    text = re.sub(r"/\*.*?\*/", keep_nl, text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def strip(text):
    """剥注释**和**字符串。找符号用这个。

    注意：找 `#include "..."` 时**不能**用它 —— 路径本身就是字符串，
    剥掉之后一条 include 都找不到，检查会恒绿。
    这个坑在 rust-surrogate 里也踩过一次（在剥掉注释的文本里找注释）。
    结论是一样的：**先想清楚要找的东西会不会被自己剥掉。**
    """
    return re.sub(r'"(?:[^"\\\n]|\\.)*"', '""', strip_comments(text))


def collect_headers():
    """本仓每个头定义了哪些「有名字的东西」。"""
    defined = {}  # 符号 -> include 路径
    for dp, _, fs in os.walk(PKGS):
        for fn in fs:
            if not fn.endswith((".h", ".hpp")):
                continue
            p = os.path.join(dp, fn)
            # include 路径 = 相对该包 include/ 的那一段
            m = re.search(r"packages[/\\][^/\\]+[/\\]include[/\\](.+)$", p.replace("\\", "/"))
            if not m:
                continue
            incpath = m.group(1)
            text = strip(open(p, encoding="utf-8", errors="replace").read())
            for pat in (
                r"\binline\s+constexpr\s+[\w:<>,\s]*?\b(\w+)\s*=",
                r"\bconstexpr\s+[\w:<>,\s]*?\b(\w+)\s*=",
                r"\b(?:class|struct|enum\s+class|enum)\s+(\w+)\s*[:{]",
                r"^\s*(?:\[\[nodiscard\]\]\s*)?[\w:<>&*,\s]+?\b(\w+)\s*\([^;{]*\)\s*(?:const\s*)?;",
            ):
                for mm in re.finditer(pat, text, re.M):
                    name = mm.group(1)
                    if len(name) > 2 and name not in ("if", "for", "while", "return"):
                        defined.setdefault(name, incpath)
    return defined


def includes_of(path, cache, depth=0):
    """一个文件的 include 闭包（本仓内部的头，展开两层就够）。"""
    if path in cache:
        return cache[path]
    out = set()
    cache[path] = out
    if depth > 3 or not os.path.exists(path):
        return out
    text = strip_comments(open(path, encoding="utf-8", errors="replace").read())
    incdirs = [
        os.path.join(PKGS, d, "include")
        for d in os.listdir(PKGS)
        if os.path.isdir(os.path.join(PKGS, d, "include"))
    ]
    for m in re.finditer(r'#\s*include\s*[<"]((?:pier|sdk)/[^>"]+)[>"]', text):
        inc = m.group(1)
        out.add(inc)
        for d in incdirs:
            cand = os.path.join(d, inc)
            if os.path.exists(cand):
                out |= includes_of(cand, cache, depth + 1)
                break
    cache[path] = out
    return out


def main():
    defined = collect_headers()
    cache = {}
    problems = []
    n = 0
    for dp, _, fs in os.walk(PKGS):
        for fn in sorted(fs):
            if not fn.endswith(".cpp"):
                continue
            p = os.path.join(dp, fn)
            rel = os.path.relpath(p, ROOT)
            n += 1
            text = strip(open(p, encoding="utf-8", errors="replace").read())
            have = includes_of(p, cache)
            used = set(re.findall(r"\b([A-Z]\w{3,})\b", text))
            for name in sorted(used):
                where = defined.get(name)
                if where is None or where in have:
                    continue
                # 同文件里自己定义的不算
                if re.search(r"\b(class|struct|enum)\s+%s\b" % name, text):
                    continue
                problems.append("%s 用了 %s，但 include 闭包里没有 %r" % (rel, name, where))

    for pb in problems:
        print("  ? %s" % pb)
    if problems:
        print()
        print("  %d 处可能缺 include。逐条人工确认 —— 宏、模板、同名局部变量都可能是误判。"
              % len(problems))
        return 1
    print("  %d 个 .cpp 的内部符号都能在 include 闭包里找到定义。" % n)
    print("  这**不**代表能编过 —— 真判据是一次真正的编译。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
