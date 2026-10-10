# -*- coding: utf-8 -*-
"""The C++ reference: sdk/abi.h itself, which is what a mod compiled with the Pier SDK calls.

abi.h orders PierApi by when each slot was appended, so one subject is spread over several of
its sections. The pages here are by subject: SLOT_RULES places every slot, and each entry
still names the abi.h section it sits in. A slot, enum, macro or type that no rule places
stops the generator with its name, so a slot appended to abi.h cannot be left out of the
reference unnoticed.
"""

import re

from .model import Entry, Group, Page, ROOT

ABI_REL = "packages/pier-abi/include/sdk/abi.h"

TOPICS = [
    ("core", "Core: entry point, logging and tasks", "核心：入口、日志与任务"),
    ("events", "Events", "事件"),
    ("commands", "Commands", "命令"),
    ("server", "Server, ticks and system information", "服务器、刻与系统信息"),
    ("world", "World", "世界"),
    ("edit", "Bulk world editing", "批量编辑世界"),
    ("player", "Players", "玩家"),
    ("entity", "Actors", "实体"),
    ("block", "Blocks", "方块"),
    ("item", "Items and containers", "物品与容器"),
    ("scoreboard", "Scoreboard", "计分板"),
    ("form", "Forms", "表单"),
    ("data", "NBT and the key-value database", "NBT 与键值数据库"),
    ("money", "Economy", "经济"),
    ("packet", "Packets", "数据包"),
    ("sim", "Simulated players", "模拟玩家"),
    ("client", "Client", "客户端"),
    ("dimensions", "Custom dimensions", "自定义维度"),
    ("crossmod", "Cross-mod: bus, services and lanes", "跨模组：总线、服务与快速通道"),
    ("registry", "Registries", "注册表"),
    ("types", "Shared types", "通用类型"),
]

SLOT_RULES = [
    (r"log|gaming_status|schedule\w*", "core"),
    (r"subscribe_event|unsubscribe_event|list_events", "events"),
    (r"execute_command|register_command\w*|update_command_soft_enum", "commands"),
    (r"get_current_tick|get_tick_delta_time|get_player_count|get_sim_paused|get_tps|get_mspt"
     r"|tick_\w+|profile_\w+|server_info_str|sys_\w+", "server"),
    (r"get_block|set_block|get_extra_block|set_extra_block|get_time|set_time|set_weather"
     r"|spawn_particle\w*|scan_region\w*|get_difficulty|set_difficulty|get_seed|game_rule_\w+"
     r"|level_\w+|villages|structures_near|explode", "world"),
    (r"edit_\w+", "edit"),
    (r"player_\w+|list_players|broadcast_message|get_player_position", "player"),
    (r"actor_\w+|list_actors|spawn_mob", "entity"),
    (r"block_\w+", "block"),
    (r"item_\w+|container_\w+", "item"),
    (r"scoreboard_op", "scoreboard"),
    (r"form_send", "form"),
    (r"nbt_\w+|kvdb_\w+", "data"),
    (r"get_money|set_money|add_money|reduce_money|trans_money|money_\w+", "money"),
    (r"packet_\w+|send_packet", "packet"),
    (r"sim_\w+", "sim"),
    (r"client_\w+", "client"),
    (r"md_\w+", "dimensions"),
    (r"bus_\w+|service_\w+|lane_\w+", "crossmod"),
    (r"registry_\w+", "registry"),
]

ENUM_PAGES = {
    "PierPlayerNumProp": "player", "PierPlayerStrProp": "player", "PierPlayerAction": "player",
    "PierActorNumProp": "entity", "PierActorStrProp": "entity", "PierActorAction": "entity",
    "PierBlockNumProp": "block", "PierBlockStrProp": "block", "PierBlockAction": "block",
    "PierItemNumProp": "item", "PierItemStrProp": "item", "PierItemOp": "item",
    "PierScoreboardOp": "scoreboard", "PierDimRule": "dimensions", "PierPackStatus": "dimensions",
    "PierSysInfoProp": "server", "PierServerInfoProp": "server", "PierMoneyEvent": "money",
}

