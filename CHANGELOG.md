# Changelog

All notable changes to this project are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). Pier is
versioned as `<BDS major>.<BDS minor>.<release>`, so `26.20.1` is the first release for
BDS 1.26.20. The ABI carries its own version, currently v2, which moves far more slowly.

## [Unreleased]

## [26.40.1] - 2026-09-11

Built for BDS 1.26.40 and LeviLamina 26.40.0. Two fixes, both of the same shape: a value
that was wrong rather than missing, and therefore acted on.

`PIER_ABI_VERSION` stays at 2. The two new `server_info_str` keys are additive — a mod
built against 26.40.0 keeps working without a rebuild, though see the note on
`PIER_SRV_PROTOCOL_VERSION` below before relying on the number it used to return.

### Fixed

- **A per-player sidebar stopped updating after its first send.** `PIER_PACT_SIDEBAR_SET`
  updates a sidebar of unchanged row count in place, sending only the rows whose text
  differs. The client stores a row as `scoreboard id -> (name, score)` and reads a
  `ChangeFakePlayerScore` for an id it already knows as a score update, not a rename: the
  number moved, the label did not. So the first send was correct and every later one
  silently did nothing.

  A changed row now releases its id with a `RemoveScore` before the `ChangeFakePlayerScore`
  re-adds it. The removes go out as their own packet ahead of the changes: nothing here can
  verify that a client applies one packet's entries in order, and a remove applied after
  the re-add blanks the row instead of updating it. If the second packet cannot be created
  after the removes have gone out, the cached state is dropped so the next call rebuilds
  from scratch — keeping it would leave those rows blank until the row count changed.

- **`PIER_SRV_PROTOCOL_VERSION` reported the save file's protocol, not the server's.** It
  read `LevelData::mNetworkVersion`, which is a storage tag sitting among
  `mStorageVersion`, `mMinCompatibleClientVersion` and `mLastOpenedWithVersion`, and
  records the version the level was last written by. A 1.21.93 world opened on a 1.26.40
  server answered 819. This looked correct for as long as the test machine's world had
  been created by the server running it.

  It is now derived from `SharedConstants::CurrentGameSemVersion`, which unlike
  `NetworkProtocolVersion` is not behind `#ifdef LL_PLAT_C` and so is readable on a
  dedicated server, mapped through a table of confirmed versions.

  **On a version the table does not list, and on any pre-release build, the key now
  fails.** That is deliberate. The old behaviour was not a crash, it was a confident wrong
  answer, and a caller handed a plausible number acts on it. A pre-release shares its
  `major.minor.patch` with the retail build but not its protocol — 1.26.40-beta.0 speaks
  2168 where retail 1.26.40 speaks 2169 — so a build carrying a `mPreRelease` is refused
  rather than answered from a retail row. The table matches the exact triple: Bedrock
  bumps the protocol on patch releases, so matching on `major.minor` would return a number
  from the wrong end of a release line.

### Added

- **`PIER_SRV_LEVEL_PROTOCOL_VERSION`**, the `LevelData::mNetworkVersion` value under the
  name it deserves. It answers how old the save is, which is a real question and exactly
  what the old key was accidentally answering. `Host::level_protocol_version` on the Rust
  side.

- **`PIER_SRV_GAME_SEM_VERSION`**, `"major.minor.patch"` of the running build, so a mod can
  carry its own version-to-protocol table. A Pier release should not be on the critical
  path for a mod supporting a BDS that shipped yesterday. `Host::game_sem_version` on the
  Rust side.

- `pier-probe` covers `server_info_str` keys 0 through 3. A `Refused` on key 1 is recorded
  as the correct answer for an unlisted version rather than as a fault; comparing it
  against key 2 shows directly when a save is older than the server.


### Added

- **A mod can supply its own terrain, and this host stops trying to understand it.**
  `md_add_dimension_generated` registers a dimension whose spec carries seed, height and
  sky and no terrain section at all; the mod hands over two palettes, resolved once here
  against the block and biome registries, and a callback the chunk workers call with a
  buffer of indices. `md_set_dimension_cells` gives the confinement rules the grid the
  mod generated, since the hooks are engine hooks but the geometry is not. What crosses
  the boundary per chunk is indices and never names: a chunk is 98304 entries and looking
  names up per chunk is the difference between a generator and a stall. The threading
  contract on `PierGenerateChunkFn` is the opposite of the rest of this file and is
  written out there rather than left to the default.

  A payload with no terrain section is what says the terrain is a mod's, so `md_add_dimension`
  refuses one -- taking it there would register a dimension nothing fills and report success --
  and a supplied dimension whose mod is gone loads as a void with its chunks intact rather than
  falling through to the pack branch, where reading the terrain as a pack would throw on a chunk
  thread.

