#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""include-surrogate —— 「用了本仓的某个符号，却没 include 定义它的头」。

## 定位：代偿，不是规矩

和 `link-surrogate.py` / `rust-surrogate.py` 同族。编译器一定会报
（`error C2065: "ModHostName": 未声明的标识符`），所以按契约 §九 的判据
它不该进那张表。它存在只因为这个仓库有一段时间是在**没有 MSVC** 的环境里
写出来的。

## 它逮两类

### 一、本仓的符号没 include 定义它的头

`pier-host/src/Entry.cpp` 用了 `ModHostName`，那个常量定义在
`pier/host/hosted_mod.h`，而 Entry.cpp 只 include 了 `mod_host.h`。
`mod_host.h` 恰好不传递包含它 —— 于是真机第一次编译直接红。

这类缺口在有传递包含的代码库里特别阴：同一个符号在别的 TU 里能用，
是因为那个 TU 碰巧 include 了别的东西。改一次 include 顺序就炸。

### 二、`std::X` 没 include 对应的标准头

同一个形状，另一侧。`spi.h` 用 `std::string` 却只 include 了 `<string_view>`
—— MSVC 报 `error C2039: "string": 不是 "std" 的成员`。

第一次真机编译只报了这一处，实查全仓有 **17 处**。其余 16 处此刻能编过，
纯粹是因为别的头碰巧把标准头带进来了 —— 那不是「没问题」，那是「问题被
另一个文件的 include 列表挡着」。删掉那个文件的一行 include，或者换一版
标准库，它们会一起炸。

## 判据（保守，只报高置信）

只看**本仓自己定义**的符号（从 `packages/*/include/**` 里抽出来的
`namespace pier` 下的 `inline constexpr` / `class` / `struct` / 自由函数），
在每个 .cpp 里检查：用到了它，但那个 TU 的 include 闭包（含头文件的
`#include` 一层展开）里没有定义它的头。

**大小写和长度都不设限。** 第一版只扫「大写开头、4 字符以上」的标识符，
于是 `sv` / `ps` / `toString` 这一族自由函数从来没进过视野 ——
`core/Log.cpp` 用了 `sv(msg)` 却没 include `pier/support/str.h`，
真机报 `C3861: "sv": 找不到标识符`，而这条检查一路绿。

短名字的代价是误报（`sv` 也可能是某个局部变量名），所以只在**函数调用
形状**（`name(`）上判定，且措辞仍是「可能」。

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


# `std::X` → 它需要的标准头。只列**用错了一定会报**的，不求全。
STD_NEEDS = {
    "string": "string", "string_view": "string_view", "vector": "vector",
    "map": "map", "unordered_map": "unordered_map", "set": "set",
    "unordered_set": "unordered_set", "optional": "optional",
    "function": "functional", "unique_ptr": "memory", "shared_ptr": "memory",
    "weak_ptr": "memory", "make_unique": "memory", "make_shared": "memory",
    "mutex": "mutex", "lock_guard": "mutex", "shared_mutex": "shared_mutex",
    "atomic": "atomic", "thread": "thread", "array": "array",
    "variant": "variant", "tuple": "tuple", "pair": "utility",
    "move": "utility", "forward": "utility", "deque": "deque",
    "runtime_error": "stdexcept", "memcpy": "cstring",
}


def std_includes_of(path, cache, depth=0):
    """一个文件的**标准头** include 闭包（含经内部头传递进来的）。"""
    key = ("std", path)
    if key in cache:
        return cache[key]
    out = set()
    cache[key] = out
    if depth > 4 or not os.path.exists(path):
        return out
    text = strip_comments(open(path, encoding="utf-8", errors="replace").read())
    for m in re.finditer(r"#\s*include\s*<([^>]+)>", text):
        out.add(m.group(1))
    incdirs = [
        os.path.join(PKGS, d, "include")
        for d in os.listdir(PKGS)
        if os.path.isdir(os.path.join(PKGS, d, "include"))
    ]
    for m in re.finditer(r'#\s*include\s*[<"]((?:pier|sdk)/[^>"]+)[>"]', text):
        for d in incdirs:
            cand = os.path.join(d, m.group(1))
            if os.path.exists(cand):
                out |= std_includes_of(cand, cache, depth + 1)
                break
    cache[key] = out
    return out


