# ✍️ Your first Rust mod

This tutorial takes you from nothing to a Rust mod: first it writes its first log line on
the server, then it greets players as they join. You need some Rust, not a lot.

## Before you start

- **The Rust toolchain**: install [rustup](https://rustup.rs); on Windows the default MSVC
  target is the right one.
- **A server with Pier installed**, to test on. If you have none yet, see
  [Installation](../guide/installation.md).

## Step one: create a project

The quickest way is the template. On its
[GitHub page](https://github.com/Maskviva/pier-mod-template), click *Use this template* to get
a repository of your own, or generate a local copy:

```bash
cargo generate --git https://github.com/Maskviva/pier-mod-template
```

The template is a mod that runs as it is: it logs, subscribes to chat and stops one message,
registers a command and schedules a delayed task. Delete what you don't need.

To start from nothing instead, create a library project and make `Cargo.toml` this:

```toml
[package]
name = "my-mod"
version = "0.1.0"
edition = "2021"

[lib]
crate-type = ["cdylib"]

[dependencies]
pier-rs = { git = "https://github.com/Maskviva/pier", tag = "26.51.2" }
```

`crate-type = ["cdylib"]` makes cargo build a DLL, which is what the server loads.

The dependency is called `pier-rs`, while the code says `use levilamina::...`: the crate the
package exposes is named `levilamina`.

## Step two: write the entry point

Make `src/lib.rs` this:

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

When the server loads the mod it calls `on_load`, and your mod logs `hello from Rust`.

`register_mod!` on the last line generates the entry function the server looks for. Without
it, the mod is refused at load.

## Step three: write the manifest

Create `manifest.json` at the root of the project:

```json
{
  "name": "my-mod",
  "entry": "my_mod.dll",
  "type": "pier",
  "version": "0.1.0",
  "dependencies": [{ "name": "pier" }]
}
```

Three things must line up:

- `name` is the name of the mod's folder under the server's `plugins/`;
- `entry` is the name of the file cargo builds. cargo turns the hyphens of the package name
  into underscores, so `my-mod` builds `my_mod.dll`;
- `type` is `pier`, which is what makes Pier take it over.

## Step four: build, install, look

```bash
cargo build --release
```

Then make a `my-mod` folder under the server's `plugins/` and put
`target/release/my_mod.dll` and `manifest.json` in it:

```
plugins/
  Pier/
  my-mod/
    my_mod.dll
    manifest.json
```

Start the server and look for `hello from Rust` in the log. When it is there, your first mod
is running. `/pier list` in the console shows `my-mod` too.

## Step five: greet players as they join

Now make the mod do something: when a player joins, write their name to the log.

Subscribe while **enabling**, and keep the listener the call returns: dropping it ends the
subscription.

```rust
use levilamina::prelude::*;
use levilamina::event::{self, names};

struct MyMod {
    join: Option<Listener>,
}

impl LeviMod for MyMod {
    fn on_load(ctx: &ModContext) -> Result<Self> {
        ctx.logger().info("hello from Rust");
        Ok(MyMod { join: None })
    }

    fn on_enable(&mut self, _ctx: &ModContext) -> Result<()> {
        self.join = Some(event::subscribe(names::PLAYER_JOIN, |ev| {
            if let Ok(name) = ev.str_at("_player.name") {
                Logger::get().info(&format!("{name} joined"));
            }
        })?);
        Ok(())
    }

    fn on_disable(&mut self, _ctx: &ModContext) -> Result<()> {
        self.join = None; // the listener is dropped here, and the subscription with it
        Ok(())
    }
}

levilamina::register_mod!(MyMod);
```

Build again, replace the DLL, restart the server and join: your name appears in the log.

`_player.name` is a field Pier adds to the event; which fields each event has is in the
[event payload reference](../guide/event-payloads.md).

## When nothing happens

Go through these in order; each one rules out one cause:

1. **Your mod is not in `/pier list`**: check that `manifest.json` says `"type": "pier"`,
   then that `name` matches the folder. With the wrong type the mod is never found, and the
   log says nothing.
2. **The log has a line refusing the load**: read it. Pier says which check failed: the
   table size, the ABI version, or the target. A target mismatch means a client mod on a
   server, or the reverse.
3. **It loaded, but the subscription never fires**: the event name is almost always wrong.
   Run `/pier events` in the console to see the names, and use the constants of `names::`,
   which turn a typo into a compile error.
4. **A call returns "not provided"**: the Pier on this server was built without that
   capability. The error names the slot; `ctx.host_abi()` gives the ABI version and table
   size to include in a report.

## Next

- [🔄 Lifecycle and logging](../tasks/lifecycle.md): when each of the four stages happens and what
  belongs in it
- [📣 Events](../tasks/events.md): reading the payload, cancelling
- [⌨️ Commands](../tasks/commands.md): give your mod a command
