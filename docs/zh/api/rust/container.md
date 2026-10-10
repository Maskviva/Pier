# levilamina::container · 容器

容器：玩家身上的四个，加上世界里某个坐标处的那一个。

在 ABI 上，容器是「所有者 + 哪一个」，即 `PierContainerRef`，不是指针。所以 `Container` 值可以一直保存：它每次调用都重新解析，玩家离开又回来以后，仍然指向正确的东西。

**写完以后调用 [`Container::refresh`](container.md#Container.refresh)**

`set_item`、`add_item` 和 `clear` 都经 `Container::setItem` 写入，它只改服务器上的副本，不发送任何数据包。客户端继续显示它最后收到的内容，直到玩家点一下某个格子，被动地重新同步。批量修改之后调用一次 `refresh`，就会把整个容器推过去。不能在循环里每改一格调用一次：它推送的是整个容器，每格一次会发出大量数据包。

## `Container` {#Container}

```rust
pub struct Container {
    // private fields
}
```

对一个容器的引用。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`、`Eq`、`Display`

### `Container::of_player` {#Container.of_player}

```rust
pub fn of_player(owner: PlayerSel, kind: ContainerKind) -> Container
```

玩家身上的某个容器。很少直接调用，通常走 `Player::inventory()` 这一族方法。

- 参数：
    - owner : `PlayerSel`
    - kind : `ContainerKind`
- 返回值类型：`Container`

### `Container::block` {#Container.block}

```rust
pub fn block(dim: i32, x: i32, y: i32, z: i32) -> Container
```

世界里某个坐标处的方块容器。

- 参数：
    - dim : `i32`
    - x : `i32`
    - y : `i32`
    - z : `i32`
- 返回值类型：`Container`

### `Container::kind` {#Container.kind}

```rust
pub fn kind(&self) -> ContainerKind
```

- 返回值类型：`ContainerKind`

### `Container::position` {#Container.position}

```rust
pub fn position(&self) -> Option<(i32, i32, i32, i32)>
```

方块容器的坐标。玩家的容器返回 `None`，因为它不在某个格子上。

- 返回值类型：`Option<(i32, i32, i32, i32)>`

### `Container::size` {#Container.size}

```rust
pub fn size(&self) -> Result<i32>
```

格子数量。

解析失败（玩家离开了，或者那一格没有容器）时返回 `Err`，不返回 0：返回 0 会让调用方的 `for i in 0..size` 悄无声息地一次都不执行（契约 §5.2）。

- 返回值类型：`Result<i32>`
- 对应槽位：[`container_size`](../cpp/item.md#container_size)

### `Container::item` {#Container.item}

```rust
pub fn item(&self, slot: i32) -> Result<ItemStack>
```

某一格里的东西。空格子给出空气物品的 SNBT，不算错误。

- 参数：
    - slot : `i32`
- 返回值类型：`Result<ItemStack>`
- 对应槽位：[`container_get_item`](../cpp/item.md#container_get_item)

### `Container::items` {#Container.items}

```rust
pub fn items(&self) -> Result<Vec<ItemStack>>
```

每一格。

有一格读不出来就整体失败，不会跳过：少了几个物品的清单，对「这个箱子里有什么」给出的是错误的答案。

- 返回值类型：`Result<Vec<ItemStack>>`
- 对应槽位：[`container_get_items`](../cpp/item.md#container_get_items)、[`container_size`](../cpp/item.md#container_size)、[`container_get_item`](../cpp/item.md#container_get_item)

### `Container::set_item` {#Container.set_item}

```rust
pub fn set_item(&self, slot: i32, item: &ItemStack) -> Result<()>
```

写入某一格。之后记得调用 [`Container::refresh`](container.md#Container.refresh)。

- 参数：
    - slot : `i32`
    - item : `&ItemStack`
- 返回值类型：`Result<()>`
- 对应槽位：[`container_set_item`](../cpp/item.md#container_set_item)

### `Container::add_item` {#Container.add_item}

```rust
pub fn add_item(&self, item: &ItemStack) -> Result<()>
```

放进去一个，由引擎挑选格子。

容器满了时返回 `Err`，不会悄悄丢弃；悄悄丢弃的症状是玩家的物品凭空消失。

- 参数：
    - item : `&ItemStack`
- 返回值类型：`Result<()>`
- 对应槽位：[`container_add_item`](../cpp/item.md#container_add_item)

### `Container::remove_item` {#Container.remove_item}

```rust
pub fn remove_item(&self, slot: i32, count: i32) -> Result<()>
```

从某一格取出 `count` 个物品。

- 参数：
    - slot : `i32`
    - count : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`container_remove_item`](../cpp/item.md#container_remove_item)

### `Container::clear` {#Container.clear}

```rust
pub fn clear(&self) -> Result<()>
```

- 返回值类型：`Result<()>`
- 对应槽位：[`container_clear`](../cpp/item.md#container_clear)

### `Container::refresh` {#Container.refresh}

```rust
pub fn refresh(&self) -> Result<()>
```

把整个容器重新发给它的主人。

方块容器返回 `Err`：箱子没有唯一的主人可以发送，正在看它的玩家由引擎自己的容器事务流程刷新。这是宿主的规则，这一层只是照着做。

- 返回值类型：`Result<()>`
- 对应槽位：[`container_refresh`](../cpp/item.md#container_refresh)

## `ContainerKind` {#ContainerKind}

```rust
pub enum ContainerKind {
        Inventory = 0,
        EnderChest = 1,
        Armor = 2,
        OffHand = 3,
        /// A container at a coordinate in the world: a chest, a hopper, a furnace.
        Block = 4,
}
```

容器的种类。取值和 `PierContainerRef::which` 对齐。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

### `ContainerKind::as_i32` {#ContainerKind.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- 返回值类型：`i32`

### `ContainerKind::is_player_owned` {#ContainerKind.is_player_owned}

```rust
pub fn is_player_owned(self) -> bool
```

方块容器没有唯一的主人，因此不能重新同步；见 [`Container::refresh`](container.md#Container.refresh)。

- 返回值类型：`bool`
