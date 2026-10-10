# 方块

??? note "abi.h 里的分节说明"

    **§D 方块与方块实体（`§D blocks & block entities`）**

    **方块：读写状态、碰撞形状（专用函数）（`Block: state get/set, collision shape (dedicated fns)`）**

## 槽位 {#slots}

### `block_get_num` {#block_get_num}

```c
bool (*block_get_num)(int32_t dim, int32_t x, int32_t y, int32_t z, int32_t prop, double* out);
```

- 调用形式：`api->block_get_num(dim, x, y, z, prop, out)`
- 参数：
    - dim : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - prop : `int32_t`
    - out : `double*`
- 返回值类型：`bool`
- 所在分节：§D 方块与方块实体（`§D blocks & block entities`）
- 表内序号：第 39 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Block::num`](../rust/block.md#Block.num)、[`Block::is_air`](../rust/block.md#Block.is_air)、[`Block::is_solid`](../rust/block.md#Block.is_solid)、[`Block::is_container`](../rust/block.md#Block.is_container)、[`Block::is_door`](../rust/block.md#Block.is_door)、[`Block::is_fence`](../rust/block.md#Block.is_fence) 等，共 30 个
    - Go：[`BlockAt.IsAir`](../go/block.md#BlockAt.IsAir)、[`BlockAt.Data`](../go/block.md#BlockAt.Data)、[`BlockAt.BlockItemId`](../go/block.md#BlockAt.BlockItemId)、[`BlockAt.IsCraftingBlock`](../go/block.md#BlockAt.IsCraftingBlock)、[`BlockAt.IsInteractiveBlock`](../go/block.md#BlockAt.IsInteractiveBlock)、[`BlockAt.HasBlockEntity`](../go/block.md#BlockAt.HasBlockEntity) 等，共 30 个
    - Zig：[`BlockAt.isAir`](../zig/block.md#BlockAt.isAir)、[`BlockAt.data`](../zig/block.md#BlockAt.data)、[`BlockAt.blockItemId`](../zig/block.md#BlockAt.blockItemId)、[`BlockAt.isCraftingBlock`](../zig/block.md#BlockAt.isCraftingBlock)、[`BlockAt.isInteractiveBlock`](../zig/block.md#BlockAt.isInteractiveBlock)、[`BlockAt.hasBlockEntity`](../zig/block.md#BlockAt.hasBlockEntity) 等，共 29 个

### `block_get_str` {#block_get_str}

```c
bool (*block_get_str)(int32_t dim, int32_t x, int32_t y, int32_t z, int32_t prop, void* ctx, PierStrSink sink);
```

- 调用形式：`api->block_get_str(dim, x, y, z, prop, ctx, sink)`
- 参数：
    - dim : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - prop : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：§D 方块与方块实体（`§D blocks & block entities`）
- 表内序号：第 40 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Block::to_nbt`](../rust/block.md#Block.to_nbt)、[`Block::text`](../rust/block.md#Block.text)、[`Block::tags`](../rust/block.md#Block.tags)、[`Block::name`](../rust/block.md#Block.name)、[`Block::snbt`](../rust/block.md#Block.snbt)、[`Block::description_id`](../rust/block.md#Block.description_id) 等，共 9 个
    - Go：[`BlockAt.TypeName`](../go/block.md#BlockAt.TypeName)、[`BlockAt.Snbt`](../go/block.md#BlockAt.Snbt)、[`BlockAt.DescriptionId`](../go/block.md#BlockAt.DescriptionId)、[`BlockAt.DebugString`](../go/block.md#BlockAt.DebugString)、[`BlockAt.Tags`](../go/block.md#BlockAt.Tags)、[`BlockAt.StateText`](../go/block.md#BlockAt.StateText) 等，共 10 个
    - Zig：[`BlockAt.typeName`](../zig/block.md#BlockAt.typeName)、[`BlockAt.snbt`](../zig/block.md#BlockAt.snbt)、[`BlockAt.descriptionId`](../zig/block.md#BlockAt.descriptionId)、[`BlockAt.debugString`](../zig/block.md#BlockAt.debugString)、[`BlockAt.tags`](../zig/block.md#BlockAt.tags)、[`BlockAt.state`](../zig/block.md#BlockAt.state) 等，共 9 个

### `block_action` {#block_action}

```c
bool (*block_action)(
    int32_t dim,
    int32_t x,
    int32_t y,
    int32_t z,
    int32_t action,
    PierStr sarg,
    void* ctx,
    PierStrSink out
);
```

- 调用形式：`api->block_action(dim, x, y, z, action, sarg, ctx, out)`
- 参数：
    - dim : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - action : `int32_t`
    - sarg : `PierStr`
    - ctx : `void*`
    - out : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：§D 方块与方块实体（`§D blocks & block entities`）
- 表内序号：第 41 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Block::act`](../rust/block.md#Block.act)、[`Block::has_tag`](../rust/block.md#Block.has_tag)、[`Block::as_item`](../rust/block.md#Block.as_item)、[`Block::pop_resource`](../rust/block.md#Block.pop_resource)
    - Go：[`BlockAt.HasTag`](../go/block.md#BlockAt.HasTag)、[`BlockAt.GetState`](../go/block.md#BlockAt.GetState)、[`BlockAt.PopResource`](../go/block.md#BlockAt.PopResource)、[`BlockAt.AsItem`](../go/block.md#BlockAt.AsItem)、[`Raw.BlockAction`](../go/raw.md#Raw.BlockAction)
    - Zig：[`BlockAt.hasTag`](../zig/block.md#BlockAt.hasTag)、[`BlockAt.getState`](../zig/block.md#BlockAt.getState)、[`BlockAt.popResource`](../zig/block.md#BlockAt.popResource)、[`BlockAt.asItem`](../zig/block.md#BlockAt.asItem)

### `block_entity_snbt` {#block_entity_snbt}

```c
bool (*block_entity_snbt)(int32_t dim, int32_t x, int32_t y, int32_t z, void* ctx, PierStrSink sink);
```

`BlockActor::save`（使用默认的 `SaveContext`）的结果，以 SNBT 给出；那个位置没有方块实体时返回 false。

- 调用形式：`api->block_entity_snbt(dim, x, y, z, ctx, sink)`
- 参数：
    - dim : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：§D 方块与方块实体（`§D blocks & block entities`）
- 表内序号：第 42 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Block::block_entity`](../rust/block.md#Block.block_entity)
    - Go：[`BlockAt.EntitySNBT`](../go/block.md#BlockAt.EntitySNBT)、[`Raw.BlockEntitySnbt`](../go/raw.md#Raw.BlockEntitySnbt)

### `block_get_state` {#block_get_state}

```c
bool (*block_get_state)(int32_t dim, int32_t x, int32_t y, int32_t z, PierStr state_name, void* ctx,
                        PierStrSink sink);
```

- 调用形式：`api->block_get_state(dim, x, y, z, state_name, ctx, sink)`
- 参数：
    - dim : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - state_name : `PierStr`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：方块：读写状态、碰撞形状（专用函数）（`Block: state get/set, collision shape (dedicated fns)`）
- 表内序号：第 122 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Block::state`](../rust/block.md#Block.state)
    - Go：[`BlockAt.State`](../go/block.md#BlockAt.State)、[`Raw.BlockGetState`](../go/raw.md#Raw.BlockGetState)

### `block_set_state` {#block_set_state}

```c
bool (*block_set_state)(int32_t dim, int32_t x, int32_t y, int32_t z, PierStr state_name, PierStr value);
```

- 调用形式：`api->block_set_state(dim, x, y, z, state_name, value)`
- 参数：
    - dim : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - state_name : `PierStr`
    - value : `PierStr`
- 返回值类型：`bool`
- 所在分节：方块：读写状态、碰撞形状（专用函数）（`Block: state get/set, collision shape (dedicated fns)`）
- 表内序号：第 123 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Block::set_state`](../rust/block.md#Block.set_state)
    - Go：[`BlockAt.SetState`](../go/block.md#BlockAt.SetState)、[`Raw.BlockSetState`](../go/raw.md#Raw.BlockSetState)

### `block_get_collision_shape` {#block_get_collision_shape}

```c
bool (*block_get_collision_shape)(int32_t dim, int32_t x, int32_t y, int32_t z, void* ctx, PierStrSink sink);
```

- 调用形式：`api->block_get_collision_shape(dim, x, y, z, ctx, sink)`
- 参数：
    - dim : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：方块：读写状态、碰撞形状（专用函数）（`Block: state get/set, collision shape (dedicated fns)`）
- 表内序号：第 124 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Block::collision_shape`](../rust/block.md#Block.collision_shape)
    - Go：[`Raw.BlockGetCollisionShape`](../go/raw.md#Raw.BlockGetCollisionShape)

## `PierBlockNumProp` {#PierBlockNumProp}

`block_get_num` 的键。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PIER_BPROP_IS_AIR"></span>`PIER_BPROP_IS_AIR` | `0` | 取自 `Block::isAir` |
| <span id="PIER_BPROP_DATA"></span>`PIER_BPROP_DATA` | `1` | 取自 `Block::getData`（旧版的数据值） |
| <span id="PIER_BPROP_BLOCK_ITEM_ID"></span>`PIER_BPROP_BLOCK_ITEM_ID` | `2` | 取自 `Block::getBlockItemId` |
| <span id="PIER_BPROP_IS_CRAFTING_BLOCK"></span>`PIER_BPROP_IS_CRAFTING_BLOCK` | `3` | 取自 `Block::isCraftingBlock` |
| <span id="PIER_BPROP_IS_INTERACTIVE_BLOCK"></span>`PIER_BPROP_IS_INTERACTIVE_BLOCK` | `4` | 取自 `Block::isInteractiveBlock` |
| <span id="PIER_BPROP_HAS_BLOCK_ENTITY"></span>`PIER_BPROP_HAS_BLOCK_ENTITY` | `5` | `BlockSource::getBlockEntity(pos) != null` |
| <span id="PIER_BPROP_LIGHT"></span>`PIER_BPROP_LIGHT` | `6` | 取自 `Block::getLight` |
| <span id="PIER_BPROP_LIGHT_EMISSION"></span>`PIER_BPROP_LIGHT_EMISSION` | `7` | 取自 `Block::getLightEmission` |
| <span id="PIER_BPROP_DESTROY_SPEED"></span>`PIER_BPROP_DESTROY_SPEED` | `8` | 取自 `Block::getDestroySpeed` |
| <span id="PIER_BPROP_EXPLOSION_RESISTANCE"></span>`PIER_BPROP_EXPLOSION_RESISTANCE` | `9` | 取自 `Block::getExplosionResistance` |
| <span id="PIER_BPROP_FRICTION"></span>`PIER_BPROP_FRICTION` | `10` | 取自 `Block::getFriction` |
| <span id="PIER_BPROP_IS_CONTAINER"></span>`PIER_BPROP_IS_CONTAINER` | `11` | 取自 `Block::isContainerBlock` |
| <span id="PIER_BPROP_IS_DOOR"></span>`PIER_BPROP_IS_DOOR` | `12` | **这个常量在当前的引擎版本上取不到值**：从 BDS 1.26.40 起不再支持：`BlockType::isDoorBlock` 已被移除 |
| <span id="PIER_BPROP_IS_FENCE"></span>`PIER_BPROP_IS_FENCE` | `13` | 取自 `Block::isFenceBlock` |
| <span id="PIER_BPROP_IS_RAIL"></span>`PIER_BPROP_IS_RAIL` | `14` | 取自 `Block::isRailBlock` |
| <span id="PIER_BPROP_IS_SLAB"></span>`PIER_BPROP_IS_SLAB` | `15` | 取自 `Block::isSlabBlock` |
| <span id="PIER_BPROP_IS_STAIR"></span>`PIER_BPROP_IS_STAIR` | `16` | **这个常量在当前的引擎版本上取不到值**：从 BDS 1.26.40 起不再支持：`BlockType::isStairBlock` 已被移除 |
| <span id="PIER_BPROP_IS_WALL"></span>`PIER_BPROP_IS_WALL` | `17` | 取自 `Block::isWallBlock` |
| <span id="PIER_BPROP_IS_CROP"></span>`PIER_BPROP_IS_CROP` | `18` | 取自 `Block::isCropBlock` |
| <span id="PIER_BPROP_IS_UNBREAKABLE"></span>`PIER_BPROP_IS_UNBREAKABLE` | `19` | 取自 `Block::isUnbreakable` |
| <span id="PIER_BPROP_REDSTONE_SIGNAL"></span>`PIER_BPROP_REDSTONE_SIGNAL` | `20` | 取自 `Block::getDirectSignal` |
| <span id="PIER_BPROP_COMPARATOR_SIGNAL"></span>`PIER_BPROP_COMPARATOR_SIGNAL` | `21` | 取自 `Block::getComparatorSignal` |
| <span id="PIER_BPROP_IS_SIGNAL_SOURCE"></span>`PIER_BPROP_IS_SIGNAL_SOURCE` | `22` | 取自 `Block::isSignalSource` |
| <span id="PIER_BPROP_VARIANT"></span>`PIER_BPROP_VARIANT` | `23` | 取自 `Block::getVariant` |
| <span id="PIER_BPROP_BURN_ODDS"></span>`PIER_BPROP_BURN_ODDS` | `24` | 取自 `Block::getBurnOdds` |
| <span id="PIER_BPROP_FLAME_ODDS"></span>`PIER_BPROP_FLAME_ODDS` | `25` | 取自 `Block::getFlameOdds` |
| <span id="PIER_BPROP_BOUNCINESS"></span>`PIER_BPROP_BOUNCINESS` | `26` | 取自 `Block::getBounciness` |
| <span id="PIER_BPROP_IS_SOLID"></span>`PIER_BPROP_IS_SOLID` | `27` | 取自 `Block::isSolid` |
| <span id="PIER_BPROP_REQUIRES_TOOL"></span>`PIER_BPROP_REQUIRES_TOOL` | `28` | 取自 `Block::requiresCorrectToolForDrops` |

## `PierBlockStrProp` {#PierBlockStrProp}

`block_get_str` 的键。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PIER_BSTR_TYPE_NAME"></span>`PIER_BSTR_TYPE_NAME` | `0` | 取自 `Block::getTypeName` |
| <span id="PIER_BSTR_SNBT"></span>`PIER_BSTR_SNBT` | `1` | 取自 `Block::mSerializationId`，以 SNBT `{name,states,version}` 给出 |
| <span id="PIER_BSTR_DESCRIPTION_ID"></span>`PIER_BSTR_DESCRIPTION_ID` | `2` | 取自 `Block::getDescriptionId` |
| <span id="PIER_BSTR_DEBUG_STRING"></span>`PIER_BSTR_DEBUG_STRING` | `3` | 取自 `Block::toDebugString` |
| <span id="PIER_BSTR_TAGS"></span>`PIER_BSTR_TAGS` | `4` | 取自 `Block::mTags`，以 SNBT 字符串列表 `["a","b"]` 给出 |
| <span id="PIER_BSTR_STATE"></span>`PIER_BSTR_STATE` | `5` | SNBT `{state_name:value,…}`，全部方块状态 |
| <span id="PIER_BSTR_COLLISION_SHAPE"></span>`PIER_BSTR_COLLISION_SHAPE` | `6` | SNBT `[{min:[x,y,z],max:[x,y,z]},…]` |
| <span id="PIER_BSTR_OUTLINE_SHAPE"></span>`PIER_BSTR_OUTLINE_SHAPE` | `7` | SNBT `[{min,max}]`，渲染用的轮廓 |
| <span id="PIER_BSTR_DISPLAY_NAME"></span>`PIER_BSTR_DISPLAY_NAME` | `8` | 取自 `Block::getDisplayName` |

## `PierBlockAction` {#PierBlockAction}

`block_action` 的动作。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PIER_BACT_HAS_TAG"></span>`PIER_BACT_HAS_TAG` | `0` | `sarg` 为标签，输出 `"0"` 或 `"1"`，调用 `Block::hasTag` |
| <span id="PIER_BACT_GET_STATE"></span>`PIER_BACT_GET_STATE` | `1` | `sarg` 为状态名，输出状态值的字符串，调用 `Block::getState` |
| <span id="PIER_BACT_POP_RESOURCE"></span>`PIER_BACT_POP_RESOURCE` | `2` | `sarg` 为物品 SNBT，在这个位置掉落该物品，调用 `Block::popResource` |
| <span id="PIER_BACT_AS_ITEM"></span>`PIER_BACT_AS_ITEM` | `3` | 输出物品 SNBT，调用 `Block::asItemInstance` |
