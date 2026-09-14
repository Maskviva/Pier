# hello-bridge

A **native LeviLamina mod** that asks a Pier mod a question. It is not a Pier mod, and
Pier does not load it.

```
xmake f -P . -a x64 -m release -p windows
xmake -P .
```

## What it shows

- `manifest.json` has `"type": "native"` and declares `pier` as a dependency, which is
  what makes Pier load first.
- `pier-bridge.h` is copied in by include path and nothing is linked. The three symbols
  are bound at runtime, so the mod loads and runs on a server with no Pier at all.
- `Client::open()` happens in `enable()`, not `load()`.
- An absent Pier and an unprovided service are both handled as normal, not as errors.

## What it is not

The bridge reaches the JSON service registry. There are no events, no hooks, no forms, no
lane and no dimensions behind it, and the traffic only goes one way: this mod can call a
Pier mod, and a Pier mod cannot call this one. A mod that needs any of that is a Pier mod
and includes `sdk/abi.h` instead — see `examples/hello-pier-cpp`.
