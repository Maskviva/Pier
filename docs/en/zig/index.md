# Zig

Pier's Zig binding writes a mod as a Zig dynamic library, for Zig 0.15.2. The package is
`pier`, in `bindings/zig`, and the module it exports is `levilamina`, so code writes
`@import("levilamina")`; the smallest working mod is `examples/hello-pier-zig`.

The package name names Pier, the ABI it belongs to, and the module name what you are
writing, a LeviLamina mod, as the Rust binding's package `pier-rs` exposes the crate
`levilamina`.

## How it is put together

**translate-c reads `abi.h` itself.** The build runs translate-c over the header, so the
types, slots and constants are the C compiler's own reading of it; nothing is mirrored by
hand. The package carries a copy of the header, because a dependency sees only its own
directory, and a check keeps the copy identical to the original.

**Zig calls the slots directly.** `levilamina.slot("name")` is the function pointer of a slot,
or null when the host's table is too short to hold it or the slot is NULL: both gates of
the contract in one call, for every slot of the table. Everything else is built on it.

**`zig build test` checks the binding against the header.** Its test makes the compiler
analyze every function of the binding against the table translate-c produced, so a slot
renamed in `abi.h`, or an argument of the wrong type, fails the build; the CI runs it.

## What you need

- Zig 0.15.2.
- Nothing else: Zig cross-compiles the Windows DLL from any system, and the example's
  default target, `x86_64-windows-gnu`, needs no C runtime beyond the system's.

## The API

| Area | Functions |
|---|---|
| Lifecycle | `exportMod`, and `load`, `enable`, `disable`, `unload` on the mod type |
| Logging | `log`, `logf`, `Context.info`, `warn`, `err` |
| Tasks | `schedule`, `scheduleAfter`, `cancel`, `pendingTasks` |
| Events | `subscribe`, `Event.cancel`, `Event.writeBack`, `Listener.unsubscribe` |
| Commands | `registerCommand`, `executeCommand` |
| Cross-mod | `registerService`, `callService`, `serviceCaller`, `listServicesJson`, `busSubscribe`, `busPublish`, `busPublishVetoable` |
| Properties and verbs | one method per constant on `Player`, `Entity`, `BlockAt` and `Item`, such as `Player.level`, `Player.setLevel`, `Entity.isOnFire` |
| Data | `nbt.parse`, with `get`, `getString`, `getInt`, `getFloat`, `getBool` |
| Everything else | `slot("name")`, with the types and constants of `levilamina.c` |

Errors are `levilamina.Error`: `NotProvided` for a slot the host does not have, `Refused` for a
call the host refused, and `NoAnswer` for a read it could not answer: an unreadable value
comes back as that error, and no zero or false stands in for it. A function that returns text takes an allocator and gives the caller the
memory.

## Talking to other mods

A Zig mod talks to mods of every language, and to native plugins through the
[bridge](../guide/bridge.md), under the same rules as the Rust and Go bindings:

- A **service** provider sends its reply and returns true, or sends the reason and returns
  false; the caller receives that reason unchanged, as `callService`'s `provider_error` and
  its body. `serviceCaller` names the calling mod, and is null for a native plugin.
- A **bus** subscriber returns its veto: true refuses a vetoable publish. A plain publish
  ignores it, and `busPublish` returning 0 means nobody listens, which is no error.
- **Lanes** are not offered: they pair mods built by one toolchain, and a mod that publishes
  one also provides the service a Zig mod uses.

## Rules worth knowing

- **Threads.** Most of the host is server-thread only. Lifecycle steps, event and command
  callbacks run there; a service provider runs on its caller's thread and a bus subscriber
  on the publisher's.
- **Borrowed views.** An `Event`'s `snbt`, an `Invocation`'s `args` and a provider's
  `request` are valid during the callback only; copy what you keep.
- **Panics.** Zig does not unwind: a panic, such as a failed safety check in a ReleaseSafe
  build, ends the server process. Return errors instead; the binding logs an error from a
  lifecycle step and refuses the step.

[Your first Zig mod](first-mod.md) walks through building and installing one.
