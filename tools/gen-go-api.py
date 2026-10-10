#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""gen-go-api: writes bindings/go/levilamina/raw_gen.go, the typed Go layer over every slot.

Each slot without a callback becomes one method of `levilamina.Raw`, typed in Go: a PierStr is a
string, a selector a PlayerSel, an output pointer a result, a (ctx, PierStrSink) pair the
strings the slot sank, and the caller's own mod handle is filled in. Every method applies
both gates of contract §10 and returns a *NotProvidedError for a slot the host lacks. What a
slot's answer means is left to the caller, as in a sys layer: a bool result is returned as
it came, and the facades of the package interpret it.

A slot that takes a callback, a sink other than PierStrSink, a lane descriptor or a block
cell array is left to hand-written code, since its shape needs a decision a generator cannot
make; `--list-skipped` names them.

`--shadow FILE` also writes a C file calling every trampoline with the C types the Go code
passes, so a C compiler checks the generator's mapping without a Go toolchain.

Usage:
    python3 tools/gen-go-api.py [--check] [--shadow FILE] [--list-skipped]
"""

import os
import re
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "checks"))
from _abi import ROOT, read_abi  # noqa: E402

GEN_SLOTS = os.path.join(os.path.dirname(__file__), "gen-go-slots.py")
OUT = os.path.join(ROOT, "bindings", "go", "levilamina", "raw_gen.go")

# C type -> (Go parameter type, expression converting a Go value v to the C argument)
SCALARS = {
    "int32_t": ("int32", "C.int32_t({v})"),
    "int64_t": ("int64", "C.int64_t({v})"),
    "uint64_t": ("uint64", "C.uint64_t({v})"),
    "uint32_t": ("uint32", "C.uint32_t({v})"),
    "uint16_t": ("uint16", "C.uint16_t({v})"),
    "double": ("float64", "C.double({v})"),
    "float": ("float32", "C.float({v})"),
    "bool": ("bool", "C.bool({v})"),
    "PierStr": ("string", "str({v})"),
    "PierPlayerSel": ("PlayerSel", "{v}.c()"),
    "PierActorId": ("ActorID", "C.PierActorId({v})"),
    "PierContainerRef": ("ContainerRef", "{v}.c()"),
    "PierKvDbHandle": ("KvDbHandle", "C.PierKvDbHandle({v}.p)"),
    "PierKeyHandle": ("KeyHandle", "C.PierKeyHandle({v}.p)"),
    "PierPacketHookHandle": ("PacketHookHandle", "C.PierPacketHookHandle({v}.p)"),
    "PierListenerHandle": ("ListenerHandle", "C.PierListenerHandle({v}.p)"),
}
# Output pointers: C pointee -> (Go result type, conversion of the C variable o)
OUTS = {
    "double": ("float64", "float64({o})"),
    "int32_t": ("int32", "int32({o})"),
    "int64_t": ("int64", "int64({o})"),
    "uint32_t": ("uint32", "uint32({o})"),
    "bool": ("bool", "bool({o})"),
    "PierActorId": ("ActorID", "ActorID({o})"),
}
RETS = {
    "bool": ("bool", "bool({r})"),
    "int32_t": ("int32", "int32({r})"),
    "int64_t": ("int64", "int64({r})"),
    "uint32_t": ("uint32", "uint32({r})"),
    "uint64_t": ("uint64", "uint64({r})"),
    "double": ("float64", "float64({r})"),
    "PierPlayerPos": ("PlayerPos", "playerPosOf({r})"),
    "PierKvDbHandle": ("KvDbHandle", "KvDbHandle{{p: unsafe.Pointer({r})}}"),
    "PierKeyHandle": ("KeyHandle", "KeyHandle{{p: unsafe.Pointer({r})}}"),
    "PierPacketHookHandle": ("PacketHookHandle", "PacketHookHandle{{p: unsafe.Pointer({r})}}"),
    "PierListenerHandle": ("ListenerHandle", "ListenerHandle{{p: unsafe.Pointer({r})}}"),
}
# C stand-ins for the shadow test: a value of each C type the Go side produces.
SHADOW = {
    "int32_t": "0", "int64_t": "0", "uint64_t": "0", "uint32_t": "0", "uint16_t": "0",
    "double": "0.0", "float": "0.0f", "bool": "false", "PierStr": "s",
    "PierPlayerSel": "sel", "PierActorId": "(PierActorId)0", "PierContainerRef": "cref",
    "PierKvDbHandle": "(PierKvDbHandle)0", "PierKeyHandle": "(PierKeyHandle)0",
    "PierPacketHookHandle": "(PierPacketHookHandle)0", "PierListenerHandle": "(PierListenerHandle)0",
}


def load_slots():
    import importlib.util
    spec = importlib.util.spec_from_file_location("gen_go_slots", GEN_SLOTS)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod.slots(read_abi())


GO_WORDS = {"break", "case", "chan", "const", "continue", "default", "defer", "else",
            "fallthrough", "for", "func", "go", "goto", "if", "import", "interface", "map",
            "package", "range", "return", "select", "struct", "switch", "type", "var",
            "len", "cap", "new", "make", "copy", "string", "error", "byte", "rune", "api", "self",
            "h", "col", "r",
            # Package-level names the generated bodies call: a parameter spelled like one
            # hides it, and `handle(uintptr(h))` then calls a KeyHandle.
            "handle", "str", "goString", "notProvided", "bytesPtr", "int32sPtr", "playerPosOf",
            "collector", "cgo", "unsafe", "C", "logAt", "okOr", "lastOr", "noAnswer", "refused"}


def docs_and_names(src):
    """Slot name -> (the first paragraph of its abi.h comment, its parameter names).

    A slot's comment is the one that closes right above its declaration and opens on a
    line of its own. A section comment further up, or a trailing comment after the
    previous `;`, belongs to something else.
    """
    body = src[src.index("typedef struct PierApi"):src.index("} PierApi;")]
    out = {}
    decl = re.compile(r"^[ \t]*[^;{}/\n]*?\(\s*\*\s*(?P<name>\w+)\s*\)\s*\((?P<params>[^;]*?)\)\s*;", re.M)
    for m in decl.finditer(body):
        doc = ""
        before = body[:m.start()].rstrip()
        if before.endswith("*/"):
            start = before.rfind("/*")
            line = before[before.rfind("\n", 0, start) + 1:start]
            if not line.strip():
                doc = before[start + 2:-2]
                if doc.startswith("*"):
                    doc = doc[1:]
        lines = [re.sub(r"^\s*\*\s?", "", l).rstrip() for l in doc.splitlines()]
        para = []
        for l in lines:
            if not l.strip():
                if para:
                    break
                continue
            para.append(l.strip())
        params = re.sub(r"/\*.*?\*/", "", m.group("params"), flags=re.S).strip()
        names = []
        if params and params != "void":
            depth, cur, parts = 0, "", []
            for ch in params:
                depth += {"(": 1, ")": -1}.get(ch, 0)
                if ch == "," and depth == 0:
                    parts.append(cur)
                    cur = ""
                else:
                    cur += ch
            parts.append(cur)
            for part in parts:
                fp = re.search(r"\(\s*\*\s*(\w+)\s*\)", part)
                nm = fp.group(1) if fp else (re.findall(r"\w+", part) or [""])[-1]
                names.append(nm)
        out[m.group("name")] = (para, names)
    return out


def go_param(name, index):
    """A Go parameter name: abi.h's own when it has one that Go accepts, a<index> otherwise."""
    if not re.match(r"^[a-z_]\w*$", name or ""):
        return "a%d" % index
    parts = name.split("_")
    camel = parts[0] + "".join(p[:1].upper() + p[1:] for p in parts[1:])
    return camel + "Arg" if camel in GO_WORDS else camel


