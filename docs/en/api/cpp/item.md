# Items and containers

??? note "Section notes in abi.h"

    **§E items (SNBT value objects) & containers**

    **Item: enchants, matching, NBT (dedicated fns)**

    **Client-side container resync**

    `container_set_item` / `_clear` / `_add_item` all write through `Container::setItem`, which mutates the server's copy and sends nothing. The client keeps rendering whatever it last received, so a bulk rewrite (swapping a player's inventory on a cross-dimension teleport, say) looks like it did nothing until the player clicks a slot and forces a resync.

    Call this once after a batch of writes. Batching matters: this pushes the whole container, so calling it per-slot inside a loop is a packet storm for no benefit.

    **Appended: bulk block reads and writes**

## Slots {#slots}

### `item_get_num` {#item_get_num}

```c
bool (*item_get_num)(PierStr item_snbt, int32_t prop, double* out);
```

- Call: `api->item_get_num(item_snbt, prop, out)`
- Parameters:
    - item_snbt : `PierStr`
    - prop : `int32_t`
    - out : `double*`
- Return type: `bool`
- Section of abi.h: §E items (SNBT value objects) & containers
- Position in the table: slot 43, counting from 0
- Callers in each binding:
    - Rust: [`ItemStack::num`](../rust/item.md#ItemStack.num), [`ItemStack::count`](../rust/item.md#ItemStack.count), [`ItemStack::max_stack_size`](../rust/item.md#ItemStack.max_stack_size), [`ItemStack::aux_value`](../rust/item.md#ItemStack.aux_value), [`ItemStack::id`](../rust/item.md#ItemStack.id), [`ItemStack::damage`](../rust/item.md#ItemStack.damage) and more, 29 in all
    - Go: [`Item.Count`](../go/item.md#Item.Count), [`Item.MaxStackSize`](../go/item.md#Item.MaxStackSize), [`Item.AuxValue`](../go/item.md#Item.AuxValue), [`Item.Id`](../go/item.md#Item.Id), [`Item.Damage`](../go/item.md#Item.Damage), [`Item.IsNull`](../go/item.md#Item.IsNull) and more, 29 in all
    - Zig: [`Item.count`](../zig/item.md#Item.count), [`Item.maxStackSize`](../zig/item.md#Item.maxStackSize), [`Item.auxValue`](../zig/item.md#Item.auxValue), [`Item.id`](../zig/item.md#Item.id), [`Item.damage`](../zig/item.md#Item.damage), [`Item.isNull`](../zig/item.md#Item.isNull) and more, 28 in all

### `item_get_str` {#item_get_str}

```c
bool (*item_get_str)(PierStr item_snbt, int32_t prop, void* ctx, PierStrSink sink);
```

- Call: `api->item_get_str(item_snbt, prop, ctx, sink)`
- Parameters:
    - item_snbt : `PierStr`
    - prop : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: §E items (SNBT value objects) & containers
- Position in the table: slot 44, counting from 0
- Callers in each binding:
    - Rust: [`ItemStack::text`](../rust/item.md#ItemStack.text), [`ItemStack::type_name`](../rust/item.md#ItemStack.type_name), [`ItemStack::name`](../rust/item.md#ItemStack.name), [`ItemStack::custom_name`](../rust/item.md#ItemStack.custom_name), [`ItemStack::hover_name`](../rust/item.md#ItemStack.hover_name), [`ItemStack::raw_name_id`](../rust/item.md#ItemStack.raw_name_id) and more, 11 in all
    - Go: [`Item.TypeName`](../go/item.md#Item.TypeName), [`Item.Name`](../go/item.md#Item.Name), [`Item.CustomName`](../go/item.md#Item.CustomName), [`Item.RawNameId`](../go/item.md#Item.RawNameId), [`Item.Lore`](../go/item.md#Item.Lore), [`Item.CanDestroy`](../go/item.md#Item.CanDestroy) and more, 12 in all
    - Zig: [`Item.typeName`](../zig/item.md#Item.typeName), [`Item.name`](../zig/item.md#Item.name), [`Item.customName`](../zig/item.md#Item.customName), [`Item.rawNameId`](../zig/item.md#Item.rawNameId), [`Item.lore`](../zig/item.md#Item.lore), [`Item.canDestroy`](../zig/item.md#Item.canDestroy) and more, 11 in all

### `item_transform` {#item_transform}

```c
bool (*item_transform)(PierStr item_snbt, int32_t op, PierStr sarg, double narg, void* ctx, PierStrSink out);
```

Rebuild → mutate → serialize; out receives the NEW item SNBT.

- Call: `api->item_transform(item_snbt, op, sarg, narg, ctx, out)`
- Parameters:
    - item_snbt : `PierStr`
    - op : `int32_t`
    - sarg : `PierStr`
    - narg : `double`
    - ctx : `void*`
    - out : `PierStrSink`
- Return type: `bool`
- Section of abi.h: §E items (SNBT value objects) & containers
- Position in the table: slot 45, counting from 0
- Callers in each binding:
    - Rust: [`ItemStack::transform`](../rust/item.md#ItemStack.transform), [`ItemStack::set_custom_name`](../rust/item.md#ItemStack.set_custom_name), [`ItemStack::reset_name`](../rust/item.md#ItemStack.reset_name), [`ItemStack::set_damage`](../rust/item.md#ItemStack.set_damage), [`ItemStack::set_count`](../rust/item.md#ItemStack.set_count), [`ItemStack::set_unbreakable`](../rust/item.md#ItemStack.set_unbreakable) and more, 14 in all
    - Go: [`Item.Transform`](../go/item.md#Item.Transform), [`Raw.ItemTransform`](../go/raw.md#Raw.ItemTransform)

### `container_size` {#container_size}

```c
bool (*container_size)(PierContainerRef ref, int32_t* out);
```

- Call: `api->container_size(ref, out)`
- Parameters:
    - ref : `PierContainerRef`
    - out : `int32_t*`
- Return type: `bool`
- Section of abi.h: §E items (SNBT value objects) & containers
- Position in the table: slot 46, counting from 0
- Callers in each binding:
    - Rust: [`Container::size`](../rust/container.md#Container.size), [`Container::items`](../rust/container.md#Container.items)
    - Go: [`Container.Size`](../go/item.md#Container.Size), [`Raw.ContainerSize`](../go/raw.md#Raw.ContainerSize)

### `container_get_item` {#container_get_item}

```c
bool (*container_get_item)(PierContainerRef ref, int32_t slot, void* ctx, PierStrSink sink);
```

Slot content as item SNBT (empty slots yield the air item's SNBT).

- Call: `api->container_get_item(ref, slot, ctx, sink)`
- Parameters:
    - ref : `PierContainerRef`
    - slot : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: §E items (SNBT value objects) & containers
- Position in the table: slot 47, counting from 0
- Callers in each binding:
    - Rust: [`Container::item`](../rust/container.md#Container.item), [`Container::items`](../rust/container.md#Container.items)
    - Go: [`Container.Item`](../go/item.md#Container.Item), [`Raw.ContainerGetItem`](../go/raw.md#Raw.ContainerGetItem)

### `container_set_item` {#container_set_item}

```c
bool (*container_set_item)(PierContainerRef ref, int32_t slot, PierStr item_snbt);
```

- Call: `api->container_set_item(ref, slot, item_snbt)`
- Parameters:
    - ref : `PierContainerRef`
    - slot : `int32_t`
    - item_snbt : `PierStr`
- Return type: `bool`
- Section of abi.h: §E items (SNBT value objects) & containers
- Position in the table: slot 48, counting from 0
- Callers in each binding:
    - Rust: [`Container::set_item`](../rust/container.md#Container.set_item)
    - Go: [`Container.SetItem`](../go/item.md#Container.SetItem), [`Raw.ContainerSetItem`](../go/raw.md#Raw.ContainerSetItem)

### `container_add_item` {#container_add_item}

```c
bool (*container_add_item)(PierContainerRef ref, PierStr item_snbt);
```

- Call: `api->container_add_item(ref, item_snbt)`
- Parameters:
    - ref : `PierContainerRef`
    - item_snbt : `PierStr`
- Return type: `bool`
- Section of abi.h: §E items (SNBT value objects) & containers
- Position in the table: slot 49, counting from 0
- Callers in each binding:
    - Rust: [`Container::add_item`](../rust/container.md#Container.add_item)
    - Go: [`Container.AddItem`](../go/item.md#Container.AddItem), [`Raw.ContainerAddItem`](../go/raw.md#Raw.ContainerAddItem)

### `container_remove_item` {#container_remove_item}

```c
bool (*container_remove_item)(PierContainerRef ref, int32_t slot, int32_t count);
```

- Call: `api->container_remove_item(ref, slot, count)`
- Parameters:
    - ref : `PierContainerRef`
    - slot : `int32_t`
    - count : `int32_t`
- Return type: `bool`
- Section of abi.h: §E items (SNBT value objects) & containers
- Position in the table: slot 50, counting from 0
- Callers in each binding:
    - Rust: [`Container::remove_item`](../rust/container.md#Container.remove_item)
    - Go: [`Container.RemoveItem`](../go/item.md#Container.RemoveItem), [`Raw.ContainerRemoveItem`](../go/raw.md#Raw.ContainerRemoveItem)

### `container_clear` {#container_clear}

```c
bool (*container_clear)(PierContainerRef ref);
```

- Call: `api->container_clear(ref)`
- Parameters:
    - ref : `PierContainerRef`
- Return type: `bool`
- Section of abi.h: §E items (SNBT value objects) & containers
- Position in the table: slot 51, counting from 0
- Callers in each binding:
    - Rust: [`Container::clear`](../rust/container.md#Container.clear)
    - Go: [`Container.Clear`](../go/item.md#Container.Clear), [`Raw.ContainerClear`](../go/raw.md#Raw.ContainerClear)

### `item_get_enchants` {#item_get_enchants}

```c
bool (*item_get_enchants)(PierStr item_snbt, void* ctx, PierStrSink sink);
```

SNBT \[{id, level},…\]

- Call: `api->item_get_enchants(item_snbt, ctx, sink)`
- Parameters:
    - item_snbt : `PierStr`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: Item: enchants, matching, NBT (dedicated fns)
- Position in the table: slot 125, counting from 0
- Callers in each binding:
    - Rust: [`ItemStack::enchants`](../rust/item.md#ItemStack.enchants)
    - Go: [`Item.Enchants`](../go/item.md#Item.Enchants), [`Raw.ItemGetEnchants`](../go/raw.md#Raw.ItemGetEnchants)

### `item_set_enchants` {#item_set_enchants}

```c
bool (*item_set_enchants)(PierStr item_snbt, PierStr enchants_snbt, void* ctx, PierStrSink out);
```

`enchants_snbt` = \[{id, level},…\]; out = new item SNBT. NULL on every current host: writing enchantments is not implemented, and `item_get_enchants` is the read side.

- Call: `api->item_set_enchants(item_snbt, enchants_snbt, ctx, out)`
- Parameters:
    - item_snbt : `PierStr`
    - enchants_snbt : `PierStr`
    - ctx : `void*`
    - out : `PierStrSink`
- Return type: `bool`
- Section of abi.h: Item: enchants, matching, NBT (dedicated fns)
- Position in the table: slot 126, counting from 0
- Callers in each binding:
    - Rust: [`ItemStack::with_enchants`](../rust/item.md#ItemStack.with_enchants)
    - Go: [`Raw.ItemSetEnchants`](../go/raw.md#Raw.ItemSetEnchants)

### `item_matches` {#item_matches}

```c
bool (*item_matches)(PierStr a, PierStr b);
```

- Call: `api->item_matches(a, b)`
- Parameters:
    - a : `PierStr`
    - b : `PierStr`
- Return type: `bool`
- Section of abi.h: Item: enchants, matching, NBT (dedicated fns)
- Position in the table: slot 127, counting from 0
- Callers in each binding:
    - Rust: [`ItemStack::matches`](../rust/item.md#ItemStack.matches)
    - Go: [`Item.Matches`](../go/item.md#Item.Matches), [`Raw.ItemMatches`](../go/raw.md#Raw.ItemMatches)

### `item_get_user_data` {#item_get_user_data}

```c
bool (*item_get_user_data)(PierStr item_snbt, void* ctx, PierStrSink sink);
```

- Call: `api->item_get_user_data(item_snbt, ctx, sink)`
- Parameters:
    - item_snbt : `PierStr`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: Item: enchants, matching, NBT (dedicated fns)
- Position in the table: slot 128, counting from 0
- Callers in each binding:
    - Rust: [`ItemStack::user_data`](../rust/item.md#ItemStack.user_data)
    - Go: [`Raw.ItemGetUserData`](../go/raw.md#Raw.ItemGetUserData)

### `container_refresh` {#container_refresh}

```c
bool (*container_refresh)(PierContainerRef ref);
```

Resend a player-owned container (which 0..3) to its owner. Returns false for block containers (which == 4) — a chest has no single owner to resend to; its viewers are refreshed by the engine's own container transaction path.

- Call: `api->container_refresh(ref)`
- Parameters:
    - ref : `PierContainerRef`
- Return type: `bool`
- Section of abi.h: Client-side container resync
- Position in the table: slot 155, counting from 0
- Callers in each binding:
    - Rust: [`Container::refresh`](../rust/container.md#Container.refresh)
    - Go: [`Raw.ContainerRefresh`](../go/raw.md#Raw.ContainerRefresh)

### `container_get_items` {#container_get_items}

```c
bool (*container_get_items)(PierContainerRef ref, void* ctx, PierSlotSink sink);
```

Every slot of a container in one call: the sink receives (slot, item SNBT) for each slot in order, empty slots included as an empty-item snapshot. One container resolution and one FFI crossing replace one of each per slot. Returns false if the container cannot be resolved. Server thread only.

- Call: `api->container_get_items(ref, ctx, sink)`
- Parameters:
    - ref : `PierContainerRef`
    - ctx : `void*`
    - sink : `PierSlotSink`
- Return type: `bool`
- Section of abi.h: Appended: bulk block reads and writes
- Position in the table: slot 192, counting from 0
- Callers in each binding:
    - Rust: [`Container::items`](../rust/container.md#Container.items)
    - Go: [`ContainerItems`](../go/item.md#ContainerItems), [`Container.Items`](../go/item.md#Container.Items)

## `PierItemNumProp` {#PierItemNumProp}

`item_get_num` keys (query a transient ItemStack rebuilt from SNBT).

| Name | Value | Description |
|---|---|---|
| <span id="PIER_IPROP_COUNT"></span>`PIER_IPROP_COUNT` | `0` | `ItemStackBase::mCount` |
| <span id="PIER_IPROP_MAX_STACK_SIZE"></span>`PIER_IPROP_MAX_STACK_SIZE` | `1` | `ItemStackBase::getMaxStackSize` |
| <span id="PIER_IPROP_AUX_VALUE"></span>`PIER_IPROP_AUX_VALUE` | `2` | `ItemStackBase::getAuxValue` |
| <span id="PIER_IPROP_ID"></span>`PIER_IPROP_ID` | `3` | `ItemStackBase::getId` |
| <span id="PIER_IPROP_DAMAGE"></span>`PIER_IPROP_DAMAGE` | `4` | `ItemStackBase::getDamageValue` |
| <span id="PIER_IPROP_IS_NULL"></span>`PIER_IPROP_IS_NULL` | `5` | `ItemStackBase::isNull` |
| <span id="PIER_IPROP_IS_BLOCK"></span>`PIER_IPROP_IS_BLOCK` | `6` | `ItemStackBase::isBlock` |
| <span id="PIER_IPROP_IS_ENCHANTED"></span>`PIER_IPROP_IS_ENCHANTED` | `7` | `ItemStackBase::isEnchanted` |
| <span id="PIER_IPROP_IS_ARMOR"></span>`PIER_IPROP_IS_ARMOR` | `8` | `ItemStackBase::isArmorItem` |
| <span id="PIER_IPROP_IS_DAMAGEABLE"></span>`PIER_IPROP_IS_DAMAGEABLE` | `9` | `ItemStackBase::isDamageableItem` |
| <span id="PIER_IPROP_IS_DAMAGED"></span>`PIER_IPROP_IS_DAMAGED` | `10` | `ItemStackBase::isDamaged` |
| <span id="PIER_IPROP_MAX_DAMAGE"></span>`PIER_IPROP_MAX_DAMAGE` | `11` | `ItemStackBase::getMaxDamage` |
| <span id="PIER_IPROP_IS_UNBREAKABLE"></span>`PIER_IPROP_IS_UNBREAKABLE` | `12` | `ItemStackBase::isUnbreakable` |
| <span id="PIER_IPROP_HAS_DURABILITY"></span>`PIER_IPROP_HAS_DURABILITY` | `13` | `ItemStackBase::hasDurability` |
| <span id="PIER_IPROP_IS_POTION"></span>`PIER_IPROP_IS_POTION` | `14` | `ItemStackBase::isPotionItem` |
| <span id="PIER_IPROP_IS_THROWABLE"></span>`PIER_IPROP_IS_THROWABLE` | `15` | `ItemStackBase::isThrowable` |
| <span id="PIER_IPROP_IS_FIRE_RESISTANT"></span>`PIER_IPROP_IS_FIRE_RESISTANT` | `16` | `ItemStackBase::isFireResistant` |
| <span id="PIER_IPROP_ATTACK_DAMAGE"></span>`PIER_IPROP_ATTACK_DAMAGE` | `17` | `ItemStackBase::getAttackDamage` |
| <span id="PIER_IPROP_REPAIR_COST"></span>`PIER_IPROP_REPAIR_COST` | `18` | `ItemStackBase::getBaseRepairCost` |
| <span id="PIER_IPROP_ENCHANT_VALUE"></span>`PIER_IPROP_ENCHANT_VALUE` | `19` | `ItemStackBase::getEnchantValue` |
| <span id="PIER_IPROP_IS_STACKABLE"></span>`PIER_IPROP_IS_STACKABLE` | `20` | `ItemStackBase::isStackable` |
| <span id="PIER_IPROP_IS_MUSIC_DISC"></span>`PIER_IPROP_IS_MUSIC_DISC` | `21` | `ItemStackBase::isMusicDiscItem` |
| <span id="PIER_IPROP_IS_OFFHAND"></span>`PIER_IPROP_IS_OFFHAND` | `22` | `ItemStackBase::isOffhandItem` |
| <span id="PIER_IPROP_USE_DURATION"></span>`PIER_IPROP_USE_DURATION` | `23` | `ItemStackBase::getMaxUseDuration` |
| <span id="PIER_IPROP_IS_GLINT"></span>`PIER_IPROP_IS_GLINT` | `24` | `ItemStackBase::isGlint` |
| <span id="PIER_IPROP_IS_BUNDLE"></span>`PIER_IPROP_IS_BUNDLE` | `25` | `ItemStackBase::isBundle` |
| <span id="PIER_IPROP_HAS_USER_DATA"></span>`PIER_IPROP_HAS_USER_DATA` | `26` | `ItemStackBase::hasUserData` |
| <span id="PIER_IPROP_HAS_CUSTOM_NAME"></span>`PIER_IPROP_HAS_CUSTOM_NAME` | `27` | `ItemStackBase::hasCustomHoverName` |

## `PierItemStrProp` {#PierItemStrProp}

`item_get_str` keys.

| Name | Value | Description |
|---|---|---|
| <span id="PIER_ISTR_TYPE_NAME"></span>`PIER_ISTR_TYPE_NAME` | `0` | `ItemStackBase::getTypeName` ("minecraft:apple") |
| <span id="PIER_ISTR_NAME"></span>`PIER_ISTR_NAME` | `1` | `ItemStackBase::getName` (display) |
| <span id="PIER_ISTR_CUSTOM_NAME"></span>`PIER_ISTR_CUSTOM_NAME` | `2` | `ItemStackBase::getCustomName` |
| <span id="PIER_ISTR_RAW_NAME_ID"></span>`PIER_ISTR_RAW_NAME_ID` | `3` | `ItemStackBase::getRawNameId` |
| <span id="PIER_ISTR_LORE"></span>`PIER_ISTR_LORE` | `4` | SNBT list \["l1","l2"\] `ItemStackBase::getCustomLore` |
| <span id="PIER_ISTR_CAN_DESTROY"></span>`PIER_ISTR_CAN_DESTROY` | `5` | SNBT list \["minecraft:stone",…\] |
| <span id="PIER_ISTR_CAN_PLACE_ON"></span>`PIER_ISTR_CAN_PLACE_ON` | `6` | SNBT list |
| <span id="PIER_ISTR_USER_DATA"></span>`PIER_ISTR_USER_DATA` | `7` | full NBT user data as SNBT |
| <span id="PIER_ISTR_HOVER_NAME"></span>`PIER_ISTR_HOVER_NAME` | `8` | `ItemStackBase::getHoverName` |
| <span id="PIER_ISTR_EFFECT_NAME"></span>`PIER_ISTR_EFFECT_NAME` | `9` | `ItemStackBase::getEffectName` |
| <span id="PIER_ISTR_COLOR"></span>`PIER_ISTR_COLOR` | `10` | SNBT {r,g,b} `ItemStackBase::getColor` |

## `PierItemOp` {#PierItemOp}

`item_transform` ops: rebuild → mutate → serialize back (out = new SNBT).

| Name | Value | Description |
|---|---|---|
| <span id="PIER_IOP_SET_CUSTOM_NAME"></span>`PIER_IOP_SET_CUSTOM_NAME` | `0` | sarg=name `ItemStackBase::setCustomName` |
| <span id="PIER_IOP_SET_DAMAGE"></span>`PIER_IOP_SET_DAMAGE` | `1` | narg=damage `ItemStackBase::setDamageValue` |
| <span id="PIER_IOP_SET_COUNT"></span>`PIER_IOP_SET_COUNT` | `2` | narg=count `ItemStackBase::mCount` |
| <span id="PIER_IOP_SET_LORE"></span>`PIER_IOP_SET_LORE` | `3` | sarg=SNBT list \["l1","l2"\] `ItemStackBase::setCustomLore` |
| <span id="PIER_IOP_SET_UNBREAKABLE"></span>`PIER_IOP_SET_UNBREAKABLE` | `4` | narg=0/1 `ItemStackBase::setUnbreakable` |
| <span id="PIER_IOP_HURT_AND_BREAK"></span>`PIER_IOP_HURT_AND_BREAK` | `5` | narg=damage `ItemStackBase::hurtAndBreak` |
| <span id="PIER_IOP_SET_REPAIR_COST"></span>`PIER_IOP_SET_REPAIR_COST` | `6` | narg=cost `ItemStackBase::setRepairCost` |
| <span id="PIER_IOP_ADD_ENCHANT"></span>`PIER_IOP_ADD_ENCHANT` | `7` | sarg="name:level" saveEnchantsToUserData |
| <span id="PIER_IOP_REMOVE_ENCHANTS"></span>`PIER_IOP_REMOVE_ENCHANTS` | `8` | `ItemStackBase::removeEnchants` |
| <span id="PIER_IOP_CLEAR_LORE"></span>`PIER_IOP_CLEAR_LORE` | `9` | `ItemStackBase::clearCustomLore` |
| <span id="PIER_IOP_RESET_NAME"></span>`PIER_IOP_RESET_NAME` | `10` | `ItemStackBase::resetHoverName` |
| <span id="PIER_IOP_SET_CAN_DESTROY"></span>`PIER_IOP_SET_CAN_DESTROY` | `11` | sarg=SNBT list `ItemStackBase::setCanDestroy` |
| <span id="PIER_IOP_SET_CAN_PLACE_ON"></span>`PIER_IOP_SET_CAN_PLACE_ON` | `12` | sarg=SNBT list `ItemStackBase::setCanPlaceOn` |
