# Events

```rust
use levilamina::prelude::*;
use levilamina::event;

let listener = event::subscribe(names::PLAYER_CHAT, |ev| {
    let Ok(message) = ev.str_at("message") else { return };
    Logger::get().info(&format!("[chat] {message}"));
})?;
```

Hold the returned `Listener`; dropping it unsubscribes.

## Use the constants, not literals

`event::subscribe` takes a string, and a misspelled id **subscribes successfully and then
never fires**. The `names` module turns that into a compile error:

```rust
event::subscribe(names::PLAYER_CHAT, ...)     // good
event::subscribe("PlayerChatEvent", ...)      // works, until you typo it
```

Every constant states whether the event can be cancelled and what its payload carries.
`/pier events` lists what a given build resolves.

## Two kinds of event

**Registry events** come from LeviLamina and are spelled in full,
`ll::event::PlayerChatEvent`. A unique suffix also resolves, but a suffix becomes
ambiguous the day upstream adds an event with the same name.

**Synthetic events** are ones Pier builds with native detours because LeviLamina has no
equivalent. They have bare names such as `BlockDestroyEvent`. There are 29, and they fill
the gaps that matter for protection: a block destroyed by something that is not a player,
an explosion, liquid flowing across a boundary, a piston reaching into a neighbouring
plot, two chests pairing across one.

`names::ALL_SYNTHETIC` is the full list, which makes a startup self-check easy.

## Reading a payload

A payload is SNBT. The typed accessors keep a missing key apart from a wrong type, and
both apart from a value that happens to be zero:

```rust
let dim = ev.dim()?;                    // Err if the host could not resolve it
let pos = ev.pos()?;                    // (x, y, z)
let who = ev.player();                  // Option<PlayerIdentity>
let n   = ev.i32_at("count")?;          // Err names the key and the actual type
let s   = ev.opt_str("reason");         // Option, for when absence is fine
```

`ev.dim()` returning an error is the point. A land protection mod that read an
unresolvable dimension as `0` refused inside the overworld and allowed everything
everywhere else, silently. Handle the error; refuse when you do not know.

The host injects `_unresolved` when it could not resolve part of the payload.
`ev.unresolved()` returns those field names, and a non-empty list means the payload is
incomplete. A protection decision should refuse rather than guess when it is.

## Cancelling

```rust
if should_block {
    ev.cancel()?;
}
```

`cancel()` returns a `Result` because not every event can be cancelled. An event that
cannot says so and names the one to block instead:

```
event `PlayerStartDestroyBlockEvent` cannot be cancelled: emitted before origin only to
record who started mining which cell; use PlayerDestroyBlockEvent or BlockDestroyEvent
to block it
```

Returning `()` here would let a protection mod believe it had blocked something. That
belief is worse than a crash, because a crash is visible.

An `Ok` means the cancel bit reached the host, not that the engine stopped. A few hook
points sit half updated and the host does not accept a cancel there at all; the event
constant says so where that applies.

## Editing a payload

```rust
ev.edit(|v| {
    v.insert("message", NbtValue::from("edited"));
});
```

`ev.set("message", value)` is the shorthand when only one field changes.

The write-back is a difference: only keys you actually touched go back, so two mods on
the same event do not erase each other's edits.

## Subscribing to many events

`Wiring` batches subscriptions and holds the handles together:

```rust
let wiring = Wiring::new("my-mod")
    .on("chat", names::PLAYER_CHAT, |ev| { ... })
    .on("break", names::PLAYER_DESTROY_BLOCK, |ev| { ... })
    .at("explode", names::EXPLOSION, Priority::High, |ev| { ... })
    .arm()?;
```

`arm()` fails as a whole if any single subscription fails, and unsubscribes the ones that
succeeded before returning. Half-attached protection is worse than none: some points
block, some do not, and nobody knows which. `arm_lenient()` is the other choice, for a
feature that is nice to have, and records what failed in `failures()`.
