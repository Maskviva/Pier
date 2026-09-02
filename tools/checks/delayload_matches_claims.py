# -*- coding: utf-8 -*-
"""delayload-matches-claims —— 说是延迟加载的，链接器那边必须真的延迟加载。

## 为什么需要它

`money_guard.h` 从第一天就写着「LLMoney_* 住在延迟加载的 LegacyMoney.dll 里」，
`Money.cpp` 的每个入口都老老实实先过 `moneyBackendReady()`，运行期降级逻辑一行
不差。但 `xmake.lua` 里从来没有 `/DELAYLOAD:LegacyMoney.dll` —— `add_packages`
把导入库直接链了进去。

后果不是「经济功能不可用」，是**整个 pier 装不上**：加载器在载入 pier.dll 时就
解析不了导入表，报 `0x7E 找不到指定的模块`。那条报错里没有「money」这个词，
所以从症状回溯到根因要跨过整个运行期降级逻辑 —— 而那套逻辑看起来完全正确，
只是一行都没机会跑。

这正是契约 §5.4 说的「注释对代码撒谎」的最坏形态:注释描述的是**设计意图**，
代码实现了意图的一半，而缺的那一半在另一个文件里，没人对得上。

## 判据

在 C++ 源码与头文件里找「延迟加载」的声称（中文「延迟加载」或英文
delay-load / delay load / DELAYLOAD），从上下文取出 `X.dll`，然后要求
根 `xmake.lua` 里有对应的 `/DELAYLOAD:X.dll`。

反向也查：`/DELAYLOAD:` 列了某个 DLL，却没有任何一处代码在运行期检查它是否
可用，那是另一种失配 —— 延迟加载失败会在第一次调用时抛 SEH 异常，没有守卫
就是把「没装这个可选依赖」变成一次崩溃。这一条只报告，不判失败:守卫可能写在
判据看不见的地方（比如别的 TU 里的一个通用 helper）。
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

CPP_DIRS = [os.path.join(ROOT, "packages")]
CLAIM = re.compile(r"延迟加载|delay-?\s?load", re.I)
DLLNAME = re.compile(r"([A-Za-z][\w.-]*\.dll)")

# 宿主自己的产物，不可能是它自己的延迟加载对象。
SELF = {"pier.dll", "pier-client.dll"}


def _claimed_dlls():
    """代码里被说成「延迟加载」的 DLL → 出处。"""
    out = {}
    for base in CPP_DIRS:
        for dp, _, fs in os.walk(base):
            for fn in sorted(fs):
                if not fn.endswith((".cpp", ".h", ".hpp")):
                    continue
                p = os.path.join(dp, fn)
                lines = open(p, encoding="utf-8", errors="replace").read().split("\n")
                for i, line in enumerate(lines):
                    if not CLAIM.search(line):
                        continue
                    # DLL 名可能落在这一行，也可能在紧邻的上下两行。
                    #
                    # 排除宿主自己:讲清楚「pier.dll 会加载失败」是**后果**的
                    # 描述，不是「pier.dll 被延迟加载」的声称。一条把后果误当
                    # 成声称的检查，第一次跑就会报一个假阳性，而假阳性正是让
                    # 人把检查加进忽略列表的那件事。
                    window = " ".join(lines[max(0, i - 1) : i + 2])
                    for m in DLLNAME.finditer(window):
                        dll = m.group(1)
                        if dll.lower() in SELF:
                            continue
                        out.setdefault(dll, []).append(
                            "%s:%d" % (os.path.relpath(p, ROOT), i + 1)
                        )
    return out


def run():
    r = Result("delayload-matches-claims")
    xm = os.path.join(ROOT, "xmake.lua")
    if not os.path.exists(xm):
        r.fail("根 xmake.lua 不存在")
        return r
    build = open(xm, encoding="utf-8").read()
    flagged = {m.group(1) for m in re.finditer(r"/DELAYLOAD:([\w.-]+\.dll)", build)}

    claimed = _claimed_dlls()
    for dll, where in sorted(claimed.items()):
        if dll in flagged:
            continue
        r.fail(
            "%s 在代码里被说成是延迟加载的（%s），但根 xmake.lua 里没有 "
            "/DELAYLOAD:%s —— 导入库会被静态链进去，宿主在**加载自己**时就失败，"
            "报 `0x7E 找不到指定的模块`，运行期的降级逻辑一行都跑不到"
            % (dll, where[0], dll)
        )

    if flagged and "delayimp" not in build:
        r.fail(
            "用了 /DELAYLOAD 但没链 delayimp —— 链接器会报 __delayLoadHelper2 未定义"
        )

    # /DELAYLOAD 要落在**对的那个 flag 通道**上。
    #
    # xmake 里 `add_ldflags` 只作用于 `kind("binary")`，共享库走 `add_shflags`。
    # 写错通道不会报错，也不会中断构建 —— 那一行被静默忽略，产物照样带硬性
    # DLL 依赖，症状和根本没加时**一模一样**。这一条就是为这个失效模式存在的:
    # 上一次修这个 bug 时写成了 ldflags，用户重编之后拿到了完全相同的报错。
    for m in re.finditer(r'target\("(\w+)"\)(.*?)target_end\(\)', build, re.S):
        name, body = m.group(1), m.group(2)
        if "/DELAYLOAD:" not in body:
            continue
        shared = 'set_kind("shared")' in body
        has_sh = re.search(r"add_shflags\([^)]*DELAYLOAD", body)
        has_ld = re.search(r"add_ldflags\([^)]*DELAYLOAD", body)
        if shared and not has_sh:
            r.fail(
                "target `%s` 是 shared，但 /DELAYLOAD 只写在 add_ldflags 上 —— "
                "xmake 对共享库读的是 add_shflags，这一行会被静默忽略，"
                "构建照常成功而产物照样带硬性 DLL 依赖" % name
            )
        if not shared and not has_ld:
            r.fail(
                "target `%s` 不是 shared，但 /DELAYLOAD 只写在 add_shflags 上" % name
            )

    for dll in sorted(flagged):
        stem = dll[:-4]
        guarded = False
        for base in CPP_DIRS:
            for dp, _, fs in os.walk(base):
                for fn in fs:
                    if not fn.endswith((".cpp", ".h", ".hpp")):
                        continue
                    t = open(
                        os.path.join(dp, fn), encoding="utf-8", errors="replace"
                    ).read()
                    if stem in t and re.search(r"Ready\(\)|available\(\)|resolve\(", t):
                        guarded = True
        if not guarded:
            r.note(
                "%s 声明了延迟加载，但没找到运行期可用性守卫。延迟加载失败会在"
                "第一次调用时抛 SEH —— 没有守卫就是把「没装这个可选依赖」变成崩溃。"
                "（判据看不见跨 TU 的通用 helper，所以这只是提醒）" % dll
            )

    if not r.failures:
        r.note(
            "声称延迟加载的 DLL %d 个，链接器标志 %d 个，一一对应。"
            "判据只比对**名字**:它保证不了那个守卫真的守住了每个入口。"
            % (len(claimed), len(flagged))
        )
    return r


if __name__ == "__main__":
    sys.exit(run().report())
