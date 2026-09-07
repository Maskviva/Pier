#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""compile-all.py: compile every translation unit and report every failure at once.

xmake stops the build at the first target that fails, and with a parallel build the
one it reports is whichever finished failing first. Adapting to an engine version
that removed a few hundred symbols means one error at a time, one full build apart,
and the file it names is not even the first one in build order.

xmake has no keep-going switch. What it does have is the compilation database, so
this drives that instead: every entry is compiled on its own, a failure is recorded
and the next entry still runs. Nothing is linked and no object is placed where the
build would find it, so this neither replaces a build nor disturbs one.

The compiler is invoked the way the database records it, which means the toolchain's
own environment has to be present. cl.exe finds the standard library through the INCLUDE
variable and not through any flag in the database, so this has to run from the same shell
the build runs from. Outside it every file fails at once on <cstdint>, which looks like a
catastrophe and is only a missing variable.

Usage:
    # from the Visual Studio Developer Command Prompt, the same one build.bat opens
    xmake project -k compile_commands        # refresh the database first
    python3 tools/compile-all.py             # every file
    python3 tools/compile-all.py pier-api    # only paths containing that text

The exit code is the number of files that failed, capped at 125.
"""

import concurrent.futures
import json
import os
import subprocess
import sys
import tempfile

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
DB = os.path.join(ROOT, "compile_commands.json")


def entries(needle):
    if not os.path.isfile(DB):
        print("compile_commands.json is not in the workspace. Generate it with:")
        print("    xmake project -k compile_commands")
        sys.exit(126)
    with open(DB, encoding="utf-8") as f:
        db = json.load(f)
    out = []
    for e in db:
        src = e.get("file") or (e.get("arguments") or [""])[-1]
        if needle and needle not in src.replace("\\", "/"):
            continue
        out.append((src, e))
    return out


def check_environment(items):
    """Refuse to run when the toolchain environment the database assumes is absent.

    Reporting 86 identical <cstdint> failures buries whatever the real errors are, so
    the condition that produces them is named up front instead.
    """
    if not items:
        return True
    argv0 = (items[0][1].get("arguments") or [""])[0].lower()
    if "cl.exe" not in argv0 and not argv0.endswith("cl"):
        return True
    if os.environ.get("INCLUDE"):
        return True
    print("INCLUDE is not set, and cl.exe reads the standard library headers from it.")
    print("Every file would fail on <cstdint> for that reason alone.")
    print("Run this from the Visual Studio Developer Command Prompt, the same shell")
    print("the build runs in, or run vcvarsall.bat x64 first.")
    return False


def compile_one(item, objdir):
    """Compile one entry to a scratch object. Returns (source, output) on failure."""
    src, e = item
    args = list(e.get("arguments") or [])
    if not args:
        return None
    # Redirect the object next to nothing the build reads. /Fo is MSVC, -o is the rest.
    stem = os.path.basename(src).replace(".", "_")
    for i, a in enumerate(args):
        if a.startswith("/Fo"):
            args[i] = "/Fo" + os.path.join(objdir, stem + ".obj")
        elif a == "-o" and i + 1 < len(args):
            args[i + 1] = os.path.join(objdir, stem + ".o")
    try:
        p = subprocess.run(args, cwd=e.get("directory") or ROOT,
                           capture_output=True, text=True, errors="replace")
    except OSError as ex:
        return (src, "could not be started: %s" % ex)
    if p.returncode == 0:
        return None
    return (src, (p.stdout or "") + (p.stderr or ""))


def main():
    needle = sys.argv[1] if len(sys.argv) > 1 else None
    items = entries(needle)
    if not items:
        print("no entry matched %r" % needle)
        return 0

    if not check_environment(items):
        return 126

    print("compiling %d translation unit(s)%s\n"
          % (len(items), " matching %r" % needle if needle else ""))
    failures = []
    with tempfile.TemporaryDirectory() as objdir:
        n = os.cpu_count() or 4
        with concurrent.futures.ThreadPoolExecutor(max_workers=n) as pool:
            futures = {pool.submit(compile_one, it, objdir): it[0] for it in items}
            done = 0
            for fut in concurrent.futures.as_completed(futures):
                done += 1
                r = fut.result()
                mark = "." if r is None else "F"
                sys.stdout.write(mark)
                sys.stdout.flush()
                if r is not None:
                    failures.append(r)
    print("\n")

    if not failures:
        print("all %d file(s) compiled" % len(items))
        return 0

    failures.sort()
    for src, output in failures:
        rel = os.path.relpath(src, ROOT) if os.path.isabs(src) else src
        print("=" * 70)
        print(rel)
        print("-" * 70)
        print(output.strip())
        print()
    print("=" * 70)
    print("%d of %d file(s) failed:" % (len(failures), len(items)))
    for src, _ in failures:
        print("    %s" % (os.path.relpath(src, ROOT) if os.path.isabs(src) else src))
    return min(len(failures), 125)


if __name__ == "__main__":
    sys.exit(main())
