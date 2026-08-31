#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""typed-storage —— `ll::TypedStorage` 成员上的 `.get()` 用得对不对。

## 规则（这份文件是它唯一的正式出处）

`ll::TypedStorage<Align, Size, T>` 不是一个统一的包装器，它按 `T` 分特化：

| `T` 是什么 | 成员是什么 | `.get()` |
|---|---|---|
| 类类型的**值**（`std::string`、`BlockPos`、`std::vector<…>`） | 包装器 | **要写** |
| **标量 / 枚举**（`int`、`bool`、`DimensionType`、`ActorDamageCause`） | 就是那个值本身 | 写了是编译错误 |
| **引用**（`Dimension&`、`Player&`） | 就是那个引用本身 | 写了是编译错误 |
| `std::unique_ptr<T>` | 就是那个 unique_ptr | 要写，但那是 `unique_ptr::get`，语义不同 |

用错的两种症状真机都见过：
* 标量：`C2228: ".get" 的左边必须有类/结构/联合`
* 引用：`C2039: "get" 不是 "Dimension" 的成员`

这条规则此前散在四个文件的注释里，措辞还各不相同（有的说「标量坍缩」，
有的说「标量和引用都坍缩」，有的只提 unique_ptr）。**规则有四个出处就等于
没有出处** —— 谁也不知道哪一份是最新的。现在正式出处是这里，那几处注释
指过来。

## 这条检查需要引擎头文件

判定「这个成员的 T 是什么」只能读 `mc/**/*.h`。那些头在 LeviLamina 的 xmake
包目录里，**这台机器上可能没有**。找不到时报 **SKIP 而不是 PASS** ——
没有头文件不等于代码没问题（契约 §九：PASS 只能给覆盖到的那部分打 ✓）。

指定位置：
    set PIER_LL_INCLUDE=C:\\Users\\<你>\\AppData\\Local\\.xmake\\packages\\l\\levilamina\\...\\include
或者让脚本自己在常见路径下找。

## 定位：surrogate，不是契约检查

