<h1 align="center">Pier</h1>

<p align="center">
  <b>A LeviLamina mod loader for languages other than C++.</b>
</p>

<p align="center">
  <a href="../../actions/workflows/build.yml"><img src="../../actions/workflows/build.yml/badge.svg" alt="Build"></a>
  <a href="https://github.com/Maskviva/pier/releases"><img src="https://img.shields.io/github/v/release/Maskviva/pier?color=334155" alt="Release"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-Apache--2.0-blue" alt="Apache-2.0"></a>
  <img src="https://img.shields.io/badge/BDS-1.26.51-62B47A" alt="BDS 1.26.51">
  <img src="https://img.shields.io/badge/LeviLamina-26.51.5-8B5CF6" alt="LeviLamina 26.51.5">
</p>

<p align="center">
  <a href="README.md">English</a> ·
  <a href="README.zh.md">简体中文</a>
</p>

Pier exposes the Bedrock server through one C ABI. A mod is a dynamic library that speaks
that ABI, so the language it is written in is the author's choice rather than the
platform's.

Rust, C++, Go and Zig have official bindings. Anything else with a C FFI can have one,
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
else in this repository implements it: the C++ host, and every language binding. The
sections below describe how the things this header settles each work.

### Every target shares one table

`PierApi` carries no conditional compilation. The client-only slots and the custom dimension
slots sit in the same place on every build, and hold NULL when the matching package was not
compiled in.

A mod that wants to know whether the host has a capability looks at that slot at runtime.
The same mod source therefore builds for server and client. The marker once kept in the top
bits of the version number, and the patches that maintained it, have been removed.

### Capability packages register themselves

Pier is eight C++ packages. `pier-host` owns the table and the mod lifecycle and does not
know what fills the table. Each capability package registers four things with the host
through a service provider interface: the slot pack it fills, the teardown steps it needs,
whether it vetoes an unload, and the events it provides.

Delete a package from the build and it registers nothing: its slots stay NULL, and the host
does not change by one line. Each package is one `includes(...)` line of the root build
file; remove the line, and configuring, building and running all go on as before. A check
removes each package in turn and builds again on every push.

Self-registration depends on every package being an object library and not a static one. A
linker drops the units of a static library that no outside symbol refers to, and the
registrations are file-level static objects with no outside reference. Once dropped, the
feature is gone and the startup log says nothing. After this was written into the contract
it was still broken once, in four packages, and the check came after that.

### What crosses the boundary: names and ids

No pointer into the server crosses the ABI. A player is a selector, an actor an id, a block a
dimension and a coordinate, and the host looks each up again on every call.

A handle can therefore be kept across ticks. Once the actor is gone, the host cannot find it
and the call returns an error; it does not land in freed memory. Each call costs a lookup.

Looking a player up by **name** needs care: when no account name matches, the selector falls
back to the display name, which another mod can change. A player who sets their display
name to an offline player's account name receives every call addressed by that name. So
permissions, money and ownership use the xuid, and the Rust binding gives the two selectors
different types, which shows at the call which one is in use.

### Whoever allocates a buffer frees it

A buffer crossing the boundary is allocated and freed by its producer, and the receiver
copies what it keeps. No call returns a pointer for the other side to free: that would need
both sides to share an allocator, and a mod and the host each have their own. Every output
goes through a sink.

### When a slot cannot answer

A slot that cannot answer does one of three things: it returns a value that can mean "no
answer"; it logs, falls back, and says in the log what it fell back to; or it refuses.

Take a function deciding whether a player may enter a plot, which cannot answer when the
player cannot be read. Returning a bare boolean, it would give `false` and the caller could
not tell "may not enter" from "unknown", or give `true` and let the unreadable player in. So
the return type of such a function has room for the third case.

### How the contract grows

Adding a capability appends a slot and leaves the ABI version alone. Reordering, removing or
changing a slot advances both version numbers together, and a mod built before that is
refused at load with the reason in the log, rather than running into a slot that moved.

The load check is a range: `MIN_SUPPORTED <= the mod's version <= the host's version`, so a
mod built against an older Pier keeps loading.

### The checks in the repository

The object library rule was first only written in the contract, and a delivery note said it
held; then it was broken in the four packages that depend on self-registration. Since then
every rule of the contract has a script in `tools/checks/`, and
`python3 tools/run-checks.py` runs them on every push: that the header parses as C11, that
the slot order only grew, that the Rust mirror matches it parameter by parameter, that no
capability package has a sideways edge, that no comment describes something the code does
not do.

Each check states what it covers and what it cannot see. A delivery note citing a check
copies that sentence, so a reader knows which parts still need a person to look.

The rules themselves are in [`CONTRACT.md`](CONTRACT.md), with the reasoning for each.

## Writing a mod

**[Rust](docs/rust/index.md)** is the first official binding and the most complete one;
**[C++](docs/cpp/index.md)** works on the header directly, **[Go](docs/go/index.md)**
builds a mod as a c-shared DLL, and **[Zig](docs/zig/index.md)** reads the header through
translate-c.

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
- [pier-mod-template](https://github.com/Maskviva/pier-mod-template) is a Rust mod that runs
  as it is: it logs, subscribes to chat, registers a command and schedules a task. Start
  from it with GitHub's *Use this template*, or with `cargo generate`.

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
lip install github.com/Maskviva/Pier
```

Or unpack the release archive into `plugins/pier/`.

Pier needs LeviLamina 26.51.5 on BDS 1.26.51.
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
