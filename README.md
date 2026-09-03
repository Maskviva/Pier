<h1 align="center">Pier</h1>

<p align="center">
  <b>A LeviLamina mod loader for languages other than C++.</b>
</p>

<p align="center">
  <a href="../../actions/workflows/build.yml"><img src="../../actions/workflows/build.yml/badge.svg" alt="Build"></a>
  <a href="https://github.com/Maskviva/pier/releases"><img src="https://img.shields.io/github/v/release/Maskviva/pier?color=334155" alt="Release"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-Apache--2.0-blue" alt="Apache-2.0"></a>
  <img src="https://img.shields.io/badge/BDS-1.26.20-62B47A" alt="BDS 1.26.20">
  <img src="https://img.shields.io/badge/LeviLamina-26.20.4-8B5CF6" alt="LeviLamina 26.20.4">
</p>

<p align="center">
  <a href="README.md">English</a> ·
  <a href="README.zh.md">简体中文</a>
</p>

Pier exposes the Bedrock server through one C ABI. A mod is a dynamic library that speaks
that ABI, so the language it is written in is the author's choice rather than the
platform's.

Rust is the first official binding. Go, Zig and anything else with a C FFI can have one,
and adding it does not require touching Pier.

## Why Pier exists

It started as [levilamina-rust-loader](https://github.com/Maskviva/levilamina-rust-loader),
built to answer one question: can a LeviLamina mod be written in Rust?

It could, and the answer surfaced a harder problem. The loader had grown its interface by
accretion, one function at a time, and the shape underneath was wrong in ways that could
not be patched out:

- **The layout forked per build target.** Conditional blocks meant the tail of the
  function table sat at a different offset on a client build than on a server one. A
  version-number marker was added to compensate, that marker could not protect a mod
  nobody had rebuilt, and every slot past the fork landed on the wrong function pointer
  while both sides compiled cleanly.
- **The center called out to every capability.** The dispatch table named each package's
  functions directly, so a capability could not be dropped from a build without the link
  failing. "Optional" was a word in the documentation, not a property of the code.
- **Missing values became plausible ones.** An unreadable field came back as a zero that
  looked like a legitimate answer. A land protection mod read an unresolvable dimension as
  the overworld, refused inside the overworld and allowed everything everywhere else, and
  logged nothing.
- **Event names matched on substrings.** The day upstream added an event sharing a stem,
  a subscriber would be silently redirected to a synthetic event with a different payload
  shape, with no route back to the real one.

Each of those is fixable in isolation. Together they say the interface was never designed,
only grown. The loader was a test, it did what a test is for, and it is not maintained.

Pier is the redesign: the interface first, the implementation second.

## How Pier is designed

The whole of Pier is one header,
[`packages/pier-abi/include/sdk/abi.h`](packages/pier-abi/include/sdk/abi.h). Everything
else in this repository is one implementation of it. That inversion is what the four
problems above turn into once they are taken seriously.

### One layout, on every target

`PierApi` carries no conditional compilation. The client-only slots and the custom
dimension slots occupy their places on every build and are simply NULL when the matching
package was not compiled in.

So "does the host have this capability" is a question about a pointer, answered at
runtime, and the same mod source builds for every target. The version-number marker and
the patches defending it are all gone, because the layout no longer forks.

### Capabilities register themselves

Pier is eight C++ packages. `pier-host` owns the table and the mod lifecycle and knows
nothing about what fills it. A capability package registers itself through a service
provider interface at four points: the slot pack it fills, the teardown steps it needs,
whether it vetoes an unload, and the events it provides.

The edge points inward. Delete a package from the build and no registration happens, the
slots stay NULL, and the host does not change by one line. That makes optionality
executable rather than declared, and a check runs it once per package on every push.

### Handles are identities

Nothing crosses the ABI as a pointer into the server. A player is a selector, an actor is
an id, a block is a dimension and a coordinate. Each call resolves again.

Keep one across ticks and it stays valid: once the actor is gone a call returns an error
rather than jumping into freed memory. The cost is a lookup per call, which is the right
trade for a boundary that mods on the other side of will get wrong.

### Buffers belong to whoever made them

Any buffer crossing the boundary is allocated and freed by its producer, and the receiver
copies anything it keeps. The ABI never returns a pointer for the other side to free,
because that needs an allocator contract and the two allocators are not the same. Every
output goes through a sink.

### An error is an error

There is no slot that answers a question it could not determine with a plausible value.
Cannot-be-determined and the answer being no are kept apart, all the way down: a decision
that might fail to answer does not return a bare boolean, because collapsed into `false` a
caller can only guess and collapsed into `true` it is a security hole.

### The contract only grows

Adding a capability appends a slot and leaves the ABI version alone. Reordering or
removing one advances both version numbers together, and a mod built before that is
refused at load with a message saying so, rather than being allowed to run into a slot
that moved.

Compatibility is a range, not an equality. A mod built against an older Pier keeps
working.

### The rules are checked, not just written

A rule with no script guarding it is a wish. Every property above has a check in
`tools/checks/`, and `python3 tools/run-checks.py` runs them on every push: that the
header parses as C11, that the slot order only ever grew, that the Rust mirror matches it
parameter by parameter, that no capability package has a sideways edge, that no comment
claims something the code does not do.

Each check states what it covers and what it cannot see, and a delivery note is allowed to
copy that sentence and nothing stronger.

The rules themselves are in [`CONTRACT.md`](CONTRACT.md), with the reasoning for each.

## Writing a mod

**[Rust](docs/rust/index.md)** is the first official binding and is
what to reach for today.

```rust
use levilamina::prelude::*;

struct MyMod;

impl LeviMod for MyMod {
    fn on_load(ctx: &ModContext) -> Result<Self> {
        ctx.logger().info("hello from Rust");
        Ok(MyMod)
    }
}

levilamina::register_mod!(MyMod);
```

- [Your first mod](docs/rust/first-mod.md) walks through building
  and installing one.
- [pier-mod-template](https://github.com/Maskviva/pier-mod-template) is a working starting
  point rather than an empty skeleton.

## Binding another language

One file is enough. `sdk/abi.h` parses as C11, so an FFI tool takes it directly, and it
doubles as the reference documentation: every slot states what its parameters mean, what
it returns on failure, and what it requires of threads.

1. Mirror `PierApi`, declaring every field unconditionally, with no target branch.
2. Export `pier_main` and fill the vtable with `struct_size`, `abi_version`, `mod_flags`
   and the three lifecycle callbacks.
3. Before calling a non-core slot, check that `struct_size` covers it and that the slot is
   not NULL.

`bindings/rust/pier-sys-rs` is the reference implementation.
[Adding a language](docs/guide/adding-a-language.md) has the
details, and section 10 of [`CONTRACT.md`](CONTRACT.md) is the authoritative version.

## Installing

With [lip](https://lip.futrime.com):

```bash
lip install github.com/Maskviva/pier
```

Or unpack the release archive into `plugins/pier/`.

Pier needs LeviLamina 26.20.4 on BDS 1.26.20.
[LegacyMoney](https://github.com/LiteLDev/LegacyMoney) is optional: it is delay-loaded, so
a server without it starts normally and only the economy calls return failure values.

## Building from source

```bash
xmake f --target_type=server && xmake   # the host, pier.dll
cargo build --release                    # the Rust binding and the examples
```

The two build lines share no build dependency, only the header, so they can be fixed
separately.

```bash
python3 tools/run-checks.py   # the machine checks of contract §9
```

## Contributing

Read [`CONTRACT.md`](CONTRACT.md) first; code that conflicts with it is the side that
changes. A new language binding is welcome and does not need anything in this repository
to change.

## License

Apache-2.0. See [`LICENSE`](LICENSE).

---

*Not affiliated with Mojang, Microsoft or LeviMC. Minecraft is a trademark of Mojang
Synergies AB.*
