# -*- coding: utf-8 -*-
"""abi-fixed-width —— 契约里不许出现宽度平台相关的整数类型。

盯的是什么：`int` / `long` / `unsigned short` 的宽度是**实现定义**的。
一份声称「任何语言都能读」的契约里出现它们，等于要求绑定作者先去查宿主
用的是哪个编译器、哪个数据模型（LP64？LLP64？），才能知道这个参数有几个
字节 —— 而这正是 §〇 那句「契约的消费方是任何语言」要消灭的东西。

这条是被真机编译逼出来的：`money_*` 一族有九个槽直接抄了 LegacyMoney 的
签名，带着 `long long` / `int` / `unsigned short`。它们在 MSVC x64 上分别是
64/32/16 位，和 `int64_t` / `int32_t` / `uint16_t` **二进制完全相同** ——
所以改过来是零成本的，而不改的话，第一个用 Zig 或 C# 写绑定的人就得猜。

`gcc -std=c11` 编得过它们，所以 `abi-c-parse` 拦不住；Rust 侧的手写镜像
把 `int` 原样抄了过去，直到 `cargo clippy` 才报 `cannot find type int`。
一条只有在**第二门语言**动手时才会显形的缺陷，正是 §九 要的那类。

放行的两个例外：
  * `char`（只在 `const char* ptr` 里，C 的字符串就是它，没有第二种写法）
  * `bool`（C99 起由 `<stdbool.h>` 保证）
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result, read_abi, strip_comments  # noqa: E402

# 宽度平台相关的拼写。带词边界，避免匹配到 `int32_t` 里的 `int`。
BARE = re.compile(
    r"\b(?:unsigned\s+(?:char|short|int|long(?:\s+long)?)"
    r"|signed\s+(?:char|short|int|long(?:\s+long)?)"
    r"|long\s+long|long|int|short|unsigned|signed)\b"
)

# 建议的替换，报错时直接给出来 —— §5.3：日志要能回答「我该做什么」。
SUGGEST = {
    "long long": "int64_t",
    "unsigned long long": "uint64_t",
    "int": "int32_t",
    "unsigned int": "uint32_t",
    "unsigned": "uint32_t",
    "short": "int16_t",
    "unsigned short": "uint16_t",
    "long": "int32_t 或 int64_t（先确认你要的是哪个）",
    "unsigned char": "uint8_t",
    "signed char": "int8_t",
}


def run():
    r = Result("abi-fixed-width")
    src = strip_comments(read_abi())
    hits = 0
    for i, line in enumerate(src.splitlines(), 1):
        # `const char*` 是 PierStr 的成员，放行。
        probe = re.sub(r"\bconst\s+char\s*\*", "", line)
        m = BARE.search(probe)
        if not m:
            continue
        hits += 1
        found = " ".join(m.group(0).split())
        r.fail(
            "sdk/abi.h:%d 用了宽度平台相关的 %r，改成 %s —— "
            "契约的消费方是任何语言，它们无从知道这个类型在宿主上有几个字节：%s"
            % (i, found, SUGGEST.get(found, "对应的定宽类型"), line.strip()[:80])
        )
    if not hits:
        r.note("契约里零裸 C 整数类型（`char` 只出现在 `const char*` 里）")
    return r


if __name__ == "__main__":
    sys.exit(run().report())
