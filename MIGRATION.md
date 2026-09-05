# The migration ledger: from the old repository to the Pier v2 rewrite

This table is the criterion for capabilities only ever increasing over the old version. It
is counted row by row before delivery, and every row is either done, meaning the new
repository has a matching implementation, or cut, saying why it was removed by design and
where it went.
A row marked outstanding is one this round has not reached: its capability is absent from
the new repository right now.

| Old file | New location | Status |
|---|---|---|
| `CONTRACT.md` | `CONTRACT.md` | ✔ done, rewritten for v2: the real dependency graph, the SPI mechanism, and empty-slot capability semantics |
| `—` | `COMMENTS.md` | ✔ done, added: an expansion of contract §7, with the three comment budgets and the list of what must not be written, and the `comment-style` check guarding the mechanical part |
| `LICENSE` | `LICENSE` | ✔ done, kept unchanged: all three Cargo.toml files declare Apache-2.0 while the file itself never came across, and the ledger was missing this row too, which the `ledger-covers-tree` check caught |
| `Cargo.toml` | `Cargo.toml` | ✔ done, rewritten to state that the two build lines share only a contract dependency and no build dependency |
| `README.md` | `README.md` | ✔ rewritten for Pier: why it exists, how it is designed, and where the first official binding lives. The progress section the old one carried is gone; MIGRATION.md is where status belongs |
| `docs/CHANGELOG.md` | `—` | ✂ cut, not migrated: the version history of v1 is archived with the old repository and v2 starts its era again at ABI v1 |
| `docs/DESIGN.md` | `—` | ✂ cut, not migrated: the design narrative of v1 was absorbed by CONTRACT v2 and the abi.h file header, and the history is archived in the old repository |
| `docs/README.zh.md` | `docs/README.zh.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/RELOAD.md` | `docs/RELOAD.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/advanced/PORTING_NOTES.md` | `docs/advanced/PORTING_NOTES.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/advanced/abi.md` | `docs/advanced/abi.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/advanced/architecture.md` | `docs/advanced/architecture.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/advanced/decisions.md` | `docs/advanced/decisions.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/advanced/extending.md` | `docs/advanced/extending.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/advanced/memory-safety.md` | `docs/advanced/memory-safety.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/actor/command.md` | `docs/api/actor/command.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/actor/entity.md` | `docs/api/actor/entity.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/actor/gui.md` | `docs/api/actor/gui.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/actor/money.md` | `docs/api/actor/money.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/actor/player.md` | `docs/api/actor/player.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/actor/sim.md` | `docs/api/actor/sim.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/core/bus.md` | `docs/api/core/bus.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/core/lane.md` | `docs/api/core/lane.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/core/objects.md` | `docs/api/core/objects.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/core/overview.md` | `docs/api/core/overview.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/core/service.md` | `docs/api/core/service.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/core/system.md` | `docs/api/core/system.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/infra/data.md` | `docs/api/infra/data.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/infra/event.md` | `docs/api/infra/event.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/infra/packet.md` | `docs/api/infra/packet.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/infra/scoreboard.md` | `docs/api/infra/scoreboard.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/infra/server.md` | `docs/api/infra/server.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/rt/client.md` | `docs/api/rt/client.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/rt/log.md` | `docs/api/rt/log.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/rt/scheduler.md` | `docs/api/rt/scheduler.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/world/block.md` | `docs/api/world/block.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/world/container.md` | `docs/api/world/container.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/world/dimensions.md` | `docs/api/world/dimensions.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/world/item.md` | `docs/api/world/item.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/world/nbt.md` | `docs/api/world/nbt.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/api/world/world.md` | `docs/api/world/world.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/guide/commands.md` | `docs/guide/commands.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/guide/concepts.md` | `docs/guide/concepts.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/guide/events.md` | `docs/guide/events.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/guide/getting-started.md` | `docs/guide/getting-started.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/guide/logging-scheduling.md` | `docs/guide/logging-scheduling.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/guide/world.md` | `docs/guide/world.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/index.md` | `docs/index.md` | ⬜ outstanding, to be migrated by rewriting file by file |
| `docs/package.json` | `docs/package.json` | ⬜ outstanding, to be migrated by rewriting file by file |
| `examples/region-scan/Cargo.toml` | `examples/hello-pier/Cargo.toml` | ✔ done, rewritten as hello-pier |
| `examples/region-scan/manifest.json` | `examples/hello-pier/manifest.json` | ✔ done, rewritten: the type and the dependency name became "pier"; the mod the old version depended on no longer existed and the example could not load |
| `examples/region-scan/src/lib.rs` | `examples/hello-pier/src/lib.rs` | ⬜ outstanding, region-scan itself is still to be migrated: it depends on the types, command and player domain modules, which were not written yet, so hello-pier verifies the four steps of contract §10 first |
| `packages/pier-abi/include/sdk/abi.h` | `packages/pier-abi/include/sdk/abi.h` | ✔ done, recut: made C11, one layout, a four-field header, 190 slots in identical order cell for cell, verified under both compilers |
| `packages/pier-abi/xmake.lua` | `packages/pier-abi/xmake.lua` | ✔ done, rewritten |
| `packages/pier-api/include/pier/api/common.h` | `pier/api/bridge.h + pier/support/*` | ✔ done, split and rewritten: the guards, strings and SNBT went to support and the resolution helpers to bridge.h |
| `packages/pier-api/include/pier/api/internal_api.h` | `packages/pier-host/include/pier/host/spi.h` | ✂ cut, retired: it was the declaration face of the forwarding table above and disappeared with it, and cross-package collaboration moved to the six registration faces of the SPI |
| `packages/pier-api/include/pier/api/money_guard.h` | `same path` | ✔ done, rewritten with the design note on the double lookup kept in full |
| `packages/pier-api/src/actors/Actors.cpp` | `same path` | ✔ done, rewritten: every property and action branch, with the comments explaining unsupported items kept |
| `packages/pier-api/src/actors/Forms.cpp` | `same path` | ✔ done, rewritten: Teardown 60, with both the index and the text of a selection control gathered, slider clamping, and PIER_TRACE_FORM all kept |
| `packages/pier-api/src/actors/Money.cpp` | `same path` | ✔ done, rewritten: Teardown 100, with the colliding LLMoneyEvent renamed to PierMoneyEvent and PierMoneyCb and the trampoline doing the PierStr conversion |
| `packages/pier-api/src/actors/MoneyGuard.cpp` | `same path` | ✔ done, rewritten |
| `packages/pier-api/src/actors/Players.cpp` | `same path` | ✔ done, rewritten: the four ability-bit traps, permission restoration, and the comments on sidebar slot hashing and clearing order all kept; the md validation moved to the blockSourceOf gate of the dimension bridge |
| `packages/pier-api/src/actors/SimPlayer.cpp` | `same path` | ✔ done, rewritten |
| `packages/pier-api/src/core/ApiTable.cpp` | `packages/pier-host/src/{ApiTable,Spi}.cpp` | ✂ cut, retired: the old version was a hardcoded forwarding table calling into each package directly, while the new architecture has each capability package fill its own slots through spi::SlotPack (contract §1 rule 2). Filling the four header scalars moved to pier-host/ApiTable.cpp |
| `packages/pier-api/src/core/Bus.cpp` | `same path` | ✔ done, rewritten: Teardown 20, with dispatching outside the lock and the depth-based cycle guard kept |
| `packages/pier-api/src/core/Common.cpp` | `core/{Bridge,Enrich}.cpp + support/snbt + the spi dimension bridge` | ✔ done, split and rewritten: the md coupling moved to spi::DimensionBridge, enrich became its own TU, and addressOwnedBy was still to move to support |
| `packages/pier-api/src/core/LogScheduler.cpp` | `core/{Log,Scheduler,GameStats}.cpp` | ✔ done, split into three and rewritten, with Scheduler registering Teardown stage 10 |
| `packages/pier-api/src/core/Services.cpp` | `same path` | ✔ done, rewritten: Teardown 30, with the hard-won comment on not checking isEnabled kept |
| `packages/pier-api/src/net/Client.cpp` | `packages/pier-client/src/Client.cpp` | ✔ done, moved into the separate pier-client package: Teardown 110, with the comments on the alive flag and the double-deregistration guard kept; KeyRegistry ownership now reports the host NativeMod while this package still cleans up per hosted mod |
| `packages/pier-api/src/net/ClientStubs.cpp` | `—` | ✂ cut, retired whole: a client stub was needed only because the old ApiTable referenced every symbol statically. In the new architecture a server-only TU is not compiled into the client, so the slot is NULL and the SDK reports the capability as unsupported under the struct_size and empty-slot discipline, which behaves the same as the old stub returning false, -1 or 0. The -1 and 0 failure conventions are noted at each real implementation |
| `packages/pier-api/src/net/PacketHooks.cpp` | `same path` | ✔ done, rewritten: server-guarded, with the two detours, the varint codec, snapshot dispatch, the three atomic gates, the double logging and the reason for never unhooking all kept; Teardown 90 |
| `packages/pier-api/src/net/Packets.cpp` | `same path` | ✔ done, rewritten: server-guarded, with ensureReadCompleted refusing trailing garbage, the Times packet sent first on its own, the three durations being all or none, and the single-byte dimension truncation warning all kept |
| `packages/pier-api/src/net/ScoreboardApi.cpp` | `same path` | ✔ done, rewritten: server-guarded, with the reason for making SET_DISPLAY native kept |
| `packages/pier-api/src/runtime/Commands.cpp` | `same path` | ✔ done, rewritten: the silent catch in update_soft_enum now logs, and the lying comment on the default branch was corrected |
| `packages/pier-api/src/runtime/Events.cpp` | `runtime/{Events,CommandEvents}.cpp` | ✔ done, rewritten: the provider routing of contract §6 plus the shadowing warning, with the command events split out as a covers_registry provider |
| `packages/pier-api/src/runtime/Extras.cpp` | `same path` | ✔ done, rewritten |
| `packages/pier-api/src/runtime/Server.cpp` | `same path` | ✔ done, rewritten: the whole file leaves NULL slots in a client build |
| `packages/pier-api/src/runtime/SysInfo.cpp` | `same path` | ✔ done, rewritten |
| `packages/pier-api/src/runtime/data/KvDbApi.cpp` | `same path` | ✔ done, rewritten: Teardown 70, with the path confinement kept |
| `packages/pier-api/src/runtime/data/NbtApi.cpp` | `same path` | ✔ done, rewritten |
| `packages/pier-api/src/world/Containers.cpp` | `same path` | ✔ done, rewritten |
| `packages/pier-api/src/world/Edit.cpp` | `world/{Edit,BlockResolve}.cpp` | ✔ done, rewritten: server-guarded, with the three helpers moved out |
| `packages/pier-api/src/world/GapFill.cpp` | `same path` | ✔ done, rewritten: server-guarded, with the retired slot returning -1, the zero-buffer chunk_keys and the root cause of the cross-DLL crash, the radius shrunk by one, and the conn_id consistency contract all kept; the PierStr comments were corrected to the new {ptr,len} meaning |
| `packages/pier-api/src/world/Items.cpp` | `same path` | ✔ done, rewritten with the ADD_ENCHANT stub kept as it was and noted |
| `packages/pier-api/src/world/World.cpp` | `world/{World,BlockResolve}.cpp` | ✔ done, rewritten: the block resolution helpers were extracted into the dual-target BlockResolve, fixing the broken link of the old client target |
| `packages/pier-api/src/world/WorldInfo.cpp` | `same path` | ✔ done, rewritten: server-guarded, with the boundary comments on not forcing a load and not counting villagers kept |
| `packages/pier-api/xmake.lua` | `packages/pier-api/xmake.lua` | ✔ done, rewritten with the includes made private |
| `packages/pier-dimensions/include/pier/dimensions/base/Macros.h` | `—` | ✂ cut, retired: that export macro existed for a dll export, while in the new architecture this package is an object package compiled into the host that exports no symbol, since capabilities fill the table through SlotPack, so the macro has nowhere to apply |
| `packages/pier-dimensions/include/pier/dimensions/base/NativeDimensions.h` | `base/native_dimensions.h` | ✔ done, rewritten with the design note on the native path and the purpose of the ledger both kept |
| `packages/pier-dimensions/include/pier/dimensions/base/SimpleCustomDimension.h` | `base/simple_custom_dimension.h` | ✔ done, rewritten with the dllexport macro removed and the reason noted |
| `packages/pier-dimensions/include/pier/dimensions/base/Utils.h` | `base/utils.h` | ✔ done, rewritten with the diagnostics for the two disagreeing height values kept |
| `packages/pier-dimensions/include/pier/dimensions/dim/ChunkTrace.h` | `packages/pier-dimensions/include/pier/dimensions/dim/chunk_trace.h` | ✔ done, rewritten: the tracing switches gathered in one place and shared by PlotGenerator, with the environment variable prefix changed to PIER_* |
| `packages/pier-dimensions/include/pier/dimensions/dim/CompleteBaseTypes.h` | `dim/complete_base_types.h` | ✔ done, rewritten with a note added that every derived-dimension .cpp includes it first |
| `packages/pier-dimensions/include/pier/dimensions/dim/CustomDimensionConfig.h` | `dim/custom_dimension_config.h` | ✔ done, rewritten with a note added on why it follows the save rather than living under configs/ |
| `packages/pier-dimensions/include/pier/dimensions/dim/CustomDimensionManager.h` | `packages/pier-dimensions/include/pier/dimensions/dim/custom_dimension_manager.h` | ✔ done, rewritten with dllexport removed; the id-by-name function was cut, because the fromString it forwarded to reads back garbage for a custom dimension |
| `packages/pier-dimensions/include/pier/dimensions/dim/DimensionHeight.h` | `dim/dimension_height.h` | ✔ done, rewritten with the table of three measurements behind the -512 bottom and the fallback plan kept |
| `packages/pier-dimensions/include/pier/dimensions/dim/DimensionRules.h` | `packages/pier-dimensions/include/pier/dimensions/dim/dimension_rules.h` | ✔ done, rewritten with 13 static_asserts pinning the numbering to PierDimRule |
| `packages/pier-dimensions/include/pier/dimensions/plot/PlotConfine.h` | `packages/pier-dimensions/include/pier/dimensions/plot/plot_confine.h` | ✔ done, rewritten |
| `packages/pier-dimensions/include/pier/dimensions/plot/PlotDimension.h` | `packages/pier-dimensions/include/pier/dimensions/plot/plot_dimension.h` | ✔ done, rewritten with dllexport removed |
| `packages/pier-dimensions/include/pier/dimensions/plot/PlotGenerator.h` | `packages/pier-dimensions/include/pier/dimensions/plot/plot_generator.h` | ✔ done, rewritten with dllexport removed |
| `packages/pier-dimensions/include/pier/dimensions/plot/PlotLayout.h` | `packages/pier-dimensions/include/pier/dimensions/plot/plot_layout.h` | ✔ done, rewritten: the self-reference in kBedrockY was fixed and the wording cleared of language names |
| `packages/pier-dimensions/src/dim/CustomDimensionConfig.cpp` | `same path` | ✔ done, rewritten: the bare printf became a log call with its danger noted, and the version upgrade path gained a comment |
| `packages/pier-dimensions/src/dim/CustomDimensionManager.cpp` | `packages/pier-dimensions/src/dim/CustomDimensionManager.cpp` | ✔ done, ported in full, with all five incident conclusions in place: the closure before the registration, ids starting at the largest plus one, salvagedIds keeping their numbers, the native path leaving Undefined alone, and comparing before writing to disk |
| `packages/pier-dimensions/src/dim/DimensionRules.cpp` | `packages/pier-dimensions/src/dim/DimensionRules.cpp` | ✔ done, ported in full: a log line whose two numbers both disagreed was corrected, and setDimensionRule gained an error for an out-of-range rule number |
| `packages/pier-dimensions/src/dim/NativeDimensions.cpp` | `same path` | ✔ done, rewritten: the bug history of DimensionDefinitionGroup not being persisted, the read-back check and the id change warning all kept; the diagnostic environment variables were renamed to PIER_DIM_DEF_* and listed in the migration notes; the never-called canCreateDimension was deleted and its conclusion folded into the three-cause comment of getOrCreateByName |
| `packages/pier-dimensions/src/dim/SimpleCustomDimension.cpp` | `packages/pier-dimensions/src/dim/SimpleCustomDimension.cpp` | ✔ done, ported in full: skylight decided per generator, the default branch speaking up, and the lazy resolution of the three symbols all kept |
| `packages/pier-dimensions/src/plot/PlotConfine.cpp` | `packages/pier-dimensions/src/plot/PlotConfine.cpp` | ✔ done, ported in full: the asymmetry of the group walk bound was kept with its reason written out, and the catch around a failed velocity clear gained an explanation |
| `packages/pier-dimensions/src/plot/PlotDimension.cpp` | `packages/pier-dimensions/src/plot/PlotDimension.cpp` | ✔ done, ported in full |
| `packages/pier-dimensions/src/plot/PlotGenerator.cpp` | `packages/pier-dimensions/src/plot/PlotGenerator.cpp` | ✔ done, ported in full, with the tracing switch no longer copied per file and taken from the one in chunk_trace.h |
| `packages/pier-dimensions/src/rt/ChunkTrace.cpp` | `packages/pier-dimensions/src/rt/ChunkTrace.cpp` | ✔ done, ported in full: it uses hostLogger now, the two empty catches gained explanations, and the environment variables were renamed |
| `packages/pier-dimensions/src/rt/MoreDimensionsBridge.cpp` | `src/rt/{Bridge,Slots}.cpp` | ✔ done, split and rewritten: Bridge.cpp is the DimensionBridge implementation, with the three-layer name source, the id consistency safety gate and the hard-won comment about toString crashing the server; Slots.cpp fills the eleven md_* slots, keeping the history of the GeneratorType numbering correction |
| `packages/pier-dimensions/src/rt/Utils.cpp` | `src/base/Utils.cpp` | ✔ done, rewritten to use the common host logger |
| `packages/pier-dimensions/xmake.lua` | `same path` | ✔ done, rewritten to state that this package is the only implementer of DimensionBridge and that its absence degrades the feature |
| `packages/pier-hooks/include/pier/hooks/decision_throttle.h` | `same path` | ✔ done, rewritten with all four design reasons kept: the XUID key, the position in the key, caching both outcomes, and bounded clearing |
| `packages/pier-hooks/include/pier/hooks/hook_events.h` | `same path` | ✔ done, rewritten as a provider contract: claiming through idMatches, no substring matching, a mandatory warning on shadowing under covers_registry=false, and priority carried into the subscription |
| `packages/pier-hooks/src/engine/HookEvents.cpp` | `same path` | ✔ done, rewritten: the EventProviderReg wiring, the cancel bit parsed uniformly through CompoundTag::fromSnbt so that missing one of the three spellings is impossible, and a callback exception printed on the spot |
| `packages/pier-hooks/src/engine/Profiler.cpp` | `same path` | ✔ done, rewritten: five inclusive timing buckets; in 26.20.2 the level_tick detour moved to Lowest priority so one call is one real tick under warp |
| `—` | `packages/pier-hooks/src/engine/TickStats.cpp` | ✔ done, added in 26.20.2: the always-on TPS and MSPT sampler behind get_tps and get_mspt, counting real ticks instead of inverting one frame period |
| `packages/pier-hooks/src/engine/TickControl.cpp` | `same path` | ✔ done, rewritten with the three states of freezing, stepping and warping unchanged, and the reason a patch cannot be removed inside a tick kept |
| `packages/pier-hooks/src/player/AttackEvent.cpp` | `same path` | ✔ done, rewritten with a thread_local guard added against double dispatch, since hooking both overloads made a counting subscriber count twice; the name lookup gained a real try/catch, making good on what the comment promised |
| `packages/pier-hooks/src/player/GameModeEvent.cpp` | `same path` | ✔ done, rewritten with the re-entry guard, the silent from==to case and the hook install warning all kept |
| `packages/pier-hooks/src/protect/DropItemEvent.cpp` | `same path` | ✔ done, rewritten: the argument for both drop paths, the reason for cancelling through NoError and the hook install warning, with the no-subscriber notice merged into one place |
| `packages/pier-hooks/src/protect/InteractEntityEvent.cpp` | `same path` | ✔ done, rewritten with the reason for keeping the swing animation, and a try/catch added |
| `packages/pier-hooks/src/protect/PressurePlateEvent.cpp` | `same path` | ✔ done, rewritten with the lesson of the two hook points and the throttling both kept |
| `packages/pier-hooks/src/protect/ProjectileEvent.cpp` | `same path` | ✔ done, rewritten: the history of the drift toward components and the division of the five hook points both kept; the re-entry gate became thread_local; the install status is reported per point with a separate error for the primary hook |
| `packages/pier-hooks/src/protect/PushEntityEvent.cpp` | `same path` | ✔ done, rewritten: the decision in both directions, player against player allowed, throttling, and a try/catch added |
| `packages/pier-hooks/src/protect/RideEvent.cpp` | `same path` | ✔ done, rewritten: the argument for the canAddPassenger hook point and the warning about the vehicle and rider being reversed, with a try/catch added |
| `packages/pier-hooks/src/protect/TakeEntityEvent.cpp` | `same path` | ✔ done, rewritten, correcting a file header that disagreed with the code: it claimed to hook Player::take while the code hooks the $playerTouch of Arrow and ThrownTrident. That isItemActor is always false, and why it is kept, are both stated |
| `packages/pier-hooks/src/world/ContainerEvents.cpp` | `same path` | ✔ done, rewritten with StopProcessing as the cancel channel |
| `packages/pier-hooks/src/world/DestroyEvents.cpp` | `same path` | ✔ done, rewritten with the timing automatic tool switching needs: dispatch before origin |
| `packages/pier-hooks/src/world/DimensionEvents.cpp` | `same path` | ✔ done, rewritten with the argument for it being the single funnel, and the rvalue request read before being forwarded |
| `packages/pier-hooks/src/world/HopperEvents.cpp` | `same path` | ✔ done, rewritten with the ICF folding guard and its discriminator kept in full |
| `packages/pier-hooks/src/world/UseItemOnEvent.cpp` | `same path` | ✔ done, rewritten: the TypedStorage reference collapse rule, the reason for flat coordinates, and isFirstEvent passed through |
| `packages/pier-hooks/xmake.lua` | `same path` | ✔ done, rewritten |
| `packages/pier-host/include/pier/host/host_api.h` | `pier/host/{api_table,spi}.h` | ✔ done, split and rewritten: table ownership plus the four SPI registration faces |
| `packages/pier-host/include/pier/host/hosted_mod.h` | `same path` | ✔ done, rewritten: ModHostName is "pier" and asMod moved in |
| `packages/pier-host/include/pier/host/mod_control.h` | `same path` | ✔ done, rewritten with type "pier" |
| `packages/pier-host/include/pier/host/mod_host.h` | `same path` | ✔ done, rewritten |
| `packages/pier-host/src/Entry.cpp` | `same path` | ✔ done, rewritten: build the table, bootstrap, register the manager; the layout self-check was deleted by design |
| `packages/pier-host/src/MemoryOperators.cpp` | `packages/pier-host/src/MemoryOperators.cpp` | ✔ done, rewritten, and load-blocking: without it LeviLamina refuses to load outright, and the error is reported only at load time |
| `packages/pier-host/src/ModControl.cpp` | `same path` | ✔ done, rewritten as /pier, folding in events and abi, where events includes the synthetic events |
| `packages/pier-host/src/ModHost.cpp` | `same path` | ✔ done, rewritten: the v1 handshake with the vtable struct_size and flags, the SPI veto and teardown, and the earlier fix kept |
| `packages/pier-host/xmake.lua` | `packages/pier-host/xmake.lua` | ✔ done, rewritten |
| `packages/pier-lane/src/Lane.cpp` | `same path` | ✔ done, rewritten: the liveness cell never freed, the release added at unload, not checking isEnabled, refusing a fingerprint of 0, and crossing a dylib outside the lock all kept; the busy veto now registers an UnloadVeto and cleanup moved to Teardown 40 |
| `packages/pier-lane/xmake.lua` | `same path` | ✔ done, rewritten with the note on optionality restated under the new discipline that an absent slot is NULL |
| `bindings/rust/pier-rs/Cargo.toml` | `bindings/rust/pier-rs/Cargo.toml` | ✔ done, rewritten with two features cut, since the v1 layout does not fork and a capability is decided at runtime; client only sets one bit of mod_flags |
| `bindings/rust/pier-rs/build.rs` | `—` | ✂ cut, not migrated: it computed the lane fingerprint, which the new repository takes from the `LaneContract::FINGERPRINT` associated constant. Referencing one contract definition on both sides makes it match automatically, while each side computing its own at build time is exactly the copying-an-identical-constant-by-hand case that guard exists for |
| `bindings/rust/pier-rs/src/block/actions.rs` | `bindings/rust/pier-rs/src/block/state.rs` | ✔ done, rewritten: block reads and writes, the native edit_* writes, and the liquid layer, where a waterlogged block is a second block in the same cell |
| `bindings/rust/pier-rs/src/block/gap_fill.rs` | `bindings/rust/pier-rs/src/block/edit.rs` | ✔ done, rewritten: block reads and writes, the native edit_* writes, and the liquid layer, where a waterlogged block is a second block in the same cell |
| `bindings/rust/pier-rs/src/block/mod.rs` | `bindings/rust/pier-rs/src/block/mod.rs` | ✔ done, rewritten: block reads and writes, the native edit_* writes, and the liquid layer, where a waterlogged block is a second block in the same cell |
| `bindings/rust/pier-rs/src/block/query.rs` | `bindings/rust/pier-rs/src/block/props.rs` | ✔ done, rewritten: block reads and writes, the native edit_* writes, and the liquid layer, where a waterlogged block is a second block in the same cell |
| `bindings/rust/pier-rs/src/client/events.rs` | `bindings/rust/pier-rs/src/client.rs` | ✔ done, rewritten: client only, an empty slot on a server host rather than a compile error |
| `bindings/rust/pier-rs/src/client/input.rs` | `bindings/rust/pier-rs/src/client.rs` | ✔ done, rewritten: client only, an empty slot on a server host rather than a compile error |
| `bindings/rust/pier-rs/src/client/mod.rs` | `bindings/rust/pier-rs/src/client.rs` | ✔ done, rewritten: client only, an empty slot on a server host rather than a compile error |
| `bindings/rust/pier-rs/src/client/status.rs` | `bindings/rust/pier-rs/src/client.rs` | ✔ done, rewritten: client only, an empty slot on a server host rather than a compile error |
| `bindings/rust/pier-rs/src/command/builder.rs` | `bindings/rust/pier-rs/src/command.rs` | ✔ done, rewritten: both raw text and typed overloads; a command cannot be deregistered, so the closure leaks on purpose |
| `bindings/rust/pier-rs/src/command/mod.rs` | `bindings/rust/pier-rs/src/command.rs` | ✔ done, rewritten: both raw text and typed overloads; a command cannot be deregistered, so the closure leaks on purpose |
| `bindings/rust/pier-rs/src/comms/bus.rs` | `bindings/rust/pier-rs/src/bus.rs` | ✔ done, rewritten: cross-mod broadcast, with dropping unsubscribing |
| `bindings/rust/pier-rs/src/comms/kvdb.rs` | `bindings/rust/pier-rs/src/kvdb.rs` | ✔ done, rewritten: the key-value store, where this family is thread safe and dropping closes it |
| `bindings/rust/pier-rs/src/comms/mod.rs` | `—` | ✂ cut, the module declaration of the directory itself; flattening left no matching file (contract §8: one concern per TU) |
| `bindings/rust/pier-rs/src/comms/more_dimensions.rs` | `bindings/rust/pier-rs/src/dimensions.rs` | ✔ done, rewritten: the facade over the optional md_* capability package, keeping a rule that was never registered apart from one registered as false |
| `bindings/rust/pier-rs/src/comms/packet.rs` | `bindings/rust/pier-rs/src/packet.rs` | ✔ done, rewritten, with a real consumer setting the priority: it was the first of the 85 outstanding rows genuinely needed. One HookDirection type split into Direction and Directions; the closure bound tightened from Send to Send plus Sync; a PacketHook deregisters on drop while forget() extends its life explicitly |
| `bindings/rust/pier-rs/src/comms/service.rs` | `bindings/rust/pier-rs/src/service.rs` | ✔ done, rewritten and folded into the flat domain modules |
| `bindings/rust/pier-rs/src/container/mod.rs` | `bindings/rust/pier-rs/src/container.rs` | ✔ done, rewritten: the five container kinds, with a refresh required after a write and a block container refusing explicitly |
| `bindings/rust/pier-rs/src/container/ops.rs` | `bindings/rust/pier-rs/src/container.rs` | ✔ done, rewritten: the five container kinds, with a refresh required after a write and a block container refusing explicitly |
| `bindings/rust/pier-rs/src/entity/actions.rs` | `bindings/rust/pier-rs/src/entity/actions.rs` | ✔ done, rewritten: the whole actor_* family, with both gates of the four relation slots kept at their own call sites |
| `bindings/rust/pier-rs/src/entity/gap_fill.rs` | `bindings/rust/pier-rs/src/entity/relations.rs` | ✔ done, rewritten: the whole actor_* family, with both gates of the four relation slots kept at their own call sites |
| `bindings/rust/pier-rs/src/entity/mod.rs` | `bindings/rust/pier-rs/src/entity/mod.rs` | ✔ done, rewritten: the whole actor_* family, with both gates of the four relation slots kept at their own call sites |
| `bindings/rust/pier-rs/src/entity/query.rs` | `bindings/rust/pier-rs/src/entity/props.rs` | ✔ done, rewritten: the whole actor_* family, with both gates of the four relation slots kept at their own call sites |
| `bindings/rust/pier-rs/src/event/mod.rs` | `bindings/rust/pier-rs/src/event/mod.rs` | ✔ done, rewritten and folded into the flat domain modules |
| `bindings/rust/pier-rs/src/event/names/mob.rs` | `bindings/rust/pier-rs/src/event/names.rs` | ✔ done, rewritten and folded into the flat domain modules |
| `bindings/rust/pier-rs/src/event/names/mod.rs` | `bindings/rust/pier-rs/src/event/names.rs` | ✔ done, rewritten and folded into the flat domain modules |
| `bindings/rust/pier-rs/src/event/names/player.rs` | `bindings/rust/pier-rs/src/event/names.rs` | ✔ done, rewritten and folded into the flat domain modules |
| `bindings/rust/pier-rs/src/event/names/server.rs` | `bindings/rust/pier-rs/src/event/names.rs` | ✔ done, rewritten and folded into the flat domain modules |
| `bindings/rust/pier-rs/src/gui/custom.rs` | `bindings/rust/pier-rs/src/gui.rs` | ✔ done, rewritten: the three form kinds, with the callback running at most once and the muted path leaking on purpose |
| `bindings/rust/pier-rs/src/gui/mod.rs` | `bindings/rust/pier-rs/src/gui.rs` | ✔ done, rewritten: the three form kinds, with the callback running at most once and the muted path leaking on purpose |
| `bindings/rust/pier-rs/src/gui/modal.rs` | `bindings/rust/pier-rs/src/gui.rs` | ✔ done, rewritten: the three form kinds, with the callback running at most once and the muted path leaking on purpose |
| `bindings/rust/pier-rs/src/gui/simple.rs` | `bindings/rust/pier-rs/src/gui.rs` | ✔ done, rewritten: the three form kinds, with the callback running at most once and the muted path leaking on purpose |
| `bindings/rust/pier-rs/src/item/gap_fill.rs` | `bindings/rust/pier-rs/src/item.rs` | ✔ done, rewritten: ItemStack as a value object, with SNBT always escaped through NbtValue::to_snbt |
| `bindings/rust/pier-rs/src/item/mod.rs` | `bindings/rust/pier-rs/src/item.rs` | ✔ done, rewritten: ItemStack as a value object, with SNBT always escaped through NbtValue::to_snbt |
| `bindings/rust/pier-rs/src/item/query.rs` | `bindings/rust/pier-rs/src/item.rs` | ✔ done, rewritten: ItemStack as a value object, with SNBT always escaped through NbtValue::to_snbt |
| `bindings/rust/pier-rs/src/lane/lane_error.rs` | `bindings/rust/pier-rs/src/lane.rs` | ✔ done, rewritten: the fast lane, with alive read under Acquire and busy held for the duration of a call |
| `bindings/rust/pier-rs/src/lane/mod.rs` | `bindings/rust/pier-rs/src/lane.rs` | ✔ done, rewritten: the fast lane, with alive read under Acquire and busy held for the duration of a call |
| `bindings/rust/pier-rs/src/lib.rs` | `bindings/rust/pier-rs/src/lib.rs` | ✔ done, rewritten with every cfg that trimmed the module tree cut, so one source compiles for both targets |
| `bindings/rust/pier-rs/src/misc/edit.rs` | `bindings/rust/pier-rs/src/block.rs` | ✔ done, rewritten: block reads and writes, the native edit_* writes, and the liquid layer, where a waterlogged block is a second block in the same cell |
| `bindings/rust/pier-rs/src/misc/mod.rs` | `—` | ✂ cut, the module declaration of the directory itself; flattening left no matching file (contract §8: one concern per TU) |
| `bindings/rust/pier-rs/src/misc/system.rs` | `bindings/rust/pier-rs/src/host.rs` | ✔ done, rewritten. This row and the host.rs row below describe the same old file; the ledger listed it twice and the two statuses contradicted each other |
| `bindings/rust/pier-rs/src/misc/types.rs` | `bindings/rust/pier-rs/src/types.rs` | ✔ done, rewritten: the shared value types gathered in one place, coordinates, enums and bit flags, shared between domain modules without them depending on one another |
| `bindings/rust/pier-rs/src/money/listen.rs` | `bindings/rust/pier-rs/src/money.rs` | ✔ done, rewritten: the whole economy family, with balance translating -1 into an Err |
| `bindings/rust/pier-rs/src/money/mod.rs` | `bindings/rust/pier-rs/src/money.rs` | ✔ done, rewritten: the whole economy family, with balance translating -1 into an Err |
| `bindings/rust/pier-rs/src/money/ops.rs` | `bindings/rust/pier-rs/src/money.rs` | ✔ done, rewritten: the whole economy family, with balance translating -1 into an Err |
| `bindings/rust/pier-rs/src/nbt/accessors.rs` | `bindings/rust/pier-rs/src/nbt/mod.rs` | ✔ done, rewritten and folded into the flat domain modules |
| `bindings/rust/pier-rs/src/nbt/binary.rs` | `bindings/rust/pier-rs/src/nbt/binary.rs` | ✔ done, rewritten: SNBT and binary in both directions, through the host parser |
| `bindings/rust/pier-rs/src/nbt/mod.rs` | `bindings/rust/pier-rs/src/nbt/mod.rs` | ✔ done, rewritten and folded into the flat domain modules |
| `bindings/rust/pier-rs/src/nbt/parser/containers.rs` | `bindings/rust/pier-rs/src/nbt/parse.rs` | ✔ done, rewritten and folded into the flat domain modules |
| `bindings/rust/pier-rs/src/nbt/parser/mod.rs` | `bindings/rust/pier-rs/src/nbt/parse.rs` | ✔ done, rewritten and folded into the flat domain modules |
| `bindings/rust/pier-rs/src/nbt/parser/scalars.rs` | `bindings/rust/pier-rs/src/nbt/parse.rs` | ✔ done, rewritten and folded into the flat domain modules |
| `bindings/rust/pier-rs/src/nbt/serde.rs` | `bindings/rust/pier-rs/src/nbt/mod.rs` | ✔ done, rewritten and folded into the flat domain modules |
| `bindings/rust/pier-rs/src/player/actions.rs` | `bindings/rust/pier-rs/src/player/admin.rs` | ✔ done, rewritten: the whole player_* family, with the selector identity discipline stated in the module header |
| `bindings/rust/pier-rs/src/player/gap_fill.rs` | `bindings/rust/pier-rs/src/player/props.rs` | ✔ done, rewritten: the whole player_* family, with the selector identity discipline stated in the module header |
| `bindings/rust/pier-rs/src/player/inventory.rs` | `bindings/rust/pier-rs/src/player/items.rs` | ✔ done, rewritten: the whole player_* family, with the selector identity discipline stated in the module header |
| `bindings/rust/pier-rs/src/player/mod.rs` | `bindings/rust/pier-rs/src/player/mod.rs` | ✔ done, rewritten: the whole player_* family, with the selector identity discipline stated in the module header |
| `bindings/rust/pier-rs/src/player/query.rs` | `bindings/rust/pier-rs/src/player/props.rs` | ✔ done, rewritten: the whole player_* family, with the selector identity discipline stated in the module header |
| `bindings/rust/pier-rs/src/player/types.rs` | `bindings/rust/pier-rs/src/types.rs` | ✔ done, rewritten: the shared value types gathered in one place, coordinates, enums and bit flags, shared between domain modules without them depending on one another |
| `bindings/rust/pier-rs/src/rt/error.rs` | `bindings/rust/pier-rs/src/rt/error.rs` | ✔ done, rewritten |
| `bindings/rust/pier-rs/src/rt/ffi.rs` | `bindings/rust/pier-rs/src/rt/ffi.rs` | ✔ done, rewritten: PierStr became a c_char pointer, the UTF-8 validation and its one-time warning were kept, and collect_byte_chunks was added |
| `bindings/rust/pier-rs/src/rt/handle.rs` | `bindings/rust/pier-rs/src/rt/handle.rs` | ✔ done, rewritten with an AtomicPtr instead of an unsafe impl Send plus Sync, and the reason written where it applies |
| `bindings/rust/pier-rs/src/rt/logger.rs` | `bindings/rust/pier-rs/src/rt/logger.rs` | ✔ done, rewritten with the slots as Option, so a table left incomplete no longer crashes inside logging; a fatal level was added |
| `bindings/rust/pier-rs/src/rt/mod.rs` | `bindings/rust/pier-rs/src/rt/mod.rs` | ✔ done, rewritten |
| `bindings/rust/pier-rs/src/rt/registration.rs` | `bindings/rust/pier-rs/src/rt/registration.rs` | ✔ done, rewritten: the three v1 handshake gates, length then version range then target flags; the high-bit target marker was cut; the vtable fills the four-field header |
| `bindings/rust/pier-rs/src/misc/system.rs` | `bindings/rust/pier-rs/src/host.rs` | ✔ done, added: the host and system level capabilities gathered in one place, and at the same time the first caller of those sinks in rt::ffi |
| `bindings/rust/pier-rs/src/rt/runtime.rs` | `bindings/rust/pier-rs/src/rt/runtime.rs` | ✔ done, rewritten: require_slot! gained the second gate for a non-null slot, and the message dropped the historical product name in favor of the host ABI version and table length |
| `bindings/rust/pier-rs/src/scoreboard/mod.rs` | `bindings/rust/pier-rs/src/scoreboard.rs` | ✔ done, rewritten: a named facade over the multiplexed scoreboard_op slot, keeping having no score apart from a score of 0 |
| `bindings/rust/pier-rs/src/scoreboard/ops.rs` | `bindings/rust/pier-rs/src/scoreboard.rs` | ✔ done, rewritten: a named facade over the multiplexed scoreboard_op slot, keeping having no score apart from a score of 0 |
| `bindings/rust/pier-rs/src/server/mod.rs` | `bindings/rust/pier-rs/src/server.rs` | ✔ done, rewritten: tick freezing, stepping and warping, plus per-subsystem sampling |
| `bindings/rust/pier-rs/src/server/ops/commands.rs` | `bindings/rust/pier-rs/src/command.rs` | ✔ done, rewritten: both raw text and typed overloads; a command cannot be deregistered, so the closure leaks on purpose |
| `bindings/rust/pier-rs/src/server/ops/events.rs` | `bindings/rust/pier-rs/src/event/mod.rs` | ✔ done, rewritten and folded into the flat domain modules |
| `bindings/rust/pier-rs/src/server/ops/mod.rs` | `—` | ✂ cut, the module declaration of the directory itself; flattening left no matching file (contract §8: one concern per TU) |
| `bindings/rust/pier-rs/src/server/ops/sim.rs` | `bindings/rust/pier-rs/src/sim.rs` | ✔ done, rewritten: the simulated player verb table |
| `bindings/rust/pier-rs/src/server/ops/status.rs` | `bindings/rust/pier-rs/src/host.rs` | ✔ done, rewritten and folded into the flat domain modules |
| `bindings/rust/pier-rs/src/server/ops/time.rs` | `bindings/rust/pier-rs/src/world.rs` | ✔ done, rewritten: level reads and writes, streaming scans, and chunk save keys, where a binary key never passes through UTF-8 |
| `bindings/rust/pier-rs/src/server/run/data.rs` | `bindings/rust/pier-rs/src/world.rs` | ✔ done, rewritten: level reads and writes, streaming scans, and chunk save keys, where a binary key never passes through UTF-8 |
| `bindings/rust/pier-rs/src/server/run/fill.rs` | `bindings/rust/pier-rs/src/world/commands.rs` | ✔ done, rewritten: block reads and writes, the native edit_* writes, and the liquid layer, where a waterlogged block is a second block in the same cell |
| `bindings/rust/pier-rs/src/server/run/mod.rs` | `—` | ✂ cut, the module declaration of the directory itself; flattening left no matching file (contract §8: one concern per TU) |
| `bindings/rust/pier-rs/src/server/run/profiler.rs` | `bindings/rust/pier-rs/src/server.rs` | ✔ done, rewritten: tick freezing, stepping and warping, plus per-subsystem sampling |
| `bindings/rust/pier-rs/src/server/run/tick.rs` | `bindings/rust/pier-rs/src/server.rs` | ✔ done, rewritten: tick freezing, stepping and warping, plus per-subsystem sampling |
| `bindings/rust/pier-rs/src/server/run/ticking.rs` | `bindings/rust/pier-rs/src/server.rs` | ✔ done, rewritten: tick freezing, stepping and warping, plus per-subsystem sampling |
| `bindings/rust/pier-rs/src/server/sel/dimsel.rs` | `bindings/rust/pier-rs/src/sel.rs` | ✔ done, rewritten and folded into the flat domain modules |
| `bindings/rust/pier-rs/src/server/sel/mod.rs` | `bindings/rust/pier-rs/src/sel.rs` | ✔ done, rewritten and folded into the flat domain modules |
| `bindings/rust/pier-rs/src/server/world/blocks.rs` | `bindings/rust/pier-rs/src/block.rs` | ✔ done, rewritten: block reads and writes, the native edit_* writes, and the liquid layer, where a waterlogged block is a second block in the same cell |
| `bindings/rust/pier-rs/src/server/world/entities.rs` | `bindings/rust/pier-rs/src/entity.rs` | ✔ done, rewritten: the whole actor_* family, with both gates of the four relation slots kept at their own call sites |
| `bindings/rust/pier-rs/src/server/world/gap_fill.rs` | `bindings/rust/pier-rs/src/world.rs` | ✔ done, rewritten: level reads and writes, streaming scans, and chunk save keys, where a binary key never passes through UTF-8 |
| `bindings/rust/pier-rs/src/server/world/mod.rs` | `bindings/rust/pier-rs/src/world.rs` | ✔ done, rewritten: level reads and writes, streaming scans, and chunk save keys, where a binary key never passes through UTF-8 |
| `bindings/rust/pier-rs/src/server/world/particles.rs` | `bindings/rust/pier-rs/src/world/edit.rs` | ✔ done, rewritten: level reads and writes, streaming scans, and chunk save keys, where a binary key never passes through UTF-8 |
| `bindings/rust/pier-rs/src/sim/actions.rs` | `bindings/rust/pier-rs/src/sim.rs` | ✔ done, rewritten: the simulated player verb table |
| `bindings/rust/pier-rs/src/sim/mod.rs` | `bindings/rust/pier-rs/src/sim.rs` | ✔ done, rewritten: the simulated player verb table |
| `bindings/rust/pier-rs/src/world/mod.rs` | `bindings/rust/pier-rs/src/world.rs` | ✔ done, rewritten: level reads and writes, streaming scans, and chunk save keys, where a binary key never passes through UTF-8 |
| `bindings/rust/pier-rs/src/world/scan.rs` | `bindings/rust/pier-rs/src/world/edit.rs` | ✔ done, rewritten: level reads and writes, streaming scans, and chunk save keys, where a binary key never passes through UTF-8 |
| `bindings/rust/pier-rs/src/world/structures.rs` | `bindings/rust/pier-rs/src/world.rs` | ✔ done, rewritten: level reads and writes, streaming scans, and chunk save keys, where a binary key never passes through UTF-8 |
| `bindings/rust/pier-rs/tests/lane.rs` | `bindings/rust/pier-rs/tests/lane.rs` | ⬜ outstanding, to be migrated by rewriting file by file |
| `bindings/rust/pier-sys-rs/Cargo.toml` | `bindings/rust/pier-sys-rs/Cargo.toml` | ✔ done, rewritten with the three layout-changing features cut, which were the root cause of the seven-slot misalignment; only client remains and it adds and removes no field |
| `bindings/rust/pier-sys-rs/src/api.rs` | `bindings/rust/pier-sys-rs/src/api.rs` | ✔ done, rewritten with 194 fields matched cell for cell against abi.h, guarded by sys-mirrors-abi, and no #[cfg] at all |
| `bindings/rust/pier-sys-rs/src/consts/actor.rs` | `bindings/rust/pier-sys-rs/src/consts/actor.rs` | ✔ done, rewritten with 85 constants; the PIER_AACT_ADD_EFFECT missed during transcription was caught by the check and added |
| `bindings/rust/pier-sys-rs/src/consts/player.rs` | `bindings/rust/pier-sys-rs/src/consts/player.rs` | ✔ done, rewritten with 73 constants |
| `bindings/rust/pier-sys-rs/src/consts/world.rs` | `bindings/rust/pier-sys-rs/src/consts/world.rs` | ✔ done, rewritten with 123 constants |
| `bindings/rust/pier-sys-rs/src/consts.rs` | `bindings/rust/pier-sys-rs/src/consts.rs` | ✔ done, rewritten |
| `bindings/rust/pier-sys-rs/src/lib.rs` | `bindings/rust/pier-sys-rs/src/lib.rs` | ✔ done, rewritten with the target mask and tagged version cut, since the target marker moved to mod_flags |
| `bindings/rust/pier-sys-rs/src/money.rs` | `src/types.rs` (folded in) | ✔ done, folded into types.rs, with LLMoneyEvent renamed to PierMoneyEvent, since the old name collided with an identically named type in the global scope of LegacyMoney |
| `bindings/rust/pier-sys-rs/src/types.rs` | `bindings/rust/pier-sys-rs/src/types.rs` | ✔ done, rewritten with PierStr an explicit {ptr,len} and 19 callback signatures matched one by one |
| `bindings/rust/pier-sys-rs/src/vtable.rs` | `bindings/rust/pier-sys-rs/src/vtable.rs` | ✔ done, rewritten with the new v1 handshake: the four-field header of struct_size, abi_version and mod_flags |
| `—` | `compile_commands.json` | ✂ cut, a build artifact, the compilation database xmake generates, and not a source file. It is in `.gitignore` now. This row exists only so that `ledger-covers-tree` has a source for that leftover in the workspace: a checklist missing an item can never reveal that item |
| `—` | `bindings/rust/pier-rs/src/rt/accessors.rs` | ✔ done, added: the `accessors!` macro generates the property accessors from the constant tables, replacing dozens of one-line lookup methods across four domains |
| `—` | `bindings/rust/pier-rs/src/block/state.rs` | ✔ done, added: block states and block entities, split out of the flat block.rs |
| `—` | `bindings/rust/pier-rs/src/block/edit.rs` | ✔ done, added: block writes and the liquid layer, split out of the flat block.rs |
| `—` | `bindings/rust/pier-rs/src/entity/relations.rs` | ✔ done, added: actor relations, equipment and effects, and rays, split out of the flat entity.rs |
| `—` | `bindings/rust/pier-rs/src/player/io.rs` | ✔ done, added: the outbound channels of a player, meaning messages, titles, particles and raw packets, split out of the flat player.rs |
| `—` | `bindings/rust/pier-rs/src/context.rs` | ✔ done, added: the facade for mod authors aggregating the entry points of every domain, moved out of rt/runtime.rs, since it is not part of the runtime foundation |
| `—` | `docs/verify-delayload.md` | ✔ done, added: a runbook for verifying that /DELAYLOAD really took effect, through the dumpbin import table, the xmake -v link command line, and really moving LegacyMoney away |
| `—` | `tools/checks/delayload_matches_claims.py` | ✔ done, added: a check that a DLL the code describes as delay-loaded really carries a /DELAYLOAD on the linker side, in the right flag channel |
| `xmake.lua` | `xmake.lua` | ✔ done, rewritten: the global macros at root scope, with the object packages aggregated |


