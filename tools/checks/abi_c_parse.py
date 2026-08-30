# -*- coding: utf-8 -*-
"""abi-c-parse —— `sdk/abi.h` 必须能被 C11 编译器解析（契约 §〇）。

盯的是什么：契约的消费方是「任何语言」，而 C 是它们唯一都读得懂的那一种。
一旦头文件里混进 C++ 语法（`enum class`、嵌套类型、默认参数、`std::` 别名），
Go / Zig / C# 的 FFI 工具会直接吃不下去 —— 而这在 C++ 侧永远编得过，
所以没有这条检查就是**零信号**。

顺带用 C++20 也编一遍：宿主侧是 C++，两边都得过。
"""

import os
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result, read_abi, strip_comments  # noqa: E402


def run():
    r = Result("abi-c-parse")
    inc = os.path.join(ROOT, "packages", "pier-abi", "include")

    for cc, std, tag in (("gcc", "c11", "C11"), ("g++", "c++20", "C++20")):
        src = "#include <sdk/abi.h>\n"
        # 顺带确认头文件可重复包含（守卫有效）——重复包含在真实 SDK 里很常见。
        src += "#include <sdk/abi.h>\n"
        suffix = ".c" if cc == "gcc" else ".cpp"
        with tempfile.NamedTemporaryFile("w", suffix=suffix, delete=False) as f:
            f.write(src)
            path = f.name
        try:
            p = subprocess.run(
                [cc, "-std=" + std, "-fsyntax-only", "-Wall", "-I", inc, path],
                capture_output=True,
                text=True,
            )
            if p.returncode != 0:
                r.fail("%s 解析失败：\n%s" % (tag, p.stderr.strip()[:2000]))
            else:
                r.note("%s 解析通过" % tag)
        finally:
            os.unlink(path)

    # gcc 那一遍才是真正的判据。这里再做一次纯文本兜底，防的是**未来**有人
    # 把 C++ 专属拼写塞进 `#ifdef __cplusplus` 分支里 —— 那种代码 gcc 看不见，
    # 却会让别的语言的 FFI 工具在预处理阶段就读到。
    #
    # 必须先剥注释再扫：`ll::event::PlayerChatEvent` 这类**文档正文**里的
    # `::` 是在描述 LL 的事件 id，不是 C++ 语法。按行猜「这行是不是注释」
    # 会把它们全报出来 —— 一条把真信号淹掉的检查等于没有检查。
    banned = ("enum class", "namespace ", "template<", "template <", "::")
    src_text = strip_comments(read_abi())
    for i, line in enumerate(src_text.splitlines(), 1):
        if 'extern "C"' in line:
            continue
        for b in banned:
            if b in line:
                r.fail("sdk/abi.h:%d 出现 C++ 专属拼写 %r：%s" % (i, b, line.strip()))
    if not r.failures:
        r.note("正文（剥注释后）零 C++ 专属拼写")
    return r


if __name__ == "__main__":
    sys.exit(run().report())
