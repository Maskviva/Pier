# Zig：物品与容器

## `Item` {#Item}

```zig
pub const Item = struct {
    item_snbt: []const u8,
    // ...
};
```

以 SNBT 表示的物品堆，是一个值：读取它，就是拿这段 SNBT 去问宿主。

### `Item.of` {#Item.of}

```zig
pub fn of(item_snbt_value: []const u8) Item
```

- 参数：
    - item_snbt_value : `[]const u8`
- 返回值类型：`Item`

### `Item.count` {#Item.count}

```zig
pub fn count(self: Item) core.Error!f64
```

`PIER_IPROP_COUNT`：取自 `ItemStackBase::mCount`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.maxStackSize` {#Item.maxStackSize}

```zig
pub fn maxStackSize(self: Item) core.Error!f64
```

`PIER_IPROP_MAX_STACK_SIZE`：取自 `ItemStackBase::getMaxStackSize`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.auxValue` {#Item.auxValue}

```zig
pub fn auxValue(self: Item) core.Error!f64
```

`PIER_IPROP_AUX_VALUE`：取自 `ItemStackBase::getAuxValue`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.id` {#Item.id}

```zig
pub fn id(self: Item) core.Error!f64
```

`PIER_IPROP_ID`：取自 `ItemStackBase::getId`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.damage` {#Item.damage}

```zig
pub fn damage(self: Item) core.Error!f64
```

`PIER_IPROP_DAMAGE`：取自 `ItemStackBase::getDamageValue`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isNull` {#Item.isNull}

```zig
pub fn isNull(self: Item) core.Error!bool
```

`PIER_IPROP_IS_NULL`：取自 `ItemStackBase::isNull`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isBlock` {#Item.isBlock}

```zig
pub fn isBlock(self: Item) core.Error!bool
```

`PIER_IPROP_IS_BLOCK`：取自 `ItemStackBase::isBlock`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isEnchanted` {#Item.isEnchanted}

```zig
pub fn isEnchanted(self: Item) core.Error!bool
```

`PIER_IPROP_IS_ENCHANTED`：取自 `ItemStackBase::isEnchanted`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isArmor` {#Item.isArmor}

```zig
pub fn isArmor(self: Item) core.Error!bool
```

`PIER_IPROP_IS_ARMOR`：取自 `ItemStackBase::isArmorItem`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isDamageable` {#Item.isDamageable}

```zig
pub fn isDamageable(self: Item) core.Error!bool
```

`PIER_IPROP_IS_DAMAGEABLE`：取自 `ItemStackBase::isDamageableItem`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isDamaged` {#Item.isDamaged}

```zig
pub fn isDamaged(self: Item) core.Error!bool
```

`PIER_IPROP_IS_DAMAGED`：取自 `ItemStackBase::isDamaged`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.maxDamage` {#Item.maxDamage}

```zig
pub fn maxDamage(self: Item) core.Error!f64
```

`PIER_IPROP_MAX_DAMAGE`：取自 `ItemStackBase::getMaxDamage`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isUnbreakable` {#Item.isUnbreakable}

```zig
pub fn isUnbreakable(self: Item) core.Error!bool
```

`PIER_IPROP_IS_UNBREAKABLE`：取自 `ItemStackBase::isUnbreakable`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.hasDurability` {#Item.hasDurability}

```zig
pub fn hasDurability(self: Item) core.Error!bool
```

`PIER_IPROP_HAS_DURABILITY`：取自 `ItemStackBase::hasDurability`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isPotion` {#Item.isPotion}

```zig
pub fn isPotion(self: Item) core.Error!bool
```

`PIER_IPROP_IS_POTION`：取自 `ItemStackBase::isPotionItem`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isThrowable` {#Item.isThrowable}

```zig
pub fn isThrowable(self: Item) core.Error!bool
```

`PIER_IPROP_IS_THROWABLE`：取自 `ItemStackBase::isThrowable`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isFireResistant` {#Item.isFireResistant}

```zig
pub fn isFireResistant(self: Item) core.Error!bool
```

`PIER_IPROP_IS_FIRE_RESISTANT`：取自 `ItemStackBase::isFireResistant`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.attackDamage` {#Item.attackDamage}

```zig
pub fn attackDamage(self: Item) core.Error!f64
```

`PIER_IPROP_ATTACK_DAMAGE`：取自 `ItemStackBase::getAttackDamage`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.repairCost` {#Item.repairCost}

```zig
pub fn repairCost(self: Item) core.Error!f64
```

`PIER_IPROP_REPAIR_COST`：取自 `ItemStackBase::getBaseRepairCost`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.enchantValue` {#Item.enchantValue}

```zig
pub fn enchantValue(self: Item) core.Error!f64
```

`PIER_IPROP_ENCHANT_VALUE`：取自 `ItemStackBase::getEnchantValue`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isStackable` {#Item.isStackable}

```zig
pub fn isStackable(self: Item) core.Error!bool
```

`PIER_IPROP_IS_STACKABLE`：取自 `ItemStackBase::isStackable`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isMusicDisc` {#Item.isMusicDisc}

```zig
pub fn isMusicDisc(self: Item) core.Error!bool
```

`PIER_IPROP_IS_MUSIC_DISC`：取自 `ItemStackBase::isMusicDiscItem`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isOffhand` {#Item.isOffhand}

```zig
pub fn isOffhand(self: Item) core.Error!bool
```

`PIER_IPROP_IS_OFFHAND`：取自 `ItemStackBase::isOffhandItem`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.useDuration` {#Item.useDuration}

```zig
pub fn useDuration(self: Item) core.Error!f64
```

`PIER_IPROP_USE_DURATION`：取自 `ItemStackBase::getMaxUseDuration`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isGlint` {#Item.isGlint}

```zig
pub fn isGlint(self: Item) core.Error!bool
```

`PIER_IPROP_IS_GLINT`：取自 `ItemStackBase::isGlint`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isBundle` {#Item.isBundle}

```zig
pub fn isBundle(self: Item) core.Error!bool
```

`PIER_IPROP_IS_BUNDLE`：取自 `ItemStackBase::isBundle`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.hasUserData` {#Item.hasUserData}

```zig
pub fn hasUserData(self: Item) core.Error!bool
```

`PIER_IPROP_HAS_USER_DATA`：取自 `ItemStackBase::hasUserData`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.hasCustomName` {#Item.hasCustomName}

```zig
pub fn hasCustomName(self: Item) core.Error!bool
```

`PIER_IPROP_HAS_CUSTOM_NAME`：取自 `ItemStackBase::hasCustomHoverName`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Item.typeName` {#Item.typeName}

```zig
pub fn typeName(self: Item, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ISTR_TYPE_NAME`：取自 `ItemStackBase::getTypeName`（形如 `"minecraft:apple"`）

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `Item.name` {#Item.name}

```zig
pub fn name(self: Item, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ISTR_NAME`：取自 `ItemStackBase::getName`（显示名）

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `Item.customName` {#Item.customName}

```zig
pub fn customName(self: Item, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ISTR_CUSTOM_NAME`：取自 `ItemStackBase::getCustomName`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `Item.rawNameId` {#Item.rawNameId}

```zig
pub fn rawNameId(self: Item, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ISTR_RAW_NAME_ID`：取自 `ItemStackBase::getRawNameId`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `Item.lore` {#Item.lore}

```zig
pub fn lore(self: Item, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ISTR_LORE`：SNBT 列表 `["l1","l2"]`，取自 `ItemStackBase::getCustomLore`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `Item.canDestroy` {#Item.canDestroy}

```zig
pub fn canDestroy(self: Item, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ISTR_CAN_DESTROY`：SNBT 列表 `["minecraft:stone",…]`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `Item.canPlaceOn` {#Item.canPlaceOn}

```zig
pub fn canPlaceOn(self: Item, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ISTR_CAN_PLACE_ON`：SNBT 列表

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `Item.userData` {#Item.userData}

```zig
pub fn userData(self: Item, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ISTR_USER_DATA`：完整的 NBT 用户数据，以 SNBT 表示

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `Item.hoverName` {#Item.hoverName}

```zig
pub fn hoverName(self: Item, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ISTR_HOVER_NAME`：取自 `ItemStackBase::getHoverName`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `Item.effectName` {#Item.effectName}

```zig
pub fn effectName(self: Item, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ISTR_EFFECT_NAME`：取自 `ItemStackBase::getEffectName`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `Item.color` {#Item.color}

```zig
pub fn color(self: Item, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ISTR_COLOR`：SNBT `{r,g,b}`，取自 `ItemStackBase::getColor`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)
