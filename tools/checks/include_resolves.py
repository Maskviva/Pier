# -*- coding: utf-8 -*-
"""include-resolves: every internal `#include` must resolve character for character to a
real file.

Contract §9 did not originally have this one. It was added because the class of accident
it watches never surfaces on a development machine:

  NTFS on Windows is case insensitive.
  `#include "pier/dimensions/base/NativeDimensions.h"` compiles under MSVC while the disk
  holds only `native_dimensions.h`. The same header spelled two ways inside one package,
  lowercase in `NativeDimensions.cpp` and uppercase in `Bridge.cpp`, produces no hint at
  all, until one day CI runs on Linux or someone uses a case-sensitive volume and the
  file suddenly does not exist.

That is exactly the class the compiler cannot find and clippy cannot find, which is what
§9 calls worth a script.

It also reports an include of a header that does not exist yet, which is the earliest
signal of an absent capability: Slots.cpp referencing `dim/CustomDimensionManager.h`
before it is written means the package does not compile at this moment, and the matching
ledger row is outstanding. The two have to agree.
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

PKGS = os.path.join(ROOT, "packages")
INTERNAL = re.compile(r'\s*#\s*include\s*[<"]((?:pier|sdk)/[^>"]+)[>"]')


def run():
    r = Result("include-resolves")
    incdirs = []
    for p in sorted(os.listdir(PKGS)):
        d = os.path.join(PKGS, p, "include")
        if os.path.isdir(d):
            incdirs.append(d)

    missing, casewrong = [], []
    n = 0
    for dp, _, names in os.walk(PKGS):
        for fn in names:
            if not fn.endswith((".cpp", ".h", ".hpp")):
                continue
            p = os.path.join(dp, fn)
            rel = os.path.relpath(p, ROOT)
            n += 1
            with open(p, encoding="utf-8", errors="replace") as f:
                for i, line in enumerate(f, 1):
                    m = INTERNAL.match(line)
                    if not m:
                        continue
                    inc = m.group(1)
                    if any(os.path.exists(os.path.join(d, inc)) for d in incdirs):
                        continue
                    # Look for a match differing only in case. The distinction decides
                    # entirely different fixes: a case mismatch is one letter, while a real
                    # absence means a whole file is still unwritten.
                    lower = inc.lower()
                    hit = None
                    for d in incdirs:
                        for dp2, _, fs2 in os.walk(d):
                            for f2 in fs2:
                                cand = os.path.relpath(os.path.join(dp2, f2), d)
                                if cand.replace(os.sep, "/").lower() == lower:
                                    hit = cand.replace(os.sep, "/")
                    if hit:
                        casewrong.append((rel, i, inc, hit))
                    else:
                        missing.append((rel, i, inc))

    for rel, i, inc, hit in casewrong:
        r.fail("%s:%d case mismatch: written as %r while the disk holds %r; this compiles on "
               "Windows and the file simply does not exist on a case-sensitive filesystem"
               % (rel, i, inc, hit))
    for rel, i, inc in missing:
        r.fail("%s:%d includes the nonexistent header %r, so this package does not compile at this moment" % (rel, i, inc))

    if not r.failures:
        r.note("every internal include across %d source file(s) resolved character for character" % n)
    return r


if __name__ == "__main__":
    sys.exit(run().report())
