# -*- coding: utf-8 -*-
"""sys-mirrors-abi: the slot order and constants of `pier-sys-rs` must match `abi.h` cell
for cell.

What it watches: the mirror is written entirely by hand, with no bindgen, for the reason
the file header of abi.h gives, that a generated binding cannot carry the reasons while
the comments of this contract are part of the product. A hand-written mirror has one
failure mode, and it is the worst one:

    abi.h appends a slot at the end, the mirror does not follow, every slot after it is
    off by one, a call to `bus_publish` lands on a different function pointer, and memory
    is corrupted with no diagnostic.

Both sides compile on their own. The compiler cannot see it and clippy cannot see it,
which makes it the kind §9 calls worth a script, and the one with the worst consequence on
that list.

## The criteria

1. The slot order matches cell for cell. Names and order both, not merely the count: an
   equal count with two slots swapped is the hardest case to find.
2. Every slot signature matches parameter by parameter. This was added after a real
   compile, for a reason worth stating: `cargo check` catches only a spelling error such
   as `int` not being a Rust type. It does not catch the mirror writing `i32` where
   `abi.h` has `int64_t`, since both sides compile and half a number is read at runtime,
   the low 32 bits on a little-endian target, whose value still looks plausible. That is
   the same family of failure as a misaligned slot: no diagnostic, and the symptom far
   from the cause. The comparison therefore has to reach down to the types.
3. Every `PIER_*` constant matches. The values too: `PIER_DIMRULE_FIRE_SPREAD` differing
   by one between the sides shows up as turning off mob spawning and stopping fire spread.
4. Every `enum` member in `abi.h` has a constant of the same name and value in the mirror.
   Reality forced this one: among the 33 members of `PierActorAction`,
   `PIER_AACT_ADD_EFFECT` was missed during manual transcription because of a wrapped
   comment, and a missing constant shows up as calling add_effect while remove_effect
   runs, since downstream substitutes the adjacent value. This was originally meant to
   rest on reading by hand, which missed one on the first try, so it became a machine
   check.
5. The mirror carries no conditional compilation. Contract §2.1: the layout is identical
   on every target, so the mirror needs no `#[cfg]`. A cfg appearing means someone rebuilt
   a fork inside the mirror, and a fork was the root cause of the seven-slot misalignment
   in v1.

## What this check cannot prove

It compares text. Whether the mirror really compiles, and whether a load really succeeds,
rest on `cargo check` and a real load. The signature comparison of criterion 2 works
through a fixed one-to-one table from C spellings to Rust spellings and is not a C parser,
so any equivalence that table cannot express is outside its coverage.

While `pier-sys-rs` has not landed, this reports SKIP and not PASS: no mirror does not mean
a correct mirror.
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import (  # noqa: E402
    ROOT, Result, defines, read_abi, same_value, slots_of, strip_comments, struct_body,
)

SYS = os.path.join(ROOT, "bindings", "rust", "pier-sys-rs", "src")


# A C type to the one correct Rust spelling. Strictly one to one, with no alternative
# spelling: allowing two spellings allows them to diverge one day.
CTYPE = {
    "void": "c_void", "bool": "bool", "char": "c_char",
    "int8_t": "i8", "int16_t": "i16", "int32_t": "i32", "int64_t": "i64",
    "uint8_t": "u8", "uint16_t": "u16", "uint32_t": "u32", "uint64_t": "u64",
    "size_t": "usize", "float": "f32", "double": "f64",
}


def _c_to_rust(ctype):
    """Translates one C parameter or return type into the spelling the mirror should carry. Returns None when it cannot."""
    t = " ".join(ctype.split())
    stars = t.count("*")
    is_const = "const" in t
    base = t.replace("*", "").replace("const", "").strip()
    base = CTYPE.get(base, base)  # A non-fundamental type is an ABI type name, kept as is
    if stars == 0:
        return base
    for _ in range(stars):
        base = ("*const " if is_const else "*mut ") + base
    return base


def _split_params(p):
    out, depth, cur = [], 0, ""
    for ch in p:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur)
            cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur)
    return [x.strip() for x in out if x.strip()]


def _c_slot_signature(decl):
    """From `ret (*name)(args)`, returns (slot name, the expected Rust signature string)."""
    decl = " ".join(decl.split())
    m = re.match(r"^(.*?)\(\s*\*\s*(\w+)\s*\)\s*\((.*)\)$", decl)
    if not m:
        return None, None
    ret, name, params = m.group(1).strip(), m.group(2), m.group(3).strip()
    ps = []
    for p in _split_params(params):
        if p in ("void", ""):
            continue
        stars = p.count("*")
        toks = p.replace("*", " ").split()
        if (len(toks) >= 2 and re.match(r"^\w+$", toks[-1])
                and toks[-1] not in CTYPE and not toks[-1].endswith("_t")):
            base = " ".join(toks[:-1])
        else:
            base = " ".join(toks)
        ptype = base + ("*" * stars)
        if "const" in p and "const" not in ptype:
            ptype = "const " + ptype
        ps.append(_c_to_rust(ptype))
    sig = 'Option<unsafe extern "C" fn(' + ", ".join(ps) + ")"
    if ret != "void":
        sig += " -> " + _c_to_rust(ret)
    sig += ">"
    return name, sig


def _split_fields(body):
    """Splits a struct body on the commas that separate fields.

    A comma inside `<>`, `()` or `[]` belongs to a type and does not end a field, so
    depth is tracked. Splitting on every comma, or reading the body a line at a time,
    truncates a wrapped signature to its first line: rustfmt breaks a long function
    pointer across lines and the field then reads as the single token `Option<`. That
    parses cleanly, compares unequal against the header, and reports a signature
    mismatch that does not exist.
    """
    parts, buf, depth, i = [], [], 0, 0
    while i < len(body):
        ch = body[i]
        # The `>` of a `->` closes nothing. Counting it as a closing bracket drives the
        # depth negative on the first return type and merges every field after it into
        # one.
        if ch == "-" and body[i : i + 2] == "->":
            buf.append("->")
            i += 2
            continue
        if ch in "<([":
            depth += 1
        elif ch in ">)]":
            depth -= 1
        elif ch == "," and depth == 0:
            parts.append("".join(buf))
            buf = []
            i += 1
            continue
        buf.append(ch)
        i += 1
    parts.append("".join(buf))
    return parts


def _rust_field_types(text, name):
    """Maps a field name to its type text for `pub struct <name>` in the mirror."""
    m = re.search(r"pub\s+struct\s+%s\s*\{(.*?)\n\}" % name, text, re.S)
    if not m:
        return None
    body = re.sub(r"//[^\n]*", "", m.group(1))
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
    out = {}
    for field in _split_fields(body):
        field = field.strip()
        if not field:
            continue
        mm = re.match(r"(?:pub\s+)?(\w+)\s*:\s*(.+)$", field, re.S)
        if mm:
            # Collapse the whitespace rustfmt introduced, and drop the trailing comma
            # it leaves before a closing angle bracket, so that a wrapped signature and
            # a single-line one normalize to the same text.
            ty = " ".join(mm.group(2).split())
            ty = re.sub(r",\s*([>)\]])", r"\1", ty)
            ty = re.sub(r"([<(\[])\s+", r"\1", ty)
            ty = re.sub(r"\s+([>)\]])", r"\1", ty)
            out[mm.group(1)] = ty
    return out


def _rust_struct_fields(text, name):
    """Returns the field names of `pub struct <name> { ... }` in the mirror, in declaration order."""
    m = re.search(r"pub\s+struct\s+%s\s*\{(.*?)\n\}" % name, text, re.S)
    if not m:
        return None
    body = re.sub(r"//[^\n]*", "", m.group(1))
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
    out = []
    for line in body.split(","):
        line = line.strip()
        if not line:
            continue
        mm = re.match(r"(?:pub\s+)?(\w+)\s*:", line)
        if mm:
            out.append(mm.group(1))
    return out


def run():
    r = Result("sys-mirrors-abi")

    if not os.path.isdir(SYS):
        # Deliberately not a PASS. Without a mirror the property cannot be spoken of at
        # all, and a PASS would put a lying checkmark in a delivery note, which is exactly
        # what scripts-come-first in contract §9 prevents.
        r.fail("bindings/rust/pier-sys-rs/ does not exist yet, so the mirror has not landed and this property cannot be checked")
        return r

    text = ""
    for dp, _, fs in os.walk(SYS):
        for fn in fs:
            if fn.endswith(".rs"):
                text += open(os.path.join(dp, fn), encoding="utf-8", errors="replace").read() + "\n"

    src = read_abi()

    # 1. The slot order
    want = slots_of(src, "PierApi")
    got = _rust_struct_fields(text, "PierApi")
    if got is None:
        r.fail("`pub struct PierApi` was not found in the mirror")
    else:
        r.note("abi.h has %d slot(s) against %d in the mirror" % (len(want), len(got)))
        n = min(len(want), len(got))
        drift = 0
        for i in range(n):
            if want[i] != got[i]:
                r.fail("slot %d is misaligned: abi.h has %r and the mirror has %r, so from this "
                       "slot onward every call lands on the wrong function pointer" % (i, want[i], got[i]))
                drift += 1
                if drift >= 5:
                    r.fail("... further misalignments are not listed; the root cause is one earlier slot being missing")
                    break
        if not drift and len(want) != len(got):
            side = "the mirror is short by" if len(got) < len(want) else "the mirror has an extra"
            r.fail("%s %d slot(s), with the first %d matching. The mirror has to follow an append"
                   % (side, abs(len(want) - len(got)), n))

    # 2. Every slot signature matches parameter by parameter
    got_types = _rust_field_types(text, "PierApi") or {}
    body = strip_comments(struct_body(src, "PierApi"))
    sig_bad = 0
    sig_ok = 0
    for decl in body.split(";"):
        decl = decl.strip()
        if not decl:
            continue
        name, want_sig = _c_slot_signature(decl)
        if name is None:
            continue  # The four scalars in the header, already compared by name above
        have = got_types.get(name)
        if have is None:
            continue  # The slot-order step already reported this
        if " ".join(have.split()) != want_sig:
            r.fail("the signature of slot %s disagrees:\n        abi.h  -> %s\n        mirror -> %s\n"
                   "      With differing type widths both sides compile and half a number is read at runtime"
                   % (name, want_sig, have))
            sig_bad += 1
            if sig_bad >= 8:
                r.fail("... further signature differences are not listed")
                break
        else:
            sig_ok += 1
    if not sig_bad:
        r.note("the signatures of %d slot(s) match parameter by parameter" % sig_ok)

    # The two handshake structs
    for struct in ("PierModVTable", "PierStr", "PierLaneDesc"):
        want_f = slots_of(src, struct)
        got_f = _rust_struct_fields(text, struct)
        if got_f is None:
            r.fail("`pub struct %s` was not found in the mirror" % struct)
            continue
        if want_f != got_f:
            r.fail("the fields of %s disagree: abi.h has %s and the mirror has %s" % (struct, want_f, got_f))

    # 3. The constants
    want_d = defines(src)
    bad = 0
    for name, val in sorted(want_d.items()):
        # The type spelling is unconstrained, since u32, i32 and &str are all valid, so that part of the pattern cannot be `\w+`.
        m = re.search(r"pub\s+const\s+%s\s*:\s*[^=]+=\s*([^;]+);" % name, text)
        if not m:
            r.fail("the mirror is missing the constant %s" % name)
            bad += 1
            continue
        if not same_value(val, m.group(1)):
            r.fail("the constant %s has different values: abi.h=%r and the mirror=%r" % (name, val, m.group(1).strip()))
            bad += 1
    if not bad:
        r.note("all %d PIER_* constant(s) match one by one" % len(want_d))

    # 4. Every enum member matches by name and by value
    enum_names = re.findall(r"^enum\s+(\w+)\s*\{", src, re.M)
    enum_names += re.findall(r"typedef\s+enum\s+(\w+)\s*\{", src)
    missing = 0
    checked = 0
    for en in sorted(set(enum_names)):
        m = re.search(r"enum\s+%s\s*\{(.*?)\n\};" % en, src, re.S)
        if not m:
            continue
        body = re.sub(r"/\*.*?\*/", "", m.group(1), flags=re.S)
        body = re.sub(r"//[^\n]*", "", body)
        for item in body.split(","):
            item = item.strip()
            if not item:
                continue
            k, _, v = item.partition("=")
            k, v = k.strip(), v.strip()
            if not re.match(r"^PIER_\w+$", k):
                continue
            checked += 1
            mm = re.search(r"pub\s+const\s+%s\s*:\s*[^=]+=\s*([^;]+);" % re.escape(k), text)
            if not mm:
                r.fail("enum %s has a member %s with no matching constant in the mirror. "
                       "Downstream substitutes the adjacent value and the symptom is calling A "
                       "while B runs" % (en, k))
                missing += 1
            elif v and not same_value(v, mm.group(1)):
                r.fail("enum %s has a member %s with different values: abi.h=%r and the mirror=%r"
                       % (en, k, v, mm.group(1).strip()))
                missing += 1
    if not missing:
        r.note("all %d enum member(s) match by name and by value" % checked)

    # 5. The mirror carries no conditional compilation
    for i, line in enumerate(text.splitlines(), 1):
        if re.search(r"#\[cfg\(", line) and "test" not in line:
            r.fail("line %d of the mirror carries conditional compilation: %s. Contract §2.1 "
                   "requires the layout to be identical on every target, and a fork in the mirror "
                   "was the root cause of the seven-slot misalignment in v1" % (i, line.strip()[:70]))
    if not any("#[cfg(" in l for l in text.splitlines()):
        r.note("the mirror carries no conditional compilation, so the layout has no fork")

    return r


if __name__ == "__main__":
    sys.exit(run().report())
