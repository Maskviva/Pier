# 🌍 世界与方块 API

时间、天气、游戏规则，以及世界里的每一个方块。

### 世界

#### 获取当前时间

Rust：`World::get().time()`  
Go：`levilamina.Time()`  
Zig：`levilamina.slot("get_time")`  

- 返回值：当天的时间，以游戏刻计算：一天 24000 刻，0 是日出，6000 是正午，18000 是午夜
- 返回值类型：Rust `Result<i64>`，Go `(int64, error)`
- 对应槽位：`get_time`
- 示例：
    - Rust

      ```rust title="Rust"
      let now = World::get().time()?;
      ```

    - Go

      ```go title="Go"
      now, err := levilamina.Time()
      ```

#### 设置时间

Rust：`World::get().set_time(t)`  
Go：`levilamina.SetTime(t)`  
Zig：`levilamina.slot("set_time")`  

- 参数：
    - t : 整数  
      游戏刻
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `error`，Zig `bool`
- 对应槽位：`set_time`
- 示例：
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

#### 设置天气

Rust：`World::get().set_weather(weather)`  
Go：`levilamina.SetWeather(weather)`  
Zig：`levilamina.slot("set_weather")`  

- 参数：
    - weather : Rust `Weather`，Go、Zig 整数  
      晴、雨、雷暴（0、1、2）
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `error`
- 对应槽位：`set_weather`
- 示例：
    - Rust

      ```rust title="Rust"
      World::get().set_weather(levilamina::Weather::Rain)?;
      ```

    - Go

      ```go title="Go"
      err := levilamina.SetWeather(1)
      ```

#### 读写游戏规则

Rust：`World::get().game_rule(name)`  
Go：`levilamina.GameRule(name)`  
Zig：`levilamina.slot("game_rule_get")`  

写入：Rust `set_game_rule(name, value)`，Go `SetGameRule(name, value)`，槽位 `game_rule_set`。值用文本传入，宿主按规则的类型去解析。

- 参数：
    - name : 字符串  
      规则名，和 `/gamerule` 里的一样
- 返回值：规则的值
- 返回值类型：Rust `Result<GameRuleValue>`，Go `(string, error)`
- 对应槽位：`game_rule_get`, `game_rule_set`
- 示例：
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

### 方块

方块用「维度 + 坐标」来指定：Rust `Block::at(dim, x, y, z)`，Go `levilamina.Block(dim, x, y, z)`，Zig `levilamina.BlockAt.at(dim, x, y, z)`。以下用 `block` 代表它。

#### 读取方块

Rust：`block.read()`  
Go：`block.Info()`  
Zig：`block.typeName(allocator)`  

- 返回值：方块的类型名，比如 `minecraft:oak_log`，以及它的方块状态
- 返回值类型：Rust `Result<BlockInfo>`，Go `(BlockInfo, error)`，Zig `levilamina.Error![]u8`
    - 世界还没加载好时，读取会返回错误。
- 对应槽位：`get_block`
- 示例：
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

#### 放置方块

Rust：`block.set(spec)`  
Go：`block.Set(spec)`  
Zig：`levilamina.slot("set_block")`  

- 参数：
    - spec : 字符串  
      方块，写法和 `/setblock` 一样，可以带方块状态，比如 `minecraft:oak_log["pillar_axis"="x"]`
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `error`，Zig `bool`
- 对应槽位：`set_block`
- 示例：
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

#### 读写方块状态

Rust：`block.state(name)`  
Go：`block.State(name)`  
Zig：`levilamina.slot("block_get_state")`  

写入：Rust `set_state(name, value)`，Go `SetState(name, value)`，槽位 `block_set_state`。

- 参数：
    - name : 字符串  
      状态名，比如原木的 `pillar_axis`
- 返回值：状态的值
- 返回值类型：Rust `Result<String>`，Go `(string, error)`
- 对应槽位：`block_get_state`, `block_set_state`
- 示例：
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

!!! warning "含水方块是两个方块"

    泡在水里的台阶、栅栏，在游戏里是「台阶」加上一个「水」叠在一起。读到的是主方块；水在额外方块那一层。
