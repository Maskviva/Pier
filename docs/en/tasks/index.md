# 📚 Mod development overview

## ⛳ Before you start

Pier is a LeviLamina mod loader: with it installed, you write mods for a Bedrock server in
**Rust**, **Go**, **Zig** or **C++**, and mods of different languages can call each other.

This part goes by what you want to do: events, commands, players, entities, the world,
items… Each page picks the interfaces a task needs most and shows them in Rust, Go and Zig.
Every interface of each binding is in the [API reference](../api/index.md), listed binding by
binding. Before opening a page, spend a few minutes on this one; every page after it uses
what it describes.

## 💊 Data types

### Common data types

The three languages have different type systems. The same idea maps to these types:

| Idea | Rust | Go | Zig | Notes |
|---|---|---|---|---|
| String | `&str` (in), `String` (out) | `string` | `[]const u8` | UTF-8 text |
| Integer | `i32`, `i64` | `int32`, `int64` | `i32`, `i64` | |
| Number | `f64` | `float64` | `f64` | |
| Boolean | `bool` | `bool` | `bool` | |
| A result that can fail | `Result<T>` | `(T, error)` | `levilamina.Error!T` | A failure carries its reason |
| A value that may be absent | `Option<T>` | `(T, bool)` | `?T` | Absent is not an error |
| Callback | closure | `func` | comptime function | The server calls it at the right time |
| SNBT | `NbtValue` | `*levilamina.Nbt` | `levilamina.nbt.Value` | Event payloads, items and entity data are SNBT |

### Engine objects

Beside those, Pier has object types for the things of the game:

| Object | Rust | Go | Zig | Named by |
|---|---|---|---|---|
| Player | `Player` | `levilamina.Player` | `levilamina.Player` | name, XUID or UUID; see [Players](player.md) |
| Entity | `Entity` | `levilamina.Entity` | `levilamina.Entity` | unique id; see [Entities](entity.md) |
| Block | `Block` | `levilamina.BlockAt` | `levilamina.BlockAt` | dimension and position; see [World and blocks](world.md) |
| Item | `ItemStack` | `levilamina.Item` | `levilamina.Item` | SNBT; see [Items and containers](item.md) |
| Container | `Container` | `levilamina.Container` | `c.PierContainerRef` | a player's inventory, or a block container |

## 📌 How an entry reads

Every API is written up the same way:

- **Title**: what it does.
- **Call form**: three lines, Rust, Go and Zig. Names such as `player` or `entity` stand for
  the object you hold.
- **Parameters**: each one's name, type and meaning. One marked optional can be left out.
- **Return value** and **return type**: what a successful call gives you.
- **Slot**: its name in the function table of `abi.h`. C++ calls it by that name, Zig gets
  it with `levilamina.slot("name")`, and Go calls the method of that name under
  `levilamina.Raw`, so an API your language has no wrapper for yet is still reachable.
- **Example**: a piece of code in each language.

## 🔀 Threads

**Most APIs may only be called on the server thread.** Event callbacks, command callbacks
and lifecycle functions already run there; call from them directly.

On a thread of your own, or in a Go goroutine, hand the work back with
[scheduled tasks](scheduler.md).

## ⚠️ When a call fails

A failed call gives you an error saying which step failed and why. Three kinds come up:

- **The host does not have it**: Pier is too old, or the capability was not built in. Go
  gives a `*levilamina.NotProvidedError` and Zig `error.NotProvided`.
- **The host refused**: the player is offline, the position is invalid, and so on.
- **The host cannot read it**: a property it does not know, say. A player at level 0 gives
  you `0`; a level the host cannot read gives you this error.

## 📜 Pages

| Page | What it covers |
|---|---|
| [🔄 Lifecycle and logging](lifecycle.md) | How a mod is loaded, enabled, disabled and unloaded, and logging |
| [📣 Events](events.md) | Subscribing, reading the payload, cancelling |
| [⌨️ Commands](commands.md) | Commands, commands with parameters, running a command |
| [🏃 Players](player.md) | Finding a player, messages, teleporting, properties |
| [🐷 Entities](entity.md) | Finding entities, properties, verbs, spawning |
| [🌍 World and blocks](world.md) | Time, weather, game rules, reading and placing blocks |
| [🎒 Items and containers](item.md) | Item SNBT, chests and inventories |
| [⏰ Scheduled tasks](scheduler.md) | Running later, getting back to the server thread |
| [🔗 Talking to other mods](crossmod.md) | Services and the bus, with mods of any language |
| [💾 Data and NBT](data.md) | A key-value database, parsing SNBT |
| [💰 Economy](economy.md) | Balances, transfers, transaction listeners |