编译器一定会报，所以按 §九 的判据它不进那张表。它的价值是**一次报完**：
编译器一次只报第一个失败的 TU，而这个脚本把全仓 30 个 `.get()` 调用点
一起验了。
"""

import glob
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
PKGS = os.path.join(ROOT, "packages")

# 标量与已知的基本类型。这些进 TypedStorage 会坍缩。
SCALARS = {
    "bool", "char", "signed char", "unsigned char", "short", "unsigned short",
    "int", "unsigned int", "uint", "long", "unsigned long", "long long",
    "unsigned long long", "float", "double", "size_t", "ptrdiff_t",
    "int8", "int16", "int32", "int64", "uint8", "uint16", "uint32", "uint64",
    "uchar", "ushort", "ulong", "uint64_t", "int64_t", "uint32_t", "int32_t",
    "uint16_t", "int16_t", "uint8_t", "int8_t", "std::byte",
}


def strip(text):
    def keep_nl(m):
        return "\n" * m.group(0).count("\n")

    text = re.sub(r"/\*.*?\*/", keep_nl, text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    return re.sub(r'"(?:[^"\\]|\\.)*"', keep_nl, text, flags=re.S)


def find_engine_include():
    env = os.environ.get("PIER_LL_INCLUDE")
    if env and os.path.isdir(env):
        return env
    home = os.path.expanduser("~")
    pats = [
        os.path.join(home, "AppData", "Local", ".xmake", "packages", "l", "levilamina",
                     "*", "*", "include"),
        os.path.join(home, ".xmake", "packages", "l", "levilamina", "*", "*", "include"),
        os.path.join(ROOT, ".xmake", "packages", "l", "levilamina", "*", "*", "include"),
    ]
    for pat in pats:
        hits = sorted(glob.glob(pat))
        if hits:
            return hits[-1]
    return None


def collect_engine_members(inc_dir):
    """成员名 -> (T 的写法, 声明它的头)。同名成员出现在多个类里就丢弃。"""
    members = {}
    ambiguous = set()
    enums = set()
    n_files = 0
    decl = re.compile(
        r"::ll::TypedStorage<\s*[^,]+,\s*[^,]+,\s*(.+?)\s*>\s+(m\w+)\s*;"
    )
    decl2 = re.compile(r"\bll::TypedStorage<\s*[^,]+,\s*[^,]+,\s*(.+?)\s*>\s+(m\w+)\s*;")
    enum_decl = re.compile(r"\benum\s+(?:class\s+|struct\s+)?(\w+)\s*(?::[^{;]+)?[{;]")

    for dp, _, fs in os.walk(inc_dir):
        for fn in fs:
            if not fn.endswith((".h", ".hpp")):
                continue
            p = os.path.join(dp, fn)
            n_files += 1
            try:
                text = strip(open(p, encoding="utf-8", errors="replace").read())
            except OSError:
                continue
            for m in enum_decl.finditer(text):
                enums.add(m.group(1))
            for rx in (decl, decl2):
                for m in rx.finditer(text):
                    t, name = " ".join(m.group(1).split()), m.group(2)
                    if name in members and members[name][0] != t:
                        ambiguous.add(name)
                    members.setdefault(name, (t, os.path.relpath(p, inc_dir)))
    for name in ambiguous:
        members.pop(name, None)
    return members, enums, ambiguous, n_files


def collapses(t, enums):
    """这个 T 进 TypedStorage 之后会不会坍缩（= `.get()` 是编译错误）。"""
    t = t.strip()
    if t.endswith("&") or t.endswith("&&"):
        return True, "引用"
    base = t.replace("::", " ").split()[-1] if t else ""
    if t in SCALARS or base in SCALARS:
        return True, "标量"
    if base in enums:
        return True, "枚举"
    return False, ""


def main():
    inc = find_engine_include()
    if inc is None:
        print("  SKIP —— 找不到 LeviLamina 的 include 目录，无从判定成员类型。")
        print("        用 PIER_LL_INCLUDE 指过去再跑。")
        print("        **这不是 PASS**：没有头文件不等于代码没问题。")
        return 0

    members, enums, ambiguous, n_files = collect_engine_members(inc)
    print("  引擎头 %d 个，TypedStorage 成员 %d 个，枚举 %d 个（歧义已排除 %d 个）"
          % (n_files, len(members), len(enums), len(ambiguous)))

    problems = []
    scanned = 0
    for dp, _, fs in os.walk(PKGS):
        for fn in sorted(fs):
            if not fn.endswith((".cpp", ".h", ".hpp")):
                continue
            p = os.path.join(dp, fn)
            rel = os.path.relpath(p, ROOT)
            scanned += 1
            code = strip(open(p, encoding="utf-8", errors="replace").read())
            for m in re.finditer(r"\b(m[A-Z]\w*)\s*\.\s*get\s*\(\s*\)", code):
                name = m.group(1)
                info = members.get(name)
                if not info:
                    continue
                t, where = info
                bad, why = collapses(t, enums)
                if bad:
                    line = code[: m.start()].count("\n") + 1
                    problems.append(
                        "%s:%d `%s.get()` —— %s 装的是 %s（%s），TypedStorage 对它有特化，"
                        "成员本身就是那个值，`.get()` 是编译错误。声明在 %s"
                        % (rel, line, name, name, t, why, where)
                    )
            # 反向：类类型的值**漏了** .get()，同样是编译错误，只是症状不同。
            for m in re.finditer(r"\b(m[A-Z]\w*)\s*\.\s*(?!get\b)(\w+)\s*\(", code):
                name = m.group(1)
                info = members.get(name)
                if not info:
                    continue
                t, where = info
                bad, _ = collapses(t, enums)
                if not bad and not t.startswith("std::unique_ptr"):
                    line = code[: m.start()].count("\n") + 1
                    problems.append(
                        "%s:%d `%s.%s(...)` —— %s 装的是类类型 %s，TypedStorage **保持包装**，"
                        "要先 `.get()` 才拿得到它。声明在 %s"
                        % (rel, line, name, m.group(2), name, t, where)
                    )

    for pb in sorted(set(problems)):
        print("  ✗ %s" % pb)
    if problems:
        print()
        print("  %d 处。规则见本文件头。" % len(set(problems)))
        return 1
    print("  %d 个源文件里的 TypedStorage 成员访问全部符合坍缩规则。" % scanned)
    return 0


if __name__ == "__main__":
    sys.exit(main())
