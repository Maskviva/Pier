#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""build-prereqs: what has to hold before a build starts.

## Why this is a script of its own

The modpacker of `levibuildscript` reads git for a version number at the packaging
stage. With a working directory that is not a git repository, or an empty one with no
commit, it reports:

    error: fatal: Not a valid object name HEAD

That error appears at 90%, meaning after all 98 TUs have compiled, prelink has run and
`pier.dll` is linked. A ten-minute build is stopped at its last step by a precondition
that takes a second to check.

That is the whole reason this is a separate script: a precondition is checked first. It
could live in `run-checks.py`, but that suite answers whether the code is right while
this one answers whether the environment works, and the two need entirely different
responses on failure, editing code versus typing one command.

## Position: a surrogate

xmake reports it eventually, late and vaguely. By the criterion of contract §9 it does
not belong in that table.

Usage:
    python3 tools/build-prereqs.py
"""

import os
import subprocess
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))


def git(*args):
    try:
        p = subprocess.run(
            ["git", "-C", ROOT] + list(args),
            capture_output=True, text=True, timeout=15,
        )
        return p.returncode, p.stdout.strip(), p.stderr.strip()
    except (OSError, subprocess.SubprocessError) as e:  # noqa: BLE001
        return 127, "", str(e)


def main():
    problems = []
    notes = []

    rc, _, _ = git("--version")
    if rc == 127:
        problems.append("git was not found. The modpacker of levibuildscript needs it for the version number.")
        _report(problems, notes)
        return 1

    rc, out, _ = git("rev-parse", "--is-inside-work-tree")
    if rc != 0 or out != "true":
        problems.append(
            "%s is not a git working tree. The modpacker reports\n"
            "      `fatal: Not a valid object name HEAD` at the packaging stage, around 90%%,\n"
            "      by which point all 98 TUs have already compiled.\n"
            "      Fix:\n"
            "        git init\n"
            "        git add -A\n"
            "        git commit -m \"pier v1\"\n"
            "        git tag v1.0.0        # optional, but the version number reads much better" % ROOT
        )
        _report(problems, notes)
        return 1

    rc, out, err = git("rev-parse", "HEAD")
    if rc != 0:
        problems.append(
            "The git repository exists with no commit at all: `git rev-parse HEAD` reports %r.\n"
            "      That is exactly the `fatal: Not a valid object name HEAD` seen on a machine.\n"
            "      Fix:\n"
            "        git add -A\n"
            "        git commit -m \"pier v1\"\n"
            "        git tag v1.0.0        # optional" % (err.splitlines()[0] if err else "failed")
        )
        _report(problems, notes)
        return 1
    notes.append("HEAD = %s" % out[:12])

    rc, out, _ = git("describe", "--tags", "--always", "--dirty")
    if rc == 0:
        notes.append("git describe = %s" % out)
    rc, out, _ = git("tag", "--list")
    if rc == 0 and not out:
        notes.append("There is no tag. Packaging works, but the version number degrades to a "
                     "commit hash. One `git tag v1.0.0` fixes it.")

    rc, out, _ = git("status", "--porcelain")
    if rc == 0 and out:
        n = len(out.splitlines())
        notes.append("The working tree has %d uncommitted change(s), so the packaged version number carries `-dirty`." % n)

    _report(problems, notes)
    return 0


def _report(problems, notes):
    for n in notes:
        print("  · %s" % n)
    for p in problems:
        print("  ✗ %s" % p)
    if not problems:
        print("  Build preconditions are satisfied.")


if __name__ == "__main__":
    sys.exit(main())
