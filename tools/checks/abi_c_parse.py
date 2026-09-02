# -*- coding: utf-8 -*-
"""abi-c-parse: `sdk/abi.h` must parse under a C11 compiler (contract §0).

What it watches: the consumers of the contract are any language, and C is the one they
all read. Once C++ syntax slips into the header, an `enum class`, a nested type, a default
argument or a `std::` alias, the FFI tooling of Go, Zig and C# cannot take it, while the
C++ side compiles as always, so without this check there is no signal at all.

C++20 compiles it as well, since the host side is C++ and both have to pass.
"""

import os
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result, read_abi, strip_comments  # noqa: E402


def run():
    r = Result("abi-c-parse")
    inc = os.path.join(ROOT, "packages", "pier-abi", "include")

    for cc, std, tag in (("gcc", "c11", "C11"), ("g++", "c++20", "C++20")):
        src = "#include <sdk/abi.h>\n"
        # Also confirms the header can be included twice, meaning the guard works, which
        # is common in a real SDK.
        src += "#include <sdk/abi.h>\n"
        suffix = ".c" if cc == "gcc" else ".cpp"
        with tempfile.NamedTemporaryFile("w", suffix=suffix, delete=False) as f:
            f.write(src)
            path = f.name
        try:
            p = subprocess.run(
                [cc, "-std=" + std, "-fsyntax-only", "-Wall", "-I", inc, path],
                capture_output=True,
                text=True,
            )
            if p.returncode != 0:
                r.fail("%s failed to parse:\n%s" % (tag, p.stderr.strip()[:2000]))
            else:
                r.note("%s parsed" % tag)
        finally:
            os.unlink(path)

    # The gcc pass is the real criterion. This adds a plain-text backstop against someone
    # later putting a C++-only spelling inside an `#ifdef __cplusplus` branch, which gcc
    # never sees while the FFI tooling of another language reads it during preprocessing.
    #
    # Comments are stripped before scanning: the `::` in documentation prose such as
    # `ll::event::PlayerChatEvent` describes an LL event id and is not C++ syntax. Guessing
    # line by line whether a line is a comment would report all of them, and a check that
    # buries the real signal is no check.
    banned = ("enum class", "namespace ", "template<", "template <", "::")
    src_text = strip_comments(read_abi())
    for i, line in enumerate(src_text.splitlines(), 1):
        if 'extern "C"' in line:
            continue
        for b in banned:
            if b in line:
                r.fail("sdk/abi.h:%d carries the C++-only spelling %r: %s" % (i, b, line.strip()))
    if not r.failures:
        r.note("the body, with comments stripped, carries no C++-only spelling")
    return r


if __name__ == "__main__":
    sys.exit(run().report())
