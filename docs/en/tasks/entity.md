# 🐷 Entity API

Mobs, dropped items, arrows, minecarts: anything in the world that moves is an "entity". In Pier you find one by its unique id, read its properties and act on it.

### Getting an entity object

#### By unique id

Rust: `Entity::from_id(id)`  
Go: `levilamina.EntityByID(id)`  
Zig: `levilamina.Entity.of(id)`  

An entity's id usually comes from an event payload: the `attackerUid` and `uid` fields of damage and death events.

- Parameters:
    - id : integer  
      the entity's unique id
- Return value: an entity object
- Return type: `Entity`
    - Creating the object does not check that the entity exists; a call after it was removed returns an error. An id can be kept across ticks.
- Example:
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

#### Every entity of a dimension

Rust: `Entity::list(Some(dim))`  
Go: `levilamina.ListActors(dim)`  
Zig: `levilamina.slot("list_actors")`  

- Parameters:
    - dim : integer  
      the dimension id
- Return value: entries of entity id and type name
- Return type: Rust `Vec<ActorEntry>`, Go `([]ActorInfo, error)`
- Slot: `list_actors`
- Example:
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

#### Spawning a mob

Rust: `World::get().spawn_mob(dim, type, x, y, z)`  
Go: `levilamina.SpawnMob(dim, type, x, y, z)`  
Zig: `levilamina.slot("spawn_mob")`  

- Parameters:
    - dim : integer  
      the dimension id
    - type : string  
      the mob type, such as `minecraft:zombie`
    - x, y, z : number  
      the position
- Return value: the new entity
- Return type: Rust `Result<Entity>`, Go `(Entity, error)`
- Slot: `spawn_mob`
- Example:
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

### Entity object: properties

Health, type, whether it is on fire… In Go and Zig every `PIER_APROP_*` and `PIER_ASTR_*` of `abi.h` has a method of its name, such as `Health()`, `IsOnFire()`, `TypeName()` (Zig: `health()`, `isOnFire()`, `typeName(allocator)`); Rust uses `entity.num(PIER_APROP_…)` and `entity.text(PIER_ASTR_…)`. A read that fails returns an error. Slots: `actor_get_num`, `actor_get_str`.

### Entity object: functions

`zombie` stands for the entity object you hold.

#### Killing an entity

Rust: `zombie.kill()`  
Go: `zombie.Kill("", 0, 0, 0)`  
Zig: `zombie.kill(allocator, "", 0, 0, 0)`  

- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `(string, error)`
- Slot: `actor_action`
- Example:
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

#### Healing an entity

Rust: `zombie.heal(amount)`  
Go: `zombie.Heal("", amount, 0, 0)`  
Zig: `zombie.heal(allocator, "", amount, 0, 0)`  

- Parameters:
    - amount : number  
      how much to heal
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `(string, error)`
    - Go and Zig verbs take a string and three numbers; which ones a verb uses is in the comment of its `PIER_AACT_*` constant in `abi.h`.
- Slot: `actor_action`
- Example:
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

#### Hurting an entity

Rust: `zombie.hurt(amount)`  
Go: `zombie.Hurt("", amount, 0, 0)`  
Zig: `zombie.hurt(allocator, "", amount, 0, 0)`  

- Parameters:
    - amount : number  
      the damage
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `(string, error)`
- Slot: `actor_action`
- Example:
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

#### Teleporting an entity

Rust: `zombie.teleport_to(dim, x, y, z)`  
Go: `zombie.Teleport(dim, x, y, z)`  
Zig: `zombie.teleport(allocator, dim, x, y, z)`  

- Parameters:
    - dim : integer (text in Go and Zig)  
      the dimension
    - x, y, z : number  
      the position
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `(string, error)`
- Slot: `actor_action`
- Example:
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

#### An entity's owner

Rust: `wolf.owner()`  
Go: `wolf.Owner()`  
Zig: `levilamina.slot("actor_get_owner")`  

The target and vehicle work the same: Rust `target()`, `vehicle()`, Go `Target()`, `Vehicle()`.

- Return value: the owner's entity object
- Return type: Rust `Result<Option<Entity>>`, Go `(Entity, error)`
    - In Rust "none" is `Ok(None)`; in Go, no owner returns an error naming what was read.
- Slot: `actor_get_owner`
- Example:
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

#### An entity's full data

Rust: `zombie.snapshot()`  
Go: `zombie.Snapshot()`  
Zig: `levilamina.slot("actor_snapshot")`  

- Return value: all of the entity's NBT: equipment, effects, a custom name…
- Return type: Rust `Result<NbtValue>`, Go `(string, error)`
    - A snapshot holds the data of that moment, and does not follow later changes.
- Slot: `actor_snapshot`
- Example:
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
