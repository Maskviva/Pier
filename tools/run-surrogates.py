#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""跑齐全部 surrogate。**它们不是契约检查。**

契约 §九 的判据是「编译器和 clippy 查不到的才值得写脚本」。下面这五个查的
东西，编译器、链接器、clippy 本来就会报 —— 它们存在只因为这个仓库有一段
时间是在**没有 MSVC / 没有 cargo** 的环境里写出来的。有工具链之后，前四个
是冗余的，跑 `xmake` 和 `cargo clippy` 就够了。

保留它们的理由只有一条：在还没有工具链的那台机器上，它们是「离线能查到多少
算多少」的下限。每一个都逐条对应一次**真机报错**：

    link-surrogate      声明了但没实现            → 链接期未解析符号
    typed-storage       TypedStorage 上的 .get()   → C2228 / C2039
    include-surrogate   用了符号/std:: 却没 include → C2065 / C2039
    rust-surrogate      C 类型残留、悬空 ///、死代码 → cannot find type / never used
    example-surrogate   示例用了 prelude 没导出的   → cannot find type in this scope
    ledger-count        统计行与表体不符            →（这条没有编译器对应，是人工清点的替身）
    build-prereqs       没有 git 提交               → 打包阶段 `Not a valid object name HEAD`

用法：
    python3 tools/run-surrogates.py
"""

import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

SCRIPTS = [
    # 前置条件排最前：它一秒跑完，而它拦的那类错误会在构建 90% 处才炸。
    ("build-prereqs.py", "构建前置条件：git 仓库与提交"),
    ("link-surrogate.py", "链接器：声明了但没实现"),
    ("include-surrogate.py", "C++ 编译器：符号与 std:: 的 include 来源"),
    ("rust-surrogate.py", "cargo clippy：C 类型残留 / 悬空 /// / 死代码"),
    ("example-surrogate.py", "示例：prelude 覆盖得到用到的符号"),
    ("typed-storage.py", "TypedStorage 坍缩规则（需引擎头，缺则 SKIP）"),
    ("ledger-count.py", "人工清点：统计行 vs 表体"),
]


def main():
    failed, skipped, passed = [], [], []
    for script, what in SCRIPTS:
        path = os.path.join(HERE, script)
        if not os.path.exists(path):
            skipped.append((script, "脚本不存在"))
            continue
        print("── %s —— %s" % (script, what))
        p = subprocess.run([sys.executable, path], capture_output=True, text=True)
        sys.stdout.write(p.stdout)
        if p.stderr:
            sys.stderr.write(p.stderr)
        # SKIP 的判据是脚本自己在输出里声明的，不是退出码 —— 一个跳过了的
        # 检查退出码也是 0，把它算进「全过」正是这套工具一直在反对的
        # 「把没覆盖到的说成覆盖到了」。
        if p.returncode != 0:
            failed.append(script)
        elif re.search(r"^\s*SKIP\b", p.stdout, re.M):
            skipped.append((script, "缺前置条件，见上面的输出"))
        else:
            passed.append(script)
        print()

    print("=" * 62)
    if failed:
        print("FAIL —— %s" % "、".join(failed))
        return 1
    line = "PASS —— %d 个 surrogate 通过" % len(passed)
    if skipped:
        line += "，%d 个**跳过**（%s）" % (
            len(skipped), "；".join("%s：%s" % (a, b) for a, b in skipped)
        )
        line += "。跳过的那几条**没有结论**。"
    print(line + ("。" if not skipped else ""))
    print("提醒：它们只是没有工具链时的下限。真判据是 `xmake` 与 `cargo clippy`。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