def check_std_headers(path, rel, cache, problems):
    text = strip(open(path, encoding="utf-8", errors="replace").read())
    have = std_includes_of(path, cache)
    for name in sorted({m.group(1) for m in re.finditer(r"\bstd::(\w+)", text)}):
        need = STD_NEEDS.get(name)
        if need and need not in have:
            problems.append("%s 用了 std::%s，但 include 闭包里没有 <%s> —— "
                            "现在能编过只是别的头碰巧带进来了" % (rel, name, need))


# C++ 关键字与常见的标准库名。它们绝不可能是「本仓定义的符号」。
NOT_SYMBOLS = {
    "if", "for", "while", "return", "switch", "case", "void", "bool", "int",
    "char", "auto", "const", "static", "inline", "class", "struct", "enum",
    "namespace", "template", "typename", "using", "sizeof", "static_assert",
    "operator", "explicit", "friend", "public", "private", "protected",
    "string", "to_string", "size", "data", "begin", "end", "get", "set",
    "value", "empty", "clear", "find", "insert", "erase", "at", "push_back",
    "getInstance", "catch", "try", "throw", "new", "delete", "this",
}


def _namespace_scope_spans(text):
    """返回「处于 namespace 作用域、且不在任何 class/函数体里」的字符区间。

    为什么必须做这个：第一版不分作用域，于是 `class X { static X& getInstance(); }`
    里的 `getInstance` 被当成了一个自由函数，然后每一个调用 `foo.getInstance()`
    的 TU 都被要求 include 那个类的头 —— 22 条全是这种误报。
    一条只会误报的检查，第一次就会被人加进忽略列表。
    """
    spans = []
    stack = []          # 每层是 "ns" / "class" / "block"
    i = 0
    n = len(text)
    seg_start = 0
    while i < n:
        ch = text[i]
        if ch == "{":
            head = text[max(0, i - 200):i]
            if re.search(r"\bnamespace\b[\w:\s]*$", head):
                kind = "ns"
            elif re.search(r"\b(class|struct|union|enum)\b[^;{]*$", head):
                kind = "class"
            else:
                kind = "block"
            if kind != "ns" and all(k == "ns" for k in stack):
                spans.append((seg_start, i))     # 进入非 ns 作用域前的一段算数
            stack.append(kind)
            if kind == "ns":
                seg_start = i + 1
            i += 1
            continue
        if ch == "}":
            if stack:
                kind = stack.pop()
                if kind != "ns" and all(k == "ns" for k in stack):
                    seg_start = i + 1
            i += 1
            continue
        i += 1
    if all(k == "ns" for k in stack):
        spans.append((seg_start, n))
    return spans


