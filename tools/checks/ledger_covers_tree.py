# -*- coding: utf-8 -*-
"""ledger-covers-tree —— 工作区里的每个文件，台账里都要有一行。

盯的是什么：`MIGRATION.md` 是「功能只多不少」的判据，而它只有在**双向
完整**时才是判据：

  * 台账 → 工作区：⬜ 的行表示这个功能此刻缺席（`ledger-count` 管计数）；
  * 工作区 → 台账：**一个在磁盘上、却不在台账里的文件，等于没有被清点过**。

第二个方向此前完全没人守。发现它的方式很难看：`LICENSE` 三份
`Cargo.toml` 都声明 `license = "Apache-2.0"`，文件却没跟过来 —— 而台账里
连这一行都没有，所以逐行清点一百遍也发现不了。**清单漏了一项，按清单
核对就永远查不出那一项。**

## 判据

工作区里的每个受版本控制的文件，要么在台账某一行的「新位置」列里被提到，
要么在豁免名单里（构建产物、台账自己、本轮新增的工具）。

反过来不查：台账里有而工作区里没有的行，正是 ⬜ 的定义。
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

LEDGER = os.path.join(ROOT, "MIGRATION.md")

SKIP_DIRS = {".git", "target", "build", ".xmake", "node_modules", "__pycache__"}

# 豁免：这些文件天然不在「旧仓 → 新仓」的迁移台账里。
EXEMPT_EXACT = {
    "MIGRATION.md",       # 台账不清点自己
    ".gitignore",
    "Cargo.lock",
}
EXEMPT_PREFIX = (
    "tools/",             # 机检与 surrogate 是新架构的产物，没有旧仓对应物
)


def run():
    r = Result("ledger-covers-tree")
    if not os.path.exists(LEDGER):
        r.fail("找不到 MIGRATION.md")
        return r
    with open(LEDGER, encoding="utf-8") as f:
        text = f.read()

    # 台账里提到过的所有路径（反引号包着的都算，不区分哪一列 ——
    # 一个文件只要在台账里被点过名，就算清点过了）。
    mentioned = set(re.findall(r"`([^`]+)`", text))
    mentioned = {m.strip().rstrip("/") for m in mentioned}

    missing = []
    n = 0
    for dp, dirs, fs in os.walk(ROOT):
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS]
        for fn in sorted(fs):
            p = os.path.join(dp, fn)
            rel = os.path.relpath(p, ROOT).replace(os.sep, "/")
            if rel in EXEMPT_EXACT or rel.startswith(EXEMPT_PREFIX):
                continue
            n += 1
            if rel in mentioned:
                continue
            # 目录形式也算（台账有时按目录记）
            if any(rel.startswith(m + "/") for m in mentioned if "/" in m or "." not in m):
                continue
            missing.append(rel)

    for rel in missing:
        r.fail("%s 在工作区里，但台账里一次都没被点名 —— 它从来没有被清点过。"
               "「功能只多不少」按一份漏了项的清单核对，永远查不出漏的那一项" % rel)
    if not missing:
        r.note("%d 个文件全部在台账里有出处（豁免：MIGRATION.md 自己、.gitignore、tools/）" % n)
    return r


if __name__ == "__main__":
    sys.exit(run().report())
