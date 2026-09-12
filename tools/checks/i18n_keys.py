# -*- coding: utf-8 -*-
"""i18n-keys: every key a `.lang` file names must exist, with matching placeholders.

What it watches: a translation is data, and the two ways it goes wrong are both silent.
A key that no longer exists in the code is simply never used, so a translator keeps
maintaining a line nobody reads. A translation whose `{}` count differs from the English
prints `[bad translation]` at the moment the line fires, which is the moment somebody
needed to read it.

Placeholder *order* is not checkable here and is the third way to get it wrong: the count
matches and the arguments land in the wrong slots. The .lang header says so.
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
BUILTIN = ROOT / "packages/pier-support/src/I18n.cpp"
LANGS = ROOT / "lang"


def builtin_keys():
    """key -> placeholder count, from the compiled-in English table.

    The value is read by collecting the C++ string literals that follow the key, which
    is more work than cutting at the first `},` — and the extra work is the point. A
    value ending in a placeholder before a comma, `... id {}, registry id {}`, contains
    `},` inside itself, so cutting there truncates the value and undercounts its
    placeholders. The symptom was a correct translation reported as having too many
    placeholders, which reads as a bug in the .lang file and is not one.
    """
    text = BUILTIN.read_text(encoding="utf-8")
    body = text.split("builtinEnglish()", 1)[-1]
    out = {}
    for m in re.finditer(r'\{"([a-z0-9_.]+)"\s*,', body):
        key = m.group(1)
        i = m.end()
        value = []
        # Adjacent literals are one string in C++, so keep taking them. Stop at the `}`
        # that closes this entry, which is the first one outside a literal.
        while i < len(body):
            c = body[i]
            if c == '"':
                i += 1
                while i < len(body):
                    if body[i] == "\\":
                        i += 2
                        continue
                    if body[i] == '"':
                        i += 1
                        break
                    value.append(body[i])
                    i += 1
                continue
            if c == "}":
                break
            if c in " \t\r\n":
                i += 1
                continue
            # Anything else on this line is not part of the value.
            break
        out[key] = "".join(value).count("{}")
    return out


def main():
    if not BUILTIN.exists():
        print("    · no built-in table, nothing to check")
        return 0
    keys = builtin_keys()
    print(f"    · {len(keys)} key(s) in the built-in English table")
    problems = []
    files = sorted(LANGS.glob("*.lang")) if LANGS.is_dir() else []
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
        if missing:
            print(f"    · {f.name} has not translated {len(missing)} key(s) yet, which fall back to English")
    for p in problems:
        print(f"    ✗ {p}")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
