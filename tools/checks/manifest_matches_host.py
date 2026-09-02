# -*- coding: utf-8 -*-
"""manifest-matches-host: the `manifest.json` of an example mod must really load under the
host.

What it watches: the host recognizes only a mod whose `"type"` equals `<ModHostName>`,
compared literally on one line of `ModControl.cpp`, and `ModHostName` is defined in
`hosted_mod.h`. One wrong character in that string in an example and the mod is never
scanned at all: no error, no log line, it is simply absent from `/pier list`.

This is not hypothetical. The v0 example manifest depended on `"levilamina-rust-loader"`,
a mod name that no longer existed after the rename, so the example could not load. It was
found by reading file by file during an architecture review, which is exactly the class
the compiler cannot find.

Three criteria:

1. `type` equals the host's `ModHostName`;
2. the host name inside `dependencies` equals it as well, since a wrong one depends on a
   mod that does not exist;
3. `entry` agrees with the crate name: cargo produces `<crate_name>.dll` and a `-` in a
   crate name becomes `_`, which is the easiest step to slip on.
"""

import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

HOSTED_MOD_H = os.path.join(
    ROOT, "packages", "pier-host", "include", "pier", "host", "hosted_mod.h"
)


def run():
    r = Result("manifest-matches-host")

    if not os.path.exists(HOSTED_MOD_H):
        r.fail("hosted_mod.h was not found, so which type the host recognizes cannot be determined")
        return r
    with open(HOSTED_MOD_H, encoding="utf-8") as f:
        m = re.search(r'ModHostName\s*=\s*"([^"]+)"', f.read())
    if not m:
        r.fail("the definition of ModHostName was not found in hosted_mod.h")
        return r
    host = m.group(1)
    r.note("the mod type name the host registers = %r" % host)

    # This may run against another repository: a consumer of Pier, meaning a mod, is bound
    # by the same equality while having no hosted_mod.h of its own. The host name is read
    # from this repository and the manifests are scanned over there.
    scan_root = sys.argv[1] if len(sys.argv) > 1 else ROOT

    found = 0
    for dp, dirs, fs in os.walk(scan_root):
        dirs[:] = [d for d in dirs if d not in (".git", "target", "node_modules")]
        if "manifest.json" not in fs:
            continue
        p = os.path.join(dp, "manifest.json")
        rel = os.path.relpath(p, scan_root)
        # The manifest.json at the repository root is the LeviLamina manifest of the pier
        # host itself, with type "native", and not a pier mod. Scanning it would always
        # produce a false failure.
        if os.path.abspath(dp) == os.path.abspath(scan_root):
            continue
        found += 1
        try:
            with open(p, encoding="utf-8") as f:
                j = json.load(f)
        except Exception as e:  # noqa: BLE001
            r.fail("%s is not valid JSON: %s" % (rel, e))
            continue

        if j.get("type") != host:
            r.fail("the type of %s is %r while the host recognizes only %r, so this mod is never "
                   "scanned and nothing is reported" % (rel, j.get("type"), host))

        deps = [d.get("name") for d in j.get("dependencies", []) if isinstance(d, dict)]
        if host not in deps:
            r.fail("the dependencies of %s do not include %r, so the load order cannot be "
                   "guaranteed; they currently are %s" % (rel, host, deps))
        for d in deps:
            if d and d != host and d.startswith(("levilamina-", "pier-")):
                r.fail("%s depends on %r, which is not the host name and is most likely a rename left half done" % (rel, d))

        # entry against the crate name
        cargo = os.path.join(dp, "Cargo.toml")
        if os.path.exists(cargo):
            with open(cargo, encoding="utf-8") as f:
                ct = f.read()
            lib = re.search(r"^\[lib\](.*?)(?=^\[|\Z)", ct, re.S | re.M)
            name = None
            if lib:
                mm = re.search(r'^\s*name\s*=\s*"([^"]+)"', lib.group(1), re.M)
                if mm:
                    name = mm.group(1)
            if name is None:
                mm = re.search(r'^\s*name\s*=\s*"([^"]+)"', ct, re.M)
                name = mm.group(1) if mm else None
            if name:
                want = name.replace("-", "_") + ".dll"
                if j.get("entry") != want:
                    r.fail("the entry of %s is %r while cargo produces %r; a - in a crate name "
                           "becomes _" % (rel, j.get("entry"), want))

    if found == 0:
        r.note("the repository has no manifest.json, so there is no example mod to check")
    elif not r.failures:
        r.note("all %d manifest(s) load under the host" % found)
    return r


if __name__ == "__main__":
    sys.exit(run().report())
