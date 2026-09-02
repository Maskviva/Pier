# -*- coding: utf-8 -*-
"""abi-additive: `PierApi` may only be appended to (contract §2.2).

What it watches: reordering or deleting a slot makes every already-compiled mod call the
wrong function pointer from that slot onward, so a call to `bus_publish` lands somewhere
else with no diagnostic at all. The compiler cannot see this, since both sides compile on
their own and the mismatch surfaces only at runtime.

The comparison runs against `tools/abi-v1.slots`, the baseline snapshot committed with the
ABI. There is one criterion: the baseline is a prefix of the current slot order.
Appending does not change the version, and once that criterion breaks,
`PIER_ABI_VERSION` and `PIER_ABI_MIN_SUPPORTED` must advance together, which this check
verifies as well.

When to refresh the baseline: only when the version really advanced, by rewriting the
snapshot with `--bless`. Otherwise it is a read-only fact.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result, defines, read_abi, slots_of  # noqa: E402

LOCK = os.path.join(ROOT, "tools", "abi-v1.slots")


def _load_lock():
    if not os.path.exists(LOCK):
        return None
    out = {"version": None, "min": None, "slots": []}
    with open(LOCK, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            if line.startswith("!version "):
                out["version"] = line.split()[1]
            elif line.startswith("!min "):
                out["min"] = line.split()[1]
            else:
                out["slots"].append(line)
    return out


def _write_lock(slots, ver, mn):
    with open(LOCK, "w", encoding="utf-8") as f:
        f.write("# The PierApi slot order baseline, what the abi-additive check compares against.\n")
        f.write("# Rewriting it with --bless is allowed only when PIER_ABI_VERSION advances.\n")
        f.write("!version %s\n!min %s\n" % (ver, mn))
        for s in slots:
            f.write(s + "\n")


def run(bless=False):
    r = Result("abi-additive")
    src = read_abi()
    slots = slots_of(src, "PierApi")
    d = defines(src)
    ver = d.get("PIER_ABI_VERSION", "?").rstrip("u")
    mn = d.get("PIER_ABI_MIN_SUPPORTED", "?").rstrip("u")

    if ver != mn:
        r.note("VERSION=%s MIN_SUPPORTED=%s, a compatible range, which is valid" % (ver, mn))

    lock = _load_lock()
    if lock is None:
        if bless:
            _write_lock(slots, ver, mn)
            r.note("no baseline existed, so %d slot(s) were written (v%s)" % (len(slots), ver))
            return r
        r.fail("the baseline tools/abi-v1.slots does not exist; run --bless once to fix v1 in place")
        return r

    if bless:
        _write_lock(slots, ver, mn)
        r.note("the baseline was rewritten with %d slot(s) (v%s)" % (len(slots), ver))
        return r

    base = lock["slots"]
    r.note("baseline %d slot(s) (v%s) against %d current slot(s) (v%s)" % (len(base), lock["version"], len(slots), ver))

    if len(slots) < len(base):
        r.fail("the slot count fell from %d to %d; a deletion is not an append" % (len(base), len(slots)))

    n = min(len(base), len(slots))
    for i in range(n):
        if base[i] != slots[i]:
            r.fail("slot %d changed from %r to %r, which is a reorder or a replacement and not an append" % (i, base[i], slots[i]))

    if r.failures:
        # A non-append change is allowed, but both version numbers must advance together,
        # otherwise an old mod loads an already-misaligned table under a version number
        # that looks compatible.
        if ver == lock["version"]:
            r.fail(
                "the change above is not an append while PIER_ABI_VERSION still reads %s; "
                "contract §2.2 requires VERSION and MIN_SUPPORTED to advance together to the "
                "same number" % ver
            )
        elif ver != mn:
            r.fail("on a non-append change MIN_SUPPORTED(%s) must equal VERSION(%s)" % (mn, ver))
    elif len(slots) > len(base):
        r.note("%d slot(s) appended, so the version stays where the contract says, which is correct" % (len(slots) - len(base)))
        if ver != lock["version"]:
            r.fail("an append alone moved the version (%s to %s), which announces an incompatibility that does not exist"
                   % (lock["version"], ver))

    return r


if __name__ == "__main__":
    sys.exit(run(bless="--bless" in sys.argv).report())
