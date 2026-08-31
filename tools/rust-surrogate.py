#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""rust-surrogate —— 没有 cargo 时对 `cargo check` 的**代偿**。

和 `link-surrogate.py` 同一个定位，理由也一样：这个仓库有一段时间是在
**没有 rustc 的环境**里写出来的。在那里，一个漏掉的逗号可以一路混到交付。

**它不是契约 §九 的检查**，有工具链之后就是冗余的。它能查的东西也少得可怜：

  * 括号 / 花括号 / 方括号配平（剥掉注释和字符串之后）
  * `pub const` / `pub type` 语句以分号收尾
  * `#[repr(C)]` 结构体的字段以逗号分隔
  * `mod` 声明指向真实存在的文件
  * **悬空的 `///`**（后面没有任何项）—— 手抄 FFI 镜像时最容易出的一种，
    因为 C 头文件里跨行的**尾注**属于上一项，原地搬过来就悬空了
  * **C 类型名残留**（`int` / `short` / `unsigned` / `long` / `size_t` …）——
    同样是手抄镜像的产物；`cargo check` 会报，但要等到有 rustc 的机器上
  * **未使用的 `use`** —— `-D warnings` 下是硬错。上一轮删掉一个
    `pub fn` 之后，它带进来的 `use ... TaskId` 立刻变成未使用；
    这类连锁是「删代码」最容易漏的一环
  * **零调用方的 `pub(crate)` 项** —— `cargo clippy -D warnings` 会以
    `never used` 报错。它拦住的是「铺了基础设施但没有调用方」，而那种
    helper 的 `# Safety` 断言从来没被任何真实调用点检验过

查不到的：类型是否存在、生命周期、trait 约束、借用检查 —— 也就是 Rust
真正会帮你挡住的绝大部分东西。**PASS 只意味着「没有显然的形状错误」。**

用法：
    python3 tools/rust-surrogate.py [crate 目录]
