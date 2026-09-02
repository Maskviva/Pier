# -*- coding: utf-8 -*-
"""abi-fixed-width: the contract must carry no integer type whose width depends on the
platform.

What it watches: the widths of `int`, `long` and `unsigned short` are implementation
defined. Their presence in a contract that claims any language can read it requires a
binding author to first find out which compiler and which data model the host uses, LP64
or LLP64, before knowing how many bytes a parameter has, which is precisely what §0
removes by saying the consumers of the contract are any language.

A real compile forced this check into existence: nine slots of the `money_*` family copied
the LegacyMoney signatures verbatim, carrying `long long`, `int` and `unsigned short`. On
MSVC x64 those are 64, 32 and 16 bits and are binary identical to `int64_t`, `int32_t` and
`uint16_t`, so changing them costs nothing, while leaving them makes the first person
writing a Zig or C# binding guess.

`gcc -std=c11` compiles them, so `abi-c-parse` does not stop them, and the hand-written
mirror on the Rust side copied `int` across until `cargo clippy` reported
`cannot find type int`. A defect that only surfaces once a second language gets involved
is exactly the kind §9 asks for.

Two exceptions pass:
  * `char`, only inside `const char* ptr`, since that is what a C string is and there is
    no second spelling
  * `bool`, guaranteed by `<stdbool.h>` since C99
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result, read_abi, strip_comments  # noqa: E402

# Spellings whose width depends on the platform. Word boundaries keep `int` inside
# `int32_t` from matching.
BARE = re.compile(
    r"\b(?:unsigned\s+(?:char|short|int|long(?:\s+long)?)"
    r"|signed\s+(?:char|short|int|long(?:\s+long)?)"
    r"|long\s+long|long|int|short|unsigned|signed)\b"
)

# The suggested replacement, printed with the failure. Per §5.3 a log line has to answer
# what to do about it.
SUGGEST = {
    "long long": "int64_t",
    "unsigned long long": "uint64_t",
    "int": "int32_t",
    "unsigned int": "uint32_t",
    "unsigned": "uint32_t",
    "short": "int16_t",
    "unsigned short": "uint16_t",
    "long": "int32_t or int64_t, once you have confirmed which one you mean",
    "unsigned char": "uint8_t",
    "signed char": "int8_t",
}


def run():
    r = Result("abi-fixed-width")
    src = strip_comments(read_abi())
    hits = 0
    for i, line in enumerate(src.splitlines(), 1):
        # `const char*` is a member of PierStr and passes.
        probe = re.sub(r"\bconst\s+char\s*\*", "", line)
        m = BARE.search(probe)
        if not m:
            continue
        hits += 1
        found = " ".join(m.group(0).split())
        r.fail(
            "sdk/abi.h:%d uses %r, whose width depends on the platform; use %s instead, "
            "since the consumers of the contract are any language and cannot know how many "
            "bytes this type has on the host: %s"
            % (i, found, SUGGEST.get(found, "the matching fixed-width type"), line.strip()[:80])
        )
    if not hits:
        r.note("the contract carries no bare C integer type; `char` appears only inside `const char*`")
    return r


if __name__ == "__main__":
    sys.exit(run().report())
