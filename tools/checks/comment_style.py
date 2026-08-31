# -*- coding: utf-8 -*-
"""comment-style —— `COMMENTS.md` 的机械部分。

盯的是什么：发布出去的注释里不许留下开发期的产物 —— 过程叙事、内部票号、
对话人称、markdown 排版、超预算的长篇。这些东西对读者是负数：他要先读完
一段与他无关的经过，才拿到那一句对他有用的结论。

判据全部是文本层面的，所以覆盖面**窄而确定**：

  · 注释块行数预算，L1/L2/L3 各一档（COMMENTS.md §一）
  · 禁用词（§三 第 2/4 类：过程叙事、对话人称）
  · 票号与阶段号（§三 第 3 类）
  · 注释内 markdown 排版与 ASCII 分隔线（§三 第 5 类、§五）
  · TODO/FIXME（§三 第 7 类）
  · abi.h 注释内的中文（§六「注释用英文」）
  · 行宽 100 列（§五）

**看不见的**：注释说的是不是真话（§七 归 comment-claims）、留下来的注释是不是
§二 那五类、有没有复述代码。这三条要人读。按契约 §九，PASS 只等于机械规则成立。
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

PKGS = os.path.join(ROOT, "packages")
ABI_REL = os.path.join("packages", "pier-abi", "include", "sdk", "abi.h")

MAX_HEAD = 16  # §一 L1：文件头
MAX_DOC = 14  # §一 L2：声明上方的 /** */
MAX_LINE = 8  # §一 L3：语句上方的 // 连续行
MAX_COL = 100  # §五

# §三 第 2、4 类。词表只收**无歧义**的：一条天天误报的检查会被加进忽略列表，
# 然后就再也不响了。「原来」「以前」这类在技术陈述里也用得着的词不进表。
BANNED = [
    (r"第一版|第一次重写|最初(?:的)?(?:做法|版本|实现)|曾经声称|旧仓|老仓", "过程叙事"),
    (r"重写了[一二三四五六七八九十\d]+(?:次|遍)|改了[一二三四五六七八九十\d]+(?:次|遍)", "过程叙事"),
    (r"血买来的|下一个人|上一个人|花了.{0,6}才(?:发现|查出)", "修辞"),
    (r"(?<![A-Za-z])我们(?![A-Za-z])|(?<![A-Za-z])咱(?:们)?(?![A-Za-z])", "对话人称"),
    (r"实测[一二三四五六七八九十\d]+(?:次|遍)|试过[一二三四五六七八九十\d]+(?:次|遍)", "过程叙事"),
]
BANNED = [(re.compile(p), why) for p, why in BANNED]

# §三 第 3 类：内部评审票号。指向读者拿不到的文档。
# 不收 `stage N`：本仓的 stage 是 spi::TeardownReg 的拆除次序，是真实的代码概念。
TICKET = re.compile(r"(?<![\w-])V-\d{2}(?![\w-])|(?<![\w])W\d{2}(?![\w])")

# §三 第 7 类。
TODO = re.compile(r"\b(TODO|FIXME|XXX|HACK)\b")

# §五：ASCII 分隔线。三个及以上的重复框线字符。
RULE_LINE = re.compile(r"[─═━╌]{3,}|-{6,}$|={6,}$")

CJK = re.compile(r"[\u4e00-\u9fff]")


def blocks_of(text):
    """切出注释块，返回 (起始行号, 行列表, 是否块注释)。

    连续的 `//` 行算**一个**块 —— 分开数会让一段 20 行的行注释每行都合规。
    行尾注释（代码在前）不参与预算，只参与词表与行宽。
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
    """剥掉注释记号，只留正文 —— 判 markdown 排版必须在剥掉 ` * ` 之后做，
    否则每一行都以 `*` 开头，`**加粗**` 和普通续行分不开。"""
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
    """行尾注释：代码在前、`//` 在后。返回 (行号, 正文)。"""
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

            # ── 行宽（§五）。只卡真正的注释行；代码行归 clang-format。
            # 行号取自 blocks_of + 行尾注释，不按行首字符猜：一行 `*ptr->f()` 是解
            # 引用，不是块注释的续行，按字符猜会把它报成超宽的注释。
            cmt_lines = set()
            for start, lns, _ in blocks_of(text):
                cmt_lines.update(range(start, start + len(lns)))
            cmt_lines.update(k for k, _ in inline_comments(text))
            for k, ln in enumerate(text.splitlines(), 1):
                if k in cmt_lines and len(ln) > MAX_COL:
                    r.fail("%s:%d 注释行宽 %d > %d 列（§五）" % (rel, k, len(ln), MAX_COL))

            # ── 块级
            for start, lns, is_block in blocks_of(text):
                blocks += 1
                body = prose_of(lns, is_block)
                joined = "\n".join(body)

                # 预算（§一）。abi.h 免除，它是产品文档（§六）。
                if start <= 5:
                    cap, tier = MAX_HEAD, "L1 文件头"
                elif is_block:
                    cap, tier = MAX_DOC, "L2 声明注释"
                else:
                    cap, tier = MAX_LINE, "L3 体内注释"
                if not is_abi and len(lns) > cap:
                    r.fail(
                        "%s:%d %s %d 行 > %d 行预算：属于设计的进 CONTRACT.md，"
                        "属于历史的进 git（§一）" % (rel, start, tier, len(lns), cap)
                    )

                # markdown 排版（§三 第 5 类）
                for off, line in enumerate(body):
                    if re.match(r"^#{1,6}\s", line):
                        r.fail("%s:%d 注释里的 markdown 标题（§三.5）" % (rel, start + off))
                    if line.startswith("```"):
                        r.fail("%s:%d 注释里的 markdown 代码块（§三.5）" % (rel, start + off))
                    if re.match(r"^\|.*\|$", line) or re.match(r"^\|?[\s:-]*-{3,}[\s:|-]*$", line):
                        r.fail("%s:%d 注释里的 markdown 表格（§三.5）" % (rel, start + off))
                    if re.search(r"\*\*\S(?:[^*]*\S)?\*\*", line):
                        r.fail("%s:%d 注释里的 markdown 加粗（§三.5）" % (rel, start + off))
                    if RULE_LINE.search(line):
                        r.fail("%s:%d 注释里的分隔线（§五）" % (rel, start + off))

                # 词表 / 票号 / TODO
                for pat, why in BANNED:
                    m = pat.search(joined)
                    if m:
                        r.fail("%s:%d 禁用措辞 %r（%s，§三）" % (rel, start, m.group(0), why))
                m = TICKET.search(joined)
                if m:
                    r.fail("%s:%d 内部评审票号 %r（§三.3）" % (rel, start, m.group(0)))
                m = TODO.search(joined)
                if m:
                    r.fail("%s:%d %s 归 issue，不留在代码里（§三.7）" % (rel, start, m.group(0)))

                # abi.h 用英文（§六）
                if is_abi and CJK.search(joined):
                    r.fail("%s:%d 契约头的注释必须用英文（§六）" % (rel, start))

            # ── 行尾注释：不参与预算，其余同管
            for k, body in inline_comments(text):
                for pat, why in BANNED:
                    m = pat.search(body)
                    if m:
                        r.fail("%s:%d 禁用措辞 %r（%s，§三）" % (rel, k, m.group(0), why))
                m = TICKET.search(body)
                if m:
                    r.fail("%s:%d 内部评审票号 %r（§三.3）" % (rel, k, m.group(0)))
                m = TODO.search(body)
                if m:
                    r.fail("%s:%d %s 归 issue，不留在代码里（§三.7）" % (rel, k, m.group(0)))
                if is_abi and CJK.search(body):
                    r.fail("%s:%d 契约头的注释必须用英文（§六）" % (rel, k))

    r.note("扫描 %d 个源文件、%d 个注释块" % (files, blocks))
    r.note(
        "判据只覆盖 COMMENTS.md 的机械部分：预算、禁用词、票号、markdown 排版、"
        "行宽、契约头语言。**注释是否真实、是否属于 §二 五类、是否复述代码，"
        "文本层面看不见** —— PASS 不等于本规范成立，只等于机械规则成立。"
    )
    return [r]


if __name__ == "__main__":
    rc = 0
    for res in run():
        rc |= res.report()
    sys.exit(rc)
