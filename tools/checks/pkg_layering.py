# -*- coding: utf-8 -*-
"""pkg-layering, object-kind and optional-drops: the three packaging rules of contract §1.

The three share one file because they read the same facts, the xmake.lua of each package
and the #include lines of each source file, and differ only in criteria. Three files would
each write their own directory walk and then drift apart.

Contract §1 rule 1: arrows point upward only, and that graph is the complete set of edges.
Both directions are checked, because they can lie independently of each other:

  - the edges declared by `add_deps`, the link graph
  - the edges actually used by `#include`, the compile graph

In v1 the two graphs disagreed: the contract drew a DAG while reality held a cycle between
api and hooks plus a hidden link edge from host to api. Checking `add_deps` one by one
checks the declaration and not the reality.

Contract §1 rule 4: every package is `set_kind("object")`. A static library lets the
linker discard a translation unit with no external symbol reference, while SPI
registration rests entirely on file-level static objects, and the symptom of a discarded
one is a capability disappearing silently. This check also verifies whether a package
really enters the artifact only through self-registration, so the failure message can
state the consequence.

Contract §1 rule 3: deleting `includes(<optional package>)` from the root xmake still
compiles. The real criterion needs xmake to run, and what happens here is the static
necessary condition: no symbol of an optional package may be referenced by any other
package. That condition is not sufficient, since build-system coupling also exists, but it
is enough to catch the v1 incident, where `ApiTable.cpp` unconditionally forwarded to
`bridge::laneModBusyName` and deleting pier-lane broke the link outright. A pass means the
static part passed, and a real `xmake f` still has to run before delivery, as the output of
run-checks.py says.
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

PKGS = os.path.join(ROOT, "packages")

# The graph of contract §1. This is the machine-readable copy of the rule: a change to the
# graph changes this too, rather than the script being made to describe the present state.
ALLOWED_DEPS = {
    "pier-abi": set(),
    "pier-support": {"pier-abi"},
    "pier-host": {"pier-abi", "pier-support"},
    "pier-api": {"pier-abi", "pier-support", "pier-host"},
    "pier-hooks": {"pier-abi", "pier-support", "pier-host"},
    "pier-lane": {"pier-abi", "pier-support", "pier-host"},
    "pier-dimensions": {"pier-abi", "pier-support", "pier-host"},
    "pier-client": {"pier-abi", "pier-support", "pier-host"},
}

# Capability packages. They are siblings with no edge between them.
CAPABILITY = {"pier-api", "pier-hooks", "pier-lane", "pier-dimensions", "pier-client"}

# Optional packages: deleting that line from the root xmake must still compile.
OPTIONAL = {"pier-lane", "pier-dimensions"}

# The directories now separate this themselves: `packages/` holds only the C++ packages of
# the host proper and `bindings/<language>/` holds the bindings. A language discriminator
# in the script, checking for an xmake.lua or a Cargo.toml, used to make the distinction,
# which was the directory failing to express a fact that already held, since contract §1
# has always said these are two independent build lines. The discriminator stays as a
# backstop: another Cargo.toml appearing under `packages/` is a layering problem and should
# be reported rather than quietly accepted.
# The bindings live under `bindings/<language>/`.
CARGO_DEPS = {
    "pier-sys-rs": set(),                 # Reads abi.h only, no crate dependency
    "pier-rs": {"pier-sys-rs"},           # The safe wrapper, depending only on the raw FFI layer
}

BINDINGS = os.path.join(ROOT, "bindings")

# The include prefix each package exposes, used to attribute an #include to a package.
INCLUDE_OWNER = {
    "sdk/": "pier-abi",
    "pier/support/": "pier-support",
    "pier/host/": "pier-host",
    "pier/api/": "pier-api",
    "pier/hooks/": "pier-hooks",
    "pier/lane/": "pier-lane",
    "pier/dimensions/": "pier-dimensions",
    "pier/client/": "pier-client",
}


def _pkg_dirs():
    """The xmake packages, those with an xmake.lua."""
    return sorted(
        d for d in os.listdir(PKGS)
        if os.path.isdir(os.path.join(PKGS, d))
        and os.path.exists(os.path.join(PKGS, d, "xmake.lua"))
    )


def _crate_dirs():
    """The cargo crates, those with a Cargo.toml."""
    return sorted(
        d for d in os.listdir(PKGS)
        if os.path.isdir(os.path.join(PKGS, d))
        and os.path.exists(os.path.join(PKGS, d, "Cargo.toml"))
    )


def _unclassified():
    """Directories belonging to neither manifest kind. No set of criteria covers them, so they must be reported."""
    known = set(_pkg_dirs()) | set(_crate_dirs())
    return sorted(
        d for d in os.listdir(PKGS)
        if os.path.isdir(os.path.join(PKGS, d)) and d not in known
    )


def _xmake_of(pkg):
    p = os.path.join(PKGS, pkg, "xmake.lua")
    if not os.path.exists(p):
        return ""
    with open(p, encoding="utf-8") as f:
        return f.read()


def _declared_deps(text):
    deps = set()
    for m in re.finditer(r"add_deps\(([^)]*)\)", text):
        for tok in re.findall(r'"([^"]+)"', m.group(1)):
            deps.add(tok)
    return deps


def _sources(pkg):
    root = os.path.join(PKGS, pkg)
    for dp, _, names in os.walk(root):
        for fn in names:
            if fn.endswith((".cpp", ".h", ".hpp")):
                yield os.path.join(dp, fn)


def check_crates(r):
    """The dependency criterion for a cargo crate: only along CARGO_DEPS, and never on a C++ package."""
    xmake_pkgs = set(_pkg_dirs())
    for crate in _crate_dirs():
        if crate not in CARGO_DEPS:
            r.fail("crate %s is not in the graph of contract §1; adding a binding starts with the contract" % crate)
            continue
        text = _read(os.path.join(PKGS, crate, "Cargo.toml"))
        # The pier-* names appearing in the `[dependencies]` section
        m = re.search(r"^\[dependencies\](.*?)(?=^\[|\Z)", text, re.S | re.M)
        deps = set(re.findall(r"^\s*(pier-[\w-]+)", m.group(1), re.M)) if m else set()
        for d in deps - CARGO_DEPS[crate]:
            r.fail("crate %s depends on %s, which is not an edge the contract allows" % (crate, d))
        for d in deps & xmake_pkgs:
            r.fail("crate %s depends on the C++ package %s. The two build lines share only a "
                   "contract dependency and never a build dependency" % (crate, d))
    if _crate_dirs():
        r.note("%d cargo crate(s), with dependencies only along %s"
               % (len(_crate_dirs()),
                  ", ".join("%s -> %s" % (k, ", ".join(v) or "(none)") for k, v in CARGO_DEPS.items())))


def _read(path):
    if not os.path.exists(path):
        return ""
    with open(path, encoding="utf-8", errors="replace") as f:
        return f.read()


def check_layering(r):
    unk = _unclassified()
    for d in unk:
        r.fail("packages/%s has neither an xmake.lua nor a Cargo.toml, so no set of criteria "
               "covers it" % d)
    for pkg in _pkg_dirs():
        if pkg not in ALLOWED_DEPS:
            r.fail("package %s is not in the graph of contract §1; adding a package starts with the contract" % pkg)
            continue
        allowed = ALLOWED_DEPS[pkg]

        declared = _declared_deps(_xmake_of(pkg))
        # Only pier-* edges. An external dependency such as levilamina is outside this check.
        declared = {d for d in declared if d.startswith("pier-")}
        for d in declared - allowed:
            kind = "a sideways edge between capability packages" if (pkg in CAPABILITY and d in CAPABILITY) else "a skipping or reversed edge"
            r.fail("link graph: the add_deps of %s contains %s, which is %s and rule 1 of contract §1 does not allow" % (pkg, d, kind))

        # Compile graph: attributing an #include
        for src in _sources(pkg):
            rel = os.path.relpath(src, ROOT)
            with open(src, encoding="utf-8", errors="replace") as f:
                for i, line in enumerate(f, 1):
                    m = re.match(r'\s*#\s*include\s*[<"]([^>"]+)[>"]', line)
                    if not m:
                        continue
                    inc = m.group(1)
                    for prefix, owner in INCLUDE_OWNER.items():
                        if inc.startswith(prefix) and owner != pkg:
                            if owner not in allowed:
                                kind = ("a sideways edge" if (pkg in CAPABILITY and owner in CAPABILITY)
                                        else "a skipping or reversed edge")
                                r.fail("compile graph: the #include \"%s\" at %s:%d makes %s depend on %s, which is %s"
                                       % (rel, i, inc, pkg, owner, kind))
                            break
    if not r.failures:
        r.note("the link graph and the compile graph both follow the edges of contract §1, with no sideways edge between capability packages")


def check_object_kind(r2):
    bad = []
    for pkg in _pkg_dirs():
        text = _xmake_of(pkg)
        m = re.search(r'set_kind\("([^"]+)"\)', text)
        kind = m.group(1) if m else "(not declared)"
        if pkg == "pier-abi":
            if kind != "headeronly":
                r2.fail("pier-abi should be headeronly, with no source file, and is %s" % kind)
            continue
        if kind != "object":
            bad.append((pkg, kind))

    for pkg, kind in bad:
        # State the consequence: how many TUs of this package enter the artifact only through self-registration?
        selfreg = []
        for src in _sources(pkg):
            with open(src, encoding="utf-8", errors="replace") as f:
                t = f.read()
            if re.search(r"\b(SlotPackReg|BootstrapReg|TeardownReg|UnloadVetoReg|"
                         r"EventProviderReg|HookEventRegistrar)\b", t):
                selfreg.append(os.path.relpath(src, ROOT))
        detail = ""
        if selfreg:
            detail = (". This package has %d TU(s) registering themselves through a file-level "
                      "static object with no external symbol reference, so a static library has "
                      "the linker discard the whole object and the symptom is a capability "
                      "disappearing silently: %s%s"
                      % (len(selfreg), ", ".join(os.path.basename(s) for s in selfreg[:4]),
                         " …" if len(selfreg) > 4 else ""))
        r2.fail("%s is set_kind(\"%s\") while rule 4 of contract §1 requires object%s" % (pkg, kind, detail))

    if not bad:
        r2.note("the set_kind of all eight packages is compliant: pier-abi is headeronly and the rest are object")


def check_optional_drops(r3):
    root_xmake = os.path.join(ROOT, "xmake.lua")
    with open(root_xmake, encoding="utf-8") as f:
        root_text = f.read()

    for opt in sorted(OPTIONAL):
        if 'includes("packages/%s")' % opt not in root_text:
            r3.fail("the root xmake has no includes(\"packages/%s\"), so the criterion for an optional package cannot run" % opt)

        # The public header prefix of that optional package
        prefix = [p for p, o in INCLUDE_OWNER.items() if o == opt]
        for pkg in _pkg_dirs():
            if pkg == opt:
                continue
            if opt in _declared_deps(_xmake_of(pkg)):
                r3.fail("the add_deps of %s contains the optional package %s, so deleting it breaks the link" % (pkg, opt))
            for src in _sources(pkg):
                with open(src, encoding="utf-8", errors="replace") as f:
                    for i, line in enumerate(f, 1):
                        m = re.match(r'\s*#\s*include\s*[<"]([^>"]+)[>"]', line)
                        if m and any(m.group(1).startswith(p) for p in prefix):
                            r3.fail("%s:%d references a header of the optional package %s, so deleting %s stops it compiling"
                                    % (os.path.relpath(src, ROOT), i, opt, opt))
        # The add_deps of the root target may mention it, being the companion of the line to be deleted
    if not r3.failures:
        r3.note("no symbol of the optional package(s) %s is referenced across packages. The static "
                "necessary condition passed; the sufficient criterion is still a real xmake f run"
                % ", ".join(sorted(OPTIONAL)))


def run():
    results = []
    r = Result("pkg-layering")
    check_layering(r)
    check_crates(r)
    results.append(r)
    r2 = Result("object-kind")
    check_object_kind(r2)
    results.append(r2)
    r3 = Result("optional-drops")
    check_optional_drops(r3)
    results.append(r3)
    return results


if __name__ == "__main__":
    rc = 0
    for res in run():
        rc |= res.report()
    sys.exit(rc)