DEFINE_RULES = [
    (r"PIER_(ABI|MAIN|FLAG)_\w+", "core"),
    (r"PIER_(SERVICE|LANE)_\w+", "crossmod"),
    (r"PIER_PKT_\w+", "packet"),
    (r"PIER_REGISTRY_\w+", "registry"),
]

TYPE_RULES = [
    (r"PierApi|PierModVTable|PierMainFn|PierModHandle|PierTaskCb", "core"),
    (r"PierEventCb|PierListenerHandle", "events"),
    (r"PierCommandCb|PierCmdOutputSink", "commands"),
    (r"PierPacket\w+|PierConnCb", "packet"),
    (r"PierLane\w+|PierBusCb|PierServiceCb", "crossmod"),
    (r"PierKey\w+|PierFocusImpact", "client"),
    (r"PierMoney\w+", "money"),
    (r"PierFormResultCb", "form"),
    (r"PierKv\w+|PierBytesSink", "data"),
    (r"PierChunkRequest|PierGenerateChunkFn|PierPaletteSink|PierCellSink|PierBlockCell", "dimensions"),
    (r"PierStr|PierStrSink|PierPlayerSel|PierActorId|PierContainerRef|PierPlayerPos|PierBlockSink"
     r"|PierEntitySink|PierSlotSink|PierActorSink", "types"),
]

# Macros that are part of the header's mechanics rather than its interface.
SKIP_DEFINES = {"PIER_SDK_ABI_H"}


def place(rules, name, what):
    for rx, page in rules:
        if re.fullmatch(rx, name):
            return page
    raise SystemExit("gen-api-docs: the %s %s has no page; add it to the rules in "
                     "tools/apidocs/cpp.py" % (what, name))


def comment_lines(block):
    """The text of a /* */ or /** */ comment, one entry per line, the * margin removed."""
    body = re.sub(r"^/\*\*?", "", block.strip())
    body = re.sub(r"\*/$", "", body)
    out = []
    for l in body.split("\n"):
        l = l.rstrip()
        m = re.match(r"^\s*\*(?!/)\s?(.*)$", l)
        out.append(m.group(1) if m else l.strip())
    return out


def split_params(text):
    text = " ".join(text.split())
    if text in ("", "void"):
        return []
    out = []
    for p in text.split(","):
        p = p.strip()
        m = re.match(r"^(.*?)(\w+)$", p)
        if not m:
            raise SystemExit("gen-api-docs: cannot read the parameter %r" % p)
        out.append((m.group(2), " ".join(m.group(1).replace("*", " * ").split()).replace(" *", "*")))
    return out


