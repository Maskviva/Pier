# 🏃 Player API

In Pier, a "player object" finds one player and acts on them or reads about them.

### Getting a player object

#### From an event

Subscribe to an **event** and take the player from its payload: the `_player` field holds their name, XUID and UUID. See [Events](events.md) and the [event payload reference](../guide/event-payloads.md).

#### By name, XUID or UUID

`Player::by_name(name)`  
`levilamina.PlayerByName(name)`  
`levilamina.Player.byName(name)`  

Builds a player object from what you know of the player. For an XUID or UUID, Rust has `by_xuid` and `by_uuid`, Go `PlayerByXuid` and `PlayerByUuid`, Zig `byXuid` and `byUuid`.

- Parameters:
    - name : string  
      the player's name (or XUID, UUID)
- Return value: a player object
- Return type: `Player`
    - Creating the object does not check that the player is online; the calls do. To check first, see "Is the player online" below.
- Example:
    - Rust

      ```rust title="Rust"
      let steve = Player::by_name("Steve");
      let same = Player::by_xuid("2535412345678901");
      ```

    - Go

      ```go title="Go"
      steve := levilamina.PlayerByName("Steve")
      same := levilamina.PlayerByXuid("2535412345678901")
      ```

    - Zig

      ```zig title="Zig"
      const steve = levilamina.Player.byName("Steve");
      const same = levilamina.Player.byXuid("2535412345678901");
      ```

!!! danger "Identify players by XUID"

    After a player renames, someone else can register the old name. If **permissions, money or land ownership** are decided by name, whoever takes the name is treated as the player who had it. Decide those by XUID.

#### Every player online

`Player::list()`  
`levilamina.ListPlayers()`  
`levilamina.slot("list_players")`  

Returns every player on the server, each with name, XUID, UUID, and dimension and position.

- Return value: the list of players online
- Return type: Rust `Vec<PlayerInfo>`, Go `[]PlayerInfo`, Zig one SNBT per player
- Slot: `list_players`
- Example:
    - Rust

      ```rust title="Rust"
      for p in Player::list() {
          Logger::get().info(&p.name);
      }
      ```

    - Go

      ```go title="Go"
      players, err := levilamina.ListPlayers()
      for _, p := range players {
      	levilamina.Logger{}.Info(p.Name)
      }
      ```

    - Zig

      ```zig title="Zig"
      const list_fn = levilamina.slot("list_players") orelse return error.NotProvided;
      var col = levilamina.Collector.init(allocator);
      defer col.deinit();
      list_fn(&col, &levilamina.Collector.sink);
      ```

!!! note "Note: don't keep player information around"

    A player object holds only a name, XUID or UUID, and Pier looks the player up again on every call, so keeping the object is fine. The position and dimension a call such as `Player::list()` returns are as they were at that moment, and go stale as the player moves; read them again when you need them.

### Player object: properties

Level, experience, hunger, game mode… these are the player's "properties". In Go and Zig every `PIER_PPROP_*` (number) and `PIER_PSTR_*` (text) of `abi.h` has a method of its name, and a writable one a `Set…`. The common ones:

| Property | Rust | Go | Zig | Type |
|---|---|---|---|---|
| Level | `num(PIER_PPROP_LEVEL)`, `set_level(n)` | `Level()`, `SetLevel(n)` | `level()`, `setLevel(n)` | number, writable |
| Experience progress | `num(PIER_PPROP_EXPERIENCE)`, `set_experience(p)` | `Experience()`, `SetExperience(p)` | `experience()`, `setExperience(p)` | 0 to 1, writable |
| Game mode | `game_type()` | `GameType()` | `gameType()` | number |
| Real name | `text(PIER_PSTR_REAL_NAME)` | `RealName()` | `realName(allocator)` | text |

A read that fails returns an error. Slots: `player_get_num`, `player_set_num`, `player_get_str`.

### Player object: functions

Every player object has these functions; `steve` stands for the player object you hold.

#### Is the player online

`steve.is_online()`  
`steve.IsOnline()`  
`levilamina.slot("player_resolve")`  

- Return value: whether the player is online
- Return type: `bool`
    - Every ABI v2 host provides this slot, so `false` means nobody online matches.