## Appendix: the renames visible at runtime when upgrading to this version

These are not internal to the code, they are what a server owner or an operator runs into.
They are listed one by one so that following the old documentation and getting no response
does not become an investigation after an upgrade.

| Old | New | Effect |
|---|---|---|
| the environment variables `MORE_DIMENSIONS_DEF_MIN` and `..._MAX` | `PIER_DIM_DEF_MIN` / `PIER_DIM_DEF_MAX` | A diagnostic switch only, overriding the dimension height advertised to the client. Unset, the behavior is unchanged. |
| the environment variable `PIER_TRACE_FORM` | unchanged | — |
| the dimension ledger `worlds/<levelName>/dimension_config.json` | **unchanged** | A comment once claimed it lived under a configs directory, which was wrong: it has always followed the save and an upgrade moves no file. |
| the command `/pier` | unchanged | — |

No save data format changed: a dimension id is still held by the NameIdStore of the engine,
and the DimensionId in a player save is unaffected by this rewrite.




### Files the new architecture added or renamed

They have no counterpart in the old repository, so they do not appear in the table above
during a row-by-row migration, which is exactly the evidence that the ledger used to run in
one direction only: adding the `ledger-covers-tree` check caught 31 of them at once.

| New location | Source |
|---|---|
| `packages/pier-api/include/pier/api/bridge.h` | ✔ done, added: the declarations of the resolution helpers |
| `packages/pier-api/src/core/Bridge.cpp` | ✔ done, added: the resolution helpers that were spread across TUs in the old repository, gathered here |
| `packages/pier-api/src/core/Enrich.cpp` | ✔ done, added: enrichEventData became its own TU |
| `packages/pier-api/src/core/GameStats.cpp` | ✔ done, added |
| `packages/pier-api/src/core/Log.cpp` | ✔ done, added: the log slot became its own TU, since it must carry no domain dependency |
| `packages/pier-api/src/core/Scheduler.cpp` | ✔ done, added |
| `packages/pier-api/src/runtime/CommandEvents.cpp` | ✔ done, added: the command event provider, with covers_registry=true |
| `packages/pier-api/src/world/BlockResolve.cpp` | ✔ done, added, fixing a real broken link in the old repository: three helpers were defined in the server-only Edit.cpp and referenced by the dual-target World.cpp |
| `packages/pier-client/xmake.lua` | ✔ done, added: the client slots became their own package |
| `packages/pier-dimensions/include/pier/dimensions/base/native_dimensions.h` | ✔ done, renamed from the old `base/NativeDimensions.h` into snake_case |
| `packages/pier-dimensions/include/pier/dimensions/base/simple_custom_dimension.h` | ✔ done, renamed from the old `base/SimpleCustomDimension.h` |
| `packages/pier-dimensions/include/pier/dimensions/base/utils.h` | ✔ done, renamed from the old `base/Utils.h` |
| `packages/pier-dimensions/include/pier/dimensions/dim/complete_base_types.h` | ✔ done, renamed from the old `dim/CompleteBaseTypes.h` |
| `packages/pier-dimensions/include/pier/dimensions/dim/custom_dimension_config.h` | ✔ done, renamed from the old `dim/CustomDimensionConfig.h` |
| `packages/pier-dimensions/include/pier/dimensions/dim/dimension_height.h` | ✔ done, renamed from the old `dim/DimensionHeight.h` |
| `packages/pier-dimensions/src/base/Utils.cpp` | ✔ done, moved from the old `src/rt/Utils.cpp`, see the row above |
| `packages/pier-dimensions/src/rt/Bridge.cpp` | ✔ done, added: the only implementation of spi::DimensionBridge, including the id consistency safety gate |
| `packages/pier-dimensions/src/rt/Slots.cpp` | ✔ done, added: it fills the eleven md_* slots |
| `packages/pier-host/include/pier/host/api_table.h` | ✔ done, added: the owner of the PierApi table |
| `packages/pier-host/include/pier/host/spi.h` | ✔ done, added: the six registration faces, replacing the hardcoded forwarding of the old ApiTable (contract §1 rule 2) |
| `packages/pier-host/src/ApiTable.cpp` | ✔ done, added: it fills the four header scalars only, and each capability package fills its own domain slots |
| `packages/pier-host/src/Spi.cpp` | ✔ done, added: the SPI registry itself |
| `packages/pier-support/include/pier/support/guard.h` | ✔ done, added: PIER_API_GUARD_*, gathering the barrier that was spread through the old repository |
| `packages/pier-support/include/pier/support/log.h` | ✔ done, added |
| `packages/pier-support/include/pier/support/module.h` | ✔ done, added |
| `packages/pier-support/include/pier/support/snbt.h` | ✔ done, added |
| `packages/pier-support/include/pier/support/str.h` | ✔ done, added: PierStr and string_view in both directions. The old repository relied on PierStr being an alias for string_view, and after the v1 recut the conversion has to be explicit |
| `packages/pier-support/src/Log.cpp` | ✔ done, added: the single source of the host logger |
| `packages/pier-support/src/Module.cpp` | ✔ done, added: addressOwnedBy sank here from pier-api |
| `packages/pier-support/src/Snbt.cpp` | ✔ done, added: SNBT escaping and number formatting, which were spread everywhere in the old repository |
| `packages/pier-support/xmake.lua` | ✔ done, added: the whole pier-support package is a layer the new architecture introduced, since the logging entry point has to live below host (contract §1) |
| `CHANGELOG.md` | ✔ added: the release notes, in Keep a Changelog form; the release workflow extracts the current section from it |
| `README.zh.md` | ✔ added: the Chinese README, marked as such. Contract §7 governs comments and not a translated document |
| `docs/zh/guide/abi.md` | ✔ added: Chinese: the ABI |
| `docs/zh/guide/adding-a-language.md` | ✔ added: Chinese: adding a language |
| `docs/zh/guide/compatibility.md` | ✔ added: Chinese: compatibility |
| `docs/zh/guide/design.md` | ✔ added: Chinese: how the C++ side is designed |
| `docs/zh/guide/installation.md` | ✔ added: Chinese: installing |
| `docs/zh/guide/manifest.md` | ✔ added: Chinese: the manifest |
| `docs/zh/guide/what-is-pier.md` | ✔ added: Chinese: what Pier is |
| `docs/zh/guide/why.md` | ✔ added: Chinese: where Pier came from |
| `docs/zh/index.md` | ✔ added: the Chinese landing page |
| `docs/zh/rust/api.md` | ✔ added: Chinese: the API map |
| `docs/zh/rust/commands.md` | ✔ added: Chinese: commands |
| `docs/zh/rust/cross-mod.md` | ✔ added: Chinese: talking to other mods |
| `docs/zh/rust/errors.md` | ✔ added: Chinese: errors and logging |
| `docs/zh/rust/events.md` | ✔ added: Chinese: events |
| `docs/zh/rust/first-mod.md` | ✔ added: Chinese: a first mod |
| `docs/zh/rust/index.md` | ✔ added: Chinese: the Rust binding overview |
| `docs/zh/rust/lifecycle.md` | ✔ added: Chinese: the mod lifecycle |
| `docs/zh/rust/threads.md` | ✔ added: Chinese: threads |
| `.github/workflows/build.yml` | ✔ added: builds the host, runs the contract checks of §9, and builds and tests the bindings |
| `.github/workflows/release.yml` | ✔ added: builds and flattens the release archive that lip places into plugins/pier/ |
| `.github/workflows/prime-cache.yml` | ✔ added: installs the packages once with a timeout long enough to finish and saves the cache explicitly. A normal build job times out during the levilamina install, and actions/cache only saves after a successful job, so the cache it needs can never be built by the build jobs |
| `docs/.vitepress/config.mts` | ✔ added: the navigation and sidebar; the site runs general to specific, /guide/ for Pier itself and /rust/ for the first official binding |
| `docs/guide/abi.md` | ✔ added: the shape of the ABI, the two gates, ownership and evolution |
| `docs/guide/adding-a-language.md` | ✔ added: the four steps of contract §10, for a binding in any language |
| `docs/guide/compatibility.md` | ✔ added: version numbering, and what the ABI does and does not promise |
| `docs/guide/design.md` | ✔ added: the C++ side: package layering, inward registration, the two gates, ownership, and the checks that guard each rule |
| `docs/guide/installation.md` | ✔ added: requirements, installing with lip or by hand, and checking it worked |
| `docs/guide/manifest.md` | ✔ added: the manifest fields and the two that go wrong |
| `docs/guide/what-is-pier.md` | ✔ added: what Pier is, the shape of it, and which binding to reach for |
| `docs/guide/why.md` | ✔ added: where Pier came from: the four structural faults of the predecessor loader and what replaced each |
| `docs/index.md` | ✔ added: the documentation landing page |
| `docs/package.json` | ✔ added: the VitePress documentation site |
| `docs/rust/api.md` | ✔ added: the map of the SDK, with rustdoc holding the detail |
| `docs/rust/commands.md` | ✔ added: raw-text commands, typed overloads, and why registration is one way |
| `docs/rust/cross-mod.md` | ✔ added: services, the bus and lanes, and which shape of question each answers |
| `docs/rust/errors.md` | ✔ added: the error discipline, identity, logging, and the panic fences |
| `docs/rust/events.md` | ✔ added: subscribing, reading a payload, cancelling, and batching with Wiring |
| `docs/rust/first-mod.md` | ✔ added: writing, building and installing a first mod, plus what to check when nothing happens |
| `docs/rust/index.md` | ✔ added: the Rust binding: the two crates, the smallest mod, and what the safe layer adds |
| `docs/rust/lifecycle.md` | ✔ added: the four callbacks and what belongs in each |
| `docs/rust/threads.md` | ✔ added: the server thread default and the packet interception exception |
| `tooth.json` | ✔ added: the lip package manifest, which is how a server owner installs a release |

