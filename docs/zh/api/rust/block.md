# levilamina::block · 方块

方块：用维度加坐标指定的一格。

**有两条写入路径，默认用原生的那条**

[`Block::set`](block.md#Block.set) 走 `set_block`，也就是 `BlockSource::setBlock`，接收方块名或完整的 SNBT。[`Block::set_states`](block.md#Block.set_states) 和 [`Block::set_nbt`](block.md#Block.set_nbt) 走 `edit_*`，多一个 [`BlockUpdate`](types.md#BlockUpdate)，让调用方决定要不要通知相邻方块、要不要同步给客户端。批量填充时两者都关掉，速度能快一个数量级，代价是之后要重新同步。

**含水方块要读写液体层**

基岩版的含水用的是同一格里的第二个方块：主层是楼梯，液体层是水。[`Block::name`](block.md#Block.name) 只看得到主层，所以复制、粘贴含水楼梯会把水全部丢掉：主层完全正确，水没了。要把水一起搬过去，就要读写 [`Block::extra`](block.md#Block.extra)。

## `Block` {#Block}

```rust
pub struct Block {
    // private fields
}
```

世界里的一格。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`、`Hash`、`Display`

### `Block::at` {#Block.at}

```rust
pub fn at(dim: i32, x: i32, y: i32, z: i32) -> Block
```

- 参数：
    - dim : `i32`
    - x : `i32`
    - y : `i32`
    - z : `i32`
- 返回值类型：`Block`

### `Block::at_pos` {#Block.at_pos}

```rust
pub fn at_pos(dim: i32, pos: PositionI32) -> Block
```

- 参数：
    - dim : `i32`
    - pos : `PositionI32`
- 返回值类型：`Block`

### `Block::dimension` {#Block.dimension}

```rust
pub fn dimension(&self) -> i32
```

- 返回值类型：`i32`

### `Block::position` {#Block.position}

```rust
pub fn position(&self) -> PositionI32
```

- 返回值类型：`PositionI32`

### `Block::read` {#Block.read}

```rust
pub fn read(&self) -> Result<BlockInfo>
```

类型名和完整的 SNBT，一次调用拿到两者。

- 返回值类型：`Result<BlockInfo>`
- 对应槽位：[`get_block`](../cpp/world.md#get_block)

### `Block::to_nbt` {#Block.to_nbt}

```rust
pub fn to_nbt(&self) -> Result<NbtValue>
```

把完整的序列化解析成 NBT 树。要原样写回，用 [`Block::set_nbt`](block.md#Block.set_nbt)，那条路径不经过这一层的解析器。

- 返回值类型：`Result<NbtValue>`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `Block::num` {#Block.num}

```rust
pub fn num(&self, prop: i32) -> Result<f64>
```

读取一个 `PIER_BPROP_*` 数值属性。

- 参数：
    - prop : `i32`
- 返回值类型：`Result<f64>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::text` {#Block.text}

```rust
pub fn text(&self, prop: i32) -> Result<String>
```

读取一个 `PIER_BSTR_*` 字符串属性。

- 参数：
    - prop : `i32`
- 返回值类型：`Result<String>`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `Block::tags` {#Block.tags}

```rust
pub fn tags(&self) -> Result<Vec<String>>
```

方块的标签。

- 返回值类型：`Result<Vec<String>>`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `Block::act` {#Block.act}

```rust
pub fn act(&self, action: i32, sarg: &str) -> Result<String>
```

执行一个 `PIER_BACT_*` 动作。

- 参数：
    - action : `i32`
    - sarg : `&str`
- 返回值类型：`Result<String>`
- 对应槽位：[`block_action`](../cpp/block.md#block_action)

### `Block::has_tag` {#Block.has_tag}

```rust
pub fn has_tag(&self, tag: &str) -> Result<bool>
```

- 参数：
    - tag : `&str`
- 返回值类型：`Result<bool>`
- 对应槽位：[`block_action`](../cpp/block.md#block_action)

### `Block::as_item` {#Block.as_item}

```rust
pub fn as_item(&self) -> Result<crate::item::ItemStack>
```

把这一格当作物品，经 `Block::asItemInstance` 转换。

- 返回值类型：`Result<crate::item::ItemStack>`
- 对应槽位：[`block_action`](../cpp/block.md#block_action)

### `Block::pop_resource` {#Block.pop_resource}

```rust
pub fn pop_resource(&self, item: &crate::item::ItemStack) -> Result<()>
```

在这一格掉落一个物品。

- 参数：
    - item : `&crate::item::ItemStack`
- 返回值类型：`Result<()>`
- 对应槽位：[`block_action`](../cpp/block.md#block_action)

### `Block::set` {#Block.set}

```rust
pub fn set(&self, spec: &str) -> Result<()>
```

放置一个方块。`spec` 是 `"minecraft:stone"` 这样的名字，或者完整的 SNBT。

名字认不出来时调用失败，不会放一个占位方块；放了占位方块的症状，是世界里出现一块来历不明的紫黑格子。

- 参数：
    - spec : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`set_block`](../cpp/world.md#set_block)

### `Block::set_nbt` {#Block.set_nbt}

```rust
pub fn set_nbt(&self, snbt: &str, update: BlockUpdate) -> Result<()>
```

用完整的 NBT 放置一个方块，更新标志由你自己决定。

- 参数：
    - snbt : `&str`
    - update : `BlockUpdate`
- 返回值类型：`Result<()>`
- 对应槽位：[`edit_set_block_nbt`](../cpp/edit.md#edit_set_block_nbt)

### `Block::set_states` {#Block.set_states}

```rust
pub fn set_states(self, name: &str, states: Option<&str>, update: BlockUpdate) -> Result<()>
```

用方块名加上部分状态放置一个方块。

`states` 为 `None` 表示所有状态都用默认值。宿主从默认状态里取版本号，调用方不能自己填：版本号错了，方块会按另一套状态含义落地。

- 参数：
    - name : `&str`
    - states : `Option<&str>`
    - update : `BlockUpdate`
- 返回值类型：`Result<()>`
- 对应槽位：[`edit_set_block_states`](../cpp/edit.md#edit_set_block_states)

### `Block::extra` {#Block.extra}

```rust
pub fn extra(&self) -> Result<String>
```

读取液体层。空的液体层读出来是 `"minecraft:air"`，不算错误。

- 返回值类型：`Result<String>`
- 对应槽位：[`get_extra_block`](../cpp/world.md#get_extra_block)

### `Block::set_extra` {#Block.set_extra}

```rust
pub fn set_extra(&self, spec: &str, update: BlockUpdate) -> Result<()>
```

写入液体层。写入 `"minecraft:air"` 就是清空。

- 参数：
    - spec : `&str`
    - update : `BlockUpdate`
- 返回值类型：`Result<()>`
- 对应槽位：[`set_extra_block`](../cpp/world.md#set_extra_block)

### `Block::container` {#Block.container}

```rust
pub fn container(&self) -> crate::container::Container
```

这一格的容器，比如箱子或漏斗。它不检查那里是不是真的有容器，因为检查要跨一次 ABI；返回的 [`crate::container::Container`](container.md#Container) 第一次使用时自然会报告。

- 返回值类型：`crate::container::Container`

### `Block::name` {#Block.name}

```rust
pub fn name(&self) -> Result<String>
```

- 返回值类型：`Result<String>`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `Block::snbt` {#Block.snbt}

```rust
pub fn snbt(&self) -> Result<String>
```

- 返回值类型：`Result<String>`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `Block::description_id` {#Block.description_id}

```rust
pub fn description_id(&self) -> Result<String>
```

本地化用的键，并非显示出来的文字。

- 返回值类型：`Result<String>`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `Block::display_name` {#Block.display_name}

```rust
pub fn display_name(&self) -> Result<String>
```

本地化以后的显示名。

- 返回值类型：`Result<String>`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `Block::debug_string` {#Block.debug_string}

```rust
pub fn debug_string(&self) -> Result<String>
```

引擎自己的调试字符串。格式随版本变化，所以不要拿它做任何判断。

- 返回值类型：`Result<String>`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `Block::is_air` {#Block.is_air}

```rust
pub fn is_air(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_solid` {#Block.is_solid}

```rust
pub fn is_solid(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_container` {#Block.is_container}

```rust
pub fn is_container(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_door` {#Block.is_door}

```rust
pub fn is_door(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_fence` {#Block.is_fence}

```rust
pub fn is_fence(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_rail` {#Block.is_rail}

```rust
pub fn is_rail(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_slab` {#Block.is_slab}

```rust
pub fn is_slab(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_stair` {#Block.is_stair}

```rust
pub fn is_stair(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_wall` {#Block.is_wall}

```rust
pub fn is_wall(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_crop` {#Block.is_crop}

```rust
pub fn is_crop(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_unbreakable` {#Block.is_unbreakable}

```rust
pub fn is_unbreakable(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_crafting_block` {#Block.is_crafting_block}

```rust
pub fn is_crafting_block(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_interactive_block` {#Block.is_interactive_block}

```rust
pub fn is_interactive_block(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::is_signal_source` {#Block.is_signal_source}

```rust
pub fn is_signal_source(&self) -> Result<bool>
```

自己能产生红石信号，像拉杆或按钮那样。

- 返回值类型：`Result<bool>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::requires_tool` {#Block.requires_tool}

```rust
pub fn requires_tool(&self) -> Result<bool>
```

不用正确的工具挖就什么都不掉。

- 返回值类型：`Result<bool>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::has_block_entity` {#Block.has_block_entity}

```rust
pub fn has_block_entity(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::data` {#Block.data}

```rust
pub fn data(&self) -> Result<i32>
```

旧版的数据值。较新的方块请用 [`Block::states`](block.md#Block.states)。

- 返回值类型：`Result<i32>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::variant` {#Block.variant}

```rust
pub fn variant(&self) -> Result<i32>
```

`variant` 数据值，含义随实体种类而不同。

- 返回值类型：`Result<i32>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::block_item_id` {#Block.block_item_id}

```rust
pub fn block_item_id(&self) -> Result<i32>
```

对应物品的数字 id。

- 返回值类型：`Result<i32>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::light` {#Block.light}

```rust
pub fn light(&self) -> Result<i32>
```

这一格的实际亮度，包括天空光。

- 返回值类型：`Result<i32>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::light_emission` {#Block.light_emission}

```rust
pub fn light_emission(&self) -> Result<i32>
```

这个方块自己发出多少光。

- 返回值类型：`Result<i32>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::destroy_speed` {#Block.destroy_speed}

```rust
pub fn destroy_speed(&self) -> Result<f64>
```

挖掘硬度；越大越慢。

- 返回值类型：`Result<f64>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::explosion_resistance` {#Block.explosion_resistance}

```rust
pub fn explosion_resistance(&self) -> Result<f64>
```

爆炸抗性。

- 返回值类型：`Result<f64>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::friction` {#Block.friction}

```rust
pub fn friction(&self) -> Result<f64>
```

摩擦系数；冰是摩擦很低的一种。

- 返回值类型：`Result<f64>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::bounciness` {#Block.bounciness}

```rust
pub fn bounciness(&self) -> Result<f64>
```

弹性；黏液块的弹性不为零。

- 返回值类型：`Result<f64>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::burn_odds` {#Block.burn_odds}

```rust
pub fn burn_odds(&self) -> Result<i32>
```

着火的概率权重。

- 返回值类型：`Result<i32>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::flame_odds` {#Block.flame_odds}

```rust
pub fn flame_odds(&self) -> Result<i32>
```

把火蔓延给相邻方块的概率权重。

- 返回值类型：`Result<i32>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::redstone_signal` {#Block.redstone_signal}

```rust
pub fn redstone_signal(&self) -> Result<i32>
```

这一格输出的红石信号强度，0 到 15。

- 返回值类型：`Result<i32>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::comparator_signal` {#Block.comparator_signal}

```rust
pub fn comparator_signal(&self) -> Result<i32>
```

比较器从这一格读到的强度；容器按装满的程度计算它。

- 返回值类型：`Result<i32>`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Block::state` {#Block.state}

```rust
pub fn state(&self, name: &str) -> Result<String>
```

读取一个方块状态的值。

- 参数：
    - name : `&str`
- 返回值类型：`Result<String>`
- 对应槽位：[`block_get_state`](../cpp/block.md#block_get_state)

### `Block::states` {#Block.states}

```rust
pub fn states(&self) -> Result<NbtValue>
```

所有方块状态。

- 返回值类型：`Result<NbtValue>`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `Block::set_state` {#Block.set_state}

```rust
pub fn set_state(&self, name: &str, value: &str) -> Result<()>
```

- 参数：
    - name : `&str`
    - value : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`block_set_state`](../cpp/block.md#block_set_state)

### `Block::collision_shape` {#Block.collision_shape}

```rust
pub fn collision_shape(&self) -> Result<Vec<Bounds>>
```

碰撞箱。

- 返回值类型：`Result<Vec<Bounds>>`
- 对应槽位：[`block_get_collision_shape`](../cpp/block.md#block_get_collision_shape)

### `Block::block_entity` {#Block.block_entity}

```rust
pub fn block_entity(&self) -> Result<Option<NbtValue>>
```

方块实体的 NBT。这一格没有方块实体时返回 `Ok(None)`。

- 返回值类型：`Result<Option<NbtValue>>`
- 对应槽位：[`block_entity_snbt`](../cpp/block.md#block_entity_snbt)

### `Block::set_block_entity` {#Block.set_block_entity}

```rust
pub fn set_block_entity(&self, snbt: &str) -> Result<()>
```

把方块实体的 NBT 写回去，经 `BlockActor::load`。这一格里必须已经是对应种类的方块。

- 参数：
    - snbt : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`edit_set_block_entity`](../cpp/edit.md#edit_set_block_entity)

## `BlockInfo` {#BlockInfo}

```rust
pub struct BlockInfo {
    pub pos: PositionI32,
    /// The type name, such as `"minecraft:redstone_wire"`.
    pub name: String,
    /// The full serialization, `{name, states, version}`.
    pub snbt: String,
}
```

一个方块格读回来的内容。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`、`Eq`

### `BlockInfo::is_air` {#BlockInfo.is_air}

```rust
pub fn is_air(&self) -> bool
```

- 返回值类型：`bool`
