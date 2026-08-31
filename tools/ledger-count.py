#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ledger-count —— 从 MIGRATION.md 的**表体**算出统计行。

为什么要有这个脚本：那一行统计曾经写着 `✔75`，而表体逐行数出来是 70，
交付说明只好附一句「以表体为准」。一个需要附注解释才能读的数字是坏数字。

**它还按区域拆 ⬜。** 这一条是被一次真实的错话逼出来的：交付说明连续三轮
写「C++ 侧全量完成」，而台账里 `packages/pier-host/src/MemoryOperators.cpp`
一直挂着 ⬜ —— 那是个**装载阻断级**的文件，没有它 LeviLamina 直接拒绝
装载。数据一直在表里，只是没人把它按区域汇总，所以那句错话三轮都没被拦下。

一个只报总数的清点，挡不住「某个区域已经完成」这种**分区断言**。
现在每次跑都打一张分区表，写交付说明时照抄它，不许凭印象。

统计行是**导出量**，不是事实。事实只有表体那一行行状态。所以这里：

    python3 tools/ledger-count.py         # 只报，对不上就非零退出
    python3 tools/ledger-count.py --fix   # 按表体重写统计行

它同时是一条弱检查：`✔ + ✂ + ⬜` 必须等于表体行数。对不上说明有行的状态
字段写错了（比如打了个全角空格），而那种行会在肉眼清点时被跳过。
"""

import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
LEDGER = os.path.join(ROOT, "MIGRATION.md")

ROW = re.compile(r"^\|\s*`([^`]+)`\s*\|(.*)\|(.*)\|\s*$")
SUMMARY = re.compile(r"^\*\*统计\*\*：.*$", re.M)


def main():
    fix = "--fix" in sys.argv
    with open(LEDGER, encoding="utf-8") as f:
        text = f.read()

    # 只数**统计行之前**的表体 —— 统计行汇总的就是它上面那张表。
    # 文件末尾还可以有别的表（比如真机修复记录），它们不是台账的一部分。
    # 第一版没划这条线，于是往文件里加一张无关的表就会让清点报「状态字段
    # 不合法」——一条会因为无关改动而变红的检查，最终会被人绕过。
    cut_at = text.find("**统计**：")
    body = text[:cut_at] if cut_at >= 0 else text

    done = cut = todo = other = 0
    rows = 0
    bad = []
    for line in body.splitlines():
        m = ROW.match(line)
        if not m:
            continue
        status = m.group(3).strip()
        if status.startswith("---") or not status:
            continue
        rows += 1
        if status.startswith("✔"):
            done += 1
        elif status.startswith("✂"):
            cut += 1
        elif status.startswith("⬜"):
            todo += 1
        else:
            other += 1
            bad.append((m.group(1), status[:40]))

    total = done + cut + todo
    print("  表体 %d 行：✔ %d ｜ ✂ %d ｜ ⬜ %d" % (rows, done, cut, todo))
    if other:
        print("  ✗ %d 行的状态字段既不是 ✔ / ✂ / ⬜ —— 肉眼清点时会被跳过：" % other)
        for name, st in bad[:8]:
            print("      %s → %r" % (name, st))
        return 1

    # ── 按区域拆 ⬜ ──────────────────────────────────────────────
    areas = {}
    for line in body.splitlines():
        m = ROW.match(line)
        if not m or not m.group(3).strip().startswith("⬜"):
            continue
        path = m.group(1)
        parts = path.split("/")
        if path.startswith("packages/"):
            area = "/".join(parts[:2])
        elif "/" in path:
            area = parts[0] + "/"
        else:
            area = "(根目录)"
        areas[area] = areas.get(area, 0) + 1
    if areas:
        print("  ⬜ 按区域：")
        for area, k in sorted(areas.items(), key=lambda kv: -kv[1]):
            print("      %-28s %d" % (area, k))
        cpp = [a for a in areas if a.startswith("packages/pier-")
               and not a.endswith(("-rs", "-sys-rs"))]
        if cpp:
            print("      ⚠ C++ 侧仍有 ⬜：%s —— 交付说明里不许写「C++ 侧全量完成」"
                  % "、".join(sorted(cpp)))
        else:
            print("      C++ 侧（八个包）零 ⬜")
    else:
        print("  零 ⬜。")

    want = "**统计**：✔ %d ｜ ✂ %d ｜ ⬜ %d（共 %d 个旧文件）" % (done, cut, todo, total)
    m = SUMMARY.search(text)
    if not m:
        print("  ✗ 找不到统计行")
        return 1
    if m.group(0) == want:
        print("  ✓ 统计行与表体一致")
        return 0

    print("  ✗ 统计行与表体不符")
    print("      现在写的：%s" % m.group(0))
    print("      表体算的：%s" % want)
    if not fix:
        print("      用 --fix 按表体重写。")
        return 1
    with open(LEDGER, "w", encoding="utf-8") as f:
        f.write(SUMMARY.sub(want.replace("\\", "\\\\"), text, count=1))
    print("      已重写。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