**Summary**: ✔ 203 | ✂ 12 | ⬜ 44 (259 old files in total)

---

## The fixes from the first compile on a real machine (2026-08-30)

`build.bat`, meaning MSVC 2022 with xmake, and `cargo clippy --workspace -- -D warnings`
really ran for the first time and reported four classes of error, one by one:

| Error | Root cause | Fix | What guards it now |
|---|---|---|---|
| `Entry.cpp(47) C2065: "ModHostName" is undeclared` | it used a constant of `hosted_mod.h` while including only `mod_host.h`, which does not include it transitively | the include was added | `tools/include-surrogate.py` (new at the time) |
| `api.rs: cannot find type int`, twice | the C `int` was carried over unchanged while transcribing the mirror by hand | see below | `tools/rust-surrogate.py`, extended |
| `api.rs: expected one of ... found short` | the same, for `unsigned short` | see below | the same |
| `player.rs:173 expected item after doc comment` | a trailing comment that wraps in `abi.h` belongs to the item above it, and moving it in place attached it to the item below | the three consts files were regenerated with the comment ownership right | the same |
| `consts.rs: unused import player::*` | a knock-on of the row above, where a failed module parse yielded zero items | fixed together with the row above | — |

The root cause was not in the mirror but in the contract itself. Nine slots of the `money_*`
family copied the LegacyMoney signatures verbatim, carrying `long long`, `int` and
`unsigned short`, whose widths depend on the platform, while the other 190 slots were all
fixed width. A C compiler reports nothing, since they are valid C, and only the Rust side
broke. They all became `int64_t`, `int32_t` and `uint16_t`, which are binary identical on
MSVC x64 and cost nothing since v1 was unreleased, and the `abi-fixed-width` check was
added.

