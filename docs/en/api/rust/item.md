# levilamina::item

Items: a value object and not a handle.

On the ABI an item is a string of SNBT throughout. Reading a property means asking a
property with that SNBT, and changing one means exchanging that SNBT for a new one
through `item_transform`. This layer copies that shape and hides no pointer in between.

**The consequence: an `ItemStack` has no connection to the item in the world**

`container.item(0)` returns a snapshot. Changing it does not move the one in the
container, and writing it back takes an explicit `container.set_item(0, &stack)`. This
is the other side of a handle being an identity and not a pointer: with no implicit
synchronization in between, there is no I changed it and nothing happened.

## `ItemStack` {#ItemStack}

```rust
pub struct ItemStack {
    // private fields
}
```

An SNBT snapshot of one item.

The fields the snapshot itself carries, `Name`, `Count`, `Damage` and the `tag`
compound, are read locally from a parse cached on first use; only properties that need
the item registry, such as the stack limit or the attack damage, cross the ABI, where the
host parses the SNBT and builds an engine ItemStack for every call.

- Implements: `Clone`, `PartialEq`, `Eq`, `Hash`, `Debug`, `Display`, `From`

### `ItemStack::from_snbt` {#ItemStack.from_snbt}

```rust
pub fn from_snbt(snbt: impl Into<String>) -> ItemStack
```

Uses a string of SNBT directly, without validation, since validating would cross the
ABI once and this constructor sits on a hot path.
A wrong shape reports an error at the first call that really uses it.

- Parameters:
    - snbt : `impl Into<String>`
- Return type: `ItemStack`

### `ItemStack::create` {#ItemStack.create}

```rust
pub fn create(type_name: &str, count: u8) -> ItemStack
```

Builds one from a type name and a count.

It assembles the minimal shape `{Name:"...",Count:Nb}` and the engine fills in the rest
inside `ItemStack::fromTag`. A name that does not exist fails at the moment the item is
used and not here.

- Parameters:
    - type_name : `&str`
    - count : `u8`
- Return type: `ItemStack`

### `ItemStack::empty` {#ItemStack.empty}

```rust
pub fn empty() -> ItemStack
```

Air. An empty slot in a container reads back as this.

- Return type: `ItemStack`

### `ItemStack::snbt` {#ItemStack.snbt}

```rust
pub fn snbt(&self) -> &str
```

The underlying SNBT.

- Return type: `&str`

### `ItemStack::to_nbt` {#ItemStack.to_nbt}

```rust
pub fn to_nbt(&self) -> Result<NbtValue>
```

Parses into an NBT tree, for reading a field the ABI gives no named accessor for.

- Return type: `Result<NbtValue>`

### `ItemStack::num` {#ItemStack.num}

```rust
pub fn num(&self, prop: i32) -> Result<f64>
```

Reads a `PIER_IPROP_*` numeric property.

A property number the host does not recognize returns `Err` and not 0:
cannot-be-determined and an answer of 0 must stay apart (contract §5.2).

- Parameters:
    - prop : `i32`
