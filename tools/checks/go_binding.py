# -*- coding: utf-8 -*-
"""go-binding: what can be checked of the Go binding without a Go toolchain.

What it watches: the Go binding reaches the host through generated C trampolines and a
hand-written glue layer, and a name that drifts between the three layers fails only at a
cgo build on Windows. This check holds them together: slots_gen.h and the package's copy of
abi.h are current (tools/gen-go-slots.py --check), every `C.piergo_*` the Go files call is
declared in slots_gen.h or glue.h, every exported Go function glue.c hands to the host has
its `//export`, every exported function recovers a panic, and no Go file has unbalanced
brackets.

What it cannot see: anything a Go compiler would, which the `go` job of build.yml runs.
"""

import glob
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

PKG = os.path.join(ROOT, "bindings", "go", "levilamina")


def balanced(text):
    text = re.sub(r"`[^`]*`", "", text)
    text = re.sub(r'"(?:\\.|[^"\\\n])*"', '""', text)
    text = re.sub(r"'(?:\\.|[^'\\\n])'", "''", text)
    text = re.sub(r"//[^\n]*", "", text)
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    pairs, stack = {")": "(", "]": "[", "}": "{"}, []
    for ch in text:
        if ch in "([{":
            stack.append(ch)
        elif ch in ")]}":
            if not stack or stack.pop() != pairs[ch]:
                return False
    return not stack


def run():
    r = Result("go-binding")
    if not os.path.isdir(PKG):
        r.fail("bindings/go/levilamina is missing")
        return r
    gen = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "gen-go-slots.py"), "--check"],
                         capture_output=True, text=True)
    if gen.returncode != 0:
        r.fail(gen.stdout.strip() or gen.stderr.strip())
    api = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "gen-go-api.py"), "--check"],
                         capture_output=True, text=True)
    if api.returncode != 0:
        r.fail(api.stdout.strip() or api.stderr.strip())
    declared = set()
    for h in ("slots_gen.h", "glue.h"):
        with open(os.path.join(PKG, h), encoding="utf-8") as f:
            declared |= set(re.findall(r"\b(piergo_\w+)\s*\(", f.read()))
    exported = set()
    gofiles = sorted(glob.glob(os.path.join(PKG, "*.go")))
    for p in gofiles:
        with open(p, encoding="utf-8") as f:
            text = f.read()
        rel = os.path.relpath(p, ROOT)
        if not balanced(text):
            r.fail("%s has unbalanced brackets" % rel)
        for name in sorted(set(re.findall(r"\bC\.(piergo_\w+)\b", text)) - declared):
            r.fail("%s calls C.%s, which neither slots_gen.h nor glue.h declares" % (rel, name))
        exported |= set(re.findall(r"^//export (\w+)$", text, re.M))
        # A panic unwinding into the host is undefined behavior, so every exported body
        # recovers, directly or through enter or runStage, which do.
        for m in re.finditer(r"^//export (\w+)\nfunc \w+\([^)]*\)[^{]*\{\n(.*?)\n\}", text, re.M | re.S):
            if not re.search(r"\b(recoverTo|enter|runStage)\(", m.group(2)):
                r.fail("%s: %s is called by the host and does not recover a panic" % (rel, m.group(1)))
    with open(os.path.join(PKG, "glue.c"), encoding="utf-8") as f:
        glue = f.read()
    for name in sorted(set(re.findall(r"\b(piergo[A-Z]\w*)\b", glue)) - exported):
        r.fail("glue.c hands %s to the host and no Go file exports it" % name)
    if "pier_main" not in exported:
        r.fail("no Go file exports pier_main, so no Go mod can be loaded")
    r.note("%d Go file(s), %d exported function(s), the trampolines current; compiling them "
           "is the go job of build.yml" % (len(gofiles), len(exported)))
    return r


if __name__ == "__main__":
    sys.exit(run().report())
