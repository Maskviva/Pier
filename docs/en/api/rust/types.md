# levilamina::types

The value types every domain shares.

Only things without behavior belong here: coordinates, enums and bit flags. They touch
no `PierApi`, so domain modules can share them without depending on one another.

**Enums carry `from_i32` and return an `Option`**

The host may be newer than the mod and report a value this side does not recognize
(contract §2.2). A `None` from `from_i32` means the host reported an unrecognized
value and stays apart from the value being 0 (§5.2). The other direction uses `as_i32`,
where no unknown value can arise.

## `Bounds` {#Bounds}

```rust
pub struct Bounds {
    pub min: PositionI32,
    pub max: PositionI32,
}
```

A closed box, with both corners included.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

### `Bounds::new` {#Bounds.new}

```rust
pub fn new(a: PositionI32, b: PositionI32) -> Bounds
```

Builds a box from any two corners, taking the smaller and larger per axis, so a caller
need not sort them first.

- Parameters:
    - a : `PositionI32`
    - b : `PositionI32`
- Return type: `Bounds`

### `Bounds::size` {#Bounds.size}

```rust
pub fn size(&self) -> (u64, u64, u64)
```

The cell count per axis. The interval is closed, so it is max - min + 1.

- Return type: `(u64, u64, u64)`

### `Bounds::volume` {#Bounds.volume}

```rust
pub fn volume(&self) -> u64
```

The total cell count. A `u64` is used because a selection spanning a dimension easily
overflows a `u32`.

- Return type: `u64`

### `Bounds::contains` {#Bounds.contains}

```rust
pub fn contains(&self, p: PositionI32) -> bool
```

- Parameters:
    - p : `PositionI32`
- Return type: `bool`

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

The result of one ray trace, from `actor_trace_ray` or `edit_trace_ray`.

- Implements: `Debug`, `Clone`, `PartialEq`

### `RayHit::block_pos` {#RayHit.block_pos}

```rust
pub fn block_pos(&self) -> Option<PositionI32>
```

- Return type: `Option<PositionI32>`

### `RayHit::entity_id` {#RayHit.entity_id}

```rust
pub fn entity_id(&self) -> Option<i64>
```

- Return type: `Option<i64>`

### `RayHit::is_none` {#RayHit.is_none}

```rust
pub fn is_none(&self) -> bool
```

- Return type: `bool`

## `GameMode` {#GameMode}

```rust
pub enum GameMode {
        Survival = 0,
        Creative = 1,
        Adventure = 2,
        Spectator = 6,
}
```

The game mode. The values are part of the ABI, through `player_set_gamemode`.

Note that `Spectator` is 6 and not 3: the values in between mean other things in the
engine, and guessing one from the order sets the player to a different mode without an
error.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

### `GameMode::from_i32` {#GameMode.from_i32}

```rust
pub fn from_i32(v: i32) -> Option<GameMode>
```

- Parameters:
    - v : `i32`
- Return type: `Option<GameMode>`

### `GameMode::as_i32` {#GameMode.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- Return type: `i32`

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

The player permission level, through `PIER_PACT_SET_PERMISSION_LEVEL` and
`PIER_PPROP_PERMISSION_LEVEL`.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

### `PlayerPermission::from_i32` {#PlayerPermission.from_i32}

```rust
pub fn from_i32(v: i32) -> Option<PlayerPermission>
```

- Parameters:
    - v : `i32`
- Return type: `Option<PlayerPermission>`

### `PlayerPermission::as_i32` {#PlayerPermission.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- Return type: `i32`

## `Weather` {#Weather}

```rust
pub enum Weather {
        Clear = 0,
        Rain = 1,
        Thunder = 2,
}
```

The weather. The three states of `set_weather`.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

### `Weather::from_i32` {#Weather.from_i32}

```rust
pub fn from_i32(v: i32) -> Option<Weather>
```

- Parameters:
    - v : `i32`
- Return type: `Option<Weather>`

### `Weather::as_i32` {#Weather.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- Return type: `i32`

## `Difficulty` {#Difficulty}

```rust
pub enum Difficulty {
        Peaceful = 0,
        Easy = 1,
        Normal = 2,
        Hard = 3,
}
```

