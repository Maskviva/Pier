# -*- coding: utf-8 -*-
"""manifest-matches-host —— 示例模组的 `manifest.json` 必须真能被宿主装上。

盯的是什么：宿主只认 `"type": "<ModHostName>"` 的模组（`ModControl.cpp` 里
那一行硬判等），而 `ModHostName` 定义在 `hosted_mod.h`。示例里那个字符串
写错一个字，模组就**根本不会被扫到** —— 没有报错，没有日志，它就是不在
`/pier list` 里。

这不是假想：v0 的示例 manifest 依赖的是 `"levilamina-rust-loader"`，
而那个名字的模组在重命名之后已经不存在了，示例因此装不上。上一轮架构评审
逐个文件读才发现的，正是「编译器查不到」的那一类。

三条判据：

1. `type` 等于宿主的 `ModHostName`；
2. `dependencies` 里的宿主名也等于它（写错就是依赖一个不存在的模组）；
3. `entry` 和 crate 名对得上 —— cargo 产出的是 `<crate_name>.dll`，
   crate 名里的 `-` 会变成 `_`，这一步最容易手滑。
"""

import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

HOSTED_MOD_H = os.path.join(
    ROOT, "packages", "pier-host", "include", "pier", "host", "hosted_mod.h"
)


def run():
    r = Result("manifest-matches-host")

    if not os.path.exists(HOSTED_MOD_H):
        r.fail("找不到 hosted_mod.h —— 无从知道宿主认哪个 type")
        return r
    with open(HOSTED_MOD_H, encoding="utf-8") as f:
        m = re.search(r'ModHostName\s*=\s*"([^"]+)"', f.read())
    if not m:
        r.fail("hosted_mod.h 里找不到 ModHostName 的定义")
        return r
    host = m.group(1)
    r.note("宿主注册的模组类型名 = %r" % host)

    found = 0
    for dp, dirs, fs in os.walk(ROOT):
        dirs[:] = [d for d in dirs if d not in (".git", "target", "node_modules")]
        if "manifest.json" not in fs:
            continue
        p = os.path.join(dp, "manifest.json")
        rel = os.path.relpath(p, ROOT)
        found += 1
        try:
            with open(p, encoding="utf-8") as f:
                j = json.load(f)
        except Exception as e:  # noqa: BLE001
            r.fail("%s 不是合法 JSON：%s" % (rel, e))
            continue

        if j.get("type") != host:
            r.fail("%s 的 type 是 %r，宿主只认 %r —— 这个模组根本不会被扫到，"
                   "而且不会有任何报错" % (rel, j.get("type"), host))

        deps = [d.get("name") for d in j.get("dependencies", []) if isinstance(d, dict)]
        if host not in deps:
            r.fail("%s 的 dependencies 里没有 %r —— 装载顺序无从保证；"
                   "当前是 %s" % (rel, host, deps))
        for d in deps:
            if d and d != host and d.startswith(("levilamina-", "pier-")):
                r.fail("%s 依赖 %r —— 这不是宿主的名字，多半是改名没改干净" % (rel, d))

        # entry 与 crate 名
        cargo = os.path.join(dp, "Cargo.toml")
        if os.path.exists(cargo):
            with open(cargo, encoding="utf-8") as f:
                ct = f.read()
            lib = re.search(r"^\[lib\](.*?)(?=^\[|\Z)", ct, re.S | re.M)
            name = None
            if lib:
                mm = re.search(r'^\s*name\s*=\s*"([^"]+)"', lib.group(1), re.M)
                if mm:
                    name = mm.group(1)
            if name is None:
                mm = re.search(r'^\s*name\s*=\s*"([^"]+)"', ct, re.M)
                name = mm.group(1) if mm else None
            if name:
                want = name.replace("-", "_") + ".dll"
                if j.get("entry") != want:
                    r.fail("%s 的 entry 是 %r，但 cargo 产出的是 %r"
                           "（crate 名里的 - 会变成 _）" % (rel, j.get("entry"), want))

    if found == 0:
        r.note("仓库里没有 manifest.json —— 没有示例模组可查")
    elif not r.failures:
        r.note("%d 份 manifest 全部能被宿主装上" % found)
    return r


if __name__ == "__main__":
    sys.exit(run().report())
