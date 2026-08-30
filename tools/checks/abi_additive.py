# -*- coding: utf-8 -*-
"""abi-additive —— `PierApi` 只许在末尾追加（契约 §2.2）。

盯的是什么：重排或删除一个槽会让**所有已编译的模组**在那一槽之后集体
错位调用 —— 调用 `bus_publish` 实际打到别的函数指针上，没有任何诊断。
编译器看不见这件事：两边各自都编得过，错位只在运行期显形。

比对对象是 `tools/abi-v1.slots`（基线快照，随 ABI 一起提交）。
判据只有一条：**基线是当前槽序的前缀**。
追加不改版本；一旦这条断了，`PIER_ABI_VERSION` 和 `PIER_ABI_MIN_SUPPORTED`
必须同时推进 —— 检查会把这个要求一起验了。

刷新基线的时机：只有真的推进了版本号时，用 `--bless` 重写快照。
平时它是只读的事实。
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result, defines, read_abi, slots_of  # noqa: E402

LOCK = os.path.join(ROOT, "tools", "abi-v1.slots")


def _load_lock():
    if not os.path.exists(LOCK):
        return None
    out = {"version": None, "min": None, "slots": []}
    with open(LOCK, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            if line.startswith("!version "):
                out["version"] = line.split()[1]
            elif line.startswith("!min "):
                out["min"] = line.split()[1]
            else:
                out["slots"].append(line)
    return out


def _write_lock(slots, ver, mn):
    with open(LOCK, "w", encoding="utf-8") as f:
        f.write("# PierApi 槽序基线 —— abi-additive 机检的比对对象。\n")
        f.write("# 只有推进 PIER_ABI_VERSION 时才允许用 --bless 重写。\n")
        f.write("!version %s\n!min %s\n" % (ver, mn))
        for s in slots:
            f.write(s + "\n")


def run(bless=False):
    r = Result("abi-additive")
    src = read_abi()
    slots = slots_of(src, "PierApi")
    d = defines(src)
    ver = d.get("PIER_ABI_VERSION", "?").rstrip("u")
    mn = d.get("PIER_ABI_MIN_SUPPORTED", "?").rstrip("u")

    if ver != mn:
        r.note("VERSION=%s MIN_SUPPORTED=%s（区间兼容，合法）" % (ver, mn))

    lock = _load_lock()
    if lock is None:
        if bless:
            _write_lock(slots, ver, mn)
            r.note("基线不存在，已写入 %d 槽（v%s）" % (len(slots), ver))
            return r
        r.fail("基线 tools/abi-v1.slots 不存在 —— 先跑一次 --bless 固化 v1")
        return r

    if bless:
        _write_lock(slots, ver, mn)
        r.note("基线已重写：%d 槽（v%s）" % (len(slots), ver))
        return r

    base = lock["slots"]
    r.note("基线 %d 槽（v%s）→ 当前 %d 槽（v%s）" % (len(base), lock["version"], len(slots), ver))

    if len(slots) < len(base):
        r.fail("槽位变少了：%d → %d（删除是非追加变更）" % (len(base), len(slots)))

    n = min(len(base), len(slots))
    for i in range(n):
        if base[i] != slots[i]:
            r.fail("第 %d 槽从 %r 变成了 %r —— 重排/替换，不是追加" % (i, base[i], slots[i]))

    if r.failures:
        # 非追加变更是允许的，但必须同时推进两个版本号 —— 否则老模组会
        # 拿着「看起来兼容」的版本号装上一张已经错位的表。
        if ver == lock["version"]:
            r.fail(
                "上面是非追加变更，但 PIER_ABI_VERSION 还停在 %s —— "
                "契约 §2.2 要求 VERSION 与 MIN_SUPPORTED 同时推进到同一个数" % ver
            )
        elif ver != mn:
            r.fail("非追加变更时 MIN_SUPPORTED(%s) 必须等于 VERSION(%s)" % (mn, ver))
    elif len(slots) > len(base):
        r.note("纯追加 %d 槽 —— 版本号按契约不动，正确" % (len(slots) - len(base)))
        if ver != lock["version"]:
            r.fail("只是追加却动了版本号（%s → %s）—— 那等于宣布一次不存在的不兼容"
                   % (lock["version"], ver))

    return r


if __name__ == "__main__":
    sys.exit(run(bless="--bless" in sys.argv).report())
