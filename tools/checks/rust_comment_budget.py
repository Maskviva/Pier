# -*- coding: utf-8 -*-
"""rust-comment-budget: the hard budget of COMMENTS.md §1, applied to Rust.

`comment_style.py` scans only `.cpp/.h/.hpp`, so the comments on the bindings side had
never been measured. The first measurement found 18 blocks over budget, the longest a file
header of 43 lines, and §1 is explicit: over budget does not mean written in detail, it
means put at the wrong level. Design goes in CONTRACT.md, history goes in git, and a
pending item goes in an issue.

The three levels map onto Rust as:

    L1  consecutive `//!` at the start of a file    at most 16 lines
    L2  consecutive `///` above a declaration       at most 14 lines
    L3  consecutive `//` above a statement          at most 8 lines

This check measures length and not content. It passes 15 lines of nothing and stops 17
lines of substance. All it can do is force an answer to whether these words belong here,
and the answer is still a human's.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

ROOTS = [os.path.join(ROOT, "bindings", "rust")]
BUDGET = {"L1 file header": 16, "L2 declaration comment": 14, "L3 body comment": 8}


def _scan(path):
    """Returns [(level, start line, line count)]."""
    lines = open(path, encoding="utf-8").read().split("\n")
    out = []

    i = 0
    while i < len(lines):
        s = lines[i].strip()
        nxt = lines[i + 1].strip() if i + 1 < len(lines) else ""
        if s.startswith("//!") or (s == "" and nxt.startswith("//!") and i > 0):
            i += 1
        else:
            break
    if i > BUDGET["L1 file header"]:
        out.append(("L1 file header", 1, i))

    for level, prefix in (("L2 declaration comment", "///"), ("L3 body comment", "//")):
        run = 0
        for j, l in enumerate(lines):
            s = l.strip()
            hit = (
                s.startswith("///")
                if prefix == "///"
                else (s.startswith("//") and not s.startswith(("///", "//!")))
            )
            if hit:
                run += 1
            else:
                if run > BUDGET[level]:
                    out.append((level, j - run + 1, run))
                run = 0
        if run > BUDGET[level]:
            out.append((level, len(lines) - run + 1, run))
    return out


def run():
    r = Result("rust-comment-budget")
    files = 0
    bad = []
    for root in ROOTS:
        if not os.path.isdir(root):
            continue
        for dp, _, fs in os.walk(root):
            if "target" in dp.split(os.sep):
                continue
            for fn in sorted(fs):
                if not fn.endswith(".rs"):
                    continue
                p = os.path.join(dp, fn)
                files += 1
                for level, line, n in _scan(p):
                    bad.append((os.path.relpath(p, ROOT), line, level, n))

    for rel, line, level, n in sorted(bad, key=lambda x: -x[3]):
        r.fail(
            "%s:%d %s is %d lines, over the %d-line budget. Over budget does not mean written "
            "in detail, it means put at the wrong level: design in CONTRACT.md, history in git, "
            "pending items in an issue"
            % (rel, line, level, n, BUDGET[level])
        )

    if not r.failures:
        r.note(
            "scanned %d .rs file(s). The criterion measures length and not content: it passes "
            "15 lines of nothing and stops 17 lines of substance. It forces an answer to whether "
            "these words belong here, and the answer is still a human's."
            % files
        )
    return r


if __name__ == "__main__":
    sys.exit(run().report())