class Header:
    """Everything the C++ pages show, read from abi.h in one pass."""

    def __init__(self, text):
        self.text = text
        self.lines = text.split("\n")
        self.slots = []        # Entry
        self.header_fields = []
        self.enums = []        # (name, doc, members[(name, value, doc)])
        self.defines = []      # (names, doc, lines, members[(name, value, doc)])
        self.types = []        # (name, doc, decl)
        self.notes = []        # top-level section notes outside every declaration
        self.file_header = []
        self.section_text = {}
        self._parse()

    def _parse(self):
        lines = self.lines
        i, n = 0, len(lines)
        pending = None
        m0 = re.match(r"^/\*\*(.*?)\*/", self.text, re.S)
        self.file_header = comment_lines(m0.group(0)) if m0 else []
        i = len(m0.group(0).split("\n")) if m0 else 0
        while i < n:
            l = lines[i]
            s = l.strip()
            if not s:
                i += 1
                continue
            if s.startswith("/*"):
                j = i
                while "*/" not in lines[j]:
                    j += 1
                block = "\n".join(lines[i:j + 1])
                if s.startswith("/**"):
                    pending = comment_lines(block)
                elif s.startswith("/*  "):
                    self.notes.append(comment_lines(block))
                    pending = None
                i = j + 1
                continue
            if s.startswith("typedef struct PierApi"):
                i = self._parse_api(i)
                pending = None
                continue
            if s.startswith("#"):
                j = i
                while j < n and lines[j].strip().startswith("#"):
                    j += 1
                self._define_block(lines[i:j], pending)
                pending = None
                i = j
                continue
            if re.match(r"^(typedef\s+)?enum\s+\w+", s):
                j = i
                while not re.match(r"^\}\s*\w*\s*;", lines[j].strip()):
                    j += 1
                name = re.match(r"^(?:typedef\s+)?enum\s+(\w+)", s).group(1)
                self.enums.append((name, pending or [], self._enum_members("\n".join(lines[i + 1:j]))))
                pending = None
                i = j + 1
                continue
            if s.startswith("typedef"):
                j = i
                if re.match(r"^typedef\s+struct\s+\w+\s*$", s):
                    while not re.match(r"^\}\s*\w+\s*;", lines[j].strip()):
                        j += 1
                else:
                    depth = 0
                    while True:
                        depth += lines[j].count("(") - lines[j].count(")")
                        if lines[j].rstrip().endswith(";") and depth == 0:
                            break
                        j += 1
                decl = "\n".join(lines[i:j + 1])
                if re.match(r"^typedef\s+struct\s+\w+\s*$", s):
                    m = re.search(r"\}\s*(\w+)\s*;\s*$", decl)
                else:
                    m = re.search(r"\(\s*\*\s*(\w+)\s*\)", decl) or re.search(r"(\w+)\s*;\s*$", decl)
                self.types.append((m.group(1), pending or [], decl))
                pending = None
                i = j + 1
                continue
            pending = None
            i += 1

    def _define_block(self, block, doc):
        members, names = [], []
        conditional = any(re.match(r"^#\s*(if|ifdef|ifndef|else|endif)\b", l.strip()) for l in block)
        for l in block:
            m = re.match(r"^#define\s+(PIER_\w+)(?:\s+(.*?))?\s*(?:/\*(.*?)\*/)?\s*$", l.strip())
            if not m or m.group(1) in SKIP_DEFINES:
                continue
            if m.group(1) not in names:
                names.append(m.group(1))
            members.append((m.group(1), (m.group(2) or "").strip(), [m.group(3).strip()] if m.group(3) else []))
        if not names:
            return
        self.defines.append((names, doc or [], block, members, conditional))

    def _enum_members(self, body):
        """Each member with the comment that follows it, on its own line or the lines below,
        the rule tools/gen-go-api.py and tools/gen-zig-api.py read the members by. A comment
        opened with two spaces marks a run of appended members and documents none of them."""
        out = []
        rx = re.compile(r"(?m)^[ \t]*(PIER_\w+)\s*=\s*(-?\w+)[ \t]*,?(?:(\s*)(/\*(?s:.*?)\*/))?")
        for m in rx.finditer(body):
            comment = m.group(4)
            if comment and "\n" in m.group(3) and comment.startswith("/*  "):
                comment = None
            doc = [t.strip() for t in comment_lines(comment) if t.strip()] if comment else []
            out.append([m.group(1), m.group(2) or "", doc])
        nxt = 0
        for row in out:
            if row[1]:
                nxt = int(row[1], 0) + 1
            else:
                row[1] = str(nxt)
                nxt += 1
        return [tuple(r) for r in out]

    def _parse_api(self, i):
        lines = self.lines
        i += 2  # the typedef line and the brace
        section = "the core slots, present since ABI v1"
        self.section_text[section] = []
        doc, note, orphan, note_used = None, None, None, False
        order = 0
        while not lines[i].startswith("} PierApi;"):
            s = lines[i].strip()
            if not s:
                if note is not None and not note_used:
                    orphan = note
                note = None
                i += 1
                continue
            if s.startswith("/*"):
                j = i
                while "*/" not in lines[j]:
                    j += 1
                block = "\n".join(lines[i:j + 1])
                text = comment_lines(block)
                if s.startswith("/**"):
                    doc = text
                elif s.startswith("/*  "):
                    section = text[0].strip()
                    self.section_text[section] = ((orphan or []) + [""] if orphan else []) + text[1:]
                    orphan, note, doc = None, None, None
                else:
                    note, note_used = text, False
                i = j + 1
                continue
            j = i
            depth = 0
            while True:
                depth += lines[j].count("(") - lines[j].count(")")
                if ";" in lines[j] and depth == 0:
                    break
                j += 1
            raw = lines[i:j + 1]
            trailing = []
            m = re.search(r";\s*/\*(.*?)\*/\s*$", raw[-1])
            if m:
                trailing = [m.group(1).strip()]
                raw[-1] = raw[-1][:m.start() + 1]
            decl = "\n".join(l[4:] if l.startswith("    ") else l for l in raw).rstrip()
            flat = " ".join(" ".join(raw).split())
            fm = re.match(r"^(.*?)\(\s*\*\s*(\w+)\s*\)\s*\((.*)\)\s*;$", flat)
            if orphan is not None:
                self.section_text[section] = self.section_text.get(section, []) + [""] + orphan
                orphan = None
            if fm:
                ret, name, params = fm.group(1).strip(), fm.group(2), fm.group(3)
                e = Entry("cpp", "slot", name, name, decl, "%s:%d" % (ABI_REL, i + 1))
                e.ret = ret if ret != "void" else None
                e.params = split_params(params)
                e.call = "api->%s(%s)" % (name, ", ".join(p for p, _ in e.params))
                e.doc = (doc or []) + ([""] if doc and trailing else []) + trailing
                e.note = note or []
                note_used = note is not None
                e.section = section
                e.order = order
                order += 1
                self.slots.append(e)
            else:
                fm = re.match(r"^(.*?)\b(\w+)\s*;$", flat)
                e = Entry("cpp", "field", fm.group(2), "PierApi." + fm.group(2), decl,
                          "%s:%d" % (ABI_REL, i + 1), anchor="PierApi." + fm.group(2))
                e.doc = (doc or []) + trailing
                self.header_fields.append(e)
            doc = None
            i = j + 1
        return i + 1


