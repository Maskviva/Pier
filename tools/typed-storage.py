#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""typed-storage: whether `.get()` on an `ll::TypedStorage` member is used correctly.

## The rules, for which this file is the single formal source

`ll::TypedStorage<Align, Size, T>` is not one uniform wrapper. It specializes on `T`:

| what `T` is | what the member is | `.get()` |
|---|---|---|
| a class type by value (`std::string`, `BlockPos`, `std::vector<...>`) | the wrapper | required |
| a scalar or enum (`int`, `bool`, `DimensionType`, `ActorDamageCause`) | the value itself | a compile error |
| a reference (`Dimension&`, `Player&`) | the reference itself | a compile error |
| `std::unique_ptr<T>` | the unique_ptr itself | required, but that is `unique_ptr::get` and means something else |

Both symptoms of getting it wrong have been seen on a machine:
* scalar: `C2228: left of ".get" must have class/struct/union`
* reference: `C2039: "get" is not a member of "Dimension"`

This rule used to be spread across the comments of four files, worded differently in each,
with one saying scalars collapse, another saying scalars and references collapse, and a
third mentioning only unique_ptr. A rule with four sources has no source, since nobody
knows which one is current. The formal source is here now and those comments point at
it.

## This check needs the engine headers

Deciding what `T` a member holds means reading `mc/**/*.h`. Those headers live in the
LeviLamina xmake package directory and may not exist on a given machine. When they are
not found this reports SKIP and not PASS, because missing headers do not mean the code is
fine; contract §9 says a pass only earns a checkmark for what it covers.

Pointing at them:
    set PIER_LL_INCLUDE=C:\\Users\\<you>\\AppData\\Local\\.xmake\\packages\\l\\levilamina\\...\\include
or let the script look in the usual places itself.

## Position: a surrogate, not a contract check