- Slot: `player_resolve`
- Example:
    - Rust

      ```rust title="Rust"
      if steve.is_online() {
          // ...
      }
      ```

    - Go

      ```go title="Go"
      if steve.IsOnline() {
      	// ...
      }
      ```

    - Zig

      ```zig title="Zig"
      const resolve = levilamina.slot("player_resolve") orelse return error.NotProvided;
      var id: i64 = 0;
      const online = resolve(steve.cSel(), &id);
      ```

#### Sending a player a message

`steve.send_message(msg)`  
`steve.SendMessage(msg)`  
`levilamina.slot("player_send_message")`  

- Parameters:
    - msg : string  
      the text to send
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `error`, Zig `bool` (success)
- Slot: `player_send_message`
- Example:
    - Rust

      ```rust title="Rust"
      steve.send_message("Welcome back!")?;
      ```

    - Go

      ```go title="Go"
      err := steve.SendMessage("Welcome back!")
      ```

    - Zig

      ```zig title="Zig"
      const send = levilamina.slot("player_send_message") orelse return error.NotProvided;
      if (!send(steve.cSel(), levilamina.str("Welcome back!"))) return error.Refused;
      ```

#### Broadcasting a message to everyone

`Player::broadcast(msg)`  
`levilamina.Broadcast(msg)`  
`levilamina.slot("broadcast_message")`  

- Parameters:
    - msg : string  
      the text to broadcast
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `error`
- Slot: `broadcast_message`
- Example:
    - Rust

      ```rust title="Rust"
      Player::broadcast("The event starts at eight tonight")?;
      ```

    - Go

      ```go title="Go"
      err := levilamina.Broadcast("The event starts at eight tonight")
      ```

    - Zig

      ```zig title="Zig"
      const bc = levilamina.slot("broadcast_message") orelse return error.NotProvided;
      bc(levilamina.str("The event starts at eight tonight"));
      ```

#### Showing a title

`steve.set_title(text)`  
`steve.SendTitle(slot, text, fadeIn, stay, fadeOut)`  
`levilamina.slot("player_send_title")`  

Shows a line in the middle of the screen. Rust also has `set_subtitle`, `set_actionbar` and `clear_title`; Go and Zig choose the place with the first argument.

- Parameters:
    - slot : integer  
      (Go, Zig) 0 title, 1 subtitle, 2 action bar
    - text : string  
      the text to show
    - fadeIn, stay, fadeOut : integer  
      (Go, Zig) fade-in, stay and fade-out in game ticks
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `error`, Zig `bool`
- Slot: `player_send_title`
- Example:
    - Rust

      ```rust title="Rust"
      steve.set_title("Welcome to the city")?;
      steve.set_subtitle("Have fun")?;
      ```

    - Go

      ```go title="Go"
      err := steve.SendTitle(0, "Welcome to the city", 10, 70, 20)
      ```

    - Zig

      ```zig title="Zig"
      const title = levilamina.slot("player_send_title") orelse return error.NotProvided;
      _ = title(steve.cSel(), 0, levilamina.str("Welcome to the city"), 10, 70, 20);
      ```

#### Teleporting a player

`steve.teleport(dim, x, y, z)`  
`steve.Teleport(dim, x, y, z)`  
`levilamina.slot("player_teleport")`  

- Parameters:
    - dim : integer  
      the dimension: 0 overworld, 1 nether, 2 end; a custom dimension's id comes from the dimension API
    - x, y, z : number  
      the position
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `error`, Zig `bool`
- Slot: `player_teleport`
- Example:
    - Rust

      ```rust title="Rust"
      steve.teleport(0, 100.5, 64.0, -20.5)?;
      ```

    - Go

      ```go title="Go"
      err := steve.Teleport(0, 100.5, 64, -20.5)
      ```

    - Zig

      ```zig title="Zig"
      const tp = levilamina.slot("player_teleport") orelse return error.NotProvided;
      if (!tp(steve.cSel(), 0, 100.5, 64, -20.5)) return error.Refused;
      ```

#### Setting a player's level

`steve.set_level(level)`  
`steve.SetLevel(level)`  
`steve.setLevel(level)`  

- Parameters:
    - level : integer (a number in Go and Zig)  
      the new level
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `error`, Zig `levilamina.Error!void`
    - The level changes through the game's own level change, as when a player levels up, so the related display and events update.
