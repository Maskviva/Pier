# 🐷 实体对象 API

生物、掉落物、箭、矿车……世界里会动的东西都是「实体」。在 Pier 里，用实体的唯一 id 来找到它、读它的属性、对它做事。

### 获取一个实体对象

#### 通过唯一 id 获取

Rust：`Entity::from_id(id)`  
Go：`levilamina.EntityByID(id)`  
Zig：`levilamina.Entity.of(id)`  

实体的 id 通常来自事件内容：受伤、死亡事件里的 `attackerUid`、`uid` 字段都是它。

- 参数：
    - id : 整数  
      实体的唯一 id
- 返回值：实体对象
- 返回值类型：`Entity`
    - 生成对象时不会检查实体还在不在，实体被移除以后再调用会返回错误。id 可以跨 tick 保存。
- 示例：
    - Rust

      ```rust title="Rust"
      let zombie = Entity::from_id(uid);
      ```

    - Go

      ```go title="Go"
      zombie := levilamina.EntityByID(levilamina.ActorID(uid))
      ```

    - Zig

      ```zig title="Zig"
      const zombie = levilamina.Entity.of(uid);
      ```

#### 列出一个维度里的所有实体

Rust：`Entity::list(Some(dim))`  
Go：`levilamina.ListActors(dim)`  
Zig：`levilamina.slot("list_actors")`  

- 参数：
    - dim : 整数  
      维度 id
- 返回值：每一项是实体 id 和类型名
- 返回值类型：Rust `Vec<ActorEntry>`，Go `([]ActorInfo, error)`
- 对应槽位：`list_actors`
- 示例：
    - Rust

      ```rust title="Rust"
      for a in Entity::list(Some(0)) {
          Logger::get().info(&format!("{} {}", a.id, a.type_name));
      }
      ```

    - Go

      ```go title="Go"
      actors, err := levilamina.ListActors(0)
      for _, a := range actors {
      	levilamina.Logger{}.Info(a.Type)
      }
      ```

#### 生成一只生物

Rust：`World::get().spawn_mob(dim, type, x, y, z)`  
Go：`levilamina.SpawnMob(dim, type, x, y, z)`  
Zig：`levilamina.slot("spawn_mob")`  

- 参数：
    - dim : 整数  
      维度 id
    - type : 字符串  
      生物类型，比如 `minecraft:zombie`
    - x, y, z : 小数  
      坐标
- 返回值：新生成的实体
- 返回值类型：Rust `Result<Entity>`，Go `(Entity, error)`
- 对应槽位：`spawn_mob`
- 示例：
    - Rust

      ```rust title="Rust"
      let zombie = World::get().spawn_mob(0, "minecraft:zombie", 100.5, 64.0, 100.5)?;
      ```

    - Go

      ```go title="Go"
      zombie, err := levilamina.SpawnMob(0, "minecraft:zombie", 100.5, 64, 100.5)
      ```

    - Zig

      ```zig title="Zig"
      const spawn = levilamina.slot("spawn_mob") orelse return error.NotProvided;
      var new_id: i64 = 0;
      if (!spawn(0, levilamina.str("minecraft:zombie"), 100.5, 64, 100.5, &new_id)) return error.Refused;
      ```

### 实体对象 - 属性

生命值、类型、是否着火……在 Go 和 Zig 里，`abi.h` 中的每个 `PIER_APROP_*`、`PIER_ASTR_*` 都生成了一个同名方法，比如 `Health()`、`IsOnFire()`、`TypeName()`（Zig 为 `health()`、`isOnFire()`、`typeName(allocator)`）；Rust 用 `entity.num(PIER_APROP_…)` 和 `entity.text(PIER_ASTR_…)`。读不出来时返回错误。对应槽位：`actor_get_num`、`actor_get_str`。

### 实体对象 - 函数

以下用 `zombie` 代表你手里的实体对象。

#### 杀死实体

Rust：`zombie.kill()`  
Go：`zombie.Kill("", 0, 0, 0)`  
Zig：`zombie.kill(allocator, "", 0, 0, 0)`  

- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `(string, error)`
- 对应槽位：`actor_action`
- 示例：
    - Rust

      ```rust title="Rust"
      zombie.kill()?;
      ```

    - Go

      ```go title="Go"
      _, err := zombie.Kill("", 0, 0, 0)
      ```

    - Zig

      ```zig title="Zig"
      allocator.free(try zombie.kill(allocator, "", 0, 0, 0));
      ```

