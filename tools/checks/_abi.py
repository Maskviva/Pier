# -*- coding: utf-8 -*-
"""共享解析器 —— 所有和 `sdk/abi.h` 有关的机检都从这里取事实。

为什么单独一份：abi-additive、sys-mirrors-abi、abi-no-lang 三条检查都要
「PierApi 的槽序」。各写一份解析器 = 三份会各自漂移，而漂移的症状是
「某一条检查悄悄不再检查它以为在检查的东西」—— 和契约 §五 反对的静默
回退同族。
"""

import os
import re

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
ABI_H = os.path.join(ROOT, "packages", "pier-abi", "include", "sdk", "abi.h")


def read_abi():
    with open(ABI_H, encoding="utf-8") as f:
        return f.read()


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def struct_body(src, name):
    """取出 `typedef struct <name> { ... } <name>;` 的花括号内文本。"""
    m = re.search(
        r"typedef\s+struct\s+%s\s*\{(.*?)\n\}\s*%s\s*;" % (name, name), src, re.S
    )
    if not m:
        raise SystemExit("abi.h 里找不到 struct %s —— 解析器和头文件已经脱节" % name)
    return m.group(1)


def slots_of(src, struct="PierApi"):
    """按声明顺序返回字段名列表。函数指针槽返回槽名，标量返回字段名。

    顺序就是 ABI 本身 —— 这个列表的任何重排都是破坏性变更（契约 §2.2）。
    """
    body = strip_comments(struct_body(src, struct))
    out = []
    for decl in body.split(";"):
        decl = decl.strip()
        if not decl:
            continue
        m = re.search(r"\(\s*\*\s*([A-Za-z_]\w*)\s*\)", decl)
        if m:
            out.append(m.group(1))
            continue
        m = re.match(r"^[\w\s\*]*?\b(\w+)$", decl.replace("\n", " ").strip())
        if not m:
            raise SystemExit("无法解析 %s 的字段：%r" % (struct, decl[:60]))
        out.append(m.group(1))
    return out


# 头文件包含守卫不是契约的一部分，不参与镜像比对。
_GUARD = {"PIER_SDK_ABI_H"}


def defines(src):
    """取出所有 `#define NAME value` 的字面量宏（用于常量镜像比对）。

    值里的行尾块注释要剥掉 —— 不剥的话比对的是「2 /* 说明 */」和「2」，
    永远不等，而那种恒红的检查等于没有检查。
    """
    out = {}
    for m in re.finditer(r"^#define\s+(PIER_\w+)\s+([^\n\\]*)$", src, re.M):
        name = m.group(1)
        if name in _GUARD:
            continue
        val = re.sub(r"/\*.*?\*/", "", m.group(2)).strip()
        if not val:
            continue  # 无值宏（纯开关），没有可比的东西
        out[name] = val
    return out


def same_value(a, b):
    """两个常量字面量在数值上相同吗。

    先按整数解（十进制、十六进制、带 u/U/L 后缀都认），解不了再退到
    去掉空白和括号之后的字符串相等 —— `(1 << PIER_PKT_INBOUND)` 和
    `1 << PIER_PKT_INBOUND` 是同一个表达式，只是 C 那边习惯加括号。

    **不要用 `rstrip("u32")` 那种写法**：rstrip 的参数是字符集合，
    `"2".rstrip("u32")` 得到空串 —— 这个坑刚在本文件的上一版踩过，
    症状是所有值为 2 或 3 的常量集体报「值不同：2 vs 2」。
    """

    def norm(x):
        x = x.strip()
        x = re.sub(r"[uUlL]+$", "", x)
        x = re.sub(r"_(u|i)(8|16|32|64|size)$", "", x)
        return x.strip()

    na, nb = norm(a), norm(b)
    try:
        return int(na, 0) == int(nb, 0)
    except ValueError:
        pass
    squash = lambda x: re.sub(r"[\s()]", "", x)
    return squash(na) == squash(nb)


def enum_values(src, name):
    """取出 `typedef enum <name> { A = 1, B = 2 } <name>;` 的成员与值。"""
    m = re.search(
        r"typedef\s+enum\s+%s\s*\{(.*?)\}\s*%s\s*;" % (name, name), src, re.S
    )
    if not m:
        return None
    body = strip_comments(m.group(1))
    out = {}
    nxt = 0
    for item in body.split(","):
        item = item.strip()
        if not item:
            continue
        if "=" in item:
            k, v = item.split("=", 1)
            k, v = k.strip(), v.strip()
            try:
                nxt = int(v, 0)
            except ValueError:
                nxt = None
            out[k] = nxt
            if nxt is not None:
                nxt += 1
        else:
            out[item] = nxt
            if nxt is not None:
                nxt += 1
    return out


# ── 结果汇报的统一形状 ────────────────────────────────────────────────
class Result:
    def __init__(self, check):
        self.check = check
        self.failures = []
        self.notes = []

    def fail(self, msg):
        self.failures.append(msg)

    def note(self, msg):
        self.notes.append(msg)

    def report(self):
        for n in self.notes:
            print("    · %s" % n)
        for f in self.failures:
            print("    ✗ %s" % f)
        ok = not self.failures
        print("  %s %s" % ("PASS" if ok else "FAIL", self.check))
        return 0 if ok else 1
