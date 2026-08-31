# -*- coding: utf-8 -*-
"""rust-comment-budget —— COMMENTS.md §一 的硬预算，套到 Rust 上。

`comment_style.py` 只扫 `.cpp/.h/.hpp`，于是绑定那一侧的注释从来没有被量过。
量了一次之后是 18 处超预算，其中最长的文件头 43 行 —— 而 §一 说得很清楚：
超预算不是「写得详细」，是**放错了层级**。属于设计的进 CONTRACT.md，属于
历史的进 git，属于待办的进 issue。

Rust 的三个层级对应：

    L1  文件开头连续的 `//!`          ≤ 16 行
    L2  声明上方连续的 `///`          ≤ 14 行
    L3  语句上方连续的 `//`           ≤ 8 行

**这条检查量的是长度，不是内容。** 一段 15 行的废话它放过，一段 17 行的干货
它拦下。它能做的只是逼人回答「这些字该不该在这里」，回答本身还是人的事。
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

ROOTS = [os.path.join(ROOT, "bindings", "rust")]
BUDGET = {"L1 文件头": 16, "L2 声明注释": 14, "L3 体内注释": 8}


def _scan(path):
    """返回 [(层级, 起始行, 行数)]。"""
    lines = open(path, encoding="utf-8").read().split("\n")
    out = []

    i = 0
    while i < len(lines):
        s = lines[i].strip()
        nxt = lines[i + 1].strip() if i + 1 < len(lines) else ""
        if s.startswith("//!") or (s == "" and nxt.startswith("//!") and i > 0):
            i += 1
        else:
            break
    if i > BUDGET["L1 文件头"]:
        out.append(("L1 文件头", 1, i))

    for level, prefix in (("L2 声明注释", "///"), ("L3 体内注释", "//")):
        run = 0
        for j, l in enumerate(lines):
            s = l.strip()
            hit = (
                s.startswith("///")
                if prefix == "///"
                else (s.startswith("//") and not s.startswith(("///", "//!")))
            )
            if hit:
                run += 1
            else:
                if run > BUDGET[level]:
                    out.append((level, j - run + 1, run))
                run = 0
        if run > BUDGET[level]:
            out.append((level, len(lines) - run + 1, run))
    return out


def run():
    r = Result("rust-comment-budget")
    files = 0
    bad = []
    for root in ROOTS:
        if not os.path.isdir(root):
            continue
        for dp, _, fs in os.walk(root):
            if "target" in dp.split(os.sep):
                continue
            for fn in sorted(fs):
                if not fn.endswith(".rs"):
                    continue
                p = os.path.join(dp, fn)
                files += 1
                for level, line, n in _scan(p):
                    bad.append((os.path.relpath(p, ROOT), line, level, n))

    for rel, line, level, n in sorted(bad, key=lambda x: -x[3]):
        r.fail(
            "%s:%d %s %d 行，超 %d 行预算 —— 超预算不是写得详细，是放错了层级"
            "（设计进 CONTRACT.md，历史进 git，待办进 issue）"
            % (rel, line, level, n, BUDGET[level])
        )

    if not r.failures:
        r.note(
            "扫描 %d 个 .rs。判据只量长度，不看内容 —— 15 行的废话它放过，"
            "17 行的干货它拦下。它逼人回答「这些字该不该在这里」，回答还是人的事。"
            % files
        )
    return r


if __name__ == "__main__":
    sys.exit(run().report())