def ctype(param):
    return re.sub(r"\s*\ba\d+$", "", param).strip()


def go_name(slot):
    return "".join(p[:1].upper() + p[1:] for p in slot.split("_"))


def plan(ret, name, params, names=()):
    """The Go shape of one slot, or the reason it is left to hand-written code."""
    types = [ctype(p) for p in params]
    names = list(names) + [""] * (len(types) - len(names))
    if len(set(types)) and any(t in ("int32_t", "uint8_t const*") for t in types):
        pass
    args, outs, ins, shadow, sink = [], [], [], [], False
    i = 0
    while i < len(types):
        t = types[i]
        nxt = types[i + 1] if i + 1 < len(types) else ""
        if t == "PierModHandle":
            args.append("self")
            shadow.append("(PierModHandle)0")
        elif t == "void*" and nxt == "PierStrSink":
            args += ["handle(uintptr(h))", "C.piergo_str_sink()"]
            shadow += ["(void*)0", "(PierStrSink)0"]
            sink = True
            i += 1
        elif t in ("uint8_t const*",) and nxt == "size_t":
            n = go_param(names[i], i)
            ins.append((n, "[]byte"))
            args += ["bytesPtr(%s)" % n, "C.size_t(len(%s))" % n]
            shadow += ["(uint8_t const*)0", "(size_t)0"]
            i += 1
        elif t == "int32_t const*" and nxt == "int32_t":
            n = go_param(names[i], i)
            ins.append((n, "[]int32"))
            args += ["int32sPtr(%s)" % n, "C.int32_t(len(%s))" % n]
            shadow += ["(int32_t const*)0", "0"]
            i += 1
        elif t.endswith("*") and t[:-1].strip() in OUTS:
            pointee = t[:-1].strip()
            outs.append(pointee)
            cvar = "o%d" % (len(outs) - 1)
            args.append("&" + cvar)
            shadow.append("&o_%s_%d" % (re.sub(r"\W", "_", pointee), len(outs) - 1))
        elif t in SCALARS:
            n = go_param(names[i], i)
            ins.append((n, SCALARS[t][0]))
            args.append(SCALARS[t][1].format(v=n))
            shadow.append(SHADOW[t])
        else:
            return None, "parameter type %s" % t
        i += 1
    if ret != "void" and ret not in RETS:
        return None, "return type %s" % ret
    return dict(args=args, outs=outs, ins=ins, shadow=shadow, sink=sink, ret=ret), None


