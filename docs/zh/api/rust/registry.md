# levilamina::registry · 注册表

引擎自己的注册表：正在运行的游戏里的每一种方块、每一种物品和每一种实体，连同按规则从中挑选时需要的信息。

发放「一个随机方块」或者「一个随机稀有物品」的模组，不应该自己维护一份名字列表：游戏里的东西比任何这样的列表都多，而且每个版本都会让列表过时。改为读取这些注册表，按规则（实心的、不是技术性方块、某个稀有度）来挑。比 `registry_list` 槽位旧的宿主会返回错误，永远不会返回一个看起来像是游戏没有任何内容的空列表。

## 函数 {#functions}

### `registry::blocks` {#fn.blocks}

```rust
pub fn blocks() -> Result<Vec<BlockType>>
```

所有方块类型。

- 返回值类型：`Result<Vec<BlockType>>`
- 对应槽位：[`registry_list`](../cpp/registry.md#registry_list)

### `registry::items` {#fn.items}

```rust
pub fn items() -> Result<Vec<ItemType>>
```

所有物品，每种以它完整的名字出现一次。

- 返回值类型：`Result<Vec<ItemType>>`
- 对应槽位：[`registry_list`](../cpp/registry.md#registry_list)

### `registry::entities` {#fn.entities}

```rust
pub fn entities() -> Result<Vec<EntityType>>
```

关卡知道的所有实体。

- 返回值类型：`Result<Vec<EntityType>>`
- 对应槽位：[`registry_list`](../cpp/registry.md#registry_list)

## `EntityType` {#EntityType}

```rust
pub struct EntityType {
    pub name: String,
    #[serde(default)]
    pub spawn_egg: bool,
    #[serde(default)]
    pub summonable: bool,
    /// An experiment has to be on for it to exist.
    #[serde(default)]
    pub experimental: bool,
    /// The engine's actor type number; see [`EntityType::is_monster`] and the others.
    #[serde(default, rename = "type", deserialize_with = "whole")]
    pub type_id: i64,
}
```

关卡知道的一种实体。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`、`serde::Deserialize`

### `EntityType::is_mob` {#EntityType.is_mob}

```rust
pub fn is_mob(&self) -> bool
```

- 返回值类型：`bool`

### `EntityType::is_monster` {#EntityType.is_monster}

```rust
pub fn is_monster(&self) -> bool
```

- 返回值类型：`bool`

### `EntityType::is_animal` {#EntityType.is_animal}

```rust
pub fn is_animal(&self) -> bool
```

陆地动物：有动物位，没有水生动物位。

- 返回值类型：`bool`

### `EntityType::is_water_animal` {#EntityType.is_water_animal}

```rust
pub fn is_water_animal(&self) -> bool
```

- 返回值类型：`bool`

## `BlockType` {#BlockType}

```rust
pub struct BlockType {
    pub name: String,
    /// A [`category`] value.
    #[serde(default, deserialize_with = "whole")]
    pub category: i32,
    #[serde(default)]
    pub solid: bool,
    #[serde(default)]
    pub vanilla: bool,
    #[serde(default)]
    pub container: bool,
    /// Gives a redstone signal of its own, as a lever or a button does.
    #[serde(default)]
    pub signal: bool,
    #[serde(default)]
    pub fence: bool,
    #[serde(default)]
    pub rail: bool,
    #[serde(default)]
    pub slab: bool,
    #[serde(default)]
    pub wall: bool,
    #[serde(default)]
    pub crop: bool,
    /// Holds a block entity: a chest, a sign, a spawner, a bed.
    #[serde(default)]
    pub entity: bool,
    /// Bare-hand destroy speed; negative for a block nothing breaks.
    #[serde(default)]
    pub destroy: f64,
    #[serde(default)]
    pub resistance: f64,
    /// The light it emits, 0 to 15.
    #[serde(default, deserialize_with = "whole")]
    pub light: i32,
    /// The localization key.
    #[serde(default)]
    pub description: String,
}
```

一种方块，按它的默认状态读取。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`、`serde::Deserialize`

## `ItemType` {#ItemType}

```rust
pub struct ItemType {
    pub name: String,
    /// 0 common, 1 uncommon, 2 rare, 3 epic.
    #[serde(default, deserialize_with = "whole")]
    pub rarity: i32,
    #[serde(default, deserialize_with = "whole")]
    pub stack: i32,
    /// A [`category`] value.
    #[serde(default, deserialize_with = "whole")]
    pub category: i32,
    /// Commands do not offer it.
    #[serde(default)]
    pub hidden: bool,
}
```

一种物品。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`、`serde::Deserialize`

## 常量 {#constants}

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="ALL"></span>`ALL` | `0` |  |
| <span id="CONSTRUCTION"></span>`CONSTRUCTION` | `1` |  |
| <span id="NATURE"></span>`NATURE` | `2` |  |
| <span id="EQUIPMENT"></span>`EQUIPMENT` | `3` |  |
| <span id="ITEMS"></span>`ITEMS` | `4` |  |
| <span id="COMMAND_ONLY"></span>`COMMAND_ONLY` | `5` | 只能通过命令获得：屏障、命令方块、结构方块之类。 |
| <span id="UNDEFINED"></span>`UNDEFINED` | `6` |  |
