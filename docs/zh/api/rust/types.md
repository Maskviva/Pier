# levilamina::types · 通用类型

各个领域共用的值类型。

只有不带行为的东西放在这里：坐标、枚举和位标志。它们不碰 `PierApi`，所以各个领域的模块可以共用它们，而不用互相依赖。

**枚举带有 `from_i32`，并返回 `Option`**

宿主可能比模组新，报告一个这一侧不认识的值（契约 §2.2）。`from_i32` 返回 `None` 表示宿主报告了一个不认识的值，这和值为 0 是两回事（§5.2）。反方向用 `as_i32`，那里不会出现未知的值。

## `Bounds` {#Bounds}

```rust
pub struct Bounds {
    pub min: PositionI32,
    pub max: PositionI32,
}
```

一个闭区间的长方体，两个角都包含在内。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

### `Bounds::new` {#Bounds.new}

```rust
pub fn new(a: PositionI32, b: PositionI32) -> Bounds
```

用任意两个角构造一个长方体，每个轴分别取较小值和较大值，调用方不需要先排序。

- 参数：
    - a : `PositionI32`
    - b : `PositionI32`
- 返回值类型：`Bounds`

### `Bounds::size` {#Bounds.size}

```rust
pub fn size(&self) -> (u64, u64, u64)
```

每个轴上的格子数。区间是闭的，所以是 max - min + 1。

- 返回值类型：`(u64, u64, u64)`

### `Bounds::volume` {#Bounds.volume}

```rust
pub fn volume(&self) -> u64
```

总格子数。用 `u64`，因为一个跨越整个维度的选区很容易超出 `u32`。

- 返回值类型：`u64`

### `Bounds::contains` {#Bounds.contains}

```rust
pub fn contains(&self, p: PositionI32) -> bool
```

- 参数：
    - p : `PositionI32`
- 返回值类型：`bool`

## `RayHit` {#RayHit}

```rust
pub enum RayHit {
        /// It hit an actor.
        Entity { id: i64, pos: PositionF64 },
        /// It hit a block. `facing` is the face hit and `block` is the block cell coordinate.
        Block {
            block: PositionI32,
            facing: i32,
            name: String,
            pos: PositionF64,
        },
        /// Nothing was within range. This is not an error and stays apart from the question being
        /// unanswerable (contract §5.2).
        None,
}
```

