#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Runs every machine check of contract §9. Any red exits non-zero.

Usage:
    python3 tools/run-checks.py            # everything
    python3 tools/run-checks.py abi        # only names containing "abi"

## What this script proves and what it does not

It does prove that each property listed below holds over the text of the current
workspace.

It does not build anything. Three of the properties have a sufficient criterion that
needs a real toolchain, and the script covers only their necessary condition:

  - `optional-drops`  the static criterion is that no symbol of an optional package is
                      referenced across packages; the sufficient one is really deleting
                      that `includes(...)` line and running `xmake f`.
  - `sys-mirrors-abi` compares the text of the slot order and the constants; whether the
                      mirror actually compiles needs `cargo check`.
  - `no-silent-fallback` covers only the unambiguous shapes inside a `catch` block, as
                      that script explains itself.

The surrogates are not here either. They live under `tools/` because §9 says they are not
part of the contract, and the ones duplicating the compiler and the linker have been
removed now that a toolchain exists.

Contract §9 says scripts come first: a property gets no checkmark in a delivery note
before a script guards it. One equally important addition: a passing script only earns a
checkmark for what it covers, and each check states its coverage in its own note.
"""

import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CHECKS = os.path.join(HERE, "checks")

# The order is deliberate: first the contract itself in abi.h, then the package
# structure, then code discipline.
SCRIPTS = [
    ("abi_c_parse.py", ["abi-c-parse"]),
    ("abi_additive.py", ["abi-additive"]),
    ("abi_no_lang.py", ["abi-no-lang"]),
    ("abi_fixed_width.py", ["abi-fixed-width"]),
    ("pkg_layering.py", ["pkg-layering", "object-kind", "optional-drops"]),
    ("build_config.py", ["build-config"]),
    ("include_resolves.py", ["include-resolves"]),
    ("sys_mirrors_abi.py", ["sys-mirrors-abi"]),
    ("rust_layering.py", ["rust-layering"]),
    ("rust_comment_budget.py", ["rust-comment-budget"]),
    ("delayload_matches_claims.py", ["delayload-matches-claims"]),
    ("manifest_matches_host.py", ["manifest-matches-host"]),
    ("host_loadable.py", ["host-loadable"]),
    ("ledger_covers_tree.py", ["ledger-covers-tree"]),
    ("prose_and_fallback.py", ["comment-claims", "no-silent-fallback"]),
    ("comment_style.py", ["comment-style"]),
]


def main():
    want = sys.argv[1] if len(sys.argv) > 1 else None
    failed = []
    ran = 0
    for script, names in SCRIPTS:
        if want and not any(want in n for n in names) and want not in script:
            continue
        path = os.path.join(CHECKS, script)
        if not os.path.exists(path):
            print("  SKIP %s (the script is not written yet)" % "/".join(names))
            continue
        print("── %s" % " + ".join(names))
        p = subprocess.run([sys.executable, path])
        ran += 1
        if p.returncode != 0:
            failed.extend(names)
        print()

    print("=" * 62)
    if failed:
        print("FAIL: %d check(s) did not pass: %s" % (len(failed), ", ".join(failed)))
        return 1
    print("PASS: all %d scripts passed." % ran)
    print("Reminder: optional-drops passed only its static necessary condition, namely that "
          "no optional-package symbol is referenced across packages;")
    print("      a real xmake f and cargo clippy still have to run before delivery.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
