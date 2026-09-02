# -*- coding: utf-8 -*-
"""build-config: consistency among the `xmake.lua` files, the build face of contract §1.

§9 did not originally have this one. The reason for adding it is the same as for
`include-resolves`: this class of error cannot be found before xmake is available, and
finding it in CI already means a red run. It also happens to be the class a
multi-package refactor most easily introduces, since once there are many packages, what
each declares and what each actually needs are two tables nobody reconciles row by row.

Three criteria:

1. Target names are unique. Once the new tree split the client slots into
   `packages/pier-client`, the root line `target(is_client and "pier-client" or "pier")`
   collided with it. The old repository had no such package, so the refactor introduced
   this rather than inheriting it.
2. Every `add_packages(X)` has a matching `add_requires(X)`. An `add_packages(
   "levilamina-client")` was once written for a package that does not exist, and the
   configure stage failed outright. A package name does not change with the build
   configuration; the client and server difference lives in the configs of
   `add_requires`.
3. Using a header of an external library requires declaring that package, computed over
   the transitive closure of includes, since a package inherits its includedirs through
   `add_packages`.

   Working over the closure was forced by a real compile. The first version looked at
   direct includes only and judged `pier-lane` to need no external package at all, since
   `Lane.cpp` really does include nothing beyond the standard library and `pier/`. But
   `pier/host/hosted_mod.h` includes `ll/api/event/ListenerBase.h`, and the compiler
   expands the closure and not the first level. The result was
   `fatal error C1083: cannot open include file`.

   The shape of this criterion is that whether something compiles depends on what is in
   the closure and not on what the file itself wrote.

The third criterion only checks what an include prefix can be matched against. A package
such as `bedrockdata`, which acts at link time only and ships no header, is invisible to
it and should be.
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

PKGS = os.path.join(ROOT, "packages")

# External header prefix to the xmake package providing it. Only packages that ship
# headers are listed.
HEADER_OWNER = {
    "ll/": "levilamina",
    "mc/": "levilamina",
    "nlohmann/": "levilamina",
    "magic_enum": "magic_enum",
    "snappy": "snappy",
}

# Packages acting at link time only and shipping no header. The third criterion has
# nothing to say about them, and they are listed so that a declaration without a matching
# include is not misreported as redundant.
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

    # xmake packages only. A cargo crate belongs to check_crates in pkg-layering.
    pkgs = sorted(
        d for d in os.listdir(PKGS)
        if os.path.isdir(os.path.join(PKGS, d))
        and os.path.exists(os.path.join(PKGS, d, "xmake.lua"))
    )
    pkg_texts = {p: _read(os.path.join(PKGS, p, "xmake.lua")) for p in pkgs}

    # 1. Target names are unique
    seen = {}
    for label, text in [("xmake.lua", root_text)] + [
        ("packages/%s/xmake.lua" % p, t) for p, t in pkg_texts.items()
    ]:
        for m in re.finditer(r'^target\("([^"]+)"\)', text, re.M):
            name = m.group(1)
            if name in seen:
                r.fail("target %r is defined once in %s and once in %s, and xmake reports a duplicate definition"
                       % (name, seen[name], label))
            seen[name] = label
        for m in re.finditer(r"^target\(([^\"][^)]*)\)", text, re.M):
            r.fail("the target name of %s is the expression %r. A name must not drift with the "
                   "build configuration, and the value of an expression may collide with the "
                   "target of some package" % (label, m.group(1).strip()))
    r.note("%d target name(s), none duplicated: %s" % (len(seen), ", ".join(sorted(seen))))

    # ── 2. add_packages ⊆ add_requires ─────────────────────────────
    required = _toks(root_text, "add_requires")
    for label, text in [("xmake.lua", root_text)] + [
        ("packages/%s/xmake.lua" % p, t) for p, t in pkg_texts.items()
    ]:
        for pk in _toks(text, "add_packages"):
            if pk not in required:
                r.fail("%s declares add_packages(%r) with no counterpart in the root "
                       "add_requires, so the xmake configure stage fails" % (label, pk))
    r.note("the root add_requires provides %d external package(s) and every add_packages matches one" % len(required))

    # 3b. Any source file carrying CJK characters requires /utf-8 for MSVC
    #
    # The criterion is conditional and says nothing about which language the comments are
    # in. Whenever a source file does carry CJK characters, MSVC under a non-UTF-8 code
    # page reports one C4819 per file, which is hundreds of lines in a full build and
    # buries the real warnings, and becomes a hard error under /WX.
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
    # The criterion has to land on code and not on the string appearing anywhere in the
    # file: the comment above explaining /utf-8 contains `/utf-8` itself, so a whole-text
    # match would stay green after that add_cxflags line is deleted. This is the fourth
    # instance of the same mistake in this project, the previous three all being a search
    # for X inside text with X stripped out. The common shape is that what the criterion
    # looks at is not the same thing as what it means to assert.
    root_code = re.sub(r"--\[\[.*?\]\]", "", root_text, flags=re.S)
    root_code = re.sub(r"^\s*--[^\n]*", "", root_code, flags=re.M)
    has_flag = re.search(r'add_cxflags\s*\(\s*"[^"]*(/utf-8|/source-charset)', root_code) is not None
    if has_cjk and not has_flag:
        r.fail("a source file carries CJK characters while the code of the root xmake has no "
               "`add_cxflags(\"/utf-8\", ...)`. MSVC reports one C4819 per file, which buries "
               "the real warnings and is a hard error under /WX")
    elif has_cjk:
        r.note("a source file carries CJK characters and /utf-8 is passed to MSVC")

    # 3. Using a package's header requires declaring it, over the transitive include closure
    incdirs = [
        os.path.join(PKGS, p, "include")
        for p in pkgs
        if os.path.isdir(os.path.join(PKGS, p, "include"))
    ]

    def _external_closure(path, seen=None, ext=None, depth=0):
        """The external headers one TU reaches once expanded. Internal headers are recursed into along their includes."""
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
            r.fail("the include closure of %s reaches a header of %s while there is no "
                   "add_packages(%r). The includedirs are not inherited and the compiler reports "
                   "that it cannot open the include file. Note the closure: this package may "
                   "write no %s include of its own and reach it through a header under pier/"
                   % (pkg, miss, miss, miss))
        for extra in sorted(declared - needed - LINK_ONLY):
            r.note("%s declares %r with no matching include. If it acts at link time only, add "
                   "it to LINK_ONLY in this check" % (pkg, extra))

    if not r.failures:
        r.note("the external dependencies each package declares cover the headers it actually includes")
    return r


if __name__ == "__main__":
    sys.exit(run().report())
