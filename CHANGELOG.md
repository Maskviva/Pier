# Changelog

All notable changes to this project are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). Pier is
versioned as `<BDS major>.<BDS minor>.<release>`, so `26.20.1` is the first release for
BDS 1.26.20. The ABI carries its own version, currently v1, which moves far more slowly.

## [26.20.2] - 2026-09-05

A performance and correctness release for BDS 1.26.20 / LeviLamina 26.20.4. The ABI stays
at v1: every new slot is appended and gated by `struct_size`, so a mod built against 26.20.1
loads unchanged and a mod built against 26.20.2 still loads on a 26.20.1 host, with the new
slots reported as absent. The Rust binding moves to pier-rs 1.1.0 / pier-sys-rs 26.20.2.

### Fixed

- **TPS and MSPT were the frame period.** `get_tick_delta_time` exposes `mTickDeltaTime`,
  the wall-clock period of the last frame including the idle sleep BDS inserts to hold
  20 Hz. Its reciprocal read above 20 on about half the frames of an idle server, stayed at
  20 while the world was frozen, disagreed with `/tick warp` in both directions, and
  `tick_delta_time * 1000` reported as MSPT read 50 on an idle server. New `get_tps` and
  `get_mspt` count the `Level::tick` calls that really run and time them, over a 1..60 s
  window (`TickStats.cpp`). `Server::tps()` reads the new slot; `Server::mspt()` is new.
- **Profiler `level_tick` under warp.** The detour wrapped the whole warp loop, so
  `/tick warp 5` made the per-tick average five times too large. It now sits innermost.
- **Text packets.** `player_send_message_typed` sent a `MessageOnly` body under the Chat,
  Whisper and Translate types; the bodies now match the type on the wire.
- **Inventory-UI drops.** `PlayerDropItemEvent` recognized any single container action as
  a drop and cancelled creative pickups and crafting remainders with it; a drop now also
  needs the world-interaction action the client sends.
- **`/tick step` stalls.** A frozen world ran every queued step tick in one frame; 1200
  steps were one frame a minute long. At most 100 run per frame, the rest carry over.
- **SNBT floats.** `snbtNum` on a floating point value could emit `1e+21`, `inf` or `nan`,
  which the parser on the far side refuses, losing every later field.
- **Unload race.** `ModHost::unload` read the callback counter and then called
  `FreeLibrary`; a callback starting on another thread in between ran in unmapped code.
  An `unloading` gate now closes first and every asynchronous dispatch site tests it.
- **Dimension rule spawn.** With natural spawning fully denied the engine still built the
  mob and despawned it; it is refused before the spawn.
- **Rust `Event::cancel`** cloned and reserialized the whole payload; it writes back only
  the keys it touched, which the host diff already supported.
- **`ProjectileEvent`** declared its detour on `BedrockSpawner`, which declares no
  override; it is declared on `Spawner`, whose symbol it hooks.

### Performance

- **Packet interception filtered by id.** `packet_hook_register_ids` and
  `Packets::intercept_ids`: a packet no subscriber listed passes through before any lock,
  snapshot or address lookup. The unfiltered slot made every packet in a direction, chunk
  data included, pay one callback per subscriber.
- **Bulk block reads and writes.** `scan_region` walks chunk by chunk and caches the SNBT
  per block state; `scan_region_indexed` reports a palette plus indices;
  `edit_fill_region` and `edit_set_blocks` resolve a spec once per call. Rust:
  `World::scan_indexed`, `fill_region`, `set_blocks`.
- **Player selectors cached** to `ActorUniqueID` with a verified `fetchEntity`; a repeat
  lookup no longer scans every player and allocates a name per player.
- **`ItemStack` reads Name, Count, Damage and `tag` locally** from a cached parse instead of
  crossing the ABI for each.
- **Event write-back** diffs against the live tag and no longer reparses the snapshot;
  hook events hand over a static id string.
- **Dimension rules and plot grids** answer from atomics on the hooked hot paths
  (liquid flow, fire, spawning, pistons, explosions, riding, `Actor::move`).
- **KvDb** locks per database instead of one lock for every database of every mod.
- **Sidebar** sends only the rows that changed; an unchanged refresh sends nothing.
- **`container_get_items`** reads a whole container in one call (`Container::items`).
- **`level_set_biome`** looks a chunk up once per chunk instead of once per column.
- **Legacy `schedule` and money listeners** resolve the owning module with one
  `GetModuleHandleExW` instead of one per hosted mod.

### Added

- ABI slots (all appended): `get_tps`, `get_mspt`, `packet_hook_register_ids`,
  `scan_region_indexed`, `edit_fill_region`, `edit_set_blocks`, `container_get_items`;
  sink types `PierPaletteSink`, `PierCellSink`, `PierSlotSink`; struct `PierBlockCell`.

## [26.20.1] - 2026-09-02

The first release of Pier.

Pier is a LeviLamina mod that loads mods written in other languages. It exposes the
Bedrock server through one C ABI, so a mod is a dynamic library that speaks that ABI and
the language is the author's choice. **Rust is the first official binding**; anything with
a C FFI can have one, and adding a binding needs no change to Pier.

