# Changelog

All notable changes to this project are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). Pier is
versioned as `<BDS major>.<BDS minor>.<release>`, so `26.20.1` is the first release for
BDS 1.26.20. The ABI carries its own version, currently v1, which moves far more slowly.

## [26.32.1] - 2026-09-07

A correctness release for BDS 1.26.32. Everything here was found and fixed after
26.32.0 was already tagged and shipped; see that entry for the migration itself.

### Changed

- **Requires LeviLamina 26.32.1**, up from 26.32.0.
- **CI pins LLVM to 21.1.8.** LeviLamina 26.32.x pins its own build to clang-cl and does
  not compile under the Clang 20.1.8 a GitHub-hosted `windows-2022` runner ships:
  `MinecraftCommands.h` holds `unique_ptr` members of four forward-declared types beside
  a defaulted destructor, which Clang 20 instantiates at the class definition and rejects
  and Clang 21 does not. Confirmed on the same commit (`31d23a0c`) and the same MSVC STL
  (14.44.35207) locally before this was added; filed upstream. This is an engine build
  defect and not a Pier one, so it costs nothing at the ABI or the mod level. The
  workflows install the toolchain via LLVM's NSIS installer rather than the `clang+llvm-`
  archive: the archive carries LLVM's own libraries on top of the toolchain, which made
  it 900 MB and slow enough extracting on a Windows runner to matter.

### Added

- **`load_level` in manifest.json.** An optional integer, default 0; lower loads earlier
  and mods sharing a level keep loading by name, which is the order they had before the
  field existed. It orders and does not sequence: a mod that cannot start without another
  declares a dependency, which the host checks. A level is for a mod that has a
  preference and nothing to hang it on. A non-integer value is reported as a problem in
  `/pier list` rather than rounded to 0.

### Fixed

- **Registering a command, an enum or a soft enum before the engine had a command
  registry took the whole server down with no log line.** All five entry points reach
  `CommandRegistrar::getServerInstance()`, which resolves the registry without checking
  that one exists; reading it too early is an access violation, and an access violation
  is SEH rather than a C++ exception, so the `catch (...)` around those calls never saw
  it and neither did `PIER_API_GUARD`. BDS left without writing anything and LeviLamina
  wrote no crash log, which is why the symptom read as "any command call kills the
  server" rather than as an ordering mistake. The five now ask
  `ll::service::getCommandRegistry()` first and refuse with a log line saying what to do.
  `getCommandRegistry` rather than `levelReady`, because a level can be up while the
  registry is not. Found by a downstream mod that hit it in production; `tools/pier-probe`
  never exercised these slots, see the amended note under 26.32.0's Verified section.

### Requirements

| | |
|---|---|
| Bedrock Dedicated Server | 1.26.32 |
| LeviLamina | 26.32.1 |
| Platform | Windows x64 |

## [26.32.0] - 2026-09-07

The BDS 1.26.32 release. Mojang's build now inlines a large part of what used to be a
callable symbol, so LeviLamina 26.32 declares about a third fewer non-virtual functions
than 26.20 did while virtual declarations are untouched. Nothing in the engine went away;
the wrappers around it did. Where a wrapper is gone this release reaches the same data
through the ECS accessor namespaces, a public member, or a surviving virtual, and where
the reachable thing is narrower than the old one the documentation says so rather than
keeping the old wording.

The ABI stays at v1 and no slot is added or retired, so a mod built against 26.20.2 loads
unchanged. One slot reports a narrower number, described below.

### Changed

- **Requires BDS 1.26.32 and LeviLamina 26.32.0.** 26.20 is no longer supported.
- **`profile_take`'s `chunk_blocks` bucket measures less than it did.** `LevelChunk::tickBlocks`
  is inlined away and has no address to detour, so the bucket now times the drain of a
  chunk's pending block-tick queues, scheduled and random, summed over every chunk. Work
  a chunk does around that drain falls outside it, so the figure is a floor on block
  ticking. `calls` counts queue drains, and a ticking chunk contributes two.
