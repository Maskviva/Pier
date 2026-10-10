# 物品与容器

??? note "abi.h 里的分节说明"

    **§E 物品（SNBT 值对象）与容器（`§E items (SNBT value objects) & containers`）**

    **物品：附魔、匹配、NBT（专用函数）（`Item: enchants, matching, NBT (dedicated fns)`）**

    **让客户端重新同步容器（`Client-side container resync`）**

    `container_set_item`、`_clear`、`_add_item` 都经 `Container::setItem` 写入，它只修改服务器上的副本，不发送任何东西。客户端继续显示它最后收到的内容，所以批量重写（比如跨维度传送时换掉玩家的物品栏）看起来像什么都没发生，直到玩家点一下某个格子，强制重新同步。

    在一批写入之后调用一次这个槽位。要成批调用：它推送的是整个容器，在循环里每改一格调一次，只会发出大量数据包，没有任何好处。

    **追加：批量读写方块（`Appended: bulk block reads and writes`）**

## 槽位 {#slots}

### `item_get_num` {#item_get_num}

```c
bool (*item_get_num)(PierStr item_snbt, int32_t prop, double* out);
```

- 调用形式：`api->item_get_num(item_snbt, prop, out)`
- 参数：
    - item_snbt : `PierStr`
    - prop : `int32_t`
    - out : `double*`
