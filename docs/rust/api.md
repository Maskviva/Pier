# API overview

The Rust SDK is documented in full by rustdoc, generated from the same comments that
govern the implementation:

```bash
cargo doc --open -p pier-rs
```

This page is the map. Each entry says what the module is for and what to watch out for;
the details are in rustdoc.

## The mod itself

| | |
|---|---|
| `LeviMod` | The trait your mod implements. Only `on_load` is required. |
| `ModContext` | What a lifecycle callback receives: `logger()`, `host()`, `world()`, `packets()`, `server()`. |
| `register_mod!` | Generates the entry point. Without it the mod is refused at load. |
| `Logger` | Logs under your mod's name. Thread safe and `Copy`. |

## The world

| | |
|---|---|
| `World` | Time, weather, difficulty, game rules, biomes, chunk save keys, villages, structures, region scans. |
| `Block` | One cell, addressed by dimension and coordinate. Reads, writes, block states, block entities, the liquid layer. |
| `Server` | Tick freezing, stepping and warping, plus per-subsystem profiling. |

::: warning A waterlogged block is two blocks
Waterlogging in Bedrock is not a block state, it is a second block in the same cell.
`Block::name` sees only the main layer, so copying a waterlogged stair loses the water
unless you also move `Block::extra`.
:::

## Players and actors

| | |
|---|---|
| `Player` | Addressed by a `PlayerSel`, resolved again on every call. |
| `PlayerSel` | By name, XUID or UUID. `is_stable()` says whether it is an identity. |
| `Entity` | Any actor by `ActorUniqueID`, players included. `Player::as_entity()` crosses over. |
| `Container` | The four on a player plus one at a coordinate. |
| `ItemStack` | A value object, not a handle. |

::: danger Only an xuid is an identity
`PlayerSel::Name` falls back to the display name, which another mod can change. A player
who sets theirs to the account name of an offline player redirects every by-name call
onto themselves. Permissions, economy and ownership use `Player::by_xuid`.
:::

::: warning An ItemStack is a snapshot
`container.item(0)` returns a copy. Changing it does not change the container; write it
back with `container.set_item(0, &stack)` and then `container.refresh()`.
:::

## Events and commands

| | |
|---|---|
| `event` | `subscribe`, `subscribe_with`, and `Wiring` for batches. |
| `event::names` | The id constants. Use these, never a literal. |
| `Event` | Typed payload access, `dim()`, `player()`, `pos()`, `edit()`, `cancel()`. |
| `command` | `register` for raw text, `builder` for typed overloads. |

## Cross-mod

| | |
|---|---|
| `service` | One to one, with a reply. Names are exclusive. |
| `bus` | One to many, no reply. You never receive your own publish. |
| `lane` | One to one, native calls, same toolchain only. |

## Everything else

| | |
|---|---|
| `gui` | The three form kinds. A callback runs at most once and may never run. |
| `scoreboard` | Objectives, scores, the sidebar. |
| `money` | The economy bridge. Degrades to failure values without a backend. |
| `kvdb` | A key-value store scoped to your mod's data directory. Thread safe. |
| `packet` | Raw packet interception. **Not on the server thread.** |
| `dimensions` | Custom dimensions, plot grids, per-dimension rules. Optional package. |
| `client` | Client-only capabilities. Empty slots on a server host. |
| `sim` | Simulated players. |
| `nbt` | `NbtValue`, SNBT parsing and writing, the binary bridge. |

## Two habits worth forming

**Handle the `Err`.** Everything that can fail to answer returns one, and it says why.
Reaching for `unwrap_or` puts back exactly the bug this SDK exists to prevent.

**Hold the handle.** `Listener`, `Registration`, `Subscription` and `PacketHook` all
release on drop. Dropping one immediately looks the same in code as forgetting to keep
it, so say `.forget()` when that is what you mean.