The compiler always reports this, so by the criterion of §9 it does not belong in that
table. Its value is reporting everything at once: the compiler reports only the first
failing TU, while this script verifies all 30 `.get()` call sites in the repository
together.
"""

import glob
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
PKGS = os.path.join(ROOT, "packages")

# Scalars and the known fundamental types. These collapse inside a TypedStorage.
SCALARS = {
    "bool", "char", "signed char", "unsigned char", "short", "unsigned short",
    "int", "unsigned int", "uint", "long", "unsigned long", "long long",
    "unsigned long long", "float", "double", "size_t", "ptrdiff_t",
    "int8", "int16", "int32", "int64", "uint8", "uint16", "uint32", "uint64",
    "uchar", "ushort", "ulong", "uint64_t", "int64_t", "uint32_t", "int32_t",
    "uint16_t", "int16_t", "uint8_t", "int8_t", "std::byte",
}


def strip(text):
    def keep_nl(m):
        return "\n" * m.group(0).count("\n")

    text = re.sub(r"/\*.*?\*/", keep_nl, text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    return re.sub(r'"(?:[^"\\]|\\.)*"', keep_nl, text, flags=re.S)


def find_engine_include():
    env = os.environ.get("PIER_LL_INCLUDE")
    if env and os.path.isdir(env):
        return env
    home = os.path.expanduser("~")
    pats = [
        os.path.join(home, "AppData", "Local", ".xmake", "packages", "l", "levilamina",
                     "*", "*", "include"),
        os.path.join(home, ".xmake", "packages", "l", "levilamina", "*", "*", "include"),
        os.path.join(ROOT, ".xmake", "packages", "l", "levilamina", "*", "*", "include"),
    ]
    for pat in pats:
        hits = sorted(glob.glob(pat))
        if hits:
            return hits[-1]
    return None


def collect_engine_members(inc_dir):
    """Member name to (how T is spelled, the header declaring it). A member name appearing in more than one class is discarded."""
    members = {}
    ambiguous = set()
    enums = set()
    n_files = 0
    decl = re.compile(
        r"::ll::TypedStorage<\s*[^,]+,\s*[^,]+,\s*(.+?)\s*>\s+(m\w+)\s*;"
    )
    decl2 = re.compile(r"\bll::TypedStorage<\s*[^,]+,\s*[^,]+,\s*(.+?)\s*>\s+(m\w+)\s*;")
    enum_decl = re.compile(r"\benum\s+(?:class\s+|struct\s+)?(\w+)\s*(?::[^{;]+)?[{;]")

    for dp, _, fs in os.walk(inc_dir):
        for fn in fs:
            if not fn.endswith((".h", ".hpp")):
                continue
            p = os.path.join(dp, fn)
            n_files += 1
            try:
                text = strip(open(p, encoding="utf-8", errors="replace").read())
            except OSError:
                continue
            for m in enum_decl.finditer(text):
                enums.add(m.group(1))
            for rx in (decl, decl2):
                for m in rx.finditer(text):
                    t, name = " ".join(m.group(1).split()), m.group(2)
                    if name in members and members[name][0] != t:
                        ambiguous.add(name)
                    members.setdefault(name, (t, os.path.relpath(p, inc_dir)))
    for name in ambiguous:
        members.pop(name, None)
    return members, enums, ambiguous, n_files


def collapses(t, enums):
    """Whether this T collapses inside a TypedStorage, which makes `.get()` a compile error."""
    t = t.strip()
    if t.endswith("&") or t.endswith("&&"):
        return True, "a reference"
    base = t.replace("::", " ").split()[-1] if t else ""
    if t in SCALARS or base in SCALARS:
        return True, "a scalar"
    if base in enums:
        return True, "an enum"
    return False, ""


def main():
    inc = find_engine_include()
    if inc is None:
        print("  SKIP: the LeviLamina include directory was not found, so member types cannot be decided.")
        print("        Point PIER_LL_INCLUDE at it and run again.")
        print("        This is not a PASS: missing headers do not mean the code is fine.")
        return 0

    members, enums, ambiguous, n_files = collect_engine_members(inc)
    print("  %d engine header(s), %d TypedStorage member(s), %d enum(s), %d ambiguous name(s) excluded"
          % (n_files, len(members), len(enums), len(ambiguous)))

    problems = []
    scanned = 0
    for dp, _, fs in os.walk(PKGS):
        for fn in sorted(fs):
            if not fn.endswith((".cpp", ".h", ".hpp")):
                continue
            p = os.path.join(dp, fn)
            rel = os.path.relpath(p, ROOT)
            scanned += 1
            code = strip(open(p, encoding="utf-8", errors="replace").read())
            for m in re.finditer(r"\b(m[A-Z]\w*)\s*\.\s*get\s*\(\s*\)", code):
                name = m.group(1)
                info = members.get(name)
                if not info:
                    continue
                t, where = info
                bad, why = collapses(t, enums)
                if bad:
                    line = code[: m.start()].count("\n") + 1
                    problems.append(
                        "%s:%d `%s.get()`: %s holds %s (%s), TypedStorage specializes on it, "
                        "the member is that value itself and `.get()` is a compile error. Declared in %s"
                        % (rel, line, name, name, t, why, where)
                    )
            # The reverse: a class type by value missing its .get() is equally a compile
            # error, only with a different symptom.
            for m in re.finditer(r"\b(m[A-Z]\w*)\s*\.\s*(?!get\b)(\w+)\s*\(", code):
                name = m.group(1)
                info = members.get(name)
                if not info:
                    continue
                t, where = info
                bad, _ = collapses(t, enums)
                if not bad and not t.startswith("std::unique_ptr"):
                    line = code[: m.start()].count("\n") + 1
                    problems.append(
                        "%s:%d `%s.%s(...)`: %s holds the class type %s, TypedStorage keeps "
                        "the wrapper, and `.get()` comes first to reach it. Declared in %s"
                        % (rel, line, name, m.group(2), name, t, where)
                    )

    for pb in sorted(set(problems)):
        print("  ✗ %s" % pb)
    if problems:
        print()
        print("  %d site(s). The rules are in this file's header." % len(set(problems)))
        return 1
    print("  every TypedStorage member access across %d source file(s) follows the collapse rules." % scanned)
    return 0


if __name__ == "__main__":
    sys.exit(main())