def render_method(name, p, doc=()):
    gname = go_name(name)
    sig_in = ", ".join("%s %s" % (n, t) for n, t in p["ins"])
    results = [OUTS[o][0] for o in p["outs"]]
    if p["sink"]:
        results.append("[]string")
    if p["ret"] != "void":
        results.append(RETS[p["ret"]][0])
    results.append("error")
    zero = {"float64": "0", "int32": "0", "int64": "0", "uint32": "0", "uint64": "0", "bool": "false",
            "ActorID": "0", "[]string": "nil", "PlayerPos": "PlayerPos{}", "KvDbHandle": "KvDbHandle{}",
            "KeyHandle": "KeyHandle{}", "PacketHookHandle": "PacketHookHandle{}",
            "ListenerHandle": "ListenerHandle{}", "error": "nil"}
    zeros = ", ".join(zero[r] for r in results[:-1])
    lines = ["// %s calls the %s slot." % (gname, name)]
    if doc:
        lines.append("//")
        lines += ["// " + l for l in doc]
    lines += [
             "func (RawAPI) %s(%s) %s {" % (gname, sig_in, results[0] if len(results) == 1 else "(%s)" % ", ".join(results)),
             "\tif api == nil || !bool(C.piergo_has_%s(api)) {" % name,
             "\t\treturn %snotProvided(\"%s\")" % (zeros + ", " if zeros else "", name),
             "\t}"]
    for k, o in enumerate(p["outs"]):
        lines.append("\tvar o%d C.%s" % (k, o))
    if p["sink"]:
        lines += ["\tcol := &collector{}", "\th := cgo.NewHandle(col)", "\tdefer h.Delete()"]
    call = "C.piergo_call_%s(%s)" % (name, ", ".join(["api"] + p["args"]))
    rets = [OUTS[o][1].format(o="o%d" % k) for k, o in enumerate(p["outs"])]
    if p["sink"]:
        rets.append("col.items")
    if p["ret"] != "void":
        lines.append("\tr := " + call)
        rets.append(RETS[p["ret"]][1].format(r="r"))
    else:
        lines.append("\t" + call)
    rets.append("nil")
    lines.append("\treturn " + ", ".join(rets))
    lines.append("}")
    return "\n".join(lines)


