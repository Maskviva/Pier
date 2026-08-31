# -*- coding: utf-8 -*-
"""host-loadable —— 宿主本体满足 LeviLamina 装载期的硬性要求。

盯的是什么：这一类要求**编译器和链接器都不检查**，只有真的把 mod 放进
`mods/` 启服务器时才显形，而报出来的话和构建过程毫无关系。

真事：`MemoryOperators.cpp` 从头到尾没写。98 个 TU 全编过、prelink 过、
`pier.dll` 链好、mod 打包完成，装的时候 LeviLamina 说：

    ERROR [LeviLamina] 无法加载 Pier
    ERROR [LeviLamina] Pier 将不会被加载因为没有使用统一的内存分配操作符。

它在台账里一直挂着 ⬜ —— **数据是对的，交付说明是错的**：连续三轮写
「C++ 侧全量完成」。所以这条检查和 `ledger-count` 的分区汇总是配对的：
一个查「东西在不在」，另一个防「总结和台账对不上」。

## 三条判据（对着 LeviLamina 的模板逐条来）

1. **统一内存算子** —— 恰好一个 TU 定义 `LL_MEMORY_OPERATORS` 并 include
   `ll/api/memory/MemoryOperators.h`。多于一个是重复定义全局 `operator new`。
2. **模组注册** —— 恰好一处 `LL_REGISTER_MOD(...)`。
3. **那个 TU 必须真的进产物** —— 它零外部符号引用，静态库会整个丢掉它
   （契约 §一 规则四）。所以它所在的包必须是 `set_kind("object")`，
   而且要在根 xmake 的必编列表里。

第 3 条是这条检查里最值钱的一半：前两条「文件在不在」肉眼也看得出来，
而「它到底有没有被链进去」看不出来 —— 而症状和文件缺失**完全一样**。
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

PKGS = os.path.join(ROOT, "packages")


def _read(p):
    with open(p, encoding="utf-8", errors="replace") as f:
        return f.read()


def _strip(text):
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def _find(pattern):
    """返回 [(包名, 相对路径)]，只看**代码**（剥注释）。"""
    hits = []
    for dp, _, fs in os.walk(PKGS):
        for fn in sorted(fs):
            if not fn.endswith((".cpp", ".h", ".hpp")):
                continue
            p = os.path.join(dp, fn)
            if re.search(pattern, _strip(_read(p)), re.M):
                rel = os.path.relpath(p, ROOT).replace(os.sep, "/")
                hits.append((rel.split("/")[1], rel))
    return hits


def run():
    r = Result("host-loadable")
    root_xmake = _read(os.path.join(ROOT, "xmake.lua"))

    # ── 1. 统一内存算子 ────────────────────────────────────────────
    mem = _find(r"^\s*#\s*define\s+LL_MEMORY_OPERATORS\b")
    if not mem:
        r.fail("全仓没有任何 TU 定义 `LL_MEMORY_OPERATORS` —— LeviLamina 会拒绝装载，"
               "报「没有使用统一的内存分配操作符」。这条错误在**构建全部成功之后**"
               "才出现，和编译期的任何检查都无关。")
    elif len(mem) > 1:
        r.fail("有 %d 个 TU 定义了 `LL_MEMORY_OPERATORS`：%s —— 全局 operator new "
               "会重复定义" % (len(mem), "、".join(x[1] for x in mem)))
    else:
        pkg, rel = mem[0]
        inc = re.search(r'#\s*include\s*"ll/api/memory/MemoryOperators\.h"',
                        _read(os.path.join(ROOT, rel)))
        if not inc:
            r.fail("%s 定义了 LL_MEMORY_OPERATORS 但没 include "
                   "`ll/api/memory/MemoryOperators.h` —— 那个宏只是开关，"
                   "算子的定义在那个头里" % rel)
        else:
            r.note("统一内存算子：%s" % rel)
        _require_linked_in(r, pkg, rel, root_xmake, "统一内存算子")

    # ── 2. 模组注册 ────────────────────────────────────────────────
    reg = _find(r"\bLL_REGISTER_MOD\s*\(")
    if not reg:
        r.fail("全仓没有 `LL_REGISTER_MOD(...)` —— LeviLamina 找不到模组入口")
    elif len(reg) > 1:
        r.fail("有 %d 处 `LL_REGISTER_MOD`：%s —— 一个 mod 只能注册一次"
               % (len(reg), "、".join(x[1] for x in reg)))
    else:
        pkg, rel = reg[0]
        r.note("模组注册：%s" % rel)
        _require_linked_in(r, pkg, rel, root_xmake, "模组注册")

    return r


def _require_linked_in(r, pkg, rel, root_xmake, what):
    """这个 TU 必须真的进最终产物。

    它零外部符号引用 —— 没有任何人调用它里面的东西。静态库会把这种 obj
    整个丢掉，而丢掉之后的症状和「文件根本不存在」**完全一样**。
    """
    xm = os.path.join(PKGS, pkg, "xmake.lua")
    if not os.path.exists(xm):
        r.fail("%s 所在的包 %s 没有 xmake.lua" % (rel, pkg))
        return
    text = _read(xm)
    m = re.search(r'set_kind\("([^"]+)"\)', text)
    kind = m.group(1) if m else "(未声明)"
    if kind != "object":
        r.fail("%s（%s）在 %s 包里，而它是 set_kind(%r) —— 这个 TU 零外部符号"
               "引用，静态库会把它整个丢掉，症状和文件不存在一模一样"
               % (rel, what, pkg, kind))
    if 'includes("packages/%s")' % pkg not in root_xmake:
        r.fail("根 xmake 没有 includes(\"packages/%s\") —— %s 不会被编译" % (pkg, what))
    # 必编：不能被包在任何 if 里
    for line in root_xmake.splitlines():
        if 'includes("packages/%s")' % pkg in line and line.startswith(" "):
            r.fail("根 xmake 里 includes(\"packages/%s\") 是**有条件**的 —— "
                   "%s 在某些配置下会缺席，而那时 LeviLamina 会拒绝装载" % (pkg, what))


if __name__ == "__main__":
    sys.exit(run().report())