A real gap was filled along the way: `sys-mirrors-abi` used to compare slot names only and
not signatures. `cargo check` catches `int` not being a Rust type and does not catch the
mirror writing `i32` where `abi.h` has `int64_t`, which compiles on both sides and reads
half a number at runtime. All 190 slot signatures are now compared parameter by parameter,
and it was verified to catch both a wrong width and two parameters swapped.

---

## The fixes from the second compile on a real machine

`xmake` got past Entry.cpp and carried on, and `cargo clippy` moved one step further too.
Each side reported one class:

| Error | Root cause | Fix | What guards it now |
|---|---|---|---|
| `spi.h(155) C2039: "string" is not a member of "std"` | it used `std::string` while including only `<string_view>` | `<string>` was added, and 17 more of the same kind were really searched out across the repository and fixed together | `include-surrogate`, extended with a second criterion |
| seven helpers in `ffi.rs` reported as `never used` under `-D warnings` | infrastructure was laid down with no caller | see below | `rust-surrogate`, extended with a dead-code criterion |

Those 17 places compile at the moment only because another header happens to bring the
standard header in. Deleting one include line in that file, or changing standard library
version, breaks all of them together. The machine reported one, because the compile stopped
at the first.

How the seven dead_code warnings were handled is worth recording. There were three routes:
adding `#[allow(dead_code)]`, deleting them, or giving them a real caller. A combination of
the last two was chosen:

