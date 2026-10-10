# C++ API

A C++ mod compiled with the Pier SDK includes `sdk/abi.h` directly, receives the function table `PierApi` in `pier_main`, and calls `api->slot_name(...)` from then on. The 201 slots of the table are spread over the pages below by subject. Each slot names its section in abi.h and its position in the table, and lists the interfaces of Rust, Go and Zig that call it.

Before a slot is called, two things are checked: that `struct_size` reaches the slot, and that the slot is not NULL. The [C++ binding](../../cpp/index.md) page shows how, with a complete mod.

`bindings/bridge/pier-bridge.h`, for LeviLamina native plugins calling Pier services, is not part of this reference; see [bridge](../../guide/bridge.md).

| Page | Slots and macros | Types | Constants |
|---|---|---|---|
| [Core](core.md) | 13 | 4 | 0 |
| [Events](events.md) | 3 | 2 | 0 |
| [Commands](commands.md) | 6 | 2 | 0 |
| [Server, ticks and system information](server.md) | 16 | 0 | 8 |
| [World](world.md) | 31 | 0 | 0 |
| [Bulk world editing](edit.md) | 7 | 0 | 0 |
| [Players](player.md) | 22 | 0 | 74 |
| [Actors](entity.md) | 19 | 0 | 85 |
| [Blocks](block.md) | 7 | 0 | 42 |
| [Items and containers](item.md) | 15 | 0 | 52 |
| [Scoreboard](scoreboard.md) | 1 | 0 | 10 |
| [Forms](form.md) | 1 | 1 | 0 |
| [NBT and the key-value database](data.md) | 10 | 3 | 0 |
| [Economy](money.md) | 10 | 1 | 4 |
| [Packets](packet.md) | 6 | 5 | 7 |
| [Simulated players](sim.md) | 4 | 0 | 0 |
| [Client](client.md) | 6 | 4 | 0 |
| [Custom dimensions](dimensions.md) | 13 | 5 | 30 |
| [Cross-mod](crossmod.md) | 16 | 5 | 8 |
| [Registries](registry.md) | 1 | 0 | 3 |
| [Shared types](types.md) | 0 | 10 | 0 |

## The header of abi.h

The file header states the conventions of the whole ABI:

```text
Pier ABI — sdk/abi.h (ABI v2)

This header is the product: the sole contract between the C++ host
(pier-host plus the capability packages) and an SDK written in any language.
The reference mirror is the pier-sys-rs crate under bindings/, hand-written with no
bindgen, which doubles as readable annotation for this file.

This file must parse as C. Consumers are "any language", so it uses C11 only:
no std::string_view, no enum class, no nested types. The C++ convenience
wrappers (PierStr to string_view and back) live in pier-support, not here; a
language-specific type in the contract forces every other language to guess
that type's layout. CI compiles this file once as C11 and once as C++20.

Rules for changing this file. These are the only versioning rules anywhere.
  1. Append at the end of PierApi only. Never reorder, remove, or change the
     signature of an existing slot. Appending does NOT bump PIER_ABI_VERSION.
  2. After appending, update every SDK mirror slot for slot; the
     sys-mirrors-abi check enforces the ordering.
  3. Only a non-append change (reorder, removal, signature change) advances
     PIER_ABI_VERSION and PIER_ABI_MIN_SUPPORTED, both to the same number.

Appending does not bump the version because the version answers "which
already-compiled mods still load". An appended slot invalidates no old mod:
the old table is a byte-identical prefix of the new one, and an old mod can
never reach the new slot. Bumping for it would announce an incompatibility
that does not exist. Each direction has its own gate instead: a new host with
an old mod is covered by the version range (see PIER_ABI_MIN_SUPPORTED); an
old host with a new mod is covered by the mod comparing struct_size slot by
slot, reporting "host lacks this capability" for the one call that overruns.

The layout is identical across all build targets. PierApi carries no
conditional compilation: slots for client-only and dimension capabilities are
always present in the layout and are simply NULL when that package was not
built into the host. "Capability present" means "slot is non-NULL", and the
SDK reports "host does not provide X" from that. This buys three things:
mirrors need no conditional compilation, a cross-target mismatch cannot call
the wrong slot, and the struct has exactly one append point, the end.

Conventions for the whole file; per-slot comments record only the exceptions.
  - Strings are UTF-8 (ptr, len) views and are NOT guaranteed NUL-terminated.
  - A string passed into a callback is owned by the caller and valid only for
    that call; copy it to keep it.
  - A mod hands strings out through a sink callback within the current call
    frame. Ownership never crosses the boundary: this ABI has no "returns a
    pointer the other side must free".
  - Threading: unless a slot says otherwise, call only on the server thread.
    log, gaming_status, schedule and schedule_after are thread-safe. Every
    callback (event, command, scheduled task) fires on the server thread.
```

