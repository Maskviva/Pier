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


def template_bodies(text):
    """The bodies of the class templates defined in one header, braces matched."""
    out = []
    for m in re.finditer(r"\btemplate\s*<[^;{}]*>\s*(?:class|struct)\s+\w+[^;{}]*\{", text):
        depth, i = 1, m.end()
        while i < len(text) and depth:
            depth += {"{": 1, "}": -1}.get(text[i], 0)
            i += 1
        out.append(text[m.end():i - 1])
    return out


def collect_engine_members(inc_dir):
    """Member name to (how T is spelled, the header declaring it). A name whose spellings in
    different classes would be judged differently is discarded as ambiguous."""
    members = {}
    ambiguous = set()
    enums = set()
    n_files = 0
    decl = re.compile(
        r"::ll::TypedStorage<\s*[^,]+,\s*[^,]+,\s*(.+?)\s*>\s+(m\w+)\s*;"
    )
    decl2 = re.compile(r"\bll::TypedStorage<\s*[^,]+,\s*[^,]+,\s*(.+?)\s*>\s+(m\w+)\s*;")
    # A plain member of the same name in a class template makes the name ambiguous too:
    # BidirectionalUnorderedMap holds a plain `mLeft`, and taking every `mLeft` for the one
    # TypedStorage `mLeft` of a camera component reported sites that are correct.
    plain = re.compile(r"^[ \t]*(?!.*TypedStorage)[\w:<>,\s\*&]+?[\s\*&](m[A-Z]\w*)\s*(?:;|\{\}|=)", re.M)
    plain_names = set()
    spellings = {}
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
            # Only a class template counts: it is a hand-written generic container that Pier
            # may instantiate itself, while a plain member of an ordinary class elsewhere,
            # such as a reference in a LeviLamina event, is not what an engine-typed object
            # in Pier holds.
            for body in template_bodies(text):
                for m in plain.finditer(body):
                    plain_names.add(m.group(1))
            for rx in (decl, decl2):
                for m in rx.finditer(text):
                    t, name = " ".join(m.group(1).split()), m.group(2)
                    spellings.setdefault(name, []).append((t, os.path.relpath(p, inc_dir)))
    # A name held by several classes is still judged when every spelling gets the same
    # verdict: mCause is ActorDamageCause in most classes and ActorHealCause in one, both
    # enums, so `.get()` on any of them is the same error. Only a name whose spellings would
    # be judged differently is ambiguous, along with the plain members above.
    for name, seen in spellings.items():
        verdicts = {collapses(t, enums)[0] for t, _ in seen}
        if len(verdicts) == 1 and name not in plain_names:
            members[name] = seen[0]
        else:
            ambiguous.add(name)
    return members, enums, ambiguous, n_files


def collect_project_members():
    """Member names Pier declares in its own types. A site using one of them may be reading
    Pier's member and not the engine's, so it is not judged; PackCache::mTemplates and an
    engine pool's mTemplates are spelled alike."""
    names = set()
    rx = re.compile(r"^[ \t]*(?!.*TypedStorage)[\w:<>,\s\*&]+?[\s\*&](m[A-Z]\w*)\s*(?:;|\{|=)", re.M)
    for dp, _, fs in os.walk(PKGS):
        for fn in fs:
            if fn.endswith((".h", ".hpp", ".cpp")):
                with open(os.path.join(dp, fn), encoding="utf-8", errors="replace") as fh:
                    names |= set(rx.findall(strip(fh.read())))
    return names


def collapses(t, enums):
    """Whether this T collapses inside a TypedStorage, which makes `.get()` a compile error."""
    t = t.strip()
    if t.endswith("&") or t.endswith("&&"):
        return True, "a reference"
    # A cv-qualifier is not the type: `ActorDamageCause const` is still that enum, and
    # taking the last word of it as the type name read "const" and judged it a class.
    t = re.sub(r"\b(?:const|volatile)\b", " ", t).strip()
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
    own = collect_project_members() & set(members)
    for name in own:
        members.pop(name, None)
    print("  %d engine header(s), %d TypedStorage member(s), %d enum(s), %d ambiguous name(s) excluded,"
          " %d more because Pier declares a member of that name itself"
          % (n_files, len(members), len(enums), len(ambiguous), len(own)))

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