"""

import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

# 文本判据分不开的同名方法。**如实报出来** —— 一条检查说得清自己漏了什么，
# 比它假装什么都覆盖到了有用得多（契约 §九：PASS 只能给覆盖到的那部分打 ✓）。
AMBIGUOUS_NOTES = []


def strip(text):
    """剥注释与字符串字面量，保留换行以便报行号。

    字符串正则必须允许**跨行**：Rust 的 `"...\\` 反斜杠续行是合法的，
    只匹配单行的正则会在续行处停住，把后半截字符串当成代码。

    这一轮踩了三次同族的坑：在剥掉注释的文本里找注释、在剥掉字符串的
    文本里找 `#include "..."`、以及这次的剥不干净。统一的教训是：
    **先想清楚要找的东西会不会被自己剥掉，以及自己剥得干不干净。**
    """

    def keep_nl(m):
        return "\n" * m.group(0).count("\n")

    text = re.sub(r"/\*.*?\*/", keep_nl, text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    # `r` 前面必须是非标识符字符，否则 `Formatter"..."` 这种**跨了两个
    # 字符串**的片段会被当成一个 raw string 整段吞掉 —— 连同里面的括号。
    # crossbind 的 item_remap.rs 就是这么从 123/123 变成 120/122 的：
    # 文件里一个 raw string 都没有，全是这条正则自己造的。
    text = re.sub(r'(?<![A-Za-z0-9_])r#*"(?:.|\n)*?"#*', keep_nl, text)
    text = re.sub(r'"(?:[^"\\]|\\.)*"', keep_nl, text, flags=re.S)
    # **不要**试图剥字符字面量。
    #
    # Rust 的 `'a` 生命周期和字符字面量 `'x'` 前缀相同，任何只看引号的正则
    # 都会把 `Formatter<'_>) -> fmt::Result {` 里的 `'_>) -> fmt::Result {`
    # 当成一个字符字面量吞掉 —— 连同那个 `{`。
    #
    # 症状：**剥离器自己破坏了配平**。crossbind 的 item_remap.rs 原文
    # 123/123 配平，剥完变成 98/100，于是「不配平」这条判据报了一个
    # 它自己造出来的问题。
    #
    # 字符字面量里的括号极其罕见（`'{'` 这种），漏剥的代价远小于误剥。
    return text


def check_file(path, rel, problems):
    raw = open(path, encoding="utf-8", errors="replace").read()
    code = strip(raw)

    for o, c in (("(", ")"), ("{", "}"), ("[", "]")):
        if code.count(o) != code.count(c):
            problems.append("%s：%s%s 不配平（%d vs %d）" % (rel, o, c, code.count(o), code.count(c)))

    # `pub const X: T = <expr>;` —— 表达式可以跨行，所以按分号切而不是按行。
    #
    # `const fn` / `const unsafe fn` 是**函数**，不是常量项，不适用这条。
    #
    # 初始化表达式的括号可能是 `{`（结构体字面量）**也可能是 `[`**
    # （数组/切片字面量：`pub const VERSIONS: &[Version] = &[ ... ];`）。
    # 第一版只找 `{`，于是数组字面量被报成「缺分号」—— crossbind 的
    # versions.rs 就是这么被误报的。判据必须覆盖它想断言的全部形状。
    for m in re.finditer(r"\bpub\s+(const|static|type)\b(?!\s+(unsafe\s+)?(fn|extern)\b)", code):
        rest = code[m.start():]
        semi = rest.find(";")
        if semi < 0:
            line = code[: m.start()].count("\n") + 1
            problems.append("%s:%d %s 语句没有分号收尾" % (rel, line, m.group(1)))
            continue
        # 括号必须从 `=` **之后**开始找。类型注解里也有括号
        # （`pub const V: &[Version] = &[...];` 的 `&[Version]`），
        # 从头找会先撞上它，然后在错误的位置判配平。
        eq = rest.find("=")
        if eq < 0 or eq > semi:
            continue
        opens = [(rest.find(o, eq), o, c) for o, c in (("{", "}"), ("[", "]"))]
        opens = [(i, o, c) for i, o, c in opens if 0 <= i < semi]
        if not opens:
            continue
        i, o, c = min(opens)
        depth = 0
        k = i
        while k < len(rest):
            if rest[k] == o:
                depth += 1
            elif rest[k] == c:
                depth -= 1
                if depth == 0:
                    break
            k += 1
        if rest[k + 1 : k + 3].strip()[:1] != ";":
            line = code[: m.start()].count("\n") + 1
            problems.append("%s:%d %s 的初始化块之后缺分号" % (rel, line, m.group(1)))

    # 悬空的 `///`：后面找不到任何项。
    #
    # 这一条是被真机编译逼出来的。C 头里 `FOO = 26,` 后面跟一段跨行块注释，
    # 那段注释属于 **FOO**；手抄时原地搬过来就变成了下一项的前置注释，
    # 而最后一项后面没有下一项 —— 于是 `expected item after doc comment`。
    #
    # 更值得记的是：那个编译错误只是**运气**。前面每一条跨行尾注都同样挂错
    # 了对象，只是它们后面碰巧有项，编译器一声不吭。
    # 必须在**原文**里找 —— `code` 是剥掉注释之后的文本，在那里面找注释
    # 永远找不到。第一版就是这么写的，于是这条检查恒绿：一条永远不响的
    # 检查比没有检查更糟，因为它会让人以为这件事有人管。
    lines = raw.splitlines()
    for i, line in enumerate(lines):
        if not line.strip().startswith("///"):
            continue
        j = i + 1
        while j < len(lines) and (not lines[j].strip() or lines[j].strip().startswith("//")):
            j += 1
        if j >= len(lines):
            problems.append("%s:%d 悬空的 `///`（后面没有任何项）：%s"
                            % (rel, i + 1, line.strip()[:60]))
            break

    # C 类型名残留。手抄 FFI 镜像的典型产物 —— 它们不是 Rust 类型，
    # rustc 会报 `cannot find type`，但那要等到有 rustc 的机器上。
    # 只认**明确的类型位置**：`: T,` / `: T)` / `-> T` / `<T>`。
    #
    # 第一版是「冒号右边出现过这个词就报」，于是 `let mut wrapper =
    # PacketWrapper::new(&signed);` 里的**变量名** `signed` 被报成 C 类型。
    # 那不是小概率：`signed` / `long` / `short` / `char` 全是常见的变量名。
    CTOK = ("int", "short", "long", "unsigned", "signed", "size_t", "float", "double", "char")
    TYPE_POS = re.compile(
        r"(?::\s*|->\s*|<\s*)(?:&\s*)?(?:mut\s+|const\s+)?\b(%s)\b\s*(?=[,;)>\]{=]|$)"
        % "|".join(CTOK)
    )
    for k, line in enumerate(code.splitlines(), 1):
        m = TYPE_POS.search(line)
        if not m:
            continue
        tok = m.group(1)
        if re.search(r"\bc_%s\b" % tok, line):
            continue
        problems.append("%s:%d 类型位置出现 C 类型名 %r（不是 Rust 类型）：%s"
                        % (rel, k, tok, line.strip()[:70]))

    # 未使用的 `use`。`pub use` 不查 —— 那是再导出，本文件不引用是正常的。
    for m in re.finditer(r"^\s*(pub(?:\([^)]*\))?\s+)?use\s+([^;]+);", code, re.M):
        if m.group(1):
            continue
        spec = m.group(2)
        if spec.rstrip().endswith("*"):
            continue  # glob 的使用与否 rustc 才判得了
        if "{" in spec:
            inner = re.search(r"\{(.*)\}", spec, re.S)
            names = [x.strip().split(" as ")[-1].strip() for x in inner.group(1).split(",")]
        else:
            names = [spec.strip().split("::")[-1].split(" as ")[-1].strip()]
        rest = code[: m.start()] + code[m.end():]
        for name in names:
            if not name or name == "self":
                continue
            # **大写开头的名字不查。** 它可能是一个 trait，而 trait 被 use
            # 进来的唯一目的往往就是让 `x.method()` 能解析 —— trait 名本身
            # 一次都不会出现。`bedrock_codec::Codec` 就是这样，第一版把它
            # 报成了未使用。
            #
            # 代价是漏掉未使用的类型导入。那一类 clippy 会报，而这里
            # 宁可漏也不能瞎报。
            if name[:1].isupper():
                continue
            if not re.search(r"\b%s\b" % re.escape(name), rest):
                line = code[: m.start()].count("\n") + 1
                problems.append("%s:%d `use ... %s` 之后再没出现过 —— "
                                "`-D warnings` 下是硬错" % (rel, line, name))

    # mod 声明指向真实文件
    d = os.path.dirname(path)
    stem = os.path.splitext(os.path.basename(path))[0]
    subdir = d if stem in ("lib", "main", "mod") else os.path.join(d, stem)
    for m in re.finditer(r"^\s*(?:pub\s+)?mod\s+(\w+)\s*;", code, re.M):
        name = m.group(1)
        cands = [
            os.path.join(subdir, name + ".rs"),
            os.path.join(subdir, name, "mod.rs"),
        ]
        if not any(os.path.exists(c) for c in cands):
            line = code[: m.start()].count("\n") + 1
            problems.append("%s:%d `mod %s;` 找不到对应文件（找过 %s）"
                            % (rel, line, name, " / ".join(os.path.relpath(c, ROOT) for c in cands)))


def check_dead_crate_items(crate_dir, problems):
    """`pub(crate)` 的项/方法在整个 crate 里有没有真正的使用点。

    ## 自由项 vs 方法，判据必须分开

    第一版对两者都用「名字出现次数 > 1」。对自由函数够用；对 **impl 块里的
    方法**不够 —— `Host::handle()` 从来没被调用过，但 `handle` 这个词在
    crate 里出现十几次（`Handle` 类型、`handle:` 字段、`set_runtime(api,
    handle)` 的参数名），计数轻松过关，clippy 照样报 `never used`。

    所以方法改看**调用形状**：`.name(` 或 `::name(`。这会漏掉经 trait
    对象间接调用的情况，但那种本来 clippy 也不报 dead_code。

    `pub`（不带 crate）的项不查：它们是给下游用的，crate 内没有调用方
    完全正常。
    """
    src = os.path.join(crate_dir, "src")
    if not os.path.isdir(src):
        return
    whole = ""
    free_defs = []    # 自由项：(名字, 文件, 行号)
    method_defs = []  # impl 里的方法：(名字, 文件, 行号)
    for dp, _, fs in os.walk(src):
        for fn in sorted(fs):
            if not fn.endswith(".rs"):
                continue
            p = os.path.join(dp, fn)
            text = open(p, encoding="utf-8", errors="replace").read()
            whole += text + "\n"
            code = strip(text)
            rel = os.path.relpath(p, ROOT)
            # 先把每个 impl 块的行号区间标出来，再判定每个定义落在哪儿。
            # 第一版的写法是「边扫边猜 in_impl」，括号配平写错了，结果
            # 所有方法都被当成自由项 —— 那条检查因此对方法**恒绿**。
            # 一条恒绿的检查比没有检查更糟，所以这里改成先算区间再判定。
            impl_ranges = []
            lines = code.splitlines()
            i = 0
            while i < len(lines):
                mi = re.match(r"^\s*impl\b[^{]*?(\w+)\s*(?:<[^>]*>)?\s*\{", lines[i]) \
                    or re.match(r"^\s*impl\s+(?:<[^>]*>\s*)?(\w+)", lines[i]) \
                    or re.match(r"^\s*(?:pub\s+)?trait\s+(\w+)", lines[i])
                if mi:
                    depth = 0
                    started = False
                    j = i
                    while j < len(lines):
                        depth += lines[j].count("{") - lines[j].count("}")
                        if "{" in lines[j]:
                            started = True
                        if started and depth <= 0:
                            break
                        j += 1
                    impl_ranges.append((i + 1, j + 1, mi.group(1)))
                    i = j + 1
                else:
                    i += 1

            def owner_of(ln):
                """这一行落在哪个 impl 块里；不在任何 impl 里返回 None。"""
                for a, b, owner in impl_ranges:
                    if a <= ln <= b:
                        return owner
                return None

            for ln, line in enumerate(lines, 1):
                m = re.match(
                    r"^\s*pub\(crate\)\s+(?:unsafe\s+)?(?:extern\s+\"C\"\s+)?"
                    r"(?:const\s+)?(fn|struct|enum|trait|type|static|const)\s+(\w+)",
                    line,
                )
                if not m:
                    continue
                owner = owner_of(ln)
                if m.group(1) == "fn" and owner:
                    method_defs.append((m.group(2), owner, rel, ln))
                else:
                    free_defs.append((m.group(2), rel, ln))
    body = strip(whole)

    for name, rel, line in free_defs:
        if len(re.findall(r"\b%s\b" % re.escape(name), body)) <= 1:
            problems.append("%s:%d `pub(crate)` 的 %s 在整个 crate 里没有第二处提及 —— "
                            "clippy 会报 never used" % (rel, line, name))

    # 同名方法定义在**不同类型**上时，文本判据分不开谁调了谁：
    # `rt.handle()` 到底是 `Runtime::handle` 还是 `Host::handle`，
    # 需要类型推导才知道。这是这个 surrogate 的硬边界 —— 不假装覆盖到了，
    # 明说排除掉，让读的人知道这几个名字得靠 `cargo clippy` 兜。
    by_name = {}
    for name, owner, rel, line in method_defs:
        by_name.setdefault(name, set()).add(owner)
    ambiguous = {n for n, owners in by_name.items() if len(owners) > 1}

    for name, owner, rel, line in method_defs:
        if name in ambiguous:
            continue
        # 调用形状：`.name(` 或 `Type::name(`。定义处是 `fn name(`，不算。
        if not re.search(r"(?:\.|::)%s\s*\(" % re.escape(name), body):
            problems.append("%s:%d `pub(crate)` 的方法 %s::%s 在整个 crate 里没有调用点 —— "
                            "clippy 会报 never used（名字出现次数不算数：同名的字段、"
                            "参数、类型都会把计数撑过去）" % (rel, line, owner, name))
    if ambiguous:
        notes = ", ".join(
            "%s（定义在 %s 上）" % (n, "、".join(sorted(by_name[n]))) for n in sorted(ambiguous)
        )
        problems_note = ("  · 有 %d 个方法名在多个类型上重复，文本判据分不开调用点归属，"
                         "已排除：%s" % (len(ambiguous), notes))
        AMBIGUOUS_NOTES.append(problems_note)


def main():
    target = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "packages")
    problems = []
    n = 0
    for dp, dirs, fs in os.walk(target):
        dirs[:] = [d for d in dirs if d not in ("target", ".git")]
        for fn in sorted(fs):
            if not fn.endswith(".rs"):
                continue
            p = os.path.join(dp, fn)
            n += 1
            check_file(p, os.path.relpath(p, ROOT), problems)

    # 每个 crate 单独看：`pub(crate)` 的可见范围就是一个 crate。
    for base in (os.path.join(ROOT, "packages"), os.path.join(ROOT, "examples")):
        if not os.path.isdir(base):
            continue
        for d in sorted(os.listdir(base)):
            cd = os.path.join(base, d)
            if os.path.exists(os.path.join(cd, "Cargo.toml")):
                check_dead_crate_items(cd, problems)

    for pb in problems:
        print("  ✗ %s" % pb)
    if problems:
        print()
        print("  %d 处形状问题。真判据仍是 `cargo check`。" % len(problems))
        return 1
    for note in AMBIGUOUS_NOTES:
        print(note)
    print("  %d 个 .rs 文件无显然的形状错误，且 pub(crate) 的项都有调用方。" % n)
    print("  这**不**代表能编过 —— 类型、生命周期、借用检查都不在覆盖内，")
    print("  真判据是 `cargo check`。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