* `r` / `push_string` / `set_string` / `call_out_str` / `collect_strs`
    -> `pier-rs/src/host.rs` was added, covering the host and system level capabilities of the
    run stage, scheduling, executing commands, listing event ids and system and server
    information, and it became the first caller of those five;
* `collect_bytes` and `collect_byte_chunks` were deleted. They had no caller, and they
    come back once the first domain with a byte sink lands, meaning binary NBT or a packet
    body.

The discipline is publishing only what has a caller. A helper with no caller has never had
its `# Safety` assertions about the `ctx` type tested at any real call site, which is
looking ready rather than being ready. `-D warnings` is right here, and getting around it
with an `#[allow]` is what is wrong.

`hello-pier` picked up the `host` layer with it: it really asks for the server stage, the
player count and the tick, lists the event ids once, and hands a delayed task back to the
server thread, so the four steps of contract §10 went from reading correctly to being
verifiable by running them.

---

## The fixes from the third compile on a real machine

The C++ side got through most of pier-host, pier-hooks and pier-dimensions, 30 TUs, and
stopped at `pier-lane`. The Rust side had one dead_code warning left.

| Error | Root cause | Fix | What guards it now |
|---|---|---|---|
| `hosted_mod.h(9) C1083: cannot open "ll/api/event/ListenerBase.h"` | `pier-lane` had no `add_packages("levilamina")` | it was added | `build-config` now computes over the include closure |
| `C4819`, one per file | MSVC read UTF-8 comments carrying CJK characters under code page 936 | the root xmake gained `add_cxflags("/utf-8")` | `build-config` gained a criterion |
| `host.rs: method handle is never used` | `Host::handle` had no caller | deleted | the dead-code criterion of `rust-surrogate` became method aware |