- **The crossbow-firework projectile guard asks at the release instead.** `CrossbowItem::_shootFirework`
  is inlined away. The guard hooks the virtual release and reads the charged item, so an
  arrow-loaded crossbow still costs one decision and not two.
- **`DimensionRule::PistonCrossPlot` and `EntityCrossPlot` are now `PistonCrossCell` and
  `EntityCrossCell`** in pier-rs, matching the ABI names since 26.20.3. The old spellings
  stay as deprecated constants with the same values, so an existing mod compiles.
- The Rust binding moves to pier-rs 1.2.0 / pier-sys-rs 26.32.0.

### Added

- **A C++ tutorial beside the Rust one, and a C++ example with four build files.**
  `docs/cpp/` covers what the Rust wrappers do for you and you now do yourself: the entry
  point, the capability check against `struct_size`, `PierStr`, and the bool-plus-out
  error convention. `examples/hello-pier-cpp` is a mod that registers one command, with
  MSVC, clang-cl, CMake and xmake files producing the same DLL from the same source.
- **`PIER_MAIN_EXPORT` in abi.h.** The header named the entry symbol without saying how to
  export it, so a C++ mod compiled, linked and was refused at load. An ELF build exports
  it by default, which is why this took a Windows server to surface.
- `pier-probe` moved from `examples/` to `tools/`. It is a diagnostic, not a thing to
  learn from; `examples/` now holds the two mods that are.
- **`md_retire_dimension(name)`.** Drops a custom dimension from
  `dimension_config.json`, from the host's tables and from the dimension factory, so the
  next boot does not register it and `md_list_dimensions` stops reporting it. The ABI
  stays at v1: the slot is appended and no existing one moves.

  It is not a delete. The chunks stay in the save, the engine keeps the dimension it
  built for this session, and a player standing in it is not moved. Nor is the id handed
  back: registering the retired name later is a new dimension with a new id. That leaves
  the old chunks orphaned and costs disk, and the alternative is worse, because reusing
  the number points a new dimension at the terrain of the old one and nothing about that
  is recoverable.
- **`sky.time` in the dimension spec.** An optional tick of day, 0..23999, beside
  `skylight` and `weather`. The dimension is held at that tick and every other dimension
  keeps following the level clock. It sits in the spec because it describes the sky and
  not the terrain, the same as its two neighbours; the alternatives on the mod side are
  `set_time`, which moves everyone's world, and rewriting SetTime per player, which needs
  a packet hook and fights any other plugin that also manages time. A nether or end
  client sky has no day cycle to begin with, so the field only changes an overworld sky.
- **`verb` in the ExecutingCommandEvent payload.** The canonical command name with
  aliases resolved, read from CommandRegistry. Splitting the raw line on whitespace gives
  a different answer: `/w`, `/tell` and `/msg` are one command behind three names and a
  gate keyed on the first word refuses one spelling while the other two pass. The alias
  table is not visible outside the registry. `/execute ... run <command>` still reports
  `execute`, because the inner command is parsed after this event has been decided.

### Verified on a real server

`tools/pier-probe` walked the whole table on BDS 1.26.32.2 with LeviLamina 26.32.2:
225 probes, 219 answered, none refused, none threw. The six absent slots are the
`client_*` family, which a server build does not fill. `server_info_str[1]` reported
protocol 1001, which is the number the server's own startup banner prints, so the
replacement for the inlined-away `NetworkProtocolVersion` reads the same value the engine
does rather than a plausible one.

*Amended after 26.32.1:* read that count for what it measures. The census half of the
probe calls nothing: it reports whether each slot is present, NULL, or past
`struct_size`. The half that does call only calls slots that change no state. **No
writing slot and no registration slot was exercised**, so "none threw" says nothing
about them; the first real call to command registration on this engine version happened
after this release shipped and found the missing guard fixed in 26.32.1.

### Fixed after the first run on a real server

