# levilamina::registry

The engine's own registries: every block type, every item and every entity the running game
has, with the facts a rule needs to pick from them.

A mod that hands out "a random block" or "a random rare item" should not keep a list of
names of its own: the game has more than any such list, and the list goes stale with every
version. It reads these instead and decides by rules (solid, not technical, of this rarity).
A host older than the `registry_list` slot answers with an error, never with an empty list
that would look like a game without content.

## Functions {#functions}

### `registry::blocks` {#fn.blocks}

```rust
pub fn blocks() -> Result<Vec<BlockType>>
```

Every block type.

- Return type: `Result<Vec<BlockType>>`
- Slots: [`registry_list`](../cpp/registry.md#registry_list)

### `registry::items` {#fn.items}

```rust
pub fn items() -> Result<Vec<ItemType>>
```

Every item, each once under its own full name.

- Return type: `Result<Vec<ItemType>>`
- Slots: [`registry_list`](../cpp/registry.md#registry_list)

### `registry::entities` {#fn.entities}

```rust
pub fn entities() -> Result<Vec<EntityType>>
```

Every entity the level knows.

- Return type: `Result<Vec<EntityType>>`
- Slots: [`registry_list`](../cpp/registry.md#registry_list)

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

One entity the level knows.

- Implements: `Debug`, `Clone`, `PartialEq`, `serde::Deserialize`

### `EntityType::is_mob` {#EntityType.is_mob}

```rust
pub fn is_mob(&self) -> bool
```

- Return type: `bool`

### `EntityType::is_monster` {#EntityType.is_monster}

```rust
pub fn is_monster(&self) -> bool
```

- Return type: `bool`

### `EntityType::is_animal` {#EntityType.is_animal}

```rust
pub fn is_animal(&self) -> bool
```

A land animal: the animal bit without the water-animal one.

- Return type: `bool`

### `EntityType::is_water_animal` {#EntityType.is_water_animal}

```rust
pub fn is_water_animal(&self) -> bool
```

- Return type: `bool`

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

One block type, read from its default state.

- Implements: `Debug`, `Clone`, `PartialEq`, `serde::Deserialize`

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

One item.

- Implements: `Debug`, `Clone`, `PartialEq`, `serde::Deserialize`

## Constants {#constants}

| Name | Value | Description |
|---|---|---|
| <span id="ALL"></span>`ALL` | `0` |  |
| <span id="CONSTRUCTION"></span>`CONSTRUCTION` | `1` |  |
| <span id="NATURE"></span>`NATURE` | `2` |  |
| <span id="EQUIPMENT"></span>`EQUIPMENT` | `3` |  |
| <span id="ITEMS"></span>`ITEMS` | `4` |  |
| <span id="COMMAND_ONLY"></span>`COMMAND_ONLY` | `5` | Reachable by commands only: barriers, command blocks, structure blocks and the like. |
| <span id="UNDEFINED"></span>`UNDEFINED` | `6` |  |