### 1. Why `build-config` under-reported is worth recording

`Lane.cpp` writes not one `ll/` include of its own, including only the standard library and
`pier/`. But `pier/host/hosted_mod.h` includes `ll/api/event/ListenerBase.h`.
The compiler expands the closure of the includes and not the first level, while the first
version of the check looked at the first level only.

The shape of this criterion is identical to the second criterion of the include surrogate,
the source of a `std::X`: whether something compiles depends on what is in the closure and
not on what the file itself wrote.

### 2. Why `Host::handle` slipped past the surrogate

The dead-code criterion was the name appearing more than once in the crate. That suffices
for a free function and not for a method inside an impl: the word `handle` appears a dozen
times in the crate, as the `Handle` type, a `handle:` field and the parameter name of
`set_runtime(api, handle)`, so the count passes easily.

Looking at the call shape instead, `.name(` or `::name(`, ran into another limit:
`Runtime::handle` and `Host::handle` share a name, and which one `rt.handle()` calls cannot
be told apart by a textual criterion. That is a hard boundary and needs type inference.

It was handled by reporting it truthfully rather than pretending to cover it: a duplicated
method name is excluded explicitly and the output lists which ones and on which types they
are defined. A check that can say what it missed is far more useful than one pretending to
cover everything (contract §9: a pass only earns a checkmark for what it covers).

