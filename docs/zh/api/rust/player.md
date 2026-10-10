# levilamina::player · 玩家

玩家：用选择器指定，每次调用都重新解析。

**只有 xuid 能当作键**

`PlayerSel::Name` 在宿主那边对不上任何账号名时，会退回去匹配显示名，而显示名是别的模组可以改的。一名玩家把自己的显示名改成某个离线玩家的账号名，所有按名字的调用就都落到了他身上。权限、经济和归属的判断都要用 [`Player::by_xuid`](player.md#Player.by_xuid)；见 [`crate::sel`](player.md#PlayerSel) 的模块文档。

**玩家也是实体**

[`Player::as_entity`](player.md#Player.as_entity) 经 `player_resolve` 取得 `ActorUniqueID`，之后 [`crate::entity::Entity`](entity.md#Entity) 的全部能力都可以用。两套接口互相补充：物品栏、能力位、标题、踢出这类玩家特有的在这里，生命值、传送、标签这类实体通用的在那里。

## `Player` {#Player}

```rust
pub struct Player {
    // private fields
}
```

一名玩家。它只存着选择器，不存指针。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`、`Eq`、`Hash`、`Display`、`From`

### `Player::by_name` {#Player.by_name}

```rust
pub fn by_name(name: impl Into<String>) -> Player
```

按名字指定。这会经过显示名的回退，所以认人要用 [`Player::by_xuid`](player.md#Player.by_xuid)。

- 参数：
    - name : `impl Into<String>`
- 返回值类型：`Player`

### `Player::by_xuid` {#Player.by_xuid}

```rust
pub fn by_xuid(xuid: impl Into<String>) -> Player
```

按 xuid 指定：唯一，伪造不了，玩家自己也改不了。

- 参数：
    - xuid : `impl Into<String>`
- 返回值类型：`Player`

### `Player::by_uuid` {#Player.by_uuid}

```rust
pub fn by_uuid(uuid: impl Into<String>) -> Player
```

- 参数：
    - uuid : `impl Into<String>`
- 返回值类型：`Player`

### `Player::from_sel` {#Player.from_sel}

```rust
pub fn from_sel(sel: PlayerSel) -> Player
```

- 参数：
    - sel : `PlayerSel`
- 返回值类型：`Player`

### `Player::sel` {#Player.sel}

```rust
pub fn sel(&self) -> &PlayerSel
```

- 返回值类型：`&PlayerSel`

### `Player::list` {#Player.list}

```rust
pub fn list() -> Vec<PlayerInfo>
```

在线玩家的列表。

宿主每个玩家输出一段 SNBT。解析不了的条目会被跳过并记一条警告，不会让整张表变空：一条坏数据不应该让「服务器上有谁」变得答不上来。每个 ABI v2 宿主都提供 `list_players`，服务器和客户端都一样，所以列表为空就表示没有人在线。

- 返回值类型：`Vec<PlayerInfo>`
- 对应槽位：[`list_players`](../cpp/player.md#list_players)、[`list_players`](../cpp/player.md#list_players)

### `Player::broadcast` {#Player.broadcast}

```rust
pub fn broadcast(msg: &str) -> Result<()>
```

给每个在线玩家发送一条消息。

- 参数：
    - msg : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`broadcast_message`](../cpp/player.md#broadcast_message)

### `Player::is_online` {#Player.is_online}

```rust
pub fn is_online(&self) -> bool
```

这个选择器当前能不能解析到某个人。每个 ABI v2 宿主都提供 `player_resolve`，服务器和客户端都一样，所以这里的 false 表示没有人对得上，不会是宿主分辨不出来。

- 返回值类型：`bool`
- 对应槽位：[`player_resolve`](../cpp/player.md#player_resolve)

### `Player::as_entity` {#Player.as_entity}

```rust
pub fn as_entity(&self) -> Result<Entity>
```

把玩家当作实体使用，获得 [`Entity`](entity.md#Entity) 的全部能力。

- 返回值类型：`Result<Entity>`
- 对应槽位：[`player_resolve`](../cpp/player.md#player_resolve)

### `Player::num` {#Player.num}

```rust
pub fn num(&self, prop: i32) -> Result<f64>
```

读取一个 `PIER_PPROP_*` 数值属性。

- 参数：
    - prop : `i32`
- 返回值类型：`Result<f64>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::set_num` {#Player.set_num}

```rust
pub fn set_num(&self, prop: i32, v: f64) -> Result<()>
```

写入一个 `PIER_PPROP_*` 数值属性。只有标着 (S) 的才能写。

- 参数：
    - prop : `i32`
    - v : `f64`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_set_num`](../cpp/player.md#player_set_num)

### `Player::text` {#Player.text}

```rust
pub fn text(&self, prop: i32) -> Result<String>
```

读取一个 `PIER_PSTR_*` 字符串属性。

- 参数：
    - prop : `i32`
- 返回值类型：`Result<String>`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player::last_death_pos` {#Player.last_death_pos}

```rust
pub fn last_death_pos(&self) -> Result<Option<(PositionF64, i32)>>
```

上一次死亡的位置。从来没死过时返回 `Ok(None)`，因为宿主给的是空字符串。

- 返回值类型：`Result<Option<(PositionF64, i32)>>`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player::game_type` {#Player.game_type}

```rust
pub fn game_type(&self) -> Result<GameMode>
```

- 返回值类型：`Result<GameMode>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::permission_level` {#Player.permission_level}

```rust
pub fn permission_level(&self) -> Result<PlayerPermission>
```

- 返回值类型：`Result<PlayerPermission>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::set_level` {#Player.set_level}

```rust
pub fn set_level(&self, level: i32) -> Result<()>
```

- 参数：
    - level : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_set_num`](../cpp/player.md#player_set_num)

### `Player::set_experience` {#Player.set_experience}

```rust
pub fn set_experience(&self, progress: f64) -> Result<()>
```

经验条的进度，从 0 到 1。

- 参数：
    - progress : `f64`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_set_num`](../cpp/player.md#player_set_num)

### `Player::set_hunger` {#Player.set_hunger}

```rust
pub fn set_hunger(&self, v: f64) -> Result<()>
```

- 参数：
    - v : `f64`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_set_num`](../cpp/player.md#player_set_num)

### `Player::set_saturation` {#Player.set_saturation}

```rust
pub fn set_saturation(&self, v: f64) -> Result<()>
```

- 参数：
    - v : `f64`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_set_num`](../cpp/player.md#player_set_num)

### `Player::set_exhaustion` {#Player.set_exhaustion}

```rust
pub fn set_exhaustion(&self, v: f64) -> Result<()>
```

- 参数：
    - v : `f64`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_set_num`](../cpp/player.md#player_set_num)

### `Player::position` {#Player.position}

```rust
pub fn position(&self) -> Result<(PositionF64, i32)>
```

位置。它走的是专用的槽位，没有用属性编号：一次调用就拿到三个坐标和维度，分三次读属性的话，玩家可能在两次调用之间移动了。

- 返回值类型：`Result<(PositionF64, i32)>`
- 对应槽位：[`get_player_position`](../cpp/player.md#get_player_position)、[`player_get_str`](../cpp/player.md#player_get_str)

### `Player::network_status` {#Player.network_status}

```rust
pub fn network_status(&self) -> Result<NetworkStatus>
```

详细的网络状态。

- 返回值类型：`Result<NetworkStatus>`
- 对应槽位：[`player_get_network_status`](../cpp/player.md#player_get_network_status)

### `Player::conn_id` {#Player.conn_id}

```rust
pub fn conn_id(&self) -> Result<u64>
```

这名玩家的连接 id，和数据包拦截器看到的是同一个数。

0 表示不在线，或者拿不到网络标识。在 ABI 上 0 不是有效的连接 id，所以这里如实地把它报告成 `Err`，不交出一个 0。

- 返回值类型：`Result<u64>`
- 对应槽位：[`player_conn_id`](../cpp/player.md#player_conn_id)

### `Player::disconnect` {#Player.disconnect}

```rust
pub fn disconnect(&self, reason: &str) -> Result<()>
```

- 参数：
    - reason : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_disconnect`](../cpp/player.md#player_disconnect)

### `Player::set_gamemode` {#Player.set_gamemode}

```rust
pub fn set_gamemode(&self, mode: GameMode) -> Result<()>
```

- 参数：
    - mode : `GameMode`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_set_gamemode`](../cpp/player.md#player_set_gamemode)

### `Player::teleport` {#Player.teleport}

```rust
pub fn teleport(&self, dim: i32, x: f64, y: f64, z: f64) -> Result<()>
```

传送。id 为 3 及以上的自定义维度也走这里；维度桥接构造不出对得上的实例时，宿主会让调用失败，不会把人丢进一个对不上的维度。

- 参数：
    - dim : `i32`
    - x : `f64`
    - y : `f64`
    - z : `f64`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_teleport`](../cpp/player.md#player_teleport)

### `Player::act` {#Player.act}

```rust
pub fn act(&self, action: i32, sarg: &str, a: f64, b: f64, c: f64) -> Result<String>
```

执行一个 `PIER_PACT_*` 动作，返回它的输出。

- 参数：
    - action : `i32`
    - sarg : `&str`
    - a : `f64`
    - b : `f64`
    - c : `f64`
- 返回值类型：`Result<String>`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player::set_ability` {#Player.set_ability}

```rust
pub fn set_ability<V: AbilityValue>(&self, ability: Ability, value: V) -> Result<()>
```

设置一个能力位。

在浮点类的能力上传布尔值不会报错，只会按另一种解释写进去，所以 [`Ability::is_float`](types.md#Ability.is_float) 会先把它拦下来。

- 参数：
    - ability : `Ability`
    - value : `V`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player::set_ability_raw` {#Player.set_ability_raw}

```rust
pub fn set_ability_raw(&self, index: i32, value: f64) -> Result<()>
```

按下标设置能力位，用于宿主比这一层新、多出了能力位的情况。

- 参数：
    - index : `i32`
    - value : `f64`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player::can_use_ability` {#Player.can_use_ability}

```rust
pub fn can_use_ability(&self, ability: Ability) -> Result<bool>
```

- 参数：
    - ability : `Ability`
- 返回值类型：`Result<bool>`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player::set_permission_level` {#Player.set_permission_level}

```rust
pub fn set_permission_level(&self, level: PlayerPermission) -> Result<()>
```

- 参数：
    - level : `PlayerPermission`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player::set_selected_slot` {#Player.set_selected_slot}

```rust
pub fn set_selected_slot(&self, slot: i32) -> Result<()>
```

- 参数：
    - slot : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player::give_item` {#Player.give_item}

```rust
pub fn give_item(&self, item: &ItemStack) -> Result<()>
```

- 参数：
    - item : `&ItemStack`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player::set_spawn_point` {#Player.set_spawn_point}

```rust
pub fn set_spawn_point(&self, dim: i32, x: i32, y: i32, z: i32) -> Result<()>
```

- 参数：
    - dim : `i32`
    - x : `i32`
    - y : `i32`
    - z : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player::add_experience` {#Player.add_experience}

```rust
pub fn add_experience(&self, xp: i32) -> Result<()>
```

- 参数：
    - xp : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player::add_levels` {#Player.add_levels}

```rust
pub fn add_levels(&self, levels: i32) -> Result<()>
```

- 参数：
    - levels : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player::set_chunk_radius` {#Player.set_chunk_radius}

```rust
pub fn set_chunk_radius(&self, radius: i32) -> Result<()>
```

- 参数：
    - radius : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player::set_enchantment_seed` {#Player.set_enchantment_seed}

```rust
pub fn set_enchantment_seed(&self, seed: i32) -> Result<()>
```

- 参数：
    - seed : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player::play_emote` {#Player.play_emote}

```rust
pub fn play_emote(&self, piece_id: &str) -> Result<()>
```

- 参数：
    - piece_id : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player::resend_all_chunks` {#Player.resend_all_chunks}

```rust
pub fn resend_all_chunks(&self) -> Result<()>
```

- 返回值类型：`Result<()>`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player::open_inventory` {#Player.open_inventory}

```rust
pub fn open_inventory(&self) -> Result<()>
```

- 返回值类型：`Result<()>`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player::start_riding` {#Player.start_riding}

```rust
pub fn start_riding(&self, vehicle: Entity) -> Result<()>
```

- 参数：
    - vehicle : `Entity`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player::stop_riding` {#Player.stop_riding}

```rust
pub fn stop_riding(&self) -> Result<()>
```

- 返回值类型：`Result<()>`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player::attack` {#Player.attack}

```rust
pub fn attack(&self, target: Entity) -> Result<()>
```

- 参数：
    - target : `Entity`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player::interact` {#Player.interact}

```rust
pub fn interact(&self, target: Entity) -> Result<()>
```

- 参数：
    - target : `Entity`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player::drop_item` {#Player.drop_item}

```rust
pub fn drop_item(&self, item: &ItemStack, random: bool) -> Result<()>
```

- 参数：
    - item : `&ItemStack`
    - random : `bool`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player::set_sidebar` {#Player.set_sidebar}

```rust
pub fn set_sidebar(&self, objective: &str, title: &str, lines: &[String]) -> Result<()>
```

只对这名玩家显示的侧边栏。`lines` 从上到下排列。

- 参数：
    - objective : `&str`
    - title : `&str`
    - lines : `&[String]`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player::clear_sidebar` {#Player.clear_sidebar}

```rust
pub fn clear_sidebar(&self, objective: &str) -> Result<()>
```

- 参数：
    - objective : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player::send_message` {#Player.send_message}

```rust
pub fn send_message(&self, msg: &str) -> Result<()>
```

- 参数：
    - msg : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_send_message`](../cpp/player.md#player_send_message)

### `Player::tell` {#Player.tell}

```rust
pub fn tell(&self, msg: &str, kind: MessageType) -> Result<()>
```

按指定的 `TextPacketType` 发送一条消息。

- 参数：
    - msg : `&str`
    - kind : `MessageType`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_send_message_typed`](../cpp/player.md#player_send_message_typed)

### `Player::send_title` {#Player.send_title}

```rust
pub fn send_title(&self, kind: TitleKind, text: &str, times: Option<TitleTimes>) -> Result<()>
```

发送一个标题。

它通过真正的 `SetTitlePacket` 发送，没有拼 `/title` 命令：拼命令会把文本原样塞进命令行，名字里带引号或者 `@e` 的地皮就成了一次命令注入。

给了 `times` 时会先发一个 Times 包，计时是确定的；省略它则沿用客户端上一次存下的时长。三个时长不能只给一部分，那样的组合宿主会直接拒绝。

- 参数：
    - kind : `TitleKind`
    - text : `&str`
    - times : `Option<TitleTimes>`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_send_title`](../cpp/player.md#player_send_title)

### `Player::set_title` {#Player.set_title}

```rust
pub fn set_title(&self, text: &str) -> Result<()>
```

- 参数：
    - text : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_send_title`](../cpp/player.md#player_send_title)

### `Player::set_subtitle` {#Player.set_subtitle}

```rust
pub fn set_subtitle(&self, text: &str) -> Result<()>
```

- 参数：
    - text : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_send_title`](../cpp/player.md#player_send_title)

### `Player::set_actionbar` {#Player.set_actionbar}

```rust
pub fn set_actionbar(&self, text: &str) -> Result<()>
```

- 参数：
    - text : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_send_title`](../cpp/player.md#player_send_title)

### `Player::clear_title` {#Player.clear_title}

```rust
pub fn clear_title(&self) -> Result<()>
```

- 返回值类型：`Result<()>`
- 对应槽位：[`player_send_title`](../cpp/player.md#player_send_title)

### `Player::spawn_particle` {#Player.spawn_particle}

```rust
pub fn spawn_particle(&self, dim: i32, effect: &str, x: f64, y: f64, z: f64) -> Result<()>
```

只为这一名玩家生成粒子。

`World::spawn_particle` 会向整个维度广播，这个则只有这名玩家看得到。选区高亮这类东西必须用这个，否则全服务器都看得到。

- 参数：
    - dim : `i32`
    - effect : `&str`
    - x : `f64`
    - y : `f64`
    - z : `f64`
- 返回值类型：`Result<()>`
- 对应槽位：[`spawn_particle_for`](../cpp/world.md#spawn_particle_for)

### `Player::send_packet` {#Player.send_packet}

```rust
pub fn send_packet(&self, packet_id: i32, body: &[u8]) -> Result<()>
```

把一个原始数据包推到这名玩家的连接上。

这是一个逃生口：`body` 是当前游戏版本的线上格式，每次版本变化都要跟着改，这由调用方负责。有命名的接口时，请优先用那些。

- 参数：
    - packet_id : `i32`
    - body : `&[u8]`
- 返回值类型：`Result<()>`
- 对应槽位：[`send_packet`](../cpp/packet.md#send_packet)

### `Player::inventory` {#Player.inventory}

```rust
pub fn inventory(&self) -> Container
```

- 返回值类型：`Container`

### `Player::ender_chest` {#Player.ender_chest}

```rust
pub fn ender_chest(&self) -> Container
```

- 返回值类型：`Container`

### `Player::armor` {#Player.armor}

```rust
pub fn armor(&self) -> Container
```

- 返回值类型：`Container`

### `Player::offhand_container` {#Player.offhand_container}

```rust
pub fn offhand_container(&self) -> Container
```

- 返回值类型：`Container`

### `Player::offhand` {#Player.offhand}

```rust
pub fn offhand(&self) -> Result<ItemStack>
```

副手里的物品。空手给出的是空气物品，不算错误。

- 返回值类型：`Result<ItemStack>`

### `Player::set_offhand` {#Player.set_offhand}

```rust
pub fn set_offhand(&self, item: &ItemStack) -> Result<()>
```

写入副手。之后记得调用 [`Container::refresh`](container.md#Container.refresh)，否则客户端会一直显示旧的物品。

- 参数：
    - item : `&ItemStack`
- 返回值类型：`Result<()>`

### `Player::carried_item` {#Player.carried_item}

```rust
pub fn carried_item(&self) -> Result<ItemStack>
```

手里拿着的物品。

- 返回值类型：`Result<ItemStack>`
- 对应槽位：[`player_get_carried_item`](../cpp/player.md#player_get_carried_item)

### `Player::item` {#Player.item}

```rust
pub fn item(&self, slot: i32) -> Result<ItemStack>
```

物品栏的某一格。

- 参数：
    - slot : `i32`
- 返回值类型：`Result<ItemStack>`
- 对应槽位：[`player_get_item`](../cpp/player.md#player_get_item)

### `Player::set_item` {#Player.set_item}

```rust
pub fn set_item(&self, slot: i32, item: &ItemStack) -> Result<()>
```

- 参数：
    - slot : `i32`
    - item : `&ItemStack`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_set_item`](../cpp/player.md#player_set_item)

### `Player::equipment` {#Player.equipment}

```rust
pub fn equipment(&self) -> Result<Vec<(i32, ItemStack)>>
```

全部装备。`slot` 的编号见 [`crate::types::EquipSlot`](types.md#EquipSlot)。

- 返回值类型：`Result<Vec<(i32, ItemStack)>>`
- 对应槽位：[`player_get_equipment`](../cpp/player.md#player_get_equipment)

### `Player::cooldown` {#Player.cooldown}

```rust
pub fn cooldown(&self, item_name: &str) -> Result<i32>
```

某个物品的冷却还剩多少刻。

在 ABI 上，-1 同时表示不在冷却中和玩家不在线。这里原样交出，并把这一点写明，不替调用方去猜（契约 §5.2）。要区分这两种情况，先调用 [`Player::is_online`](player.md#Player.is_online)。

- 参数：
    - item_name : `&str`
- 返回值类型：`Result<i32>`
- 对应槽位：[`player_get_cooldown`](../cpp/player.md#player_get_cooldown)

### `Player::start_cooldown` {#Player.start_cooldown}

```rust
pub fn start_cooldown(&self, item_name: &str, ticks: i32) -> Result<()>
```

- 参数：
    - item_name : `&str`
    - ticks : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`player_start_cooldown`](../cpp/player.md#player_start_cooldown)

### `Player::real_name` {#Player.real_name}

```rust
pub fn real_name(&self) -> Result<String>
```

账号名。

- 返回值类型：`Result<String>`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player::uuid` {#Player.uuid}

```rust
pub fn uuid(&self) -> Result<String>
```

- 返回值类型：`Result<String>`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player::xuid` {#Player.xuid}

```rust
pub fn xuid(&self) -> Result<String>
```

- 返回值类型：`Result<String>`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player::ip_and_port` {#Player.ip_and_port}

```rust
pub fn ip_and_port(&self) -> Result<String>
```

`地址:端口`。IPv6 也是这个形状，所以不能按最后一个冒号去切分。

- 返回值类型：`Result<String>`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player::locale_code` {#Player.locale_code}

```rust
pub fn locale_code(&self) -> Result<String>
```

- 返回值类型：`Result<String>`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player::respawn_pos` {#Player.respawn_pos}

```rust
pub fn respawn_pos(&self) -> Result<String>
```

以 SNBT `{x,y,z,dim}` 给出这名玩家会在哪里重生。不会为空：没有床的玩家报告的是世界出生点，这两种情况区分不开。

- 返回值类型：`Result<String>`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player::name_tag` {#Player.name_tag}

```rust
pub fn name_tag(&self) -> Result<String>
```

显示在头顶上的名字，可以被修改。认人要用 [`Player::xuid`](player.md#Player.xuid)。

- 返回值类型：`Result<String>`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player::platform_online_id` {#Player.platform_online_id}

```rust
pub fn platform_online_id(&self) -> Result<String>
```

- 返回值类型：`Result<String>`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player::dimension` {#Player.dimension}

```rust
pub fn dimension(&self) -> Result<i32>
```

- 返回值类型：`Result<i32>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::level` {#Player.level}

```rust
pub fn level(&self) -> Result<i32>
```

经验等级。

- 返回值类型：`Result<i32>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::experience` {#Player.experience}

```rust
pub fn experience(&self) -> Result<f64>
```

经验条的进度，从 0 到 1。它表示的并非累计的经验值。

- 返回值类型：`Result<f64>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::hunger` {#Player.hunger}

```rust
pub fn hunger(&self) -> Result<f64>
```

- 返回值类型：`Result<f64>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::saturation` {#Player.saturation}

```rust
pub fn saturation(&self) -> Result<f64>
```

饱和度；它耗尽以后饥饿值才开始下降。

- 返回值类型：`Result<f64>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::exhaustion` {#Player.exhaustion}

```rust
pub fn exhaustion(&self) -> Result<f64>
```

消耗度；每攒满一个单位，扣掉一点饱和度。

- 返回值类型：`Result<f64>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::xp_needed_for_next_level` {#Player.xp_needed_for_next_level}

```rust
pub fn xp_needed_for_next_level(&self) -> Result<i32>
```

距离下一级还差多少经验。

- 返回值类型：`Result<i32>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::luck` {#Player.luck}

```rust
pub fn luck(&self) -> Result<f64>
```

- 返回值类型：`Result<f64>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::selected_slot` {#Player.selected_slot}

```rust
pub fn selected_slot(&self) -> Result<i32>
```

- 返回值类型：`Result<i32>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::score` {#Player.score}

```rust
pub fn score(&self) -> Result<i32>
```

计分板上的 `score` 伪计分项，和任何自定义的计分项无关。

- 返回值类型：`Result<i32>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::chunk_radius` {#Player.chunk_radius}

```rust
pub fn chunk_radius(&self) -> Result<i32>
```

客户端请求的视距，单位是区块。

- 返回值类型：`Result<i32>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::enchantment_seed` {#Player.enchantment_seed}

```rust
pub fn enchantment_seed(&self) -> Result<i32>
```

- 返回值类型：`Result<i32>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::platform` {#Player.platform}

```rust
pub fn platform(&self) -> Result<i32>
```

`BuildPlatform` 的值，并非操作系统的名字。

- 返回值类型：`Result<i32>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::direction` {#Player.direction}

```rust
pub fn direction(&self) -> Result<i32>
```

朝向：0 南，1 西，2 北，3 东。

- 返回值类型：`Result<i32>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::ping` {#Player.ping}

```rust
pub fn ping(&self) -> Result<i32>
```

往返延迟，单位毫秒。详细情况见 [`Player::network_status`](player.md#Player.network_status)。

- 返回值类型：`Result<i32>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::client_sub_id` {#Player.client_sub_id}

```rust
pub fn client_sub_id(&self) -> Result<i32>
```

- 返回值类型：`Result<i32>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::fall_distance` {#Player.fall_distance}

```rust
pub fn fall_distance(&self) -> Result<f64>
```

- 返回值类型：`Result<f64>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::is_operator` {#Player.is_operator}

```rust
pub fn is_operator(&self) -> Result<bool>
```

在 `permissions.json` 里被列为管理员。它和 [`Player::permission_level`](player.md#Player.permission_level) 是两回事，后者在运行时可能改变。

- 返回值类型：`Result<bool>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::can_use_operator_blocks` {#Player.can_use_operator_blocks}

```rust
pub fn can_use_operator_blocks(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::is_flying` {#Player.is_flying}

```rust
pub fn is_flying(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::can_jump` {#Player.can_jump}

```rust
pub fn can_jump(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::is_emoting` {#Player.is_emoting}

```rust
pub fn is_emoting(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::is_in_raid` {#Player.is_in_raid}

```rust
pub fn is_in_raid(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::is_hurt` {#Player.is_hurt}

```rust
pub fn is_hurt(&self) -> Result<bool>
```

正处在受伤后的无敌帧里。

- 返回值类型：`Result<bool>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::is_scoping` {#Player.is_scoping}

```rust
pub fn is_scoping(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::can_sleep` {#Player.can_sleep}

```rust
pub fn can_sleep(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::has_respawn_position` {#Player.has_respawn_position}

```rust
pub fn has_respawn_position(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::is_using_item` {#Player.is_using_item}

```rust
pub fn is_using_item(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::is_blocking` {#Player.is_blocking}

```rust
pub fn is_blocking(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::is_gliding` {#Player.is_gliding}

```rust
pub fn is_gliding(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::is_swimming` {#Player.is_swimming}

```rust
pub fn is_swimming(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::is_dead` {#Player.is_dead}

```rust
pub fn is_dead(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player::has_died_before` {#Player.has_died_before}

```rust
pub fn has_died_before(&self) -> Result<bool>
```

在这个存档里至少死过一次。

- 返回值类型：`Result<bool>`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

## `PlayerSel` {#PlayerSel}

```rust
pub enum PlayerSel {
        /// By name. The account name is matched first and the host falls back to the display
        /// name on a miss; see the module documentation and do not use it as an identity.
        Name(String),
        /// By xuid: unique, unforgeable and unchangeable by the player. This is the one to use
        /// as a key.
        /// It may be an empty string on an offline-mode server, where only `Name` remains.
        Xuid(String),
        /// By uuid in its canonical string form. Equally stable and suited to a save key.
        Uuid(String),
}
```

怎样指定一名玩家。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`、`Eq`、`Hash`、`From`、`Display`

### `PlayerSel::name` {#PlayerSel.name}

```rust
pub fn name(v: impl Into<String>) -> PlayerSel
```

- 参数：
    - v : `impl Into<String>`
- 返回值类型：`PlayerSel`

### `PlayerSel::xuid` {#PlayerSel.xuid}

```rust
pub fn xuid(v: impl Into<String>) -> PlayerSel
```

- 参数：
    - v : `impl Into<String>`
- 返回值类型：`PlayerSel`

### `PlayerSel::uuid` {#PlayerSel.uuid}

```rust
pub fn uuid(v: impl Into<String>) -> PlayerSel
```

- 参数：
    - v : `impl Into<String>`
- 返回值类型：`PlayerSel`

### `PlayerSel::kind` {#PlayerSel.kind}

```rust
pub fn kind(&self) -> i32
```

底层的 `kind` 值，0、1 或 2，和 `abi.h` 对齐。

- 返回值类型：`i32`

### `PlayerSel::value` {#PlayerSel.value}

```rust
pub fn value(&self) -> &str
```

- 返回值类型：`&str`

### `PlayerSel::is_stable` {#PlayerSel.is_stable}

```rust
pub fn is_stable(&self) -> bool
```

这个选择器是不是可靠的身份，也就是 xuid 或 uuid。

权限、经济或归属的判断拿到 `false` 时要当心；见模块文档。

- 返回值类型：`bool`

### `PlayerSel::is_empty` {#PlayerSel.is_empty}

```rust
pub fn is_empty(&self) -> bool
```

检查是否为空：空的选择器解析不到任何人，早点发现，好过在调用处看到一个莫名其妙的 `false`。

- 返回值类型：`bool`

## `PlayerInfo` {#PlayerInfo}

```rust
pub struct PlayerInfo {
    pub name: String,
    pub xuid: String,
    pub uuid: String,
    pub dimension: Option<i32>,
    pub pos: Option<PositionF64>,
}
```

`list_players` 报告的一项。

`dimension` 和 `pos` 是 `Option`，没有用裸值。世界还没就绪，或者这名玩家正在换维度时，宿主会省略这两个键，而填 0 就等于说他在主世界的原点。契约 §5.1 记录的领地保护绕过就是这个形状：自定义维度里的一个事件读不到 `dim`，使用方写了 `unwrap_or(0)`，结果一切都被当成主世界放行了。

想要默认值的调用方自己写 `.unwrap_or(0)`，这个默认值的后果也由它自己负责。

- 实现的 trait：`Debug`、`Clone`、`Default`、`PartialEq`

### `PlayerInfo::selector` {#PlayerInfo.selector}

```rust
pub fn selector(&self) -> PlayerSel
```

用 xuid 构造一个稳定的选择器。离线模式的服务器上 xuid 为空，这时退回到名字，并且会表明这一点，调用方由此知道这个键靠不住。

- 返回值类型：`PlayerSel`

## `NetworkStatus` {#NetworkStatus}

```rust
pub struct NetworkStatus {
    pub ping: i32,
    pub avg_ping: i32,
    pub max_ping: i32,
    /// Per mille, exactly as the host gives it.
    pub packet_loss: i32,
}
```

一名玩家的网络状态，取自 `PIER_PSTR_NETWORK_STATUS`。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`Default`、`PartialEq`、`Eq`
