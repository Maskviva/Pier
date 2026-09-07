# -*- coding: utf-8 -*-
"""sources-are-built: every .cpp under a package must be reached by that package's add_files.

What it watches: every other check in this directory enumerates by walking the filesystem,
so a `.cpp` that xmake never compiles is still parsed by `include-resolves`, budgeted by
`comment-style` and layered by `pkg-layering`. Those passes make a dead file look like a
maintained one. `pkg-layering` calls its `#include` graph the compile graph, and that name
is only true while the two sets agree; nothing was comparing them.

The way it was found is ugly: `pier-dimensions` listed `src/base`, `src/dim`, `src/spec`,
`src/gen` and `src/rt` in `add_files` and had a `src/plot` directory next to them holding
three files. Sixteen checks passed over those three files without one of them asking
whether anything compiles them.

## The criterion

For each package with an `xmake.lua`, the set of `.cpp` files under `src/` equals the set
matched by the `add_files` patterns of that package. A file on disk that no pattern reaches
fails.

Coverage is the text of `add_files` and not xmake's own resolution: this reimplements the
two glob forms in use, `*` for one directory level and `**` for a recursive walk, so a
pattern form that arrives later is read by xmake and not by this script. It also reads
patterns regardless of the `if is_config(...)` branch they sit in, so a file reached only
on one target still counts as built; deciding per target needs a real `xmake f`. What it
does guarantee is the direction that matters here: nothing sits under `src/` unreferenced.

The reverse, a pattern matching nothing, is not a failure. A capability package whose
directory is empty on a target is the deliberate degradation of contract §1 rule 4.
"""

import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

PKGS = os.path.join(ROOT, "packages")

ADD_FILES = re.compile(r"add_files\(([^)]*)\)")
STRING = re.compile(r'"([^"]+)"')


def matched_by(base, pattern):
    """The files one add_files pattern reaches, in xmake's glob spelling.

    xmake writes a recursive walk as `src/**.cpp` while Python's glob wants
    `src/**/*.cpp` plus recursive=True. Translating here rather than calling xmake keeps
    the check runnable with no toolchain, which is the whole point of tools/.
    """
    py = pattern.replace("/**.", "/**/*.")
    out = set()
    for f in glob.glob(os.path.join(base, py), recursive=True):
        if os.path.isfile(f):
            out.add(os.path.relpath(f, base).replace(os.sep, "/"))
    return out


def run():
    r = Result("sources-are-built")
    if not os.path.isdir(PKGS):
        r.fail("packages/ was not found")
        return r

    dead = []
    n_pkg = 0
    n_src = 0
    for pkg in sorted(os.listdir(PKGS)):
        base = os.path.join(PKGS, pkg)
        lua = os.path.join(base, "xmake.lua")
        if not os.path.isfile(lua):
            continue
        n_pkg += 1
        with open(lua, encoding="utf-8") as f:
            text = f.read()

        built = set()
        for call in ADD_FILES.finditer(text):
            for pattern in STRING.findall(call.group(1)):
                built |= matched_by(base, pattern)

        on_disk = set()
        src = os.path.join(base, "src")
        for dp, dirs, fs in os.walk(src):
            dirs[:] = [d for d in dirs if d not in ("__pycache__",)]
            for fn in fs:
                if fn.endswith(".cpp"):
                    p = os.path.join(dp, fn)
                    on_disk.add(os.path.relpath(p, base).replace(os.sep, "/"))
        n_src += len(on_disk)

        for rel in sorted(on_disk - built):
            dead.append((pkg, rel))

    for pkg, rel in dead:
        r.fail("%s/%s is on disk and no add_files pattern of %s reaches it, so nothing "
               "compiles it while every text check still parses and budgets it, which is "
               "what makes a retired file read as a maintained one" % (pkg, rel, pkg))
    if not dead:
        r.note("all %d .cpp file(s) across %d package(s) are reached by an add_files pattern. "
               "The criterion reimplements the `*` and `**` globs and ignores which "
               "is_config branch a pattern sits in, so it proves nothing under src/ is "
               "unreferenced and not that every file builds on every target" % (n_src, n_pkg))
    return r


if __name__ == "__main__":
    sys.exit(run().report())