- **`terrain.kind: layers` is served again, and needs no tool and no file.** A layer stack,
  optionally cut into a grid of cells and gaps, is what a plot world and a superflat world
  are, and it had been withdrawn in favour of terrain packs built offline with
  `tools/pier-pack`. A caller now sends the recipe inline, as before, and it is assembled
  into a template pack in memory and handed to the pack path: the chunk fill, the layout
  expansion and the confinement hook underneath are the ones the pack path uses and the
  ones the tests cover, so this adds a front door rather than a second generator.
  `pierpack/from_layers.py` performs the identical mapping offline and stays the reference.
  Every expression the assembled pack carries is a constant, because the inline recipe
  names concrete numbers, so nothing has to be bound and no parameter can fail to bind.
  A recipe whose border does not fit inside its cell, or whose stack runs past the top of
  the dimension, is refused with the numbers rather than quietly shrunk. `terrain.kind:
  noise` is still not served inline: it is a volume pack and not a layer stack, and the
  in-memory assembly does not reach it.

### Fixed

- **A setting with no implementation behind it is refused, not recorded.** `aquifers_enabled`
  and `ore_veins_enabled` each had a bit in the volume pack format and nothing anywhere that
  read it: the builder wrote the flag, the host loaded the pack, and the terrain came out
  without the aquifers or the ore veins its author had asked for, with no line on either
  side. The tool refuses the two keys now, the host refuses a pack whose settings carry
  either bit, and a bit outside the known set is refused as well, so a pack from a newer
  tool cannot be half-honoured. The bit values stay recorded so their meaning is fixed if
  either is ever implemented.
- **A source key the builder does not read is refused.** Neither pack builder validated its
  top-level keys, so a misspelled name, or a field copied from a Java datapack that this
  format has no equivalent for, was dropped without a word and the pack was built as though
  the author had not written it. Both builders now carry the list of keys they act on and
  refuse anything else.
- **The shipped terrain packs are built for the world bottom that exists.** `fixtures/town.json`,
  `fixtures/plot.json`, `pierpack/from_layers.py` and two of the tool's tests all declared
  a bottom of -512, the value `kWorldMinY` held before it was lowered to the vanilla -64;
  nothing updated them at the time. A pack made for -512..320 does not fit a dimension of
  -64..320, so mounting one was refused with `PIER_PACK_HEIGHT` and no world could be built
  from it. The declared range is -64..320 now, the empty bottom layer that ran from -512 to
  the bedrock is gone, and the two equivalence tests mount at the range the packs actually
  carry.
- **A binary the host cannot open names the path it tried.** `binary` in a pack config is
  joined onto the config's own directory, and a config written with a path relative to the
  server root instead produces a doubled prefix; the refusal said only that a file could
  not be opened, which sends the reader to check a path the host never tried.
- **A mistyped field in a spec is a refusal with a reason, not an exception.** The
  conversions on `CompoundTagVariant` are not uniform: a number reached through the wrong
  type throws `std::runtime_error`, a string does it through `std::get` and throws
  `std::bad_variant_access`, and `.get<CompoundTag>()` does the same for a section. Any of
  them escaped the reader, was caught by the API guard several frames away, and came back
  as a registration that failed naming neither the field, the dimension, nor the recipe.
  Every field is now type-checked before it is read, a wrong one reads as absent and is
  reported, and `md_add_dimension` names the dimension and the spec text if anything still
  throws.
- **The host's startup line carries a build stamp**, so a report can be told apart from one
  taken against the previous binary without asking.
- **`md_add_dimension` no longer throws on a layers terrain.** The slot refused a pack
  terrain and then read the spec as native with `std::get`, which throws for the third
  alternative rather than falling through. A layers recipe reached that line and the
  registration came back as a refusal with no reason attached, since the throw was caught
  by the API guard. Both served kinds are read with `get_if` now, and the biome a layers
  recipe names is verified at registration for the same reason the pack path verifies its
  own: `TemplateGenerator` reports a biome it cannot find as the registry having changed
  underneath it, which is the wrong thing to tell someone who mistyped a name.
- **This host owns the dimensions it registers.** The instance ledger held a `WeakRef`,
  and the engine does not keep an `OwnerPtr` handed to `DimensionRegistry::registerDimension`
  alive: the reference the call returned locked, the level listed the dimension, and the
  weak reference was expired again as soon as the frame that built it returned. Every path
  that reads the ledger treats an empty answer as permission to build, so the same
  dimension was built and registered twice within a millisecond, and the second
  registration replaced an object the engine had already been told about during
  `initializeDimension`. The next player to connect into it dereferenced a null and the
  server died before they had spawned. The ledger now holds an `OwnerPtr`, it is asked
  before the engine on every path that decides whether to build, and letting go is a
  handover to the quarantine rather than a destruction, because a player may be standing
  in the dimension at that moment. `native::forgetAllInstances` releases them while the
  level is still standing, rather than among the static destructors at process exit.
