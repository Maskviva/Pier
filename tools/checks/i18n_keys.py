# -*- coding: utf-8 -*-
"""i18n-keys: the shipped `.lang` files name real keys, with matching placeholders.

What it watches: a translation is data, and the three ways it goes wrong are all silent.
A key that no longer exists in the code is simply never used, so a translator keeps
maintaining a line nobody reads. A translation whose `{}` count differs from the English
prints `[bad translation]` at the moment the line fires, which is the moment somebody
needed to read it. And a shipped file missing a key falls back to English for that line,
which on a translated server reads as a half-finished release.

The two files under `lang/` are what the build copies next to the dll, so a language is
only as complete as the file here. `en_US.lang` has to carry every key: it is both the
reference a translator copies and the file an operator edits.

Placeholder *order* is not checkable here and is the fourth way to get it wrong: the count
matches and the arguments land in the wrong slots. The .lang header says so.
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
BUILTIN = ROOT / "lang/en_US.lang"
LANGS = ROOT / "lang"


def builtin_keys():
    """key -> placeholder count, from `lang/en_US.lang`.

    That file is the reference rather than a table in the source: it is what the host
    embeds, what it writes into a fresh mod directory, and what it falls back to. A
    second copy in C++ was the thing this check used to read, and holding the lines
    twice is what `lang-embedded` now exists to stop.
    """
    out = {}
    for line in BUILTIN.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        out[key.strip()] = value.count("{}")
    return out


def main():
    if not BUILTIN.exists():
        print("    · lang/en_US.lang is missing, nothing to check against")
        return 1
    keys = builtin_keys()
    print(f"    · {len(keys)} key(s) in lang/en_US.lang")
    problems = []
    files = sorted(LANGS.glob("*.lang")) if LANGS.is_dir() else []
    for required in ("en_US.lang", "zh_CN.lang"):
        if not (LANGS / required).is_file():
            problems.append(f"lang/{required} is missing; the build copies this file next to the dll")
    for f in files:
        n = 0
        for lineno, line in enumerate(f.read_text(encoding="utf-8").splitlines(), 1):
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, value = line.split("=", 1)
            key = key.strip()
            n += 1
            if key not in keys:
                problems.append(f"{f.name}:{lineno} names '{key}', which the code no longer has")
            elif value.count("{}") != keys[key]:
                problems.append(
                    f"{f.name}:{lineno} '{key}' has {value.count('{}')} placeholder(s) "
                    f"against {keys[key]} in English; it would print [bad translation]"
                )
        print(f"    · {f.name}: {n} key(s)")
        missing = sorted(set(keys) - {
            l.split("=", 1)[0].strip()
            for l in f.read_text(encoding="utf-8").splitlines()
            if "=" in l and not l.strip().startswith("#")
        })
        if missing and f.name == "en_US.lang":
            problems.append(
                f"en_US.lang is missing {len(missing)} key(s) ({', '.join(missing[:3])}...); "
                "it is the reference a translator copies and has to carry every key"
            )
        elif missing:
            print(f"    · {f.name} has not translated {len(missing)} key(s) yet, which fall back to English")
    for p in problems:
        print(f"    ✗ {p}")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
