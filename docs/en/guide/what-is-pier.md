# What is Pier

Pier is a LeviLamina mod that loads other mods. Those mods are dynamic libraries that
speak one C ABI, so the language they are written in is the author's choice.

There is no interpreter and no sandbox. A mod is native code the server loads and calls
directly, at the speed a C++ mod runs at. What Pier adds is a boundary, not a runtime.

## The shape of it

```
your mod  (any language)
    |
    |  sdk/abi.h        one C header, ~190 slots
    v
  Pier    (C++, eight packages)
    |
    v
LeviLamina  ->  Bedrock Dedicated Server
```

The header is the product. Everything in this repository below it is one implementation,
and a binding above it needs nothing else.

## Bindings

| Language | Status | |
|---|---|---|
| Rust | Official, the most complete | [Documentation](../rust/index.md) |
| C++ | Official, on the header directly | [Documentation](../cpp/index.md) |
| Go | Official, a c-shared DLL through cgo | [Documentation](../go/index.md) |
| Zig | Official, on the header through translate-c | [Documentation](../zig/index.md) |
| Anything with a C FFI | Yours to write | [Adding a language](adding-a-language.md) |

Rust came first because Pier grew out of wanting to write LeviLamina mods in Rust. Nothing
in the design prefers it, and a Go or Zig binding needs no change here to exist.

## What a mod reaches

| Area | |
|---|---|
| Players | Selectors, properties, inventory, equipment, abilities, permissions, teleporting, titles, messages |
| World | Time, weather, difficulty, game rules, biomes, chunk save keys, region scans |
| Blocks | Reads and writes by coordinate, block states, block entities, the liquid layer |
| Actors | Snapshots, relations, effects, status flags, ray casts, spawning from NBT |
| Events | The LeviLamina registry, plus 29 events Pier synthesizes with native detours |
| Commands | Raw-text commands and typed overloads with client-side completion |
| Cross-mod | A named service channel, a broadcast bus, and a same-toolchain fast lane |
| Dimensions | Registering a custom dimension, per-dimension rules, cell confinement |

Plus forms, scoreboards, containers, an economy bridge, a key-value store, raw packet
interception, simulated players, tick control and profiling.

## Where to go

- **You want to write a mod.** [Rust](../rust/index.md) is the most complete binding;
  [C++](../cpp/index.md) and [Go](../go/index.md) are there for those languages.
- **You want to know why it looks like this.** [Why Pier exists](why.md), then
  [How it is designed](design.md).
- **You want to bind a language.** [Adding a language](adding-a-language.md).
- **You run a server.** [Installing](installation.md).
