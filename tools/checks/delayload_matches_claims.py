# -*- coding: utf-8 -*-
"""delayload-matches-claims: whatever is described as delay-loaded must really be
delay-loaded by the linker.

## Why this exists

`money_guard.h` said from day one that the LLMoney_* symbols live in a delay-loaded
LegacyMoney.dll, and every entry point in `Money.cpp` dutifully goes through
`moneyBackendReady()` first, with the runtime degradation logic complete to the line. But
`xmake.lua` never carried `/DELAYLOAD:LegacyMoney.dll`, and `add_packages` linked the
import library straight in.

The consequence is not that the economy features are unavailable, it is that pier does not
load at all: the loader cannot resolve the import table while loading pier.dll and reports
`0x7E, the specified module could not be found`. That message contains no word about
money, so tracing the symptom back to the cause means crossing the entire runtime
degradation logic, which looks completely correct and simply never got a line to run.

This is the worst form of what contract §5.4 calls a comment lying about the code: the
comment describes the design intent, the code implements half of that intent, and the
missing half lives in another file where nobody lines the two up.

## The criteria

C++ sources and headers are searched for a delay-load claim, `X.dll` is taken from the
surrounding context, and the root `xmake.lua` is then required to carry the matching
`/DELAYLOAD:X.dll`.

The reverse is checked too: a DLL listed under `/DELAYLOAD:` with no code anywhere
checking its availability at runtime is a different mismatch, since a failed delay load
raises an SEH exception on the first call and no guard turns a missing optional dependency
into a crash. That one only reports and does not fail, because the guard may sit where the
criterion cannot see it, such as a shared helper in another TU.
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

CPP_DIRS = [os.path.join(ROOT, "packages")]
CLAIM = re.compile(r"延迟加载|delay-?\s?load", re.I)
DLLNAME = re.compile(r"([A-Za-z][\w.-]*\.dll)")

# The host's own artifact cannot be a delay-load target of itself.
SELF = {"pier.dll", "pier-client.dll"}


def _claimed_dlls():
    """Maps a DLL the code describes as delay-loaded to where that claim is made."""
    out = {}
    for base in CPP_DIRS:
        for dp, _, fs in os.walk(base):
            for fn in sorted(fs):
                if not fn.endswith((".cpp", ".h", ".hpp")):
                    continue
                p = os.path.join(dp, fn)
                lines = open(p, encoding="utf-8", errors="replace").read().split("\n")
                for i, line in enumerate(lines):
                    if not CLAIM.search(line):
                        continue
                    # The DLL name may sit on this line or on either adjacent line.
                    #
                    # The host itself is excluded: explaining that pier.dll fails to load
                    # describes a consequence and is not a claim that pier.dll is
                    # delay-loaded. A check mistaking a consequence for a claim reports a
                    # false positive on its first run, and a false positive is exactly what
                    # gets a check added to an ignore list.
                    window = " ".join(lines[max(0, i - 1) : i + 2])
                    for m in DLLNAME.finditer(window):
                        dll = m.group(1)
                        if dll.lower() in SELF:
                            continue
                        out.setdefault(dll, []).append(
                            "%s:%d" % (os.path.relpath(p, ROOT), i + 1)
                        )
    return out


def run():
    r = Result("delayload-matches-claims")
    xm = os.path.join(ROOT, "xmake.lua")
    if not os.path.exists(xm):
        r.fail("the root xmake.lua does not exist")
        return r
    build = open(xm, encoding="utf-8").read()
    flagged = {m.group(1) for m in re.finditer(r"/DELAYLOAD:([\w.-]+\.dll)", build)}

    claimed = _claimed_dlls()
    for dll, where in sorted(claimed.items()):
        if dll in flagged:
            continue
        r.fail(
            "%s is described in the code as delay-loaded (%s) while the root xmake.lua carries "
            "no /DELAYLOAD:%s. The import library is linked in statically, the host fails while "
            "loading itself and reports `0x7E, the specified module could not be found`, and "
            "not one line of the runtime degradation logic runs"
            % (dll, where[0], dll)
        )

    if flagged and "delayimp" not in build:
        r.fail(
            "/DELAYLOAD is used without linking delayimp, so the linker reports __delayLoadHelper2 as undefined"
        )

    # /DELAYLOAD has to land in the right flag channel. In xmake `add_ldflags` applies only
    # to `kind("binary")` while a shared library reads `add_shflags`. The wrong channel
    # raises no error and stops no build: the line is silently ignored, the artifact still
    # carries a hard DLL dependency, and the symptom is identical to never adding it. This
    # criterion exists for that failure mode, because the previous fix for this bug was
    # written as ldflags and a rebuild produced exactly the same error.
    for m in re.finditer(r'target\("(\w+)"\)(.*?)target_end\(\)', build, re.S):
        name, body = m.group(1), m.group(2)
        if "/DELAYLOAD:" not in body:
            continue
        shared = 'set_kind("shared")' in body
        has_sh = re.search(r"add_shflags\([^)]*DELAYLOAD", body)
        has_ld = re.search(r"add_ldflags\([^)]*DELAYLOAD", body)
        if shared and not has_sh:
            r.fail(
                "target `%s` is shared while /DELAYLOAD appears only in add_ldflags. xmake reads "
                "add_shflags for a shared library, so that line is silently ignored, the build "
                "succeeds as usual and the artifact still carries a hard DLL dependency" % name
            )
        if not shared and not has_ld:
            r.fail(
                "target `%s` is not shared while /DELAYLOAD appears only in add_shflags" % name
            )

    for dll in sorted(flagged):
        stem = dll[:-4]
        guarded = False
        for base in CPP_DIRS:
            for dp, _, fs in os.walk(base):
                for fn in fs:
                    if not fn.endswith((".cpp", ".h", ".hpp")):
                        continue
                    t = open(
                        os.path.join(dp, fn), encoding="utf-8", errors="replace"
                    ).read()
                    if stem in t and re.search(r"Ready\(\)|available\(\)|resolve\(", t):
                        guarded = True
        if not guarded:
            r.note(
                "%s declares delay loading while no runtime availability guard was found. A "
                "failed delay load raises SEH on the first call, so no guard turns a missing "
                "optional dependency into a crash. The criterion cannot see a shared helper in "
                "another TU, so this is a reminder only" % dll
            )

    if not r.failures:
        r.note(
            "%d DLL(s) claimed as delay-loaded against %d linker flag(s), matching one to one. "
            "The criterion compares names only and cannot guarantee the guard really covers "
            "every entry point."
            % (len(claimed), len(flagged))
        )
    return r


if __name__ == "__main__":
    sys.exit(run().report())
