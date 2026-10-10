# levilamina::item · 物品

物品：一个值对象，不是句柄。

在 ABI 上，物品自始至终是一段 SNBT 字符串。读一个属性，就是拿这段 SNBT 去问；改一个属性，就是通过 `item_transform` 把这段 SNBT 换成一段新的。这一层照搬这个形状，中间不藏任何指针。

**结果：`ItemStack` 和世界里的物品没有联系**

`container.item(0)` 返回的是一份快照。修改它不会影响容器里的那个，要写回去需要明确调用 `container.set_item(0, &stack)`。这是「句柄是身份，不是指针」的另一面：中间没有隐式的同步，也就不会出现「我改了却什么都没变」。

## `ItemStack` {#ItemStack}

```rust
pub struct ItemStack {
    // private fields
}
```

一个物品的 SNBT 快照。

快照本身带有的字段，即 `Name`、`Count`、`Damage` 和 `tag` 复合标签，从第一次使用时缓存的解析结果里在本地读取；只有需要物品注册表的属性，比如堆叠上限或攻击伤害，才会跨过 ABI，这时宿主每次调用都要解析 SNBT，并构造一个引擎的 ItemStack。

- 实现的 trait：`Clone`、`PartialEq`、`Eq`、`Hash`、`Debug`、`Display`、`From`

### `ItemStack::from_snbt` {#ItemStack.from_snbt}

```rust
pub fn from_snbt(snbt: impl Into<String>) -> ItemStack
```

直接使用一段 SNBT 字符串，不做校验：校验要跨一次 ABI，而这个构造函数在热路径上。形状不对时，会在第一次真正使用它的调用处报错。

- 参数：
    - snbt : `impl Into<String>`
- 返回值类型：`ItemStack`

### `ItemStack::create` {#ItemStack.create}

```rust
pub fn create(type_name: &str, count: u8) -> ItemStack
```

用类型名和数量构造一个物品。

它拼出最小的形状 `{Name:"...",Count:Nb}`，其余部分由引擎在 `ItemStack::fromTag` 里补全。名字不存在时，会在物品被使用的时候失败，不在这里失败。

- 参数：
    - type_name : `&str`
    - count : `u8`
- 返回值类型：`ItemStack`

### `ItemStack::empty` {#ItemStack.empty}

```rust
pub fn empty() -> ItemStack
```

空气。容器里的空格子读出来就是它。

- 返回值类型：`ItemStack`

### `ItemStack::snbt` {#ItemStack.snbt}

```rust
pub fn snbt(&self) -> &str
```

底层的 SNBT。

- 返回值类型：`&str`

### `ItemStack::to_nbt` {#ItemStack.to_nbt}

```rust
pub fn to_nbt(&self) -> Result<NbtValue>
```

解析成 NBT 树，用来读取 ABI 没有提供命名访问方式的字段。

- 返回值类型：`Result<NbtValue>`

### `ItemStack::num` {#ItemStack.num}

```rust
pub fn num(&self, prop: i32) -> Result<f64>
```

读取一个 `PIER_IPROP_*` 数值属性。

宿主不认识的属性编号返回 `Err`，不返回 0：「无法确定」和「答案是 0」必须分开（契约 §5.2）。

- 参数：
    - prop : `i32`
