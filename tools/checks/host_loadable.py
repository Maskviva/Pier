# -*- coding: utf-8 -*-
"""host-loadable: the host itself satisfies the hard requirements LeviLamina imposes at
load time.

What it watches: neither the compiler nor the linker checks this class of requirement. It
surfaces only when the mod is really placed in `mods/` and the server starts, and the
message that comes out has nothing to do with the build.

A real case: `MemoryOperators.cpp` was never written. All 98 TUs compiled, prelink ran,
`pier.dll` linked and the mod packaged, and at load time LeviLamina refused with a message
saying Pier could not be loaded and would not be loaded because it does not use the
unified memory allocation operators.

The ledger had that row outstanding the whole time: the data was right and the delivery
note was wrong, claiming the C++ side was complete three rounds running. So this check
pairs with the per-area summary of `ledger-count`: one asks whether the thing is there,
the other stops a summary from disagreeing with the ledger.

## Three criteria, taken one by one from the LeviLamina template

1. Unified memory operators: exactly one TU defines `LL_MEMORY_OPERATORS` and includes
   `ll/api/memory/MemoryOperators.h`. More than one redefines the global `operator new`.
2. Mod registration: exactly one `LL_REGISTER_MOD(...)`.
3. That TU must really reach the artifact. It has no external symbol reference, so a
   static library drops it whole (contract §1 rule 4). Its package therefore has to be
   `set_kind("object")` and must be in the unconditional include list of the root xmake.

The third is the valuable half of this check: whether the files exist is visible to the
eye, whether they were actually linked in is not, and the symptom is identical to the
files being absent.
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

PKGS = os.path.join(ROOT, "packages")


def _read(p):
    with open(p, encoding="utf-8", errors="replace") as f:
        return f.read()


def _strip(text):
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def _find(pattern):
    """Returns [(package name, relative path)], looking at code only, with comments stripped."""
    hits = []
    for dp, _, fs in os.walk(PKGS):
        for fn in sorted(fs):
            if not fn.endswith((".cpp", ".h", ".hpp")):
                continue
            p = os.path.join(dp, fn)
            if re.search(pattern, _strip(_read(p)), re.M):
                rel = os.path.relpath(p, ROOT).replace(os.sep, "/")
                hits.append((rel.split("/")[1], rel))
    return hits


def run():
    r = Result("host-loadable")
    root_xmake = _read(os.path.join(ROOT, "xmake.lua"))

    # 1. Unified memory operators
    mem = _find(r"^\s*#\s*define\s+LL_MEMORY_OPERATORS\b")
    if not mem:
        r.fail("no TU in the repository defines `LL_MEMORY_OPERATORS`, so LeviLamina refuses "
               "to load and reports that the unified memory allocation operators are not "
               "used. That error appears after the build has fully succeeded and relates to "
               "no compile-time check at all.")
    elif len(mem) > 1:
        r.fail("%d TUs define `LL_MEMORY_OPERATORS`: %s. The global operator new would be "
               "defined more than once" % (len(mem), ", ".join(x[1] for x in mem)))
    else:
        pkg, rel = mem[0]
        inc = re.search(r'#\s*include\s*"ll/api/memory/MemoryOperators\.h"',
                        _read(os.path.join(ROOT, rel)))
        if not inc:
            r.fail("%s defines LL_MEMORY_OPERATORS without including "
                   "`ll/api/memory/MemoryOperators.h`. The macro is only a switch and the "
                   "operators are defined in that header" % rel)
        else:
            r.note("unified memory operators: %s" % rel)
        _require_linked_in(r, pkg, rel, root_xmake, "the unified memory operators")

    # 2. Mod registration
    reg = _find(r"\bLL_REGISTER_MOD\s*\(")
    if not reg:
        r.fail("the repository has no `LL_REGISTER_MOD(...)`, so LeviLamina finds no mod entry point")
    elif len(reg) > 1:
        r.fail("`LL_REGISTER_MOD` appears %d times: %s. A mod registers exactly once"
               % (len(reg), ", ".join(x[1] for x in reg)))
    else:
        pkg, rel = reg[0]
        r.note("mod registration: %s" % rel)
        _require_linked_in(r, pkg, rel, root_xmake, "the mod registration")

    return r


def _require_linked_in(r, pkg, rel, root_xmake, what):
    """This TU must really reach the final artifact.

    It has no external symbol reference, since nothing calls anything inside it. A static
    library drops such an object whole, and the symptom afterwards is identical to the file
    not existing at all.
    """
    xm = os.path.join(PKGS, pkg, "xmake.lua")
    if not os.path.exists(xm):
        r.fail("%s sits in package %s, which has no xmake.lua" % (rel, pkg))
        return
    text = _read(xm)
    m = re.search(r'set_kind\("([^"]+)"\)', text)
    kind = m.group(1) if m else "(not declared)"
    if kind != "object":
        r.fail("%s (%s) sits in package %s, which is set_kind(%r). This TU has no external "
               "symbol reference, a static library drops it whole, and the symptom is identical "
               "to the file not existing"
               % (rel, what, pkg, kind))
    if 'includes("packages/%s")' % pkg not in root_xmake:
        r.fail("the root xmake has no includes(\"packages/%s\"), so %s is never compiled" % (pkg, what))
    # Unconditional: it must not be wrapped in any if
    for line in root_xmake.splitlines():
        if 'includes("packages/%s")' % pkg in line and line.startswith(" "):
            r.fail("includes(\"packages/%s\") in the root xmake is conditional, so %s is absent "
                   "under some configurations and LeviLamina refuses to load then" % (pkg, what))


if __name__ == "__main__":
    sys.exit(run().report())
