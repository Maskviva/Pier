# Go

Pier's Go binding writes a mod as a Go package built into a DLL. You import
`github.com/Maskviva/pier/bindings/go/levilamina` and write `levilamina.Subscribe(...)`; the
smallest working mod is `examples/hello-pier-go`.

The path names Pier, the ABI the package belongs to, and the package name names what you are
writing, a LeviLamina mod. The Rust binding does the same: its package is `pier-rs` and its
crate `levilamina`.

## How it is put together

**cgo reads `abi.h` itself.** The binding does not mirror the function table by hand: cgo
includes the header, so the C compiler lays out `PierApi` exactly as the host does. The
package carries a copy of the header, because `go get` fetches nothing outside the module;
a check keeps the copy identical to the original.

**Every slot is reached through a generated C function.** Go cannot call a C function
pointer, so `slots_gen.h` has, for each slot, a `piergo_call_<slot>` that forwards the call
and a `piergo_has_<slot>` that applies both gates of the contract: the table is long enough
to hold the slot, and the slot is not NULL. `tools/gen-go-slots.py` writes it from `abi.h`,
and the `go-binding` check fails when it is stale.

**The host calls exported Go functions.** `pier_main`, the lifecycle callbacks, and the
callbacks of tasks, events and commands are `//export`ed, and a small C file hands them to
the host as the function pointers it expects. A panic in any of them is recovered at the
boundary and logged; nothing unwinds into the host.

## What you need

- Go 1.21 or later.
- A MinGW-w64 `gcc` on `PATH`, which cgo builds the C side with. The DLL meets Pier through
  C functions only, which MinGW and MSVC call the same way on x64, so the MSVC requirement
  of a C++ mod does not apply.
- `CGO_ENABLED=1`, and `go build -buildmode=c-shared`.

Build in a way that leaves the DLL depending on system DLLs only; a DLL that needs a MinGW
runtime DLL the server does not have fails to load with error 0x7E.

## The API

The API comes in three layers, all gated the same way:

- **`levilamina.Raw`**: one typed method per slot without a callback, 173 of them, generated
  from `abi.h` with its documentation. A result means what `abi.h` says it means.
- **Generated facades**: one method per property and verb of the constant tables, on
  `Player`, `Entity`, `BlockAt` and `Item`, such as `Player.Level`, `Player.SetLevel`,
  `Entity.IsOnFire` or `Item.Count`, and a Go constant for each value, such as `PPropLevel`.
- **Hand-written functions** over both, close to the Rust binding's:

| Area | Functions |
|---|---|
| Lifecycle and logging | `Register`, `Mod`, `Loader`, `Unloader`, `Context.Logger()` |
| Server and world | `Status`, `CurrentTick`, `Tps`, `Mspt`, `Time`, `SetWeather`, `GameRule`, `ExecuteCommand`, `ListPlayers`, `ListActors`, `SpawnMob`, `Explode`, `FillRegion`, `SaveLevel` |
| Tasks | `Schedule`, `ScheduleAfter`, `Cancel`, `PendingTasks` |
| Events | `Subscribe`, `Event.Payload`, `Event.Dim`, `Event.CheckComplete`, `Event.Set`, `Event.Cancel` |
| Commands and forms | `RegisterCommand`, `NewCommand` with typed overloads, `RegisterCommandEnum`, `RegisterSoftEnum`, `SendForm` |
| Players and entities | `Player.SendMessage`, `Teleport`, `Inventory`, `CarriedItem`, `Entity.Snapshot`, `Owner`, `Target`, and the generated properties |
| Blocks, items, containers | `BlockAt.Info`, `State`, `SetState`, `Item.Payload`, `Container.Items`, `AddItem`, `Clear` |
| Data | `OpenKvDb`, `ParseSNBT`, `Nbt`, `SnbtToBinary` |
| Economy | `Money`, `AddMoney`, `TransferMoney`, `OnMoneyBefore`, `OnMoneyAfter` |
| Cross-mod | `BusPublish`, `BusPublishVetoable`, `BusSubscribe`, `RegisterService`, `CallService`, `ServiceCaller`; see [Talking to other mods](cross-mod.md) |
| Network | `RegisterPacketHook`, `RegisterPacketHookIDs`, `RegisterConnHook` |
| Regions | `ScanRegion`, `ScanRegionIndexed`, `SetBlocks` |
| Dimensions and simulated players | `AddDimension`, `AddGeneratedDimension`, `DimensionRule`, `SimSpawn`, `SimDo` |
| Client | `RegisterKey` |

A call whose slot the host does not have returns a `*NotProvidedError`, which
`IsNotProvided` recognizes, and a read the host cannot answer is an error rather than a zero
or a false. Every slot of the table is reachable from Go except the two of lanes, which by design only
pair mods built by the same toolchain; a mod that publishes a lane also provides the service
a Go mod uses instead.

## Rules worth knowing

- **Threads.** Most of the host is server-thread only. Lifecycle steps, event and command
  callbacks already run there; a goroutine gets back onto it with `Schedule`.
- **Tasks belong to the mod.** A task that has not run when the mod unloads is dropped by
  the host, so nothing calls into a DLL that is gone.
- **Payloads are copies.** `Event.SNBT` and `Invocation.Args` are Go strings, valid after
  the callback returns. The fields of a payload are in the
  [event payload reference](../guide/event-payloads.md).
- **One runtime per DLL.** Every Go mod carries a whole Go runtime. Several Go mods on one
  server are several runtimes in one process, which Go does not test; check that it holds on
  your server before relying on it.

[Your first Go mod](first-mod.md) walks through building and installing one, and
[Talking to other mods](cross-mod.md) shows how a Go mod works with mods of other languages.
