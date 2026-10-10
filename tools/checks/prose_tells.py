# -*- coding: utf-8 -*-
"""prose-tells: the constructions that make documentation and comments read as generated.

What it watches: the documentation is written to show what happens, step by step, and leave
the reader to draw the conclusion; COMMENTS.md section 9 explains why. A few constructions
skip that process and hand over a verdict, a slogan or a metaphor instead, and they are
recognizable enough to check mechanically: "不是……而是……" used as a link inside a passage,
"带着一丝不易察觉的……", a closing metaphor such as a time bomb or an island, and their
English counterparts. This check fails on them in every Markdown file and in the comments
of every C, C++, Rust, Go, Zig and Python file.

What it cannot see: a passage that states a verdict in plain words, which is most of the
problem. It catches the tells that are almost never written by a person, and a reviewer
reads for the rest.
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

ZH = [
    (r"不是[^。，；：\n]{1,24}而是", "不是……而是……"),
    (r"并非[^。，；：\n]{1,24}而是", "并非……而是……"),
    (r"带着一丝|不易察觉", "带着一丝不易察觉的……"),
    (r"如同|仿佛|宛如", "如同、仿佛、宛如"),
    (r"孤岛|定时炸弹|盖棺定论|账本算不清", "收尾的比喻或口号"),
    (r"有(?:标准|规矩)[^。\n]{0,8}才(?:不会|能)", "有规矩才不会出错"),
    (r"归根结底|总而言之|说白了", "总结腔"),
    (r"糊弄|迟早出事", "下判词的口头禅"),
]
EN = [
    (r"\bno island\b|\bis an island\b|\btime bomb\b", "a closing metaphor"),
    (r"\bpaper over\b|\bnothing is lost\b|\bthe whole point\b|\bsooner or later\b", "a verdict phrase"),
    (r"\bis a wish\b", "a slogan"),
    (r"\bit'?s worth noting\b|\bdelve\b|\bseamless(?:ly)?\b", "filler"),
    (r"\ba hint of\b|\bimperceptible\b", "a hint of something imperceptible"),
    (r"\bnot just\b[^.\n]{1,60}\bbut\b|\bisn'?t just\b", "not just X, but Y"),
]
COMMENT = re.compile(r"//[^\n]*|/\*.*?\*/|^\s*#[^\n]*", re.S | re.M)
SKIP_DIRS = {"node_modules", ".git", "target", ".zig-cache", "zig-cache", "__pycache__", "dist", "cache",
             "venv", "zig-out", "build", "bin", ".xmake", ".idea", ".vs", ".vscode"}

# This file and the comment standard quote the tells in order to name them.
SKIP_FILES = {os.path.join("tools", "checks", "prose_tells.py"), "COMMENTS.md"}


def run():
    r = Result("prose-tells")
    files = 0
    for dp, dirs, names in os.walk(ROOT):
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS]
        for fn in sorted(names):
            p = os.path.join(dp, fn)
            rel = os.path.relpath(p, ROOT)
            if rel in SKIP_FILES or "include/sdk/abi.h" in rel.replace(os.sep, "/") and not rel.startswith("packages"):
                continue
            if fn.endswith(".md"):
                text = open(p, encoding="utf-8", errors="replace").read()
                spans = [(0, text)]
            elif fn.endswith((".c", ".h", ".cpp", ".hpp", ".rs", ".go", ".zig", ".py")):
                text = open(p, encoding="utf-8", errors="replace").read()
                spans = [(m.start(), m.group(0)) for m in COMMENT.finditer(text)]
            else:
                continue
            files += 1
            for start, span in spans:
                for rx, what in ZH + EN:
                    for m in re.finditer(rx, span, re.I):
                        line = text.count("\n", 0, start + m.start()) + 1
                        r.fail("%s:%d %s: \"%s\"; describe what happens instead (COMMENTS.md section 9)"
                               % (rel, line, what, m.group(0)))
    r.note("%d file(s) read for the constructions that hand over a verdict" % files)
    return r


if __name__ == "__main__":
    sys.exit(run().report())
