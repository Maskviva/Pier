#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""gen-api-docs: writes the API reference of the four bindings into docs/{en,zh}/api/.

Each binding's public interface is read from its source: the `levilamina` crate of
bindings/rust/pier-rs, package levilamina of bindings/go, the `levilamina` module of
bindings/zig, and sdk/abi.h for C++ mods compiled with the Pier SDK. The pages, the overview
and the API section of the navigation in docs/mkdocs.yml and docs/mkdocs.zh.yml are all
written from that reading; the navigation goes between the `# BEGIN gen-api-docs` and
`# END gen-api-docs` lines and nothing else in those files is touched.

The Chinese pages take their descriptions from the catalog under docs/i18n/zh/api/, one unit
per English comment, keyed by a fingerprint of the English (tools/apidocs/zh.py says how).

Usage:
    python3 tools/gen-api-docs.py                    # write the pages and the navigation
    python3 tools/gen-api-docs.py --check            # compare only; exit 1 when anything differs
    python3 tools/gen-api-docs.py --zh-todo [FILTER] [--out FILE]
                                                     # the comments with no Chinese unit yet
    python3 tools/gen-api-docs.py --zh-merge FILE    # add a batch of `@@ key` units, then write
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from apidocs.model import DOCS, ROOT  # noqa: E402
from apidocs.site import Site, generated_dirs  # noqa: E402


def arg(flag):
    k = sys.argv.index(flag)
    return sys.argv[k + 1] if k + 1 < len(sys.argv) and not sys.argv[k + 1].startswith("--") else None


def main():
    if "--zh-merge" in sys.argv:
        with open(arg("--zh-merge"), encoding="utf-8") as f:
            batch = f.read()
        site = Site()
        site.files()
        print("merged %d unit(s) into docs/i18n/zh/api/" % site.zh.merge(batch))
    site = Site()
    want = {os.path.normpath(os.path.join(DOCS, p)): t for p, t in site.files().items()}
    zh = site.zh
    coverage = "Chinese units %d of %d" % (len(zh.needed) - len(zh.missing()), len(zh.needed))
    if "--zh-todo" in sys.argv:
        items = zh.todo(arg("--zh-todo"))
        text = "\n".join(items)
        if "--out" in sys.argv:
            with open(arg("--out"), "w", encoding="utf-8", newline="\n") as f:
                f.write(text)
        else:
            print(text)
        print("%d comment(s) without a Chinese unit listed; %s" % (len(items), coverage))
        return 0
    want.update(site.mkdocs_texts())
    have = set()
    for d in generated_dirs():
        for dp, _, fs in os.walk(d):
            have.update(os.path.join(dp, f) for f in fs)
    stale = []
    for path, text in sorted(want.items()):
        current = None
        if os.path.exists(path):
            with open(path, encoding="utf-8") as f:
                current = f.read()
        if current != text:
            stale.append(path)
    extra = sorted(have - set(want))
    if have and not (have & set(want)):
        raise SystemExit("gen-api-docs: not one generated path matches a file on disk; refusing "
                         "to treat the %d file(s) under the generated directories as stale" % len(extra))
    counts = ", ".join("%s %d" % (b, sum(site.counts(b))) for b in ("rust", "go", "zig", "cpp"))
    gone = zh.stale()
    for k in gone:
        print("docs/i18n/zh/api/%s.md: the English of unit %s is no longer in the source; "
              "translate the comment that replaced it and delete the unit" % (zh.home.get(k, "?"), k))
    # Every description has a Chinese unit, so a comment added or reworded in a binding
    # without one is reported here, by the entry it belongs to.
    untranslated = zh.missing()
    for k, (lines, bindings, context) in untranslated:
        print("%s: no Chinese unit for this comment (fingerprint %s); "
              "python3 tools/gen-api-docs.py --zh-todo lists it with its English" % (context, k))
    if "--check" in sys.argv:
        for p in stale:
            print("stale: %s" % os.path.relpath(p, ROOT))
        for p in extra:
            print("not generated, yet under a generated directory: %s" % os.path.relpath(p, ROOT))
        if stale or extra:
            print("the API reference does not match the bindings; run python3 tools/gen-api-docs.py")
        if stale or extra or gone or untranslated:
            return 1
        print("the API reference matches the bindings: %d page(s); interfaces listed: %s; %s"
              % (len(want) - 2, counts, coverage))
        return 0
    for path in stale:
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            f.write(want[path])
    for path in extra:
        os.remove(path)
    print("wrote %d of %d file(s), removed %d; interfaces listed: %s; %s"
          % (len(stale), len(want), len(extra), counts, coverage))
    return 0


if __name__ == "__main__":
    sys.exit(main())
