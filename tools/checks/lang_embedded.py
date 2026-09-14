# -*- coding: utf-8 -*-
"""lang-embedded: `LangFiles.cpp` still holds byte for byte what `lang/en_US.lang` holds.

What it watches: English is the fallback of the language chain, so it has to answer with
no file on disk, which means the text exists twice: once as the file the release ships and
once as string data the binary carries. Two copies of anything drift, and this pair drifts
silently. The repository looks translated while the binary falls back to older wording, and
nothing on either side reports it.

Byte equality and not key equality. The header comments of a `.lang` are what tell a
translator the format, so the embedded copy has to be the file and not a reconstruction of
its keys.

The criterion reconstructs the array literal rather than compiling anything, so an escape
form that arrives later is read by the compiler and not by this script.
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
EMBED = ROOT / "packages/pier-support/src/LangFiles.cpp"
REFERENCE = ROOT / "lang/en_US.lang"

ARRAY = re.compile(r"constexpr std::string_view kEnUS\[\] = \{(.*?)\n        \};", re.S)
LINE = re.compile(r'^\s*"((?:[^"\\]|\\.)*)",\s*$')


def unescape(s):
    return s.replace('\\"', '"').replace("\\\\", "\\")


def main():
    problems = []
    if not EMBED.exists():
        print("    ✗ LangFiles.cpp was not found, so nothing carries the English lines")
        return 1
    if not REFERENCE.exists():
        print("    ✗ lang/en_US.lang was not found, so there is nothing to compare against")
        return 1

    m = ARRAY.search(EMBED.read_text(encoding="utf-8"))
    if not m:
        print("    ✗ the kEnUS array was not found in LangFiles.cpp; its shape changed")
        return 1

    lines = []
    for raw in m.group(1).split("\n"):
        hit = LINE.match(raw)
        if hit:
            lines.append(unescape(hit.group(1)))
    embedded = "\n".join(lines) + "\n"
    want = REFERENCE.read_text(encoding="utf-8")

    print("    · lang/en_US.lang: %d line(s), %d byte(s)" % (want.count("\n"), len(want)))
    if embedded != want:
        first = next(
            (i for i, (a, b) in enumerate(zip(embedded.splitlines(), want.splitlines()), 1) if a != b),
            min(len(embedded.splitlines()), len(want.splitlines())) + 1,
        )
        problems.append(
            "the copy in LangFiles.cpp differs from lang/en_US.lang, first at line %d; "
            "regenerate the array" % first
        )
    else:
        print("    · the copy in LangFiles.cpp is identical")

    for p in problems:
        print("    ✗ %s" % p)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
