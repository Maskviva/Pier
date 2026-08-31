#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""build-prereqs —— 构建**开始前**就该满足的条件。

## 为什么单独一个脚本

`levibuildscript` 的 modpacker 在打包阶段读 git 拿版本号。工作目录不是 git
仓库、或者是空仓库（没有任何提交）时，它报：

    error: fatal: Not a valid object name HEAD

这条错误出现在 **90%**，也就是全部 98 个 TU 编完、prelink 跑完、
`pier.dll` 都链好之后。一次十分钟的构建，被一个**一秒就能查出来**的前置
条件拦在最后一步。

这就是它单独成脚本的全部理由：**前置条件要在最前面查。** 放进
`run-checks.py` 也行，但那套是「代码对不对」；这一套是「环境行不行」，
两者失败时该做的事完全不同 —— 前者改代码，后者敲一行命令。

## 定位：surrogate

xmake 自己最终会报（虽然报得晚、报得含糊）。按契约 §九 的判据它不进那张表。

用法：
    python3 tools/build-prereqs.py
"""

import os
import subprocess
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))


def git(*args):
    try:
        p = subprocess.run(
            ["git", "-C", ROOT] + list(args),
            capture_output=True, text=True, timeout=15,
        )
        return p.returncode, p.stdout.strip(), p.stderr.strip()
    except (OSError, subprocess.SubprocessError) as e:  # noqa: BLE001
        return 127, "", str(e)


def main():
    problems = []
    notes = []

    rc, _, _ = git("--version")
    if rc == 127:
        problems.append("找不到 git。levibuildscript 的 modpacker 要用它取版本号。")
        _report(problems, notes)
        return 1

    rc, out, _ = git("rev-parse", "--is-inside-work-tree")
    if rc != 0 or out != "true":
        problems.append(
            "%s 不是一个 git 工作区。modpacker 会在打包阶段（约 90%%）报\n"
            "      `fatal: Not a valid object name HEAD`，而那时 98 个 TU 已经全部编完。\n"
            "      解决：\n"
            "        git init\n"
            "        git add -A\n"
            "        git commit -m \"pier v1\"\n"
            "        git tag v1.0.0        # 可选，但版本号会好看很多" % ROOT
        )
        _report(problems, notes)
        return 1

    rc, out, err = git("rev-parse", "HEAD")
    if rc != 0:
        problems.append(
            "git 仓库存在但**没有任何提交** —— `git rev-parse HEAD` 报 %r。\n"
            "      这正是真机那条 `fatal: Not a valid object name HEAD`。\n"
            "      解决：\n"
            "        git add -A\n"
            "        git commit -m \"pier v1\"\n"
            "        git tag v1.0.0        # 可选" % (err.splitlines()[0] if err else "失败")
        )
        _report(problems, notes)
        return 1
    notes.append("HEAD = %s" % out[:12])

    rc, out, _ = git("describe", "--tags", "--always", "--dirty")
    if rc == 0:
        notes.append("git describe = %s" % out)
    rc, out, _ = git("tag", "--list")
    if rc == 0 and not out:
        notes.append("没有任何 tag —— 能打包，但版本号会退化成提交哈希。"
                     "`git tag v1.0.0` 一行的事。")

    rc, out, _ = git("status", "--porcelain")
    if rc == 0 and out:
        n = len(out.splitlines())
        notes.append("工作区有 %d 处未提交改动 —— 打出来的版本号会带 `-dirty`。" % n)

    _report(problems, notes)
    return 0


def _report(problems, notes):
    for n in notes:
        print("  · %s" % n)
    for p in problems:
        print("  ✗ %s" % p)
    if not problems:
        print("  构建前置条件满足。")


if __name__ == "__main__":
    sys.exit(main())