### 3. Two invented APIs cleared away along the way

* `Host::handle()` had no caller;
* `Host::no_task()` only wrapped the already public `TaskId::NONE` one layer further.

Deleting `no_task` immediately made `use ... TaskId` unused. Such a chain is the easiest
link to miss while deleting code, so the surrogate gained a criterion for an unused `use`.

### 4. The fourth trap of the same family in this round

The first version of the `/utf-8` check asked whether `/utf-8` appeared anywhere in the
text of the root xmake. The explanatory comment written for that line contains `/utf-8`
itself, so deleting the `add_cxflags` line left the check green.

The first three were searching for X inside text with X stripped out, and this one was
searching for a code feature inside text that includes comments. The common shape is that
what the criterion looks at is not the same thing as what it means to assert.
It now strips the Lua comments before matching `add_cxflags(...)`.

---

## The fixes from the fourth compile on a real machine

The C++ side reached `pier-api` at 45% and stopped at `core/Log.cpp`.

| Error | Root cause | Fix |
|---|---|---|
| `Log.cpp(24) C3861: identifier "sv" not found`, six times | it used `pier::sv` without including `pier/support/str.h` | the include was added |

The point of this round is not the fix, it is that the include surrogate should have caught
it and did not. It had been written for exactly this in the previous round and missed on
its first try. Four independent defects were stacked on top of one another:

1. It scanned only identifiers starting with a capital and at least four characters long.
      The `sv`, `ps` and `toString` family of free functions never entered its view.
2. The symbol table took a class member for a free function. The `getInstance` inside
      `class X { static X& getInstance(); }` was collected as a free symbol, so every TU
      calling `foo.getInstance()` was required to include that class header. Relaxing item 1
      immediately produced 22 reports, all of that kind.
3. It recognized only a function declaration ending in `;`. An inline definition in a
      header, such as `inline std::string_view sv(PierStr s) { ... }`, ends in `{`, and the
      scope splitting breaks the fragment at exactly that `{`, which requires a character
      that necessarily is not in the fragment.
4. A forward declaration was taken for a definition. A `struct DimensionFactoryInfo;` made
      the real definition ambiguous and excluded the whole name from the check. That one is a
      silent under-report and the hardest to find.

Once fixed it caught four places at once, the `sv` of `Log.cpp` and the `sv` plus
`<mutex>` and `<unordered_set>` of `Bridge.cpp`. The machine reported one, since the other
three lay beyond where the compiler reached.

### The fifth trap of the same family, in this round

Item 3 is exactly it: what the criterion looks at, the terminating `{`, is not the same
thing as what it means to assert, that there is a function here. The first four were
searching for a comment inside text with comments stripped, searching for an include path
inside text with strings stripped, a string regex that did not span lines, and searching for
a code feature inside text that includes comments.

All five have the identical shape. That is no longer a coincidence, it is the inherent trap
of this kind of textual check: writing a criterion starts with asking whether the text
being scanned still contains the thing being looked for.

### Along the way

The symbol table now covers 102 names with no ambiguity, including the whole family of
`sv`, `ps`, `toString`, `snbtEscape`, `asMod` and `idMatches`, which is what the 21 files
of `pier-api` not yet reached are most likely to use.

---

## The fixes from the fifth compile on a real machine

The C++ side reached 50% at `NbtApi.cpp` and stopped at `core/Enrich.cpp`. This was the
first error at the engine API level rather than a formal problem such as an include.

| Error | Root cause |
|---|---|
| `Enrich.cpp(199) C2228: left of ".get" must have class/struct/union`, for `ActorDamageCause` | `mCause` holds an enum, and TypedStorage specializes on a scalar |
| `Enrich.cpp(216) C2039: "get" is not a member of "Dimension"` | `mDimension` holds a `Dimension&`, the reference specialization |

### The rule was already in the repository, spread across the comments of four files

`UseItemOnEvent.cpp` said scalars and references both collapse and only a class type by
value stays wrapped, `ChunkTrace.cpp` said operator-> is not guaranteed when it holds a
reference, and `DropItemEvent.cpp` covered the unique_ptr cell. All three were worded
differently.

A rule with four sources has no source: nobody knows which is current and which is
complete. The single source is now the file header of `tools/typed-storage.py`, holding the
full four-cell rule table, and those three comments point at it.

### `tools/typed-storage.py` was added

It reads the engine headers to decide the `T` of each `m*` member and then verifies each
use of `.get()`, in both directions:

* a `.get()` written on a collapsed type is a compile error, which is what the machine
  reported twice;
* a class type by value missing its `.get()` is equally a compile error, only with a
  different symptom.

The engine headers live in the LeviLamina xmake package directory and were not on this
machine, so it reports SKIP and not PASS when it cannot find them. Pointing
`PIER_LL_INCLUDE` at them makes it run. A synthetic set of engine headers verified that it
reproduces those two machine errors exactly.

Its value is reporting everything at once: the compiler reports only the first failing TU
while this script verified all 30 `.get()` call sites in the repository together, and 19
TUs of `pier-api` were not yet reached, holding the still unverified `mSerializationId`,
`mGameRules` and `mAttachedBlocks`.

### The driver changed too

`run-surrogates.py` used to count a skip as a pass, which is exactly the
claiming-coverage-that-does-not-exist these tools exist to oppose. PASS and SKIP are now
reported separately, saying plainly that a skipped check has no conclusion.

### A lying comment corrected along the way

The sentence in `Enrich.cpp` about going through a public member and not calling a virtual
stopped holding once it moved to `getDimensionId().value()`. It was deleted under §5.4,
with the reason for not saving that one virtual call written out: the TypedStorage shape of
`Dimension::mId` was never verified while `getDimensionId().value()` already compiled along
with the whole of pier-dimensions, and saving one virtual call with an unverified spelling
is not worth it.

---

## The sixth build on a real machine: the C++ side compiled

All 98 TUs compiled, prelink ran, `pier.dll` linked, and it reached 90%.

It stopped here:

    error: fatal: Not a valid object name HEAD

That is neither a compile nor a link error, it is a git one. The modpacker of
`levibuildscript` reads git for a version number at the packaging stage, and the working
directory was a repository with no commit, or not a repository at all.

### The shape of this error is worth recording

It appears at the last step of a ten-minute build while what it stops takes a second to
check. `tools/build-prereqs.py` was added and runs first inside `run-surrogates`:

    git init && git add -A && git commit -m "pier v1"
        git tag v1.0.0        # optional; without it the version degrades to a commit hash

All three states were verified, not a repository, an empty repository and one with a
commit, and what it reports gives the command to type (contract §5.3: a log line has to
answer what to do about it).

### The ledger turned out to run in one direction only

While adding `LICENSE` it emerged that all three `Cargo.toml` files declare
`license = "Apache-2.0"` while the LICENSE file never came across. The ledger did not even
have that row, so counting it line by line a hundred times would not have found it.

> A checklist that is missing an item can never reveal that item.

The `ledger-covers-tree` check was added, covering the reverse direction, that every file
in the workspace has a row in the ledger. It caught 31 files at once that had never been
counted, all added or renamed by the new architecture with no counterpart in the old
repository, so a row-by-row migration never surfaced them. They are all in the ledger now
with their source stated.

The ledger runs in both directions from now on: `ledger-count` covers ledger to count and
`ledger-covers-tree` covers workspace to ledger.

---

## The seventh: the build passed and the load was refused

`pier.dll` was packaged, put into `mods/` and the server started:

        ERROR [LeviLamina] Pier could not be loaded
        ERROR [LeviLamina] Pier will not be loaded because it does not use the unified memory allocation operators.

`packages/pier-host/src/MemoryOperators.cpp` was never written. It is 5 lines.

### The ledger was right and the delivery note was wrong

That row sat outstanding in the ledger the whole time while the delivery note claimed the
C++ side was complete three rounds running.

The data was in the table the whole time and the counting script reported a total only,
while the C++ side being complete is a per-area assertion, which a count reporting only a
total cannot stop. It went unchallenged all three times.

