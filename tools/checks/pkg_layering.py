# -*- coding: utf-8 -*-
"""pkg-layering / object-kind / optional-drops —— 分包三条（契约 §一）。

这三条放一个文件，因为它们查的是同一份事实（每个包的 xmake.lua + 每个源
文件的 #include），只是判据不同。分三份会各写一遍目录遍历，然后各自漂移。

## pkg-layering
契约 §一 规则一：箭头只能向上，且那张图就是全部的边。
两个方向都要查，因为它们能各自独立地撒谎：
  - `add_deps` 声明的边（链接图）
  - `#include` 实际用的边（编译图）
v1 的实况是两张图对不上：契约画的是 DAG，真实是 api↔hooks 的环加一条
host→api 的隐藏链接边。「逐个查 add_deps ✓」查的是声明，不是现实。

## object-kind
契约 §一 规则四：所有包 `set_kind("object")`。
静态库会让链接器丢弃**没有外部符号引用**的编译单元，而 SPI 注册全靠
文件级静态对象 —— 被丢弃的症状是功能静默消失。这条检查还额外核对
「这个包是不是真的只靠自注册进入产物」，好让报错能说清后果。

## optional-drops
契约 §一 规则三：删掉根 xmake 的 `includes(<可选包>)` 后照常编译。
真正的判据要跑 xmake，这里做的是**静态的必要条件**：可选包的符号不许
被任何别的包引用。这个条件不充分（还有构建系统层面的耦合），但它足以
逮住 v1 那次事故 —— `ApiTable.cpp` 无条件转调 `bridge::laneModBusyName`，
删掉 pier-lane 直接链接断裂。脚本报 PASS 只代表「静态部分过了」，
交付前仍要真跑一次 `xmake f`，见 run-checks.py 的输出提示。
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

PKGS = os.path.join(ROOT, "packages")

# 契约 §一 的那张图。这里是**规矩**的机器可读副本 —— 图变了，这里必须一起变，
# 而不是反过来让脚本去描述现状。
ALLOWED_DEPS = {
    "pier-abi": set(),
    "pier-support": {"pier-abi"},
    "pier-host": {"pier-abi", "pier-support"},
    "pier-api": {"pier-abi", "pier-support", "pier-host"},
    "pier-hooks": {"pier-abi", "pier-support", "pier-host"},
    "pier-lane": {"pier-abi", "pier-support", "pier-host"},
    "pier-dimensions": {"pier-abi", "pier-support", "pier-host"},
    "pier-client": {"pier-abi", "pier-support", "pier-host"},
}

# 能力包 —— 它们互为兄弟，之间零边。
CAPABILITY = {"pier-api", "pier-hooks", "pier-lane", "pier-dimensions", "pier-client"}

# 可选包：从根 xmake 删掉那一行后必须照常编译。
OPTIONAL = {"pier-lane", "pier-dimensions"}

# `packages/` 下有**两种**包，判据完全不同：
#
#   xmake 包（有 xmake.lua）  宿主本体的 C++ 分包，受契约 §一 那张图约束
#   cargo crate（有 Cargo.toml）一门语言的绑定，受 §十 与 sys-mirrors-abi 约束
#
# 两者之间没有构建依赖，只有契约依赖 —— 绑定读的是 abi.h 那一份头文件。
# 把 crate 塞进 xmake 那套判据里只会得到「pier-sys-rs 没有 set_kind」这种
# 毫无意义的红，而一条会误报的检查最终会被人加进忽略列表。
CARGO_DEPS = {
    "pier-sys-rs": set(),                 # 只读 abi.h，零 crate 依赖
    "pier-rs": {"pier-sys-rs"},           # 安全封装，只依赖裸 FFI 那一层
}

# 每个包对外暴露的 include 前缀 —— 用来把 #include 归属到包。
INCLUDE_OWNER = {
    "sdk/": "pier-abi",
    "pier/support/": "pier-support",
    "pier/host/": "pier-host",
    "pier/api/": "pier-api",
    "pier/hooks/": "pier-hooks",
    "pier/lane/": "pier-lane",
    "pier/dimensions/": "pier-dimensions",
    "pier/client/": "pier-client",
}


def _pkg_dirs():
    """xmake 包（有 xmake.lua 的那些）。"""
    return sorted(
        d for d in os.listdir(PKGS)
        if os.path.isdir(os.path.join(PKGS, d))
        and os.path.exists(os.path.join(PKGS, d, "xmake.lua"))
    )


def _crate_dirs():
    """cargo crate（有 Cargo.toml 的那些）。"""
    return sorted(
        d for d in os.listdir(PKGS)
        if os.path.isdir(os.path.join(PKGS, d))
        and os.path.exists(os.path.join(PKGS, d, "Cargo.toml"))
    )


def _unclassified():
    """两种清单都不属于的目录。它们不会被任何一套判据覆盖 —— 必须报出来。"""
    known = set(_pkg_dirs()) | set(_crate_dirs())
    return sorted(
        d for d in os.listdir(PKGS)
        if os.path.isdir(os.path.join(PKGS, d)) and d not in known
    )


def _xmake_of(pkg):
    p = os.path.join(PKGS, pkg, "xmake.lua")
    if not os.path.exists(p):
        return ""
    with open(p, encoding="utf-8") as f:
        return f.read()


def _declared_deps(text):
    deps = set()
    for m in re.finditer(r"add_deps\(([^)]*)\)", text):
        for tok in re.findall(r'"([^"]+)"', m.group(1)):
            deps.add(tok)
    return deps


def _sources(pkg):
    root = os.path.join(PKGS, pkg)
    for dp, _, names in os.walk(root):
        for fn in names:
            if fn.endswith((".cpp", ".h", ".hpp")):
                yield os.path.join(dp, fn)


def check_crates(r):
    """cargo crate 的依赖判据：只能沿 CARGO_DEPS，且不许依赖任何 C++ 包。"""
    xmake_pkgs = set(_pkg_dirs())
    for crate in _crate_dirs():
        if crate not in CARGO_DEPS:
            r.fail("crate %s 不在契约 §一 的图里 —— 加绑定要先改契约" % crate)
            continue
        text = _read(os.path.join(PKGS, crate, "Cargo.toml"))
        # `[dependencies]` 段里出现的 pier-* 名字
        m = re.search(r"^\[dependencies\](.*?)(?=^\[|\Z)", text, re.S | re.M)
        deps = set(re.findall(r"^\s*(pier-[\w-]+)", m.group(1), re.M)) if m else set()
        for d in deps - CARGO_DEPS[crate]:
            r.fail("crate %s 依赖 %s —— 不在契约允许的边里" % (crate, d))
        for d in deps & xmake_pkgs:
            r.fail("crate %s 依赖 C++ 包 %s —— 两条构建线之间只有契约依赖，"
                   "不许有构建依赖" % (crate, d))
    if _crate_dirs():
        r.note("cargo crate %d 个，依赖只沿 %s"
               % (len(_crate_dirs()),
                  "、".join("%s→%s" % (k, "、".join(v) or "(无)") for k, v in CARGO_DEPS.items())))


def _read(path):
    if not os.path.exists(path):
        return ""
    with open(path, encoding="utf-8", errors="replace") as f:
        return f.read()


def check_layering(r):
    unk = _unclassified()
    for d in unk:
        r.fail("packages/%s 既没有 xmake.lua 也没有 Cargo.toml —— "
               "任何一套判据都覆盖不到它" % d)
    for pkg in _pkg_dirs():
        if pkg not in ALLOWED_DEPS:
            r.fail("包 %s 不在契约 §一 的图里 —— 加包要先改契约" % pkg)
            continue
        allowed = ALLOWED_DEPS[pkg]

        declared = _declared_deps(_xmake_of(pkg))
        # 只看 pier-* 的边；外部依赖（levilamina 等）不归这条检查管。
        declared = {d for d in declared if d.startswith("pier-")}
        for d in declared - allowed:
            kind = "横边（能力包互相认识）" if (pkg in CAPABILITY and d in CAPABILITY) else "越级/反向边"
            r.fail("链接图：%s 的 add_deps 含 %s —— %s，契约 §一 规则一不允许" % (pkg, d, kind))

        # 编译图：#include 归属
        for src in _sources(pkg):
            rel = os.path.relpath(src, ROOT)
            with open(src, encoding="utf-8", errors="replace") as f:
                for i, line in enumerate(f, 1):
                    m = re.match(r'\s*#\s*include\s*[<"]([^>"]+)[>"]', line)
                    if not m:
                        continue
                    inc = m.group(1)
                    for prefix, owner in INCLUDE_OWNER.items():
                        if inc.startswith(prefix) and owner != pkg:
                            if owner not in allowed:
                                kind = ("横边" if (pkg in CAPABILITY and owner in CAPABILITY)
                                        else "越级/反向边")
                                r.fail("编译图：%s:%d 的 #include \"%s\" 让 %s 依赖 %s —— %s"
                                       % (rel, i, inc, pkg, owner, kind))
                            break
    if not r.failures:
        r.note("链接图与编译图都只沿契约 §一 的边，能力包之间零横边")


def check_object_kind(r2):
    bad = []
    for pkg in _pkg_dirs():
        text = _xmake_of(pkg)
        m = re.search(r'set_kind\("([^"]+)"\)', text)
        kind = m.group(1) if m else "(未声明)"
        if pkg == "pier-abi":
            if kind != "headeronly":
                r2.fail("pier-abi 应为 headeronly（零源文件），实为 %s" % kind)
            continue
        if kind != "object":
            bad.append((pkg, kind))

    for pkg, kind in bad:
        # 把后果说清楚：这个包有几个「只靠自注册进产物」的 TU？
        selfreg = []
        for src in _sources(pkg):
            with open(src, encoding="utf-8", errors="replace") as f:
                t = f.read()
            if re.search(r"\b(SlotPackReg|BootstrapReg|TeardownReg|UnloadVetoReg|"
                         r"EventProviderReg|HookEventRegistrar)\b", t):
                selfreg.append(os.path.relpath(src, ROOT))
        detail = ""
        if selfreg:
            detail = ("；该包有 %d 个 TU 只靠文件级静态对象自注册"
                      "（无外部符号引用），静态库会被链接器整 obj 丢弃，"
                      "症状是功能静默消失：%s%s"
                      % (len(selfreg), ", ".join(os.path.basename(s) for s in selfreg[:4]),
                         " …" if len(selfreg) > 4 else ""))
        r2.fail("%s 是 set_kind(\"%s\")，契约 §一 规则四要求 object%s" % (pkg, kind, detail))

    if not bad:
        r2.note("八个包的 set_kind 全部合规（pier-abi=headeronly，其余 object）")


def check_optional_drops(r3):
    root_xmake = os.path.join(ROOT, "xmake.lua")
    with open(root_xmake, encoding="utf-8") as f:
        root_text = f.read()

    for opt in sorted(OPTIONAL):
        if 'includes("packages/%s")' % opt not in root_text:
            r3.fail("根 xmake 里找不到 includes(\"packages/%s\") —— 可选包的判据无从执行" % opt)

        # 该可选包的公开头前缀
        prefix = [p for p, o in INCLUDE_OWNER.items() if o == opt]
        for pkg in _pkg_dirs():
            if pkg == opt:
                continue
            if opt in _declared_deps(_xmake_of(pkg)):
                r3.fail("%s 的 add_deps 里有可选包 %s —— 删掉它就链接不上" % (pkg, opt))
            for src in _sources(pkg):
                with open(src, encoding="utf-8", errors="replace") as f:
                    for i, line in enumerate(f, 1):
                        m = re.match(r'\s*#\s*include\s*[<"]([^>"]+)[>"]', line)
                        if m and any(m.group(1).startswith(p) for p in prefix):
                            r3.fail("%s:%d 引用了可选包 %s 的头 —— 删掉 %s 就编不过"
                                    % (os.path.relpath(src, ROOT), i, opt, opt))
        # 根 target 的 add_deps 允许提到它（那正是要删的那一行的同伴）
    if not r3.failures:
        r3.note("可选包 %s 的符号无人跨包引用（静态必要条件通过；"
                "充分判据仍是真跑一次 xmake f）" % "、".join(sorted(OPTIONAL)))


def run():
    results = []
    r = Result("pkg-layering")
    check_layering(r)
    check_crates(r)
    results.append(r)
    r2 = Result("object-kind")
    check_object_kind(r2)
    results.append(r2)
    r3 = Result("optional-drops")
    check_optional_drops(r3)
    results.append(r3)
    return results


if __name__ == "__main__":
    rc = 0
    for res in run():
        rc |= res.report()
    sys.exit(rc)
