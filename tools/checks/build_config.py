# -*- coding: utf-8 -*-
"""build-config —— 各 `xmake.lua` 之间的一致性（契约 §一 的构建面）。

§九 原本没有这一条。加它的理由和 `include-resolves` 一样：这一类错误
**在你手上有 xmake 之前查不到，而 CI 上查到时已经是一次红**。它们又恰好
是分包重构最容易引入的一类 —— 包多了之后，「谁声明了什么」和「谁真的
需要什么」是两张表，没人逐条对。

三条判据：

1. **target 名唯一。** 新树把客户端槽位拆成 `packages/pier-client` 之后，
   根里那句 `target(is_client and "pier-client" or "pier")` 就和它重名了。
   旧仓没有这个包，所以这是重构**引入**的，不是遗留。

2. **`add_packages(X)` 必须有对应的 `add_requires(X)`。** 曾经写过
   `add_packages("levilamina-client")` —— 那个包不存在，配置阶段直接失败。
   包名不随构建配置改，客户端/服务端的差别在 `add_requires` 的 configs 里。

3. **用到某个外部库的头，就必须声明那个包。** 包的 includedirs 靠
   `add_packages` 继承。`pier-hooks` 满篇 `ll/api/memory/Hook.h` 却一个
   `add_packages` 都没有 —— 一个 TU 都编不过。

第 3 条只查 include 前缀能对上的部分。像 `bedrockdata` 那样**只在链接期**
起作用、不提供头文件的包，这条查不到，也不该由它来查。
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

PKGS = os.path.join(ROOT, "packages")

# 外部头前缀 → 提供它的 xmake 包。只列**提供头文件**的包。
HEADER_OWNER = {
    "ll/": "levilamina",
    "mc/": "levilamina",
    "nlohmann/": "levilamina",
    "magic_enum": "magic_enum",
    "snappy": "snappy",
}

# 只在链接期起作用、不提供头的包 —— 第 3 条对它们无话可说，列出来是为了
# 让「声明了但没有对应 include」不被误报成多余。
LINK_ONLY = {"bedrockdata", "prelink", "zlib", "levibuildscript", "legacymoney"}


def _read(p):
    with open(p, encoding="utf-8", errors="replace") as f:
        return f.read()


def _toks(text, fn):
    out = set()
    for m in re.finditer(r"%s\(([^)]*)\)" % fn, text):
        for t in re.findall(r'"([^"]+)"', m.group(1)):
            out.add(t.split()[0])  # "levilamina 26.20.4" -> "levilamina"
    return out


def run():
    r = Result("build-config")
    root_text = _read(os.path.join(ROOT, "xmake.lua"))

    # 只看 xmake 包。cargo crate 归 pkg-layering 的 check_crates 管。
    pkgs = sorted(
        d for d in os.listdir(PKGS)
        if os.path.isdir(os.path.join(PKGS, d))
        and os.path.exists(os.path.join(PKGS, d, "xmake.lua"))
    )
    pkg_texts = {p: _read(os.path.join(PKGS, p, "xmake.lua")) for p in pkgs}

    # ── 1. target 名唯一 ────────────────────────────────────────────
    seen = {}
    for label, text in [("xmake.lua", root_text)] + [
        ("packages/%s/xmake.lua" % p, t) for p, t in pkg_texts.items()
    ]:
        for m in re.finditer(r'^target\("([^"]+)"\)', text, re.M):
            name = m.group(1)
            if name in seen:
                r.fail("target %r 在 %s 和 %s 里各定义了一次 —— xmake 会报重复定义"
                       % (name, seen[name], label))
            seen[name] = label
        for m in re.finditer(r"^target\(([^\"][^)]*)\)", text, re.M):
            r.fail("%s 的 target 名是表达式 %r —— 名字不该随构建配置漂，"
                   "而且表达式的取值可能和某个包的 target 重名" % (label, m.group(1).strip()))
    r.note("target 名 %d 个，无重名：%s" % (len(seen), "、".join(sorted(seen))))

    # ── 2. add_packages ⊆ add_requires ─────────────────────────────
    required = _toks(root_text, "add_requires")
    for label, text in [("xmake.lua", root_text)] + [
        ("packages/%s/xmake.lua" % p, t) for p, t in pkg_texts.items()
    ]:
        for pk in _toks(text, "add_packages"):
            if pk not in required:
                r.fail("%s 的 add_packages(%r) 在根 add_requires 里没有对应项 —— "
                       "xmake 配置阶段就会失败" % (label, pk))
    r.note("根 add_requires 提供 %d 个外部包，所有 add_packages 都能对上" % len(required))

    # ── 3. 用了谁的头就要声明谁 ────────────────────────────────────
    for pkg in pkgs:
        declared = _toks(pkg_texts[pkg], "add_packages")
        needed = set()
        for dp, _, names in os.walk(os.path.join(PKGS, pkg)):
            for fn in names:
                if not fn.endswith((".cpp", ".h", ".hpp")):
                    continue
                for line in _read(os.path.join(dp, fn)).splitlines():
                    m = re.match(r'\s*#\s*include\s*[<"]([^>"]+)[>"]', line)
                    if not m:
                        continue
                    for prefix, owner in HEADER_OWNER.items():
                        if m.group(1).startswith(prefix):
                            needed.add(owner)
        for miss in sorted(needed - declared):
            r.fail("%s 用了 %s 的头，但没有 add_packages(%r) —— includedirs 继承不到，"
                   "一个 TU 都编不过" % (pkg, miss, miss))
        for extra in sorted(declared - needed - LINK_ONLY):
            r.note("%s 声明了 %r 但没有对应的 include —— 若它只在链接期起作用，"
                   "把它加进本检查的 LINK_ONLY" % (pkg, extra))

    if not r.failures:
        r.note("每个包声明的外部依赖都覆盖了它实际 include 的头")
    return r


if __name__ == "__main__":
    sys.exit(run().report())
