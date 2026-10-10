# levilamina::container

Containers: the four on a player, plus the one at a coordinate in the world.

On the ABI a container is an owner plus which one, a `PierContainerRef`, and not a
pointer. A `Container` value can therefore be kept indefinitely: it resolves again on
every call and still points at the right thing after a player leaves and rejoins.

**Call [`Container::refresh`] after writing**

`set_item`, `add_item` and `clear` go through `Container::setItem`, which changes only
the server copy and sends no packet. The client keeps rendering what it last received
until the player clicks a slot and it resynchronizes passively. One `refresh` after a
bulk change pushes the whole container across. It must not be called per slot in a
loop, since it pushes the whole container and doing so per slot is a packet storm.

## `Container` {#Container}

```rust
pub struct Container {
    // private fields
}
```

A reference to one container.

- Implements: `Debug`, `Clone`, `PartialEq`, `Eq`, `Display`

### `Container::of_player` {#Container.of_player}

```rust
pub fn of_player(owner: PlayerSel, kind: ContainerKind) -> Container
```

One container on a player. Rarely called directly; the `Player::inventory()` family is
the usual route.

- Parameters:
    - owner : `PlayerSel`
    - kind : `ContainerKind`
- Return type: `Container`

### `Container::block` {#Container.block}

```rust
pub fn block(dim: i32, x: i32, y: i32, z: i32) -> Container
```

A block container at a coordinate in the world.

- Parameters:
    - dim : `i32`
    - x : `i32`
    - y : `i32`
    - z : `i32`
- Return type: `Container`

### `Container::kind` {#Container.kind}

```rust
pub fn kind(&self) -> ContainerKind
```

- Return type: `ContainerKind`

### `Container::position` {#Container.position}

```rust
pub fn position(&self) -> Option<(i32, i32, i32, i32)>
```

The coordinate of a block container. A player container gives `None`, since it is not
on a cell.

- Return type: `Option<(i32, i32, i32, i32)>`

### `Container::size` {#Container.size}

```rust
pub fn size(&self) -> Result<i32>
```

The slot count.

Failing to resolve, because the player left or that cell holds no container, returns
`Err` and not 0: a 0 would make a caller's `for i in 0..size` run zero times in
silence (contract §5.2).

- Return type: `Result<i32>`
- Slots: [`container_size`](../cpp/item.md#container_size)

### `Container::item` {#Container.item}

```rust
pub fn item(&self, slot: i32) -> Result<ItemStack>
```

What is in one slot. An empty slot gives the SNBT of the air item and is not an error.

- Parameters:
    - slot : `i32`
- Return type: `Result<ItemStack>`
- Slots: [`container_get_item`](../cpp/item.md#container_get_item)

### `Container::items` {#Container.items}

```rust
pub fn items(&self) -> Result<Vec<ItemStack>>
```

Every slot.

One slot failing to read fails the whole thing rather than being skipped: a listing
missing a few items gives the wrong answer to what is in this chest.

- Return type: `Result<Vec<ItemStack>>`
- Slots: [`container_get_items`](../cpp/item.md#container_get_items), [`container_size`](../cpp/item.md#container_size), [`container_get_item`](../cpp/item.md#container_get_item)

### `Container::set_item` {#Container.set_item}

```rust
pub fn set_item(&self, slot: i32, item: &ItemStack) -> Result<()>
```

Writes one slot. Remember [`Container::refresh`](container.md#Container.refresh) afterwards.

- Parameters:
    - slot : `i32`
    - item : `&ItemStack`
- Return type: `Result<()>`
- Slots: [`container_set_item`](../cpp/item.md#container_set_item)

### `Container::add_item` {#Container.add_item}

```rust
pub fn add_item(&self, item: &ItemStack) -> Result<()>
```

Puts one in and lets the engine pick the slot.

A full container is an `Err` and not a silent discard, whose symptom is a player's
items vanishing.

- Parameters:
    - item : `&ItemStack`
- Return type: `Result<()>`
- Slots: [`container_add_item`](../cpp/item.md#container_add_item)

### `Container::remove_item` {#Container.remove_item}

```rust
pub fn remove_item(&self, slot: i32, count: i32) -> Result<()>
```

Takes `count` items out of one slot.

- Parameters:
    - slot : `i32`
    - count : `i32`
- Return type: `Result<()>`
- Slots: [`container_remove_item`](../cpp/item.md#container_remove_item)

### `Container::clear` {#Container.clear}

```rust
pub fn clear(&self) -> Result<()>
```

- Return type: `Result<()>`
- Slots: [`container_clear`](../cpp/item.md#container_clear)

### `Container::refresh` {#Container.refresh}

```rust
pub fn refresh(&self) -> Result<()>
```

Resends the whole container to its owner.

A block container returns `Err`: a chest has no single owner to send to and its
viewers are refreshed by the engine's own container transaction path. That is a host
rule and not a choice of this layer.

- Return type: `Result<()>`
- Slots: [`container_refresh`](../cpp/item.md#container_refresh)

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

The kind of a container. The values align with `PierContainerRef::which`.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

### `ContainerKind::as_i32` {#ContainerKind.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- Return type: `i32`

### `ContainerKind::is_player_owned` {#ContainerKind.is_player_owned}

```rust
pub fn is_player_owned(self) -> bool
```

A block container has no single owner and therefore cannot be resynchronized; see
[`Container::refresh`](container.md#Container.refresh).

- Return type: `bool`
