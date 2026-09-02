# -*- coding: utf-8 -*-
"""abi-no-lang: the contract surface must favor no consumer language
(contract §1 rule 5 plus §7).

The check has three halves and the criteria are deliberately asymmetric; the reasons
follow.

## Part one: no consumer-language spelling anywhere in `pier-abi/`, comments included

The consumers are any language. Once a per-slot comment says `#[repr(C)] function table`,
`usually Arc::into_raw` or `&'static or a leaked Box`, a Go, Zig or C# author reading this
contract receives a dialect of another language rather than a specification: they have to
work through the Rust ownership model in their head before knowing what their own side
must guarantee.

The mechanism itself is language neutral, a function table with C layout that lives until
the lane is withdrawn, and the wording describing it must be too. The original v1 incident
had exactly this shape: only after "Rust has no stable ABI" was rewritten as "most native
languages have no stable ABI" did the passage hold for everyone.

## Part two: C++ spellings pass only inside comments

The reason for the asymmetry: C++ is the implementation language of the host and not one
of the contract's consumers. A comment saying the host once accumulated a
std::vector<std::string> and destroyed it across a DLL boundary, which crashed, so it
became a zero-buffer pipeline, explains why this slot looks the way it does, which is
useful to a binding author in any language. The difference from "you should use
Arc::into_raw" is that the former describes the history of the host while the latter
instructs the consumer, in the dialect of one particular consumer.

Declarations do not get this exemption, since `abi-c-parse` scans declarations after
stripping comments. Only both checks together make up the whole of rule 5.

## Part three: historical product names in user-visible strings (§7)

Only string literals are scanned, not comments and not documentation. The reason is the
same: recording that the old repository was called levilamina-rust-loader and that a
comment even had the path wrong is history, while
`log("update levilamina-rust-loader")` really does print in front of a user.
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

# The names and exclusive spellings of consumer languages. Word boundaries avoid false
# positives such as "trust" and "zigzag".
CONSUMER_LANG = re.compile(
    r"(\bRust\b|\brust\b|\bGolang\b|\bZig\b|\bKotlin\b|\bSwift\b|C#|"
    r"#\[|\brepr\(|&'static|\bArc::|\bBox<|\bunsafe\b|\bimpl\b|"
    r"\bOption<|\bResult<|\bVec<)"
)

# C++ spellings: permitted only inside comments, see part two.
CXX_SPELL = ("std::", "string_view", "enum class", "template<", "template <")

# Historical product names. Only a string literal counts as a leftover.
LEGACY = (
    "levilamina-rust-loader",
    "levilamina-rs",
    "levi_rs",
    "LEVI_RS",
    "/llr",
    "/levirs",
)

STR_LIT = re.compile(r'"((?:[^"\\]|\\.)*)"')
CODE_EXT = (".h", ".hpp", ".cpp", ".rs")


def split_comments(text):
    """Returns (comment lines, code lines), each aligned with the original line numbers."""
    out_code, out_cmt = [], []
    in_block = False
    for line in text.splitlines():
        code, cmt = [], []
        i = 0
        while i < len(line):
            if in_block:
                j = line.find("*/", i)
                if j < 0:
                    cmt.append(line[i:])
                    break
                cmt.append(line[i:j])
                in_block = False
                i = j + 2
            elif line.startswith("/*", i):
                in_block = True
                i += 2
            elif line.startswith("//", i):
                cmt.append(line[i:])
                break
            else:
                code.append(line[i])
                i += 1
        out_code.append("".join(code))
        out_cmt.append("".join(cmt))
    return out_cmt, out_code


def run():
    r = Result("abi-no-lang")

    # Parts one and two: pier-abi/
    abi_dir = os.path.join(ROOT, "packages", "pier-abi")
    files = 0
    for dirpath, _, names in os.walk(abi_dir):
        for fn in names:
            p = os.path.join(dirpath, fn)
            rel = os.path.relpath(p, ROOT)
            files += 1
            with open(p, encoding="utf-8", errors="replace") as f:
                text = f.read()
            _, code_lines = split_comments(text)
            for i, line in enumerate(text.splitlines(), 1):
                m = CONSUMER_LANG.search(line)
                if m:
                    r.fail("%s:%d carries the consumer-language spelling %r, which is not allowed even in a comment: %s"
                           % (rel, i, m.group(0), line.strip()[:90]))
            for i, code in enumerate(code_lines, 1):
                for s in CXX_SPELL:
                    if s in code:
                        r.fail("%s:%d has the C++ spelling %r in a declaration: %s"
                               % (rel, i, s, code.strip()[:90]))
    r.note("scanned %d file(s) under pier-abi/: no consumer language in comments, no C++ type in declarations" % files)

    # Part three: historical product names in string literals across the repository
    skip_dirs = {".git", "target", "node_modules", "tools"}
    hits = 0
    scanned = 0
    for dirpath, dirs, names in os.walk(ROOT):
        dirs[:] = [d for d in dirs if d not in skip_dirs]
        for fn in names:
            if not fn.endswith(CODE_EXT):
                continue
            p = os.path.join(dirpath, fn)
            rel = os.path.relpath(p, ROOT)
            scanned += 1
            with open(p, encoding="utf-8", errors="replace") as f:
                text = f.read()
            _, code_lines = split_comments(text)
            for i, code in enumerate(code_lines, 1):
                for lit in STR_LIT.findall(code):
                    for bad in LEGACY:
                        if bad in lit:
                            r.fail("%s:%d has the historical product name %r in a user-visible string: %r"
                                   % (rel, i, bad, lit[:70]))
                            hits += 1
    if not hits:
        r.note("no historical product name in the string literals of %d source file(s)" % scanned)
    return r


if __name__ == "__main__":
    sys.exit(run().report())
