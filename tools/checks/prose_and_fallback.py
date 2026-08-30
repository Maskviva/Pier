# -*- coding: utf-8 -*-
"""no-silent-fallback / comment-claims —— 契约 §五 的两条（5.1、5.4）。

两条都是「文本层面能查、编译器查不到」的性质，所以合一个文件。

## comment-claims（§5.4：注释不许对代码撒谎）
盯的是什么：声称了**安全性质**的注释，旁边必须就是实现那个性质的代码。
v1 有三处「异常已吞掉」而同函数内根本没有 try —— 而 detour 里漏出去的
异常等于整服崩。下一个人会照注释推理，所以撒谎的注释比没有注释危险。

判据：函数体内的注释出现「吞掉 / 捕获 / 不会抛 / 不外抛 / swallow」等
断言时，同一个函数体里必须有 `try` 或 `catch`。

## no-silent-fallback（§5.1：不许静默回退）
盯的是什么：`unwrap_or(0)` 那一族 —— 读不出值就悄悄拿一个默认值顶上。
真实事故：事件载荷读不出 dim，消费方补 0，自定义维度里的每个事件都被
当成发生在主世界，土地保护「主世界拒绝、别处放行」，零日志。

判据（保守，只报**高置信**形状）：`catch (...)` 之后的块里既没有任何
日志调用、也没有重新抛出、也没有返回一个能表达「没有答案」的值。
纯文本判据必然不完备 —— 它逮的是「catch 完什么都不干」这种最典型的，
漏掉的复杂形状靠 §五 的人工评审。脚本报 PASS **不等于**这条性质成立，
只等于最典型的那一类不存在；这一点在 CONTRACT §九 里注明。
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402

PKGS = os.path.join(ROOT, "packages")

# 断言词。必须**同时**出现异常词汇才算 —— 「swallow the packet entirely」
# 说的是丢包语义，不是吞异常；只按断言词匹配会把 ABI 文档整段报出来，
# 而一条天天误报的检查等于没有检查。
CLAIM = re.compile(r"(吞掉|吞下|已捕获|捕获后|不会抛|不外抛|不向外抛|"
                   r"swallow(?:ed|s)?|never throws?|caught here)")
EXC_WORD = re.compile(r"(异常|抛出|throw|exception|catch|try)")
LOGCALL = re.compile(r"\b(log|Log|logger|hostLogger|warn|error|info|debug|trace|"
                     r"PIER_TRACE\w*)\b")

# 「复位成可判别的空」—— §5.1 允许的第一种做法。
NO_ANSWER = re.compile(r"\.clear\(\)|=\s*\{\s*\}|=\s*nullptr|=\s*std::nullopt|"
                       r"=\s*\"\"|=\s*false|=\s*-1|\.reset\(\)")

# 「把失败装进错误字段/错误对象」—— 也是把「问不出来」如实交出去。
ERR_CARRY = re.compile(r"\b\w*([Pp]roblem|[Ee]rror|[Ff]ail\w*|[Rr]eason)\w*\s*=|"
                       r"makeStringError|\bunexpected\(|\bErr\(")


def _functions(text):
    """粗略切出函数体：从一个 `{` 到配平的 `}`，只取顶层缩进较浅的。

    不做真正的 C++ 解析 —— 那是编译器的活。这里只要能把「注释和它旁边的
    try/catch」放进同一个窗口就够了。宁可窗口大一点（漏报）也不要切碎
    （误报）：一条天天误报的检查会被人加进忽略列表，然后就再也不响了。
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
    """把注释和字符串挖空，但**保留换行** —— 否则报出来的行号是错的，
    而一条报错行号的检查会让人去看不相干的代码，然后不再相信它。"""

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
                    r1.fail("%s:~%d 注释断言 %r 但同一个函数体里没有 catch：%s"
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
                # 「把要填的东西重置成空」就是**返回能表达「没有答案」的值**
                # ——§5.1 三种合法做法的第一种。`targetName.clear()` 之后
                # 订阅方看到空串，那是可判别的「读不出来」，不是猜。
                if NO_ANSWER.search(handler):
                    continue
                # 把失败装进一个错误字段/错误对象，同样是把「问不出来」交出去。
                if ERR_CARRY.search(handler):
                    continue

                if not handler.strip("{} \n\t"):
                    # 空处理块 + 一条说明为什么的注释 = 放行。理由：这种形状里
                    # 「没有答案」是由**已经默认构造的空值**表达的（订阅方看到
                    # 空串即知读不出来），而那件事文本检查看不见 —— 注释是唯一
                    # 可查的凭据。注释一旦撒谎，归 §5.4 的 comment-claims 管。
                    # 判据必须回到**原文**取：挖空后的文本里注释已经没了，
                    # 拿它判「有没有注释」会得出恒真的错误结论。
                    # 挖空只保行数、不保字符偏移，所以按**行号**回原文取，
                    # 不能按字符下标切。
                    l0 = code[:j].count("\n")
                    l1 = code[:k].count("\n")
                    raw = "\n".join(text.splitlines()[l0 : l1 + 1])
                    if re.search(r"//|/\*", raw):
                        continue
                    r2.fail("%s:%d `catch` 块完全空白（连注释都没有）—— "
                            "读者无从判断这里是有意回退还是漏写（契约 §5.1）"
                            % (rel, line))
                else:
                    r2.fail("%s:%d `catch` 块既不打日志、不重抛、不返回、也不把"
                            "目标复位或装进错误值 —— 异常被丢掉且不留痕迹"
                            "（契约 §5.1）" % (rel, line))

    r1.note("扫描 %d 个源文件" % files)
    r2.note("扫描 %d 个源文件。判据只覆盖 `catch` 处理块，且只报**无歧义**的"
            "形状：既不打日志、不重抛、不返回、不复位目标、也不装进错误值。"
            "文本层面看不见的静默回退（跨函数的默认值补齐、`value_or(0)` 之类）"
            "不在覆盖内 —— **PASS 不等于 §5.1 成立**，只等于最典型的那一类不存在。"
            % files)
    return [r1, r2]


if __name__ == "__main__":
    rc = 0
    for res in run():
        rc |= res.report()
    sys.exit(rc)
