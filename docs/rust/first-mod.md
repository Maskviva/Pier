# Your first mod

This is the Rust binding. Pier itself has to be on the server first;
[Installing](/guide/installation) covers that.

## Start from the template

```bash
cargo generate --git https://github.com/Maskviva/pier-rs-mod-template
```

The template is a working mod, not an empty skeleton. It logs, subscribes to chat and
cancels a message, registers a command, and schedules a task. Delete what you do not
need.

## Or start from scratch

```toml
# Cargo.toml
[package]
name = "my-mod"
version = "0.1.0"
edition = "2021"

[lib]
crate-type = ["cdylib"]

[dependencies]
pier-rs = { git = "https://github.com/Maskviva/pier", tag = "26.20.1" }
```

The package is `pier-rs` and the crate it exposes is `levilamina`, which is why the
imports read `use levilamina::`. The package name says which ABI it belongs to and the
crate name follows what you are actually writing: LeviLamina mod code.

```rust
// src/lib.rs
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

`register_mod!` generates the entry point the host looks for. Without it the mod is
refused at load time.

## The manifest

```json
{
  "name": "my-mod",
  "entry": "my_mod.dll",
  "type": "pier",
  "version": "0.1.0",
  "dependencies": [{ "name": "pier" }]
}
```

Three things have to agree:

- `name` matches the directory under `mods/`.
- `entry` matches what cargo produced. Cargo turns hyphens into underscores, so the
  crate `my-mod` builds `my_mod.dll`.
- `type` is exactly `pier`.

## Build and install

```bash
cargo build --release
```

Copy `target/release/my_mod.dll` and `manifest.json` into `<server>/mods/my-mod/`, then
start the server.

## When nothing happens

Work down this list; each rules out one thing.

**The mod does not appear in `/pier list`.** The manifest is the usual cause: check
`"type": "pier"`, and check that `name` matches the directory. A wrong type means the
mod is never scanned, and nothing is reported anywhere.

**The server refuses to load it and says so.** Read the line. The host states which of
the three handshake checks failed: the table length, the ABI version range, or the target
flags. A target mismatch means a client mod on a server build or the reverse.

**It loads, but a subscription never fires.** The event id is almost certainly wrong.
Run `/pier events` for the ids this build resolves, and use the constants in `names`
instead of a literal so a typo fails at compile time.

**A call returns an error saying the host does not provide it.** That capability package
was not compiled into this build. The message names the slot; `ctx.host_abi()` gives the
version and table length to include in a report.

## Next

- [The mod lifecycle](./lifecycle) covers the four callbacks and what may be done in each.
- [Events](./events) covers subscribing, reading a payload and cancelling.
- [Errors and logging](./errors) covers the error discipline this binding is built around.
