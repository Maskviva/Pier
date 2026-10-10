# -*- coding: utf-8 -*-
"""The Chinese descriptions of the API reference.

Source comments are written in English (COMMENTS.md section 4), so the Chinese site takes its
descriptions from the catalog under docs/i18n/zh/api/, one unit per English comment. A unit
is keyed by a fingerprint of the English text with its whitespace collapsed. When a comment
changes in the source, its fingerprint changes, the unit written for the old text is used by
no page any more, and the check names it.

The Go and Zig generators write their property and slot methods from a few fixed sentences.
Such a description is put together from the sentence's Chinese and the unit of the part it
quotes, so the comment of one abi.h constant is translated once for the C++, Go and Zig pages.
"""

import hashlib
import os
import re

from .model import DOCS

CATALOG = os.path.join(DOCS, "i18n", "zh", "api")
FILES = ("cpp", "rust", "go", "zig")

def normalize(lines):
    return " ".join(" ".join(lines).split())


def key_of(lines):
    text = normalize(lines)
    return hashlib.sha1(text.encode("utf-8")).hexdigest()[:10] if text else None


def paragraphs(lines):
    out, cur = [], []
    for l in lines:
        if l.strip():
            cur.append(l)
        elif cur:
            out.append(cur)
            cur = []
    if cur:
        out.append(cur)
    return out


def _with(prefix, part):
    return None if part is None else prefix + part


# The fixed sentences of tools/gen-go-api.py and tools/gen-zig-api.py.
TEMPLATES = [
    (r"^\w+ reads (PIER_\w+): (.+)$", lambda m, tr: _with("读取 `%s`：" % m.group(1), tr(m.group(2)))),
    (r"^\w+ writes (PIER_\w+): (.+)$", lambda m, tr: _with("写入 `%s`：" % m.group(1), tr(m.group(2)))),
    (r"^\w+ runs (PIER_\w+): (.+)$", lambda m, tr: _with("执行 `%s`：" % m.group(1), tr(m.group(2)))),
    (r"^The output, when the verb has one, is returned; abi\.h names each argument\.$",
     lambda m, tr: "这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。"),
    (r"^The host answers no on every current engine version: (.+)$",
     lambda m, tr: _with("在当前所有的引擎版本上，宿主读这一项都没有结果：", tr(m.group(1)))),
    (r"^\w+ calls the (\w+) slot\.$", lambda m, tr: "调用 `%s` 槽位。" % m.group(1)),
    (r"^\w+ is (PIER_\w+): (.+)$", lambda m, tr: _with("即 `%s`：" % m.group(1), tr(m.group(2)))),
    (r"^Writes (PIER_\w+): (.+)$", lambda m, tr: _with("写入 `%s`：" % m.group(1), tr(m.group(2)))),
    (r"^(PIER_\w+): (.+)$", lambda m, tr: _with("`%s`：" % m.group(1), tr(m.group(2)))),
]
TEMPLATES = [(re.compile(rx, re.S), fn) for rx, fn in TEMPLATES]