def collect_headers():
    """本仓每个头在**命名空间作用域**定义了哪些名字。

    同一个名字出现在两个以上的头里就**丢弃**：这个检查的产出是
    「你应该 include 哪个头」，而那个问题在名字有歧义时没有答案。
    宁可漏，不能瞎指。
    """
    defined = {}
    ambiguous = set()
    for dp, _, fs in os.walk(PKGS):
        for fn in fs:
            if not fn.endswith((".h", ".hpp")):
                continue
            p = os.path.join(dp, fn)
            m = re.search(r"packages[/\\][^/\\]+[/\\]include[/\\](.+)$", p.replace("\\", "/"))
            if not m:
                continue
            incpath = m.group(1)
            text = strip(open(p, encoding="utf-8", errors="replace").read())
            for a, b in _namespace_scope_spans(text):
                seg = text[a:b]
                for pat in (
                    r"\b(?:inline\s+)?constexpr\s+[\w:<>,\s]*?\b(\w+)\s*=",
                    # **前向声明不算定义**：`struct X;` 只是承诺 X 存在，
                    # include 它所在的头解决不了任何问题。第一版把 `[:{;]`
                    # 里的 `;` 也算上了，于是 `struct DimensionFactoryInfo;`
                    # 这种前向声明让真正的定义变成了「有歧义」，整个名字被
                    # 排除出检查 —— 一次漏报，而且是静默的。
                    r"\b(?:class|struct|enum\s+class|enum)\s+(\w+)\s*[:{]",
                    r"\busing\s+(\w+)\s*=",
                    # 函数：声明和头文件里的内联定义都要。
                    #
                    # **不要求以 `;` 或 `{` 收尾。** 内联定义的 `{` 是函数体的
                    # 开始，而上面的作用域切分正是在 `{` 处断开片段的 ——
                    # 于是要求收尾符等于要求一个必然不在片段里的字符，
                    # `inline std::string_view sv(PierStr)` 这一族整个漏掉。
                    # 真机报的 `C3861: "sv" 找不到标识符` 就是它。
                    #
                    # 这是本工程第五次踩同族的坑：判据看的东西（收尾符）
                    # 和它想断言的东西（这里有个函数）不是同一个东西。
                    r"^[ \t]*(?:\[\[nodiscard\]\]\s*)?(?:inline\s+|static\s+|constexpr\s+)*"
                    r"[\w:<>&*,\s]+?\b(\w+)\s*\([^;{)]*\)\s*"
                    r"(?:const\s*)?(?:noexcept\s*)?[;{]?\s*$",
                ):
                    for mm in re.finditer(pat, seg, re.M):
                        name = mm.group(1)
                        if name in NOT_SYMBOLS or len(name) < 2:
                            continue
                        if name in defined and defined[name] != incpath:
                            ambiguous.add(name)
                        defined.setdefault(name, incpath)
    for name in ambiguous:
        defined.pop(name, None)
    return defined, ambiguous


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
    defined, ambiguous = collect_headers()
    cache = {}
    problems = []
    n = 0
    # 标准头这一类**头文件也要查** —— spi.h 那次就是头文件自己缺。
    for dp, _, fs in os.walk(PKGS):
        for fn in sorted(fs):
            if fn.endswith((".cpp", ".h", ".hpp")):
                p = os.path.join(dp, fn)
                check_std_headers(p, os.path.relpath(p, ROOT), cache, problems)

    for dp, _, fs in os.walk(PKGS):
        for fn in sorted(fs):
            if not fn.endswith(".cpp"):
                continue
            p = os.path.join(dp, fn)
            rel = os.path.relpath(p, ROOT)
            n += 1
            text = strip(open(p, encoding="utf-8", errors="replace").read())
            have = includes_of(p, cache)
            # 大写开头的类型/常量：整词匹配就够。
            used = set(re.findall(r"\b([A-Z]\w{2,})\b", text))
            # 小写开头的自由函数：只认**调用形状**，避免把同名局部变量算进去。
            used |= set(re.findall(r"\b([a-z]\w*)\s*\(", text))
            for name in sorted(used):
                where = defined.get(name)
                if where is None or where in have:
                    continue
                # 同文件里自己定义的不算
                if re.search(r"\b(class|struct|enum)\s+%s\b" % name, text):
                    continue
                problems.append("%s 用了 %s，但 include 闭包里没有 %r" % (rel, name, where))

    if ambiguous:
        print("  · %d 个名字在多个头里都有定义，无从指出该 include 哪一个，已排除：%s"
              % (len(ambiguous), "、".join(sorted(ambiguous)[:10])
                 + (" …" if len(ambiguous) > 10 else "")))
    for pb in problems:
        print("  ? %s" % pb)
    if problems:
        print()
        print("  %d 处可能缺 include。逐条人工确认 —— 宏、模板、同名局部变量都可能是误判。"
              % len(problems))
        return 1
    print("  %d 个 .cpp 的内部符号、以及全部 TU 的 std:: 用法，都能在 include 闭包里找到来源。" % n)
    print("  这**不**代表能编过 —— 真判据是一次真正的编译。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
