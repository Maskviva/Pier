#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""link-surrogate —— 「声明了但没人实现」的**代偿**检查。

## 这个脚本为什么不在 tools/checks/ 里

契约 §九 的判据是「编译器和 clippy 查不到的才值得写脚本」。缺定义这一类
**链接器一定会报**，所以按那条判据它不该成为契约的一条检查 —— 有工具链的
机器上它是冗余的。

它存在的唯一理由是：这个仓库有一段时间是在**没有 MSVC / xmake 的环境**里
写出来的。在那里，一个只声明没定义的函数可以一路混到交付，而症状是别人在
自己机器上第一次链接时才炸。这就是它的定位 —— **临时替身，不是规矩**。

判据是文本层面的，会漏（模板、内联定义、别的 TU 里的定义都可能误判），
所以它只报**可能**缺失，措辞也按这个来。真判据永远是一次真正的链接。

用法：
    python3 tools/link-surrogate.py [包名]
"""

import collections
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
PKGS = os.path.join(ROOT, "packages")

RET = r"(?:void|bool|int|short|uint\w*|int\d+_t|std::string|std::size_t|size_t|char const\*|" \
      r"std::optional<[^>]+>|std::unique_ptr<[^>]+>|Dimension\*|DimensionType|mce::Color|Vec3)"
DECL = re.compile(r"^\s+(?:\[\[nodiscard\]\]\s*)?(?:static\s+)?%s\s+(\w+)\s*\([^;{]*\)\s*"
                  r"(?:const\s*)?(?:noexcept\s*)?;" % RET, re.M)


def strip(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def main():
    only = sys.argv[1] if len(sys.argv) > 1 else None
    problems = 0
    for pkg in sorted(os.listdir(PKGS)):
        d = os.path.join(PKGS, pkg)
        if not os.path.isdir(d):
            continue
        if only and only not in pkg:
            continue
        inc = os.path.join(d, "include")
        src = os.path.join(d, "src")
        if not os.path.isdir(inc) or not os.path.isdir(src):
            continue

        declared = collections.OrderedDict()
        for dp, _, fs in os.walk(inc):
            for fn in fs:
                p = os.path.join(dp, fn)
                text = strip(open(p, encoding="utf-8", errors="replace").read())
                # 类体内的成员声明单独处理（下面），这里只要自由函数：
                # 粗略排除法 —— 类体一律有 `class X` 开头，把它们整段挖掉。
                text = re.sub(r"\bclass\s+\w+[^;{]*\{.*?\n    \};", "", text, flags=re.S)
                for m in DECL.finditer(text):
                    declared.setdefault(m.group(1), os.path.relpath(p, ROOT))

        body = ""
        for dp, _, fs in os.walk(src):
            for fn in fs:
                if fn.endswith((".cpp", ".h", ".hpp")):
                    body += open(os.path.join(dp, fn), encoding="utf-8", errors="replace").read()
        body = strip(body)

        missing = []
        for name, where in declared.items():
            # 定义形如 `<ret> name(` 出现在实现里；调用形如 `name(` 也匹配，
            # 所以只看**行首缩进后紧跟返回类型**的那种，减少误判。
            if re.search(r"(?:^|\n)\s*(?:\[\[nodiscard\]\]\s*)?%s\s+%s\s*\(" % (RET, re.escape(name)), body):
                continue
            # 内联定义在头里（`{` 紧跟）也算数
            missing.append((name, where))

        if missing:
            problems += len(missing)
            print("  %s" % pkg)
            for name, where in missing:
                print("      ? %-32s 声明于 %s，在 src/ 里找不到定义" % (name + "()", where))

    if problems == 0:
        print("  未发现可能缺失的定义（文本判据，会漏；真判据是一次真正的链接）")
        return 0
    print()
    print("  共 %d 处可疑。逐条人工确认 —— 模板、内联、别处的定义都可能是误判。" % problems)
    return 1


if __name__ == "__main__":
    sys.exit(main())