- Slot: `player_set_num`
- Example:
    - Rust

      ```rust title="Rust"
      steve.set_level(30)?;
      ```

    - Go

      ```go title="Go"
      err := steve.SetLevel(30)
      ```

    - Zig

      ```zig title="Zig"
      try steve.setLevel(30);
      ```

#### Setting a player's game mode

`steve.set_gamemode(mode)`  
`steve.SetGameType(mode)`  
`levilamina.slot("player_set_gamemode")`  

- Parameters:
    - mode : Rust `GameMode`, Go and Zig an integer  
      0 survival, 1 creative, 2 adventure, 6 spectator
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `error`, Zig `bool`
- Slot: `player_set_gamemode`
- Example:
    - Rust

      ```rust title="Rust"
      steve.set_gamemode(GameMode::Creative)?;
      ```

    - Go

      ```go title="Go"
      err := steve.SetGameType(1)
      ```

    - Zig

      ```zig title="Zig"
      const set_mode = levilamina.slot("player_set_gamemode") orelse return error.NotProvided;
      _ = set_mode(steve.cSel(), 1);
      ```

#### Removing a player from the server

`steve.disconnect(reason)`  
`steve.Disconnect(reason)`  
`levilamina.slot("player_disconnect")`  

- Parameters:
    - reason : string  
      the reason the player sees
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `error`, Zig `bool`
- Slot: `player_disconnect`
- Example:
    - Rust

      ```rust title="Rust"
      steve.disconnect("You were removed from the server")?;
      ```

    - Go

      ```go title="Go"
      err := steve.Disconnect("You were removed from the server")
      ```

    - Zig

      ```zig title="Zig"
      const kick = levilamina.slot("player_disconnect") orelse return error.NotProvided;
      _ = kick(steve.cSel(), levilamina.str("You were removed from the server"));
      ```

#### The item in hand

`steve.carried_item()`  
`steve.CarriedItem()`  
`levilamina.slot("player_get_carried_item")`  

- Return value: the item in the player's main hand
- Return type: Rust `Result<ItemStack>`, Go `(Item, error)`, Zig the item's SNBT
    - The item is a snapshot of that moment; see [Items and containers](item.md).
- Slot: `player_get_carried_item`
- Example:
    - Rust

      ```rust title="Rust"
      let hand = steve.carried_item()?;
      ```

    - Go

      ```go title="Go"
      hand, err := steve.CarriedItem()
      ```

    - Zig

      ```zig title="Zig"
      const get_hand = levilamina.slot("player_get_carried_item") orelse return error.NotProvided;
      var col = levilamina.Collector.init(allocator);
      defer col.deinit();
      _ = get_hand(steve.cSel(), &col, &levilamina.Collector.sink);
      ```

#### The player's inventory

`steve.inventory()`  
`steve.Inventory()`  
`c.PierContainerRef{ .which = 0, ... }`  

The ender chest, armor and offhand: Rust has `ender_chest`, `armor` and `offhand_container`, Go `EnderChest`, `Armor` and `Offhand`.

- Return value: the inventory as a container; see [Items and containers](item.md)
- Return type: `Container`
- Example:
    - Rust

      ```rust title="Rust"
      let bag = steve.inventory();
      let first = bag.item(0)?;
      ```

    - Go

      ```go title="Go"
      bag := steve.Inventory()
      first, err := bag.Item(0)
      ```

#### Giving a player an item

`steve.give_item(&item)`  
`steve.GiveItem(item.SNBT, 0, 0, 0)`  
`steve.giveItem(allocator, snbt, 0, 0, 0)`  

- Parameters:
    - item : item  
      the item to give; see [Items and containers](item.md)
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `(string, error)`, Zig `levilamina.Error![]u8`
- Slot: `player_action`
- Example:
    - Rust

      ```rust title="Rust"
      let diamonds = ItemStack::create("minecraft:diamond", 3);
      steve.give_item(&diamonds)?;
      ```

    - Go

      ```go title="Go"
      diamonds := levilamina.ItemOf(`{Name:"minecraft:diamond",Count:3b}`)
      _, err := steve.GiveItem(diamonds.SNBT, 0, 0, 0)
      ```

    - Zig

      ```zig title="Zig"
      allocator.free(try steve.giveItem(allocator, "{Name:\"minecraft:diamond\",Count:3b}", 0, 0, 0));
      ```