- **`game_rule_get` and `game_rule_set` answered for `pvp` and nothing else.**
  `GameRules::nameToGameRuleIndex` is inlined away in 26.32 and the host walks
  `GameRule::mName` instead. That field is spelled the way `GameRulesIndex` is,
  `showCoordinates` rather than `showcoordinates`, while `/gamerule` and every existing
  caller write it in lower case, so an exact comparison matched only the one rule name
  with no case to get wrong. The match folds ASCII case now.

### Capabilities that now report failure

Every one of these is a slot that still exists and still returns; it answers false or
nothing instead of a value. The engine function behind it is inlined away with no field
or component left to read it from, or it is compiled only on the client platform. A mod
that treats a false from one of these as fatal will refuse to start, so the list is here
rather than only in the source.

- `actor_property`: IS_IN_LAVA, IS_PERSISTENT, HAS_TOTEM.
- `actor_action`: SET_VARIANT, SET_MARK_VARIANT, SET_SKIN_ID. SET_SCORE_TAG still works:
  the engine exports the string specialization of the synched-data write and not the
  integer one.
- `player_property`: LUCK, IS_EMOTING, IS_IN_RAID, HAS_RESPAWN_POSITION, DIRECTION,
  IS_USING_ITEM.
- `player_action`: RESEND_ALL_CHUNKS, REGISTER_TRACKED_BOSS, UNREGISTER_TRACKED_BOSS.
  The last two are client-platform only.
- Attribute writes through `player_attribute_set`. Writing the field directly would skip
  the listener pass that syncs the value, so the attribute would move on the server and
  not on screen.
- `item_property`: HAS_DURABILITY, IS_FIRE_RESISTANT, IS_MUSIC_DISC, IS_OFFHAND,
  IS_UNBREAKABLE. `item_string`: EFFECT_NAME.
- `item_op`: SET_CAN_DESTROY, SET_CAN_PLACE_ON. The field holds resolved block pointers
  beside a hash the engine compares against, and a wrong hash stops the restriction from
  applying without saying so.
- `block_property`: IS_UNBREAKABLE.
- Actor copying in `world_edit`: the id-remapping helper needed to avoid colliding with
  the source actor is gone.
- **Creating a custom dimension.** `md_add_dimension` refuses and returns -1: it needs an
  id from the engine, and both entry points that supplied one
  (`DimensionManager::serverRegisterCustomDimension`, which allocated it and wrote it to
  the save's NameIdStore, and `getDimensionId`, which read that table back) are inlined
  away with no symbol left. A dimension a save already holds cannot be found again
  either. `md_retire_dimension`, `md_list_dimensions` and the dimension rules are
  unaffected, and so is every non-dimension slot.

  Separately, and unrelated to this release: the four `md_*` slots retired in 26.20.3
  (`md_add_simple_dimension`, `md_add_plot_dimension`, `md_set_plot_grid`,
  `md_clear_plot_grid`) are still stubs that log once and refuse.

Two more answer a narrower question than before rather than failing: `profile_take`'s
`chunk_blocks` bucket, described above, and the potion effect list, which no longer
distinguishes an infinite duration.

### Fixed

- **Comments that were not true.** `SpecDimension.cpp` said a file that was still in the
  tree and still compiling had been deleted, and both it and `spec_dimension.h` said the
  mod-side record and `dimension_config.json` hold the same bytes. The manager writes the
  tag back out with `SnbtFormat::Minimize`, so the equality that holds is structural.
- **Retired source was still in the tree.** `SimpleCustomDimension` and the four `plot/`
  files are cut; `SpecDimension` and `gen/` replace them.

### Requirements

| | |
|---|---|
| Bedrock Dedicated Server | 1.26.32 |
| LeviLamina | 26.32.0 |
| Platform | Windows x64 |

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

[26.32.1]: https://github.com/Maskviva/pier/releases/tag/26.32.1
[26.32.0]: https://github.com/Maskviva/pier/releases/tag/26.32.0
[26.20.2]: https://github.com/Maskviva/pier/releases/tag/26.20.2
[26.20.1]: https://github.com/Maskviva/pier/releases/tag/26.20.1