def build():
    with open("%s/%s" % (ROOT, ABI_REL), encoding="utf-8") as f:
        h = Header(f.read())
    pages = {slug: Page("cpp", slug, en, zh, [ABI_REL]) for slug, en, zh in TOPICS}

    def group(page, key, title, title_zh=None, kind="functions"):
        p = pages[page]
        for g in p.groups:
            if g.anchor == key:
                return g
        g = Group(title, key, kind)
        g.title_zh = title_zh
        p.groups.append(g)
        return g

    # The table header comes first on the core page, then the module's side of the handshake.
    core = group("core", "PierApi-header", "PierApi: the table header", "PierApi：表头字段", "fields")
    for e in h.header_fields:
        core.entries.append(e)

    for e in h.slots:
        page = place(SLOT_RULES, e.name, "slot")
        g = group(page, "slots", "Slots", "槽位")
        g.entries.append(e)
        if e.section and h.section_text.get(e.section) is not None:
            notes = pages[page].section_notes
            if all(t != e.section for t, _ in notes):
                notes.append((e.section, h.section_text[e.section]))

    for name, doc, members in h.enums:
        page = ENUM_PAGES[name] if name in ENUM_PAGES else place([], name, "enum")
        g = Group("`%s`" % name, name, "consts")
        g.doc = doc
        for mname, value, mdoc in members:
            g.consts.append((mname, value, mdoc, "unsupported since" in " ".join(mdoc)))
        pages[page].groups.append(g)

    for names, doc, block, members, conditional in h.defines:
        page = place(DEFINE_RULES, names[0], "macro")
        if conditional or len(members) == 1:
            e = Entry("cpp", "macro", names[-1], " / ".join(names), "\n".join(block),
                      ABI_REL, anchor=names[-1])
            e.doc = doc
            group(page, "macros", "Macros", "宏").entries.append(e)
        else:
            g = Group("`%s`" % (re.sub(r"_[A-Z0-9]+$", "_*", names[0])), names[0] + "-group", "consts")
            g.doc = doc
            for mname, value, mdoc in members:
                g.consts.append((mname, value, mdoc, False))
            pages[page].groups.append(g)

    for name, doc, decl in h.types:
        page = place(TYPE_RULES, name, "type")
        e = Entry("cpp", "type", name, name, decl, ABI_REL, anchor=name)
        e.doc = doc
        group(page, "types", "Types", "类型").entries.append(e)

    # Within a page: the slot group first, then constants, then macros and types.
    rank = {"fields": 0, "functions": 1, "consts": 2}
    for p in pages.values():
        p.groups.sort(key=lambda g: (rank.get(g.kind, 3), g.anchor == "types"))
    return h, [p for p in pages.values() if p.groups]
