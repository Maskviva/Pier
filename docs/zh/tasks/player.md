# 🏃 玩家对象 API

在 Pier 里，使用「玩家对象」来找到某一个玩家，并操作、获取他的相关信息。

### 获取一个玩家对象

#### 从事件获取

通过注册**事件监听**，从事件内容里拿到与事件有关的玩家。事件内容的 `_player` 字段写着玩家的名字、XUID 和 UUID，详见 [事件监听](events.md) 和 [事件载荷参考](../guide/event-payloads.md)。

#### 通过名字、XUID 或 UUID 获取

`Player::by_name(name)`  
`levilamina.PlayerByName(name)`  
`levilamina.Player.byName(name)`  

通过玩家信息手动生成玩家对象。按 XUID、UUID 获取时，Rust 用 `by_xuid`、`by_uuid`，Go 用 `PlayerByXuid`、`PlayerByUuid`，Zig 用 `byXuid`、`byUuid`。

- 参数：
    - name : 字符串  
      玩家的名字（或 XUID、UUID）
- 返回值：玩家对象
- 返回值类型：`Player`
    - 生成对象时不会检查玩家是否在线，真正调用功能时才会；想先确认，用下面的「判断玩家是否在线」。
- 示例：
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

!!! danger "认人请用 XUID"

    玩家改名以后，旧名字可以被另一个人注册。如果**权限、金钱、领地归属**是按名字判断的，新拿到这个名字的人就会被当成原来那个人。所以这类判断请用 XUID。

#### 获取所有在线玩家

`Player::list()`  
`levilamina.ListPlayers()`  
`levilamina.slot("list_players")`  

返回服务器上所有在线玩家，每一项写着名字、XUID、UUID，以及所在维度和坐标。

- 返回值：在线玩家的列表
- 返回值类型：Rust `Vec<PlayerInfo>`，Go `[]PlayerInfo`，Zig 每个玩家一段 SNBT
- 对应槽位：`list_players`
- 示例：
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

!!! note "注意：不要长期保存玩家信息"

    玩家对象只记着名字、XUID 或 UUID，每次调用时 Pier 都会重新查找这个玩家，所以玩家对象本身可以放心保存。但 `Player::list()` 这类调用返回的坐标、维度是调用那一刻的值，玩家走动以后就过时了，需要时请重新获取。

### 玩家对象 - 属性

等级、经验、饥饿值、游戏模式……这些都是玩家的「属性」。在 Go 和 Zig 里，`abi.h` 中的每个 `PIER_PPROP_*`（数值）和 `PIER_PSTR_*`（文本）都生成了一个同名方法，可写的属性还有对应的 `Set…`。常用的几个：

| 属性 | Rust | Go | Zig | 类型 |
|---|---|---|---|---|
| 等级 | `num(PIER_PPROP_LEVEL)`、`set_level(n)` | `Level()`、`SetLevel(n)` | `level()`、`setLevel(n)` | 数值，可写 |
| 经验进度 | `num(PIER_PPROP_EXPERIENCE)`、`set_experience(p)` | `Experience()`、`SetExperience(p)` | `experience()`、`setExperience(p)` | 0 到 1，可写 |
| 游戏模式 | `game_type()` | `GameType()` | `gameType()` | 数值 |
| 真实名字 | `text(PIER_PSTR_REAL_NAME)` | `RealName()` | `realName(allocator)` | 文本 |

读不出来时返回错误。对应槽位：`player_get_num`、`player_set_num`、`player_get_str`。

### 玩家对象 - 函数

每一个玩家对象都有下面这些函数。以下用 `steve` 代表你手里的玩家对象。

#### 判断玩家是否在线

`steve.is_online()`  
`steve.IsOnline()`  
`levilamina.slot("player_resolve")`  

- 返回值：玩家是否在线
- 返回值类型：`bool`
    - 每个 ABI v2 宿主都提供这个槽位，所以返回 `false` 表示没有在线的人对得上。
- 对应槽位：`player_resolve`
- 示例：
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

#### 发送一个文本消息给玩家

`steve.send_message(msg)`  
`steve.SendMessage(msg)`  
`levilamina.slot("player_send_message")`  

- 参数：
    - msg : 字符串  
      要发送的文本
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `error`，Zig `bool`（是否成功）
- 对应槽位：`player_send_message`
- 示例：
    - Rust

      ```rust title="Rust"
      steve.send_message("欢迎回来！")?;
      ```

    - Go

      ```go title="Go"
      err := steve.SendMessage("欢迎回来！")
      ```

    - Zig

      ```zig title="Zig"
      const send = levilamina.slot("player_send_message") orelse return error.NotProvided;
      if (!send(steve.cSel(), levilamina.str("欢迎回来！"))) return error.Refused;
      ```

#### 广播一个文本消息给所有玩家

`Player::broadcast(msg)`  
`levilamina.Broadcast(msg)`  
`levilamina.slot("broadcast_message")`  

