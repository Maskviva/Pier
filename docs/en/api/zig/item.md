# Zig: Items and containers

## `Item` {#Item}

```zig
pub const Item = struct {
    item_snbt: []const u8,
    // ...
};
```

An item stack as SNBT, a value: reading it asks the host about that SNBT.

### `Item.of` {#Item.of}

```zig
pub fn of(item_snbt_value: []const u8) Item
```

- Parameters:
    - item_snbt_value : `[]const u8`
- Return type: `Item`

### `Item.count` {#Item.count}

```zig
pub fn count(self: Item) core.Error!f64
```

`PIER_IPROP_COUNT`: `ItemStackBase::mCount`

- Return type: `core.Error!f64`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.maxStackSize` {#Item.maxStackSize}

```zig
pub fn maxStackSize(self: Item) core.Error!f64
```

`PIER_IPROP_MAX_STACK_SIZE`: `ItemStackBase::getMaxStackSize`

- Return type: `core.Error!f64`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.auxValue` {#Item.auxValue}

```zig
pub fn auxValue(self: Item) core.Error!f64
```

`PIER_IPROP_AUX_VALUE`: `ItemStackBase::getAuxValue`

- Return type: `core.Error!f64`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.id` {#Item.id}

```zig
pub fn id(self: Item) core.Error!f64
```

`PIER_IPROP_ID`: `ItemStackBase::getId`

- Return type: `core.Error!f64`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.damage` {#Item.damage}

```zig
pub fn damage(self: Item) core.Error!f64
```

`PIER_IPROP_DAMAGE`: `ItemStackBase::getDamageValue`

- Return type: `core.Error!f64`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isNull` {#Item.isNull}

```zig
pub fn isNull(self: Item) core.Error!bool
```

`PIER_IPROP_IS_NULL`: `ItemStackBase::isNull`

- Return type: `core.Error!bool`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isBlock` {#Item.isBlock}

```zig
pub fn isBlock(self: Item) core.Error!bool
```

`PIER_IPROP_IS_BLOCK`: `ItemStackBase::isBlock`

- Return type: `core.Error!bool`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isEnchanted` {#Item.isEnchanted}

```zig
pub fn isEnchanted(self: Item) core.Error!bool
```

`PIER_IPROP_IS_ENCHANTED`: `ItemStackBase::isEnchanted`

- Return type: `core.Error!bool`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isArmor` {#Item.isArmor}

```zig
pub fn isArmor(self: Item) core.Error!bool
```

`PIER_IPROP_IS_ARMOR`: `ItemStackBase::isArmorItem`

- Return type: `core.Error!bool`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isDamageable` {#Item.isDamageable}

```zig
pub fn isDamageable(self: Item) core.Error!bool
```

`PIER_IPROP_IS_DAMAGEABLE`: `ItemStackBase::isDamageableItem`

- Return type: `core.Error!bool`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isDamaged` {#Item.isDamaged}

```zig
pub fn isDamaged(self: Item) core.Error!bool
```

`PIER_IPROP_IS_DAMAGED`: `ItemStackBase::isDamaged`

- Return type: `core.Error!bool`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.maxDamage` {#Item.maxDamage}

```zig
pub fn maxDamage(self: Item) core.Error!f64
```

`PIER_IPROP_MAX_DAMAGE`: `ItemStackBase::getMaxDamage`

- Return type: `core.Error!f64`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isUnbreakable` {#Item.isUnbreakable}

```zig
pub fn isUnbreakable(self: Item) core.Error!bool
```

`PIER_IPROP_IS_UNBREAKABLE`: `ItemStackBase::isUnbreakable`

- Return type: `core.Error!bool`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.hasDurability` {#Item.hasDurability}

```zig
pub fn hasDurability(self: Item) core.Error!bool
```

`PIER_IPROP_HAS_DURABILITY`: `ItemStackBase::hasDurability`

- Return type: `core.Error!bool`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isPotion` {#Item.isPotion}

```zig
pub fn isPotion(self: Item) core.Error!bool
```

`PIER_IPROP_IS_POTION`: `ItemStackBase::isPotionItem`

- Return type: `core.Error!bool`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isThrowable` {#Item.isThrowable}

```zig
pub fn isThrowable(self: Item) core.Error!bool
```

`PIER_IPROP_IS_THROWABLE`: `ItemStackBase::isThrowable`

- Return type: `core.Error!bool`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isFireResistant` {#Item.isFireResistant}

```zig
pub fn isFireResistant(self: Item) core.Error!bool
```

`PIER_IPROP_IS_FIRE_RESISTANT`: `ItemStackBase::isFireResistant`

- Return type: `core.Error!bool`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.attackDamage` {#Item.attackDamage}

```zig
pub fn attackDamage(self: Item) core.Error!f64
```

`PIER_IPROP_ATTACK_DAMAGE`: `ItemStackBase::getAttackDamage`

- Return type: `core.Error!f64`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.repairCost` {#Item.repairCost}

```zig
pub fn repairCost(self: Item) core.Error!f64
```

`PIER_IPROP_REPAIR_COST`: `ItemStackBase::getBaseRepairCost`

- Return type: `core.Error!f64`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.enchantValue` {#Item.enchantValue}

```zig
pub fn enchantValue(self: Item) core.Error!f64
```

`PIER_IPROP_ENCHANT_VALUE`: `ItemStackBase::getEnchantValue`

- Return type: `core.Error!f64`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isStackable` {#Item.isStackable}

```zig
pub fn isStackable(self: Item) core.Error!bool
```

`PIER_IPROP_IS_STACKABLE`: `ItemStackBase::isStackable`

- Return type: `core.Error!bool`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isMusicDisc` {#Item.isMusicDisc}

```zig
pub fn isMusicDisc(self: Item) core.Error!bool
```

`PIER_IPROP_IS_MUSIC_DISC`: `ItemStackBase::isMusicDiscItem`

- Return type: `core.Error!bool`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isOffhand` {#Item.isOffhand}

```zig
pub fn isOffhand(self: Item) core.Error!bool
```

`PIER_IPROP_IS_OFFHAND`: `ItemStackBase::isOffhandItem`

- Return type: `core.Error!bool`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.useDuration` {#Item.useDuration}

```zig
pub fn useDuration(self: Item) core.Error!f64
```

`PIER_IPROP_USE_DURATION`: `ItemStackBase::getMaxUseDuration`

- Return type: `core.Error!f64`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isGlint` {#Item.isGlint}

```zig
pub fn isGlint(self: Item) core.Error!bool
```

`PIER_IPROP_IS_GLINT`: `ItemStackBase::isGlint`

- Return type: `core.Error!bool`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.isBundle` {#Item.isBundle}

```zig
pub fn isBundle(self: Item) core.Error!bool
```

`PIER_IPROP_IS_BUNDLE`: `ItemStackBase::isBundle`

- Return type: `core.Error!bool`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.hasUserData` {#Item.hasUserData}

```zig
pub fn hasUserData(self: Item) core.Error!bool
```

`PIER_IPROP_HAS_USER_DATA`: `ItemStackBase::hasUserData`

- Return type: `core.Error!bool`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.hasCustomName` {#Item.hasCustomName}

```zig
pub fn hasCustomName(self: Item) core.Error!bool
```

`PIER_IPROP_HAS_CUSTOM_NAME`: `ItemStackBase::hasCustomHoverName`

- Return type: `core.Error!bool`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Item.typeName` {#Item.typeName}

```zig
pub fn typeName(self: Item, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ISTR_TYPE_NAME`: `ItemStackBase::getTypeName` ("minecraft:apple")

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`item_get_str`](../cpp/item.md#item_get_str)

### `Item.name` {#Item.name}

```zig
pub fn name(self: Item, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ISTR_NAME`: `ItemStackBase::getName` (display)

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`item_get_str`](../cpp/item.md#item_get_str)

### `Item.customName` {#Item.customName}

```zig
pub fn customName(self: Item, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ISTR_CUSTOM_NAME`: `ItemStackBase::getCustomName`

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`item_get_str`](../cpp/item.md#item_get_str)

### `Item.rawNameId` {#Item.rawNameId}

```zig
pub fn rawNameId(self: Item, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ISTR_RAW_NAME_ID`: `ItemStackBase::getRawNameId`

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`item_get_str`](../cpp/item.md#item_get_str)

### `Item.lore` {#Item.lore}

```zig
pub fn lore(self: Item, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ISTR_LORE`: SNBT list \["l1","l2"\] `ItemStackBase::getCustomLore`

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`item_get_str`](../cpp/item.md#item_get_str)

### `Item.canDestroy` {#Item.canDestroy}

```zig
pub fn canDestroy(self: Item, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ISTR_CAN_DESTROY`: SNBT list \["minecraft:stone",…\]

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`item_get_str`](../cpp/item.md#item_get_str)

### `Item.canPlaceOn` {#Item.canPlaceOn}

```zig
pub fn canPlaceOn(self: Item, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ISTR_CAN_PLACE_ON`: SNBT list

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`item_get_str`](../cpp/item.md#item_get_str)

### `Item.userData` {#Item.userData}

```zig
pub fn userData(self: Item, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ISTR_USER_DATA`: full NBT user data as SNBT

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`item_get_str`](../cpp/item.md#item_get_str)

### `Item.hoverName` {#Item.hoverName}

```zig
pub fn hoverName(self: Item, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ISTR_HOVER_NAME`: `ItemStackBase::getHoverName`

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`item_get_str`](../cpp/item.md#item_get_str)

### `Item.effectName` {#Item.effectName}

```zig
pub fn effectName(self: Item, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ISTR_EFFECT_NAME`: `ItemStackBase::getEffectName`

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`item_get_str`](../cpp/item.md#item_get_str)

### `Item.color` {#Item.color}

```zig
pub fn color(self: Item, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ISTR_COLOR`: SNBT {r,g,b} `ItemStackBase::getColor`

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`item_get_str`](../cpp/item.md#item_get_str)