- Return type: `Result<f64>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::count` {#ItemStack.count}

```rust
pub fn count(&self) -> Result<u8>
```

- Return type: `Result<u8>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::max_stack_size` {#ItemStack.max_stack_size}

```rust
pub fn max_stack_size(&self) -> Result<u8>
```

- Return type: `Result<u8>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::aux_value` {#ItemStack.aux_value}

```rust
pub fn aux_value(&self) -> Result<i32>
```

- Return type: `Result<i32>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::id` {#ItemStack.id}

```rust
pub fn id(&self) -> Result<i32>
```

- Return type: `Result<i32>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::damage` {#ItemStack.damage}

```rust
pub fn damage(&self) -> Result<i32>
```

- Return type: `Result<i32>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::max_damage` {#ItemStack.max_damage}

```rust
pub fn max_damage(&self) -> Result<i32>
```

- Return type: `Result<i32>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::attack_damage` {#ItemStack.attack_damage}

```rust
pub fn attack_damage(&self) -> Result<i32>
```

- Return type: `Result<i32>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::repair_cost` {#ItemStack.repair_cost}

```rust
pub fn repair_cost(&self) -> Result<i32>
```

- Return type: `Result<i32>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::enchant_value` {#ItemStack.enchant_value}

```rust
pub fn enchant_value(&self) -> Result<i32>
```

- Return type: `Result<i32>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::use_duration` {#ItemStack.use_duration}

```rust
pub fn use_duration(&self) -> Result<i32>
```

- Return type: `Result<i32>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_null` {#ItemStack.is_null}

```rust
pub fn is_null(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_block` {#ItemStack.is_block}

```rust
pub fn is_block(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_enchanted` {#ItemStack.is_enchanted}

```rust
pub fn is_enchanted(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_armor` {#ItemStack.is_armor}

```rust
pub fn is_armor(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_damageable` {#ItemStack.is_damageable}

```rust
pub fn is_damageable(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_damaged` {#ItemStack.is_damaged}

```rust
pub fn is_damaged(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_unbreakable` {#ItemStack.is_unbreakable}

```rust
pub fn is_unbreakable(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::has_durability` {#ItemStack.has_durability}

```rust
pub fn has_durability(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_potion` {#ItemStack.is_potion}

```rust
pub fn is_potion(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_throwable` {#ItemStack.is_throwable}

```rust
pub fn is_throwable(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_fire_resistant` {#ItemStack.is_fire_resistant}

```rust
pub fn is_fire_resistant(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_stackable` {#ItemStack.is_stackable}

```rust
pub fn is_stackable(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_music_disc` {#ItemStack.is_music_disc}

```rust
pub fn is_music_disc(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_offhand` {#ItemStack.is_offhand}

```rust
pub fn is_offhand(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_glint` {#ItemStack.is_glint}

```rust
pub fn is_glint(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_bundle` {#ItemStack.is_bundle}

```rust
pub fn is_bundle(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::has_user_data` {#ItemStack.has_user_data}

```rust
pub fn has_user_data(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::has_custom_name` {#ItemStack.has_custom_name}

```rust
pub fn has_custom_name(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::text` {#ItemStack.text}

```rust
pub fn text(&self, prop: i32) -> Result<String>
```

Reads a `PIER_ISTR_*` string property.

- Parameters:
    - prop : `i32`
- Return type: `Result<String>`
- Slots: [`item_get_str`](../cpp/item.md#item_get_str)

### `ItemStack::type_name` {#ItemStack.type_name}

```rust
pub fn type_name(&self) -> Result<String>
```

- Return type: `Result<String>`
- Slots: [`item_get_str`](../cpp/item.md#item_get_str)

### `ItemStack::name` {#ItemStack.name}

```rust
pub fn name(&self) -> Result<String>
```

- Return type: `Result<String>`
- Slots: [`item_get_str`](../cpp/item.md#item_get_str)

### `ItemStack::custom_name` {#ItemStack.custom_name}

```rust
pub fn custom_name(&self) -> Result<String>
```

- Return type: `Result<String>`
- Slots: [`item_get_str`](../cpp/item.md#item_get_str)

### `ItemStack::hover_name` {#ItemStack.hover_name}

```rust
pub fn hover_name(&self) -> Result<String>
```

- Return type: `Result<String>`
- Slots: [`item_get_str`](../cpp/item.md#item_get_str)

### `ItemStack::raw_name_id` {#ItemStack.raw_name_id}

```rust
pub fn raw_name_id(&self) -> Result<String>
```

- Return type: `Result<String>`
- Slots: [`item_get_str`](../cpp/item.md#item_get_str)

### `ItemStack::effect_name` {#ItemStack.effect_name}

```rust
pub fn effect_name(&self) -> Result<String>
```

- Return type: `Result<String>`
- Slots: [`item_get_str`](../cpp/item.md#item_get_str)

### `ItemStack::lore` {#ItemStack.lore}

```rust
pub fn lore(&self) -> Result<Vec<String>>
```

The custom lore. The host gives an SNBT string list, parsed here into a `Vec<String>`.

- Return type: `Result<Vec<String>>`
- Slots: [`item_get_str`](../cpp/item.md#item_get_str)

### `ItemStack::can_destroy` {#ItemStack.can_destroy}

```rust
pub fn can_destroy(&self) -> Result<Vec<String>>
```

- Return type: `Result<Vec<String>>`
- Slots: [`item_get_str`](../cpp/item.md#item_get_str)

### `ItemStack::can_place_on` {#ItemStack.can_place_on}

```rust
pub fn can_place_on(&self) -> Result<Vec<String>>
```

- Return type: `Result<Vec<String>>`
- Slots: [`item_get_str`](../cpp/item.md#item_get_str)

### `ItemStack::user_data` {#ItemStack.user_data}

```rust
pub fn user_data(&self) -> Result<NbtValue>
```

The custom NBT of the item, the `tag` section. It goes through a dedicated slot rather
than `PIER_ISTR_USER_DATA`: the content is the same and the dedicated slot saves one
property-number dispatch on the host side.

- Return type: `Result<NbtValue>`
- Slots: [`item_get_user_data`](../cpp/item.md#item_get_user_data)

### `ItemStack::color` {#ItemStack.color}

```rust
pub fn color(&self) -> Result<(i32, i32, i32)>
```

The color as `{r,g,b}`. Only a dyeable item has one.

- Return type: `Result<(i32, i32, i32)>`
- Slots: [`item_get_str`](../cpp/item.md#item_get_str)

### `ItemStack::transform` {#ItemStack.transform}

```rust
pub fn transform(&mut self, op: i32, sarg: &str, narg: f64) -> Result<()>
```

Runs one `PIER_IOP_*` transform and replaces itself with the result.

On failure it stays unchanged: with no new SNBT from the host there is nothing to write
back, and writing half a result in is harder to diagnose than doing nothing.

- Parameters:
    - op : `i32`
    - sarg : `&str`
    - narg : `f64`
- Return type: `Result<()>`
- Slots: [`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::transformed` {#ItemStack.transformed}

```rust
pub fn transformed(&self, op: i32, sarg: &str, narg: f64) -> Result<ItemStack>
```

As above, returning a new item and leaving this one unchanged.

- Parameters:
    - op : `i32`
    - sarg : `&str`
    - narg : `f64`
- Return type: `Result<ItemStack>`

### `ItemStack::set_custom_name` {#ItemStack.set_custom_name}

```rust
pub fn set_custom_name(&mut self, name: &str) -> Result<()>
```

- Parameters:
    - name : `&str`
- Return type: `Result<()>`
- Slots: [`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::reset_name` {#ItemStack.reset_name}

```rust
pub fn reset_name(&mut self) -> Result<()>
```

- Return type: `Result<()>`
- Slots: [`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::set_damage` {#ItemStack.set_damage}

```rust
pub fn set_damage(&mut self, damage: i32) -> Result<()>
```

- Parameters:
    - damage : `i32`
- Return type: `Result<()>`
- Slots: [`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::set_count` {#ItemStack.set_count}

```rust
pub fn set_count(&mut self, count: u8) -> Result<()>
```

- Parameters:
    - count : `u8`
- Return type: `Result<()>`
- Slots: [`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::set_unbreakable` {#ItemStack.set_unbreakable}

```rust
pub fn set_unbreakable(&mut self, on: bool) -> Result<()>
```

- Parameters:
    - on : `bool`
- Return type: `Result<()>`
- Slots: [`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::hurt_and_break` {#ItemStack.hurt_and_break}

```rust
pub fn hurt_and_break(&mut self, damage: i32) -> Result<()>
```

- Parameters:
    - damage : `i32`
- Return type: `Result<()>`
- Slots: [`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::set_repair_cost` {#ItemStack.set_repair_cost}

```rust
pub fn set_repair_cost(&mut self, cost: i32) -> Result<()>
```

- Parameters:
    - cost : `i32`
- Return type: `Result<()>`
- Slots: [`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::clear_lore` {#ItemStack.clear_lore}

```rust
pub fn clear_lore(&mut self) -> Result<()>
```

- Return type: `Result<()>`
- Slots: [`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::remove_enchants` {#ItemStack.remove_enchants}

```rust
pub fn remove_enchants(&mut self) -> Result<()>
```

- Return type: `Result<()>`
- Slots: [`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::set_lore` {#ItemStack.set_lore}

```rust
pub fn set_lore(&mut self, lines: &[&str]) -> Result<()>
```

- Parameters:
    - lines : `&[&str]`
- Return type: `Result<()>`
- Slots: [`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::set_can_destroy` {#ItemStack.set_can_destroy}

```rust
pub fn set_can_destroy(&mut self, blocks: &[&str]) -> Result<()>
```

- Parameters:
    - blocks : `&[&str]`
- Return type: `Result<()>`
- Slots: [`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::set_can_place_on` {#ItemStack.set_can_place_on}

```rust
pub fn set_can_place_on(&mut self, blocks: &[&str]) -> Result<()>
```

- Parameters:
    - blocks : `&[&str]`
- Return type: `Result<()>`
- Slots: [`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::add_enchant` {#ItemStack.add_enchant}

```rust
pub fn add_enchant(&mut self, id: &str, level: i32) -> Result<()>
```

Adds an enchantment. A level of 0 removes it, as far as the engine is concerned.

- Parameters:
    - id : `&str`
    - level : `i32`
- Return type: `Result<()>`
- Slots: [`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::enchants` {#ItemStack.enchants}

```rust
pub fn enchants(&self) -> Result<Vec<Enchant>>
```

- Return type: `Result<Vec<Enchant>>`
- Slots: [`item_get_enchants`](../cpp/item.md#item_get_enchants)

### `ItemStack::with_enchants` {#ItemStack.with_enchants}

```rust
pub fn with_enchants(&self, enchants: &[Enchant]) -> Result<ItemStack>
```

Replaces the whole enchantment set and returns a new item.

- Parameters:
    - enchants : `&[Enchant]`
- Return type: `Result<ItemStack>`
- Slots: [`item_set_enchants`](../cpp/item.md#item_set_enchants)

### `ItemStack::matches` {#ItemStack.matches}

```rust
pub fn matches(&self, other: &ItemStack) -> Result<bool>
```

Whether two items are the same kind of thing.

The criterion comes from the engine, `ItemStack::matches`, and is not string equality:
fields such as count and durability take no part, while comparing SNBT text would
include them.
A missing slot returns `Err` and does not fall back to a text comparison, which would
make the criterion differ between hosts.

- Parameters:
    - other : `&ItemStack`
- Return type: `Result<bool>`
- Slots: [`item_matches`](../cpp/item.md#item_matches)

## `Enchant` {#Enchant}

```rust
pub struct Enchant {
    /// The enchantment id. Whether the host reports a numeric id or a name depends on the
    /// BDS version, and it is carried through unchanged.
    pub id: String,
    pub level: i32,
}
```

One enchantment.

- Implements: `Debug`, `Clone`, `PartialEq`, `Eq`
