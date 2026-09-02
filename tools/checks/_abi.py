# -*- coding: utf-8 -*-
"""The shared parser: every machine check concerning `sdk/abi.h` takes its facts here.

Why there is only one: abi-additive, sys-mirrors-abi and abi-no-lang all need the slot
order of PierApi. Three separate parsers would drift apart, and the symptom of that drift
is a check quietly no longer checking what it believes it checks, which is the same family
as the silent fallback contract §5 forbids.
"""

import os
import re

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
ABI_H = os.path.join(ROOT, "packages", "pier-abi", "include", "sdk", "abi.h")


def read_abi():
    with open(ABI_H, encoding="utf-8") as f:
        return f.read()


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def struct_body(src, name):
    """Returns the text inside the braces of `typedef struct <name> { ... } <name>;`."""
    m = re.search(
        r"typedef\s+struct\s+%s\s*\{(.*?)\n\}\s*%s\s*;" % (name, name), src, re.S
    )
    if not m:
        raise SystemExit("struct %s was not found in abi.h: the parser and the header have come apart" % name)
    return m.group(1)


def slots_of(src, struct="PierApi"):
    """Returns the field names in declaration order. A function-pointer slot yields the
    slot name and a scalar yields the field name.

    The order is the ABI itself, so any reordering of this list is a breaking change
    (contract §2.2).
    """
    body = strip_comments(struct_body(src, struct))
    out = []
    for decl in body.split(";"):
        decl = decl.strip()
        if not decl:
            continue
        m = re.search(r"\(\s*\*\s*([A-Za-z_]\w*)\s*\)", decl)
        if m:
            out.append(m.group(1))
            continue
        m = re.match(r"^[\w\s\*]*?\b(\w+)$", decl.replace("\n", " ").strip())
        if not m:
            raise SystemExit("a field of %s could not be parsed: %r" % (struct, decl[:60]))
        out.append(m.group(1))
    return out


# An include guard is not part of the contract and takes no part in the mirror comparison.
_GUARD = {"PIER_SDK_ABI_H"}


def defines(src):
    """Returns every `#define NAME value` literal macro, for the constant mirror
    comparison.

    A trailing block comment in the value is stripped. Without stripping, the comparison
    runs between `2 /* note */` and `2`, which never match, and a permanently red check is
    the same as no check.
    """
    out = {}
    for m in re.finditer(r"^#define\s+(PIER_\w+)\s+([^\n\\]*)$", src, re.M):
        name = m.group(1)
        if name in _GUARD:
            continue
        val = re.sub(r"/\*.*?\*/", "", m.group(2)).strip()
        if not val:
            continue  # A macro with no value, a pure switch, has nothing to compare
        out[name] = val
    return out


def same_value(a, b):
    """Whether two constant literals are numerically the same.

    Integers are parsed first, in decimal or hexadecimal and with u, U or L suffixes,
    and anything unparsable falls back to string equality after whitespace and parentheses
    are removed, since `(1 << PIER_PKT_INBOUND)` and `1 << PIER_PKT_INBOUND` are the same
    expression and C simply tends to add the parentheses.

    A spelling such as `rstrip("u32")` must not be used: the argument of rstrip is a set of
    characters, so `"2".rstrip("u32")` yields an empty string. The previous version of this
    file hit exactly that, and the symptom was every constant valued 2 or 3 reporting a
    difference between 2 and 2.
    """

    def norm(x):
        x = x.strip()
        x = re.sub(r"[uUlL]+$", "", x)
        x = re.sub(r"_(u|i)(8|16|32|64|size)$", "", x)
        return x.strip()

    na, nb = norm(a), norm(b)
    try:
        return int(na, 0) == int(nb, 0)
    except ValueError:
        pass
    squash = lambda x: re.sub(r"[\s()]", "", x)
    return squash(na) == squash(nb)


def enum_values(src, name):
    """Returns the members and values of `typedef enum <name> { A = 1, B = 2 } <name>;`."""
    m = re.search(
        r"typedef\s+enum\s+%s\s*\{(.*?)\}\s*%s\s*;" % (name, name), src, re.S
    )
    if not m:
        return None
    body = strip_comments(m.group(1))
    out = {}
    nxt = 0
    for item in body.split(","):
        item = item.strip()
        if not item:
            continue
        if "=" in item:
            k, v = item.split("=", 1)
            k, v = k.strip(), v.strip()
            try:
                nxt = int(v, 0)
            except ValueError:
                nxt = None
            out[k] = nxt
            if nxt is not None:
                nxt += 1
        else:
            out[item] = nxt
            if nxt is not None:
                nxt += 1
    return out


# The common shape for reporting a result
class Result:
    def __init__(self, check):
        self.check = check
        self.failures = []
        self.notes = []

    def fail(self, msg):
        self.failures.append(msg)

    def note(self, msg):
        self.notes.append(msg)

    def report(self):
        for n in self.notes:
            print("    · %s" % n)
        for f in self.failures:
            print("    ✗ %s" % f)
        ok = not self.failures
        print("  %s %s" % ("PASS" if ok else "FAIL", self.check))
        return 0 if ok else 1
