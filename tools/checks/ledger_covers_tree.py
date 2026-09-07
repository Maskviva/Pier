# -*- coding: utf-8 -*-
"""ledger-covers-tree: every file in the workspace needs a row in the ledger.

What it watches: `MIGRATION.md` is the criterion for capabilities only ever increasing,
and it is a criterion only while it is complete in both directions:

  * ledger to workspace: an outstanding row means that capability is absent right now,
    and `ledger-count` handles the counting;
  * workspace to ledger: a file on disk that is not in the ledger has never been counted
    at all.

Nobody guarded the second direction before. The way it was found is ugly: all three
`Cargo.toml` files declare `license = "Apache-2.0"` while `LICENSE` itself never came
across, and the ledger did not even have that row, so counting it line by line a hundred
times would not have found it. A checklist that is missing an item can never reveal that
item.

## The criterion

Every file on disk under the workspace root, minus the two exemption lists below, is
either mentioned in the new-location column of some ledger row or fails. The walk is the
filesystem and not git: nothing here reads `.gitignore` or asks git what is tracked, so
an untracked stray at the root is caught, which is the point, and a build artifact is
caught too, which is not. That is why the exemption lists exist and why they have to be
kept level with `.gitignore` by hand. Each entry below says which line of `.gitignore` it
answers.

The reverse is not checked: a row in the ledger with nothing in the workspace is the
definition of outstanding.
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

LEDGER = os.path.join(ROOT, "MIGRATION.md")

# `bin` is where the mod packer writes and `.idea` / `.vs` / `.vscode` are IDE state. Neither
# is a migrated capability, and the same reasoning already exempts `build` and `target`.
SKIP_DIRS = {".git", "target", "build", "bin", ".xmake", "node_modules", "__pycache__",
             ".idea", ".vs", ".vscode"}

# Exempt: these files inherently do not belong in an old-repository to new-repository
# migration ledger.
EXEMPT_EXACT = {
    "MIGRATION.md",       # The ledger does not count itself
    ".gitignore",
    "Cargo.lock",
    "docs/pnpm-lock.yaml",  # `.gitignore` line: a lockfile the docs site resolves, not a migrated capability
}
EXEMPT_PREFIX = (
    "tools/",             # The checks and surrogates came with the new architecture and have no old counterpart
    "docs/.vitepress/dist/",   # `.gitignore` line: what `vitepress build` writes, from sources already in the ledger
    "docs/.vitepress/cache/",  # `.gitignore` line: what `vitepress dev`/`build` writes for its own dependency cache
)


def run():
    r = Result("ledger-covers-tree")
    if not os.path.exists(LEDGER):
        r.fail("MIGRATION.md was not found")
        return r
    with open(LEDGER, encoding="utf-8") as f:
        text = f.read()

    # Every path the ledger mentions. Anything in backticks counts, regardless of column:
    # a file named anywhere in the ledger has been counted.
    mentioned = set(re.findall(r"`([^`]+)`", text))
    mentioned = {m.strip().rstrip("/") for m in mentioned}

    missing = []
    n = 0
    for dp, dirs, fs in os.walk(ROOT):
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS]
        for fn in sorted(fs):
            p = os.path.join(dp, fn)
            rel = os.path.relpath(p, ROOT).replace(os.sep, "/")
            if rel in EXEMPT_EXACT or rel.startswith(EXEMPT_PREFIX):
                continue
            n += 1
            if rel in mentioned:
                continue
            # A directory form counts too, since the ledger sometimes records by directory
            if any(rel.startswith(m + "/") for m in mentioned if "/" in m or "." not in m):
                continue
            missing.append(rel)

    for rel in missing:
        r.fail("%s is in the workspace and is never named in the ledger, so it has never been "
               "counted. Checking that capabilities only increase against a checklist that is "
               "missing an item can never reveal that item" % rel)
    if not missing:
        r.note("all %d file(s) have a source in the ledger. Exempt are %s and the prefixes %s, "
               "each kept level with .gitignore by hand; an artifact .gitignore knows and this "
               "list does not will fail here"
               % (n, ", ".join(sorted(EXEMPT_EXACT)), ", ".join(sorted(EXEMPT_PREFIX))))
    return r


if __name__ == "__main__":
    sys.exit(run().report())