```text
World reads (scan)
```

```text
Per-domain payload types
```

```text
Cross-mod event bus FFI types
A mod cannot hand another mod a function pointer: `ModHost::unload`
calls FreeLibrary, so the publisher would be left holding a pointer into an
unmapped dylib. The loader therefore owns the subscription table, with the
same weak_ptr + ticket discipline as Forms.cpp and the mod-scoped scheduler.

The loader never parses `payload` — it is opaque UTF-8 (JSON, SNBT, or
anything else the two mods agree on). Keeping the loader format-agnostic is
deliberate: the alternative is a schema that every publisher has to satisfy
and that the loader has to version.

Topics are plain strings; namespace them (`plot:enter`, not `enter`).
```

```text
Same-toolchain fast lane

bus and service are both "(name, UTF-8 payload) -> UTF-8 payload". That shape
is the cross-language common denominator: a mod in any language can speak it.
The price is a serialization round trip per call, with all type information
lost inside the string.

This lane serves one special case: both sides built by the same toolchain, so
the C-layout function tables in the two dynamic libraries are byte-identical
and pointers can be handed over directly.

The loader owns the name -> lane table (exclusive, like service), validates
the fingerprint, issues and collects leases, and holds a liveness flag that it
clears the moment the provider goes away. It does not interpret a single byte
of data or vtable; both pointers are opaque to it, exactly like a bus payload.

The loader has to be involved because ModHost::unload calls FreeLibrary. The
provider's memory can stay alive by reference counting, but its code section
is unmapped, so the consumer's function pointer becomes a use-after-free. The
crash then lands in the consumer, with nothing in the log pointing at the mod
that just left. Hence:
  1. alive points at one cell on the loader's own heap and is never freed
     (lanes number in the dozens, so this leaks a few dozen uint32). The
     loader writes 0 when the provider goes away, and the consumer reads the
     cell before each call: one plain atomic read, no FFI, no lock. That is
     what "fast" means here, the loader runs no code on the hot path.
  2. When the provider goes away, the loader calls release for every
     outstanding lease before FreeLibrary, so the provider frees its own
     objects inside its own dylib with its own allocator.

Most native languages have no stable ABI. The same contract type compiled
twice into two cdylibs can end up with different field order when compiler
metadata differs, and that is silent memory corruption rather than a crash.
So the check is a fingerprint, not a version number: compiler version, target
triple, contract name and version, and the type identity, size and alignment
of the function table, all folded into one u64. Any difference yields a
different fingerprint, lane_acquire returns PIER_LANE_FINGERPRINT, and not a
single pointer is handed over.

The failure mode is "slow" (the consumer falls back to the service channel),
never undefined behavior. That property is the entire reason this lane is
allowed to exist.

The host compares fingerprints for equality and never interprets them; it has
to be that way, or "add one more item to the fingerprint" would become an ABI
change.
```

```text
Packet interception FFI types
Used by packet_hook_register / packet_conn_hook_register. See the block
comment on those fields in PierApi for the full contract.
```

```text
FFI types for the client capability group. The type declarations are always
present (they take no layout); whether the capability is available is decided
by whether the client_* slots in PierApi are NULL.
```

```text
Property and action keys. APPEND-ONLY: never renumber or remove. Unknown
values make the call return false; a safe SDK layer maps that to an
"unsupported" error, which is the forward-compatibility negotiation.
```
