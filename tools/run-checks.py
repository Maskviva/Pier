#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""跑齐契约 §九 的机器检查。任何一条红就以非零退出。

用法：
    python3 tools/run-checks.py            # 全跑
    python3 tools/run-checks.py abi        # 只跑名字里含 "abi" 的

## 这个脚本能证明什么、不能证明什么

**能**：下面列出的每一条性质，在当前工作区的**文本**上成立。

**不能**：它不构建任何东西。三条性质的充分判据需要真正的工具链，
脚本只覆盖了它们的必要条件：

  - `optional-drops`  静态判据是「可选包的符号无人跨包引用」；
                      充分判据是真的删掉那行 `includes(...)` 再 `xmake f`。
  - `sys-mirrors-abi` 比对的是槽序与常量的**文本**；
                      「镜像真的能编过」要 `cargo check`。
  - `no-silent-fallback` 只覆盖 `catch` 块里的无歧义形状，见该脚本自己的说明。

四个 surrogate（`link-` / `include-` / `rust-` / `ledger-count`）也不在这里，
它们盯的东西编译器和链接器本来就会报，只是这个仓库有一段时间没有工具链。
按 §九 的判据它们不该是契约的一部分，所以单独放在 `tools/` 下。

契约 §九 说「脚本先行：一条性质没有脚本守着之前，交付说明里不许给它打 ✓」。
这里再补一句同样重要的：**脚本 PASS 也只能给它覆盖到的那部分打 ✓**，
每条检查自己的 note 里写了它覆盖到哪。
"""

import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CHECKS = os.path.join(HERE, "checks")

# 顺序有意：先证明契约本体（abi.h）成立，再证明分包结构成立，最后是代码纪律。
SCRIPTS = [
    ("abi_c_parse.py", ["abi-c-parse"]),
    ("abi_additive.py", ["abi-additive"]),
    ("abi_no_lang.py", ["abi-no-lang"]),
    ("abi_fixed_width.py", ["abi-fixed-width"]),
    ("pkg_layering.py", ["pkg-layering", "object-kind", "optional-drops"]),
    ("build_config.py", ["build-config"]),
    ("include_resolves.py", ["include-resolves"]),
    ("sys_mirrors_abi.py", ["sys-mirrors-abi"]),
    ("rust_layering.py", ["rust-layering"]),
    ("rust_comment_budget.py", ["rust-comment-budget"]),
    ("manifest_matches_host.py", ["manifest-matches-host"]),
    ("host_loadable.py", ["host-loadable"]),
    ("ledger_covers_tree.py", ["ledger-covers-tree"]),
    ("prose_and_fallback.py", ["comment-claims", "no-silent-fallback"]),
    ("comment_style.py", ["comment-style"]),
]


def main():
    want = sys.argv[1] if len(sys.argv) > 1 else None
    failed = []
    ran = 0
    for script, names in SCRIPTS:
        if want and not any(want in n for n in names) and want not in script:
            continue
        path = os.path.join(CHECKS, script)
        if not os.path.exists(path):
            print("  SKIP %s（脚本尚未写）" % "/".join(names))
            continue
        print("── %s" % " + ".join(names))
        p = subprocess.run([sys.executable, path])
        ran += 1
        if p.returncode != 0:
            failed.extend(names)
        print()

    print("=" * 62)
    if failed:
        print("FAIL —— %d 条不通过：%s" % (len(failed), "、".join(failed)))
        return 1
    print("PASS —— %d 个脚本全部通过。" % ran)
    print("提醒：optional-drops 只过了静态必要条件（可选包符号无人跨包引用）；")
    print("      交付前仍要真跑一次 xmake f 与 cargo clippy。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
