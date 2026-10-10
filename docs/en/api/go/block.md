# Go: Blocks

## Functions {#functions}

### `Block` {#Block}

```go
func Block(dim, x, y, z int32) BlockAt
```

Block is the cell at x, y, z of dimension dim.

- Parameters:
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
- Return type: `BlockAt`

## `BlockAt` {#BlockAt}

```go
type BlockAt struct {
    Dim     int32
    X, Y, Z int32
}
```

BlockAt is one block cell of a dimension.

### `BlockAt.Info` {#BlockAt.Info}

```go
func (b BlockAt) Info() (BlockInfo, error)
```

Info reads the block: its type name and state.

- Return type: `(BlockInfo, error)`
- Slots: [`get_block`](../cpp/world.md#get_block)

### `BlockAt.Set` {#BlockAt.Set}

```go
func (b BlockAt) Set(blockSpec string) error
```

Set places a block from a block spec, as /setblock reads one.

- Parameters:
    - blockSpec : `string`
- Return type: `error`
- Slots: [`set_block`](../cpp/world.md#set_block)

### `BlockAt.State` {#BlockAt.State}

```go
func (b BlockAt) State(name string) (string, error)
```

State reads one block state by name.

- Parameters:
    - name : `string`
- Return type: `(string, error)`
- Slots: [`block_get_state`](../cpp/block.md#block_get_state)

### `BlockAt.SetState` {#BlockAt.SetState}

```go
func (b BlockAt) SetState(name, value string) error
```

SetState writes one block state by name.

- Parameters:
    - name : `string`
    - value : `string`
- Return type: `error`
- Slots: [`block_set_state`](../cpp/block.md#block_set_state)

### `BlockAt.EntitySNBT` {#BlockAt.EntitySNBT}

```go
func (b BlockAt) EntitySNBT() (string, error)
```

EntitySNBT is the block entity at the cell, such as a chest's contents, as SNBT.

- Return type: `(string, error)`
- Slots: [`block_entity_snbt`](../cpp/block.md#block_entity_snbt)

### `BlockAt.Biome` {#BlockAt.Biome}

```go
func (b BlockAt) Biome() (string, error)
```

Biome is the biome at the cell.

- Return type: `(string, error)`
- Slots: [`level_get_biome`](../cpp/world.md#level_get_biome)

### `BlockAt.Container` {#BlockAt.Container}

```go
func (b BlockAt) Container() Container
```

Container is the cell's block container, such as a chest.

- Return type: `Container`

### `BlockAt.IsAir` {#BlockAt.IsAir}

```go
func (b BlockAt) IsAir() (bool, error)
```

IsAir reads `PIER_BPROP_IS_AIR`: `Block::isAir`

- Return type: `(bool, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.Data` {#BlockAt.Data}

```go
func (b BlockAt) Data() (float64, error)
```

Data reads `PIER_BPROP_DATA`: `Block::getData` (legacy data value)

- Return type: `(float64, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.BlockItemId` {#BlockAt.BlockItemId}

```go
func (b BlockAt) BlockItemId() (float64, error)
```

BlockItemId reads `PIER_BPROP_BLOCK_ITEM_ID`: `Block::getBlockItemId`

- Return type: `(float64, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsCraftingBlock` {#BlockAt.IsCraftingBlock}

```go
func (b BlockAt) IsCraftingBlock() (bool, error)
```

IsCraftingBlock reads `PIER_BPROP_IS_CRAFTING_BLOCK`: `Block::isCraftingBlock`

- Return type: `(bool, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsInteractiveBlock` {#BlockAt.IsInteractiveBlock}

```go
func (b BlockAt) IsInteractiveBlock() (bool, error)
```

IsInteractiveBlock reads `PIER_BPROP_IS_INTERACTIVE_BLOCK`: `Block::isInteractiveBlock`

- Return type: `(bool, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.HasBlockEntity` {#BlockAt.HasBlockEntity}

```go
func (b BlockAt) HasBlockEntity() (bool, error)
```

HasBlockEntity reads `PIER_BPROP_HAS_BLOCK_ENTITY`: `BlockSource::getBlockEntity`(pos) != null

- Return type: `(bool, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.Light` {#BlockAt.Light}

```go
func (b BlockAt) Light() (float64, error)
```

Light reads `PIER_BPROP_LIGHT`: `Block::getLight`

- Return type: `(float64, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.LightEmission` {#BlockAt.LightEmission}

```go
func (b BlockAt) LightEmission() (float64, error)
```

LightEmission reads `PIER_BPROP_LIGHT_EMISSION`: `Block::getLightEmission`

- Return type: `(float64, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.DestroySpeed` {#BlockAt.DestroySpeed}

```go
func (b BlockAt) DestroySpeed() (float64, error)
```

DestroySpeed reads `PIER_BPROP_DESTROY_SPEED`: `Block::getDestroySpeed`

- Return type: `(float64, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.ExplosionResistance` {#BlockAt.ExplosionResistance}

```go
func (b BlockAt) ExplosionResistance() (float64, error)
```

ExplosionResistance reads `PIER_BPROP_EXPLOSION_RESISTANCE`: `Block::getExplosionResistance`

- Return type: `(float64, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.Friction` {#BlockAt.Friction}

```go
func (b BlockAt) Friction() (float64, error)
```

Friction reads `PIER_BPROP_FRICTION`: `Block::getFriction`

- Return type: `(float64, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsContainer` {#BlockAt.IsContainer}

```go
func (b BlockAt) IsContainer() (bool, error)
```

IsContainer reads `PIER_BPROP_IS_CONTAINER`: `Block::isContainerBlock`

- Return type: `(bool, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsDoor` {#BlockAt.IsDoor}

```go
func (b BlockAt) IsDoor() (bool, error)
```

IsDoor reads `PIER_BPROP_IS_DOOR`: unsupported since BDS 1.26.40: `BlockType::isDoorBlock` is gone

The host answers no on every current engine version: unsupported since BDS 1.26.40: `BlockType::isDoorBlock` is gone

- Return type: `(bool, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsFence` {#BlockAt.IsFence}

```go
func (b BlockAt) IsFence() (bool, error)
```

IsFence reads `PIER_BPROP_IS_FENCE`: `Block::isFenceBlock`

- Return type: `(bool, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsRail` {#BlockAt.IsRail}

```go
func (b BlockAt) IsRail() (bool, error)
```

IsRail reads `PIER_BPROP_IS_RAIL`: `Block::isRailBlock`

- Return type: `(bool, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsSlab` {#BlockAt.IsSlab}

```go
func (b BlockAt) IsSlab() (bool, error)
```

IsSlab reads `PIER_BPROP_IS_SLAB`: `Block::isSlabBlock`

- Return type: `(bool, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsStair` {#BlockAt.IsStair}

```go
func (b BlockAt) IsStair() (bool, error)
```

IsStair reads `PIER_BPROP_IS_STAIR`: unsupported since BDS 1.26.40: `BlockType::isStairBlock` is gone

The host answers no on every current engine version: unsupported since BDS 1.26.40: `BlockType::isStairBlock` is gone

- Return type: `(bool, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsWall` {#BlockAt.IsWall}

```go
func (b BlockAt) IsWall() (bool, error)
```

IsWall reads `PIER_BPROP_IS_WALL`: `Block::isWallBlock`

- Return type: `(bool, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsCrop` {#BlockAt.IsCrop}

```go
func (b BlockAt) IsCrop() (bool, error)
```

IsCrop reads `PIER_BPROP_IS_CROP`: `Block::isCropBlock`

- Return type: `(bool, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsUnbreakable` {#BlockAt.IsUnbreakable}

```go
func (b BlockAt) IsUnbreakable() (bool, error)
```

IsUnbreakable reads `PIER_BPROP_IS_UNBREAKABLE`: `Block::isUnbreakable`

- Return type: `(bool, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.RedstoneSignal` {#BlockAt.RedstoneSignal}

```go
func (b BlockAt) RedstoneSignal() (float64, error)
```

RedstoneSignal reads `PIER_BPROP_REDSTONE_SIGNAL`: `Block::getDirectSignal`

- Return type: `(float64, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.ComparatorSignal` {#BlockAt.ComparatorSignal}

```go
func (b BlockAt) ComparatorSignal() (float64, error)
```

ComparatorSignal reads `PIER_BPROP_COMPARATOR_SIGNAL`: `Block::getComparatorSignal`

- Return type: `(float64, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsSignalSource` {#BlockAt.IsSignalSource}

```go
func (b BlockAt) IsSignalSource() (bool, error)
```

IsSignalSource reads `PIER_BPROP_IS_SIGNAL_SOURCE`: `Block::isSignalSource`

- Return type: `(bool, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.Variant` {#BlockAt.Variant}

```go
func (b BlockAt) Variant() (float64, error)
```

Variant reads `PIER_BPROP_VARIANT`: `Block::getVariant`

- Return type: `(float64, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.BurnOdds` {#BlockAt.BurnOdds}

```go
func (b BlockAt) BurnOdds() (float64, error)
```

BurnOdds reads `PIER_BPROP_BURN_ODDS`: `Block::getBurnOdds`

- Return type: `(float64, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.FlameOdds` {#BlockAt.FlameOdds}

```go
func (b BlockAt) FlameOdds() (float64, error)
```

FlameOdds reads `PIER_BPROP_FLAME_ODDS`: `Block::getFlameOdds`

- Return type: `(float64, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.Bounciness` {#BlockAt.Bounciness}

```go
func (b BlockAt) Bounciness() (float64, error)
```

Bounciness reads `PIER_BPROP_BOUNCINESS`: `Block::getBounciness`

- Return type: `(float64, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.IsSolid` {#BlockAt.IsSolid}

```go
func (b BlockAt) IsSolid() (bool, error)
```

IsSolid reads `PIER_BPROP_IS_SOLID`: `Block::isSolid`

- Return type: `(bool, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.RequiresTool` {#BlockAt.RequiresTool}

```go
func (b BlockAt) RequiresTool() (float64, error)
```

RequiresTool reads `PIER_BPROP_REQUIRES_TOOL`: `Block::requiresCorrectToolForDrops`

- Return type: `(float64, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `BlockAt.TypeName` {#BlockAt.TypeName}

```go
func (b BlockAt) TypeName() (string, error)
```

TypeName reads `PIER_BSTR_TYPE_NAME`: `Block::getTypeName`

- Return type: `(string, error)`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.Snbt` {#BlockAt.Snbt}

```go
func (b BlockAt) Snbt() (string, error)
```

Snbt reads `PIER_BSTR_SNBT`: `Block::mSerializationId` → SNBT {name,states,version}

- Return type: `(string, error)`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.DescriptionId` {#BlockAt.DescriptionId}

```go
func (b BlockAt) DescriptionId() (string, error)
```

DescriptionId reads `PIER_BSTR_DESCRIPTION_ID`: `Block::getDescriptionId`

- Return type: `(string, error)`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.DebugString` {#BlockAt.DebugString}

```go
func (b BlockAt) DebugString() (string, error)
```

DebugString reads `PIER_BSTR_DEBUG_STRING`: `Block::toDebugString`

- Return type: `(string, error)`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.Tags` {#BlockAt.Tags}

```go
func (b BlockAt) Tags() (string, error)
```

Tags reads `PIER_BSTR_TAGS`: `Block::mTags` → SNBT string list \["a","b"\]

- Return type: `(string, error)`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.StateText` {#BlockAt.StateText}

```go
func (b BlockAt) StateText() (string, error)
```

StateText reads `PIER_BSTR_STATE`: SNBT {`state_name`:value,…} all block states

- Return type: `(string, error)`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.CollisionShape` {#BlockAt.CollisionShape}

```go
func (b BlockAt) CollisionShape() (string, error)
```

CollisionShape reads `PIER_BSTR_COLLISION_SHAPE`: SNBT \[{min:\[x,y,z\],max:\[x,y,z\]},…\]

- Return type: `(string, error)`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.OutlineShape` {#BlockAt.OutlineShape}

```go
func (b BlockAt) OutlineShape() (string, error)
```

OutlineShape reads `PIER_BSTR_OUTLINE_SHAPE`: SNBT \[{min,max}\] render outline

- Return type: `(string, error)`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.DisplayName` {#BlockAt.DisplayName}

```go
func (b BlockAt) DisplayName() (string, error)
```

DisplayName reads `PIER_BSTR_DISPLAY_NAME`: `Block::getDisplayName`

- Return type: `(string, error)`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `BlockAt.HasTag` {#BlockAt.HasTag}

```go
func (b BlockAt) HasTag(sarg string) (string, error)
```

HasTag runs `PIER_BACT_HAS_TAG`: sarg=tag → out "0"/"1" `Block::hasTag`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
- Return type: `(string, error)`
- Slots: [`block_action`](../cpp/block.md#block_action)

### `BlockAt.GetState` {#BlockAt.GetState}

```go
func (b BlockAt) GetState(sarg string) (string, error)
```

GetState runs `PIER_BACT_GET_STATE`: sarg=state name → out value string `Block::getState`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
- Return type: `(string, error)`
- Slots: [`block_action`](../cpp/block.md#block_action)

### `BlockAt.PopResource` {#BlockAt.PopResource}

```go
func (b BlockAt) PopResource(sarg string) (string, error)
```

PopResource runs `PIER_BACT_POP_RESOURCE`: sarg=item SNBT → pop resource at pos `Block::popResource`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
- Return type: `(string, error)`
- Slots: [`block_action`](../cpp/block.md#block_action)

### `BlockAt.AsItem` {#BlockAt.AsItem}

```go
func (b BlockAt) AsItem(sarg string) (string, error)
```

AsItem runs `PIER_BACT_AS_ITEM`: → out item SNBT `Block::asItemInstance`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
- Return type: `(string, error)`
- Slots: [`block_action`](../cpp/block.md#block_action)

## `BlockInfo` {#BlockInfo}

```go
type BlockInfo struct {
    X, Y, Z int32
    Name    string
    SNBT    string
}
```

BlockInfo is one block: its cell, its type name and its block state as SNBT.

## `BProp*` {#BProp}

| Name | Value | Description |
|---|---|---|
| <span id="BPropIsAir"></span>`BPropIsAir` | `0` | BPropIsAir is `PIER_BPROP_IS_AIR`: `Block::isAir` |
| <span id="BPropData"></span>`BPropData` | `1` | BPropData is `PIER_BPROP_DATA`: `Block::getData` (legacy data value) |
| <span id="BPropBlockItemId"></span>`BPropBlockItemId` | `2` | BPropBlockItemId is `PIER_BPROP_BLOCK_ITEM_ID`: `Block::getBlockItemId` |
| <span id="BPropIsCraftingBlock"></span>`BPropIsCraftingBlock` | `3` | BPropIsCraftingBlock is `PIER_BPROP_IS_CRAFTING_BLOCK`: `Block::isCraftingBlock` |
| <span id="BPropIsInteractiveBlock"></span>`BPropIsInteractiveBlock` | `4` | BPropIsInteractiveBlock is `PIER_BPROP_IS_INTERACTIVE_BLOCK`: `Block::isInteractiveBlock` |
| <span id="BPropHasBlockEntity"></span>`BPropHasBlockEntity` | `5` | BPropHasBlockEntity is `PIER_BPROP_HAS_BLOCK_ENTITY`: `BlockSource::getBlockEntity`(pos) != null |
| <span id="BPropLight"></span>`BPropLight` | `6` | BPropLight is `PIER_BPROP_LIGHT`: `Block::getLight` |
| <span id="BPropLightEmission"></span>`BPropLightEmission` | `7` | BPropLightEmission is `PIER_BPROP_LIGHT_EMISSION`: `Block::getLightEmission` |
| <span id="BPropDestroySpeed"></span>`BPropDestroySpeed` | `8` | BPropDestroySpeed is `PIER_BPROP_DESTROY_SPEED`: `Block::getDestroySpeed` |
| <span id="BPropExplosionResistance"></span>`BPropExplosionResistance` | `9` | BPropExplosionResistance is `PIER_BPROP_EXPLOSION_RESISTANCE`: `Block::getExplosionResistance` |
| <span id="BPropFriction"></span>`BPropFriction` | `10` | BPropFriction is `PIER_BPROP_FRICTION`: `Block::getFriction` |
| <span id="BPropIsContainer"></span>`BPropIsContainer` | `11` | BPropIsContainer is `PIER_BPROP_IS_CONTAINER`: `Block::isContainerBlock` |
| <span id="BPropIsDoor"></span>`BPropIsDoor` | `12` | **the current engine version gives no value for this constant**: BPropIsDoor is `PIER_BPROP_IS_DOOR`: unsupported since BDS 1.26.40: `BlockType::isDoorBlock` is gone |
| <span id="BPropIsFence"></span>`BPropIsFence` | `13` | BPropIsFence is `PIER_BPROP_IS_FENCE`: `Block::isFenceBlock` |
| <span id="BPropIsRail"></span>`BPropIsRail` | `14` | BPropIsRail is `PIER_BPROP_IS_RAIL`: `Block::isRailBlock` |
| <span id="BPropIsSlab"></span>`BPropIsSlab` | `15` | BPropIsSlab is `PIER_BPROP_IS_SLAB`: `Block::isSlabBlock` |
| <span id="BPropIsStair"></span>`BPropIsStair` | `16` | **the current engine version gives no value for this constant**: BPropIsStair is `PIER_BPROP_IS_STAIR`: unsupported since BDS 1.26.40: `BlockType::isStairBlock` is gone |
| <span id="BPropIsWall"></span>`BPropIsWall` | `17` | BPropIsWall is `PIER_BPROP_IS_WALL`: `Block::isWallBlock` |
| <span id="BPropIsCrop"></span>`BPropIsCrop` | `18` | BPropIsCrop is `PIER_BPROP_IS_CROP`: `Block::isCropBlock` |
| <span id="BPropIsUnbreakable"></span>`BPropIsUnbreakable` | `19` | BPropIsUnbreakable is `PIER_BPROP_IS_UNBREAKABLE`: `Block::isUnbreakable` |
| <span id="BPropRedstoneSignal"></span>`BPropRedstoneSignal` | `20` | BPropRedstoneSignal is `PIER_BPROP_REDSTONE_SIGNAL`: `Block::getDirectSignal` |
| <span id="BPropComparatorSignal"></span>`BPropComparatorSignal` | `21` | BPropComparatorSignal is `PIER_BPROP_COMPARATOR_SIGNAL`: `Block::getComparatorSignal` |
| <span id="BPropIsSignalSource"></span>`BPropIsSignalSource` | `22` | BPropIsSignalSource is `PIER_BPROP_IS_SIGNAL_SOURCE`: `Block::isSignalSource` |
| <span id="BPropVariant"></span>`BPropVariant` | `23` | BPropVariant is `PIER_BPROP_VARIANT`: `Block::getVariant` |
| <span id="BPropBurnOdds"></span>`BPropBurnOdds` | `24` | BPropBurnOdds is `PIER_BPROP_BURN_ODDS`: `Block::getBurnOdds` |
| <span id="BPropFlameOdds"></span>`BPropFlameOdds` | `25` | BPropFlameOdds is `PIER_BPROP_FLAME_ODDS`: `Block::getFlameOdds` |
| <span id="BPropBounciness"></span>`BPropBounciness` | `26` | BPropBounciness is `PIER_BPROP_BOUNCINESS`: `Block::getBounciness` |
| <span id="BPropIsSolid"></span>`BPropIsSolid` | `27` | BPropIsSolid is `PIER_BPROP_IS_SOLID`: `Block::isSolid` |
| <span id="BPropRequiresTool"></span>`BPropRequiresTool` | `28` | BPropRequiresTool is `PIER_BPROP_REQUIRES_TOOL`: `Block::requiresCorrectToolForDrops` |

## `BStr*` {#BStr}

| Name | Value | Description |
|---|---|---|
| <span id="BStrTypeName"></span>`BStrTypeName` | `0` | BStrTypeName is `PIER_BSTR_TYPE_NAME`: `Block::getTypeName` |
| <span id="BStrSnbt"></span>`BStrSnbt` | `1` | BStrSnbt is `PIER_BSTR_SNBT`: `Block::mSerializationId` → SNBT {name,states,version} |
| <span id="BStrDescriptionId"></span>`BStrDescriptionId` | `2` | BStrDescriptionId is `PIER_BSTR_DESCRIPTION_ID`: `Block::getDescriptionId` |
| <span id="BStrDebugString"></span>`BStrDebugString` | `3` | BStrDebugString is `PIER_BSTR_DEBUG_STRING`: `Block::toDebugString` |
| <span id="BStrTags"></span>`BStrTags` | `4` | BStrTags is `PIER_BSTR_TAGS`: `Block::mTags` → SNBT string list \["a","b"\] |
| <span id="BStrState"></span>`BStrState` | `5` | BStrState is `PIER_BSTR_STATE`: SNBT {`state_name`:value,…} all block states |
| <span id="BStrCollisionShape"></span>`BStrCollisionShape` | `6` | BStrCollisionShape is `PIER_BSTR_COLLISION_SHAPE`: SNBT \[{min:\[x,y,z\],max:\[x,y,z\]},…\] |
| <span id="BStrOutlineShape"></span>`BStrOutlineShape` | `7` | BStrOutlineShape is `PIER_BSTR_OUTLINE_SHAPE`: SNBT \[{min,max}\] render outline |
| <span id="BStrDisplayName"></span>`BStrDisplayName` | `8` | BStrDisplayName is `PIER_BSTR_DISPLAY_NAME`: `Block::getDisplayName` |

## `BAct*` {#BAct}

| Name | Value | Description |
|---|---|---|
| <span id="BActHasTag"></span>`BActHasTag` | `0` | BActHasTag is `PIER_BACT_HAS_TAG`: sarg=tag → out "0"/"1" `Block::hasTag` |
| <span id="BActGetState"></span>`BActGetState` | `1` | BActGetState is `PIER_BACT_GET_STATE`: sarg=state name → out value string `Block::getState` |
| <span id="BActPopResource"></span>`BActPopResource` | `2` | BActPopResource is `PIER_BACT_POP_RESOURCE`: sarg=item SNBT → pop resource at pos `Block::popResource` |
| <span id="BActAsItem"></span>`BActAsItem` | `3` | BActAsItem is `PIER_BACT_AS_ITEM`: → out item SNBT `Block::asItemInstance` |