- 参数：
    - msg : 字符串  
      要广播的文本
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `error`
- 对应槽位：`broadcast_message`
- 示例：
    - Rust

      ```rust title="Rust"
      Player::broadcast("今晚八点活动开始")?;
      ```

    - Go

      ```go title="Go"
      err := levilamina.Broadcast("今晚八点活动开始")
      ```

    - Zig

      ```zig title="Zig"
      const bc = levilamina.slot("broadcast_message") orelse return error.NotProvided;
      bc(levilamina.str("今晚八点活动开始"));
      ```

#### 设置玩家显示标题

`steve.set_title(text)`  
`steve.SendTitle(slot, text, fadeIn, stay, fadeOut)`  
`levilamina.slot("player_send_title")`  

在屏幕中央显示一行标题。Rust 另有 `set_subtitle`、`set_actionbar`、`clear_title`；Go 和 Zig 用第一个参数选择位置。

- 参数：
    - slot : 整数  
      （Go、Zig）0 标题，1 副标题，2 动作栏
    - text : 字符串  
      要显示的文本
    - fadeIn, stay, fadeOut : 整数  
      （Go、Zig）淡入、停留、淡出的游戏刻数
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `error`，Zig `bool`
- 对应槽位：`player_send_title`
- 示例：
    - Rust

      ```rust title="Rust"
      steve.set_title("欢迎来到主城")?;
      steve.set_subtitle("祝你玩得开心")?;
      ```

    - Go

      ```go title="Go"
      err := steve.SendTitle(0, "欢迎来到主城", 10, 70, 20)
      ```

    - Zig

      ```zig title="Zig"
      const title = levilamina.slot("player_send_title") orelse return error.NotProvided;
      _ = title(steve.cSel(), 0, levilamina.str("欢迎来到主城"), 10, 70, 20);
      ```

#### 传送玩家至指定位置

`steve.teleport(dim, x, y, z)`  
`steve.Teleport(dim, x, y, z)`  
`levilamina.slot("player_teleport")`  

- 参数：
    - dim : 整数  
      目标维度：0 主世界，1 下界，2 末地，自定义维度的 id 用维度 API 查询
    - x, y, z : 小数  
      目标坐标
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `error`，Zig `bool`
- 对应槽位：`player_teleport`
- 示例：
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

#### 设置玩家等级

`steve.set_level(level)`  
`steve.SetLevel(level)`  
`steve.setLevel(level)`  

- 参数：
    - level : 整数（Go、Zig 为小数）  
      新的等级
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `error`，Zig `levilamina.Error!void`
    - 等级通过游戏自己的加减等级流程修改，和玩家用经验升级时一样，相关的界面和事件都会更新。
- 对应槽位：`player_set_num`
- 示例：
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

#### 设置玩家的游戏模式

`steve.set_gamemode(mode)`  
`steve.SetGameType(mode)`  
`levilamina.slot("player_set_gamemode")`  

- 参数：
    - mode : Rust `GameMode`，Go、Zig 整数  
      0 生存，1 创造，2 冒险，6 旁观
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `error`，Zig `bool`
- 对应槽位：`player_set_gamemode`
- 示例：
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

#### 断开玩家连接

`steve.disconnect(reason)`  
`steve.Disconnect(reason)`  
`levilamina.slot("player_disconnect")`  

- 参数：
    - reason : 字符串  
      玩家看到的断开原因
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `error`，Zig `bool`
- 对应槽位：`player_disconnect`
- 示例：
    - Rust

      ```rust title="Rust"
      steve.disconnect("你被请出了服务器")?;
      ```

    - Go

      ```go title="Go"
      err := steve.Disconnect("你被请出了服务器")
      ```

    - Zig

      ```zig title="Zig"
      const kick = levilamina.slot("player_disconnect") orelse return error.NotProvided;
      _ = kick(steve.cSel(), levilamina.str("你被请出了服务器"));
      ```

#### 获取玩家手中的物品

`steve.carried_item()`  
`steve.CarriedItem()`  
`levilamina.slot("player_get_carried_item")`  

- 返回值：玩家主手里的物品
- 返回值类型：Rust `Result<ItemStack>`，Go `(Item, error)`，Zig 物品的 SNBT
    - 物品是读取那一刻的快照，详见 [物品与容器](item.md)。
- 对应槽位：`player_get_carried_item`
- 示例：
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

#### 获取玩家背包的容器对象

`steve.inventory()`  
`steve.Inventory()`  
`c.PierContainerRef{ .which = 0, ... }`  

末影箱、盔甲栏、副手：Rust 用 `ender_chest`、`armor`、`offhand_container`，Go 用 `EnderChest`、`Armor`、`Offhand`。

- 返回值：玩家背包的容器对象，用法见 [物品与容器](item.md)
- 返回值类型：`Container`
- 示例：
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

#### 给予玩家物品

`steve.give_item(&item)`  
`steve.GiveItem(item.SNBT, 0, 0, 0)`  
`steve.giveItem(allocator, snbt, 0, 0, 0)`  

- 参数：
    - item : 物品  
      要给的物品，见 [物品与容器](item.md)
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `(string, error)`，Zig `levilamina.Error![]u8`
- 对应槽位：`player_action`
- 示例：
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