def render(slots):
    done, skipped = [], []
    info = docs_and_names(read_abi())
    for ret, name, params in slots:
        doc, names = info.get(name, ((), ()))
        p, why = plan(ret, name, params, names)
        if p is None:
            skipped.append((name, why))
        else:
            p["doc"] = doc
            done.append((name, p))
    head = '''// Code generated by tools/gen-go-api.py from sdk/abi.h. DO NOT EDIT.

package levilamina

/*
#include "sdk/abi.h"
#include "slots_gen.h"
#include "glue.h"
*/
import "C"

import (
	"runtime/cgo"
	"unsafe"
)

// RawAPI reaches every slot without a callback, typed in Go and gated: a slot the host lacks
// is a *NotProvidedError. A result means what abi.h says it means; the rest of the package
// builds the interpreted API on top of it. Use it through Raw.
type RawAPI struct{}

// Raw is the typed slot layer. Prefer the facades of this package where one exists.
var Raw RawAPI

var _ = unsafe.Pointer(nil)
var _ = cgo.Handle(0)
'''
    body = "\n\n".join(render_method(n, p, p.get("doc", ())) for n, p in done)
    return head + "\n" + body + "\n", done, skipped


def render_shadow(done):
    lines = ['#include "slots_gen.h"', "void shadow(const PierApi* api) {",
             "    PierStr s = {0}; PierPlayerSel sel = {0}; PierContainerRef cref = {0};"]
    seen = set()
    for _, p in done:
        for k, o in enumerate(p["outs"]):
            v = "o_%s_%d" % (re.sub(r"\W", "_", o), k)
            if v not in seen:
                seen.add(v)
                lines.append("    %s %s = 0;" % (o, v))
    for name, p in done:
        lines.append("    (void)piergo_call_%s(%s);" % (name, ", ".join(["api"] + p["shadow"])))
    lines.append("}")
    return "\n".join(lines) + "\n"


def main():
    text, done, skipped = render(load_slots())
    if "--list-skipped" in sys.argv:
        for n, why in skipped:
            print("%-36s %s" % (n, why))
        return 0
    if "--shadow" in sys.argv:
        with open(sys.argv[sys.argv.index("--shadow") + 1], "w", encoding="utf-8") as f:
            f.write(render_shadow(done))
    if "--check" in sys.argv:
        current = open(OUT, encoding="utf-8").read() if os.path.exists(OUT) else ""
        if current != text:
            print("raw_gen.go is stale against abi.h; run python3 tools/gen-go-api.py")
            return 1
        print("raw_gen.go matches abi.h (%d slot(s), %d left to hand-written code)"
              % (len(done), len(skipped)))
        return 0
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    print("wrote %s: %d slot(s), %d left to hand-written code" % (os.path.relpath(OUT, ROOT), len(done), len(skipped)))
    return 0


PROPS_OUT = os.path.join(ROOT, "bindings", "go", "levilamina", "props_gen.go")

