#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Runs every surrogate. They are not contract checks.

The criterion of contract §9 is that only what the compiler and clippy cannot find is
worth a script. The surrogates that duplicated the compiler, the linker and clippy have
been removed now that a toolchain exists; `xmake` and `cargo clippy` cover what they
covered. What remains has no compiler counterpart, and each corresponds to a real
failure seen on a machine:

    typed-storage       a .get() on a TypedStorage  -> C2228 / C2039, and the rule table
                                                       for the collapse lives in its header
    ledger-count        the summary line disagrees  -> no compiler counterpart; this
                        with the table body            stands in for counting by hand
    build-prereqs       no git commit               -> `Not a valid object name HEAD` at
                                                       the packaging stage

Usage:
    python3 tools/run-surrogates.py
"""

import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

SCRIPTS = [
    # Preconditions come first: this runs in a second while the errors it catches only
    # blow up at 90% of a build.
    ("build-prereqs.py", "build preconditions: a git repository with a commit"),
    ("typed-storage.py", "TypedStorage collapse rules (needs the engine headers, SKIP without them)"),
    ("ledger-count.py", "counting by hand: the summary line against the table body"),
]


def main():
    failed, skipped, passed = [], [], []
    for script, what in SCRIPTS:
        path = os.path.join(HERE, script)
        if not os.path.exists(path):
            skipped.append((script, "the script does not exist"))
            continue
        print("── %s —— %s" % (script, what))
        p = subprocess.run([sys.executable, path], capture_output=True, text=True)
        sys.stdout.write(p.stdout)
        if p.stderr:
            sys.stderr.write(p.stderr)
        # A SKIP is declared by the script in its own output and is not the exit code: a
        # check that skipped also exits 0, and counting that as a pass is exactly the
        # claiming-coverage-that-does-not-exist these tools exist to prevent.
        if p.returncode != 0:
            failed.append(script)
        elif re.search(r"^\s*SKIP\b", p.stdout, re.M):
            skipped.append((script, "a precondition is missing, see the output above"))
        else:
            passed.append(script)
        print()

    print("=" * 62)
    if failed:
        print("FAIL: %s" % ", ".join(failed))
        return 1
    line = "PASS: %d surrogate(s) passed" % len(passed)
    if skipped:
        line += ", %d skipped (%s)" % (
            len(skipped), "; ".join("%s: %s" % (a, b) for a, b in skipped)
        )
        line += ". A skipped check has no conclusion."
    print(line + ("." if not skipped else ""))
    print("Reminder: these are only the floor without a toolchain. The real criteria are `xmake` and `cargo clippy`.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
