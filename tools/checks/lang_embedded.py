#!/usr/bin/env python3
"""lang_embedded：编译进去的那份翻译和 lang/*.lang 对得上。

The translations are compiled in (see tools/embed-lang.py for why). A generated file
that is not regenerated is worse than no generated file: it compiles, it runs, and it
ships text that someone already corrected in the .lang and reasonably believes is live.

Covered: the generated .inc byte for byte against what the .lang files produce now.
Not covered: whether a translation is correct, or whether its placeholders match the
English — i18n_keys.py answers the second.
"""

import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GEN = os.path.join(ROOT, "tools", "embed-lang.py")

if not os.path.exists(GEN):
    print("  tools/embed-lang.py 不在，跳过")
    sys.exit(0)

r = subprocess.run([sys.executable, GEN, "--check"], capture_output=True, text=True)
out = (r.stdout + r.stderr).strip()

if r.returncode == 0:
    print("  lang_embedded：通过")
else:
    print(out if out else "  生成的表和 lang/*.lang 对不上")
print("  覆盖：生成的 .inc 和 lang/*.lang 逐字节一致")
print("  看不见：翻译对不对；占位符对不对（那是 i18n_keys）")
sys.exit(r.returncode)