- **The by-name ledger answers to the qualified name too.** The engine calls by
  `native::engineNameOf`, the ledger is keyed by the name the caller chose, and the detour
  on the by-name overload therefore handed the engine's own call straight back to it.
- **A refused registration no longer takes the server down with it.** `initializeDimension`
  is where the engine wires a `Dimension` into the level, and the references it hands out
  there it does not own. `buildAndRegister` moved its only `OwnerPtr` into
  `registerDimension`, so a refused registration destroyed a dimension the engine had
  already been told about, and the level tick reached one of those references about a
  second later and aborted the process. The reference is now kept across the call, and a
  dimension the registry declines is held rather than released.
- **An empty return from `registerDimension` is no longer read as a refusal on its own.**
  Whether the dimension was stored is asked of the level, through `ILevel::forEachDimension`,
  and an empty reference beside a dimension the level is holding is reported as the return
  value being unreliable rather than as a failed registration.
- **A dimension is in the host ledger before anything builds it.** The detours on the
  resolution calls decide whether an id belongs to this host by asking the ledger, and the
  ledger was written only after registration returned. During its own registration a
  dimension was therefore invisible to them, and the call went to the engine, which built
  and registered it on a path this host does not see.
- **A player is no longer sent into a dimension their game has not been told about.** The
  set of dimensions reaches a client once, in the data sent while joining, and there is no
  way to send it again mid-session, so a dimension registered while a player is online is
  one that player cannot enter: the transfer runs on the server, the client has no
  definition for the id, and the player is left standing where they were with nothing said.
  `Level::requestPlayerChangeDimension` now refuses the transfer and tells the player to
  rejoin. A player whose session start this host did not see is allowed through.
- **The startup check on the vanilla dimensions asked the wrong question.** The nether and
  the end are built on first use, so a boot with nobody in them holds the overworld alone,
  and reporting that as two destroyed dimensions was wrong. Only a missing overworld is
  reported now.
- **A custom dimension no longer takes the overworld's slot in `DimensionRegistry`.** A
  `Dimension` carries two numbers: `mId`, a `DimensionType`, the signed int this host, the
  commands and the save all speak, and `mRegistryId`, a `DimensionIdType`, the unsigned
  short that keys `DimensionRegistry::mDimensions` and that `registerDimension` takes. The
  only thing mapping between them is `DimensionManager::mDimensionNameIdStore`, the table
  `serverRegisterCustomDimension` wrote until 26.20 and that nothing exported writes now,
  so a dimension built for a name that table has never held is constructed with a registry
  id that is not its id. `registerDimension` assigns into the map, so such a dimension is
  stored over the slot that number belongs to and the `OwnerPtr` that was there is
  dropped. The level goes on running against a destroyed dimension: the first custom
  dimension of a boot reports success, a second is refused because the slot is taken, and
  the server dies shortly after anyone joins, on a chunk worker or in the level tick,
  inside engine code that names neither this host nor the dimension that was added. The
  terrain the new dimension generates has no bearing on it. `native::claimRegistryKey`
  now sets the registry id of any dimension at or above `firstCustomDimensionId` to its
  own `DimensionType` before it enters the registry, and a detour on
  `DimensionRegistry::registerDimension` applies it to the engine's own creation paths as
  well. `buildAndRegister` reports the as-built registry id on the line it already writes,
  and refuses to register when the two still disagree. Chunks a dimension wrote under a
  registry id it no longer holds stay in the save and are not reachable from it.
- **Teleporting into a custom dimension lands in that dimension.** Every engine path that
  resolves a dimension goes through `getOrCreateDimension`, which maps an id to a name
  through `DimensionManager::mDimensionNameIdStore`. Nothing exported writes that table
  since `serverRegisterCustomDimension` was inlined away, so it comes back empty for every
  dimension this host registers. `TeleportCommand::computeTarget`, which is what
  `Actor::teleport` and the teleport commands run on, only attaches a
  `ChangeDimensionRequest` when it can resolve the destination, so without one the player
  is moved to the coordinates inside the dimension they were already standing in. The host
  had worked around the same gap for its own calls by building the instance into
  `DimensionRegistry` directly, where the plain `getDimension` lookup does find it, which
  is why block writes worked while teleports did not. Four detours now answer for this
  host's dimensions out of that registry, on both `getOrCreateDimension` overloads, on
  `Level::getOrCreateDimension` and on `Level::isDimensionTypeActive`; every other id goes
  to the original. Nothing is invented: an id is answered only when the host ledger holds
  its name and an instance is registered under it.
