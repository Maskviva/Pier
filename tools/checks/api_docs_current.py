# -*- coding: utf-8 -*-
"""api-docs-current: the API reference under docs/{en,zh}/api/ matches the four bindings.

What it watches: every page of the reference, its overview, and the API section of the
navigation in both mkdocs files are written by tools/gen-api-docs.py from the source of the
bindings and from sdk/abi.h. A public function added, renamed or deprecated in any binding
changes what the generator writes, and this check runs the generator with --check, which
fails when the files on disk differ from that output or when a file under docs/*/api/ is one
the generator would not write. The same run holds the Chinese pages complete: it fails when a
source comment has no unit in docs/i18n/zh/api/, and when a unit there translates English
that is no longer in any source. The generator also stops on a slot, enum, macro, type or
declaration that none of its page rules places, so an interface cannot drop out of the
reference by having no page.

What it cannot see: whether a description reads well, since descriptions are the source
comments themselves, and the slots reached through a call on a variable whose type the
generator does not know, which the overview page of the reference describes.
"""

import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))
from _abi import ROOT, Result  # noqa: E402


def run():
    r = Result("api-docs-current")
    gen = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "gen-api-docs.py"), "--check"],
                         capture_output=True, text=True)
    out = (gen.stdout.strip() or gen.stderr.strip())
    if gen.returncode != 0:
        for line in out.splitlines():
            r.fail(line)
    else:
        r.note(out)
    return r


if __name__ == "__main__":
    sys.exit(run().report())
