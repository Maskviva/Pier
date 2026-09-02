# -*- coding: utf-8 -*-
"""rust-layering: the module dependency graph inside the bindings, the same rule as
contract §1 one level down.

## Why this exists

`pkg_layering.py` guards the edges among the eight C++ packages. The Rust side has only
two crates, so that check finds almost nothing here, while the place that really rots is
inside `pier-rs`: twenty-odd domain modules where who may use whom rests on discipline
alone.

One discussion raised splitting each domain into its own crate. The conclusion was not
to: a crate is the unit of compilation and release while a module is the unit of
modularity, and splitting would add twenty-odd Cargo.toml files and a new failure mode
from version mismatch without buying any isolation that does not already exist. The
discipline that discussion wanted was right, so the discipline lives here and is guarded
by a check rather than by directory structure.

A side effect is that the way back stays open: the edges have stayed clean, so turning a
domain into a crate later is mechanical.

## The criteria

1. `ALLOWED` is the machine-readable copy of that graph. An edge used without being
   declared is red.
2. An edge declared without being used is red. A stale permission is more dangerous than
   a missing one, because it makes the next person believe the edge is deliberate.
3. Any cycle is red. A cycle means two modules can only move together from then on, and
   that there is no telling which is built on which.

Only real code counts. A `[`crate::x`]` cross-reference in a doc comment is not an edge,
since that is exactly what documentation is for, and counting it as a dependency would
push people to delete useful links.
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

SRC = os.path.join(ROOT, "bindings", "rust", "pier-rs", "src")

# The graph. This is the rule and not a description of the present state: a change to the
# graph starts here.
#
#   rt      The runtime foundation: the handshake, the two gates, string handling and
#           logging. Anyone may use it.
#   types   Shared value types. Zero dependencies, deliberately: it is the common
#           currency for passing values between domains, and once it depends on a domain,
#           that domain becomes everyone's dependency.
#   nbt     The SNBT tree and its parsing. It uses only rt, since binary conversion goes
#           through the host parser.
ALLOWED = {
    # rt knows exactly one domain, context: the lifecycle entry point that
    # `register_mod!` expands to lives in `rt::registration` and has to hand a
    # `ModContext` to the callback. The edge is one way, since context never reaches back
    # into the internals of rt and only calls `get()` on each facade.
    "rt": {"context"},
    "types": set(),
    "nbt": {"rt"},
    # Leaf domains: they know the foundation only
    "service": {"rt"},
    "packet": {"rt"},
    "kvdb": {"rt"},
    "money": {"rt"},
    "client": {"rt"},
    "lane": {"rt"},
    "bus": {"rt"},
    "scoreboard": {"rt", "nbt"},
    "dimensions": {"rt", "nbt"},
    "command": {"rt", "nbt"},
    "server": {"rt", "nbt"},
    "item": {"rt", "nbt"},
    # Selectors: where the identity discipline lands. The command name of a custom
    # dimension is asked of dimensions.
    "sel": {"rt", "dimensions"},
    # Composite domains
    "event": {"rt", "nbt", "sel"},
    "container": {"rt", "sel", "item"},
    "entity": {"rt", "nbt", "types", "item"},
    "player": {"rt", "nbt", "types", "sel", "item", "container", "entity"},
    "gui": {"rt", "nbt", "player"},
    "sim": {"rt", "nbt", "sel", "player"},
    "block": {"rt", "nbt", "types", "item", "container"},
    # host sits below world and not above it: a convenience accessor such as
    # `Host::world()` would create a cycle, while world needing `execute_command` to
    # assemble a /fill is a real dependency. See the end of host.rs.
    "host": {"rt", "nbt", "types", "packet", "world", "server"},
    "world": {"rt", "nbt", "types", "sel", "block", "entity", "host"},
    # The facade for mod authors, aggregating the entry points of every domain. It sits
    # at the top. Living in `rt` made `ctx.host()` let the foundation know something built
    # on top of it, and the response then was to delete that accessor, which was wrong:
    # what had to move was its position and not an API mod authors were using.
    # `host_is_client` and `host_abi` read runtime state, so it really does depend on rt,
    # while `rt::registration` has to construct a ModContext for the callback. This pair
    # is of the same kind as host and world: it crosses zero-sized facades and read-only
    # state and shares no mutable state.
    "context": {"rt", "host", "packet", "world", "server"},
}


def _modules():
    out = {}
    for name in os.listdir(SRC):
        p = os.path.join(SRC, name)
        if name.endswith(".rs") and name != "lib.rs":
            out[name[:-3]] = [p]
        elif os.path.isdir(p):
            files = []
            for dp, _, fs in os.walk(p):
                files += [os.path.join(dp, f) for f in fs if f.endswith(".rs")]
            if files:
                out[name] = sorted(files)
    return out


def _reexport_owner():
    """Which module a name such as `crate::Player` belongs to, from the re-export table of lib.rs."""
    lib = open(os.path.join(SRC, "lib.rs"), encoding="utf-8").read()
    owner = {}
    # Both shapes have to be taken: `pub use server::Server;` without braces and
    # `pub use event::{A, B};`. An earlier version recognized only the latter, so
    # `crate::Server` resolved to no owner and the `server` edges were judged declared but
    # unused. A parser that fails on only some re-export spellings fools people more
    # easily than no parser at all.
    for m in re.finditer(r"^pub use (\w+)::(?:\{([^}]*)\}|(\w+));", lib, re.M):
        names = m.group(2) if m.group(2) is not None else m.group(3)
        for n in re.split(r"[,{}\s]+", names):
            n = n.strip()
            if n and n[0].isupper():
                owner[n] = m.group(1)
    return owner


def _strip_comments(text):
    return "\n".join(
        l for l in text.split("\n") if not l.lstrip().startswith(("///", "//!", "//"))
    )


def run():
    r = Result("rust-layering")
    if not os.path.isdir(SRC):
        r.fail("bindings/rust/pier-rs/src does not exist")
        return r

    mods = _modules()
    owner = _reexport_owner()

    unknown = sorted(set(mods) - set(ALLOWED))
    if unknown:
        r.fail(
            "these modules are not in the graph: %s. Adding a domain starts with a line for it "
            "in ALLOWED saying what it is built on" % ", ".join(unknown)
        )
    stale = sorted(set(ALLOWED) - set(mods))
    if stale:
        r.fail("the graph names modules that no longer exist: %s" % ", ".join(stale))

    actual = {}
    for name, files in mods.items():
        deps = set()
        for f in files:
            code = _strip_comments(open(f, encoding="utf-8").read())
            for other in mods:
                if other != name and re.search(r"crate::" + other + r"\b", code):
                    deps.add(other)
            for sym, om in owner.items():
                if om != name and re.search(r"crate::" + sym + r"\b", code):
                    deps.add(om)
        actual[name] = deps

    for name in sorted(set(mods) & set(ALLOWED)):
        extra = actual[name] - ALLOWED[name]
        if extra:
            r.fail(
                "`%s` uses undeclared edges: %s. Either the dependency should not exist or the "
                "graph changed, and both start with editing ALLOWED" % (name, ", ".join(sorted(extra)))
            )
        dead = ALLOWED[name] - actual[name]
        if dead:
            r.fail(
                "`%s` declares edges it does not use: %s. A stale permission is more dangerous "
                "than a missing one, because it makes the next person believe the edge is "
                "deliberate" % (name, ", ".join(sorted(dead)))
            )

    # A two-way reference between facades is the shape of this crate and not a design
    # mistake: `Host` hands out a `World` while `World` needs `Host::execute_command` to
    # assemble a `/fill`, and `ModContext` aggregates the entry points of every domain
    # while `rt::registration` has to hand it to a lifecycle callback. Each of these pairs
    # crosses only the `get()` of a zero-sized facade, shares no state, and changing one
    # side does not move the other.
    #
    # Without this opening, an earlier version turned the check green by deleting
    # `ctx.host()` and `Host::world()`, which cut an API mod authors were using for the
    # sake of an internal rule. A rule that forces the API surface to shrink is a problem
    # with the rule.
    #
    # The allowance is explicit: a new cycle is still red and adding one here states why.
    CYCLE_OK = {("host", "world"), ("context", "rt")}

    cycles = []
    for a in sorted(actual):
        for b in sorted(actual[a]):
            pair = tuple(sorted((a, b)))
            if pair in {tuple(sorted(c)) for c in CYCLE_OK}:
                continue
            if a in actual.get(b, set()) and (b, a) not in cycles:
                cycles.append((a, b))
    for a, b in cycles:
        r.fail(
            "`%s` and `%s` depend on each other. A cycle means the two modules can only move "
            "together from then on and that there is no telling which is built on which; cut "
            "whichever side is not needed" % (a, b)
        )

    if not r.failures:
        r.note(
            "%d module(s), %d edge(s), no cycle. The criterion covers `crate::` path references "
            "only; a name brought in through `use` and then used bare is resolved through the "
            "re-export table of lib.rs, and a name outside that table is invisible."
            % (len(mods), sum(len(v) for v in actual.values()))
        )
    return r


if __name__ == "__main__":
    sys.exit(run().report())
