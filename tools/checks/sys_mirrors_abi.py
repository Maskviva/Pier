# -*- coding: utf-8 -*-
"""sys-mirrors-abi —— `pier-sys-rs` 的槽序与常量必须和 `abi.h` 逐格对上。

盯的是什么：这份镜像是**全手写**的（没有 bindgen，理由见 abi.h 的文件头 ——
生成的绑定读不出「为什么」，而这份契约的注释就是产品的一部分）。手写镜像的
失效方式只有一种，但它是最坏的那一种：

    abi.h 在表尾追加了一个槽，镜像忘了跟 → 之后每一槽都错位一格 →
    调用 `bus_publish` 实际打到别的函数指针上 → **无诊断的内存错乱**。

两侧各自都编得过。编译器看不见这件事，clippy 也看不见 —— 它正是 §九 说的
「才值得写脚本」的那一类，而且是这张清单上后果最严重的一条。

## 判据

1. **槽序逐格相同。** 名字和顺序都要对，不只是数量 —— 数量相同而两个槽调了
   个个儿，是最难查的那种。

2. **每个槽的签名逐参数相同。** 这一条是真机编译之后补的，理由值得写清楚：
   `cargo check` 只逮得到「`int` 不是 Rust 类型」这种拼写错。它逮**不到**
   镜像写 `i32` 而 `abi.h` 是 `int64_t` —— 两边各自都编得过，运行期读到的
   是半个数（小端上是低 32 位，值看起来还挺合理）。那是和槽位错位同一个
   家族的失效：无诊断、症状离根因很远。所以比对必须下到类型。
2. **`PIER_*` 常量逐个相同。** 值也要对：`PIER_DIMRULE_FIRE_SPREAD` 在两侧
   差一个数，症状是「关了刷怪，火焰蔓延停了」。
4. **`abi.h` 里每一个 `enum` 成员，镜像里都要有同名同值的常量。**
   这一条是被现实逼出来的：`PierActorAction` 的 33 个成员里，
   `PIER_AACT_ADD_EFFECT` 因为注释换行而在人工搬运时被漏掉了一个 ——
   而漏一个常量的症状是「调 add_effect 实际执行了 remove_effect」，
   因为下游会拿相邻的那个值去顶。原本这一条打算靠「人工对读」保证，
   一次就漏了，所以改成机器查。

5. **镜像里不许有条件编译。** 契约 §2.1：布局在所有目标下相同，镜像因此
   不需要任何 `#[cfg]`。出现 cfg 就意味着有人在镜像里重建了分岔 ——
   而分岔正是 v1 那次「两侧错位 7 槽」的根因。

## 这条检查**不**能证明什么

它比对的是文本。「镜像真的能编过」「函数签名的类型真的一致」要靠
`cargo check` 和一次真正的加载。签名比对没做，因为把 C 的函数指针类型和
镜像里的写法做等价判断需要一个真正的 C 解析器 —— 那是另一个量级的工程，
而槽序错位是实际发生过的失效，签名不一致目前只是理论风险。

`pier-sys-rs` 还没落地时这条报 SKIP 而不是 PASS：**没有镜像不等于镜像正确**。
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import (  # noqa: E402
    ROOT, Result, defines, read_abi, same_value, slots_of, strip_comments, struct_body,
)

SYS = os.path.join(ROOT, "bindings", "rust", "pier-sys-rs", "src")


# C 类型 → 唯一正确的 Rust 拼写。**一对一**，没有「也可以写成」——
# 允许两种拼法就等于允许它们哪天分叉。
CTYPE = {
    "void": "c_void", "bool": "bool", "char": "c_char",
    "int8_t": "i8", "int16_t": "i16", "int32_t": "i32", "int64_t": "i64",
    "uint8_t": "u8", "uint16_t": "u16", "uint32_t": "u32", "uint64_t": "u64",
    "size_t": "usize", "float": "f32", "double": "f64",
}


def _c_to_rust(ctype):
    """把一个 C 参数/返回类型翻成它在镜像里应有的拼写。翻不了返回 None。"""
    t = " ".join(ctype.split())
    stars = t.count("*")
    is_const = "const" in t
    base = t.replace("*", "").replace("const", "").strip()
    base = CTYPE.get(base, base)  # 非基本类型 = ABI 自己的类型名，原样
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
    """从 `ret (*name)(args)` 得到 (槽名, 期望的 Rust 签名字符串)。"""
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


def _rust_field_types(text, name):
    """镜像里 `pub struct <name>` 的 字段名 -> 类型文本。"""
    m = re.search(r"pub\s+struct\s+%s\s*\{(.*?)\n\}" % name, text, re.S)
    if not m:
        return None
    body = re.sub(r"//[^\n]*", "", m.group(1))
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
    out = {}
    for line in body.splitlines():
        line = line.strip().rstrip(",")
        if not line:
            continue
        mm = re.match(r"(?:pub\s+)?(\w+)\s*:\s*(.+)$", line)
        if mm:
            out[mm.group(1)] = " ".join(mm.group(2).split())
    return out


def _rust_struct_fields(text, name):
    """取出镜像里 `pub struct <name> { ... }` 的字段名，按声明顺序。"""
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
        # 故意不是 PASS。没有镜像时这条性质**无从谈起**，而 PASS 会让
        # 交付说明里出现一个骗人的 ✓（契约 §九 的「脚本先行」正是防这个）。
        r.fail("bindings/rust/pier-sys-rs/ 还不存在 —— 镜像尚未落地，这条性质无从检查")
        return r

    text = ""
    for dp, _, fs in os.walk(SYS):
        for fn in fs:
            if fn.endswith(".rs"):
                text += open(os.path.join(dp, fn), encoding="utf-8", errors="replace").read() + "\n"

    src = read_abi()

    # ── 1. 槽序 ────────────────────────────────────────────────────
    want = slots_of(src, "PierApi")
    got = _rust_struct_fields(text, "PierApi")
    if got is None:
        r.fail("镜像里找不到 `pub struct PierApi`")
    else:
        r.note("abi.h %d 槽 vs 镜像 %d 槽" % (len(want), len(got)))
        n = min(len(want), len(got))
        drift = 0
        for i in range(n):
            if want[i] != got[i]:
                r.fail("第 %d 槽错位：abi.h 是 %r，镜像是 %r —— "
                       "从这一槽起后面全部打到错误的函数指针上" % (i, want[i], got[i]))
                drift += 1
                if drift >= 5:
                    r.fail("… 后续错位不再逐条列出（前面某一槽漏了才是根因）")
                    break
        if not drift and len(want) != len(got):
            side = "镜像少了" if len(got) < len(want) else "镜像多了"
            r.fail("%s %d 槽（前 %d 槽一致）—— 追加之后必须同步镜像"
                   % (side, abs(len(want) - len(got)), n))

    # ── 1b. 每个槽的签名逐参数相同 ─────────────────────────────────
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
            continue  # 表头那四个标量，上面已按名字比过
        have = got_types.get(name)
        if have is None:
            continue  # 槽序那一步已经报过了
        if " ".join(have.split()) != want_sig:
            r.fail("槽 %s 的签名不一致：\n        abi.h  → %s\n        镜像   → %s\n"
                   "      （类型宽度不同时两边都编得过，运行期读到半个数）"
                   % (name, want_sig, have))
            sig_bad += 1
            if sig_bad >= 8:
                r.fail("… 后续签名差异不再逐条列出")
                break
        else:
            sig_ok += 1
    if not sig_bad:
        r.note("%d 个槽的签名逐参数对上" % sig_ok)

    # ── 2. 两个握手结构体 ─────────────────────────────────────────
    for struct in ("PierModVTable", "PierStr", "PierLaneDesc"):
        want_f = slots_of(src, struct)
        got_f = _rust_struct_fields(text, struct)
        if got_f is None:
            r.fail("镜像里找不到 `pub struct %s`" % struct)
            continue
        if want_f != got_f:
            r.fail("%s 字段不一致：abi.h %s，镜像 %s" % (struct, want_f, got_f))

    # ── 3. 常量 ───────────────────────────────────────────────────
    want_d = defines(src)
    bad = 0
    for name, val in sorted(want_d.items()):
        # 类型写法不限：u32 / i32 / &str 都合法，所以类型那一段不能是 `\w+`。
        m = re.search(r"pub\s+const\s+%s\s*:\s*[^=]+=\s*([^;]+);" % name, text)
        if not m:
            r.fail("镜像里缺常量 %s" % name)
            bad += 1
            continue
        if not same_value(val, m.group(1)):
            r.fail("常量 %s 值不同：abi.h=%r 镜像=%r" % (name, val, m.group(1).strip()))
            bad += 1
    if not bad:
        r.note("%d 个 PIER_* 常量逐个对上" % len(want_d))

    # ── 4. 枚举成员逐个同名同值 ────────────────────────────────────
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
                r.fail("枚举 %s 的成员 %s 在镜像里没有对应常量 —— "
                       "下游会拿相邻的值去顶，症状是调了 A 却执行了 B" % (en, k))
                missing += 1
            elif v and not same_value(v, mm.group(1)):
                r.fail("枚举 %s 的成员 %s 值不同：abi.h=%r 镜像=%r"
                       % (en, k, v, mm.group(1).strip()))
                missing += 1
    if not missing:
        r.note("%d 个枚举成员逐个同名同值" % checked)

    # ── 5. 镜像里不许有条件编译 ────────────────────────────────────
    for i, line in enumerate(text.splitlines(), 1):
        if re.search(r"#\[cfg\(", line) and "test" not in line:
            r.fail("镜像第 %d 行有条件编译：%s —— 契约 §2.1 要求布局在所有目标下相同，"
                   "镜像出现分岔就是 v1「两侧错位 7 槽」的根因" % (i, line.strip()[:70]))
    if not any("#[cfg(" in l for l in text.splitlines()):
        r.note("镜像里零条件编译（布局无分岔）")

    return r


if __name__ == "__main__":
    sys.exit(run().report())
