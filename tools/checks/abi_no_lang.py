# -*- coding: utf-8 -*-
"""abi-no-lang —— 契约面不许偏袒任何一门消费方语言（契约 §一 规则五 + §七）。

这条检查分三半，判据**故意不对称**，理由写在下面。

## 半一：`pier-abi/` 里不许出现任何消费方语言的拼写（连注释也不行）

消费方是「任何语言」。一旦逐槽注释里写着 ``#[repr(C)] 函数表``、
``通常是 Arc::into_raw``、``&'static 或泄漏出来的 Box``，读这份契约的
Go / Zig / C# 作者拿到的就不是规格，而是**另一门语言的方言**：他得先把
Rust 的所有权模型在脑子里翻一遍，才知道自己那一侧要保证什么。
机制本身是语言中立的（「C 布局的函数表」「活到车道撤销为止」），
描述它的措辞也必须是。v1 的原始事故就是这个形状 ——
「Rust 没有稳定 ABI」重写成「多数原生语言没有稳定 ABI」之后，
那段话才第一次对所有人成立。

## 半二：C++ 拼写只在**注释**里放行

不对称的理由：C++ 是**宿主的实现语言**，不是契约的消费方之一。
注释说「宿主这里曾经攒一个 std::vector<std::string> 再跨 DLL 析构，崩了，
所以改成零暂存流水线」——那是在交代**这个槽为什么长这样**，对任何语言的
绑定作者都是有效信息。它和「你应该用 Arc::into_raw」的区别是：前者描述
宿主的历史，后者给消费方下指令、而且用了某一门消费方的方言。

声明部分不受这条放行 —— `abi-c-parse` 剥掉注释后扫的就是声明。
两条检查合起来才是完整的规则五。

## 半三：用户可见字符串里的历史产品名（§七）

只扫**字符串字面量**，不扫注释和文档。理由同上：记录「旧仓叫
levilamina-rust-loader、注释还写错了路径」是史料；而
`log("update levilamina-rust-loader")` 是真的会印到用户眼前。
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

# 消费方语言的名字与专属拼写。带词边界避免 "trust" / "zigzag" 之类误报。
CONSUMER_LANG = re.compile(
    r"(\bRust\b|\brust\b|\bGolang\b|\bZig\b|\bKotlin\b|\bSwift\b|C#|"
    r"#\[|\brepr\(|&'static|\bArc::|\bBox<|\bunsafe\b|\bimpl\b|"
    r"\bOption<|\bResult<|\bVec<)"
)

# C++ 拼写：只允许出现在注释里（见半二）。
CXX_SPELL = ("std::", "string_view", "enum class", "template<", "template <")

# 历史产品名。只在字符串字面量里算残留。
LEGACY = (
    "levilamina-rust-loader",
    "levilamina-rs",
    "levi_rs",
    "LEVI_RS",
    "/llr",
    "/levirs",
)

STR_LIT = re.compile(r'"((?:[^"\\]|\\.)*)"')
CODE_EXT = (".h", ".hpp", ".cpp", ".rs")


def split_comments(text):
    """返回 (注释行列表, 代码行列表)，与原文行号一一对应。"""
    out_code, out_cmt = [], []
    in_block = False
    for line in text.splitlines():
        code, cmt = [], []
        i = 0
        while i < len(line):
            if in_block:
                j = line.find("*/", i)
                if j < 0:
                    cmt.append(line[i:])
                    break
                cmt.append(line[i:j])
                in_block = False
                i = j + 2
            elif line.startswith("/*", i):
                in_block = True
                i += 2
            elif line.startswith("//", i):
                cmt.append(line[i:])
                break
            else:
                code.append(line[i])
                i += 1
        out_code.append("".join(code))
        out_cmt.append("".join(cmt))
    return out_cmt, out_code


def run():
    r = Result("abi-no-lang")

    # ── 半一 + 半二：pier-abi/ ───────────────────────────────────────
    abi_dir = os.path.join(ROOT, "packages", "pier-abi")
    files = 0
    for dirpath, _, names in os.walk(abi_dir):
        for fn in names:
            p = os.path.join(dirpath, fn)
            rel = os.path.relpath(p, ROOT)
            files += 1
            with open(p, encoding="utf-8", errors="replace") as f:
                text = f.read()
            _, code_lines = split_comments(text)
            for i, line in enumerate(text.splitlines(), 1):
                m = CONSUMER_LANG.search(line)
                if m:
                    r.fail("%s:%d 消费方语言拼写 %r（注释也不行）：%s"
                           % (rel, i, m.group(0), line.strip()[:90]))
            for i, code in enumerate(code_lines, 1):
                for s in CXX_SPELL:
                    if s in code:
                        r.fail("%s:%d 声明里出现 C++ 拼写 %r：%s"
                               % (rel, i, s, code.strip()[:90]))
    r.note("pier-abi/ 扫描 %d 个文件（注释禁消费方语言，声明禁 C++ 类型）" % files)

    # ── 半三：全仓库字符串字面量里的历史产品名 ──────────────────────
    skip_dirs = {".git", "target", "node_modules", "tools"}
    hits = 0
    scanned = 0
    for dirpath, dirs, names in os.walk(ROOT):
        dirs[:] = [d for d in dirs if d not in skip_dirs]
        for fn in names:
            if not fn.endswith(CODE_EXT):
                continue
            p = os.path.join(dirpath, fn)
            rel = os.path.relpath(p, ROOT)
            scanned += 1
            with open(p, encoding="utf-8", errors="replace") as f:
                text = f.read()
            _, code_lines = split_comments(text)
            for i, code in enumerate(code_lines, 1):
                for lit in STR_LIT.findall(code):
                    for bad in LEGACY:
                        if bad in lit:
                            r.fail("%s:%d 用户可见字符串里有历史产品名 %r：%r"
                                   % (rel, i, bad, lit[:70]))
                            hits += 1
    if not hits:
        r.note("%d 个源文件的字符串字面量里零历史产品名" % scanned)
    return r


if __name__ == "__main__":
    sys.exit(run().report())
