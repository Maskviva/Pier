# -*- coding: utf-8 -*-
"""abi-values: a PIER_* constant or enum member keeps its value once released.

What it watches: `abi-additive` guards the order of the slots, and nothing guarded the
numbers. A constant renumbered in abi.h and in every mirror at once passes every other
check, while each mod already built still sends the old number and now means something
else. The baseline `tools/abi-v2.values` records every name with its value; a name may be
added, and one that is there may neither disappear nor change value until
PIER_ABI_VERSION advances, when the baseline is rewritten with --bless.

What it cannot see: a value whose meaning changed while the number stayed, which is a
change of semantics that only the prose of abi.h can carry.
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result, defines, read_abi  # noqa: E402

LOCK = os.path.join(ROOT, "tools", "abi-v2.values")


def current(src):
    out = {}
    for k, v in defines(src).items():
        if k.startswith("PIER_") and re.match(r"^-?(0x[0-9a-fA-F]+|\d+)u?$", v.strip()):
            out[k] = v.strip().rstrip("u")
    for m in re.finditer(r"enum\s+\w+\s*\{(.*?)\n\}\s*\w*\s*;", src, re.S):
        body = re.sub(r"/\*.*?\*/", "", m.group(1), flags=re.S)
        body = re.sub(r"//[^\n]*", "", body)
        for item in body.split(","):
            k, _, v = item.strip().partition("=")
            k, v = k.strip(), v.strip()
            if re.match(r"^PIER_\w+$", k) and v:
                out[k] = v
    return out


def run(bless=False):
    r = Result("abi-values")
    src = read_abi()
    now = current(src)
    ver = defines(src).get("PIER_ABI_VERSION", "?").rstrip("u")
    if bless or not os.path.exists(LOCK):
        with open(LOCK, "w", encoding="utf-8") as f:
            f.write("# Every PIER_* constant and enum member of abi.h with its value, the baseline\n")
            f.write("# abi-values compares against. Rewrite it with --bless only when\n")
            f.write("# PIER_ABI_VERSION advances.\n!version %s\n" % ver)
            for k in sorted(now):
                f.write("%s=%s\n" % (k, now[k]))
        r.note("wrote the baseline with %d value(s) at ABI v%s" % (len(now), ver))
        return r
    base, base_ver = {}, None
    with open(LOCK, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            if line.startswith("!version "):
                base_ver = line.split()[1]
                continue
            k, _, v = line.partition("=")
            base[k] = v
    if base_ver != ver:
        r.fail("the baseline is for ABI v%s and abi.h says v%s; after a version change run "
               "this script with --bless" % (base_ver, ver))
        return r
    bad = 0
    for k, v in sorted(base.items()):
        if k not in now:
            r.fail("%s was released and is gone from abi.h; a mod built against it still sends "
                   "%s" % (k, v))
            bad += 1
        elif now[k] != v:
            r.fail("%s was released as %s and abi.h now says %s; every mod already built still "
                   "means %s" % (k, v, now[k], v))
            bad += 1
    if not bad:
        r.note("all %d released value(s) are unchanged, %d added since the baseline; what a "
               "value means is not checked" % (len(base), len(set(now) - set(base))))
    return r


if __name__ == "__main__":
    sys.exit(run("--bless" in sys.argv).report())
