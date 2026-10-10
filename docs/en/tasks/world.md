# 🌍 World and blocks API

Time, weather, game rules, and every block of the world.

### The world

#### The time of day

Rust: `World::get().time()`  
Go: `levilamina.Time()`  
Zig: `levilamina.slot("get_time")`  

- Return value: the time of day in game ticks: 24000 to a day, 0 sunrise, 6000 noon, 18000 midnight
- Return type: Rust `Result<i64>`, Go `(int64, error)`
- Slot: `get_time`
- Example:
    - Rust

      ```rust title="Rust"
      let now = World::get().time()?;
      ```

    - Go

      ```go title="Go"
      now, err := levilamina.Time()
      ```

#### Setting the time

Rust: `World::get().set_time(t)`  
Go: `levilamina.SetTime(t)`  
Zig: `levilamina.slot("set_time")`  

- Parameters:
    - t : integer  
      game ticks
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `error`, Zig `bool`
- Slot: `set_time`
- Example:
    - Rust

      ```rust title="Rust"
      World::get().set_time(6000)?;
      ```

    - Go

      ```go title="Go"
      err := levilamina.SetTime(6000)
      ```

    - Zig

      ```zig title="Zig"
      const set_time = levilamina.slot("set_time") orelse return error.NotProvided;
      if (!set_time(6000)) return error.Refused;
      ```

#### Setting the weather

Rust: `World::get().set_weather(weather)`  
Go: `levilamina.SetWeather(weather)`  
Zig: `levilamina.slot("set_weather")`  

- Parameters:
    - weather : Rust `Weather`, Go and Zig an integer  
      clear, rain, thunder (0, 1, 2)
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `error`
- Slot: `set_weather`
- Example:
    - Rust

      ```rust title="Rust"
      World::get().set_weather(levilamina::Weather::Rain)?;
      ```

    - Go

      ```go title="Go"
      err := levilamina.SetWeather(1)
      ```

#### Game rules

Rust: `World::get().game_rule(name)`  
Go: `levilamina.GameRule(name)`  
Zig: `levilamina.slot("game_rule_get")`  

Writing: Rust `set_game_rule(name, value)`, Go `SetGameRule(name, value)`, slot `game_rule_set`. A value goes in as text, read according to the rule's type.

- Parameters:
    - name : string  
      the rule name, as `/gamerule` takes it
- Return value: the rule's value
- Return type: Rust `Result<GameRuleValue>`, Go `(string, error)`
- Slot: `game_rule_get`, `game_rule_set`
- Example:
    - Rust

      ```rust title="Rust"
      let keep = World::get().game_rule("keepInventory")?;
      World::get().set_game_rule("keepInventory", "true")?;
      ```

    - Go

      ```go title="Go"
      keep, err := levilamina.GameRule("keepInventory")
      err = levilamina.SetGameRule("keepInventory", "true")
      ```

### Blocks

A block is named by its dimension and position: Rust `Block::at(dim, x, y, z)`, Go `levilamina.Block(dim, x, y, z)`, Zig `levilamina.BlockAt.at(dim, x, y, z)`. `block` stands for it below.

#### Reading a block

Rust: `block.read()`  
Go: `block.Info()`  
Zig: `block.typeName(allocator)`  

- Return value: the block's type name, such as `minecraft:oak_log`, and its block state
- Return type: Rust `Result<BlockInfo>`, Go `(BlockInfo, error)`, Zig `levilamina.Error![]u8`
    - Before the level has loaded, a read returns an error.
- Slot: `get_block`
- Example:
    - Rust

      ```rust title="Rust"
      let info = Block::at(0, 10, 64, -5).read()?;
      ```

    - Go

      ```go title="Go"
      info, err := levilamina.Block(0, 10, 64, -5).Info()
      ```

    - Zig

      ```zig title="Zig"
      const name = try levilamina.BlockAt.at(0, 10, 64, -5).typeName(allocator);
      defer allocator.free(name);
      ```

#### Placing a block

Rust: `block.set(spec)`  
Go: `block.Set(spec)`  
Zig: `levilamina.slot("set_block")`  

- Parameters:
    - spec : string  
      the block, as `/setblock` takes it, block states included, such as `minecraft:oak_log["pillar_axis"="x"]`
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `error`, Zig `bool`
- Slot: `set_block`
- Example:
    - Rust

      ```rust title="Rust"
      Block::at(0, 10, 64, -5).set("minecraft:stone")?;
      ```

    - Go

      ```go title="Go"
      err := levilamina.Block(0, 10, 64, -5).Set("minecraft:stone")
      ```

    - Zig

      ```zig title="Zig"
      const set_block = levilamina.slot("set_block") orelse return error.NotProvided;
      if (!set_block(0, 10, 64, -5, levilamina.str("minecraft:stone"))) return error.Refused;
      ```

#### Block states

Rust: `block.state(name)`  
Go: `block.State(name)`  
Zig: `levilamina.slot("block_get_state")`  

Writing: Rust `set_state(name, value)`, Go `SetState(name, value)`, slot `block_set_state`.

- Parameters:
    - name : string  
      the state name, such as a log's `pillar_axis`
- Return value: the state's value
- Return type: Rust `Result<String>`, Go `(string, error)`
- Slot: `block_get_state`, `block_set_state`
- Example:
    - Rust

      ```rust title="Rust"
      let axis = Block::at(0, 10, 64, -5).state("pillar_axis")?;
      Block::at(0, 10, 64, -5).set_state("pillar_axis", "x")?;
      ```

    - Go

      ```go title="Go"
      axis, err := levilamina.Block(0, 10, 64, -5).State("pillar_axis")
      err = levilamina.Block(0, 10, 64, -5).SetState("pillar_axis", "x")
      ```

!!! warning "A waterlogged block is two blocks"

    A slab or fence standing in water is the slab with water layered on top. A read gives the main block; the water is in the extra block layer.