The fix has `ledger-count` print a per-area table of the outstanding rows on every run and
warn outright, whenever the eight C++ packages still have one, that a delivery note must
not claim the C++ side is complete. It was verified to fire when `MemoryOperators.cpp` is
set back to outstanding.

Two other rows that had sat outstanding were cleared along the way; they should have been
cut long before. `pier-api/src/core/ApiTable.cpp` and `internal_api.h` were the hardcoded
forwarding table of the old version and its declaration face. The new architecture has each
capability package fill its own slots through `spi::SlotPack` (contract §1 rule 2), the
table no longer exists, and nobody went back to change the status.

The C++ side really has nothing outstanding now, and that sentence can be checked.

### The `host-loadable` check was added

Three criteria, taken one by one from the official LeviLamina template:

1. exactly one TU defines `LL_MEMORY_OPERATORS` and includes that header;
2. exactly one `LL_REGISTER_MOD(...)`;
3. the package holding those two TUs is `set_kind("object")` and is in the unconditional
   include list of the root xmake.

The third is the valuable half: whether the files exist is visible to the eye while whether
they were actually linked in is not. Those two TUs have no external symbol reference, a
static library discards them whole, and the symptom afterwards is identical to the files not
existing at all. All three were verified to fire.

### The shape of this family

`manifest-matches-host`, where a wrong type means the mod is never scanned with nothing
reported; `build-prereqs`, where no git commit means an error at 90% that looks nothing
like a build error; and `host-loadable`, where missing memory operators means the build
passes and the load is refused.

What they share: entirely invisible at compile time, reported at load time, and what is
reported has nothing to do with the build. All three have a script guarding them now.

---

## Calibrating against a real consumer

That consumer is a 15880-line cross-version protocol adaptation mod, originally written
against the old loader. Migrating it to Pier changed three files; the other two crates,
`bedrock-codec` and `bedrock-protocol`, 15749 lines together, were untouched, because they
know no host.

The value of the migration is not the migration, it is that it decided which of the 85
outstanding rows to write first.

### The three places the migration changed

| File | What changed |
|---|---|
| `crates/crossbind-mod/src/lib.rs` | `ctx.server()` became `ctx.host()`; `HookDirection` split into `Direction` and `Directions` |
| `crates/crossbind-mod/Cargo.toml` | the dependency moved from an archived loader, whose path now points at a directory that does not exist, to `pier-rs`; all three features were deleted |
| `manifest.json` | `"type": "rust"` became `"pier"`; the dependency became `pier`; `abi_version` went from 5 to 1 |

It also emerged that the consumer had two manifests, one at the root and one under the
crate, whose version numbers disagreed, 0.1.2 against 0.1.0. Only the one at the root, used
for releases, was kept.

### `pier-rs/src/packet.rs` was newly written

One API design change is worth recording: the old SDK had a single `HookDirection` enum
serve both which direction a packet has and which directions a registration wants, so
`Both` was a value that made no sense in the first position while still having to be
handled in a `match`, which could only be written as `_ =>` and swallowed the real
omissions with it. Split into `Direction` with two values and `Directions` with three, that
match became exhaustive.

The closure bound tightened from `Send` to `Send + Sync` as well: the ABI states plainly
that an inbound callback runs on the thread pumping the connection and an outbound one on
the thread that started the send, so the same closure may be entered by several threads at
once. The old signature made that invisible.

### That consumer exposed all four defects of the Rust surrogate

Its code style is broader than Pier's own, so it reported 12 findings at once, every one of
them a false positive:

| Defect | Root cause |
|---|---|
| unbalanced brackets, in four files | the `r` of the raw-string regex had no word boundary, so a fragment spanning two strings was swallowed whole as one raw string. The stripper itself broke the balance: 123 against 123 in the original and 98 against 100 after stripping |
| the same | the character literal regex took the lifetime in `Formatter<'_>) -> Result {` for the start of a string |
| `signed` was reported as a C type | the criterion was the word appearing right of a colon, while `signed` was a variable name |
| `use ... Codec` was reported as unused | the only purpose of a use for a trait is often to make `x.method()` resolve, and the trait name never appears |
| `pub const V: &[T] = &[...]` was reported as missing a semicolon | it looked for `{` and not `[`, and after looking for `[` it hit the `&[T]` of the type annotation, so the search has to start after the `=` |

The five root causes all differ while the shape is still that one: what the criterion looks
at is not the same thing as what it means to assert.

Running a check against a real project written in a different style is the only way to
calibrate it: running it on your own code tests whether it agrees with your habits and not
whether it is right.

## Filling in the synthetic events, against the LeviLamina event registry and the
## iListenAttentively event table

The two event lists were compared and what Pier lacked, and would really be used in a
protection or economy setting, was filled in. Each is pinned to an engine symbol whose
signature was checked, and cancelling always uses the engine's own failure path, returning
false, nullptr or NotPossibleHere, so no half-updated state is created.

| File | Event |
|---|---|
| `packages/pier-hooks/src/world/BlockDestroyEvent.cpp` | `BlockDestroyEvent`, hooking `Level::$destroyBlock`, covering non-player destruction by endermen, withers, explosions and commands |
| `packages/pier-hooks/src/world/ExplosionEvent.cpp` | `ExplosionEvent`, hooking `Level::$explode` at High priority outside the dimension rules, carrying the source actor and the radius |
| `packages/pier-hooks/src/world/LiquidFlowEvent.cpp` | `LiquidFlowEvent`, hooking `LiquidBlock::_trySpreadTo`, blocking water flowing onto a neighbor's plot |
| `packages/pier-hooks/src/world/FarmlandDecayEvent.cpp` | `FarmlandDecayEvent`, hooking `FarmBlock::$transformOnFall`, blocking farmland from being trampled |
| `packages/pier-hooks/src/world/PistonPushEvent.cpp` | `PistonPushEvent`, hooking `PistonBlockActor::_checkAttachedBlocks`, carrying the list of attached blocks |
| `packages/pier-hooks/src/world/ChestPairEvent.cpp` | `ChestPairEvent`, hooking `ChestBlockActor::_tryToPairWith`, blocking a double chest across a plot boundary |
| `packages/pier-hooks/src/world/SpawnItemActorEvent.cpp` | `SpawnItemActorEvent`, hooking `Spawner::$spawnItem`, for anti-duplication and drop ownership |
| `packages/pier-hooks/src/world/WeatherChangeEvent.cpp` | `WeatherChangeEvent`, hooking `Level::$updateWeather`, observation only |
| `packages/pier-hooks/src/player/SleepEvent.cpp` | `PlayerSleepEvent`, hooking `Player::$startSleepInBed`, cancelling through `BedSleepingResult::NotPossibleHere` |
| `packages/pier-hooks/src/player/SlotChangeEvent.cpp` | `PlayerChangeSlotEvent`, hooking `Player::setSelectedSlot`, observation only |
| `packages/pier-hooks/src/player/EatEvent.cpp` | `PlayerUseItemCompleteEvent`, hooking `Player::completeUsingItem`, observation only |
| `packages/pier-hooks/src/protect/ArmorStandEvent.cpp` | `ArmorStandSwapItemEvent`, hooking `ArmorStand::_trySwapItem`, protecting the equipment on an armor stand |
| `packages/pier-hooks/src/protect/ItemFrameEvent.cpp` | `PlayerAttackItemFrameEvent`, hooking `ItemFrameBlock::$attack`, protecting the item in an item frame |
| `packages/pier-hooks/src/protect/RideEvent.cpp` | `ActorRideEvent` appended, for a non-player passenger, sharing the detour with `PlayerRideEvent` |
| `packages/pier-hooks/src/protect/PressurePlateEvent.cpp` | `ActorStepOnPressurePlateEvent` appended, for a non-player actor, with its own throttle table |

## Filling in the Rust SDK domain wrappers, batch 1: the NBT foundation

The safe wrapper of the earlier generation covered 188 of the 190 slots. Slot coverage was
not the problem, the feel of using it was. To read one field out of an event payload,
business code wrote 695 lines of glue for payload, hit, wiring and dispatch, full of
`unwrap_or(0)`, collapsing a missing key and a value of 0 into the same answer, which is
exactly the shape of the land protection bypass contract §5.1 records.

This batch therefore lays the foundation first: every piece of structured data crossing the
boundary is SNBT, and it decides how reading a value feels.

| File | Contents |
|---|---|
| `bindings/rust/pier-rs/src/nbt/mod.rs` | the `NbtValue` value type; two families of accessors, `opt_*` returning an Option where the caller handles the rest, and `get_*` returning a Result where a missing key and a type mismatch are different errors carrying the key name; array index paths such as `a.b[2].c`; `as_vec3` and `as_block_pos`; `From` construction helpers; a two-way `serde_json` bridge |
| `bindings/rust/pier-rs/src/nbt/parse.rs` | SNBT parsing: the type suffixes `b`, `s`, `L`, `f` and `d`, bare keys and bare strings, `true` and `false`, the typed arrays `[B;...]`, `[I;...]` and `[L;...]`, `\uXXXX` escapes and multi-byte UTF-8; an error carries a byte offset |
| `bindings/rust/pier-rs/src/nbt/write.rs` | SNBT writing: no type suffix omitted, every key quoted, a control character written as `\uXXXX`, and a non-finite float landing as 0 |

## Filling in the Rust SDK domain wrappers, batch 2: events and services

This batch pulls those 695 lines of business-side glue into the SDK and, following the
problems it exposed, redid how a value is read.

| File | Contents |
|---|---|
| `bindings/rust/pier-rs/src/event/mod.rs` | `Event`, with typed access, a `dim()` aware of `_unresolved`, a `player()` unifying three shapes, and a differential `edit` and `cancel`; an RAII `Listener` that does not fail silently on unsubscribe; and the `Wiring` batch subscription builder, where `arm()` withdraws everything on any failure and `arm_lenient()` does not |
| `bindings/rust/pier-rs/src/event/names.rs` | the event id constants: the LL registry events plus all 29 synthetic events, each stating whether it can be cancelled and what its payload fields are; `ALL_SYNTHETIC` makes a startup self-check easy |
| `bindings/rust/pier-rs/src/service.rs` | `call`, `call_json::<T>`, `call_with` and `call_optional`; the `CallError` categories NotFound, Provider, Refused, Decode and Unavailable; `register` and `register_json`; and `exists` really parsing now, where an earlier generation substring-matched JSON text |
| `bindings/rust/pier-rs/src/sel.rs` | the `PlayerSel` enum replaces a bare `kind: i32`, and the fact that `Name` goes through the display-name fallback and cannot serve as an identity is written into the type layer through `is_stable()` |