- 返回值类型：`bool`
- 所在分节：§E 物品（SNBT 值对象）与容器（`§E items (SNBT value objects) & containers`）
- 表内序号：第 43 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`ItemStack::num`](../rust/item.md#ItemStack.num)、[`ItemStack::count`](../rust/item.md#ItemStack.count)、[`ItemStack::max_stack_size`](../rust/item.md#ItemStack.max_stack_size)、[`ItemStack::aux_value`](../rust/item.md#ItemStack.aux_value)、[`ItemStack::id`](../rust/item.md#ItemStack.id)、[`ItemStack::damage`](../rust/item.md#ItemStack.damage) 等，共 29 个
    - Go：[`Item.Count`](../go/item.md#Item.Count)、[`Item.MaxStackSize`](../go/item.md#Item.MaxStackSize)、[`Item.AuxValue`](../go/item.md#Item.AuxValue)、[`Item.Id`](../go/item.md#Item.Id)、[`Item.Damage`](../go/item.md#Item.Damage)、[`Item.IsNull`](../go/item.md#Item.IsNull) 等，共 29 个
    - Zig：[`Item.count`](../zig/item.md#Item.count)、[`Item.maxStackSize`](../zig/item.md#Item.maxStackSize)、[`Item.auxValue`](../zig/item.md#Item.auxValue)、[`Item.id`](../zig/item.md#Item.id)、[`Item.damage`](../zig/item.md#Item.damage)、[`Item.isNull`](../zig/item.md#Item.isNull) 等，共 28 个

### `item_get_str` {#item_get_str}

```c
bool (*item_get_str)(PierStr item_snbt, int32_t prop, void* ctx, PierStrSink sink);
```

- 调用形式：`api->item_get_str(item_snbt, prop, ctx, sink)`
- 参数：
    - item_snbt : `PierStr`
    - prop : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：§E 物品（SNBT 值对象）与容器（`§E items (SNBT value objects) & containers`）
- 表内序号：第 44 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`ItemStack::text`](../rust/item.md#ItemStack.text)、[`ItemStack::type_name`](../rust/item.md#ItemStack.type_name)、[`ItemStack::name`](../rust/item.md#ItemStack.name)、[`ItemStack::custom_name`](../rust/item.md#ItemStack.custom_name)、[`ItemStack::hover_name`](../rust/item.md#ItemStack.hover_name)、[`ItemStack::raw_name_id`](../rust/item.md#ItemStack.raw_name_id) 等，共 11 个
    - Go：[`Item.TypeName`](../go/item.md#Item.TypeName)、[`Item.Name`](../go/item.md#Item.Name)、[`Item.CustomName`](../go/item.md#Item.CustomName)、[`Item.RawNameId`](../go/item.md#Item.RawNameId)、[`Item.Lore`](../go/item.md#Item.Lore)、[`Item.CanDestroy`](../go/item.md#Item.CanDestroy) 等，共 12 个
    - Zig：[`Item.typeName`](../zig/item.md#Item.typeName)、[`Item.name`](../zig/item.md#Item.name)、[`Item.customName`](../zig/item.md#Item.customName)、[`Item.rawNameId`](../zig/item.md#Item.rawNameId)、[`Item.lore`](../zig/item.md#Item.lore)、[`Item.canDestroy`](../zig/item.md#Item.canDestroy) 等，共 11 个

### `item_transform` {#item_transform}

```c
bool (*item_transform)(PierStr item_snbt, int32_t op, PierStr sarg, double narg, void* ctx, PierStrSink out);
```

先重建物品，再修改，最后序列化；`out` 收到的是**新的**物品 SNBT。

- 调用形式：`api->item_transform(item_snbt, op, sarg, narg, ctx, out)`
- 参数：
    - item_snbt : `PierStr`
    - op : `int32_t`
    - sarg : `PierStr`
    - narg : `double`
    - ctx : `void*`
    - out : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：§E 物品（SNBT 值对象）与容器（`§E items (SNBT value objects) & containers`）
- 表内序号：第 45 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`ItemStack::transform`](../rust/item.md#ItemStack.transform)、[`ItemStack::set_custom_name`](../rust/item.md#ItemStack.set_custom_name)、[`ItemStack::reset_name`](../rust/item.md#ItemStack.reset_name)、[`ItemStack::set_damage`](../rust/item.md#ItemStack.set_damage)、[`ItemStack::set_count`](../rust/item.md#ItemStack.set_count)、[`ItemStack::set_unbreakable`](../rust/item.md#ItemStack.set_unbreakable) 等，共 14 个
    - Go：[`Item.Transform`](../go/item.md#Item.Transform)、[`Raw.ItemTransform`](../go/raw.md#Raw.ItemTransform)

### `container_size` {#container_size}

```c
bool (*container_size)(PierContainerRef ref, int32_t* out);
```

- 调用形式：`api->container_size(ref, out)`
- 参数：
    - ref : `PierContainerRef`
    - out : `int32_t*`
- 返回值类型：`bool`
- 所在分节：§E 物品（SNBT 值对象）与容器（`§E items (SNBT value objects) & containers`）
- 表内序号：第 46 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Container::size`](../rust/container.md#Container.size)、[`Container::items`](../rust/container.md#Container.items)
    - Go：[`Container.Size`](../go/item.md#Container.Size)、[`Raw.ContainerSize`](../go/raw.md#Raw.ContainerSize)

### `container_get_item` {#container_get_item}

```c
bool (*container_get_item)(PierContainerRef ref, int32_t slot, void* ctx, PierStrSink sink);
```

这一格的内容，以物品 SNBT 给出（空格子给出空气物品的 SNBT）。

- 调用形式：`api->container_get_item(ref, slot, ctx, sink)`
- 参数：
    - ref : `PierContainerRef`
    - slot : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：§E 物品（SNBT 值对象）与容器（`§E items (SNBT value objects) & containers`）
- 表内序号：第 47 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Container::item`](../rust/container.md#Container.item)、[`Container::items`](../rust/container.md#Container.items)
    - Go：[`Container.Item`](../go/item.md#Container.Item)、[`Raw.ContainerGetItem`](../go/raw.md#Raw.ContainerGetItem)

### `container_set_item` {#container_set_item}

```c
bool (*container_set_item)(PierContainerRef ref, int32_t slot, PierStr item_snbt);
```

- 调用形式：`api->container_set_item(ref, slot, item_snbt)`
- 参数：
    - ref : `PierContainerRef`
    - slot : `int32_t`
    - item_snbt : `PierStr`
- 返回值类型：`bool`
- 所在分节：§E 物品（SNBT 值对象）与容器（`§E items (SNBT value objects) & containers`）
- 表内序号：第 48 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Container::set_item`](../rust/container.md#Container.set_item)
    - Go：[`Container.SetItem`](../go/item.md#Container.SetItem)、[`Raw.ContainerSetItem`](../go/raw.md#Raw.ContainerSetItem)

### `container_add_item` {#container_add_item}

```c
bool (*container_add_item)(PierContainerRef ref, PierStr item_snbt);
```

- 调用形式：`api->container_add_item(ref, item_snbt)`
- 参数：
    - ref : `PierContainerRef`
    - item_snbt : `PierStr`
- 返回值类型：`bool`
- 所在分节：§E 物品（SNBT 值对象）与容器（`§E items (SNBT value objects) & containers`）
- 表内序号：第 49 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Container::add_item`](../rust/container.md#Container.add_item)
    - Go：[`Container.AddItem`](../go/item.md#Container.AddItem)、[`Raw.ContainerAddItem`](../go/raw.md#Raw.ContainerAddItem)

### `container_remove_item` {#container_remove_item}

```c
bool (*container_remove_item)(PierContainerRef ref, int32_t slot, int32_t count);
```

- 调用形式：`api->container_remove_item(ref, slot, count)`
- 参数：
    - ref : `PierContainerRef`
    - slot : `int32_t`
    - count : `int32_t`
- 返回值类型：`bool`
- 所在分节：§E 物品（SNBT 值对象）与容器（`§E items (SNBT value objects) & containers`）
- 表内序号：第 50 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Container::remove_item`](../rust/container.md#Container.remove_item)
    - Go：[`Container.RemoveItem`](../go/item.md#Container.RemoveItem)、[`Raw.ContainerRemoveItem`](../go/raw.md#Raw.ContainerRemoveItem)

### `container_clear` {#container_clear}

```c
bool (*container_clear)(PierContainerRef ref);
```

- 调用形式：`api->container_clear(ref)`
- 参数：
    - ref : `PierContainerRef`
- 返回值类型：`bool`
- 所在分节：§E 物品（SNBT 值对象）与容器（`§E items (SNBT value objects) & containers`）
- 表内序号：第 51 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Container::clear`](../rust/container.md#Container.clear)
    - Go：[`Container.Clear`](../go/item.md#Container.Clear)、[`Raw.ContainerClear`](../go/raw.md#Raw.ContainerClear)

### `item_get_enchants` {#item_get_enchants}

```c
bool (*item_get_enchants)(PierStr item_snbt, void* ctx, PierStrSink sink);
```

SNBT `[{id, level},…]`

- 调用形式：`api->item_get_enchants(item_snbt, ctx, sink)`
- 参数：
    - item_snbt : `PierStr`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：物品：附魔、匹配、NBT（专用函数）（`Item: enchants, matching, NBT (dedicated fns)`）
- 表内序号：第 125 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`ItemStack::enchants`](../rust/item.md#ItemStack.enchants)
    - Go：[`Item.Enchants`](../go/item.md#Item.Enchants)、[`Raw.ItemGetEnchants`](../go/raw.md#Raw.ItemGetEnchants)

### `item_set_enchants` {#item_set_enchants}

```c
bool (*item_set_enchants)(PierStr item_snbt, PierStr enchants_snbt, void* ctx, PierStrSink out);
```

`enchants_snbt` 形如 `[{id, level},…]`；`out` 收到新的物品 SNBT。当前所有宿主上这个槽位都是 NULL：写附魔还没有实现，读取一侧是 `item_get_enchants`。

- 调用形式：`api->item_set_enchants(item_snbt, enchants_snbt, ctx, out)`
- 参数：
    - item_snbt : `PierStr`
    - enchants_snbt : `PierStr`
    - ctx : `void*`
    - out : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：物品：附魔、匹配、NBT（专用函数）（`Item: enchants, matching, NBT (dedicated fns)`）
- 表内序号：第 126 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`ItemStack::with_enchants`](../rust/item.md#ItemStack.with_enchants)
    - Go：[`Raw.ItemSetEnchants`](../go/raw.md#Raw.ItemSetEnchants)

### `item_matches` {#item_matches}

```c
bool (*item_matches)(PierStr a, PierStr b);
```

- 调用形式：`api->item_matches(a, b)`
- 参数：
    - a : `PierStr`
    - b : `PierStr`
- 返回值类型：`bool`
- 所在分节：物品：附魔、匹配、NBT（专用函数）（`Item: enchants, matching, NBT (dedicated fns)`）
- 表内序号：第 127 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`ItemStack::matches`](../rust/item.md#ItemStack.matches)
    - Go：[`Item.Matches`](../go/item.md#Item.Matches)、[`Raw.ItemMatches`](../go/raw.md#Raw.ItemMatches)

### `item_get_user_data` {#item_get_user_data}

```c
bool (*item_get_user_data)(PierStr item_snbt, void* ctx, PierStrSink sink);
```

- 调用形式：`api->item_get_user_data(item_snbt, ctx, sink)`
- 参数：
    - item_snbt : `PierStr`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：物品：附魔、匹配、NBT（专用函数）（`Item: enchants, matching, NBT (dedicated fns)`）
- 表内序号：第 128 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`ItemStack::user_data`](../rust/item.md#ItemStack.user_data)
    - Go：[`Raw.ItemGetUserData`](../go/raw.md#Raw.ItemGetUserData)

### `container_refresh` {#container_refresh}

```c
bool (*container_refresh)(PierContainerRef ref);
```

把玩家自己的容器（`which` 为 0 到 3）重新发给它的主人。方块容器（`which == 4`）返回 false：箱子没有唯一的主人可以重发，正在看它的玩家由引擎自己的容器事务流程刷新。

- 调用形式：`api->container_refresh(ref)`
- 参数：
    - ref : `PierContainerRef`
- 返回值类型：`bool`
- 所在分节：让客户端重新同步容器（`Client-side container resync`）
- 表内序号：第 155 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Container::refresh`](../rust/container.md#Container.refresh)
    - Go：[`Raw.ContainerRefresh`](../go/raw.md#Raw.ContainerRefresh)

### `container_get_items` {#container_get_items}

```c
bool (*container_get_items)(PierContainerRef ref, void* ctx, PierSlotSink sink);
```

一次调用读出容器的每一格：输出回调按顺序对每一格收到 (槽位, 物品 SNBT)，空格子也在内，给的是空物品的快照。解析一次容器、跨一次 FFI，代替原来每一格各一次。容器解析不出来时返回 false。只能在服务器线程调用。

- 调用形式：`api->container_get_items(ref, ctx, sink)`
- 参数：
    - ref : `PierContainerRef`
    - ctx : `void*`
    - sink : `PierSlotSink`
- 返回值类型：`bool`
- 所在分节：追加：批量读写方块（`Appended: bulk block reads and writes`）
- 表内序号：第 192 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Container::items`](../rust/container.md#Container.items)
    - Go：[`ContainerItems`](../go/item.md#ContainerItems)、[`Container.Items`](../go/item.md#Container.Items)

## `PierItemNumProp` {#PierItemNumProp}

`item_get_num` 的键（查询的是从 SNBT 临时重建的 `ItemStack`）。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PIER_IPROP_COUNT"></span>`PIER_IPROP_COUNT` | `0` | 取自 `ItemStackBase::mCount` |
| <span id="PIER_IPROP_MAX_STACK_SIZE"></span>`PIER_IPROP_MAX_STACK_SIZE` | `1` | 取自 `ItemStackBase::getMaxStackSize` |
| <span id="PIER_IPROP_AUX_VALUE"></span>`PIER_IPROP_AUX_VALUE` | `2` | 取自 `ItemStackBase::getAuxValue` |
| <span id="PIER_IPROP_ID"></span>`PIER_IPROP_ID` | `3` | 取自 `ItemStackBase::getId` |
| <span id="PIER_IPROP_DAMAGE"></span>`PIER_IPROP_DAMAGE` | `4` | 取自 `ItemStackBase::getDamageValue` |
| <span id="PIER_IPROP_IS_NULL"></span>`PIER_IPROP_IS_NULL` | `5` | 取自 `ItemStackBase::isNull` |
| <span id="PIER_IPROP_IS_BLOCK"></span>`PIER_IPROP_IS_BLOCK` | `6` | 取自 `ItemStackBase::isBlock` |
| <span id="PIER_IPROP_IS_ENCHANTED"></span>`PIER_IPROP_IS_ENCHANTED` | `7` | 取自 `ItemStackBase::isEnchanted` |
| <span id="PIER_IPROP_IS_ARMOR"></span>`PIER_IPROP_IS_ARMOR` | `8` | 取自 `ItemStackBase::isArmorItem` |
| <span id="PIER_IPROP_IS_DAMAGEABLE"></span>`PIER_IPROP_IS_DAMAGEABLE` | `9` | 取自 `ItemStackBase::isDamageableItem` |
| <span id="PIER_IPROP_IS_DAMAGED"></span>`PIER_IPROP_IS_DAMAGED` | `10` | 取自 `ItemStackBase::isDamaged` |
| <span id="PIER_IPROP_MAX_DAMAGE"></span>`PIER_IPROP_MAX_DAMAGE` | `11` | 取自 `ItemStackBase::getMaxDamage` |
| <span id="PIER_IPROP_IS_UNBREAKABLE"></span>`PIER_IPROP_IS_UNBREAKABLE` | `12` | 取自 `ItemStackBase::isUnbreakable` |
| <span id="PIER_IPROP_HAS_DURABILITY"></span>`PIER_IPROP_HAS_DURABILITY` | `13` | 取自 `ItemStackBase::hasDurability` |
| <span id="PIER_IPROP_IS_POTION"></span>`PIER_IPROP_IS_POTION` | `14` | 取自 `ItemStackBase::isPotionItem` |
| <span id="PIER_IPROP_IS_THROWABLE"></span>`PIER_IPROP_IS_THROWABLE` | `15` | 取自 `ItemStackBase::isThrowable` |
| <span id="PIER_IPROP_IS_FIRE_RESISTANT"></span>`PIER_IPROP_IS_FIRE_RESISTANT` | `16` | 取自 `ItemStackBase::isFireResistant` |
| <span id="PIER_IPROP_ATTACK_DAMAGE"></span>`PIER_IPROP_ATTACK_DAMAGE` | `17` | 取自 `ItemStackBase::getAttackDamage` |
| <span id="PIER_IPROP_REPAIR_COST"></span>`PIER_IPROP_REPAIR_COST` | `18` | 取自 `ItemStackBase::getBaseRepairCost` |
| <span id="PIER_IPROP_ENCHANT_VALUE"></span>`PIER_IPROP_ENCHANT_VALUE` | `19` | 取自 `ItemStackBase::getEnchantValue` |
| <span id="PIER_IPROP_IS_STACKABLE"></span>`PIER_IPROP_IS_STACKABLE` | `20` | 取自 `ItemStackBase::isStackable` |
| <span id="PIER_IPROP_IS_MUSIC_DISC"></span>`PIER_IPROP_IS_MUSIC_DISC` | `21` | 取自 `ItemStackBase::isMusicDiscItem` |
| <span id="PIER_IPROP_IS_OFFHAND"></span>`PIER_IPROP_IS_OFFHAND` | `22` | 取自 `ItemStackBase::isOffhandItem` |
| <span id="PIER_IPROP_USE_DURATION"></span>`PIER_IPROP_USE_DURATION` | `23` | 取自 `ItemStackBase::getMaxUseDuration` |
| <span id="PIER_IPROP_IS_GLINT"></span>`PIER_IPROP_IS_GLINT` | `24` | 取自 `ItemStackBase::isGlint` |
| <span id="PIER_IPROP_IS_BUNDLE"></span>`PIER_IPROP_IS_BUNDLE` | `25` | 取自 `ItemStackBase::isBundle` |
| <span id="PIER_IPROP_HAS_USER_DATA"></span>`PIER_IPROP_HAS_USER_DATA` | `26` | 取自 `ItemStackBase::hasUserData` |
| <span id="PIER_IPROP_HAS_CUSTOM_NAME"></span>`PIER_IPROP_HAS_CUSTOM_NAME` | `27` | 取自 `ItemStackBase::hasCustomHoverName` |

## `PierItemStrProp` {#PierItemStrProp}

`item_get_str` 的键。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PIER_ISTR_TYPE_NAME"></span>`PIER_ISTR_TYPE_NAME` | `0` | 取自 `ItemStackBase::getTypeName`（形如 `"minecraft:apple"`） |
| <span id="PIER_ISTR_NAME"></span>`PIER_ISTR_NAME` | `1` | 取自 `ItemStackBase::getName`（显示名） |
| <span id="PIER_ISTR_CUSTOM_NAME"></span>`PIER_ISTR_CUSTOM_NAME` | `2` | 取自 `ItemStackBase::getCustomName` |
| <span id="PIER_ISTR_RAW_NAME_ID"></span>`PIER_ISTR_RAW_NAME_ID` | `3` | 取自 `ItemStackBase::getRawNameId` |
| <span id="PIER_ISTR_LORE"></span>`PIER_ISTR_LORE` | `4` | SNBT 列表 `["l1","l2"]`，取自 `ItemStackBase::getCustomLore` |
| <span id="PIER_ISTR_CAN_DESTROY"></span>`PIER_ISTR_CAN_DESTROY` | `5` | SNBT 列表 `["minecraft:stone",…]` |
| <span id="PIER_ISTR_CAN_PLACE_ON"></span>`PIER_ISTR_CAN_PLACE_ON` | `6` | SNBT 列表 |
| <span id="PIER_ISTR_USER_DATA"></span>`PIER_ISTR_USER_DATA` | `7` | 完整的 NBT 用户数据，以 SNBT 表示 |
| <span id="PIER_ISTR_HOVER_NAME"></span>`PIER_ISTR_HOVER_NAME` | `8` | 取自 `ItemStackBase::getHoverName` |
| <span id="PIER_ISTR_EFFECT_NAME"></span>`PIER_ISTR_EFFECT_NAME` | `9` | 取自 `ItemStackBase::getEffectName` |
| <span id="PIER_ISTR_COLOR"></span>`PIER_ISTR_COLOR` | `10` | SNBT `{r,g,b}`，取自 `ItemStackBase::getColor` |

## `PierItemOp` {#PierItemOp}

`item_transform` 的操作：重建，修改，再序列化回去（`out` 收到新的 SNBT）。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PIER_IOP_SET_CUSTOM_NAME"></span>`PIER_IOP_SET_CUSTOM_NAME` | `0` | `sarg` 为名字，调用 `ItemStackBase::setCustomName` |
| <span id="PIER_IOP_SET_DAMAGE"></span>`PIER_IOP_SET_DAMAGE` | `1` | `narg` 为损耗值，调用 `ItemStackBase::setDamageValue` |
| <span id="PIER_IOP_SET_COUNT"></span>`PIER_IOP_SET_COUNT` | `2` | `narg` 为数量，写入 `ItemStackBase::mCount` |
| <span id="PIER_IOP_SET_LORE"></span>`PIER_IOP_SET_LORE` | `3` | `sarg` 为 SNBT 列表 `["l1","l2"]`，调用 `ItemStackBase::setCustomLore` |
| <span id="PIER_IOP_SET_UNBREAKABLE"></span>`PIER_IOP_SET_UNBREAKABLE` | `4` | `narg` 为 0 或 1，调用 `ItemStackBase::setUnbreakable` |
| <span id="PIER_IOP_HURT_AND_BREAK"></span>`PIER_IOP_HURT_AND_BREAK` | `5` | `narg` 为伤害值，调用 `ItemStackBase::hurtAndBreak` |
| <span id="PIER_IOP_SET_REPAIR_COST"></span>`PIER_IOP_SET_REPAIR_COST` | `6` | `narg` 为修复花费，调用 `ItemStackBase::setRepairCost` |
| <span id="PIER_IOP_ADD_ENCHANT"></span>`PIER_IOP_ADD_ENCHANT` | `7` | `sarg` 为 `"name:level"`，经 `saveEnchantsToUserData` 写入 |
| <span id="PIER_IOP_REMOVE_ENCHANTS"></span>`PIER_IOP_REMOVE_ENCHANTS` | `8` | 调用 `ItemStackBase::removeEnchants` |
| <span id="PIER_IOP_CLEAR_LORE"></span>`PIER_IOP_CLEAR_LORE` | `9` | 调用 `ItemStackBase::clearCustomLore` |
| <span id="PIER_IOP_RESET_NAME"></span>`PIER_IOP_RESET_NAME` | `10` | 调用 `ItemStackBase::resetHoverName` |
| <span id="PIER_IOP_SET_CAN_DESTROY"></span>`PIER_IOP_SET_CAN_DESTROY` | `11` | `sarg` 为 SNBT 列表，调用 `ItemStackBase::setCanDestroy` |
| <span id="PIER_IOP_SET_CAN_PLACE_ON"></span>`PIER_IOP_SET_CAN_PLACE_ON` | `12` | `sarg` 为 SNBT 列表，调用 `ItemStackBase::setCanPlaceOn` |