- **A custom dimension is no longer rebuilt over the one already running**, which crashed
  the server on the first teleport into it. On BDS 1.26.40 both `getOrCreateDimension`
  overloads come back empty for a custom dimension, so every `getOrCreateByName` fell
  through to `buildAndRegister`, and `DimensionRegistry::registerDimension` assigns into
  its map: the second registration under the same id destroyed the `Dimension` the level,
  the players and the chunk threads were holding. The log showed it plainly, with
  `built through the factory and registered as id 3` twice at startup and once more when
  a player was sent in, followed by an access violation on a chunk worker thread with no
  frame of the mod on its stack. `getOrCreateByName` now asks
  `DimensionManager::getDimension` first and returns the live instance, `buildAndRegister`
  refuses an id that already carries one, and a registry that cannot be read refuses to
  build rather than guessing. A correct startup now logs the build line once.
- **The registry table is read directly; `getDimension` is not the judge.** The fix above
  asked `DimensionManager::getDimension` whether an id carries an instance. On a live
  server it answered empty for a dimension that was registered and running, so every
  teleport still went through `buildAndRegister`, and `registerDimension` then returned an
  empty reference twice and a full one the third time for the same id and the same code.
  The lookup resolves a `DimensionType` through the engine name table, which has no row
  for a host-registered dimension, so it answers empty for exactly these ids while the
  table it writes into holds them. `registeredInstance` now walks
  `DimensionRegistry::mDimensions` when the lookup comes back empty, which needs only a
  `std::hash` for `DimensionIdType` that the SDK does not supply for the derived type;
  the table, not the returned reference, decides whether a registration took, the built
  instance is held by a second owner until that is known so it is never destroyed under
  the chunk work `initializeDimension` started, and a registration the engine stored
  nowhere is placed in the table directly. The three `getOrCreateDimension` detours hand
  the engine `native::instanceRef`, a reference out of the same table, instead of calling
  `getDimension` after a build and returning the empty answer that told the teleport there
  is no such dimension.
- **A custom dimension is allocated an id from 1000, not from 3.** Everything above was
  necessary and none of it was sufficient: with the instance built, placed in the registry
  and handed back by the detours, a teleport still left the player standing where they
  were. The number was the problem. `VanillaDimensions::Undefined()` sits just past the
  vanilla three, and the engine compares against it to mean no dimension, so a dimension
  holding that number is read as absent by every path that makes the comparison:
  `DimensionRegistry::registerDimension` stored nothing for it, `getDimension` came back
  empty, and `TeleportCommand::computeTarget` dropped the `ChangeDimensionRequest`. Writing
  blocks through a `BlockSource` taken straight out of the registry never makes that
  comparison, which is why the id looked usable for everything except entering it. Ids are
  now allocated from `native::firstCustomDimensionId`, 1000, which is where the engine's own
  script-api registration allocates from and where the ids in a save written by
  MoreDimensions 0.14 come from. A `dimension_config.json` entry below that floor keeps its
  name and payload and is reallocated on the next registration, with a line saying that
  anything built under the old number stays in the save and will not appear in the new
  dimension.
- **The engine registers its own dimensions again, and this host no longer puts one into
  the registry by hand.** Entering a dimension registered that way fastfailed the server on
  a chunk worker thread with `FAST_FAIL_GUARD_ICALL_CHECK_FAILURE`, an indirect call
  through a pointer that was never set: `DimensionRegistry::registerDimension` is the
  engine's own entry point and does more than store a pointer, so a `Dimension` placed
  beside it is one the engine never finished wiring. A registration the engine declines is
  now reported as a failed registration and the dimension is unavailable, which is a
  server that says so instead of one that crashes minutes later. For the same reason the
  three `getOrCreateDimension` detours call `origin` first and build only when it comes
  back with nothing: the evidence that the engine could not create these dimensions was
  gathered while they were numbered 3, where every engine path refuses on principle.