#### 治疗实体

Rust：`zombie.heal(amount)`  
Go：`zombie.Heal("", amount, 0, 0)`  
Zig：`zombie.heal(allocator, "", amount, 0, 0)`  

- 参数：
    - amount : 小数  
      治疗量
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `(string, error)`
    - Go、Zig 的动作方法都是「一个字符串加三个数字」，每个动作用到哪几个，写在 `abi.h` 里 `PIER_AACT_*` 常量的注释中。
- 对应槽位：`actor_action`
- 示例：
    - Rust

      ```rust title="Rust"
      zombie.heal(5.0)?;
      ```

    - Go

      ```go title="Go"
      _, err := zombie.Heal("", 5, 0, 0)
      ```

    - Zig

      ```zig title="Zig"
      allocator.free(try zombie.heal(allocator, "", 5, 0, 0));
      ```

#### 对实体造成伤害

Rust：`zombie.hurt(amount)`  
Go：`zombie.Hurt("", amount, 0, 0)`  
Zig：`zombie.hurt(allocator, "", amount, 0, 0)`  

- 参数：
    - amount : 小数  
      伤害值
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `(string, error)`
- 对应槽位：`actor_action`
- 示例：
    - Rust

      ```rust title="Rust"
      zombie.hurt(3.0)?;
      ```

    - Go

      ```go title="Go"
      _, err := zombie.Hurt("", 3, 0, 0)
      ```

    - Zig

      ```zig title="Zig"
      allocator.free(try zombie.hurt(allocator, "", 3, 0, 0));
      ```

#### 传送实体

Rust：`zombie.teleport_to(dim, x, y, z)`  
Go：`zombie.Teleport(dim, x, y, z)`  
Zig：`zombie.teleport(allocator, dim, x, y, z)`  

- 参数：
    - dim : 整数（Go、Zig 为文本）  
      目标维度
    - x, y, z : 小数  
      目标坐标
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `(string, error)`
- 对应槽位：`actor_action`
- 示例：
    - Rust

      ```rust title="Rust"
      zombie.teleport_to(0, 100.0, 64.0, 100.0)?;
      ```

    - Go

      ```go title="Go"
      _, err := zombie.Teleport("0", 100, 64, 100)
      ```

    - Zig

      ```zig title="Zig"
      allocator.free(try zombie.teleport(allocator, "0", 100, 64, 100));
      ```

#### 获取实体的主人

Rust：`wolf.owner()`  
Go：`wolf.Owner()`  
Zig：`levilamina.slot("actor_get_owner")`  

目标、坐骑同理：Rust `target()`、`vehicle()`，Go `Target()`、`Vehicle()`。

- 返回值：主人的实体对象
- 返回值类型：Rust `Result<Option<Entity>>`，Go `(Entity, error)`
    - Rust 里「没有」是 `Ok(None)`；Go 里没有主人时返回错误，错误里写着读的是哪一项。
- 对应槽位：`actor_get_owner`
- 示例：
    - Rust

      ```rust title="Rust"
      if let Some(owner) = wolf.owner()? {
          // ...
      }
      ```

    - Go

      ```go title="Go"
      owner, err := wolf.Owner()
      ```

    - Zig

      ```zig title="Zig"
      const get_owner = levilamina.slot("actor_get_owner") orelse return error.NotProvided;
      var owner_id: i64 = 0;
      const has_owner = get_owner(wolf.actor_id, &owner_id);
      ```

#### 获取实体的完整数据

Rust：`zombie.snapshot()`  
Go：`zombie.Snapshot()`  
Zig：`levilamina.slot("actor_snapshot")`  

- 返回值：实体身上的全部 NBT：装备、效果、自定义名字……
- 返回值类型：Rust `Result<NbtValue>`，Go `(string, error)`
    - 快照是取那一刻的数据，实体之后再变化，快照里的内容不会跟着变。
- 对应槽位：`actor_snapshot`
- 示例：
    - Rust

      ```rust title="Rust"
      let nbt = zombie.snapshot()?;
      let name = nbt.opt_str("CustomName");
      ```

    - Go

      ```go title="Go"
      snbt, err := zombie.Snapshot()
      v, err := levilamina.ParseSNBT(snbt)
      ```
