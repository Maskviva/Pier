# -*- coding: utf-8 -*-
"""hook-install-once: a detour is installed from exactly one place.

What it watches: `hook()` of an LL hook definition registers the detour with the loader on
every call, and each `HookEventDef` runs its installer once, behind its own `installed`
flag. Two event definitions that ride one detour therefore ran `hook()` on it twice, and the
second event subscribed was refused with the message that the native detour failed to
install, while the first kept working. The ride events and the pressure plate events were
written that way, and it showed only on a server where both events of a pair were
subscribed.

The criterion is textual: in each source file under packages/pier-hooks/src, a call of the
form `Name::hook()` appears for a given `Name` at most once, comments excluded. Events that
share detours install them from one function that both definitions call, which leaves one
call site.

What it cannot see: a detour installed from two files, a call through a pointer or an
alias, and an installer that guards itself in some other way while still naming
`Name::hook()` twice.
"""

import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

HOOKS = os.path.join(ROOT, "packages", "pier-hooks", "src")
CALL = re.compile(r"\b(\w+)::hook\(\s*\)")
COMMENT = re.compile(r"/\*.*?\*/|//[^\n]*", re.S)


def run():
    r = Result("hook-install-once")
    files = sorted(glob.glob(os.path.join(HOOKS, "**", "*.cpp"), recursive=True))
    if not files:
        r.fail("no .cpp file was found under packages/pier-hooks/src, so nothing was compared")
        return r
    calls = 0
    for path in files:
        with open(path, encoding="utf-8") as fh:
            body = COMMENT.sub("", fh.read())
        seen = {}
        for name in CALL.findall(body):
            seen[name] = seen.get(name, 0) + 1
        calls += sum(seen.values())
        for name, count in sorted(seen.items()):
            if count > 1:
                r.fail("%s calls %s::hook() %d times; the second call registers the same "
                       "detour again. Install it from one function that every event "
                       "definition sharing it calls" % (os.path.relpath(path, ROOT), name, count))
    if calls == 0:
        r.fail("no `Name::hook()` call was found under packages/pier-hooks/src; the pattern "
               "no longer matches the source, so nothing was compared")
    elif not r.failures:
        r.note("%d detour install call(s) in %d file(s), each named once per file; whether "
               "an installer runs once at run time needs a server" % (calls, len(files)))
    return r


if __name__ == "__main__":
    sys.exit(run().report())
