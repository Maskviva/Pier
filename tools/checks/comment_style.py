# -*- coding: utf-8 -*-
"""comment-style: the mechanical part of `COMMENTS.md`.

What it watches: a released comment must not carry the leftovers of development, meaning
process narrative, internal ticket numbers, conversational person, markdown layout and
over-budget essays. These are a negative for a reader, who has to read through a sequence
of events that does not concern them before reaching the one sentence that does.

Every criterion sits at the text level, so the coverage is narrow and certain:

  * the line budget of a comment block, one tier each for L1, L2 and L3 (COMMENTS.md §1)
  * banned wording (§3 items 2 and 4: process narrative and conversational person)
  * ticket and stage numbers (§3 item 3)
  * markdown layout and ASCII rules inside a comment (§3 item 5, and §5)
  * TODO and FIXME (§3 item 7)
  * CJK characters inside the abi.h comments (§6, comments in English)
  * a line width of 100 columns (§5)

What it cannot see: whether a comment tells the truth, which belongs to comment-claims
under §7; whether a surviving comment is one of the five kinds of §2; and whether it
restates the code. Those three need a human. Per contract §9, a pass means only that the
mechanical rules hold.
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

PKGS = os.path.join(ROOT, "packages")
ABI_REL = os.path.join("packages", "pier-abi", "include", "sdk", "abi.h")

MAX_HEAD = 16  # §1 L1: a file header
MAX_DOC = 14  # §1 L2: a /** */ above a declaration
MAX_LINE = 8  # §1 L3: consecutive // lines above a statement
MAX_COL = 100  # §5

# §3 items 2 and 4. The list takes only unambiguous wording: a check that misreports daily
# gets added to an ignore list and never speaks again. Words that a technical statement
# also needs stay out of the list.
BANNED = [
    (r"第一版|第一次重写|最初(?:的)?(?:做法|版本|实现)|曾经声称|旧仓|老仓", "process narrative"),
    (r"重写了[一二三四五六七八九十\d]+(?:次|遍)|改了[一二三四五六七八九十\d]+(?:次|遍)", "process narrative"),
    (r"血买来的|下一个人|上一个人|花了.{0,6}才(?:发现|查出)", "rhetoric"),
    (r"(?<![A-Za-z])我们(?![A-Za-z])|(?<![A-Za-z])咱(?:们)?(?![A-Za-z])", "conversational person"),
    (r"实测[一二三四五六七八九十\d]+(?:次|遍)|试过[一二三四五六七八九十\d]+(?:次|遍)", "process narrative"),
]
BANNED = [(re.compile(p), why) for p, why in BANNED]

# §3 item 3: an internal review ticket number, pointing at a document the reader cannot get.
# `stage N` is not taken: a stage here is the teardown order of spi::TeardownReg, a real
# concept in the code.
TICKET = re.compile(r"(?<![\w-])V-\d{2}(?![\w-])|(?<![\w])W\d{2}(?![\w])")

# §3 item 7.
TODO = re.compile(r"\b(TODO|FIXME|XXX|HACK)\b")

# §5: an ASCII rule, three or more repeated box-drawing characters.
RULE_LINE = re.compile(r"[─═━╌]{3,}|-{6,}$|={6,}$")

CJK = re.compile(r"[\u4e00-\u9fff]")


def blocks_of(text):
    """Cuts out comment blocks and returns (start line, list of lines, whether it is a block
    comment).

    Consecutive `//` lines count as one block; counting them separately would make every
    line of a 20-line run of line comments compliant on its own. A trailing comment, with
    code before it, takes no part in the budget and only in the word list and the line
    width.
    """
    lines = text.splitlines()
    out = []
    i = 0
    n = len(lines)
    while i < n:
        s = lines[i].strip()
        if s.startswith("/*"):
            j = i
            while j < n and "*/" not in lines[j]:
                j += 1
            out.append((i + 1, lines[i : j + 1], True))
            i = j + 1
            continue
        if s.startswith("//"):
            j = i
            while j + 1 < n and lines[j + 1].strip().startswith("//"):
                j += 1
            out.append((i + 1, lines[i : j + 1], False))
            i = j + 1
            continue
        i += 1
    return out


def prose_of(block_lines, is_block):
    """Strips the comment markers and leaves the body. The markdown-layout test has to run
    after ` * ` is stripped, otherwise every line starts with `*` and bold cannot be told
    apart from an ordinary continuation."""
    out = []
    for ln in block_lines:
        s = ln.strip()
        if is_block:
            s = re.sub(r"^/\*+", "", s)
            s = re.sub(r"\*+/$", "", s)
            s = re.sub(r"^\*+ ?", "", s)
        else:
            s = re.sub(r"^//+ ?", "", s)
        out.append(s)
    return out


def inline_comments(text):
    """A trailing comment, with code before the `//`. Returns (line number, body)."""
    out = []
    for k, ln in enumerate(text.splitlines(), 1):
        m = re.search(r"(?<![:/])//(?!/)(.*)$", ln)
        if m and ln[: m.start()].strip() and not ln.strip().startswith("//"):
            out.append((k, m.group(1).strip()))
    return out


def run():
    r = Result("comment-style")
    files = 0
    blocks = 0

    for dp, _, names in os.walk(PKGS):
        for fn in sorted(names):
            if not fn.endswith((".cpp", ".h", ".hpp")):
                continue
            p = os.path.join(dp, fn)
            rel = os.path.relpath(p, ROOT)
            is_abi = rel.replace("\\", "/") == ABI_REL.replace("\\", "/")
            files += 1
            with open(p, encoding="utf-8", errors="replace") as f:
                text = f.read()

            # Line width (§5). Only real comment lines are held to it; a code line belongs
            # to clang-format. The line numbers come from blocks_of plus the trailing
            # comments and are not guessed from the first character: a line such as
            # `*ptr->f()` is a dereference and not the continuation of a block comment, and
            # guessing by character would report it as an over-wide comment.
            cmt_lines = set()
            for start, lns, _ in blocks_of(text):
                cmt_lines.update(range(start, start + len(lns)))
            cmt_lines.update(k for k, _ in inline_comments(text))
            for k, ln in enumerate(text.splitlines(), 1):
                if k in cmt_lines and len(ln) > MAX_COL:
                    r.fail("%s:%d comment line width %d exceeds %d columns (§5)" % (rel, k, len(ln), MAX_COL))

            # Block level
            for start, lns, is_block in blocks_of(text):
                blocks += 1
                body = prose_of(lns, is_block)
                joined = "\n".join(body)

                # The budget (§1). abi.h is exempt, being product documentation (§6).
                if start <= 5:
                    cap, tier = MAX_HEAD, "L1 file header"
                elif is_block:
                    cap, tier = MAX_DOC, "L2 declaration comment"
                else:
                    cap, tier = MAX_LINE, "L3 body comment"
                if not is_abi and len(lns) > cap:
                    r.fail(
                        "%s:%d %s is %d lines, over the %d-line budget: design goes in "
                        "CONTRACT.md and history goes in git (§1)" % (rel, start, tier, len(lns), cap)
                    )

                # Markdown layout (§3 item 5)
                for off, line in enumerate(body):
                    if re.match(r"^#{1,6}\s", line):
                        r.fail("%s:%d a markdown heading inside a comment (§3.5)" % (rel, start + off))
                    if line.startswith("```"):
                        r.fail("%s:%d a markdown code fence inside a comment (§3.5)" % (rel, start + off))
                    if re.match(r"^\|.*\|$", line) or re.match(r"^\|?[\s:-]*-{3,}[\s:|-]*$", line):
                        r.fail("%s:%d a markdown table inside a comment (§3.5)" % (rel, start + off))
                    if re.search(r"\*\*\S(?:[^*]*\S)?\*\*", line):
                        r.fail("%s:%d markdown bold inside a comment (§3.5)" % (rel, start + off))
                    if RULE_LINE.search(line):
                        r.fail("%s:%d an ASCII rule inside a comment (§5)" % (rel, start + off))

                # Word list, ticket numbers, TODO
                for pat, why in BANNED:
                    m = pat.search(joined)
                    if m:
                        r.fail("%s:%d banned wording %r (%s, §3)" % (rel, start, m.group(0), why))
                m = TICKET.search(joined)
                if m:
                    r.fail("%s:%d an internal review ticket number %r (§3.3)" % (rel, start, m.group(0)))
                m = TODO.search(joined)
                if m:
                    r.fail("%s:%d %s belongs in an issue and not in the code (§3.7)" % (rel, start, m.group(0)))

                # abi.h is in English (§6)
                if is_abi and CJK.search(joined):
                    r.fail("%s:%d comments in the contract header must be in English (§6)" % (rel, start))

            # Trailing comments: no budget, everything else applies
            for k, body in inline_comments(text):
                for pat, why in BANNED:
                    m = pat.search(body)
                    if m:
                        r.fail("%s:%d banned wording %r (%s, §3)" % (rel, k, m.group(0), why))
                m = TICKET.search(body)
                if m:
                    r.fail("%s:%d an internal review ticket number %r (§3.3)" % (rel, k, m.group(0)))
                m = TODO.search(body)
                if m:
                    r.fail("%s:%d %s belongs in an issue and not in the code (§3.7)" % (rel, k, m.group(0)))
                if is_abi and CJK.search(body):
                    r.fail("%s:%d comments in the contract header must be in English (§6)" % (rel, k))

    r.note("scanned %d source file(s) and %d comment block(s)" % (files, blocks))
    r.note(
        "The criteria cover only the mechanical part of COMMENTS.md: budgets, banned "
        "wording, ticket numbers, markdown layout, line width and the language of the "
        "contract header. Whether a comment is true, whether it is one of the five kinds of "
        "§2 and whether it restates the code are invisible at the text level, so a pass does "
        "not mean the standard holds, only that the mechanical rules do."
    )
    return [r]


if __name__ == "__main__":
    rc = 0
    for res in run():
        rc |= res.report()
    sys.exit(rc)
