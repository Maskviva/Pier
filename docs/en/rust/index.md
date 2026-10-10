# The Rust binding

Rust is the first official Pier binding. This section is for writing a mod; it assumes
nothing about the ABI underneath.

If you want to know what Pier is or why it looks the way it does, that is
[the Pier section](../guide/what-is-pier.md). If you want to bind a different language, that is
[Adding a language](../guide/adding-a-language.md).

## Two crates

| | |
|---|---|
| `pier-rs`, exposed as the crate **`levilamina`** | What you write against. Safe, and the only one you name. |
| `pier-sys-rs`, exposed as **`levilamina_sys`** | The raw FFI mirror of the header. Every call is `unsafe`. |

You depend on `pier-rs` and write `use levilamina::`. The package name says which ABI it
belongs to; the crate name follows what you are actually writing, which is LeviLamina mod
code.

```toml
[lib]
crate-type = ["cdylib"]

[dependencies]
pier-rs = { git = "https://github.com/Maskviva/pier", tag = "26.51.2" }
```

## The smallest mod

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

`on_enable`, `on_disable` and `on_unload` are optional and default to doing nothing.
`register_mod!` generates the entry point the host looks for; without it the mod is
refused at load.

## What this layer adds

`levilamina` sits on `levilamina_sys` and does four things, and no more:

- **The two slot gates.** Every non-core call checks that the host's table reaches the
  slot and that the slot is non-null, and turns a failure into an error naming which.
- **String handling.** Nothing crossing the boundary outlives the call it came from, so
  what you receive is already copied.
- **A panic fence.** A panic crossing back into C++ is undefined behaviour, so every
  callback is wrapped. See [Errors and logging](errors.md#panics) for which way each
  degrades.
- **Copying inside a sink.** The host's pointer dies when the sink returns.

It caches no host state, feigns no synchronization, and covers for the host in nothing. A
failure is an `Err` that says why and never quietly becomes a default.

## Where to go

- [Your first mod](first-mod.md), building and installing one
- [The mod lifecycle](lifecycle.md), the four callbacks and what belongs in each
- [Events](events.md) and [Commands](commands.md), the two things almost every mod does
- [Errors and logging](errors.md), the discipline this binding is built around
- [Threads](threads.md), which thread you are on and how to get back
- [API map](api.md), the tour, with rustdoc holding the detail

[pier-mod-template](https://github.com/Maskviva/pier-mod-template) is a Rust mod that runs as
it is: it logs, subscribes to chat, registers a command and schedules a task. Start from it
with GitHub's *Use this template*, or with `cargo generate`.
