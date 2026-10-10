# 🎒 Items and containers API

In Pier an item is the SNBT describing it: type, count, enchantments and name are in it. A chest, a hopper or a player's inventory is a "container": a row of numbered slots, one item each.

### Items

#### Creating an item

Rust: `ItemStack::create(type, count)`  
Go: `levilamina.ItemOf(snbt)`  
Zig: `levilamina.Item.of(snbt)`  

- Parameters:
    - type, count : a string and an integer (Rust)  
      the item type and count
    - snbt : string (Go, Zig)  
      the item's SNBT, as `{Name:"type",Count:<n>b}`
- Return value: an item object; this writes the SNBT inside your mod, without the host
- Return type: Rust `ItemStack`, Go and Zig `Item`
- Example:
    - Rust

      ```rust title="Rust"
      let diamonds = ItemStack::create("minecraft:diamond", 3);
      ```

    - Go

      ```go title="Go"
      diamonds := levilamina.ItemOf(`{Name:"minecraft:diamond",Count:3b}`)
      ```

    - Zig

      ```zig title="Zig"
      const diamonds = levilamina.Item.of("{Name:\"minecraft:diamond\",Count:3b}");
      ```

#### Reading the count

Rust: `diamonds.count()`  
Go: `diamonds.Count()`  
Zig: `diamonds.count()`  

Other properties work the same: in Go and Zig every `PIER_IPROP_*` and `PIER_ISTR_*` of `abi.h` has a method of its name, and Rust has `max_stack_size()`, `damage()` and more.

- Return value: the count
- Return type: Rust `Result<u8>`, Go `(float64, error)`, Zig `levilamina.Error!f64`
- Slot: `item_get_num`
- Example:
    - Rust

      ```rust title="Rust"
      let n = diamonds.count()?;
      ```

    - Go

      ```go title="Go"
      n, err := diamonds.Count()
      ```

    - Zig

      ```zig title="Zig"
      const n = try diamonds.count();
      ```

!!! warning "An item is a snapshot"

    An item read from an inventory is as it was then; when the player drops or uses it, your SNBT stays the same. To change the inventory, write the new item back.

### Containers

Getting one: Rust `Container::block(dim, x, y, z)` or `player.inventory()`, Go `levilamina.Block(dim, x, y, z).Container()` or `player.Inventory()`. `chest` stands for it below.

#### Reading a slot

Rust: `chest.item(slot)`  
Go: `chest.Item(slot)`  
Zig: `levilamina.slot("container_get_item")`  

- Parameters:
    - slot : integer  
      the slot number, from 0
- Return value: the item in the slot
- Return type: Rust `Result<ItemStack>`, Go `(Item, error)`
- Slot: `container_get_item`
- Example:
    - Rust

      ```rust title="Rust"
      let first = chest.item(0)?;
      ```

    - Go

      ```go title="Go"
      first, err := chest.Item(0)
      ```

#### Writing a slot

Rust: `chest.set_item(slot, &item)`  
Go: `chest.SetItem(slot, item)`  
Zig: `levilamina.slot("container_set_item")`  

- Parameters:
    - slot : integer  
      the slot number
    - item : item  
      the item to put in
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `error`
- Slot: `container_set_item`
- Example:
    - Rust

      ```rust title="Rust"
      chest.set_item(0, &diamonds)?;
      ```

    - Go

      ```go title="Go"
      err := chest.SetItem(0, diamonds)
      ```

#### Adding an item

Rust: `chest.add_item(&item)`  
Go: `chest.AddItem(item)`  
Zig: `levilamina.slot("container_add_item")`  

- Parameters:
    - item : item  
      goes into the first slot that fits
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `error`, Zig `bool`
- Slot: `container_add_item`
- Example:
    - Rust

      ```rust title="Rust"
      chest.add_item(&diamonds)?;
      ```

    - Go

      ```go title="Go"
      err := chest.AddItem(diamonds)
      ```

    - Zig

      ```zig title="Zig"
      const add = levilamina.slot("container_add_item") orelse return error.NotProvided;
      if (!add(chest, levilamina.str(diamonds.item_snbt))) return error.Refused;
      ```

#### Every slot that holds something

Rust: `chest.items()`  
Go: `chest.Items()`  
Zig: `levilamina.slot("container_get_items")`  

- Return value: empty slots are left out
- Return type: Rust `Result<Vec<ItemStack>>`, Go `([]SlotItem, error)`
- Slot: `container_get_items`
- Example:
    - Rust

      ```rust title="Rust"
      for item in chest.items()? {
          // ...
      }
      ```

    - Go

      ```go title="Go"
      slots, err := chest.Items()
      ```

#### Clearing a container

Rust: `chest.clear()`  
Go: `chest.Clear()`  
Zig: `levilamina.slot("container_clear")`  

- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `error`
- Slot: `container_clear`
- Example:
    - Rust

      ```rust title="Rust"
      chest.clear()?;
      ```

    - Go

      ```go title="Go"
      err := chest.Clear()
      ```