The difficulty.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

### `Difficulty::from_i32` {#Difficulty.from_i32}

```rust
pub fn from_i32(v: i32) -> Option<Difficulty>
```

- Parameters:
    - v : `i32`
- Return type: `Option<Difficulty>`

### `Difficulty::as_i32` {#Difficulty.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- Return type: `i32`

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

The `SetTitlePacketPayload::TitleType` of `player_send_title`.

The three TextObject variants at 6 through 8 need a `ResolvedTextObject`, which the
host refuses explicitly, so they are not offered here at all.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

### `TitleKind::as_i32` {#TitleKind.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- Return type: `i32`

### `TitleKind::uses_text` {#TitleKind.uses_text}

```rust
pub fn uses_text(self) -> bool
```

The host does not read the `text` argument of Clear, Reset and Times.

- Return type: `bool`

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

An equipment slot. The numbering `actor_get_equipped_item` and `player_get_equipment`
use.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

### `EquipSlot::from_i32` {#EquipSlot.from_i32}

```rust
pub fn from_i32(v: i32) -> Option<EquipSlot>
```

- Parameters:
    - v : `i32`
- Return type: `Option<EquipSlot>`

### `EquipSlot::as_i32` {#EquipSlot.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- Return type: `i32`

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

A player ability bit. The index of `PIER_PACT_SET_ABILITY` and
`PIER_PACT_CAN_USE_ABILITY`.

The three carrying `Speed` are floating-point abilities and the rest are boolean.
Passing the wrong type raises no error and only has the value interpreted differently,
which is why [`Ability::is_float`](types.md#Ability.is_float) exists and
[`crate::player::Player::set_ability`](player.md#Player.set_ability) checks against it.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

### `Ability::is_float` {#Ability.is_float}

```rust
pub fn is_float(self) -> bool
```

- Return type: `bool`

### `Ability::as_i32` {#Ability.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- Return type: `i32`

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

The `TextPacketType` of `player_send_message_typed`.

The host falls back to `Raw` on an out-of-range value, so no fallback branch is needed
here. The variants carrying an author or parameters, Chat, Whisper and Translate, take
a single body string on the ABI, and the author field reaches the client empty.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

### `MessageType::as_i32` {#MessageType.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- Return type: `i32`

## `TitleTimes` {#TitleTimes}

```rust
pub struct TitleTimes {
    pub fade_in: i32,
    pub stay: i32,
    pub fade_out: i32,
}
```

The three title durations, in ticks.

All three are given together or not at all: the host refuses a half-specified
combination rather than guessing the rest for the caller, since half a set of durations
has no sensible default.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`, `Default`

### `TitleTimes::new` {#TitleTimes.new}

```rust
pub const fn new(fade_in: i32, stay: i32, fade_out: i32) -> TitleTimes
```

- Parameters:
    - fade_in : `i32`
    - stay : `i32`
    - fade_out : `i32`
- Return type: `TitleTimes`

## `BlockUpdate` {#BlockUpdate}

```rust
pub struct BlockUpdate(pub i32);
```

Tells the engine which follow-up work a block write needs.

`NONE` is fastest and leaves the client unaware that the block changed, so a bulk fill
has to resynchronize afterwards, otherwise the player keeps seeing the old world until
that chunk is resent.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`, `Default`

### `BlockUpdate::bits` {#BlockUpdate.bits}

```rust
pub fn bits(self) -> i32
```

- Return type: `i32`

## `AbilityValue` {#AbilityValue}

```rust
pub trait AbilityValue: Copy { /* ... */ }
```

The value an ability bit can take. There is a boolean family and a floating-point
family, and [`Ability::is_float`](types.md#Ability.is_float) decides which applies.

### `AbilityValue::as_f64` {#AbilityValue.as_f64}

```rust
fn as_f64(self) -> f64
```

- Return type: `f64`

## `PositionI32` {#PositionI32}

```rust
pub type PositionI32 = (i32, i32, i32);
```

A block coordinate.

## `PositionF64` {#PositionF64}

```rust
pub type PositionF64 = (f64, f64, f64);
```

An actor or exact coordinate.

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

The local time, from `PIER_SYS_LOCAL_TIME`.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`, `Default`