class Catalog:
    def __init__(self):
        self.units = {}    # key -> Chinese text
        self.quotes = {}   # key -> the English quoted in the catalog file
        self.home = {}     # key -> the file it was read from
        self.needed = {}   # key -> [English lines, bindings, first context]
        for b in FILES:
            self._load(b)

    def path(self, b):
        return os.path.join(CATALOG, "%s.md" % b)

    def _load(self, b):
        p = self.path(b)
        if not os.path.exists(p):
            return
        with open(p, encoding="utf-8") as f:
            text = f.read()
        for block in re.split(r"(?m)^## ", text)[1:]:
            lines = block.split("\n")
            key = lines[0].split()[0]
            quote, body, in_quote = [], [], True
            for l in lines[1:]:
                if in_quote and (l.startswith(">") or not l.strip()):
                    if l.startswith(">"):
                        quote.append(l[2:] if l.startswith("> ") else l[1:])
                    continue
                in_quote = False
                body.append(l)
            zh = "\n".join(body).strip()
            if key in self.units:
                raise SystemExit("gen-api-docs: the unit %s is in the catalog twice" % key)
            if zh:
                self.units[key] = zh
                self.quotes[key] = quote
                self.home[key] = b

    def _unit(self, lines, binding, context):
        text = normalize(lines)
        if not text:
            return ""
        if re.fullmatch(r"PIER_\w+", text):
            return "`%s`" % text
        k = key_of(lines)
        rec = self.needed.setdefault(k, [lines, set(), context])
        rec[1].add(binding)
        return self.units.get(k)

    def _template(self, binding, text):
        if binding not in ("go", "zig"):
            return None
        for rx, fn in TEMPLATES:
            m = rx.match(text)
            if m:
                return m, fn
        return None

    def text(self, binding, lines, context):
        """The Chinese of a description, or None when some unit of it has no translation."""
        paras = paragraphs(lines)
        if not paras:
            return ""
        if not self._template(binding, normalize(paras[0])):
            return self._unit(lines, binding, context)
        out, ok, i = [], True, 0
        tr = lambda part: self._unit([part], binding, context)
        while i < len(paras):
            hit = self._template(binding, normalize(paras[i]))
            if hit:
                piece = hit[1](hit[0], tr)
                i += 1
            else:
                j = i
                while j < len(paras) and not self._template(binding, normalize(paras[j])):
                    j += 1
                block = []
                for p in paras[i:j]:
                    block += p + [""]
                piece = self._unit(block[:-1], binding, context)
                i = j
            if piece is None:
                ok = False
            else:
                out.append(piece)
        return "\n\n".join(out) if ok else None

    # Reporting and writing

    def missing(self):
        return [(k, v) for k, v in self.needed.items() if k not in self.units]

    def stale(self):
        return [k for k in self.units if k not in self.needed]

    def file_of(self, key):
        bindings = self.needed[key][1] if key in self.needed else set()
        for b in FILES:
            if b in bindings:
                return b
        return self.home.get(key, "cpp")

    def merge(self, batch):
        """Adds the units of a batch, `@@ key` and its Chinese, and rewrites the catalog."""
        added = 0
        for block in re.split(r"(?m)^@@ ", batch)[1:]:
            lines = block.split("\n")
            key = lines[0].split()[0]
            zh = "\n".join(lines[1:]).strip()
            if key not in self.needed:
                print("skipped %s: no description has this fingerprint" % key)
                continue
            if zh:
                self.units[key] = zh
                added += 1
        self.write()
        return added

    def write(self):
        os.makedirs(CATALOG, exist_ok=True)
        files = {b: [] for b in FILES}
        for key in self.needed:
            if key in self.units:
                files[self.file_of(key)].append(key)
        for key in self.units:
            if key not in self.needed:
                files[self.home.get(key, "cpp")].append(key)
        for b, keys in files.items():
            out = []
            for key in keys:
                lines = self.needed[key][0] if key in self.needed else self.quotes.get(key, [])
                out.append("## %s\n" % key)
                quote = [l.rstrip() for l in lines]
                while quote and not quote[-1].strip():
                    quote.pop()
                out.append("\n".join("> " + l if l.strip() else ">" for l in quote) + "\n")
                out.append(self.units[key].strip() + "\n")
            text = "\n".join(out)
            path = self.path(b)
            if keys or os.path.exists(path):
                with open(path, "w", encoding="utf-8", newline="\n") as f:
                    f.write(text)

    def todo(self, want=None):
        out = []
        for key, (lines, bindings, context) in self.needed.items():
            if key in self.units:
                continue
            if want and not any(want in str(x) for x in (context, ",".join(sorted(bindings)))):
                continue
            out.append("@@ %s %s\n%s\n" % (key, context, "\n".join(l.rstrip() for l in lines).strip()))
        return out
