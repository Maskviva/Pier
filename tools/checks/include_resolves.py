# -*- coding: utf-8 -*-
"""include-resolves —— 每一条内部 `#include` 都必须**逐字符**解析得到一个真实文件。

契约 §九 原本没有这一条，加进来的理由是它盯的那一类事故在开发机上**永远
不会显形**：

  Windows 的 NTFS 大小写不敏感。`#include "pier/dimensions/base/NativeDimensions.h"`
  在磁盘上只有 `native_dimensions.h` 时，MSVC 照样编过。同一个头在同一个包
  里出现两种拼法（`NativeDimensions.cpp` 写小写、`Bridge.cpp` 写大写）不会
  有任何提示 —— 直到某天在 Linux 上跑一次 CI 或有人用大小写敏感的卷，
  才会突然「文件不存在」。

这正是编译器查不到、clippy 查不到的那一类，也就是 §九 说的「才值得写脚本」。

顺带把「include 了一个还不存在的头」一起报出来 —— 那是**功能缺席**的
最早信号：Slots.cpp 引用 `dim/CustomDimensionManager.h` 而它还没写，
说明这个包此刻编不过，而台账里那一行是 ⬜。两处必须对得上。
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

PKGS = os.path.join(ROOT, "packages")
INTERNAL = re.compile(r'\s*#\s*include\s*[<"]((?:pier|sdk)/[^>"]+)[>"]')


def run():
    r = Result("include-resolves")
    incdirs = []
    for p in sorted(os.listdir(PKGS)):
        d = os.path.join(PKGS, p, "include")
        if os.path.isdir(d):
            incdirs.append(d)

    missing, casewrong = [], []
    n = 0
    for dp, _, names in os.walk(PKGS):
        for fn in names:
            if not fn.endswith((".cpp", ".h", ".hpp")):
                continue
            p = os.path.join(dp, fn)
            rel = os.path.relpath(p, ROOT)
            n += 1
            with open(p, encoding="utf-8", errors="replace") as f:
                for i, line in enumerate(f, 1):
                    m = INTERNAL.match(line)
                    if not m:
                        continue
                    inc = m.group(1)
                    if any(os.path.exists(os.path.join(d, inc)) for d in incdirs):
                        continue
                    # 找一找是不是只差大小写 —— 这个区分决定了修法完全不同：
                    # 大小写不符 = 改一个字母；真缺失 = 还有一整个文件没写。
                    lower = inc.lower()
                    hit = None
                    for d in incdirs:
                        for dp2, _, fs2 in os.walk(d):
                            for f2 in fs2:
                                cand = os.path.relpath(os.path.join(dp2, f2), d)
                                if cand.replace(os.sep, "/").lower() == lower:
                                    hit = cand.replace(os.sep, "/")
                    if hit:
                        casewrong.append((rel, i, inc, hit))
                    else:
                        missing.append((rel, i, inc))

    for rel, i, inc, hit in casewrong:
        r.fail("%s:%d 大小写不符：写的是 %r，磁盘上是 %r（Windows 上编得过，"
               "大小写敏感的文件系统上直接找不到文件）" % (rel, i, inc, hit))
    for rel, i, inc in missing:
        r.fail("%s:%d include 了不存在的头 %r —— 这个包此刻编不过" % (rel, i, inc))

    if not r.failures:
        r.note("%d 个源文件的内部 include 全部逐字符解析成功" % n)
    return r


if __name__ == "__main__":
    sys.exit(run().report())
