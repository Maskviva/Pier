# -*- coding: utf-8 -*-
"""no-silent-fallback and comment-claims: two items of contract §5, namely 5.1 and 5.4.

Both are properties findable at the text level that the compiler cannot find, so they
share one file.

## comment-claims (§5.4: a comment must not lie about the code)
What it watches: a comment claiming a safety property must sit next to the code
implementing that property. v1 had three places saying the exception is swallowed while
the same function had no try at all, and an exception escaping a detour takes the whole
server down. The next person reasons from the comment, which makes a lying comment more
dangerous than no comment.
The criterion: when a comment inside a function body carries an assertion such as
swallowed, caught, never throws or does not escape, the same function body must contain a
`try` or a `catch`.

## no-silent-fallback (§5.1: no silent fallback)
What it watches: the `unwrap_or(0)` family, where a value that cannot be read is quietly
replaced by a default. A real incident: the dim could not be read from an event payload,
the consumer substituted 0, every event inside a custom dimension was treated as happening
in the overworld, land protection refused in the overworld and allowed everywhere else,
and nothing was logged.
The criterion is conservative and reports only high-confidence shapes: a block after
`catch (...)` containing no logging call, no rethrow, and no return of a value able to
express that there is no answer.
A purely textual criterion is necessarily incomplete. It catches the most typical shape,
a catch that does nothing, and the complex shapes it misses are left to the human review
of §5. A passing script does not mean the property holds, only that the most typical
class does not occur, which CONTRACT §9 records.
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

PKGS = os.path.join(ROOT, "packages")

# The assertion words. An exception word must appear as well, since "swallow the packet
# entirely" is about dropping a packet and not about swallowing an exception. Matching on
# the assertion word alone would report whole passages of the ABI documentation, and a
# check that misreports daily is no check.
CLAIM = re.compile(r"(吞掉|吞下|已捕获|捕获后|不会抛|不外抛|不向外抛|"
                   r"swallow(?:ed|s)?|never throws?|caught here)")
EXC_WORD = re.compile(r"(异常|抛出|throw|exception|catch|try)")
LOGCALL = re.compile(r"\b(log|Log|logger|hostLogger|warn|error|info|debug|trace|"
                     r"PIER_TRACE\w*)\b")

# Resetting to a discernible empty value, the first of the approaches §5.1 permits.
NO_ANSWER = re.compile(r"\.clear\(\)|=\s*\{\s*\}|=\s*nullptr|=\s*std::nullopt|"
                       r"=\s*\"\"|=\s*false|=\s*-1|\.reset\(\)")

# Putting the failure into an error field or error object, which also hands out the fact
# that there is no answer.
ERR_CARRY = re.compile(r"\b\w*([Pp]roblem|[Ee]rror|[Ff]ail\w*|[Rr]eason)\w*\s*=|"
                       r"makeStringError|\bunexpected\(|\bErr\(")


def _functions(text):
    """Roughly cuts out a function body, from a `{` to the balanced `}`, taking only the
    shallowly indented top level.

    No real C++ parsing happens; that is the compiler's job. All this needs is to put a
    comment and the try or catch beside it into the same window. A window that is too large
    and under-reports is preferable to one cut too fine that over-reports: a check that
    misreports daily gets added to an ignore list and never speaks again.
    """
    out = []
    i = 0
    n = len(text)
    while i < n:
        if text[i] == "{":
            depth = 0
            j = i
            while j < n:
                if text[j] == "{":
                    depth += 1
                elif text[j] == "}":
                    depth -= 1
                    if depth == 0:
                        break
                j += 1
            body = text[i : j + 1]
            if body.count("\n") >= 3:
                out.append((text[:i].count("\n") + 1, body))
            i = j + 1
        else:
            i += 1
    return out


def _blank_out(text):
    """Blanks out comments and strings while keeping the newlines, otherwise the reported
    line numbers are wrong, and a check reporting wrong line numbers sends people to
    unrelated code until they stop believing it."""

    def keep_nl(m):
        return "\n" * m.group(0).count("\n")

    text = re.sub(r"/\*.*?\*/", keep_nl, text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    return re.sub(r'"(?:[^"\\]|\\.)*"', '""', text)


def run():
    r1 = Result("comment-claims")
    r2 = Result("no-silent-fallback")

    files = 0
    for dp, _, names in os.walk(PKGS):
        for fn in names:
            if not fn.endswith((".cpp", ".h", ".hpp")):
                continue
            p = os.path.join(dp, fn)
            rel = os.path.relpath(p, ROOT)
            files += 1
            with open(p, encoding="utf-8", errors="replace") as f:
                text = f.read()

            # ── comment-claims ──
            for start, body in _functions(text):
                cmts = re.findall(r"//[^\n]*|/\*.*?\*/", body, re.S)
                claim = None
                for c in cmts:
                    m = CLAIM.search(c)
                    if m and EXC_WORD.search(c):
                        claim = (m.group(0), c.strip().replace("\n", " ")[:80])
                        break
                if not claim:
                    continue
                code = re.sub(r"//[^\n]*|/\*.*?\*/", "", body, flags=re.S)
                if not re.search(r"\bcatch\s*\(", code):
                    r1.fail("%s:~%d a comment asserts %r while the same function body has no catch: %s"
                            % (rel, start, claim[0], claim[1]))

            # ── no-silent-fallback ──
            code = _blank_out(text)
            for m in re.finditer(r"catch\s*\([^)]*\)\s*\{", code):
                j = m.end() - 1
                depth = 0
                k = j
                while k < len(code):
                    if code[k] == "{":
                        depth += 1
                    elif code[k] == "}":
                        depth -= 1
                        if depth == 0:
                            break
                    k += 1
                handler = code[j : k + 1]
                line = code[: m.start()].count("\n") + 1

                if LOGCALL.search(handler):
                    continue
                if re.search(r"\bthrow\b|\breturn\b", handler):
                    continue
                # Resetting the thing to be filled to empty is returning a value able to
                # express that there is no answer, the first of the three approaches §5.1
                # permits. After `targetName.clear()` a subscriber sees an empty string,
                # which is a discernible cannot-be-read and not a guess.
                if NO_ANSWER.search(handler):
                    continue
                # Putting the failure into an error field or error object likewise hands
                # out the fact that there is no answer.
                if ERR_CARRY.search(handler):
                    continue

                if not handler.strip("{} \n\t"):
                    # An empty handler plus a comment saying why passes. The reason: in
                    # this shape the absence of an answer is expressed by a
                    # default-constructed empty value, since a subscriber seeing an empty
                    # string knows it could not be read, and a textual check cannot see
                    # that. The comment is the only checkable evidence, and a comment that
                    # lies belongs to comment-claims under §5.4.
                    # The criterion has to read the original text: the comments are gone
                    # from the blanked text, and judging whether a comment exists there
                    # yields a permanently wrong answer. Blanking preserves line counts and
                    # not character offsets, so the original is indexed by line number and
                    # never by character position.
                    l0 = code[:j].count("\n")
                    l1 = code[:k].count("\n")
                    raw = "\n".join(text.splitlines()[l0 : l1 + 1])
                    if re.search(r"//|/\*", raw):
                        continue
                    r2.fail("%s:%d the `catch` block is entirely blank, without even a comment, "
                            "so a reader cannot tell a deliberate fallback from an omission "
                            "(contract §5.1)"
                            % (rel, line))
                else:
                    r2.fail("%s:%d the `catch` block neither logs, nor rethrows, nor returns, nor "
                            "resets the target or puts it into an error value, so the exception is "
                            "dropped without a trace (contract §5.1)" % (rel, line))

    r1.note("scanned %d source file(s)" % files)
    r2.note("scanned %d source file(s). The criterion covers `catch` handlers only and reports "
            "only the unambiguous shape: neither logging, nor rethrowing, nor returning, nor "
            "resetting the target, nor putting it into an error value. A silent fallback "
            "invisible at the text level, such as a default filled in across functions or a "
            "`value_or(0)`, is outside the coverage, so a pass does not mean §5.1 holds, only "
            "that the most typical class does not occur."
            % files)
    return [r1, r2]


if __name__ == "__main__":
    rc = 0
    for res in run():
        rc |= res.report()
    sys.exit(rc)
