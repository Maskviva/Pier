"""container.py: the outer file of a pack, the section table and the string table.

A writer collects finished section bodies and lays them out 8-byte aligned behind the
header, hashing each one; a reader validates the magic, the version, the table bounds
and every section hash before handing out a single section. The string table is one of
the sections and is built through `Strings`, which interns and returns indices.
"""
from __future__ import annotations

import hashlib
import struct

from . import format as F


class Strings:
    def __init__(self):
        self._items = [""]
        self._index = {"": 0}

    def add(self, s: str) -> int:
        if s is None:
            s = ""
        if s in self._index:
            return self._index[s]
        idx = len(self._items)
        self._items.append(s)
        self._index[s] = idx
        return idx

    def __len__(self):
        return len(self._items)

    def __getitem__(self, i):
        return self._items[i]

    def body(self) -> bytes:
        refs = bytearray()
        blob = bytearray()
        for s in self._items:
            b = s.encode("utf-8")
            refs += F.STRING_REF.pack(offset=len(blob), length=len(b))
            blob += b
        return struct.pack("<I", len(self._items)) + bytes(refs) + bytes(blob)

    @staticmethod
    def parse(body: bytes) -> list:
        (count,) = struct.unpack_from("<I", body, 0)
        refs = []
        off = 4
        for _ in range(count):
            refs.append(F.STRING_REF.unpack(body, off))
            off += F.STRING_REF.size
        out = []
        for r in refs:
            start = off + r["offset"]
            out.append(body[start:start + r["length"]].decode("utf-8"))
        return out


class Writer:
    def __init__(self, magic: bytes):
        assert magic in (F.MAGIC_TEMPLATE, F.MAGIC_VOLUME)
        self.magic = magic
        self.sections = []  # (type, version, body)

    def add(self, sec_type: int, body: bytes, version: int = 1) -> None:
        self.sections.append((sec_type, version, bytes(body)))

    def build(self) -> bytes:
        n = len(self.sections)
        table_size = n * F.SECTION_ENTRY_SIZE
        cursor = F.align8(F.HEADER_SIZE + table_size)
        entries = []
        bodies = bytearray()
        for sec_type, version, body in self.sections:
            pad = cursor - (F.HEADER_SIZE + table_size + len(bodies))
            bodies += b"\0" * pad
            entries.append((sec_type, version, cursor, len(body), hashlib.sha256(body).digest()))
            bodies += body
            cursor = F.align8(cursor + len(body))
        total = F.HEADER_SIZE + table_size + len(bodies)
        header = F.HEADER.pack(
            magic=self.magic, format_version=F.FORMAT_VERSION, flags=0,
            section_count=n, header_size=F.HEADER_SIZE, total_size=total,
        )
        table = b"".join(
            F.SECTION.pack(type=t, version=v, offset=o, length=l, sha256=h) for (t, v, o, l, h) in entries
        )
        return header + table + bytes(bodies)


class PackError(Exception):
    pass


class Reader:
    """Validates the container and exposes sections by tag."""

    def __init__(self, data: bytes):
        self.data = data
        if len(data) < F.HEADER_SIZE:
            raise PackError("file is shorter than a pack header")
        h = F.HEADER.unpack(data, 0)
        if h["magic"] not in (F.MAGIC_TEMPLATE, F.MAGIC_VOLUME):
            raise PackError("not a Pier terrain pack")
        if h["format_version"] > F.FORMAT_VERSION:
            raise PackError(f"format version {h['format_version']} is newer than this tool reads ({F.FORMAT_VERSION})")
        if h["header_size"] != F.HEADER_SIZE:
            raise PackError("header size does not match this format version")
        if h["total_size"] != len(data):
            raise PackError(f"total size {h['total_size']} does not match the file length {len(data)}")
        self.magic = h["magic"]
        self.kind = "template" if self.magic == F.MAGIC_TEMPLATE else "volume"
        self.sections = {}
        off = F.HEADER_SIZE
        for _ in range(h["section_count"]):
            if off + F.SECTION_ENTRY_SIZE > len(data):
                raise PackError("section table runs past the end of the file")
            e = F.SECTION.unpack(data, off)
            off += F.SECTION_ENTRY_SIZE
            end = e["offset"] + e["length"]
            if e["offset"] % F.SECTION_ALIGN or end > len(data) or e["offset"] < F.HEADER_SIZE:
                raise PackError(f"section {F.tag_name(e['type'])} lies outside the file or is misaligned")
            body = data[e["offset"]:end]
            if hashlib.sha256(body).digest() != e["sha256"]:
                raise PackError(f"section {F.tag_name(e['type'])} fails its hash")
            if e["type"] in self.sections:
                raise PackError(f"section {F.tag_name(e['type'])} appears twice")
            self.sections[e["type"]] = (e["version"], body)
        self.strings = Strings.parse(self.section(F.SEC_STRS))

    def has(self, tag: int) -> bool:
        return tag in self.sections

    def section(self, tag: int) -> bytes:
        if tag not in self.sections:
            raise PackError(f"section {F.tag_name(tag)} is missing")
        return self.sections[tag][1]

    def str(self, idx: int) -> str:
        if idx >= len(self.strings):
            raise PackError(f"string index {idx} is out of range")
        return self.strings[idx]

    def file_sha256(self) -> str:
        return hashlib.sha256(self.data).hexdigest()


def read_array(body: bytes, off: int, layout: F.Layout, count: int) -> tuple[list, int]:
    out = []
    for _ in range(count):
        if off + layout.size > len(body):
            raise PackError(f"{layout.name} array runs past the end of its section")
        out.append(layout.unpack(body, off))
        off += layout.size
    return out, off


def read_u32s(body: bytes, off: int, count: int) -> tuple[list, int]:
    if off + 4 * count > len(body):
        raise PackError("u32 array runs past the end of its section")
    return list(struct.unpack_from("<%dI" % count, body, off)), off + 4 * count
