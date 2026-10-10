# Go：物品与容器

## 函数 {#functions}

### `ContainerItems` {#ContainerItems}

```go
func ContainerItems(ref ContainerRef) ([]SlotItem, error)
```

列出一个容器里有东西的格子。只能在服务器线程调用。

- 参数：
    - ref : `ContainerRef`
- 返回值类型：`([]SlotItem, error)`
- 对应槽位：[`container_get_items`](../cpp/item.md#container_get_items)

### `ItemOf` {#ItemOf}

```go
func ItemOf(snbt string) Item
```

这段 SNBT 描述的物品堆。

- 参数：
    - snbt : `string`
- 返回值类型：`Item`

## `Item` {#Item}

```go
type Item struct {
    SNBT string
}
```

以 SNBT 表示的物品堆，是一个值：读取它，就是拿这段 SNBT 去问宿主；修改它会得到一个新的 `Item`。

### `Item.Payload` {#Item.Payload}

```go
func (i Item) Payload() (*Nbt, error)
```

解析后的物品 SNBT。

- 返回值类型：`(*Nbt, error)`

### `Item.Enchants` {#Item.Enchants}

```go
func (i Item) Enchants() (string, error)
```

以 SNBT 列出这个物品的附魔。

- 返回值类型：`(string, error)`
- 对应槽位：[`item_get_enchants`](../cpp/item.md#item_get_enchants)

### `Item.Matches` {#Item.Matches}

```go
func (i Item) Matches(other Item) (bool, error)
```

判断两个物品能否堆叠：同一种物品，数据值和用户数据都相同。

- 参数：
    - other : `Item`
- 返回值类型：`(bool, error)`
- 对应槽位：[`item_matches`](../cpp/item.md#item_matches)

### `Item.Transform` {#Item.Transform}

```go
func (i Item) Transform(op int32, sarg string, narg float64) (Item, error)
```

执行 abi.h 里的某一个 `PIER_IOP_*` 操作，返回新的物品。

- 参数：
    - op : `int32`
    - sarg : `string`
    - narg : `float64`
- 返回值类型：`(Item, error)`
- 对应槽位：[`item_transform`](../cpp/item.md#item_transform)

### `Item.Count` {#Item.Count}

```go
func (i Item) Count() (float64, error)
```

读取 `PIER_IPROP_COUNT`：取自 `ItemStackBase::mCount`

- 返回值类型：`(float64, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.MaxStackSize` {#Item.MaxStackSize}

```go
func (i Item) MaxStackSize() (float64, error)
```

读取 `PIER_IPROP_MAX_STACK_SIZE`：取自 `ItemStackBase::getMaxStackSize`

- 返回值类型：`(float64, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.AuxValue` {#Item.AuxValue}

```go
func (i Item) AuxValue() (float64, error)
```

读取 `PIER_IPROP_AUX_VALUE`：取自 `ItemStackBase::getAuxValue`

- 返回值类型：`(float64, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.Id` {#Item.Id}

```go
func (i Item) Id() (float64, error)
```

读取 `PIER_IPROP_ID`：取自 `ItemStackBase::getId`

- 返回值类型：`(float64, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.Damage` {#Item.Damage}

```go
func (i Item) Damage() (float64, error)
```

读取 `PIER_IPROP_DAMAGE`：取自 `ItemStackBase::getDamageValue`

- 返回值类型：`(float64, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.IsNull` {#Item.IsNull}

```go
func (i Item) IsNull() (bool, error)
```

读取 `PIER_IPROP_IS_NULL`：取自 `ItemStackBase::isNull`

- 返回值类型：`(bool, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.IsBlock` {#Item.IsBlock}

```go
func (i Item) IsBlock() (bool, error)
```

读取 `PIER_IPROP_IS_BLOCK`：取自 `ItemStackBase::isBlock`

- 返回值类型：`(bool, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.IsEnchanted` {#Item.IsEnchanted}

```go
func (i Item) IsEnchanted() (bool, error)
```

读取 `PIER_IPROP_IS_ENCHANTED`：取自 `ItemStackBase::isEnchanted`

- 返回值类型：`(bool, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.IsArmor` {#Item.IsArmor}

```go
func (i Item) IsArmor() (bool, error)
```

读取 `PIER_IPROP_IS_ARMOR`：取自 `ItemStackBase::isArmorItem`

- 返回值类型：`(bool, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.IsDamageable` {#Item.IsDamageable}

```go
func (i Item) IsDamageable() (bool, error)
```

读取 `PIER_IPROP_IS_DAMAGEABLE`：取自 `ItemStackBase::isDamageableItem`

- 返回值类型：`(bool, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.IsDamaged` {#Item.IsDamaged}

```go
func (i Item) IsDamaged() (bool, error)
```

读取 `PIER_IPROP_IS_DAMAGED`：取自 `ItemStackBase::isDamaged`

- 返回值类型：`(bool, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.MaxDamage` {#Item.MaxDamage}

```go
func (i Item) MaxDamage() (float64, error)
```

读取 `PIER_IPROP_MAX_DAMAGE`：取自 `ItemStackBase::getMaxDamage`

- 返回值类型：`(float64, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.IsUnbreakable` {#Item.IsUnbreakable}

```go
func (i Item) IsUnbreakable() (bool, error)
```

读取 `PIER_IPROP_IS_UNBREAKABLE`：取自 `ItemStackBase::isUnbreakable`

- 返回值类型：`(bool, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.HasDurability` {#Item.HasDurability}

```go
func (i Item) HasDurability() (bool, error)
```

读取 `PIER_IPROP_HAS_DURABILITY`：取自 `ItemStackBase::hasDurability`

- 返回值类型：`(bool, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.IsPotion` {#Item.IsPotion}

```go
func (i Item) IsPotion() (bool, error)
```

读取 `PIER_IPROP_IS_POTION`：取自 `ItemStackBase::isPotionItem`

- 返回值类型：`(bool, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.IsThrowable` {#Item.IsThrowable}

```go
func (i Item) IsThrowable() (bool, error)
```

读取 `PIER_IPROP_IS_THROWABLE`：取自 `ItemStackBase::isThrowable`

- 返回值类型：`(bool, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.IsFireResistant` {#Item.IsFireResistant}

```go
func (i Item) IsFireResistant() (bool, error)
```

读取 `PIER_IPROP_IS_FIRE_RESISTANT`：取自 `ItemStackBase::isFireResistant`

- 返回值类型：`(bool, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.AttackDamage` {#Item.AttackDamage}

```go
func (i Item) AttackDamage() (float64, error)
```

读取 `PIER_IPROP_ATTACK_DAMAGE`：取自 `ItemStackBase::getAttackDamage`

- 返回值类型：`(float64, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.RepairCost` {#Item.RepairCost}

```go
func (i Item) RepairCost() (float64, error)
```

读取 `PIER_IPROP_REPAIR_COST`：取自 `ItemStackBase::getBaseRepairCost`

- 返回值类型：`(float64, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.EnchantValue` {#Item.EnchantValue}

```go
func (i Item) EnchantValue() (float64, error)
```

读取 `PIER_IPROP_ENCHANT_VALUE`：取自 `ItemStackBase::getEnchantValue`

- 返回值类型：`(float64, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.IsStackable` {#Item.IsStackable}

```go
func (i Item) IsStackable() (bool, error)
```

读取 `PIER_IPROP_IS_STACKABLE`：取自 `ItemStackBase::isStackable`

- 返回值类型：`(bool, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.IsMusicDisc` {#Item.IsMusicDisc}

```go
func (i Item) IsMusicDisc() (bool, error)
```

读取 `PIER_IPROP_IS_MUSIC_DISC`：取自 `ItemStackBase::isMusicDiscItem`

- 返回值类型：`(bool, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.IsOffhand` {#Item.IsOffhand}

```go
func (i Item) IsOffhand() (bool, error)
```

读取 `PIER_IPROP_IS_OFFHAND`：取自 `ItemStackBase::isOffhandItem`

- 返回值类型：`(bool, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.UseDuration` {#Item.UseDuration}

```go
func (i Item) UseDuration() (float64, error)
```

读取 `PIER_IPROP_USE_DURATION`：取自 `ItemStackBase::getMaxUseDuration`

- 返回值类型：`(float64, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.IsGlint` {#Item.IsGlint}

```go
func (i Item) IsGlint() (bool, error)
```

读取 `PIER_IPROP_IS_GLINT`：取自 `ItemStackBase::isGlint`

- 返回值类型：`(bool, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.IsBundle` {#Item.IsBundle}

```go
func (i Item) IsBundle() (bool, error)
```

读取 `PIER_IPROP_IS_BUNDLE`：取自 `ItemStackBase::isBundle`

- 返回值类型：`(bool, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.HasUserData` {#Item.HasUserData}

```go
func (i Item) HasUserData() (bool, error)
```

读取 `PIER_IPROP_HAS_USER_DATA`：取自 `ItemStackBase::hasUserData`

- 返回值类型：`(bool, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.HasCustomName` {#Item.HasCustomName}

```go
func (i Item) HasCustomName() (bool, error)
```

读取 `PIER_IPROP_HAS_CUSTOM_NAME`：取自 `ItemStackBase::hasCustomHoverName`

- 返回值类型：`(bool, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.TypeName` {#Item.TypeName}

```go
func (i Item) TypeName() (string, error)
```

读取 `PIER_ISTR_TYPE_NAME`：取自 `ItemStackBase::getTypeName`（形如 `"minecraft:apple"`）

- 返回值类型：`(string, error)`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `Item.Name` {#Item.Name}

```go
func (i Item) Name() (string, error)
```

读取 `PIER_ISTR_NAME`：取自 `ItemStackBase::getName`（显示名）

- 返回值类型：`(string, error)`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `Item.CustomName` {#Item.CustomName}

```go
func (i Item) CustomName() (string, error)
```

读取 `PIER_ISTR_CUSTOM_NAME`：取自 `ItemStackBase::getCustomName`

- 返回值类型：`(string, error)`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `Item.RawNameId` {#Item.RawNameId}

```go
func (i Item) RawNameId() (string, error)
```

读取 `PIER_ISTR_RAW_NAME_ID`：取自 `ItemStackBase::getRawNameId`

- 返回值类型：`(string, error)`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `Item.Lore` {#Item.Lore}

```go
func (i Item) Lore() (string, error)
```

读取 `PIER_ISTR_LORE`：SNBT 列表 `["l1","l2"]`，取自 `ItemStackBase::getCustomLore`

- 返回值类型：`(string, error)`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `Item.CanDestroy` {#Item.CanDestroy}

```go
func (i Item) CanDestroy() (string, error)
```

读取 `PIER_ISTR_CAN_DESTROY`：SNBT 列表 `["minecraft:stone",…]`

- 返回值类型：`(string, error)`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `Item.CanPlaceOn` {#Item.CanPlaceOn}

```go
func (i Item) CanPlaceOn() (string, error)
```

读取 `PIER_ISTR_CAN_PLACE_ON`：SNBT 列表

- 返回值类型：`(string, error)`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `Item.UserData` {#Item.UserData}

```go
func (i Item) UserData() (string, error)
```

读取 `PIER_ISTR_USER_DATA`：完整的 NBT 用户数据，以 SNBT 表示

- 返回值类型：`(string, error)`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `Item.HoverName` {#Item.HoverName}

```go
func (i Item) HoverName() (string, error)
```

读取 `PIER_ISTR_HOVER_NAME`：取自 `ItemStackBase::getHoverName`

- 返回值类型：`(string, error)`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `Item.EffectName` {#Item.EffectName}

```go
func (i Item) EffectName() (string, error)
```

读取 `PIER_ISTR_EFFECT_NAME`：取自 `ItemStackBase::getEffectName`

- 返回值类型：`(string, error)`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `Item.Color` {#Item.Color}

```go
func (i Item) Color() (string, error)
```

读取 `PIER_ISTR_COLOR`：SNBT `{r,g,b}`，取自 `ItemStackBase::getColor`

- 返回值类型：`(string, error)`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

## `Container` {#Container}

```go
type Container struct {
    Ref ContainerRef
}
```

一名玩家或一个方块的容器。

### `Container.Size` {#Container.Size}

```go
func (c Container) Size() (int32, error)
```

格子的数量。

- 返回值类型：`(int32, error)`
- 对应槽位：[`container_size`](../cpp/item.md#container_size)

### `Container.Item` {#Container.Item}

```go
func (c Container) Item(slot int32) (Item, error)
```

某一格里的物品。

- 参数：
    - slot : `int32`
- 返回值类型：`(Item, error)`
- 对应槽位：[`container_get_item`](../cpp/item.md#container_get_item)

### `Container.SetItem` {#Container.SetItem}

```go
func (c Container) SetItem(slot int32, item Item) error
```

把一个物品放进某一格。

- 参数：
    - slot : `int32`
    - item : `Item`
- 返回值类型：`error`
- 对应槽位：[`container_set_item`](../cpp/item.md#container_set_item)

### `Container.AddItem` {#Container.AddItem}

```go
func (c Container) AddItem(item Item) error
```

把一个物品放进放得下的地方。

- 参数：
    - item : `Item`
- 返回值类型：`error`
- 对应槽位：[`container_add_item`](../cpp/item.md#container_add_item)

### `Container.RemoveItem` {#Container.RemoveItem}

```go
func (c Container) RemoveItem(slot, count int32) error
```

从某一格移除 `count` 个物品。

- 参数：
    - slot : `int32`
    - count : `int32`
- 返回值类型：`error`
- 对应槽位：[`container_remove_item`](../cpp/item.md#container_remove_item)

### `Container.Clear` {#Container.Clear}

```go
func (c Container) Clear() error
```

清空这个容器。

- 返回值类型：`error`
- 对应槽位：[`container_clear`](../cpp/item.md#container_clear)

### `Container.Items` {#Container.Items}

```go
func (c Container) Items() ([]SlotItem, error)
```

列出有东西的格子。

- 返回值类型：`([]SlotItem, error)`
- 对应槽位：[`container_get_items`](../cpp/item.md#container_get_items)

## `SlotItem` {#SlotItem}

```go
type SlotItem struct {
    Slot int32
    SNBT string
}
```

容器里一个有东西的格子，以及以 SNBT 表示的物品。

## `ContainerRef` {#ContainerRef}

```go
type ContainerRef struct {
    Which   int32
    Player  PlayerSel
    Dim     int32
    X, Y, Z int32
}
```

指定一个容器：玩家的物品栏、末影箱、盔甲栏或副手，或者一个方块的容器。

## `IProp*` {#IProp}

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="IPropCount"></span>`IPropCount` | `0` | 即 `PIER_IPROP_COUNT`：取自 `ItemStackBase::mCount` |
| <span id="IPropMaxStackSize"></span>`IPropMaxStackSize` | `1` | 即 `PIER_IPROP_MAX_STACK_SIZE`：取自 `ItemStackBase::getMaxStackSize` |
| <span id="IPropAuxValue"></span>`IPropAuxValue` | `2` | 即 `PIER_IPROP_AUX_VALUE`：取自 `ItemStackBase::getAuxValue` |
| <span id="IPropId"></span>`IPropId` | `3` | 即 `PIER_IPROP_ID`：取自 `ItemStackBase::getId` |
| <span id="IPropDamage"></span>`IPropDamage` | `4` | 即 `PIER_IPROP_DAMAGE`：取自 `ItemStackBase::getDamageValue` |
| <span id="IPropIsNull"></span>`IPropIsNull` | `5` | 即 `PIER_IPROP_IS_NULL`：取自 `ItemStackBase::isNull` |
| <span id="IPropIsBlock"></span>`IPropIsBlock` | `6` | 即 `PIER_IPROP_IS_BLOCK`：取自 `ItemStackBase::isBlock` |
| <span id="IPropIsEnchanted"></span>`IPropIsEnchanted` | `7` | 即 `PIER_IPROP_IS_ENCHANTED`：取自 `ItemStackBase::isEnchanted` |
| <span id="IPropIsArmor"></span>`IPropIsArmor` | `8` | 即 `PIER_IPROP_IS_ARMOR`：取自 `ItemStackBase::isArmorItem` |
| <span id="IPropIsDamageable"></span>`IPropIsDamageable` | `9` | 即 `PIER_IPROP_IS_DAMAGEABLE`：取自 `ItemStackBase::isDamageableItem` |
| <span id="IPropIsDamaged"></span>`IPropIsDamaged` | `10` | 即 `PIER_IPROP_IS_DAMAGED`：取自 `ItemStackBase::isDamaged` |
| <span id="IPropMaxDamage"></span>`IPropMaxDamage` | `11` | 即 `PIER_IPROP_MAX_DAMAGE`：取自 `ItemStackBase::getMaxDamage` |
| <span id="IPropIsUnbreakable"></span>`IPropIsUnbreakable` | `12` | 即 `PIER_IPROP_IS_UNBREAKABLE`：取自 `ItemStackBase::isUnbreakable` |
| <span id="IPropHasDurability"></span>`IPropHasDurability` | `13` | 即 `PIER_IPROP_HAS_DURABILITY`：取自 `ItemStackBase::hasDurability` |
| <span id="IPropIsPotion"></span>`IPropIsPotion` | `14` | 即 `PIER_IPROP_IS_POTION`：取自 `ItemStackBase::isPotionItem` |
| <span id="IPropIsThrowable"></span>`IPropIsThrowable` | `15` | 即 `PIER_IPROP_IS_THROWABLE`：取自 `ItemStackBase::isThrowable` |
| <span id="IPropIsFireResistant"></span>`IPropIsFireResistant` | `16` | 即 `PIER_IPROP_IS_FIRE_RESISTANT`：取自 `ItemStackBase::isFireResistant` |
| <span id="IPropAttackDamage"></span>`IPropAttackDamage` | `17` | 即 `PIER_IPROP_ATTACK_DAMAGE`：取自 `ItemStackBase::getAttackDamage` |
| <span id="IPropRepairCost"></span>`IPropRepairCost` | `18` | 即 `PIER_IPROP_REPAIR_COST`：取自 `ItemStackBase::getBaseRepairCost` |
| <span id="IPropEnchantValue"></span>`IPropEnchantValue` | `19` | 即 `PIER_IPROP_ENCHANT_VALUE`：取自 `ItemStackBase::getEnchantValue` |
| <span id="IPropIsStackable"></span>`IPropIsStackable` | `20` | 即 `PIER_IPROP_IS_STACKABLE`：取自 `ItemStackBase::isStackable` |
| <span id="IPropIsMusicDisc"></span>`IPropIsMusicDisc` | `21` | 即 `PIER_IPROP_IS_MUSIC_DISC`：取自 `ItemStackBase::isMusicDiscItem` |
| <span id="IPropIsOffhand"></span>`IPropIsOffhand` | `22` | 即 `PIER_IPROP_IS_OFFHAND`：取自 `ItemStackBase::isOffhandItem` |
| <span id="IPropUseDuration"></span>`IPropUseDuration` | `23` | 即 `PIER_IPROP_USE_DURATION`：取自 `ItemStackBase::getMaxUseDuration` |
| <span id="IPropIsGlint"></span>`IPropIsGlint` | `24` | 即 `PIER_IPROP_IS_GLINT`：取自 `ItemStackBase::isGlint` |
| <span id="IPropIsBundle"></span>`IPropIsBundle` | `25` | 即 `PIER_IPROP_IS_BUNDLE`：取自 `ItemStackBase::isBundle` |
| <span id="IPropHasUserData"></span>`IPropHasUserData` | `26` | 即 `PIER_IPROP_HAS_USER_DATA`：取自 `ItemStackBase::hasUserData` |
| <span id="IPropHasCustomName"></span>`IPropHasCustomName` | `27` | 即 `PIER_IPROP_HAS_CUSTOM_NAME`：取自 `ItemStackBase::hasCustomHoverName` |

## `IStr*` {#IStr}

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="IStrTypeName"></span>`IStrTypeName` | `0` | 即 `PIER_ISTR_TYPE_NAME`：取自 `ItemStackBase::getTypeName`（形如 `"minecraft:apple"`） |
| <span id="IStrName"></span>`IStrName` | `1` | 即 `PIER_ISTR_NAME`：取自 `ItemStackBase::getName`（显示名） |
| <span id="IStrCustomName"></span>`IStrCustomName` | `2` | 即 `PIER_ISTR_CUSTOM_NAME`：取自 `ItemStackBase::getCustomName` |
| <span id="IStrRawNameId"></span>`IStrRawNameId` | `3` | 即 `PIER_ISTR_RAW_NAME_ID`：取自 `ItemStackBase::getRawNameId` |
| <span id="IStrLore"></span>`IStrLore` | `4` | 即 `PIER_ISTR_LORE`：SNBT 列表 `["l1","l2"]`，取自 `ItemStackBase::getCustomLore` |
| <span id="IStrCanDestroy"></span>`IStrCanDestroy` | `5` | 即 `PIER_ISTR_CAN_DESTROY`：SNBT 列表 `["minecraft:stone",…]` |
| <span id="IStrCanPlaceOn"></span>`IStrCanPlaceOn` | `6` | 即 `PIER_ISTR_CAN_PLACE_ON`：SNBT 列表 |
| <span id="IStrUserData"></span>`IStrUserData` | `7` | 即 `PIER_ISTR_USER_DATA`：完整的 NBT 用户数据，以 SNBT 表示 |
| <span id="IStrHoverName"></span>`IStrHoverName` | `8` | 即 `PIER_ISTR_HOVER_NAME`：取自 `ItemStackBase::getHoverName` |
| <span id="IStrEffectName"></span>`IStrEffectName` | `9` | 即 `PIER_ISTR_EFFECT_NAME`：取自 `ItemStackBase::getEffectName` |
| <span id="IStrColor"></span>`IStrColor` | `10` | 即 `PIER_ISTR_COLOR`：SNBT `{r,g,b}`，取自 `ItemStackBase::getColor` |

## `Container*` {#Container-values}

`ContainerRef.Which` 的取值。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="ContainerInventory"></span>`ContainerInventory` | `0` |  |
| <span id="ContainerEnderChest"></span>`ContainerEnderChest` | `1` |  |
| <span id="ContainerArmor"></span>`ContainerArmor` | `2` |  |
| <span id="ContainerOffhand"></span>`ContainerOffhand` | `3` |  |
| <span id="ContainerBlock"></span>`ContainerBlock` | `4` |  |