# prefix, Go constant prefix, receiver type, receiver variable, kind, raw call
FAMILIES = [
    ("PIER_PPROP_", "PProp", "Player", "p", "num", "Raw.PlayerGetNum(p.Sel, {c})", "Raw.PlayerSetNum(p.Sel, {c}, v)"),
    ("PIER_PSTR_", "PStr", "Player", "p", "str", "Raw.PlayerGetStr(p.Sel, {c})", None),
    ("PIER_PACT_", "PAct", "Player", "p", "act", "Raw.PlayerAction(p.Sel, {c}, sarg, a, b, c)", None),
    ("PIER_APROP_", "AProp", "Entity", "e", "num", "Raw.ActorGetNum(e.ID, {c})", None),
    ("PIER_ASTR_", "AStr", "Entity", "e", "str", "Raw.ActorGetStr(e.ID, {c})", None),
    ("PIER_AACT_", "AAct", "Entity", "e", "act", "Raw.ActorAction(e.ID, {c}, sarg, a, b, c)", None),
    ("PIER_BPROP_", "BProp", "BlockAt", "b", "num", "Raw.BlockGetNum(b.Dim, b.X, b.Y, b.Z, {c})", None),
    ("PIER_BSTR_", "BStr", "BlockAt", "b", "str", "Raw.BlockGetStr(b.Dim, b.X, b.Y, b.Z, {c})", None),
    ("PIER_BACT_", "BAct", "BlockAt", "b", "bact", "Raw.BlockAction(b.Dim, b.X, b.Y, b.Z, {c}, sarg)", None),
    ("PIER_IPROP_", "IProp", "Item", "i", "num", "Raw.ItemGetNum(i.SNBT, {c})", None),
    ("PIER_ISTR_", "IStr", "Item", "i", "str", "Raw.ItemGetStr(i.SNBT, {c})", None),
    ("PIER_SRV_", "Srv", None, None, "str", "Raw.ServerInfoStr({c})", None),
    ("PIER_SYS_", "Sys", None, None, "str", "Raw.SysInfoStr({c})", None),
]


def enum_members(src):
    """(name, value, comment) for every PIER_ enum member, the comment being the one that
    follows the member, as abi.h writes them."""
    out = []
    for body in re.findall(r"enum\s+\w+\s*\{(.*?)\n\}", src, re.S):
        for m in re.finditer(r"(PIER_\w+)\s*=\s*(-?\d+)\s*,?\s*(?:/\*(.*?)\*/)?", body, re.S):
            doc = " ".join(l.strip().lstrip("*").strip() for l in (m.group(3) or "").splitlines())
            out.append((m.group(1), int(m.group(2)), " ".join(doc.split())))
    return out


def camel(words):
    return "".join(w[:1] + w[1:].lower() for w in words.split("_"))


def hand_written_methods():
    """Type -> method names declared in the package's hand-written files."""
    names = {}
    pkg = os.path.dirname(PROPS_OUT)
    for fn in os.listdir(pkg):
        if fn.endswith(".go") and not fn.endswith("_gen.go"):
            text = open(os.path.join(pkg, fn), encoding="utf-8").read()
            for t, n in re.findall(r"^func \(\w+ \*?(\w+)\) (\w+)\(", text, re.M):
                names.setdefault(t, set()).add(n)
            for n in re.findall(r"^func (\w+)\(", text, re.M):
                names.setdefault(None, set()).add(n)
    return names


