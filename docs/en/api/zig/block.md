# Zig: Blocks

## `BlockAt` {#BlockAt}

```zig
pub const BlockAt = struct {
    dim_id: i32,
    pos_x: i32,
    pos_y: i32,
    pos_z: i32,
    // ...
};
```

One block cell of a dimension.

### `BlockAt.at` {#BlockAt.at}

```zig
pub fn at(at_dim: i32, at_x: i32, at_y: i32, at_z: i32) BlockAt
```

- Parameters:
    - at_dim : `i32`
    - at_x : `i32`
    - at_y : `i32`
    - at_z : `i32`
- Return type: `BlockAt`

### `BlockAt.isAir` {#BlockAt.isAir}

```zig
pub fn isAir(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_AIR`: `Block::isAir`

- Return type: `core.Error!bool`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.data` {#BlockAt.data}

```zig
pub fn data(self: BlockAt) core.Error!f64
```

`PIER_BPROP_DATA`: `Block::getData` (legacy data value)

- Return type: `core.Error!f64`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.blockItemId` {#BlockAt.blockItemId}

```zig
pub fn blockItemId(self: BlockAt) core.Error!f64
```

`PIER_BPROP_BLOCK_ITEM_ID`: `Block::getBlockItemId`

- Return type: `core.Error!f64`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isCraftingBlock` {#BlockAt.isCraftingBlock}

```zig
pub fn isCraftingBlock(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_CRAFTING_BLOCK`: `Block::isCraftingBlock`

- Return type: `core.Error!bool`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isInteractiveBlock` {#BlockAt.isInteractiveBlock}

```zig
pub fn isInteractiveBlock(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_INTERACTIVE_BLOCK`: `Block::isInteractiveBlock`

- Return type: `core.Error!bool`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.hasBlockEntity` {#BlockAt.hasBlockEntity}

```zig
pub fn hasBlockEntity(self: BlockAt) core.Error!bool
```

`PIER_BPROP_HAS_BLOCK_ENTITY`: `BlockSource::getBlockEntity`(pos) != null

- Return type: `core.Error!bool`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.light` {#BlockAt.light}

```zig
pub fn light(self: BlockAt) core.Error!f64
```

`PIER_BPROP_LIGHT`: `Block::getLight`

- Return type: `core.Error!f64`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.lightEmission` {#BlockAt.lightEmission}

```zig
pub fn lightEmission(self: BlockAt) core.Error!f64
```

`PIER_BPROP_LIGHT_EMISSION`: `Block::getLightEmission`

- Return type: `core.Error!f64`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.destroySpeed` {#BlockAt.destroySpeed}

```zig
pub fn destroySpeed(self: BlockAt) core.Error!f64
```

`PIER_BPROP_DESTROY_SPEED`: `Block::getDestroySpeed`

- Return type: `core.Error!f64`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.explosionResistance` {#BlockAt.explosionResistance}

```zig
pub fn explosionResistance(self: BlockAt) core.Error!f64
```

`PIER_BPROP_EXPLOSION_RESISTANCE`: `Block::getExplosionResistance`

- Return type: `core.Error!f64`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.friction` {#BlockAt.friction}

```zig
pub fn friction(self: BlockAt) core.Error!f64
```

`PIER_BPROP_FRICTION`: `Block::getFriction`

- Return type: `core.Error!f64`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isContainer` {#BlockAt.isContainer}

```zig
pub fn isContainer(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_CONTAINER`: `Block::isContainerBlock`

- Return type: `core.Error!bool`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isDoor` {#BlockAt.isDoor}

```zig
pub fn isDoor(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_DOOR`: unsupported since BDS 1.26.40: `BlockType::isDoorBlock` is gone

- Return type: `core.Error!bool`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isFence` {#BlockAt.isFence}

```zig
pub fn isFence(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_FENCE`: `Block::isFenceBlock`

- Return type: `core.Error!bool`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isRail` {#BlockAt.isRail}

```zig
pub fn isRail(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_RAIL`: `Block::isRailBlock`

- Return type: `core.Error!bool`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isSlab` {#BlockAt.isSlab}

```zig
pub fn isSlab(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_SLAB`: `Block::isSlabBlock`

- Return type: `core.Error!bool`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isStair` {#BlockAt.isStair}

```zig
pub fn isStair(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_STAIR`: unsupported since BDS 1.26.40: `BlockType::isStairBlock` is gone

- Return type: `core.Error!bool`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isWall` {#BlockAt.isWall}

```zig
pub fn isWall(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_WALL`: `Block::isWallBlock`

- Return type: `core.Error!bool`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isCrop` {#BlockAt.isCrop}

```zig
pub fn isCrop(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_CROP`: `Block::isCropBlock`

- Return type: `core.Error!bool`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isUnbreakable` {#BlockAt.isUnbreakable}

```zig
pub fn isUnbreakable(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_UNBREAKABLE`: `Block::isUnbreakable`

- Return type: `core.Error!bool`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.redstoneSignal` {#BlockAt.redstoneSignal}

```zig
pub fn redstoneSignal(self: BlockAt) core.Error!f64
```

`PIER_BPROP_REDSTONE_SIGNAL`: `Block::getDirectSignal`

- Return type: `core.Error!f64`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.comparatorSignal` {#BlockAt.comparatorSignal}

```zig
pub fn comparatorSignal(self: BlockAt) core.Error!f64
```

`PIER_BPROP_COMPARATOR_SIGNAL`: `Block::getComparatorSignal`

- Return type: `core.Error!f64`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isSignalSource` {#BlockAt.isSignalSource}

```zig
pub fn isSignalSource(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_SIGNAL_SOURCE`: `Block::isSignalSource`

- Return type: `core.Error!bool`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.variant` {#BlockAt.variant}

```zig
pub fn variant(self: BlockAt) core.Error!f64
```

`PIER_BPROP_VARIANT`: `Block::getVariant`

- Return type: `core.Error!f64`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.burnOdds` {#BlockAt.burnOdds}

```zig
pub fn burnOdds(self: BlockAt) core.Error!f64
```

`PIER_BPROP_BURN_ODDS`: `Block::getBurnOdds`

- Return type: `core.Error!f64`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.flameOdds` {#BlockAt.flameOdds}

```zig
pub fn flameOdds(self: BlockAt) core.Error!f64
```

`PIER_BPROP_FLAME_ODDS`: `Block::getFlameOdds`

- Return type: `core.Error!f64`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.bounciness` {#BlockAt.bounciness}

```zig
pub fn bounciness(self: BlockAt) core.Error!f64
```

`PIER_BPROP_BOUNCINESS`: `Block::getBounciness`

- Return type: `core.Error!f64`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isSolid` {#BlockAt.isSolid}

```zig
pub fn isSolid(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_SOLID`: `Block::isSolid`

- Return type: `core.Error!bool`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.requiresTool` {#BlockAt.requiresTool}

```zig
pub fn requiresTool(self: BlockAt) core.Error!f64
```

`PIER_BPROP_REQUIRES_TOOL`: `Block::requiresCorrectToolForDrops`

- Return type: `core.Error!f64`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.typeName` {#BlockAt.typeName}

```zig
pub fn typeName(self: BlockAt, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_BSTR_TYPE_NAME`: `Block::getTypeName`

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.snbt` {#BlockAt.snbt}

```zig
pub fn snbt(self: BlockAt, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_BSTR_SNBT`: `Block::mSerializationId` → SNBT {name,states,version}

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.descriptionId` {#BlockAt.descriptionId}

```zig
pub fn descriptionId(self: BlockAt, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_BSTR_DESCRIPTION_ID`: `Block::getDescriptionId`

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.debugString` {#BlockAt.debugString}

```zig
pub fn debugString(self: BlockAt, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_BSTR_DEBUG_STRING`: `Block::toDebugString`

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.tags` {#BlockAt.tags}

```zig
pub fn tags(self: BlockAt, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_BSTR_TAGS`: `Block::mTags` → SNBT string list \["a","b"\]

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.state` {#BlockAt.state}

```zig
pub fn state(self: BlockAt, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_BSTR_STATE`: SNBT {`state_name`:value,…} all block states

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.collisionShape` {#BlockAt.collisionShape}

```zig
pub fn collisionShape(self: BlockAt, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_BSTR_COLLISION_SHAPE`: SNBT \[{min:\[x,y,z\],max:\[x,y,z\]},…\]

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.outlineShape` {#BlockAt.outlineShape}

```zig
pub fn outlineShape(self: BlockAt, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_BSTR_OUTLINE_SHAPE`: SNBT \[{min,max}\] render outline

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.displayName` {#BlockAt.displayName}

```zig
pub fn displayName(self: BlockAt, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_BSTR_DISPLAY_NAME`: `Block::getDisplayName`

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.hasTag` {#BlockAt.hasTag}

```zig
pub fn hasTag(self: BlockAt, allocator: std.mem.Allocator, s_arg: []const u8) core.Error![]u8
```

`PIER_BACT_HAS_TAG`: sarg=tag → out "0"/"1" `Block::hasTag`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
- Return type: `core.Error![]u8`
- Slots: [`block_action`](../cpp/block.md#block_action)

### `BlockAt.getState` {#BlockAt.getState}

```zig
pub fn getState(self: BlockAt, allocator: std.mem.Allocator, s_arg: []const u8) core.Error![]u8
```

`PIER_BACT_GET_STATE`: sarg=state name → out value string `Block::getState`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
- Return type: `core.Error![]u8`
- Slots: [`block_action`](../cpp/block.md#block_action)

### `BlockAt.popResource` {#BlockAt.popResource}

```zig
pub fn popResource(self: BlockAt, allocator: std.mem.Allocator, s_arg: []const u8) core.Error![]u8
```

`PIER_BACT_POP_RESOURCE`: sarg=item SNBT → pop resource at pos `Block::popResource`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
- Return type: `core.Error![]u8`
- Slots: [`block_action`](../cpp/block.md#block_action)

### `BlockAt.asItem` {#BlockAt.asItem}

```zig
pub fn asItem(self: BlockAt, allocator: std.mem.Allocator, s_arg: []const u8) core.Error![]u8
```

`PIER_BACT_AS_ITEM`: → out item SNBT `Block::asItemInstance`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
- Return type: `core.Error![]u8`
- Slots: [`block_action`](../cpp/block.md#block_action)