一次射线检测的结果，来自 `actor_trace_ray` 或 `edit_trace_ray`。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`

### `RayHit::block_pos` {#RayHit.block_pos}

```rust
pub fn block_pos(&self) -> Option<PositionI32>
```

- 返回值类型：`Option<PositionI32>`

### `RayHit::entity_id` {#RayHit.entity_id}

```rust
pub fn entity_id(&self) -> Option<i64>
```

- 返回值类型：`Option<i64>`

### `RayHit::is_none` {#RayHit.is_none}

```rust
pub fn is_none(&self) -> bool
```

- 返回值类型：`bool`

## `GameMode` {#GameMode}

```rust
pub enum GameMode {
        Survival = 0,
        Creative = 1,
        Adventure = 2,
        Spectator = 6,
}
```

游戏模式。取值属于 ABI，经 `player_set_gamemode` 传递。

注意 `Spectator` 是 6，不是 3：中间的值在引擎里有别的含义，按顺序猜一个值，会在没有任何报错的情况下把玩家设成另一种模式。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

### `GameMode::from_i32` {#GameMode.from_i32}

```rust
pub fn from_i32(v: i32) -> Option<GameMode>
```

- 参数：
    - v : `i32`
- 返回值类型：`Option<GameMode>`

### `GameMode::as_i32` {#GameMode.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- 返回值类型：`i32`

## `PlayerPermission` {#PlayerPermission}

```rust
pub enum PlayerPermission {
        Visitor = 0,
        Member = 1,
        Operator = 2,
        /// In the engine this means customized per item and is not a level above Operator.
        Custom = 3,
}
```

玩家的权限等级，经 `PIER_PACT_SET_PERMISSION_LEVEL` 和 `PIER_PPROP_PERMISSION_LEVEL` 传递。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

### `PlayerPermission::from_i32` {#PlayerPermission.from_i32}

```rust
pub fn from_i32(v: i32) -> Option<PlayerPermission>
```

- 参数：
    - v : `i32`
- 返回值类型：`Option<PlayerPermission>`

### `PlayerPermission::as_i32` {#PlayerPermission.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- 返回值类型：`i32`

## `Weather` {#Weather}

```rust
pub enum Weather {
        Clear = 0,
        Rain = 1,
        Thunder = 2,
}
```

天气。`set_weather` 的三种状态。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

### `Weather::from_i32` {#Weather.from_i32}

```rust
pub fn from_i32(v: i32) -> Option<Weather>
```

- 参数：
    - v : `i32`
- 返回值类型：`Option<Weather>`

### `Weather::as_i32` {#Weather.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- 返回值类型：`i32`

## `Difficulty` {#Difficulty}

```rust
pub enum Difficulty {
        Peaceful = 0,
        Easy = 1,
        Normal = 2,
        Hard = 3,
}
```

难度。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

### `Difficulty::from_i32` {#Difficulty.from_i32}

```rust
pub fn from_i32(v: i32) -> Option<Difficulty>
```

- 参数：
    - v : `i32`
- 返回值类型：`Option<Difficulty>`

### `Difficulty::as_i32` {#Difficulty.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- 返回值类型：`i32`

## `TitleKind` {#TitleKind}

```rust
pub enum TitleKind {
        Clear = 0,
        Reset = 1,
        Title = 2,
        Subtitle = 3,
        Actionbar = 4,
        Times = 5,
}
```

`player_send_title` 用的 `SetTitlePacketPayload::TitleType`。

6 到 8 的三种 TextObject 变体需要 `ResolvedTextObject`，宿主会明确拒绝，所以这里根本不提供它们。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

### `TitleKind::as_i32` {#TitleKind.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- 返回值类型：`i32`

### `TitleKind::uses_text` {#TitleKind.uses_text}

```rust
pub fn uses_text(self) -> bool
```

Clear、Reset 和 Times 这三种，宿主不读 `text` 参数。

- 返回值类型：`bool`

## `EquipSlot` {#EquipSlot}

```rust
pub enum EquipSlot {
        MainHand = 0,
        OffHand = 1,
        Helmet = 2,
        Chestplate = 3,
        Leggings = 4,
        Boots = 5,
}
```

装备槽位。`actor_get_equipped_item` 和 `player_get_equipment` 用的编号。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

### `EquipSlot::from_i32` {#EquipSlot.from_i32}

```rust
pub fn from_i32(v: i32) -> Option<EquipSlot>
```

- 参数：
    - v : `i32`
- 返回值类型：`Option<EquipSlot>`

### `EquipSlot::as_i32` {#EquipSlot.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- 返回值类型：`i32`

## `Ability` {#Ability}

```rust
pub enum Ability {
        Build = 0,
        Mine = 1,
        DoorsAndSwitches = 2,
        OpenContainers = 3,
        AttackPlayers = 4,
        AttackMobs = 5,
        Operator = 6,
        Teleport = 7,
        Invulnerable = 8,
        Flying = 9,
        MayFly = 10,
        Instabuild = 11,
        Lightning = 12,
        FlySpeed = 13,
        WalkSpeed = 14,
        Muted = 15,
        WorldBuilder = 16,
        NoClip = 17,
        PrivilegedBuilder = 18,
        VerticalFlySpeed = 19,
}
```

玩家的一个能力位。`PIER_PACT_SET_ABILITY` 和 `PIER_PACT_CAN_USE_ABILITY` 的下标。

名字里带 `Speed` 的三个是浮点类的能力，其余是布尔类的。传错类型不会报错，只会让值被按另一种方式解读，所以才有 [`Ability::is_float`](types.md#Ability.is_float)，[`crate::player::Player::set_ability`](player.md#Player.set_ability) 也会拿它来检查。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

### `Ability::is_float` {#Ability.is_float}

```rust
pub fn is_float(self) -> bool
```

- 返回值类型：`bool`

### `Ability::as_i32` {#Ability.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- 返回值类型：`i32`

## `MessageType` {#MessageType}

```rust
pub enum MessageType {
        Raw = 0,
        Chat = 1,
        Translate = 2,
        Popup = 3,
        JukeboxPopup = 4,
        Tip = 5,
        SystemMessage = 6,
        Whisper = 7,
        Announcement = 8,
        TextObjectWhisper = 9,
        TextObject = 10,
        TextObjectAnnouncement = 11,
}
```

`player_send_message_typed` 用的 `TextPacketType`。

超出范围的值，宿主会按 `Raw` 处理，所以这里不需要兜底的分支。带作者或参数的变体，即 Chat、Whisper 和 Translate，在 ABI 上只接收一个消息体字符串，作者字段到达客户端时是空的。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

### `MessageType::as_i32` {#MessageType.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- 返回值类型：`i32`

## `TitleTimes` {#TitleTimes}

```rust
pub struct TitleTimes {
    pub fade_in: i32,
    pub stay: i32,
    pub fade_out: i32,
}
```

标题的三个时长，单位是刻。

三个要么一起给，要么都不给：只指定一部分的组合会被宿主拒绝，它不替调用方猜剩下的值，因为一半的时长没有合理的默认值。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`、`Default`

### `TitleTimes::new` {#TitleTimes.new}

```rust
pub const fn new(fade_in: i32, stay: i32, fade_out: i32) -> TitleTimes
```

- 参数：
    - fade_in : `i32`
    - stay : `i32`
    - fade_out : `i32`
- 返回值类型：`TitleTimes`

## `BlockUpdate` {#BlockUpdate}

```rust
pub struct BlockUpdate(pub i32);
```

告诉引擎一次方块写入之后需要做哪些后续工作。

`NONE` 最快，但客户端不知道方块变了，所以批量填充之后必须重新同步，否则玩家会一直看到旧的世界，直到那个区块被重新发送。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`、`Default`

### `BlockUpdate::bits` {#BlockUpdate.bits}

```rust
pub fn bits(self) -> i32
```

- 返回值类型：`i32`

## `AbilityValue` {#AbilityValue}

```rust
pub trait AbilityValue: Copy { /* ... */ }
```

能力位可以取的值。有布尔和浮点两类，由 [`Ability::is_float`](types.md#Ability.is_float) 决定用哪一类。

### `AbilityValue::as_f64` {#AbilityValue.as_f64}

```rust
fn as_f64(self) -> f64
```

- 返回值类型：`f64`

## `PositionI32` {#PositionI32}

```rust
pub type PositionI32 = (i32, i32, i32);
```

一个方块坐标。

## `PositionF64` {#PositionF64}

```rust
pub type PositionF64 = (f64, f64, f64);
```

一个实体坐标或精确坐标。

## `LocalTime` {#LocalTime}

```rust
pub struct LocalTime {
    pub year: i32,
    pub month: i32,
    pub day: i32,
    pub hour: i32,
    pub minute: i32,
    pub second: i32,
    pub ms: i32,
}
```

本地时间，取自 `PIER_SYS_LOCAL_TIME`。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`、`Default`