def render_props(src):
    members = enum_members(src)
    taken = hand_written_methods()
    used = {}
    consts, methods = [], []
    for prefix, cprefix, recv, var, kind, call, setcall in FAMILIES:
        fam = [(n, v, d) for n, v, d in members if n.startswith(prefix)]
        if not fam:
            raise SystemExit("no constant starts with %s" % prefix)
        # One declaration each, its comment on the line above: gofmt aligns the trailing
        # comments of a const block in columns, and a generator spelling that alignment
        # out would have to reproduce gofmt exactly.
        for n, v, d in fam:
            cn = cprefix + camel(n[len(prefix):])
            consts.append("// %s is %s: %s" % (cn, n, d or n))
            consts.append("const %s int32 = %d\n" % (cn, v))
        for n, v, d in fam:
            base = camel(n[len(prefix):])
            cname = cprefix + base
            doc = d or n
            if "unsupported since" in doc:
                note = "\n//\n// The host answers no on every current engine version: " + doc
            else:
                note = ""
            mine = used.setdefault(recv, set()) | taken.get(recv, set())
            name = base
            if kind == "str" and name in mine:
                name += "Text"
            if kind in ("act", "bact") and name in mine:
                name += "Action"
            if recv is None:
                # Server and System, not Srv and Sys: the constants already have those names,
                # and a function spelled like a constant is a redeclaration.
                name = {"Srv": "Server", "Sys": "System"}[cprefix] + base
            if name in mine:
                raise SystemExit("%s.%s would be declared twice" % (recv, name))
            used.setdefault(recv, set()).add(name)
            head = "func (%s %s) %s" % (var, recv, name) if recv else "func %s" % name
            c = call.format(c=cname)
            if kind == "num":
                is_bool = re.match(r"^(Is|Has|Can)[A-Z]", base) is not None
                rt = "bool" if is_bool else "float64"
                methods.append("// %s reads %s: %s%s" % (name, n, doc, note))
                methods.append("%s() (%s, error) {" % (head, rt))
                methods.append("\tv, ok, err := " + c)
                methods.append("\tif err != nil {\n\t\treturn %s, err\n\t}" % ("false" if is_bool else "0"))
                methods.append("\tif !ok {\n\t\treturn %s, noAnswer(%s)\n\t}" % ("false" if is_bool else "0", '"%s"' % n))
                methods.append("\treturn %s, nil\n}\n" % ("v != 0" if is_bool else "v"))
                if setcall and "(S)" in doc:
                    sname = "Set" + base
                    if sname in mine or sname in used.get(recv, set()):
                        raise SystemExit("%s.%s would be declared twice" % (recv, sname))
                    used[recv].add(sname)
                    methods.append("// %s writes %s: %s" % (sname, n, doc))
                    methods.append("func (%s %s) %s(v float64) error {" % (var, recv, sname))
                    methods.append("\tok, err := " + setcall.format(c=cname))
                    methods.append("\tif err != nil {\n\t\treturn err\n\t}")
                    methods.append("\tif !ok {\n\t\treturn refused(%s)\n\t}" % ('"%s"' % n))
                    methods.append("\treturn nil\n}\n")
            elif kind == "str":
                methods.append("// %s reads %s: %s%s" % (name, n, doc, note))
                methods.append("%s() (string, error) {" % head)
                methods.append("\tout, ok, err := " + c)
                methods.append("\tif err != nil {\n\t\treturn \"\", err\n\t}")
                methods.append("\tif !ok || len(out) == 0 {\n\t\treturn \"\", noAnswer(%s)\n\t}" % ('"%s"' % n))
                methods.append("\treturn out[len(out)-1], nil\n}\n")
            else:
                params = "sarg string" if kind == "bact" else "sarg string, a, b, c float64"
                methods.append("// %s runs %s: %s%s" % (name, n, doc, note))
                methods.append("//\n// The output, when the verb has one, is returned; abi.h names each argument.")
                methods.append("func (%s %s) %s(%s) (string, error) {" % (var, recv, name, params))
                methods.append("\tout, ok, err := " + c)
                methods.append("\tif err != nil {\n\t\treturn \"\", err\n\t}")
                methods.append("\tif !ok {\n\t\treturn \"\", refused(%s)\n\t}" % ('"%s"' % n))
                methods.append("\tif len(out) == 0 {\n\t\treturn \"\", nil\n\t}")
                methods.append("\treturn out[len(out)-1], nil\n}\n")
    head = ("// Code generated by tools/gen-go-api.py from sdk/abi.h. DO NOT EDIT.\n\n"
            "package levilamina\n\n")
    return head + "\n".join(consts) + "\n" + "\n".join(methods)


def props_main():
    text = render_props(read_abi())
    if "--check" in sys.argv:
        current = open(PROPS_OUT, encoding="utf-8").read() if os.path.exists(PROPS_OUT) else ""
        if current != text:
            print("props_gen.go is stale against abi.h; run python3 tools/gen-go-api.py")
            return 1
        return 0
    with open(PROPS_OUT, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    print("wrote %s: %d method(s)" % (os.path.relpath(PROPS_OUT, ROOT), text.count("\nfunc ")))
    return 0


if __name__ == "__main__":
    rc = main()
    if "--list-skipped" not in sys.argv:
        rc = rc or props_main()
    sys.exit(rc)