- **`getDimension` answers for a dimension this host registered.** It is a plain lookup and
  every part of the engine uses it, and it resolves through the engine name table, which has
  no row for a dimension registered from outside: it answered empty for one that was
  registered and running, so whatever asked walked into nothing. That is the shape of every
  crash in this series, on the tick and on the chunk workers alike, and the thing that
  proves it is a void dimension crashing exactly like one with terrain, at the same
  instruction, with no generator of the engine's involved at all. `DimensionManager::
  getDimension` and `Level::getDimension` now fall back to the host's ledger after the
  engine's own answer, the same order the `getOrCreateDimension` detours use, reading the
  ledger directly so a hook cannot call back into itself.
- **The engine's own terrain generators are used again, which is what an overworld, nether
  or end template asks for.** They had been replaced with a void carrying the same sky,
  because entering such a dimension took the server down every time it was tried. That
  reason was wrong: the fault was this host destroying a `Dimension` the engine was still
  using, a void dimension died at the same instruction with no generator involved, and it
  is fixed. What the fallback cost meanwhile was the whole point of those three templates,
  which arrived as empty voids. `engine_terrain: 0b` in a dimension's terrain turns the
  engine generator off again, for an operator who meets a fault in engine code this host
  cannot repair and still needs the dimension to open. A dimension that generated void
  chunks under the old default keeps them; only ground it has not reached yet comes out as
  terrain, so a test world is worth recreating.
- **The end and nether generators are handed their own biome.** Both were given the level's
  biome override, which is what vanilla passes and which on a normal level is empty, so the
  lookup answered with nothing and the generator kept it. `OverworldGeneratorMultinoise`
  builds its own biome source and does not care; these two are each written for one biome.
  A server generating end terrain read a pointer of all ones inside the column loop, at the
  y the registers put at the top of the dimension, which is what a biome that was never
  there looks like once something walks it. The vanilla name is used when the level names
  nothing, and when neither is in the registry the dimension is generated as a void with a
  line saying so, rather than handing the generator nothing.
- **A dimension keeps the id it already holds.** The suggestion stepped over every id in
  the caller's ledger, including the dimension's own: one recorded as 1000 was handed 1001,
  the record was corrected to 1001, and the next boot handed it 1000 again. It changed id
  on every start, and the chunks it had written stayed behind under the number it no longer
  had. The id from the record is now preferred when nothing else holds it.
- **A dimension whose terrain is an engine generator gets that generator's own height.**
  These generators are not parameterised by the dimension they are handed: `TheEndGenerator`
  carries a fixed 16x16x128 block buffer and each of them indexes columns from the bottom
  of the dimension it was written for. A custom end dimension running from -64 instead of 0
  had it write past what it allocated, and the server died on the tick after a player
  entered, with the end sky already drawn and no blocks in it. `spec::dimensionHeightOf`
  now answers with the generator's range for a native terrain, so the `Dimension`, the
  definition and the height check all get the same number, and says so in the log when it
  differs from what the spec asked for. A spec's own height still governs terrain that
  comes from this host.
- **A generator is told about structure sets only when its structure features went in.**
  `ChunkGeneratorStructureState::createNormal` fills `mPossibleStructures` from the level's
  structure sets, and placing one means looking the matching feature up in the generator's
  own `StructureFeatureRegistry`. That registry is filled by three engine functions this
  host reaches by symbol, and on 26.40 they are not there: the generator was told about
  every structure set in the game while its feature registry stayed empty, and the first
  tick after a player entered called through a pointer nothing had set. With no features
  the state is now `createFlat` with an empty list, which says there are no structures to
  place, and the log says the dimension generates without structures.
- **The factory closure builds the dimension with the id the engine's definition holds,
  not the number in `dimension_config.json`.** With the closure finally being called, its
  first build of a boot took `shared->id` from the record, which is the previous boot's
  answer; the engine had allocated a different number, so the object said 1001 while it
  was registered under 1000, the read-back reported 1001, the record was "corrected" to
  1001, and the first player to ask had a second instance built and registered under 1001.
  The level tick died with two instances for one name. The closure now asks the definition
  group first, which is what the registration is happening against, and `buildAndRegister`
  refuses to register an object under a key that is not its own id.
- **The host's factory closure is bound after the engine registers its own, so the
  dimension the engine builds is the host's class.** Every build so far has logged "built
  through the factory" and never the line the closure itself prints when it runs, which
  means `DimensionFactory::create` was not finding it: `_registerCustomDimensionWithFactory`,
  called after the closure was put in the map, registers the engine's own factory for the
  name over whatever is there, and what that built for the definition was a vanilla-class
  dimension with the definition's generator, never `SpecDimension`. That dimension was
  registered under the host's id, handed to a player, and fastfailed a chunk worker a third
  of a second later. The closure is now bound again right after that call, and the "is
  building" line in the log is what proves the host's class is the one being built. On
  26.20 `serverRegisterCustomDimension` used the closure already in the map, which is why
  the previous host never had to do this.
- **The default bottom of a custom dimension is the vanilla bottom, -64, not -512.** The
  -512 was a workaround: a client with no definition of its own to go by fell back to the
  largest possible world and requested subchunks -32..-24, which a server validating
  against -64 refused. With the definition now carrying the height that fallback has
  nothing left to do, and the workaround had a bill of its own: a dimension 52 subchunks
  tall is outside anything the engine ships, and across every crash on entering one the
  height was the single thing about the dimension that never varied. A spec may still ask
  for more; nothing forbids it and nothing vouches for it.
- **The engine's dimension definition is given the height and generator the dimension is
  actually built with.** `_registerCustomDimensionWithDimensionDefinitionGroup` takes a name
  and an id and nothing else, so the definition carried the engine's defaults while the
  `Dimension` was constructed from the stored spec, and the two are read by different parts
  of the engine. A server that entered such a dimension died on a chunk worker a third of a
  second later, every time, on an indirect call through a value that is not a function,
  which is what a shape written into an allocation sized for a different shape looks like
  once something walks it. The definition entry is now reached through the engine's own
  iterator, checked against the id just registered so the fields are known to be where the
  header says, written, and read back; both the engine's original values and the spec's are
  logged either way.
- **A custom dimension no longer pretends to be the overworld to upgrade a chunk.**
  `SpecDimension` implemented `levelChunkNeedsUpgrade`, `upgradeLevelChunk`, `fixWallChunk`
  and `_upgradeOldLimboEntity` by `reinterpret_cast`-ing itself to `OverworldDimension` and
  calling that class's exported thunks, on the reasoning that `OverworldDimension` declares
  no data member and the two layouts therefore coincide. With the registration finally
  working end to end, a server fastfailed on a chunk worker inside those bodies a third of
  a second after a player entered the dimension, on `FAST_FAIL_GUARD_ICALL_CHECK_FAILURE`:
  an indirect call through a value that is not a function. Whatever those bodies read at
  that offset, it is not what this object holds there. The four now do nothing, which is
  also the right answer on its own terms: these upgrade a chunk written by an older BDS,
  and a dimension of this kind is created by the version that is running, so every chunk in
  it was written by that version. The `WorldGenerator` that adds vanilla structure features
  through resolved symbols is untouched, but each of its three paths now logs a line, so
  the next report says whether it ran.
- **The dimension factory hands back the instance in use instead of building a second
  one.** The closure this host puts in `DimensionFactory::mFactoryMap` is called by the
  engine too, on paths the host never sees and without a line in the log, and it built a
  new `Dimension` every time; whatever the engine then did with it ended in an assignment
  over the object the level, the players and the chunk threads were holding. It now returns
  the live instance when there is one, which makes that assignment a no-op, and it logs
  every call, so the log shows who is asking for a dimension to be built.
- **A dimension this host registered is built once, because the host remembers it.** This
  was the fault behind every crash in this series. `DimensionManager::getDimension` resolves
  a `DimensionType` through the engine name table, and nothing exported writes a row there
  for a dimension registered from outside, so it answers empty for one that is registered
  and running. With no memory of its own, the host read that as "not registered" on every
  resolution and built another, and `DimensionRegistry::registerDimension` assigns into its
  map: each new instance replaced the one the level, the players and the chunk threads were
  holding, and a chunk worker fastfailed on a call through a pointer inside the destroyed
  object. A live server built one dimension six times in three seconds, then began logging a
  refusal thirty times a second until it died. The instance `registerDimension` returns is
  now kept in a host-side ledger of `WeakRef<Dimension>` by id, consulted right after the
  engine's own lookup and dropped when the dimension is retired or the engine lets it go.
- **The registry table is no longer read for an answer, only reported.** It is reached
  through a header-declared offset into a private field, and the first server to print it
  read two entries with ids 59024 and 0, which the engine's dimension table cannot contain:
  that offset does not describe the running binary, so a `Dimension*` taken out of it is a
  pointer into whatever the memory really is. Every decision is back on
  `DimensionManager::getDimension`, on what `registerDimension` hands back and on the host's
  own ledger, and the field is not touched at all any more.
- **A dimension is registered with the engine under `namespace:name`.** The engine's own
  registration path takes a qualified name, and MoreDimensions, the only implementation
  known to work on this generation, rejects anything else before it reaches the engine. A
  bare name is now qualified with `pier:` at the boundary that talks to the engine, which
  is also the key the factory map uses because `DimensionFactory::create` is called with
  the name the engine knows. The ledger, `dimension_config.json` and every mod-facing call
  keep the name the caller chose. A dimension already registered under a bare name is a new
  dimension under the qualified one and gets a new id.
- **An id already spoken for in `dimension_config.json` is never suggested to the engine.**
  The suggestion walked the definition group, which holds only what the current boot has
  registered so far, so a dimension named in the config but not yet registered was
  invisible and its number could be handed to another dimension. A collision already in the
  config now names both sides and says that the chunks of the loser are not reachable.

### Added

- **`service_caller`: a provider can ask who is calling it.** A `PierServiceCb` receives a
  request and nothing about its sender, so a provider keying anything on a name inside
  the request was trusting the request. The new slot, appended and struct_size-gated
  (the ABI stays at v2), sinks the manifest name of the mod whose `service_call` is on
  the stack; nested calls report the innermost one, and outside a callback or for a call
  made without a handle the sink is not called. `levilamina::service::caller()` is the
  Rust side. The first consumer is the RSW permission manager, which binds a wire owner
  to the mod that first announced it and refuses the name from any other mod.

## [26.40.0] - 2026-09-08 — Pre-release

Built for BDS 1.26.40 and LeviLamina 26.40.0. A release is built against the one BDS its
number names, so 26.32.2 stays the build for BDS 1.26.32 and does not run on 1.26.40.
The engine changes are small and none of them reaches the ABI: `PIER_ABI_VERSION` stays
at 2 and a mod built against 26.32.2 keeps working without a rebuild.

### Changed

- **Custom dimension registration carries a pack id.**
  `DimensionManager::_registerCustomDimensionWithDimensionDefinitionGroup` takes a third
  argument in 26.40, the resource pack a definition came from, and
  `DimensionDefinitionGroup::DimensionDefinition` gained the matching `mPackId`. A Pier
  dimension comes from no pack and registers with a zero UUID.
- **The scoreboard packet became a payload packet.** `SetScorePacket` derives from
  `ll::PayloadPacket<SetScorePacketPayload>`, its `mType` is gone, and a row is one
  alternative of `std::variant<RemoveScore, ChangePlayerScore, ChangeEntityScore,
  ChangeFakePlayerScore>` instead of a `ScorePacketInfo` with an identity type field. The
  sidebar builds `ChangeFakePlayerScore` rows, which is what the identity field said.
- **`Player::startSleepInBed` takes `setsRespawn` and `sleepOffset`**, and the sleep hook
  passes both through unchanged.
- **`DedicatedServer::runDedicatedServerLoop` returns `::ServerExitCode`** rather than a
  nested one and takes an `EditorAllowList`; the client-side-generation hook follows.
- **`DimensionDataPacket`, `LevelChunkPacket` and `SubChunkPacket` are payload packets**,
  so their fields are inherited and `$write` has two overloads. The chunk trace names the
  one-argument overload explicitly. `LevelChunkPacket` no longer carries
  `mClientNeedsToRequestSubchunks`: a set `mClientRequestSubChunkLimit` is the request,
  and the trace reports it that way.
- **`NetworkChunkPublisher::moveRegion` is gone**, and every other entry point of that
  class carrying a region sits behind `LL_PLAT_C`, so none of them is declared on a
  server build. The send-region trace reads `mLastChunkUpdatePosition` and
  `mLastChunkUpdateRadius` from inside the queued-chunk hook instead, and reports the
  region when it changes rather than on every call.
- **`MobEffectInstance` has no constructor of its own.** Adding an effect builds the
  instance empty and assigns the id, duration, amplifier and visibility, which is what
  the removed two-argument constructor did.

### Removed

- **`PIER_BPROP_IS_DOOR` and `PIER_BPROP_IS_STAIR` are unsupported.**
  `BlockType::isDoorBlock` and `isStairBlock` are gone in 26.40 and nothing replaces
  them; a block tag would be a guess at the name the engine files carry.
- **`PIER_APROP_IS_IN_LOVE` is unsupported.** `Actor::isInLove` is gone in 26.40 and
  nothing replaces it. The `mInLovePartner` field survives, and reading it would be a
  guess at what the accessor tested, so the property is reported as unsupported rather
  than answered with a value that may be wrong.

## [26.32.2] - 2026-09-08 — Pre-release

Terrain packs replace the declarative `layers` and `noise` terrains, and the ABI moves
to v2. Nothing in this section had shipped in a tagged release, which is why the
removals below are removals and not deprecations.

### Changed

- **ABI v2.** `PIER_ABI_VERSION` and `PIER_ABI_MIN_SUPPORTED` are both 2. The four slots
  retired in 26.20.3, `md_add_simple_dimension`, `md_add_plot_dimension`,
  `md_set_plot_grid` and `md_clear_plot_grid`, are deleted from `PierApi`; every slot
  after them moves, so a mod built against v1 is refused at load with the version range
  in the message. `tools/abi-v1.slots` was re-blessed for v2.
- **Three terrain kinds.** `md_add_dimension` serves `terrain:{kind:"native"}` only, now
  with `generator:"flat"` and `"void"` beside the three vanilla worlds and a `biome` for
  the void. A `layers` or `noise` terrain is refused with a pointer to the pack tool;
  `pier-pack from-layers` converts an old layers spec into a template source.
- **Dimension spec.** The stored form of a pack terrain is
  `terrain:{kind, pack, sha256, params, roles}`: the config path, the binary's hash at
  registration, every bound parameter and the role overrides, so the terrain is
  regenerable from the save alone and a changed binary is refused rather than mixed in.

### Added

- **`md_add_dimension_pack(name, config_path, spec_snbt)`.** Registers a dimension from a
  terrain pack: a directory with a config file and a binary. The host reads four keys of
  the config (`pier_terrain`, `type`, `binary`, `sha256`), compares the spec's terrain
  kind with the config's type and the binary's magic, hashes the binary against the
  config, binds the parameters by kind (free, fixed, choice, derived), checks the pack's
  constraints, and only then stores the spec. Refusals return one of the `PIER_PACK_*`
  codes, all negative; a dimension id is never negative.
- **`md_pack_inspect(config_path, ctx, sink)`.** What a pack asks for as one JSON
  document, the same shape `pier-pack inspect` prints, so a mod builds its form from the
  pack's own parameter list without a parser of the binary.
- **Template packs (PIERTPL).** A fixed range with parameters and cells whose data is
  the same: zones along each axis with expression lengths, a block stack per combined
  zone, constraints, and 3D structures as a CSG graph (box, cylinder, wedge, translate,
  rotate, mirror, repeat, union, difference, intersect, shell, paint, choose by parameter
  or cell hash, voxel blobs with keep and air cells) picked per cell with weights and
  turns. A CONF section registers the cell grid for the confinement rules from the same
  mount that produced the terrain. The plot world is the acceptance fixture and is
  compared cell by cell with a line-for-line port of the 26.20.2 `PlotGenerator`.
- **Volume packs (PIERVOL).** Java's density function graph: 31 operators including
  splines, shifted noise, the caches, interpolation, end islands, the 1.18 blended
  noise and the weird scaled sampler; the multi-noise biome parameter list; and the
  surface rule subset (block, sequence, condition, biome, noise_threshold,
  vertical_gradient, y_above, water, temperature, steep, not, hole,
  above_preliminary_surface, stone_depth). A pack stores noise parameters, never tables:
  the host builds the tables from the world seed with Java's Xoroshiro128++ or legacy
  random source, so one pack serves any seed. `pier-pack from-datapack` assembles a
  source from a Java datapack directory.
- **`tools/pier-pack`.** `build`, `inspect`, `hash`, `from-layers`, `from-datapack`, a
  Python reference generator for each pack kind, and tests that compile the engine-free
  pack layer under g++ and compare it with the reference cell by cell.
- **Rust SDK.** `dimensions::add_dimension_pack`, `dimensions::pack_inspect`, `PackStatus`
  and `PackError`; `GeneratorType::spec_name` for the spelling a spec uses. The crate is
  `2.0.0`, since the four removed functions are a breaking change.

- **`tools/migrate_dimension_config.py` no longer writes a `layers` terrain.** A void or a
  vanilla generator migrates in place as before; a pre-26.20.3 plot entry is left alone and
  the three steps to rebuild it as a template pack are printed, since the terrain of a plot
  world is a binary the host verifies by hash and cannot be written into the config file.

### Removed

- `spec::Layers`, `spec::Noise`, `LayersGenerator` and `NoiseGenerator`, superseded by
  the two pack kinds; the Rust SDK's `add_simple`, `add_plot`, `set_plot_grid`,
  `clear_plot_grid` and `PlotLayout`, superseded by `add_dimension_pack` and
  `pack_inspect`.

### Known limits

- The liquid layer of a voxel blob is decoded but not yet written into the chunk's
  second block layer, so waterlogged blocks in a blob come out dry.
- A volume pack has no aquifers, ore veins or bandlands, and the biome of a column is the
  one at its surface rather than a 3D field; `temperature` uses the biome's base
  temperature without the vanilla noise term; `sea_level` of the pack is used for the
  fluid fill but the dimension's own sea level stays at the sky's default.

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

- **Every command a player typed took the server down.** The `verb` field added to the
  ExecutingCommandEvent payload in 26.32.0 was read through
  `CommandRegistry::getCommandName`, which is MCAPI: LeviLamina resolves its address out
  of the BDS binary at the call, and on 1.26.32 that address is not there. The raise goes
  through `ll::memory::throwMemoryException` and ends in a CRT fastfail (0xC0000409,
  INVALID_ARG) rather than a C++ exception, so the `catch (...)` written around the call
  caught nothing and BDS left with no log from either side. The verb is now resolved by
  reading `mAliases` and `mSignatures`, which are plain public members and need no symbol.

  Two things about the shape of this. The failure needed a real player to type a real
  command, so nothing in the build, the checks or `tools/pier-probe` could reach it — the
  probe walks the table at enable time and this path only runs on a live command. And a
  `catch (...)` is not a guard against a missing symbol; that is the second time in this
  release cycle that an uncatchable failure hid behind one, after the command-registry
  guard below.

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

  *As shipped in 26.32.0 this crashed the server on every command typed by a player; see
  26.32.1.*

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
