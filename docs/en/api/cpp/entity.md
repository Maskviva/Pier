# Actors

??? note "Section notes in abi.h"

    **§C actors (players resolve here too, via player\_resolve)**

    **Actor: relationships, equipment, effects, geometry (dedicated fns)**

## Slots {#slots}

### `list_actors` {#list_actors}

```c
void (*list_actors)(int32_t dim, void* ctx, PierActorSink sink);
```

Enumerate live actors; dim = -1 for all dimensions.

- Call: `api->list_actors(dim, ctx, sink)`
- Parameters:
    - dim : `int32_t`
    - ctx : `void*`
    - sink : `PierActorSink`
- Section of abi.h: §C actors (players resolve here too, via player\_resolve)
- Position in the table: slot 32, counting from 0
- Callers in each binding:
    - Rust: [`Entity::list`](../rust/entity.md#Entity.list), [`Entity::list`](../rust/entity.md#Entity.list)
    - Go: [`ListActors`](../go/entity.md#ListActors)

### `actor_snapshot` {#actor_snapshot}

```c
bool (*actor_snapshot)(PierActorId id, void* ctx, PierStrSink snbt_sink);
```

Full `Actor::save` NBT as SNBT.

- Call: `api->actor_snapshot(id, ctx, snbt_sink)`
- Parameters:
    - id : `PierActorId`
    - ctx : `void*`
    - snbt_sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: §C actors (players resolve here too, via player\_resolve)
- Position in the table: slot 33, counting from 0
- Callers in each binding:
    - Rust: [`Entity::snapshot`](../rust/entity.md#Entity.snapshot)
    - Go: [`Entity.Snapshot`](../go/entity.md#Entity.Snapshot), [`Raw.ActorSnapshot`](../go/raw.md#Raw.ActorSnapshot)

### `actor_get_num` {#actor_get_num}

```c
bool (*actor_get_num)(PierActorId id, int32_t prop, double* out);
```

- Call: `api->actor_get_num(id, prop, out)`
- Parameters:
    - id : `PierActorId`
    - prop : `int32_t`
    - out : `double*`
- Return type: `bool`
- Section of abi.h: §C actors (players resolve here too, via player\_resolve)
- Position in the table: slot 34, counting from 0
- Callers in each binding:
    - Rust: [`Entity::num`](../rust/entity.md#Entity.num), [`Entity::pos`](../rust/entity.md#Entity.pos), [`Entity::feet_pos`](../rust/entity.md#Entity.feet_pos), [`Entity::head_pos`](../rust/entity.md#Entity.head_pos), [`Entity::velocity`](../rust/entity.md#Entity.velocity), [`Entity::view_vector`](../rust/entity.md#Entity.view_vector) and more, 37 in all
    - Go: [`Entity.PosX`](../go/entity.md#Entity.PosX), [`Entity.PosY`](../go/entity.md#Entity.PosY), [`Entity.PosZ`](../go/entity.md#Entity.PosZ), [`Entity.RotPitch`](../go/entity.md#Entity.RotPitch), [`Entity.RotYaw`](../go/entity.md#Entity.RotYaw), [`Entity.Dimension`](../go/entity.md#Entity.Dimension) and more, 49 in all
    - Zig: [`Entity.posX`](../zig/entity.md#Entity.posX), [`Entity.posY`](../zig/entity.md#Entity.posY), [`Entity.posZ`](../zig/entity.md#Entity.posZ), [`Entity.rotPitch`](../zig/entity.md#Entity.rotPitch), [`Entity.rotYaw`](../zig/entity.md#Entity.rotYaw), [`Entity.dimension`](../zig/entity.md#Entity.dimension) and more, 48 in all

### `actor_get_str` {#actor_get_str}

```c
bool (*actor_get_str)(PierActorId id, int32_t prop, void* ctx, PierStrSink sink);
```

- Call: `api->actor_get_str(id, prop, ctx, sink)`
- Parameters:
    - id : `PierActorId`
    - prop : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: §C actors (players resolve here too, via player\_resolve)
- Position in the table: slot 35, counting from 0
- Callers in each binding:
    - Rust: [`Entity::exists`](../rust/entity.md#Entity.exists), [`Entity::text`](../rust/entity.md#Entity.text), [`Entity::type_name`](../rust/entity.md#Entity.type_name), [`Entity::name_tag`](../rust/entity.md#Entity.name_tag), [`Entity::score_tag`](../rust/entity.md#Entity.score_tag), [`Entity::filtered_name`](../rust/entity.md#Entity.filtered_name)
    - Go: [`Entity.TypeName`](../go/entity.md#Entity.TypeName), [`Entity.NameTag`](../go/entity.md#Entity.NameTag), [`Entity.ScoreTag`](../go/entity.md#Entity.ScoreTag), [`Entity.FilteredName`](../go/entity.md#Entity.FilteredName), [`Raw.ActorGetStr`](../go/raw.md#Raw.ActorGetStr)
    - Zig: [`Entity.typeName`](../zig/entity.md#Entity.typeName), [`Entity.nameTag`](../zig/entity.md#Entity.nameTag), [`Entity.scoreTag`](../zig/entity.md#Entity.scoreTag), [`Entity.filteredName`](../zig/entity.md#Entity.filteredName)

### `actor_action` {#actor_action}

```c
bool (*actor_action)(
    PierActorId id,
    int32_t action,
    PierStr sarg,
    double a,
    double b,
    double c,
    void* ctx,
    PierStrSink out
);
```

- Call: `api->actor_action(id, action, sarg, a, b, c, ctx, out)`
- Parameters:
    - id : `PierActorId`
    - action : `int32_t`
    - sarg : `PierStr`
    - a : `double`
    - b : `double`
    - c : `double`
    - ctx : `void*`
    - out : `PierStrSink`
- Return type: `bool`
- Section of abi.h: §C actors (players resolve here too, via player\_resolve)
- Position in the table: slot 36, counting from 0
- Callers in each binding:
    - Rust: [`Entity::act`](../rust/entity.md#Entity.act), [`Entity::kill`](../rust/entity.md#Entity.kill), [`Entity::despawn`](../rust/entity.md#Entity.despawn), [`Entity::clear_effects`](../rust/entity.md#Entity.clear_effects), [`Entity::stop_fire`](../rust/entity.md#Entity.stop_fire), [`Entity::remove_all_passengers`](../rust/entity.md#Entity.remove_all_passengers) and more, 35 in all
    - Go: [`Entity.Kill`](../go/entity.md#Entity.Kill), [`Entity.Despawn`](../go/entity.md#Entity.Despawn), [`Entity.Heal`](../go/entity.md#Entity.Heal), [`Entity.SetOnFire`](../go/entity.md#Entity.SetOnFire), [`Entity.Teleport`](../go/entity.md#Entity.Teleport), [`Entity.SetNameTag`](../go/entity.md#Entity.SetNameTag) and more, 34 in all
    - Zig: [`Entity.kill`](../zig/entity.md#Entity.kill), [`Entity.despawn`](../zig/entity.md#Entity.despawn), [`Entity.heal`](../zig/entity.md#Entity.heal), [`Entity.setOnFire`](../zig/entity.md#Entity.setOnFire), [`Entity.teleport`](../zig/entity.md#Entity.teleport), [`Entity.setNameTag`](../zig/entity.md#Entity.setNameTag) and more, 33 in all

### `spawn_mob` {#spawn_mob}

```c
bool (*spawn_mob)(int32_t dim, PierStr type_name, double x, double y, double z, PierActorId* out);
```

Spawn a mob (`Spawner::spawnMob`); on success \*out = its ActorUniqueID.

- Call: `api->spawn_mob(dim, type_name, x, y, z, out)`
- Parameters:
    - dim : `int32_t`
    - type_name : `PierStr`
    - x : `double`
    - y : `double`
    - z : `double`
    - out : `PierActorId*`
- Return type: `bool`
- Section of abi.h: §C actors (players resolve here too, via player\_resolve)
- Position in the table: slot 37, counting from 0
- Callers in each binding:
    - Rust: [`World::spawn_mob`](../rust/world.md#World.spawn_mob)
    - Go: [`SpawnMob`](../go/entity.md#SpawnMob), [`Raw.SpawnMob`](../go/raw.md#Raw.SpawnMob)

### `actor_get_vehicle` {#actor_get_vehicle}

```c
bool (*actor_get_vehicle)(PierActorId id, PierActorId* out);
```

- Call: `api->actor_get_vehicle(id, out)`
- Parameters:
    - id : `PierActorId`
    - out : `PierActorId*`
- Return type: `bool`
- Section of abi.h: Actor: relationships, equipment, effects, geometry (dedicated fns)
- Position in the table: slot 109, counting from 0
- Callers in each binding:
    - Rust: [`Entity::vehicle`](../rust/entity.md#Entity.vehicle)
    - Go: [`Entity.Vehicle`](../go/entity.md#Entity.Vehicle), [`Raw.ActorGetVehicle`](../go/raw.md#Raw.ActorGetVehicle)

### `actor_get_first_passenger` {#actor_get_first_passenger}

```c
bool (*actor_get_first_passenger)(PierActorId id, PierActorId* out);
```

- Call: `api->actor_get_first_passenger(id, out)`
- Parameters:
    - id : `PierActorId`
    - out : `PierActorId*`
- Return type: `bool`
- Section of abi.h: Actor: relationships, equipment, effects, geometry (dedicated fns)
- Position in the table: slot 110, counting from 0
- Callers in each binding:
    - Rust: [`Entity::first_passenger`](../rust/entity.md#Entity.first_passenger)
    - Go: [`Raw.ActorGetFirstPassenger`](../go/raw.md#Raw.ActorGetFirstPassenger)

### `actor_get_owner` {#actor_get_owner}

```c
bool (*actor_get_owner)(PierActorId id, PierActorId* out);
```

- Call: `api->actor_get_owner(id, out)`
- Parameters:
    - id : `PierActorId`
    - out : `PierActorId*`
- Return type: `bool`
- Section of abi.h: Actor: relationships, equipment, effects, geometry (dedicated fns)
- Position in the table: slot 111, counting from 0
- Callers in each binding:
    - Rust: [`Entity::owner`](../rust/entity.md#Entity.owner)
    - Go: [`Entity.Owner`](../go/entity.md#Entity.Owner), [`Raw.ActorGetOwner`](../go/raw.md#Raw.ActorGetOwner)

### `actor_get_target` {#actor_get_target}

```c
bool (*actor_get_target)(PierActorId id, PierActorId* out);
```

- Call: `api->actor_get_target(id, out)`
- Parameters:
    - id : `PierActorId`
    - out : `PierActorId*`
- Return type: `bool`
- Section of abi.h: Actor: relationships, equipment, effects, geometry (dedicated fns)
- Position in the table: slot 112, counting from 0
- Callers in each binding:
    - Rust: [`Entity::target`](../rust/entity.md#Entity.target)
    - Go: [`Entity.Target`](../go/entity.md#Entity.Target), [`Raw.ActorGetTarget`](../go/raw.md#Raw.ActorGetTarget)

### `actor_get_equipped_item` {#actor_get_equipped_item}

```c
bool (*actor_get_equipped_item)(PierActorId id, int32_t slot, void* ctx, PierStrSink sink);
```

slot: 0=mainhand 1=offhand 2=helmet 3=chestplate 4=leggings 5=boots

- Call: `api->actor_get_equipped_item(id, slot, ctx, sink)`
- Parameters:
    - id : `PierActorId`
    - slot : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: Actor: relationships, equipment, effects, geometry (dedicated fns)
- Position in the table: slot 113, counting from 0
- Callers in each binding:
    - Rust: [`Entity::equipped_item`](../rust/entity.md#Entity.equipped_item)
    - Go: [`Raw.ActorGetEquippedItem`](../go/raw.md#Raw.ActorGetEquippedItem)

### `actor_set_equipped_item` {#actor_set_equipped_item}

```c
bool (*actor_set_equipped_item)(PierActorId id, int32_t slot, PierStr item_snbt);
```

- Call: `api->actor_set_equipped_item(id, slot, item_snbt)`
- Parameters:
    - id : `PierActorId`
    - slot : `int32_t`
    - item_snbt : `PierStr`
- Return type: `bool`
- Section of abi.h: Actor: relationships, equipment, effects, geometry (dedicated fns)
- Position in the table: slot 114, counting from 0
- Callers in each binding:
    - Rust: [`Entity::set_equipped_item`](../rust/entity.md#Entity.set_equipped_item)
    - Go: [`Raw.ActorSetEquippedItem`](../go/raw.md#Raw.ActorSetEquippedItem)

### `actor_get_effects` {#actor_get_effects}

```c
bool (*actor_get_effects)(PierActorId id, void* ctx, PierStrSink sink);
```

SNBT \[{id, ticks, amplifier, visible},…\]

- Call: `api->actor_get_effects(id, ctx, sink)`
- Parameters:
    - id : `PierActorId`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: Actor: relationships, equipment, effects, geometry (dedicated fns)
- Position in the table: slot 115, counting from 0
- Callers in each binding:
    - Rust: [`Entity::effects`](../rust/entity.md#Entity.effects)
    - Go: [`Raw.ActorGetEffects`](../go/raw.md#Raw.ActorGetEffects)

### `actor_get_status_flag` {#actor_get_status_flag}

```c
bool (*actor_get_status_flag)(PierActorId id, int32_t flag_index);
```

`flag_index`: ActorFlags enum value (0-based).

- Call: `api->actor_get_status_flag(id, flag_index)`
- Parameters:
    - id : `PierActorId`
    - flag_index : `int32_t`
- Return type: `bool`
- Section of abi.h: Actor: relationships, equipment, effects, geometry (dedicated fns)
- Position in the table: slot 116, counting from 0
- Callers in each binding:
    - Rust: [`Entity::status_flag`](../rust/entity.md#Entity.status_flag)
    - Go: [`Raw.ActorGetStatusFlag`](../go/raw.md#Raw.ActorGetStatusFlag)

### `actor_set_status_flag` {#actor_set_status_flag}

```c
bool (*actor_set_status_flag)(PierActorId id, int32_t flag_index, bool value);
```

- Call: `api->actor_set_status_flag(id, flag_index, value)`
- Parameters:
    - id : `PierActorId`
    - flag_index : `int32_t`
    - value : `bool`
- Return type: `bool`
- Section of abi.h: Actor: relationships, equipment, effects, geometry (dedicated fns)
- Position in the table: slot 117, counting from 0
- Callers in each binding:
    - Rust: [`Entity::set_status_flag`](../rust/entity.md#Entity.set_status_flag)
    - Go: [`Raw.ActorSetStatusFlag`](../go/raw.md#Raw.ActorSetStatusFlag)

### `actor_trace_ray` {#actor_trace_ray}

```c
bool (*actor_trace_ray)(PierActorId id, float max_dist, bool include_actors, bool include_blocks, void* ctx,
                        PierStrSink sink);
```

SNBT {type:"entity"|"block"|"none", pos:\[x,y,z\], `entity_id`?, `block_name`?}

- Call: `api->actor_trace_ray(id, max_dist, include_actors, include_blocks, ctx, sink)`
- Parameters:
    - id : `PierActorId`
    - max_dist : `float`
    - include_actors : `bool`
    - include_blocks : `bool`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: Actor: relationships, equipment, effects, geometry (dedicated fns)
- Position in the table: slot 118, counting from 0
- Callers in each binding:
    - Rust: [`Entity::trace_ray`](../rust/entity.md#Entity.trace_ray)
    - Go: [`Raw.ActorTraceRay`](../go/raw.md#Raw.ActorTraceRay)

### `actor_distance_to` {#actor_distance_to}

```c
bool (*actor_distance_to)(PierActorId id, PierActorId other, double* out);
```

- Call: `api->actor_distance_to(id, other, out)`
- Parameters:
    - id : `PierActorId`
    - other : `PierActorId`
    - out : `double*`
- Return type: `bool`
- Section of abi.h: Actor: relationships, equipment, effects, geometry (dedicated fns)
- Position in the table: slot 119, counting from 0
- Callers in each binding:
    - Rust: [`Entity::distance_to`](../rust/entity.md#Entity.distance_to)
    - Go: [`Entity.DistanceTo`](../go/entity.md#Entity.DistanceTo), [`Raw.ActorDistanceTo`](../go/raw.md#Raw.ActorDistanceTo)

### `actor_get_aabb` {#actor_get_aabb}

```c
bool (*actor_get_aabb)(PierActorId id, void* ctx, PierStrSink sink);
```

SNBT {min:\[x,y,z\], max:\[x,y,z\]}

- Call: `api->actor_get_aabb(id, ctx, sink)`
- Parameters:
    - id : `PierActorId`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: Actor: relationships, equipment, effects, geometry (dedicated fns)
- Position in the table: slot 120, counting from 0
- Callers in each binding:
    - Rust: [`Entity::aabb`](../rust/entity.md#Entity.aabb)
    - Go: [`Raw.ActorGetAabb`](../go/raw.md#Raw.ActorGetAabb)

### `actor_clone` {#actor_clone}

```c
bool (*actor_clone)(PierActorId id, int32_t dim, double x, double y, double z, PierActorId* out);
```

- Call: `api->actor_clone(id, dim, x, y, z, out)`
- Parameters:
    - id : `PierActorId`
    - dim : `int32_t`
    - x : `double`
    - y : `double`
    - z : `double`
    - out : `PierActorId*`
- Return type: `bool`
- Section of abi.h: Actor: relationships, equipment, effects, geometry (dedicated fns)
- Position in the table: slot 121, counting from 0
- Callers in each binding:
    - Rust: [`Entity::clone_at`](../rust/entity.md#Entity.clone_at)
    - Go: [`Entity.Clone`](../go/entity.md#Entity.Clone), [`Raw.ActorClone`](../go/raw.md#Raw.ActorClone)

## `PierActorNumProp` {#PierActorNumProp}

`actor_get_num` / `actor_set_num` keys. (S)=settable via `actor_set_num`.

| Name | Value | Description |
|---|---|---|
| <span id="PIER_APROP_POS_X"></span>`PIER_APROP_POS_X` | `0` | (G) `Actor::getPosition()`.x (feet: getFeetPos for players; POS\_\* uses getPosition) |
| <span id="PIER_APROP_POS_Y"></span>`PIER_APROP_POS_Y` | `1` | (G) |
| <span id="PIER_APROP_POS_Z"></span>`PIER_APROP_POS_Z` | `2` | (G) |
| <span id="PIER_APROP_ROT_PITCH"></span>`PIER_APROP_ROT_PITCH` | `3` | (G) `Actor::getRotation()`.x |
| <span id="PIER_APROP_ROT_YAW"></span>`PIER_APROP_ROT_YAW` | `4` | (G) `Actor::getRotation()`.y |
| <span id="PIER_APROP_DIMENSION"></span>`PIER_APROP_DIMENSION` | `5` | (G) `Actor::getDimensionId` |
| <span id="PIER_APROP_HEALTH"></span>`PIER_APROP_HEALTH` | `6` | (G) `Actor::getHealth`; heal/hurt via actions |
| <span id="PIER_APROP_MAX_HEALTH"></span>`PIER_APROP_MAX_HEALTH` | `7` | (G) `Actor::getMaxHealth` |
| <span id="PIER_APROP_IS_ALIVE"></span>`PIER_APROP_IS_ALIVE` | `8` | (G) `Actor::isAlive` |
| <span id="PIER_APROP_IS_ON_GROUND"></span>`PIER_APROP_IS_ON_GROUND` | `9` | (G) `Actor::isOnGround` |
| <span id="PIER_APROP_IS_IN_WATER"></span>`PIER_APROP_IS_IN_WATER` | `10` | (G) `Actor::isInWater` |
| <span id="PIER_APROP_IS_IN_LAVA"></span>`PIER_APROP_IS_IN_LAVA` | `11` | (G) `Actor::isInLava` |
| <span id="PIER_APROP_IS_ON_FIRE"></span>`PIER_APROP_IS_ON_FIRE` | `12` | (G) `Actor::isOnFire` |
| <span id="PIER_APROP_IS_INVISIBLE"></span>`PIER_APROP_IS_INVISIBLE` | `13` | (G) `Actor::isInvisible` |
| <span id="PIER_APROP_IS_SNEAKING"></span>`PIER_APROP_IS_SNEAKING` | `14` | (G) `Actor::isSneaking` |
| <span id="PIER_APROP_IS_BABY"></span>`PIER_APROP_IS_BABY` | `15` | (G) `Actor::isBaby` |
| <span id="PIER_APROP_IS_RIDING"></span>`PIER_APROP_IS_RIDING` | `16` | (G) `Actor::isRiding` |
| <span id="PIER_APROP_IS_TAME"></span>`PIER_APROP_IS_TAME` | `17` | (G) `Actor::isTame` |
| <span id="PIER_APROP_SPEED"></span>`PIER_APROP_SPEED` | `18` | (G) `Actor::getSpeedInMetersPerSecond` |
| <span id="PIER_APROP_VIEW_X"></span>`PIER_APROP_VIEW_X` | `19` | (G) `Actor::getViewVector()`.x |
| <span id="PIER_APROP_VIEW_Y"></span>`PIER_APROP_VIEW_Y` | `20` | (G) `Actor::getViewVector()`.y |
| <span id="PIER_APROP_VIEW_Z"></span>`PIER_APROP_VIEW_Z` | `21` | (G) `Actor::getViewVector()`.z |
| <span id="PIER_APROP_VEL_X"></span>`PIER_APROP_VEL_X` | `22` | (G) `Actor::getVelocity()`.x |
| <span id="PIER_APROP_VEL_Y"></span>`PIER_APROP_VEL_Y` | `23` | (G) `Actor::getVelocity()`.y |
| <span id="PIER_APROP_VEL_Z"></span>`PIER_APROP_VEL_Z` | `24` | (G) `Actor::getVelocity()`.z |
| <span id="PIER_APROP_HEAD_X"></span>`PIER_APROP_HEAD_X` | `25` | (G) `Actor::getHeadPos()`.x |
| <span id="PIER_APROP_HEAD_Y"></span>`PIER_APROP_HEAD_Y` | `26` | (G) `Actor::getHeadPos()`.y |
| <span id="PIER_APROP_HEAD_Z"></span>`PIER_APROP_HEAD_Z` | `27` | (G) `Actor::getHeadPos()`.z |
| <span id="PIER_APROP_FEET_X"></span>`PIER_APROP_FEET_X` | `28` | (G) `Actor::getFeetPos()`.x |
| <span id="PIER_APROP_FEET_Y"></span>`PIER_APROP_FEET_Y` | `29` | (G) `Actor::getFeetPos()`.y |
| <span id="PIER_APROP_FEET_Z"></span>`PIER_APROP_FEET_Z` | `30` | (G) `Actor::getFeetPos()`.z |
| <span id="PIER_APROP_FALL_DISTANCE"></span>`PIER_APROP_FALL_DISTANCE` | `31` | (G) `Actor::getFallDistance` |
| <span id="PIER_APROP_IS_PERSISTENT"></span>`PIER_APROP_IS_PERSISTENT` | `32` | (G) `Actor::isPersistent` |
| <span id="PIER_APROP_IS_LEASHED"></span>`PIER_APROP_IS_LEASHED` | `33` | (G) `Actor::isLeashed` |
| <span id="PIER_APROP_IS_INVULNERABLE"></span>`PIER_APROP_IS_INVULNERABLE` | `34` | (G) `Actor::isInvulnerable` |
| <span id="PIER_APROP_VARIANT"></span>`PIER_APROP_VARIANT` | `35` | (G) `Actor::getVariant` |
| <span id="PIER_APROP_MARK_VARIANT"></span>`PIER_APROP_MARK_VARIANT` | `36` | (G) `Actor::getMarkVariant` |
| <span id="PIER_APROP_SCALE"></span>`PIER_APROP_SCALE` | `37` | (G) `Actor::getScaleFactor` |
| <span id="PIER_APROP_BRIGHTNESS"></span>`PIER_APROP_BRIGHTNESS` | `38` | (G) `Actor::getBrightness` |
| <span id="PIER_APROP_RADIUS"></span>`PIER_APROP_RADIUS` | `39` | (G) `Actor::getRadius` |
| <span id="PIER_APROP_HAS_TOTEM"></span>`PIER_APROP_HAS_TOTEM` | `40` | (G) `Actor::hasTotemEquipped` |
| <span id="PIER_APROP_IS_IN_RAIN"></span>`PIER_APROP_IS_IN_RAIN` | `41` | (G) `Actor::isInRain` |
| <span id="PIER_APROP_IS_IN_SNOW"></span>`PIER_APROP_IS_IN_SNOW` | `42` | (G) `Actor::isInSnow` |
| <span id="PIER_APROP_IS_IN_THUNDERSTORM"></span>`PIER_APROP_IS_IN_THUNDERSTORM` | `43` | (G) `Actor::isInThunderstorm` |
| <span id="PIER_APROP_IS_FROZEN"></span>`PIER_APROP_IS_FROZEN` | `44` | (G) `Actor::isFrozen` |
| <span id="PIER_APROP_IS_IN_LOVE"></span>`PIER_APROP_IS_IN_LOVE` | `45` | **the current engine version gives no value for this constant**: (G) unsupported since BDS 1.26.40: `Actor::isInLove` is gone |
| <span id="PIER_APROP_DEATH_TIME"></span>`PIER_APROP_DEATH_TIME` | `46` | (G) `Actor::getDeathTime` |
| <span id="PIER_APROP_HAS_PASSENGER"></span>`PIER_APROP_HAS_PASSENGER` | `47` | (G) `Actor::hasPassenger` |

## `PierActorStrProp` {#PierActorStrProp}

`actor_get_str` keys.

| Name | Value | Description |
|---|---|---|
| <span id="PIER_ASTR_TYPE_NAME"></span>`PIER_ASTR_TYPE_NAME` | `0` | `Actor::getTypeName` |
| <span id="PIER_ASTR_NAME_TAG"></span>`PIER_ASTR_NAME_TAG` | `1` | `Actor::getNameTag` |
| <span id="PIER_ASTR_SCORE_TAG"></span>`PIER_ASTR_SCORE_TAG` | `2` | `Actor::getScoreTag` |
| <span id="PIER_ASTR_FILTERED_NAME"></span>`PIER_ASTR_FILTERED_NAME` | `3` | `Actor::getFilteredNameTag` |

## `PierActorAction` {#PierActorAction}

`actor_action` verbs. Args (sarg, a, b, c); `out` receives a result where noted.

| Name | Value | Description |
|---|---|---|
| <span id="PIER_AACT_KILL"></span>`PIER_AACT_KILL` | `0` | `Actor::kill` |
| <span id="PIER_AACT_DESPAWN"></span>`PIER_AACT_DESPAWN` | `1` | `Actor::despawn` |
| <span id="PIER_AACT_HEAL"></span>`PIER_AACT_HEAL` | `2` | a=amount `Actor::heal` |
| <span id="PIER_AACT_SET_ON_FIRE"></span>`PIER_AACT_SET_ON_FIRE` | `3` | a=seconds `Actor::setOnFire` |
| <span id="PIER_AACT_TELEPORT"></span>`PIER_AACT_TELEPORT` | `4` | a,b,c=pos, sarg=dim ("0".."2") `Actor::teleport` |
| <span id="PIER_AACT_SET_NAME_TAG"></span>`PIER_AACT_SET_NAME_TAG` | `5` | sarg=name `Actor::setNameTag` |
| <span id="PIER_AACT_ADD_TAG"></span>`PIER_AACT_ADD_TAG` | `6` | sarg=tag → out "0"/"1" `Actor::addTag` |
| <span id="PIER_AACT_REMOVE_TAG"></span>`PIER_AACT_REMOVE_TAG` | `7` | sarg=tag → out "0"/"1" `Actor::removeTag` |
| <span id="PIER_AACT_HAS_TAG"></span>`PIER_AACT_HAS_TAG` | `8` | sarg=tag → out "0"/"1" `Actor::hasTag` |
| <span id="PIER_AACT_ADD_EFFECT"></span>`PIER_AACT_ADD_EFFECT` | `9` | sarg=effect name, a=ticks, b=amplifier, c=visible(0/1) `MobEffect::getByName` + `Actor::addEffect` |
| <span id="PIER_AACT_REMOVE_EFFECT"></span>`PIER_AACT_REMOVE_EFFECT` | `10` | sarg=effect name `Actor::removeEffect`(id) |
| <span id="PIER_AACT_CLEAR_EFFECTS"></span>`PIER_AACT_CLEAR_EFFECTS` | `11` | `Actor::removeAllEffects` |
| <span id="PIER_AACT_HURT"></span>`PIER_AACT_HURT` | `12` | a=damage (generic damage source) `Actor::hurt` |
| <span id="PIER_AACT_ATTRIBUTE_GET"></span>`PIER_AACT_ATTRIBUTE_GET` | `13` | sarg=attribute name ("minecraft:health"…) → out value |
| <span id="PIER_AACT_SET_VARIANT"></span>`PIER_AACT_SET_VARIANT` | `14` | a=variant `Actor::setVariant` |
| <span id="PIER_AACT_SET_MARK_VARIANT"></span>`PIER_AACT_SET_MARK_VARIANT` | `15` | a=variant `Actor::setMarkVariant` |
| <span id="PIER_AACT_SET_PERSISTENT"></span>`PIER_AACT_SET_PERSISTENT` | `16` | `Actor::setPersistent` |
| <span id="PIER_AACT_SET_LEASH_HOLDER"></span>`PIER_AACT_SET_LEASH_HOLDER` | `17` | a=holder ActorUniqueID `Actor::setLeashHolder` |
| <span id="PIER_AACT_SET_INVISIBLE"></span>`PIER_AACT_SET_INVISIBLE` | `18` | a=0/1 `Actor::setInvisible` |
| <span id="PIER_AACT_SET_SNEAKING"></span>`PIER_AACT_SET_SNEAKING` | `19` | a=0/1 `Actor::setSneaking` |
| <span id="PIER_AACT_SET_NAME_TAG_VISIBLE"></span>`PIER_AACT_SET_NAME_TAG_VISIBLE` | `20` | a=0/1 `Actor::setNameTagVisible` |
| <span id="PIER_AACT_SET_TARGET"></span>`PIER_AACT_SET_TARGET` | `21` | a=target ActorUniqueID `Actor::setTarget` |
| <span id="PIER_AACT_SET_OWNER"></span>`PIER_AACT_SET_OWNER` | `22` | a=owner ActorUniqueID `Actor::setOwner` |
| <span id="PIER_AACT_BURN"></span>`PIER_AACT_BURN` | `23` | a=damage `Actor::burn` |
| <span id="PIER_AACT_STOP_FIRE"></span>`PIER_AACT_STOP_FIRE` | `24` | `Actor::extinguishFire` |
| <span id="PIER_AACT_SET_VELOCITY"></span>`PIER_AACT_SET_VELOCITY` | `25` | a,b,c=vel `Actor::setVelocity` |
| <span id="PIER_AACT_APPLY_IMPULSE"></span>`PIER_AACT_APPLY_IMPULSE` | `26` | a,b,c=impulse `Actor::applyImpulse` |
| <span id="PIER_AACT_SET_SCORE_TAG"></span>`PIER_AACT_SET_SCORE_TAG` | `27` | sarg=text `Actor::setScoreTag` |
| <span id="PIER_AACT_SET_SKIN_ID"></span>`PIER_AACT_SET_SKIN_ID` | `28` | a=skin id `Actor::setSkinID` |
| <span id="PIER_AACT_SET_STRENGTH"></span>`PIER_AACT_SET_STRENGTH` | `29` | a=strength `Actor::setStrength` |
| <span id="PIER_AACT_REMOVE_ALL_PASSENGERS"></span>`PIER_AACT_REMOVE_ALL_PASSENGERS` | `30` | `Actor::removeAllPassengers` |
| <span id="PIER_AACT_EXECUTE_EVENT"></span>`PIER_AACT_EXECUTE_EVENT` | `31` | sarg=event name `Actor::executeEvent` |
| <span id="PIER_AACT_SET_ROTATION"></span>`PIER_AACT_SET_ROTATION` | `32` | a=pitch b=yaw `Actor::setRotationWrapped` |
