#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ledger-count: derives the summary line from the body of the MIGRATION.md table.

Why this script exists: that summary line once read 75 done while counting the body row
by row gave 70, and the delivery note had to add that the body is authoritative. A number
that needs a footnote to be read is a bad number.

It also breaks the outstanding rows down by area. That came out of a real false claim: a
delivery note said the C++ side was complete three rounds running while
`packages/pier-host/src/MemoryOperators.cpp` was still outstanding in the ledger, and
that file is load-blocking, since LeviLamina refuses to load without it. The data was in
the table the whole time and nobody summed it by area, so the false claim went unchallenged
three times.

A count that reports only a total cannot stop a per-area assertion such as one area being
finished. Every run now prints a per-area table, and a delivery note copies it rather than
going by impression.

The summary line is derived, not a fact. The facts are the per-row statuses in the body.
Hence:

    python3 tools/ledger-count.py         # report only, exit non-zero on a mismatch
    python3 tools/ledger-count.py --fix   # rewrite the summary line from the body

It is also a weak check: done plus cut plus outstanding must equal the number of body
rows. A mismatch means some row has a malformed status field, such as a full-width space,
and such a row is skipped when counting by eye.
"""

import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
LEDGER = os.path.join(ROOT, "MIGRATION.md")

ROW = re.compile(r"^\|\s*`([^`]+)`\s*\|(.*)\|(.*)\|\s*$")
SUMMARY = re.compile(r"^\*\*Summary\*\*:.*$", re.M)


def main():
    fix = "--fix" in sys.argv
    with open(LEDGER, encoding="utf-8") as f:
        text = f.read()

    # Only the body above the summary line is counted, since the summary sums the table
    # directly above it. The file may hold other tables further down, such as a record of
    # fixes made on a machine, and those are not part of the ledger. Without this cut,
    # adding an unrelated table makes the count report an invalid status field, and a
    # check that turns red on unrelated changes ends up being worked around.
    cut_at = text.find("**Summary**:")
    body = text[:cut_at] if cut_at >= 0 else text

    done = cut = todo = other = 0
    rows = 0
    bad = []
    for line in body.splitlines():
        m = ROW.match(line)
        if not m:
            continue
        status = m.group(3).strip()
        if status.startswith("---") or not status:
            continue
        rows += 1
        if status.startswith("✔"):
            done += 1
        elif status.startswith("✂"):
            cut += 1
        elif status.startswith("⬜"):
            todo += 1
        else:
            other += 1
            bad.append((m.group(1), status[:40]))

    total = done + cut + todo
    print("  body %d row(s): done %d | cut %d | outstanding %d" % (rows, done, cut, todo))
    if other:
        print("  FAIL %d row(s) carry a status that is none of the three glyphs, and such a row is skipped when counting by eye:" % other)
        for name, st in bad[:8]:
            print("      %s → %r" % (name, st))
        return 1

    # Break the outstanding rows down by area
    areas = {}
    for line in body.splitlines():
        m = ROW.match(line)
        if not m or not m.group(3).strip().startswith("⬜"):
            continue
        path = m.group(1)
        parts = path.split("/")
        if path.startswith("packages/"):
            area = "/".join(parts[:2])
        elif "/" in path:
            area = parts[0] + "/"
        else:
            area = "(root)"
        areas[area] = areas.get(area, 0) + 1
    if areas:
        print("  outstanding by area:")
        for area, k in sorted(areas.items(), key=lambda kv: -kv[1]):
            print("      %-28s %d" % (area, k))
        cpp = [a for a in areas if a.startswith("packages/pier-")
               and not a.endswith(("-rs", "-sys-rs"))]
        if cpp:
            print("      WARNING the C++ side still has outstanding rows: %s. A delivery note must not claim the C++ side is complete."
                  % ", ".join(sorted(cpp)))
        else:
            print("      the C++ side, all eight packages, has nothing outstanding")
    else:
        print("  nothing outstanding.")

    want = "**Summary**: ✔ %d | ✂ %d | ⬜ %d (%d old files in total)" % (done, cut, todo, total)
    m = SUMMARY.search(text)
    if not m:
        print("  FAIL the summary line was not found")
        return 1
    if m.group(0) == want:
        print("  OK the summary line agrees with the body")
        return 0

    print("  FAIL the summary line disagrees with the body")
    print("      as written:     %s" % m.group(0))
    print("      from the body:  %s" % want)
    if not fix:
        print("      use --fix to rewrite it from the body.")
        return 1
    with open(LEDGER, "w", encoding="utf-8") as f:
        f.write(SUMMARY.sub(want.replace("\\", "\\\\"), text, count=1))
    print("      rewritten.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
