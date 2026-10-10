# Zig：方块

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

一个维度里的一个方块格。

### `BlockAt.at` {#BlockAt.at}

```zig
pub fn at(at_dim: i32, at_x: i32, at_y: i32, at_z: i32) BlockAt
```

- 参数：
    - at_dim : `i32`
    - at_x : `i32`
    - at_y : `i32`
    - at_z : `i32`
- 返回值类型：`BlockAt`

### `BlockAt.isAir` {#BlockAt.isAir}

```zig
pub fn isAir(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_AIR`：取自 `Block::isAir`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.data` {#BlockAt.data}

```zig
pub fn data(self: BlockAt) core.Error!f64
```

`PIER_BPROP_DATA`：取自 `Block::getData`（旧版的数据值）

- 返回值类型：`core.Error!f64`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.blockItemId` {#BlockAt.blockItemId}

```zig
pub fn blockItemId(self: BlockAt) core.Error!f64
```

`PIER_BPROP_BLOCK_ITEM_ID`：取自 `Block::getBlockItemId`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isCraftingBlock` {#BlockAt.isCraftingBlock}

```zig
pub fn isCraftingBlock(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_CRAFTING_BLOCK`：取自 `Block::isCraftingBlock`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isInteractiveBlock` {#BlockAt.isInteractiveBlock}

```zig
pub fn isInteractiveBlock(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_INTERACTIVE_BLOCK`：取自 `Block::isInteractiveBlock`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.hasBlockEntity` {#BlockAt.hasBlockEntity}

```zig
pub fn hasBlockEntity(self: BlockAt) core.Error!bool
```

`PIER_BPROP_HAS_BLOCK_ENTITY`：`BlockSource::getBlockEntity(pos) != null`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.light` {#BlockAt.light}

```zig
pub fn light(self: BlockAt) core.Error!f64
```

`PIER_BPROP_LIGHT`：取自 `Block::getLight`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.lightEmission` {#BlockAt.lightEmission}

```zig
pub fn lightEmission(self: BlockAt) core.Error!f64
```

`PIER_BPROP_LIGHT_EMISSION`：取自 `Block::getLightEmission`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.destroySpeed` {#BlockAt.destroySpeed}

```zig
pub fn destroySpeed(self: BlockAt) core.Error!f64
```

`PIER_BPROP_DESTROY_SPEED`：取自 `Block::getDestroySpeed`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.explosionResistance` {#BlockAt.explosionResistance}

```zig
pub fn explosionResistance(self: BlockAt) core.Error!f64
```

`PIER_BPROP_EXPLOSION_RESISTANCE`：取自 `Block::getExplosionResistance`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.friction` {#BlockAt.friction}

```zig
pub fn friction(self: BlockAt) core.Error!f64
```

`PIER_BPROP_FRICTION`：取自 `Block::getFriction`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isContainer` {#BlockAt.isContainer}

```zig
pub fn isContainer(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_CONTAINER`：取自 `Block::isContainerBlock`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isDoor` {#BlockAt.isDoor}

```zig
pub fn isDoor(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_DOOR`：从 BDS 1.26.40 起不再支持：`BlockType::isDoorBlock` 已被移除

- 返回值类型：`core.Error!bool`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isFence` {#BlockAt.isFence}

```zig
pub fn isFence(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_FENCE`：取自 `Block::isFenceBlock`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isRail` {#BlockAt.isRail}

```zig
pub fn isRail(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_RAIL`：取自 `Block::isRailBlock`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isSlab` {#BlockAt.isSlab}

```zig
pub fn isSlab(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_SLAB`：取自 `Block::isSlabBlock`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isStair` {#BlockAt.isStair}

```zig
pub fn isStair(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_STAIR`：从 BDS 1.26.40 起不再支持：`BlockType::isStairBlock` 已被移除

- 返回值类型：`core.Error!bool`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isWall` {#BlockAt.isWall}

```zig
pub fn isWall(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_WALL`：取自 `Block::isWallBlock`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isCrop` {#BlockAt.isCrop}

```zig
pub fn isCrop(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_CROP`：取自 `Block::isCropBlock`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isUnbreakable` {#BlockAt.isUnbreakable}

```zig
pub fn isUnbreakable(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_UNBREAKABLE`：取自 `Block::isUnbreakable`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.redstoneSignal` {#BlockAt.redstoneSignal}

```zig
pub fn redstoneSignal(self: BlockAt) core.Error!f64
```

`PIER_BPROP_REDSTONE_SIGNAL`：取自 `Block::getDirectSignal`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.comparatorSignal` {#BlockAt.comparatorSignal}

```zig
pub fn comparatorSignal(self: BlockAt) core.Error!f64
```

`PIER_BPROP_COMPARATOR_SIGNAL`：取自 `Block::getComparatorSignal`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isSignalSource` {#BlockAt.isSignalSource}

```zig
pub fn isSignalSource(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_SIGNAL_SOURCE`：取自 `Block::isSignalSource`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.variant` {#BlockAt.variant}

```zig
pub fn variant(self: BlockAt) core.Error!f64
```

`PIER_BPROP_VARIANT`：取自 `Block::getVariant`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.burnOdds` {#BlockAt.burnOdds}

```zig
pub fn burnOdds(self: BlockAt) core.Error!f64
```

`PIER_BPROP_BURN_ODDS`：取自 `Block::getBurnOdds`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.flameOdds` {#BlockAt.flameOdds}

```zig
pub fn flameOdds(self: BlockAt) core.Error!f64
```

`PIER_BPROP_FLAME_ODDS`：取自 `Block::getFlameOdds`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.bounciness` {#BlockAt.bounciness}

```zig
pub fn bounciness(self: BlockAt) core.Error!f64
```

`PIER_BPROP_BOUNCINESS`：取自 `Block::getBounciness`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.isSolid` {#BlockAt.isSolid}

```zig
pub fn isSolid(self: BlockAt) core.Error!bool
```

`PIER_BPROP_IS_SOLID`：取自 `Block::isSolid`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.requiresTool` {#BlockAt.requiresTool}

```zig
pub fn requiresTool(self: BlockAt) core.Error!f64
```

`PIER_BPROP_REQUIRES_TOOL`：取自 `Block::requiresCorrectToolForDrops`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.typeName` {#BlockAt.typeName}

```zig
pub fn typeName(self: BlockAt, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_BSTR_TYPE_NAME`：取自 `Block::getTypeName`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.snbt` {#BlockAt.snbt}

```zig
pub fn snbt(self: BlockAt, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_BSTR_SNBT`：取自 `Block::mSerializationId`，以 SNBT `{name,states,version}` 给出

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.descriptionId` {#BlockAt.descriptionId}

```zig
pub fn descriptionId(self: BlockAt, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_BSTR_DESCRIPTION_ID`：取自 `Block::getDescriptionId`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.debugString` {#BlockAt.debugString}

```zig
pub fn debugString(self: BlockAt, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_BSTR_DEBUG_STRING`：取自 `Block::toDebugString`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.tags` {#BlockAt.tags}

```zig
pub fn tags(self: BlockAt, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_BSTR_TAGS`：取自 `Block::mTags`，以 SNBT 字符串列表 `["a","b"]` 给出

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.state` {#BlockAt.state}

```zig
pub fn state(self: BlockAt, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_BSTR_STATE`：SNBT `{state_name:value,…}`，全部方块状态

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.collisionShape` {#BlockAt.collisionShape}

```zig
pub fn collisionShape(self: BlockAt, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_BSTR_COLLISION_SHAPE`：SNBT `[{min:[x,y,z],max:[x,y,z]},…]`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.outlineShape` {#BlockAt.outlineShape}

```zig
pub fn outlineShape(self: BlockAt, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_BSTR_OUTLINE_SHAPE`：SNBT `[{min,max}]`，渲染用的轮廓

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.displayName` {#BlockAt.displayName}

```zig
pub fn displayName(self: BlockAt, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_BSTR_DISPLAY_NAME`：取自 `Block::getDisplayName`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.hasTag` {#BlockAt.hasTag}

```zig
pub fn hasTag(self: BlockAt, allocator: std.mem.Allocator, s_arg: []const u8) core.Error![]u8
```

`PIER_BACT_HAS_TAG`：`sarg` 为标签，输出 `"0"` 或 `"1"`，调用 `Block::hasTag`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`block_action`](../cpp/block.md#block_action)

### `BlockAt.getState` {#BlockAt.getState}

```zig
pub fn getState(self: BlockAt, allocator: std.mem.Allocator, s_arg: []const u8) core.Error![]u8
```

`PIER_BACT_GET_STATE`：`sarg` 为状态名，输出状态值的字符串，调用 `Block::getState`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`block_action`](../cpp/block.md#block_action)

### `BlockAt.popResource` {#BlockAt.popResource}

```zig
pub fn popResource(self: BlockAt, allocator: std.mem.Allocator, s_arg: []const u8) core.Error![]u8
```

`PIER_BACT_POP_RESOURCE`：`sarg` 为物品 SNBT，在这个位置掉落该物品，调用 `Block::popResource`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`block_action`](../cpp/block.md#block_action)

### `BlockAt.asItem` {#BlockAt.asItem}

```zig
pub fn asItem(self: BlockAt, allocator: std.mem.Allocator, s_arg: []const u8) core.Error![]u8
```

`PIER_BACT_AS_ITEM`：输出物品 SNBT，调用 `Block::asItemInstance`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`block_action`](../cpp/block.md#block_action)