- 返回值类型：`Result<f64>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::count` {#ItemStack.count}

```rust
pub fn count(&self) -> Result<u8>
```

- 返回值类型：`Result<u8>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::max_stack_size` {#ItemStack.max_stack_size}

```rust
pub fn max_stack_size(&self) -> Result<u8>
```

- 返回值类型：`Result<u8>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::aux_value` {#ItemStack.aux_value}

```rust
pub fn aux_value(&self) -> Result<i32>
```

- 返回值类型：`Result<i32>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::id` {#ItemStack.id}

```rust
pub fn id(&self) -> Result<i32>
```

- 返回值类型：`Result<i32>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::damage` {#ItemStack.damage}

```rust
pub fn damage(&self) -> Result<i32>
```

- 返回值类型：`Result<i32>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::max_damage` {#ItemStack.max_damage}

```rust
pub fn max_damage(&self) -> Result<i32>
```

- 返回值类型：`Result<i32>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::attack_damage` {#ItemStack.attack_damage}

```rust
pub fn attack_damage(&self) -> Result<i32>
```

- 返回值类型：`Result<i32>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::repair_cost` {#ItemStack.repair_cost}

```rust
pub fn repair_cost(&self) -> Result<i32>
```

- 返回值类型：`Result<i32>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::enchant_value` {#ItemStack.enchant_value}

```rust
pub fn enchant_value(&self) -> Result<i32>
```

- 返回值类型：`Result<i32>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::use_duration` {#ItemStack.use_duration}

```rust
pub fn use_duration(&self) -> Result<i32>
```

- 返回值类型：`Result<i32>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_null` {#ItemStack.is_null}

```rust
pub fn is_null(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_block` {#ItemStack.is_block}

```rust
pub fn is_block(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_enchanted` {#ItemStack.is_enchanted}

```rust
pub fn is_enchanted(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_armor` {#ItemStack.is_armor}

```rust
pub fn is_armor(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_damageable` {#ItemStack.is_damageable}

```rust
pub fn is_damageable(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_damaged` {#ItemStack.is_damaged}

```rust
pub fn is_damaged(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_unbreakable` {#ItemStack.is_unbreakable}

```rust
pub fn is_unbreakable(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::has_durability` {#ItemStack.has_durability}

```rust
pub fn has_durability(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_potion` {#ItemStack.is_potion}

```rust
pub fn is_potion(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_throwable` {#ItemStack.is_throwable}

```rust
pub fn is_throwable(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_fire_resistant` {#ItemStack.is_fire_resistant}

```rust
pub fn is_fire_resistant(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_stackable` {#ItemStack.is_stackable}

```rust
pub fn is_stackable(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_music_disc` {#ItemStack.is_music_disc}

```rust
pub fn is_music_disc(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_offhand` {#ItemStack.is_offhand}

```rust
pub fn is_offhand(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_glint` {#ItemStack.is_glint}

```rust
pub fn is_glint(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::is_bundle` {#ItemStack.is_bundle}

```rust
pub fn is_bundle(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::has_user_data` {#ItemStack.has_user_data}

```rust
pub fn has_user_data(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::has_custom_name` {#ItemStack.has_custom_name}

```rust
pub fn has_custom_name(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `ItemStack::text` {#ItemStack.text}

```rust
pub fn text(&self, prop: i32) -> Result<String>
```

读取一个 `PIER_ISTR_*` 字符串属性。

- 参数：
    - prop : `i32`
- 返回值类型：`Result<String>`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `ItemStack::type_name` {#ItemStack.type_name}

```rust
pub fn type_name(&self) -> Result<String>
```

- 返回值类型：`Result<String>`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `ItemStack::name` {#ItemStack.name}

```rust
pub fn name(&self) -> Result<String>
```

- 返回值类型：`Result<String>`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `ItemStack::custom_name` {#ItemStack.custom_name}

```rust
pub fn custom_name(&self) -> Result<String>
```

- 返回值类型：`Result<String>`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `ItemStack::hover_name` {#ItemStack.hover_name}

```rust
pub fn hover_name(&self) -> Result<String>
```

- 返回值类型：`Result<String>`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `ItemStack::raw_name_id` {#ItemStack.raw_name_id}

```rust
pub fn raw_name_id(&self) -> Result<String>
```

- 返回值类型：`Result<String>`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `ItemStack::effect_name` {#ItemStack.effect_name}

```rust
pub fn effect_name(&self) -> Result<String>
```

- 返回值类型：`Result<String>`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `ItemStack::lore` {#ItemStack.lore}

```rust
pub fn lore(&self) -> Result<Vec<String>>
```

自定义的描述文字。宿主给的是 SNBT 字符串列表，这里解析成 `Vec<String>`。

- 返回值类型：`Result<Vec<String>>`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `ItemStack::can_destroy` {#ItemStack.can_destroy}

```rust
pub fn can_destroy(&self) -> Result<Vec<String>>
```

- 返回值类型：`Result<Vec<String>>`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `ItemStack::can_place_on` {#ItemStack.can_place_on}

```rust
pub fn can_place_on(&self) -> Result<Vec<String>>
```

- 返回值类型：`Result<Vec<String>>`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `ItemStack::user_data` {#ItemStack.user_data}

```rust
pub fn user_data(&self) -> Result<NbtValue>
```

物品的自定义 NBT，也就是 `tag` 段。它走专用的槽位，没有用 `PIER_ISTR_USER_DATA`：内容一样，专用槽位让宿主少做一次属性编号的分派。

- 返回值类型：`Result<NbtValue>`
- 对应槽位：[`item_get_user_data`](../cpp/item.md#item_get_user_data)

### `ItemStack::color` {#ItemStack.color}

```rust
pub fn color(&self) -> Result<(i32, i32, i32)>
```

颜色，形如 `{r,g,b}`。只有能染色的物品才有。

- 返回值类型：`Result<(i32, i32, i32)>`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `ItemStack::transform` {#ItemStack.transform}

```rust
pub fn transform(&mut self, op: i32, sarg: &str, narg: f64) -> Result<()>
```

执行一个 `PIER_IOP_*` 变换，用结果替换自己。

失败时保持不变：宿主没有给出新的 SNBT，也就没有东西可以写回；写进去半个结果，比什么都不做更难排查。

- 参数：
    - op : `i32`
    - sarg : `&str`
    - narg : `f64`
- 返回值类型：`Result<()>`
- 对应槽位：[`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::transformed` {#ItemStack.transformed}

```rust
pub fn transformed(&self, op: i32, sarg: &str, narg: f64) -> Result<ItemStack>
```

同上，但返回一个新物品，这个物品保持不变。

- 参数：
    - op : `i32`
    - sarg : `&str`
    - narg : `f64`
- 返回值类型：`Result<ItemStack>`

### `ItemStack::set_custom_name` {#ItemStack.set_custom_name}

```rust
pub fn set_custom_name(&mut self, name: &str) -> Result<()>
```

- 参数：
    - name : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::reset_name` {#ItemStack.reset_name}

```rust
pub fn reset_name(&mut self) -> Result<()>
```

- 返回值类型：`Result<()>`
- 对应槽位：[`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::set_damage` {#ItemStack.set_damage}

```rust
pub fn set_damage(&mut self, damage: i32) -> Result<()>
```

- 参数：
    - damage : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::set_count` {#ItemStack.set_count}

```rust
pub fn set_count(&mut self, count: u8) -> Result<()>
```

- 参数：
    - count : `u8`
- 返回值类型：`Result<()>`
- 对应槽位：[`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::set_unbreakable` {#ItemStack.set_unbreakable}

```rust
pub fn set_unbreakable(&mut self, on: bool) -> Result<()>
```

- 参数：
    - on : `bool`
- 返回值类型：`Result<()>`
- 对应槽位：[`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::hurt_and_break` {#ItemStack.hurt_and_break}

```rust
pub fn hurt_and_break(&mut self, damage: i32) -> Result<()>
```

- 参数：
    - damage : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::set_repair_cost` {#ItemStack.set_repair_cost}

```rust
pub fn set_repair_cost(&mut self, cost: i32) -> Result<()>
```

- 参数：
    - cost : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::clear_lore` {#ItemStack.clear_lore}

```rust
pub fn clear_lore(&mut self) -> Result<()>
```

- 返回值类型：`Result<()>`
- 对应槽位：[`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::remove_enchants` {#ItemStack.remove_enchants}

```rust
pub fn remove_enchants(&mut self) -> Result<()>
```

- 返回值类型：`Result<()>`
- 对应槽位：[`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::set_lore` {#ItemStack.set_lore}

```rust
pub fn set_lore(&mut self, lines: &[&str]) -> Result<()>
```

- 参数：
    - lines : `&[&str]`
- 返回值类型：`Result<()>`
- 对应槽位：[`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::set_can_destroy` {#ItemStack.set_can_destroy}

```rust
pub fn set_can_destroy(&mut self, blocks: &[&str]) -> Result<()>
```

- 参数：
    - blocks : `&[&str]`
- 返回值类型：`Result<()>`
- 对应槽位：[`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::set_can_place_on` {#ItemStack.set_can_place_on}

```rust
pub fn set_can_place_on(&mut self, blocks: &[&str]) -> Result<()>
```

- 参数：
    - blocks : `&[&str]`
- 返回值类型：`Result<()>`
- 对应槽位：[`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::add_enchant` {#ItemStack.add_enchant}

```rust
pub fn add_enchant(&mut self, id: &str, level: i32) -> Result<()>
```

添加一个附魔。等级为 0 时，在引擎看来就是移除它。

- 参数：
    - id : `&str`
    - level : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`item_transform`](../cpp/item.md#item_transform)

### `ItemStack::enchants` {#ItemStack.enchants}

```rust
pub fn enchants(&self) -> Result<Vec<Enchant>>
```

- 返回值类型：`Result<Vec<Enchant>>`
- 对应槽位：[`item_get_enchants`](../cpp/item.md#item_get_enchants)

### `ItemStack::with_enchants` {#ItemStack.with_enchants}

```rust
pub fn with_enchants(&self, enchants: &[Enchant]) -> Result<ItemStack>
```

替换整组附魔，返回一个新物品。

- 参数：
    - enchants : `&[Enchant]`
- 返回值类型：`Result<ItemStack>`
- 对应槽位：[`item_set_enchants`](../cpp/item.md#item_set_enchants)

### `ItemStack::matches` {#ItemStack.matches}

```rust
pub fn matches(&self, other: &ItemStack) -> Result<bool>
```

两个物品是不是同一种东西。

判断标准来自引擎的 `ItemStack::matches`，和字符串相等不同：数量、耐久这些字段不参与比较，而比较 SNBT 文本会把它们算进去。缺少槽位时返回 `Err`，不会退回到文本比较，否则判断标准会因宿主而异。

- 参数：
    - other : `&ItemStack`
- 返回值类型：`Result<bool>`
- 对应槽位：[`item_matches`](../cpp/item.md#item_matches)

## `Enchant` {#Enchant}

```rust
pub struct Enchant {
    /// The enchantment id. Whether the host reports a numeric id or a name depends on the
    /// BDS version, and it is carried through unchanged.
    pub id: String,
    pub level: i32,
}
```

一个附魔。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`、`Eq`
