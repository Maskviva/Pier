# 🎒 物品与容器 API

在 Pier 里，一个物品就是一段描述它的 SNBT：类型、数量、附魔、名字都写在里面。箱子、漏斗、玩家背包这些「容器」，是一排编了号的格子，每格放一个物品。

### 物品

#### 创建一个物品

Rust：`ItemStack::create(type, count)`  
Go：`levilamina.ItemOf(snbt)`  
Zig：`levilamina.Item.of(snbt)`  

- 参数：
    - type, count : 字符串和整数（Rust）  
      物品类型和数量
    - snbt : 字符串（Go、Zig）  
      物品的 SNBT，格式 `{Name:"类型",Count:数量b}`
- 返回值：物品对象。这一步只是在你的模组里写下 SNBT，不经过宿主
- 返回值类型：Rust `ItemStack`，Go、Zig `Item`
- 示例：
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

#### 读取物品数量

Rust：`diamonds.count()`  
Go：`diamonds.Count()`  
Zig：`diamonds.count()`  

其他属性同理：Go、Zig 里 `abi.h` 中的每个 `PIER_IPROP_*`、`PIER_ISTR_*` 都有同名方法，Rust 另有 `max_stack_size()`、`damage()` 等。

- 返回值：物品数量
- 返回值类型：Rust `Result<u8>`，Go `(float64, error)`，Zig `levilamina.Error!f64`
- 对应槽位：`item_get_num`
- 示例：
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

!!! warning "物品是快照"

    从玩家背包里读出来的物品是读取那一刻的样子，之后玩家把它扔掉、用掉，你手里这份 SNBT 不会变。想改背包里的东西，请把新的物品写回去。

### 容器

拿到容器：Rust `Container::block(dim, x, y, z)` 或 `player.inventory()`，Go `levilamina.Block(dim, x, y, z).Container()` 或 `player.Inventory()`。以下用 `chest` 代表它。

#### 读取一个格子

Rust：`chest.item(slot)`  
Go：`chest.Item(slot)`  
Zig：`levilamina.slot("container_get_item")`  

- 参数：
    - slot : 整数  
      格子编号，从 0 开始
- 返回值：格子里的物品
- 返回值类型：Rust `Result<ItemStack>`，Go `(Item, error)`
- 对应槽位：`container_get_item`
- 示例：
    - Rust

      ```rust title="Rust"
      let first = chest.item(0)?;
      ```

    - Go

      ```go title="Go"
      first, err := chest.Item(0)
      ```

#### 写入一个格子

Rust：`chest.set_item(slot, &item)`  
Go：`chest.SetItem(slot, item)`  
Zig：`levilamina.slot("container_set_item")`  

- 参数：
    - slot : 整数  
      格子编号
    - item : 物品  
      放进去的物品
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `error`
- 对应槽位：`container_set_item`
- 示例：
    - Rust

      ```rust title="Rust"
      chest.set_item(0, &diamonds)?;
      ```

    - Go

      ```go title="Go"
      err := chest.SetItem(0, diamonds)
      ```

#### 添加一个物品

Rust：`chest.add_item(&item)`  
Go：`chest.AddItem(item)`  
Zig：`levilamina.slot("container_add_item")`  

- 参数：
    - item : 物品  
      放进第一个放得下的格子
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `error`，Zig `bool`
- 对应槽位：`container_add_item`
- 示例：
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

#### 列出所有有东西的格子

Rust：`chest.items()`  
Go：`chest.Items()`  
Zig：`levilamina.slot("container_get_items")`  

- 返回值：空格子不会出现在结果里
- 返回值类型：Rust `Result<Vec<ItemStack>>`，Go `([]SlotItem, error)`
- 对应槽位：`container_get_items`
- 示例：
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

#### 清空容器

Rust：`chest.clear()`  
Go：`chest.Clear()`  
Zig：`levilamina.slot("container_clear")`  

- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `error`
- 对应槽位：`container_clear`
- 示例：
    - Rust

      ```rust title="Rust"
      chest.clear()?;
      ```

    - Go

      ```go title="Go"
      err := chest.Clear()
      ```
