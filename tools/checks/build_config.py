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

3. **用到某个外部库的头，就必须声明那个包 —— 按 include 的传递闭包算。**
   包的 includedirs 靠 `add_packages` 继承。

   「按闭包算」是被真机编译逼出来的。第一版只看**直接** include，于是
   `pier-lane` 判成「不需要任何外部包」：`Lane.cpp` 自己确实只 include 了
   标准库和 `pier/`。但 `pier/host/hosted_mod.h` 里有
   `ll/api/event/ListenerBase.h` —— 编译器展开的是闭包，不是第一层。
   结果是 `fatal error C1083: 无法打开包括文件`。

   这条判据的形状和 `include-surrogate` 的第二类是同一个：**能不能编过，
   取决于闭包里有什么，不取决于这个文件自己写了什么。**

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

    # ── 3b. 有中文源文件就必须给 MSVC 加 /utf-8 ────────────────────
    #
    # 这个仓库的注释是中文的（那是它的一部分：每条注释都写「为什么」）。
    # 不加 /utf-8 的话，MSVC 在非 UTF-8 代码页下每个文件报一条 C4819，
    # 一次全量构建上百条，把真正的警告淹掉；加了 /WX 就是硬错。
    has_cjk = False
    for dp, _, names in os.walk(PKGS):
        if has_cjk:
            break
        for fn in names:
            if not fn.endswith((".cpp", ".h", ".hpp")):
                continue
            if any("\u4e00" <= ch <= "\u9fff" for ch in _read(os.path.join(dp, fn))[:4000]):
                has_cjk = True
                break
    # 判据必须落在**代码**上，不能是「文件里出现过这个串」——
    # 上面那段解释 /utf-8 的注释里就写着 `/utf-8`，按全文匹配的话，
    # 把那行 add_cxflags 删掉这条检查照样绿。
    # 这是本工程里同一类错误的第四次（前三次都是「在剥掉 X 的文本里找 X」）。
    # 统一的形状是：**判据看的东西和它想断言的东西不是同一个东西。**
    root_code = re.sub(r"--\[\[.*?\]\]", "", root_text, flags=re.S)
    root_code = re.sub(r"^\s*--[^\n]*", "", root_code, flags=re.M)
    has_flag = re.search(r'add_cxflags\s*\(\s*"[^"]*(/utf-8|/source-charset)', root_code) is not None
    if has_cjk and not has_flag:
        r.fail("源文件里有中文，但根 xmake 的**代码**里没有 "
               "`add_cxflags(\"/utf-8\", ...)` —— MSVC 会对每个文件报 C4819，"
               "把真正的警告淹掉；加了 /WX 就是硬错")
    elif has_cjk:
        r.note("源文件含中文，已给 MSVC 加 /utf-8")

    # ── 3. 用了谁的头就要声明谁（按 include 的传递闭包） ─────────────
    incdirs = [
        os.path.join(PKGS, p, "include")
        for p in pkgs
        if os.path.isdir(os.path.join(PKGS, p, "include"))
    ]

    def _external_closure(path, seen=None, ext=None, depth=0):
        """一个 TU 展开后会碰到的**外部**头。内部头照 include 递归进去。"""
        if seen is None:
            seen, ext = set(), set()
        if depth > 6 or path in seen or not os.path.exists(path):
            return ext
        seen.add(path)
        text = re.sub(r"/\*.*?\*/", "", _read(path), flags=re.S)
        text = re.sub(r"//[^\n]*", "", text)
        for m in re.finditer(r'#\s*include\s*[<"]([^>"]+)[>"]', text):
            inc = m.group(1)
            if inc.startswith(("pier/", "sdk/")):
                for d in incdirs:
                    cand = os.path.join(d, inc)
                    if os.path.exists(cand):
                        _external_closure(cand, seen, ext, depth + 1)
                        break
            else:
                ext.add(inc)
        return ext

    for pkg in pkgs:
        declared = _toks(pkg_texts[pkg], "add_packages")
        needed = set()
        for dp, _, names in os.walk(os.path.join(PKGS, pkg)):
            for fn in names:
                if not fn.endswith((".cpp", ".h", ".hpp")):
                    continue
                for inc in _external_closure(os.path.join(dp, fn)):
                    for prefix, owner in HEADER_OWNER.items():
                        if inc.startswith(prefix):
                            needed.add(owner)
        for miss in sorted(needed - declared):
            r.fail("%s 的 include 闭包里有 %s 的头，但没有 add_packages(%r) —— "
                   "includedirs 继承不到，编译器会报「无法打开包括文件」。"
                   "注意闭包：这个包自己可能一行 %s 的 include 都没写，"
                   "是经 pier/ 的头带进来的" % (pkg, miss, miss, miss))
        for extra in sorted(declared - needed - LINK_ONLY):
            r.note("%s 声明了 %r 但没有对应的 include —— 若它只在链接期起作用，"
                   "把它加进本检查的 LINK_ONLY" % (pkg, extra))

    if not r.failures:
        r.note("每个包声明的外部依赖都覆盖了它实际 include 的头")
    return r


if __name__ == "__main__":
    sys.exit(run().report())
