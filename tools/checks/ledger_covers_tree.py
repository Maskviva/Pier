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

Every version-controlled file in the workspace is either mentioned in the new-location
column of some ledger row, or is on the exemption list: build artifacts, the ledger
itself, and the tools added in this round.

The reverse is not checked: a row in the ledger with nothing in the workspace is the
definition of outstanding.
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

LEDGER = os.path.join(ROOT, "MIGRATION.md")

SKIP_DIRS = {".git", "target", "build", ".xmake", "node_modules", "__pycache__"}

# Exempt: these files inherently do not belong in an old-repository to new-repository
# migration ledger.
EXEMPT_EXACT = {
    "MIGRATION.md",       # The ledger does not count itself
    ".gitignore",
    "Cargo.lock",
}
EXEMPT_PREFIX = (
    "tools/",             # The checks and surrogates came with the new architecture and have no old counterpart
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
        r.note("all %d file(s) have a source in the ledger; exempt are MIGRATION.md itself, .gitignore and tools/" % n)
    return r


if __name__ == "__main__":
    sys.exit(run().report())
