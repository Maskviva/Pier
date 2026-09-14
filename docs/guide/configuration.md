# Configuring Pier

`plugins/Pier/config.json`. Written once when it is absent and **never rewritten**, so
anything edited stays edited and a missing key uses its default.

```json
{
  "language": "auto",
  "mods": {
    "disabled": []
  },
  "hooks": {
    "disabled": [],
    "decision_ttl_ms": 250
  }
}
```

A key the host does not read is dropped without a word. Nothing here remembers a former
spelling of a setting or one that no longer exists.

## `language`

Which `lang/<code>.lang` the host speaks. The directory ships in the release, beside the
dll, and the host only ever reads it.

- `"auto"` follows the locale the engine reports, which is right for a server with one
  language.
- Any other value names the file directly. `zh_CN` and `zh-CN` both work; the file itself
  is `zh_CN.lang`, after Minecraft's own naming.

The chain is the selected language, then `en_US`, then the key itself. A half-finished
translation falls back per key, so a file is useful while it is being written.

The startup line says which of the two decided it, because an operator who edited the
setting and sees no change needs to tell a value that did not apply from a language that
is simply not translated:

```
[host] language zh_CN (named in config.json): 29 key(s) from plugins/Pier/lang
```

A code with no file behind it gets one more line, at warning level, because the count
above still looks healthy — the built-in English answers every key — while nothing the
operator asked for is in effect.

Adding a language needs no rebuild: copy `en_US.lang` to a new code, translate the
values, drop it in `lang/`. See [Adding a language](./adding-a-language).

## `mods.disabled`

Pier mods this host refuses to load, by manifest name.

```json
"mods": { "disabled": ["some-mod"] }
```

The refusal happens **before the dylib is mapped**, so nothing of that mod runs: no
static constructor, no `DllMain`. Refusing later would still let in whatever ran on the
way, which is usually the half an operator is trying to stop.

The name must match the manifest exactly. A fuzzy match here would silently refuse a mod
nobody named.

## `hooks.disabled`

Synthetic events no mod may subscribe to, by event name.

```json
"hooks": { "disabled": ["PlayerDropItemEvent"] }
```

The subscription is **refused**, not silently dropped, and the refusal is logged with the
mod that asked. A mod handed a handle that never fires would report itself as protecting
something for the rest of the session, which is the failure this host refuses everywhere
else.

Use it to turn off one interception without uninstalling the mod that wanted it. Expect
that mod to behave as though the event does not exist.

## `hooks.decision_ttl_ms`

How long a pressure plate or push decision stays cached, in milliseconds. Default 250,
which is five ticks. Range 0 to 5000; a value outside it is clamped and the clamp is
reported.

This cache is a requirement rather than an optimisation. `entityInside` runs once per
tick for every actor standing on a plate, and each dispatch assembles SNBT, crosses the
FFI, parses on the other side, queries a claim database and crosses back. At 20 Hz one
idle player on a plate costs more than everything else combined.

- **0** dispatches every tick. Offered so that a claim mod under test sees each call; it
  is not a setting for a running server.
- **Higher** widens the window in which a stale allow outlives the claim it was read
  from. The position is part of the cache key, so moving one cell invalidates it
  immediately; the risk is a player standing still while their permission changes.

## When the file cannot be read

Every setting falls back to its default **for that run**, and the error line says so
explicitly, naming the consequence: `mods.disabled` is empty, so anything listed in it
loads as usual.

The start is not refused. Pier is the loader, and refusing to come up would take down
every mod on the server over one missing comma. The permissive direction is the dangerous
one here, which is why it is stated rather than merely logged.
