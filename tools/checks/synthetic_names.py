# -*- coding: utf-8 -*-
"""synthetic-names: the SDK's list of synthetic events is the set the host registers.

What it watches: `ALL_SYNTHETIC` in the Rust binding is what a mod compares against the
host's event list at startup to learn which capability packages were built in. It is kept
by hand, while the host side is one `HookEventDef` per event under `pier-hooks`. A name in
the list that the host never registers makes that comparison report a missing package on
every host; a name the host registers and the list lacks is invisible to that comparison.

The criterion is set equality between the first string argument of every `HookEventDef`
under packages/pier-hooks/src and the entries of `ALL_SYNTHETIC`, resolved through the
`pub const NAME: &str = "..."` declarations of the same file.

What it cannot see: whether an event a hook registers actually fires, which needs a server.
"""

import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

HOOKS = os.path.join(ROOT, "packages", "pier-hooks", "src")
NAMES = os.path.join(ROOT, "bindings", "rust", "pier-rs", "src", "event", "names.rs")


def run():
    r = Result("synthetic-names")
    host = set()
    for f in glob.glob(os.path.join(HOOKS, "**", "*.cpp"), recursive=True):
        with open(f, encoding="utf-8") as fh:
            host |= set(re.findall(r'HookEventDef\s+\w+\s*\{\s*"(\w+)"', fh.read()))
    if not host:
        r.fail("no HookEventDef was found under packages/pier-hooks/src; the pattern no longer "
               "matches the source, so nothing was compared")
        return r
    with open(NAMES, encoding="utf-8") as fh:
        text = fh.read()
    m = re.search(r"pub const ALL_SYNTHETIC: &\[&str\] = &\[(.*?)\];", text, re.S)
    if not m:
        r.fail("ALL_SYNTHETIC was not found in names.rs; its shape changed")
        return r
    consts = dict(re.findall(r'pub const (\w+): &str = "([^"]+)";', text))
    sdk = set()
    for item in (x.strip() for x in m.group(1).split(",")):
        if not item:
            continue
        if item.startswith('"'):
            sdk.add(item.strip('"'))
        elif item in consts:
            sdk.add(consts[item])
        else:
            r.fail("ALL_SYNTHETIC names %s, which is not a string constant of names.rs" % item)
    for name in sorted(sdk - host):
        r.fail("ALL_SYNTHETIC lists %s, which no HookEventDef registers, so the startup "
               "comparison reports it missing on every host" % name)
    for name in sorted(host - sdk):
        r.fail("the host registers %s and ALL_SYNTHETIC lacks it" % name)
    if sdk == host:
        r.note("all %d synthetic event(s) match in both directions; whether each one fires "
               "needs a server" % len(host))
    return r


if __name__ == "__main__":
    sys.exit(run().report())
