# -*- coding: utf-8 -*-
"""zig-binding: what can be checked of the Zig binding without a Zig toolchain.

What it watches: the Zig binding reaches every slot through `slot("name")` and the constants
of abi.h through translate-c, so a slot or constant renamed in abi.h breaks it only at a Zig
build. This check holds the names together: props_gen.zig and the package's copy of abi.h
are current (tools/gen-zig-api.py --check), every slot name the Zig sources pass to `slot`
is a slot of PierApi, every PIER_* constant they read is one abi.h defines, and no Zig file
has unbalanced brackets.

What it cannot see: anything a Zig compiler would. `zig build test` in bindings/zig is that
check, and the zig job of build.yml runs it: it makes the compiler analyze every function
of the binding against the table translate-c read from abi.h.
"""

import glob
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result, defines, read_abi, slots_of  # noqa: E402

PKG = os.path.join(ROOT, "bindings", "zig")


def code_only(text):
    """The text with char literals, multiline strings, strings and comments removed, in that
    order: '\\' is a char literal and not the start of a multiline string, and '"' holds a
    quote rather than starting a string. A multiline string line starts with \\ after
    indentation only."""
    text = re.sub(r"'(?:\\.|[^'\\\n])'", "''", text)
    text = re.sub(r"(?m)^[ \t]*\\\\[^\n]*", "", text)
    text = re.sub(r'"(?:\\.|[^"\\\n])*"', '""', text)
    return re.sub(r"//[^\n]*", "", text)


def balanced(text):
    text = code_only(text)
    pairs, stack = {")": "(", "]": "[", "}": "{"}, []
    for ch in text:
        if ch in "([{":
            stack.append(ch)
        elif ch in ")]}":
            if not stack or stack.pop() != pairs[ch]:
                return False
    return not stack


def run():
    r = Result("zig-binding")
    if not os.path.isdir(PKG):
        r.fail("bindings/zig is missing")
        return r
    gen = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "gen-zig-api.py"), "--check"],
                         capture_output=True, text=True)
    if gen.returncode != 0:
        r.fail(gen.stdout.strip() or gen.stderr.strip())
    src = read_abi()
    slots = set(slots_of(src))
    consts = set(k for k in defines(src) if k.startswith("PIER_"))
    consts |= set(re.findall(r"\b(PIER_\w+)\s*=", src))
    files = sorted(glob.glob(os.path.join(PKG, "**", "*.zig"), recursive=True))
    files += sorted(glob.glob(os.path.join(ROOT, "examples", "hello-pier-zig", "**", "*.zig"), recursive=True))
    used_slots = 0
    for p in files:
        with open(p, encoding="utf-8") as f:
            text = f.read()
        rel = os.path.relpath(p, ROOT)
        if not balanced(text):
            r.fail("%s has unbalanced brackets" % rel)
        no_comments = re.sub(r"//[^\n]*", "", text)
        for name in re.findall(r'\bslot\("(\w+)"\)', no_comments):
            used_slots += 1
            if name not in slots:
                r.fail("%s asks for slot %s, which PierApi does not have" % (rel, name))
        for name in sorted(set(re.findall(r"\bc\.(PIER_\w+)", no_comments)) - consts):
            r.fail("%s reads %s, which abi.h does not define" % (rel, name))
    r.note("%d Zig file(s), %d slot reference(s) checked by name; type-checking them is "
           "`zig build test`, the zig job of build.yml" % (len(files), used_slots))
    return r


if __name__ == "__main__":
    sys.exit(run().report())