Pier replaces [levilamina-rust-loader](https://github.com/Maskviva/levilamina-rust-loader),
which was a test and is no longer maintained. It is a redesign rather than a rename, and
mods written for the loader do not carry over. See **Migrating from the loader** below.

### Added

- **The contract.** `sdk/abi.h`, 194 slots, parseable as C11 and as C++20. It is the
  product; everything else in the repository is one implementation of it, and a binding
  needs no other file.
- **One layout on every build target.** No conditional compilation inside `PierApi`. An
  absent capability is a NULL slot rather than a missing field, so the same mod source
  builds for a client host and a server host alike.
- **Capabilities register inward.** Eight C++ packages; a capability registers itself with
  the host through an SPI at four points rather than being called out to. Deleting an
  optional package from the build changes no host code and leaves its slots NULL.
- **The Rust binding**, `pier-rs`, exposed as the crate `levilamina`. Players, world,
  blocks, actors, events, commands, forms, scoreboards, containers, an economy bridge, a
  key-value store, raw packet interception, custom dimensions, simulated players, tick
  control and profiling.
- **29 synthetic events** built with native detours, covering what LeviLamina has no
  equivalent for: a block destroyed by something that is not a player, an explosion,
  liquid flowing across a boundary, a piston reaching into a neighbouring plot, two chests
  pairing across one, and more.
- **Three cross-mod channels**: a named service with a reply, a broadcast bus, and a
  same-toolchain fast lane.
- **19 machine checks**, run by `python3 tools/run-checks.py` on every push. Each states
  what it covers and what it cannot see.
- **Documentation** at `docs/`, and
  [pier-mod-template](https://github.com/Maskviva/pier-rs-mod-template) as a working starting
  point.

### Design decisions worth knowing

- **An error is an error.** No slot answers a question it could not determine with a
  plausible value. Cannot-be-determined and the answer being no are kept apart all the way
  down, because a mod reading an unresolvable dimension as the overworld is how land
  protection silently stops protecting anything outside it.
- **Handles are identities, not pointers.** A player is a selector, an actor is an id.
  Each call resolves again, so a handle kept across ticks returns an error once the actor
  is gone rather than jumping into freed memory.
- **A player name is not an identity.** The host falls back to the display name when no
  account name matches, and another mod can change a display name. Permissions, economy
  and ownership take an xuid, and the Rust binding writes that distinction into the type.
- **The contract only grows.** Adding a capability appends a slot and leaves the ABI
  version alone. A mod built against an older Pier keeps working.

### Requirements

| | |
|---|---|
| Bedrock Dedicated Server | 1.26.20 |
| LeviLamina | 26.20.4 |
| Platform | Windows x64 |

[LegacyMoney](https://github.com/LiteLDev/LegacyMoney) is optional. It is delay-loaded, so
a server without it starts normally and only the economy calls return failure values.

### Installing

```bash
lip install github.com/Maskviva/pier
```

Two archives are attached to this release:

| | For |
|---|---|
| `pier-server-windows-x64.zip` | Bedrock Dedicated Server. This is what lip installs. |
| `pier-client-windows-x64.zip` | The Minecraft client, through LeviLamina client. |

Unpack the matching one into `plugins/pier/`. They are not interchangeable: bit 0 of
`mod_flags` differs, and a mod built for one target is refused during the handshake with a
message saying so rather than being allowed to fail later.

### Migrating from the loader

A mod written for levilamina-rust-loader needs three changes:

1. `manifest.json`: `"type"` becomes `"pier"`, and the dependency becomes
   `{ "name": "pier" }`. A wrong type means the mod is never scanned and nothing is
   reported.
2. `Cargo.toml`: depend on `pier-rs` rather than the loader.
3. `ctx.server()` becomes `ctx.host()` for the host and system level calls. The event and
   command APIs changed shape in places; the compiler finds those.

The ABI is not compatible and no attempt is made to load a loader mod.

### Known limitations

- **Windows x64 only.** That is what BDS ships for.
- **The client build is new and less exercised than the server one.** The 19 machine
  checks all evaluate the server configuration, and the client target excludes
  `pier-hooks` and `pier-dimensions`, so the synthetic events and custom dimensions are
  absent there and their slots are NULL. Report anything that looks wrong.
- **A command cannot be deregistered.** Bedrock has no route for it, so a command
  registered by a mod lives until the server stops. While the mod is disabled the host
  mutes its callback and re-enabling resumes it.
- **`optional-drops` passes a necessary condition only.** It verifies that no optional
  package's symbols are referenced across packages; the sufficient criterion is really
  deleting the line and running a configure.
- **A form callback may never run.** If the mod is disabled before the player answers,
  the host mutes the callback and the closure is leaked on purpose, because the code that
  could free it lives in a library that may already be unloaded. Cleanup that has to
  happen does not belong in a form callback.

[26.20.2]: https://github.com/Maskviva/pier/releases/tag/26.20.2
[26.20.1]: https://github.com/Maskviva/pier/releases/tag/26.20.1
