# The manifest

Every Pier mod is a directory under `mods/` holding a dynamic library and a
`manifest.json`:

```
mods/
  my-mod/
    my_mod.dll
    manifest.json
```

```json
{
  "name": "my-mod",
  "entry": "my_mod.dll",
  "type": "pier",
  "version": "0.1.0",
  "description": "What this mod does.",
  "dependencies": [{ "name": "pier" }]
}
```

## Fields

| Field | Required | Notes |
|---|---|---|
| `name` | yes | Must match the directory name. |
| `entry` | yes | The library the build produced. |
| `type` | yes | Exactly `pier`. |
| `version` | yes | Semver. |
| `description` | no | Shown in `/pier list`. |
| `dependencies` | yes | Must include `{ "name": "pier" }`. |
| `reload_safe` | no | `true` opens the mod to `/pier reload`. Default false. |
| `load_level` | no | Integer, default 0. Lower loads earlier. |

## The two that go wrong

**`type` must be exactly `pier`.** The host compares it literally against one string. Any
other value and the mod is never scanned: no error, no log line, it is simply absent from
`/pier list`. Pier's own CI has a check for this, because the failure reports nothing at
all.

**`entry` must match what the build produced.** Cargo turns hyphens in a crate name into
underscores, so the crate `my-mod` builds `my_mod.dll`. This is the easiest one to slip
on.

## Load order

Mods load in `load_level` order, lowest first, and mods sharing a level load by name.
Omitting the field puts you at 0 with everyone else, which is where a mod with no
preference belongs.

```json
{ "load_level": -10 }
```

A level orders, it does not sequence. If your mod cannot work unless another one is
already loaded, that is a **dependency**, and the host checks it and refuses to load you
without it. A level only says which way to lean among mods that have a preference and
nothing to hang it on: a permission manager that would rather register its gates before
anything can consult them, say.

A non-integer value is reported as a problem in `/pier list` rather than treated as 0.
Writing `"load_level": "early"` means an ordering was intended, and quietly giving that
mod the default is exactly the silent fallback this project refuses elsewhere.

## Dependencies

Listing `pier` is what orders the load, so that the host exists before your mod is
dispatched to it. A name that is not `pier` is a dependency on a mod that does not exist.

Depending on another Pier mod works the same way:

```json
"dependencies": [
  { "name": "pier" },
  { "name": "plot-manager" }
]
```

That orders the load. It does not make the other mod's API reachable; use
[a service or the bus](/rust/cross-mod) for that.
