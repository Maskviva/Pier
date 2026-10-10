# Go：方块

## 函数 {#functions}

### `Block` {#Block}

```go
func Block(dim, x, y, z int32) BlockAt
```

维度 `dim` 里 x、y、z 处的那一格。

- 参数：
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
- 返回值类型：`BlockAt`

## `BlockAt` {#BlockAt}

```go
type BlockAt struct {
    Dim     int32
    X, Y, Z int32
}
```

一个维度里的一个方块格。

### `BlockAt.Info` {#BlockAt.Info}

```go
func (b BlockAt) Info() (BlockInfo, error)
```

读取这个方块：它的类型名和状态。

- 返回值类型：`(BlockInfo, error)`
- 对应槽位：[`get_block`](../cpp/world.md#get_block)

### `BlockAt.Set` {#BlockAt.Set}

```go
func (b BlockAt) Set(blockSpec string) error
```

按方块描述放置一个方块，描述的写法和 `/setblock` 读的一样。

- 参数：
    - blockSpec : `string`
- 返回值类型：`error`
- 对应槽位：[`set_block`](../cpp/world.md#set_block)

### `BlockAt.State` {#BlockAt.State}

```go
func (b BlockAt) State(name string) (string, error)
```

按名字读取一个方块状态。

- 参数：
    - name : `string`
- 返回值类型：`(string, error)`
- 对应槽位：[`block_get_state`](../cpp/block.md#block_get_state)

### `BlockAt.SetState` {#BlockAt.SetState}

```go
func (b BlockAt) SetState(name, value string) error
```

按名字写入一个方块状态。

- 参数：
    - name : `string`
    - value : `string`
- 返回值类型：`error`
- 对应槽位：[`block_set_state`](../cpp/block.md#block_set_state)

### `BlockAt.EntitySNBT` {#BlockAt.EntitySNBT}

```go
func (b BlockAt) EntitySNBT() (string, error)
```

这一格的方块实体（比如箱子里的东西），以 SNBT 给出。

- 返回值类型：`(string, error)`
- 对应槽位：[`block_entity_snbt`](../cpp/block.md#block_entity_snbt)

### `BlockAt.Biome` {#BlockAt.Biome}

```go
func (b BlockAt) Biome() (string, error)
```

这一格的生物群系。

- 返回值类型：`(string, error)`
- 对应槽位：[`level_get_biome`](../cpp/world.md#level_get_biome)

### `BlockAt.Container` {#BlockAt.Container}

```go
func (b BlockAt) Container() Container
```

这一格的方块容器，比如箱子。

- 返回值类型：`Container`

### `BlockAt.IsAir` {#BlockAt.IsAir}

```go
func (b BlockAt) IsAir() (bool, error)
```

读取 `PIER_BPROP_IS_AIR`：取自 `Block::isAir`

- 返回值类型：`(bool, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.Data` {#BlockAt.Data}

```go
func (b BlockAt) Data() (float64, error)
```

读取 `PIER_BPROP_DATA`：取自 `Block::getData`（旧版的数据值）

- 返回值类型：`(float64, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.BlockItemId` {#BlockAt.BlockItemId}

```go
func (b BlockAt) BlockItemId() (float64, error)
```

读取 `PIER_BPROP_BLOCK_ITEM_ID`：取自 `Block::getBlockItemId`

- 返回值类型：`(float64, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsCraftingBlock` {#BlockAt.IsCraftingBlock}

```go
func (b BlockAt) IsCraftingBlock() (bool, error)
```

读取 `PIER_BPROP_IS_CRAFTING_BLOCK`：取自 `Block::isCraftingBlock`

- 返回值类型：`(bool, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsInteractiveBlock` {#BlockAt.IsInteractiveBlock}

```go
func (b BlockAt) IsInteractiveBlock() (bool, error)
```

读取 `PIER_BPROP_IS_INTERACTIVE_BLOCK`：取自 `Block::isInteractiveBlock`

- 返回值类型：`(bool, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.HasBlockEntity` {#BlockAt.HasBlockEntity}

```go
func (b BlockAt) HasBlockEntity() (bool, error)
```

读取 `PIER_BPROP_HAS_BLOCK_ENTITY`：`BlockSource::getBlockEntity(pos) != null`

- 返回值类型：`(bool, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.Light` {#BlockAt.Light}

```go
func (b BlockAt) Light() (float64, error)
```

读取 `PIER_BPROP_LIGHT`：取自 `Block::getLight`

- 返回值类型：`(float64, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.LightEmission` {#BlockAt.LightEmission}

```go
func (b BlockAt) LightEmission() (float64, error)
```

读取 `PIER_BPROP_LIGHT_EMISSION`：取自 `Block::getLightEmission`

- 返回值类型：`(float64, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.DestroySpeed` {#BlockAt.DestroySpeed}

```go
func (b BlockAt) DestroySpeed() (float64, error)
```

读取 `PIER_BPROP_DESTROY_SPEED`：取自 `Block::getDestroySpeed`

- 返回值类型：`(float64, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.ExplosionResistance` {#BlockAt.ExplosionResistance}

```go
func (b BlockAt) ExplosionResistance() (float64, error)
```

读取 `PIER_BPROP_EXPLOSION_RESISTANCE`：取自 `Block::getExplosionResistance`

- 返回值类型：`(float64, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.Friction` {#BlockAt.Friction}

```go
func (b BlockAt) Friction() (float64, error)
```

读取 `PIER_BPROP_FRICTION`：取自 `Block::getFriction`

- 返回值类型：`(float64, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsContainer` {#BlockAt.IsContainer}

```go
func (b BlockAt) IsContainer() (bool, error)
```

读取 `PIER_BPROP_IS_CONTAINER`：取自 `Block::isContainerBlock`

- 返回值类型：`(bool, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsDoor` {#BlockAt.IsDoor}

```go
func (b BlockAt) IsDoor() (bool, error)
```

读取 `PIER_BPROP_IS_DOOR`：从 BDS 1.26.40 起不再支持：`BlockType::isDoorBlock` 已被移除

在当前所有的引擎版本上，宿主读这一项都没有结果：从 BDS 1.26.40 起不再支持：`BlockType::isDoorBlock` 已被移除

- 返回值类型：`(bool, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsFence` {#BlockAt.IsFence}

```go
func (b BlockAt) IsFence() (bool, error)
```

读取 `PIER_BPROP_IS_FENCE`：取自 `Block::isFenceBlock`

- 返回值类型：`(bool, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsRail` {#BlockAt.IsRail}

```go
func (b BlockAt) IsRail() (bool, error)
```

读取 `PIER_BPROP_IS_RAIL`：取自 `Block::isRailBlock`

- 返回值类型：`(bool, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsSlab` {#BlockAt.IsSlab}

```go
func (b BlockAt) IsSlab() (bool, error)
```

读取 `PIER_BPROP_IS_SLAB`：取自 `Block::isSlabBlock`

- 返回值类型：`(bool, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsStair` {#BlockAt.IsStair}

```go
func (b BlockAt) IsStair() (bool, error)
```

读取 `PIER_BPROP_IS_STAIR`：从 BDS 1.26.40 起不再支持：`BlockType::isStairBlock` 已被移除

在当前所有的引擎版本上，宿主读这一项都没有结果：从 BDS 1.26.40 起不再支持：`BlockType::isStairBlock` 已被移除

- 返回值类型：`(bool, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsWall` {#BlockAt.IsWall}

```go
func (b BlockAt) IsWall() (bool, error)
```

读取 `PIER_BPROP_IS_WALL`：取自 `Block::isWallBlock`

- 返回值类型：`(bool, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsCrop` {#BlockAt.IsCrop}

```go
func (b BlockAt) IsCrop() (bool, error)
```

读取 `PIER_BPROP_IS_CROP`：取自 `Block::isCropBlock`

- 返回值类型：`(bool, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsUnbreakable` {#BlockAt.IsUnbreakable}

```go
func (b BlockAt) IsUnbreakable() (bool, error)
```

读取 `PIER_BPROP_IS_UNBREAKABLE`：取自 `Block::isUnbreakable`

- 返回值类型：`(bool, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.RedstoneSignal` {#BlockAt.RedstoneSignal}

```go
func (b BlockAt) RedstoneSignal() (float64, error)
```

读取 `PIER_BPROP_REDSTONE_SIGNAL`：取自 `Block::getDirectSignal`

- 返回值类型：`(float64, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.ComparatorSignal` {#BlockAt.ComparatorSignal}

```go
func (b BlockAt) ComparatorSignal() (float64, error)
```

读取 `PIER_BPROP_COMPARATOR_SIGNAL`：取自 `Block::getComparatorSignal`

- 返回值类型：`(float64, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsSignalSource` {#BlockAt.IsSignalSource}

```go
func (b BlockAt) IsSignalSource() (bool, error)
```

读取 `PIER_BPROP_IS_SIGNAL_SOURCE`：取自 `Block::isSignalSource`

- 返回值类型：`(bool, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.Variant` {#BlockAt.Variant}

```go
func (b BlockAt) Variant() (float64, error)
```

读取 `PIER_BPROP_VARIANT`：取自 `Block::getVariant`

- 返回值类型：`(float64, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.BurnOdds` {#BlockAt.BurnOdds}

```go
func (b BlockAt) BurnOdds() (float64, error)
```

读取 `PIER_BPROP_BURN_ODDS`：取自 `Block::getBurnOdds`

- 返回值类型：`(float64, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.FlameOdds` {#BlockAt.FlameOdds}

```go
func (b BlockAt) FlameOdds() (float64, error)
```

读取 `PIER_BPROP_FLAME_ODDS`：取自 `Block::getFlameOdds`

- 返回值类型：`(float64, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.Bounciness` {#BlockAt.Bounciness}

```go
func (b BlockAt) Bounciness() (float64, error)
```

读取 `PIER_BPROP_BOUNCINESS`：取自 `Block::getBounciness`

- 返回值类型：`(float64, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsSolid` {#BlockAt.IsSolid}

```go
func (b BlockAt) IsSolid() (bool, error)
```

读取 `PIER_BPROP_IS_SOLID`：取自 `Block::isSolid`

- 返回值类型：`(bool, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.RequiresTool` {#BlockAt.RequiresTool}

```go
func (b BlockAt) RequiresTool() (float64, error)
```

读取 `PIER_BPROP_REQUIRES_TOOL`：取自 `Block::requiresCorrectToolForDrops`

- 返回值类型：`(float64, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.TypeName` {#BlockAt.TypeName}

```go
func (b BlockAt) TypeName() (string, error)
```

读取 `PIER_BSTR_TYPE_NAME`：取自 `Block::getTypeName`

- 返回值类型：`(string, error)`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.Snbt` {#BlockAt.Snbt}

```go
func (b BlockAt) Snbt() (string, error)
```

读取 `PIER_BSTR_SNBT`：取自 `Block::mSerializationId`，以 SNBT `{name,states,version}` 给出

- 返回值类型：`(string, error)`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.DescriptionId` {#BlockAt.DescriptionId}

```go
func (b BlockAt) DescriptionId() (string, error)
```

读取 `PIER_BSTR_DESCRIPTION_ID`：取自 `Block::getDescriptionId`

- 返回值类型：`(string, error)`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.DebugString` {#BlockAt.DebugString}

```go
func (b BlockAt) DebugString() (string, error)
```

读取 `PIER_BSTR_DEBUG_STRING`：取自 `Block::toDebugString`

- 返回值类型：`(string, error)`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.Tags` {#BlockAt.Tags}

```go
func (b BlockAt) Tags() (string, error)
```

读取 `PIER_BSTR_TAGS`：取自 `Block::mTags`，以 SNBT 字符串列表 `["a","b"]` 给出

- 返回值类型：`(string, error)`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.StateText` {#BlockAt.StateText}

```go
func (b BlockAt) StateText() (string, error)
```

读取 `PIER_BSTR_STATE`：SNBT `{state_name:value,…}`，全部方块状态

- 返回值类型：`(string, error)`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.CollisionShape` {#BlockAt.CollisionShape}

```go
func (b BlockAt) CollisionShape() (string, error)
```

读取 `PIER_BSTR_COLLISION_SHAPE`：SNBT `[{min:[x,y,z],max:[x,y,z]},…]`

- 返回值类型：`(string, error)`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.OutlineShape` {#BlockAt.OutlineShape}

```go
func (b BlockAt) OutlineShape() (string, error)
```

读取 `PIER_BSTR_OUTLINE_SHAPE`：SNBT `[{min,max}]`，渲染用的轮廓

- 返回值类型：`(string, error)`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.DisplayName` {#BlockAt.DisplayName}

```go
func (b BlockAt) DisplayName() (string, error)
```

读取 `PIER_BSTR_DISPLAY_NAME`：取自 `Block::getDisplayName`

- 返回值类型：`(string, error)`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.HasTag` {#BlockAt.HasTag}

```go
func (b BlockAt) HasTag(sarg string) (string, error)
```

执行 `PIER_BACT_HAS_TAG`：`sarg` 为标签，输出 `"0"` 或 `"1"`，调用 `Block::hasTag`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
- 返回值类型：`(string, error)`
- 对应槽位：[`block_action`](../cpp/block.md#block_action)

### `BlockAt.GetState` {#BlockAt.GetState}

```go
func (b BlockAt) GetState(sarg string) (string, error)
```

执行 `PIER_BACT_GET_STATE`：`sarg` 为状态名，输出状态值的字符串，调用 `Block::getState`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
- 返回值类型：`(string, error)`
- 对应槽位：[`block_action`](../cpp/block.md#block_action)

### `BlockAt.PopResource` {#BlockAt.PopResource}

```go
func (b BlockAt) PopResource(sarg string) (string, error)
```

执行 `PIER_BACT_POP_RESOURCE`：`sarg` 为物品 SNBT，在这个位置掉落该物品，调用 `Block::popResource`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
- 返回值类型：`(string, error)`
- 对应槽位：[`block_action`](../cpp/block.md#block_action)

### `BlockAt.AsItem` {#BlockAt.AsItem}

```go
func (b BlockAt) AsItem(sarg string) (string, error)
```

执行 `PIER_BACT_AS_ITEM`：输出物品 SNBT，调用 `Block::asItemInstance`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
- 返回值类型：`(string, error)`
- 对应槽位：[`block_action`](../cpp/block.md#block_action)

## `BlockInfo` {#BlockInfo}

```go
type BlockInfo struct {
    X, Y, Z int32
    Name    string
    SNBT    string
}
```

一个方块：它的格子、类型名，以及以 SNBT 表示的方块状态。

## `BProp*` {#BProp}

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="BPropIsAir"></span>`BPropIsAir` | `0` | 即 `PIER_BPROP_IS_AIR`：取自 `Block::isAir` |
| <span id="BPropData"></span>`BPropData` | `1` | 即 `PIER_BPROP_DATA`：取自 `Block::getData`（旧版的数据值） |
| <span id="BPropBlockItemId"></span>`BPropBlockItemId` | `2` | 即 `PIER_BPROP_BLOCK_ITEM_ID`：取自 `Block::getBlockItemId` |
| <span id="BPropIsCraftingBlock"></span>`BPropIsCraftingBlock` | `3` | 即 `PIER_BPROP_IS_CRAFTING_BLOCK`：取自 `Block::isCraftingBlock` |
| <span id="BPropIsInteractiveBlock"></span>`BPropIsInteractiveBlock` | `4` | 即 `PIER_BPROP_IS_INTERACTIVE_BLOCK`：取自 `Block::isInteractiveBlock` |
| <span id="BPropHasBlockEntity"></span>`BPropHasBlockEntity` | `5` | 即 `PIER_BPROP_HAS_BLOCK_ENTITY`：`BlockSource::getBlockEntity(pos) != null` |
| <span id="BPropLight"></span>`BPropLight` | `6` | 即 `PIER_BPROP_LIGHT`：取自 `Block::getLight` |
| <span id="BPropLightEmission"></span>`BPropLightEmission` | `7` | 即 `PIER_BPROP_LIGHT_EMISSION`：取自 `Block::getLightEmission` |
| <span id="BPropDestroySpeed"></span>`BPropDestroySpeed` | `8` | 即 `PIER_BPROP_DESTROY_SPEED`：取自 `Block::getDestroySpeed` |
| <span id="BPropExplosionResistance"></span>`BPropExplosionResistance` | `9` | 即 `PIER_BPROP_EXPLOSION_RESISTANCE`：取自 `Block::getExplosionResistance` |
| <span id="BPropFriction"></span>`BPropFriction` | `10` | 即 `PIER_BPROP_FRICTION`：取自 `Block::getFriction` |
| <span id="BPropIsContainer"></span>`BPropIsContainer` | `11` | 即 `PIER_BPROP_IS_CONTAINER`：取自 `Block::isContainerBlock` |
| <span id="BPropIsDoor"></span>`BPropIsDoor` | `12` | **这个常量在当前的引擎版本上取不到值**：即 `PIER_BPROP_IS_DOOR`：从 BDS 1.26.40 起不再支持：`BlockType::isDoorBlock` 已被移除 |
| <span id="BPropIsFence"></span>`BPropIsFence` | `13` | 即 `PIER_BPROP_IS_FENCE`：取自 `Block::isFenceBlock` |
| <span id="BPropIsRail"></span>`BPropIsRail` | `14` | 即 `PIER_BPROP_IS_RAIL`：取自 `Block::isRailBlock` |
| <span id="BPropIsSlab"></span>`BPropIsSlab` | `15` | 即 `PIER_BPROP_IS_SLAB`：取自 `Block::isSlabBlock` |
| <span id="BPropIsStair"></span>`BPropIsStair` | `16` | **这个常量在当前的引擎版本上取不到值**：即 `PIER_BPROP_IS_STAIR`：从 BDS 1.26.40 起不再支持：`BlockType::isStairBlock` 已被移除 |
| <span id="BPropIsWall"></span>`BPropIsWall` | `17` | 即 `PIER_BPROP_IS_WALL`：取自 `Block::isWallBlock` |
| <span id="BPropIsCrop"></span>`BPropIsCrop` | `18` | 即 `PIER_BPROP_IS_CROP`：取自 `Block::isCropBlock` |
| <span id="BPropIsUnbreakable"></span>`BPropIsUnbreakable` | `19` | 即 `PIER_BPROP_IS_UNBREAKABLE`：取自 `Block::isUnbreakable` |
| <span id="BPropRedstoneSignal"></span>`BPropRedstoneSignal` | `20` | 即 `PIER_BPROP_REDSTONE_SIGNAL`：取自 `Block::getDirectSignal` |
| <span id="BPropComparatorSignal"></span>`BPropComparatorSignal` | `21` | 即 `PIER_BPROP_COMPARATOR_SIGNAL`：取自 `Block::getComparatorSignal` |
| <span id="BPropIsSignalSource"></span>`BPropIsSignalSource` | `22` | 即 `PIER_BPROP_IS_SIGNAL_SOURCE`：取自 `Block::isSignalSource` |
| <span id="BPropVariant"></span>`BPropVariant` | `23` | 即 `PIER_BPROP_VARIANT`：取自 `Block::getVariant` |
| <span id="BPropBurnOdds"></span>`BPropBurnOdds` | `24` | 即 `PIER_BPROP_BURN_ODDS`：取自 `Block::getBurnOdds` |
| <span id="BPropFlameOdds"></span>`BPropFlameOdds` | `25` | 即 `PIER_BPROP_FLAME_ODDS`：取自 `Block::getFlameOdds` |
| <span id="BPropBounciness"></span>`BPropBounciness` | `26` | 即 `PIER_BPROP_BOUNCINESS`：取自 `Block::getBounciness` |
| <span id="BPropIsSolid"></span>`BPropIsSolid` | `27` | 即 `PIER_BPROP_IS_SOLID`：取自 `Block::isSolid` |
| <span id="BPropRequiresTool"></span>`BPropRequiresTool` | `28` | 即 `PIER_BPROP_REQUIRES_TOOL`：取自 `Block::requiresCorrectToolForDrops` |

## `BStr*` {#BStr}

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="BStrTypeName"></span>`BStrTypeName` | `0` | 即 `PIER_BSTR_TYPE_NAME`：取自 `Block::getTypeName` |
| <span id="BStrSnbt"></span>`BStrSnbt` | `1` | 即 `PIER_BSTR_SNBT`：取自 `Block::mSerializationId`，以 SNBT `{name,states,version}` 给出 |
| <span id="BStrDescriptionId"></span>`BStrDescriptionId` | `2` | 即 `PIER_BSTR_DESCRIPTION_ID`：取自 `Block::getDescriptionId` |
| <span id="BStrDebugString"></span>`BStrDebugString` | `3` | 即 `PIER_BSTR_DEBUG_STRING`：取自 `Block::toDebugString` |
| <span id="BStrTags"></span>`BStrTags` | `4` | 即 `PIER_BSTR_TAGS`：取自 `Block::mTags`，以 SNBT 字符串列表 `["a","b"]` 给出 |
| <span id="BStrState"></span>`BStrState` | `5` | 即 `PIER_BSTR_STATE`：SNBT `{state_name:value,…}`，全部方块状态 |
| <span id="BStrCollisionShape"></span>`BStrCollisionShape` | `6` | 即 `PIER_BSTR_COLLISION_SHAPE`：SNBT `[{min:[x,y,z],max:[x,y,z]},…]` |
| <span id="BStrOutlineShape"></span>`BStrOutlineShape` | `7` | 即 `PIER_BSTR_OUTLINE_SHAPE`：SNBT `[{min,max}]`，渲染用的轮廓 |
| <span id="BStrDisplayName"></span>`BStrDisplayName` | `8` | 即 `PIER_BSTR_DISPLAY_NAME`：取自 `Block::getDisplayName` |

## `BAct*` {#BAct}

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="BActHasTag"></span>`BActHasTag` | `0` | 即 `PIER_BACT_HAS_TAG`：`sarg` 为标签，输出 `"0"` 或 `"1"`，调用 `Block::hasTag` |
| <span id="BActGetState"></span>`BActGetState` | `1` | 即 `PIER_BACT_GET_STATE`：`sarg` 为状态名，输出状态值的字符串，调用 `Block::getState` |
| <span id="BActPopResource"></span>`BActPopResource` | `2` | 即 `PIER_BACT_POP_RESOURCE`：`sarg` 为物品 SNBT，在这个位置掉落该物品，调用 `Block::popResource` |
| <span id="BActAsItem"></span>`BActAsItem` | `3` | 即 `PIER_BACT_AS_ITEM`：输出物品 SNBT，调用 `Block::asItemInstance` |
