# levilamina::block

Blocks: a cell addressed by a dimension plus a coordinate.

**There are two write paths, and the native one is the default**

[`Block::set`](block.md#Block.set) goes through `set_block`, meaning `BlockSource::setBlock`, and takes a name or
full SNBT. [`Block::set_states`](block.md#Block.set_states) and [`Block::set_nbt`](block.md#Block.set_nbt) go through `edit_*` and add a
[`BlockUpdate`](types.md#BlockUpdate) letting the caller decide whether to notify neighbors and synchronize the
client. Turning both off during a bulk fill is an order of magnitude faster, at the cost of
resynchronizing afterwards.

**A waterlogged block needs the liquid layer**

Waterlogging in Bedrock is not a block state but a second block in the same cell: the main layer
is the stair and the liquid layer is the water. [`Block::name`](block.md#Block.name) sees only the main layer, so
copying and pasting a waterlogged stair loses the water entirely: the main layer is exact and
the water is gone. Moving the water with it means reading and writing [`Block::extra`](block.md#Block.extra).

## `Block` {#Block}

```rust
pub struct Block {
    // private fields
}
```

One cell in the world.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`, `Hash`, `Display`

### `Block::at` {#Block.at}

```rust
pub fn at(dim: i32, x: i32, y: i32, z: i32) -> Block
```

- Parameters:
    - dim : `i32`
    - x : `i32`
    - y : `i32`
    - z : `i32`
- Return type: `Block`

### `Block::at_pos` {#Block.at_pos}

```rust
pub fn at_pos(dim: i32, pos: PositionI32) -> Block
```

- Parameters:
    - dim : `i32`
    - pos : `PositionI32`
- Return type: `Block`

### `Block::dimension` {#Block.dimension}

```rust
pub fn dimension(&self) -> i32
```

- Return type: `i32`

### `Block::position` {#Block.position}

```rust
pub fn position(&self) -> PositionI32
```

- Return type: `PositionI32`

### `Block::read` {#Block.read}

```rust
pub fn read(&self) -> Result<BlockInfo>
```

The type name and the full SNBT, both from one call.

- Return type: `Result<BlockInfo>`
- Slots: [`get_block`](../cpp/world.md#get_block)

### `Block::to_nbt` {#Block.to_nbt}

```rust
pub fn to_nbt(&self) -> Result<NbtValue>
```

Parses the full serialization into an NBT tree. Writing it back unchanged uses
[`Block::set_nbt`](block.md#Block.set_nbt), a path that does not go through the parser of this layer.

- Return type: `Result<NbtValue>`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `Block::num` {#Block.num}

```rust
pub fn num(&self, prop: i32) -> Result<f64>
```

Reads a `PIER_BPROP_*` numeric property.

- Parameters:
    - prop : `i32`
- Return type: `Result<f64>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::text` {#Block.text}

```rust
pub fn text(&self, prop: i32) -> Result<String>
```

Reads a `PIER_BSTR_*` string property.

- Parameters:
    - prop : `i32`
- Return type: `Result<String>`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `Block::tags` {#Block.tags}

```rust
pub fn tags(&self) -> Result<Vec<String>>
```

The block tags.

- Return type: `Result<Vec<String>>`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `Block::act` {#Block.act}

```rust
pub fn act(&self, action: i32, sarg: &str) -> Result<String>
```

Runs a `PIER_BACT_*` action.

- Parameters:
    - action : `i32`
    - sarg : `&str`
- Return type: `Result<String>`
- Slots: [`block_action`](../cpp/block.md#block_action)

### `Block::has_tag` {#Block.has_tag}

```rust
pub fn has_tag(&self, tag: &str) -> Result<bool>
```

- Parameters:
    - tag : `&str`
- Return type: `Result<bool>`
- Slots: [`block_action`](../cpp/block.md#block_action)

### `Block::as_item` {#Block.as_item}

```rust
pub fn as_item(&self) -> Result<crate::item::ItemStack>
```

Treats this cell as an item, through `Block::asItemInstance`.

- Return type: `Result<crate::item::ItemStack>`
- Slots: [`block_action`](../cpp/block.md#block_action)

### `Block::pop_resource` {#Block.pop_resource}

```rust
pub fn pop_resource(&self, item: &crate::item::ItemStack) -> Result<()>
```

Drops one item at this cell.

- Parameters:
    - item : `&crate::item::ItemStack`
- Return type: `Result<()>`
- Slots: [`block_action`](../cpp/block.md#block_action)

### `Block::set` {#Block.set}

```rust
pub fn set(&self, spec: &str) -> Result<()>
```

Places a block. `spec` is a name such as `"minecraft:stone"`, or full SNBT.

An unrecognized name fails and no placeholder block is put down, whose symptom would
be a patch of purple-and-black in the world with no visible origin.

- Parameters:
    - spec : `&str`
- Return type: `Result<()>`
- Slots: [`set_block`](../cpp/world.md#set_block)

### `Block::set_nbt` {#Block.set_nbt}

```rust
pub fn set_nbt(&self, snbt: &str, update: BlockUpdate) -> Result<()>
```

Places a block from full NBT, deciding the update flags yourself.

- Parameters:
    - snbt : `&str`
    - update : `BlockUpdate`
- Return type: `Result<()>`
- Slots: [`edit_set_block_nbt`](../cpp/edit.md#edit_set_block_nbt)

### `Block::set_states` {#Block.set_states}

```rust
pub fn set_states(self, name: &str, states: Option<&str>, update: BlockUpdate) -> Result<()>
```

Places a block by name plus a subset of its states.

A `states` of `None` means every state at its default. The host takes the version
number from the default states and a caller must not fill it in: a wrong version number
lands the block under a different set of state meanings.

- Parameters:
    - name : `&str`
    - states : `Option<&str>`
    - update : `BlockUpdate`
- Return type: `Result<()>`
- Slots: [`edit_set_block_states`](../cpp/edit.md#edit_set_block_states)

### `Block::extra` {#Block.extra}

```rust
pub fn extra(&self) -> Result<String>
```

Reads the liquid layer. An empty one reads back as `"minecraft:air"` and is not an
error.

- Return type: `Result<String>`
- Slots: [`get_extra_block`](../cpp/world.md#get_extra_block)

### `Block::set_extra` {#Block.set_extra}

```rust
pub fn set_extra(&self, spec: &str, update: BlockUpdate) -> Result<()>
```

Writes the liquid layer. Writing `"minecraft:air"` clears it.

- Parameters:
    - spec : `&str`
    - update : `BlockUpdate`
- Return type: `Result<()>`
- Slots: [`set_extra_block`](../cpp/world.md#set_extra_block)

### `Block::container` {#Block.container}

```rust
pub fn container(&self) -> crate::container::Container
```

The container at this cell, a chest or a hopper. It does not check whether one is
really there, since checking would cross the ABI once, and the returned
[`crate::container::Container`](container.md#Container) reports it naturally the first time it is used.

- Return type: `crate::container::Container`

### `Block::name` {#Block.name}

```rust
pub fn name(&self) -> Result<String>
```

- Return type: `Result<String>`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `Block::snbt` {#Block.snbt}

```rust
pub fn snbt(&self) -> Result<String>
```

- Return type: `Result<String>`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `Block::description_id` {#Block.description_id}

```rust
pub fn description_id(&self) -> Result<String>
```

The localization key, not the text that is displayed.

- Return type: `Result<String>`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `Block::display_name` {#Block.display_name}

```rust
pub fn display_name(&self) -> Result<String>
```

The localized display name.

- Return type: `Result<String>`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `Block::debug_string` {#Block.debug_string}

```rust
pub fn debug_string(&self) -> Result<String>
```

The engine's own debug string. Its format follows the version, so no decision rests on
it.

- Return type: `Result<String>`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `Block::is_air` {#Block.is_air}

```rust
pub fn is_air(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_solid` {#Block.is_solid}

```rust
pub fn is_solid(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_container` {#Block.is_container}

```rust
pub fn is_container(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_door` {#Block.is_door}

```rust
pub fn is_door(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_fence` {#Block.is_fence}

```rust
pub fn is_fence(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_rail` {#Block.is_rail}

```rust
pub fn is_rail(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_slab` {#Block.is_slab}

```rust
pub fn is_slab(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_stair` {#Block.is_stair}

```rust
pub fn is_stair(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_wall` {#Block.is_wall}

```rust
pub fn is_wall(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_crop` {#Block.is_crop}

```rust
pub fn is_crop(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_unbreakable` {#Block.is_unbreakable}

```rust
pub fn is_unbreakable(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_crafting_block` {#Block.is_crafting_block}

```rust
pub fn is_crafting_block(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_interactive_block` {#Block.is_interactive_block}

```rust
pub fn is_interactive_block(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_signal_source` {#Block.is_signal_source}

```rust
pub fn is_signal_source(&self) -> Result<bool>
```

It can produce a redstone signal itself, as a lever or a button does.

- Return type: `Result<bool>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::requires_tool` {#Block.requires_tool}

```rust
pub fn requires_tool(&self) -> Result<bool>
```

It drops nothing unless mined with the right tool.

- Return type: `Result<bool>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::has_block_entity` {#Block.has_block_entity}

```rust
pub fn has_block_entity(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::data` {#Block.data}

```rust
pub fn data(&self) -> Result<i32>
```

The legacy data value. A newer block uses [`Block::states`](block.md#Block.states).

- Return type: `Result<i32>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::variant` {#Block.variant}

```rust
pub fn variant(&self) -> Result<i32>
```

The `variant` data value, whose meaning differs per actor kind.

- Return type: `Result<i32>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::block_item_id` {#Block.block_item_id}

```rust
pub fn block_item_id(&self) -> Result<i32>
```

The numeric id of the matching item.

- Return type: `Result<i32>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::light` {#Block.light}

```rust
pub fn light(&self) -> Result<i32>
```

The actual brightness of this cell, skylight included.

- Return type: `Result<i32>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::light_emission` {#Block.light_emission}

```rust
pub fn light_emission(&self) -> Result<i32>
```

How much light this block emits itself.

- Return type: `Result<i32>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::destroy_speed` {#Block.destroy_speed}

```rust
pub fn destroy_speed(&self) -> Result<f64>
```

The mining hardness; larger is slower.

- Return type: `Result<f64>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::explosion_resistance` {#Block.explosion_resistance}

```rust
pub fn explosion_resistance(&self) -> Result<f64>
```

The blast resistance.

- Return type: `Result<f64>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::friction` {#Block.friction}

```rust
pub fn friction(&self) -> Result<f64>
```

The friction coefficient; ice is one of the low ones.

- Return type: `Result<f64>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::bounciness` {#Block.bounciness}

```rust
pub fn bounciness(&self) -> Result<f64>
```

The bounciness; a slime block is non-zero.

- Return type: `Result<f64>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::burn_odds` {#Block.burn_odds}

```rust
pub fn burn_odds(&self) -> Result<i32>
```

The probability weight of catching fire.

- Return type: `Result<i32>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::flame_odds` {#Block.flame_odds}

```rust
pub fn flame_odds(&self) -> Result<i32>
```

The probability weight of spreading fire to a neighbor.

- Return type: `Result<i32>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::redstone_signal` {#Block.redstone_signal}

```rust
pub fn redstone_signal(&self) -> Result<i32>
```

The redstone signal strength this cell outputs, from 0 to 15.

- Return type: `Result<i32>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::comparator_signal` {#Block.comparator_signal}

```rust
pub fn comparator_signal(&self) -> Result<i32>
```

The strength a comparator reads from this cell; a container computes it from how full
it is.

- Return type: `Result<i32>`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Block::state` {#Block.state}

```rust
pub fn state(&self, name: &str) -> Result<String>
```

Reads the value of one block state.

- Parameters:
    - name : `&str`
- Return type: `Result<String>`
- Slots: [`block_get_state`](../cpp/block.md#block_get_state)

### `Block::states` {#Block.states}

```rust
pub fn states(&self) -> Result<NbtValue>
```

Every block state.

- Return type: `Result<NbtValue>`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `Block::set_state` {#Block.set_state}

```rust
pub fn set_state(&self, name: &str, value: &str) -> Result<()>
```

- Parameters:
    - name : `&str`
    - value : `&str`
- Return type: `Result<()>`
- Slots: [`block_set_state`](../cpp/block.md#block_set_state)

### `Block::collision_shape` {#Block.collision_shape}

```rust
pub fn collision_shape(&self) -> Result<Vec<Bounds>>
```

The collision box.

- Return type: `Result<Vec<Bounds>>`
- Slots: [`block_get_collision_shape`](../cpp/block.md#block_get_collision_shape)

### `Block::block_entity` {#Block.block_entity}

```rust
pub fn block_entity(&self) -> Result<Option<NbtValue>>
```

The NBT of the block entity. A cell with no block entity gives `Ok(None)`.

- Return type: `Result<Option<NbtValue>>`
- Slots: [`block_entity_snbt`](../cpp/block.md#block_entity_snbt)

### `Block::set_block_entity` {#Block.set_block_entity}

```rust
pub fn set_block_entity(&self, snbt: &str) -> Result<()>
```

Writes the NBT of the block entity back, through `BlockActor::load`.
The cell already has to hold the matching kind of block.

- Parameters:
    - snbt : `&str`
- Return type: `Result<()>`
- Slots: [`edit_set_block_entity`](../cpp/edit.md#edit_set_block_entity)

## `BlockInfo` {#BlockInfo}

```rust
pub struct BlockInfo {
    pub pos: PositionI32,
    /// The type name, such as `"minecraft:redstone_wire"`.
    pub name: String,
    /// The full serialization, `{name, states, version}`.
    pub snbt: String,
}
```

What one block cell reads back as.

- Implements: `Debug`, `Clone`, `PartialEq`, `Eq`

### `BlockInfo::is_air` {#BlockInfo.is_air}

```rust
pub fn is_air(&self) -> bool
```

- Return type: `bool`
