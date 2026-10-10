# Zig: Actors

## `Entity` {#Entity}

```zig
pub const Entity = struct {
    actor_id: i64,
    // ...
};
```

One actor, named by its unique id, the uid of event payloads.

### `Entity.of` {#Entity.of}

```zig
pub fn of(actor_id_value: i64) Entity
```

- Parameters:
    - actor_id_value : `i64`
- Return type: `Entity`

### `Entity.posX` {#Entity.posX}

```zig
pub fn posX(self: Entity) core.Error!f64
```

`PIER_APROP_POS_X`: (G) `Actor::getPosition()`.x (feet: getFeetPos for players; POS\_\* uses getPosition)

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.posY` {#Entity.posY}

```zig
pub fn posY(self: Entity) core.Error!f64
```

`PIER_APROP_POS_Y`: (G)

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.posZ` {#Entity.posZ}

```zig
pub fn posZ(self: Entity) core.Error!f64
```

`PIER_APROP_POS_Z`: (G)

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.rotPitch` {#Entity.rotPitch}

```zig
pub fn rotPitch(self: Entity) core.Error!f64
```

`PIER_APROP_ROT_PITCH`: (G) `Actor::getRotation()`.x

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.rotYaw` {#Entity.rotYaw}

```zig
pub fn rotYaw(self: Entity) core.Error!f64
```

`PIER_APROP_ROT_YAW`: (G) `Actor::getRotation()`.y

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.dimension` {#Entity.dimension}

```zig
pub fn dimension(self: Entity) core.Error!f64
```

`PIER_APROP_DIMENSION`: (G) `Actor::getDimensionId`

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.health` {#Entity.health}

```zig
pub fn health(self: Entity) core.Error!f64
```

`PIER_APROP_HEALTH`: (G) `Actor::getHealth`; heal/hurt via actions

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.maxHealth` {#Entity.maxHealth}

```zig
pub fn maxHealth(self: Entity) core.Error!f64
```

`PIER_APROP_MAX_HEALTH`: (G) `Actor::getMaxHealth`

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isAlive` {#Entity.isAlive}

```zig
pub fn isAlive(self: Entity) core.Error!bool
```

`PIER_APROP_IS_ALIVE`: (G) `Actor::isAlive`

- Return type: `core.Error!bool`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isOnGround` {#Entity.isOnGround}

```zig
pub fn isOnGround(self: Entity) core.Error!bool
```

`PIER_APROP_IS_ON_GROUND`: (G) `Actor::isOnGround`

- Return type: `core.Error!bool`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isInWater` {#Entity.isInWater}

```zig
pub fn isInWater(self: Entity) core.Error!bool
```

`PIER_APROP_IS_IN_WATER`: (G) `Actor::isInWater`

- Return type: `core.Error!bool`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isInLava` {#Entity.isInLava}

```zig
pub fn isInLava(self: Entity) core.Error!bool
```

`PIER_APROP_IS_IN_LAVA`: (G) `Actor::isInLava`

- Return type: `core.Error!bool`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isOnFire` {#Entity.isOnFire}

```zig
pub fn isOnFire(self: Entity) core.Error!bool
```

`PIER_APROP_IS_ON_FIRE`: (G) `Actor::isOnFire`

- Return type: `core.Error!bool`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isInvisible` {#Entity.isInvisible}

```zig
pub fn isInvisible(self: Entity) core.Error!bool
```

`PIER_APROP_IS_INVISIBLE`: (G) `Actor::isInvisible`

- Return type: `core.Error!bool`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isSneaking` {#Entity.isSneaking}

```zig
pub fn isSneaking(self: Entity) core.Error!bool
```

`PIER_APROP_IS_SNEAKING`: (G) `Actor::isSneaking`

- Return type: `core.Error!bool`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isBaby` {#Entity.isBaby}

```zig
pub fn isBaby(self: Entity) core.Error!bool
```

`PIER_APROP_IS_BABY`: (G) `Actor::isBaby`

- Return type: `core.Error!bool`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isRiding` {#Entity.isRiding}

```zig
pub fn isRiding(self: Entity) core.Error!bool
```

`PIER_APROP_IS_RIDING`: (G) `Actor::isRiding`

- Return type: `core.Error!bool`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isTame` {#Entity.isTame}

```zig
pub fn isTame(self: Entity) core.Error!bool
```

`PIER_APROP_IS_TAME`: (G) `Actor::isTame`

- Return type: `core.Error!bool`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.speed` {#Entity.speed}

```zig
pub fn speed(self: Entity) core.Error!f64
```

`PIER_APROP_SPEED`: (G) `Actor::getSpeedInMetersPerSecond`

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.viewX` {#Entity.viewX}

```zig
pub fn viewX(self: Entity) core.Error!f64
```

`PIER_APROP_VIEW_X`: (G) `Actor::getViewVector()`.x

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.viewY` {#Entity.viewY}

```zig
pub fn viewY(self: Entity) core.Error!f64
```

`PIER_APROP_VIEW_Y`: (G) `Actor::getViewVector()`.y

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.viewZ` {#Entity.viewZ}

```zig
pub fn viewZ(self: Entity) core.Error!f64
```

`PIER_APROP_VIEW_Z`: (G) `Actor::getViewVector()`.z

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.velX` {#Entity.velX}

```zig
pub fn velX(self: Entity) core.Error!f64
```

`PIER_APROP_VEL_X`: (G) `Actor::getVelocity()`.x

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.velY` {#Entity.velY}

```zig
pub fn velY(self: Entity) core.Error!f64
```

`PIER_APROP_VEL_Y`: (G) `Actor::getVelocity()`.y

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.velZ` {#Entity.velZ}

```zig
pub fn velZ(self: Entity) core.Error!f64
```

`PIER_APROP_VEL_Z`: (G) `Actor::getVelocity()`.z

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.headX` {#Entity.headX}

```zig
pub fn headX(self: Entity) core.Error!f64
```

`PIER_APROP_HEAD_X`: (G) `Actor::getHeadPos()`.x

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.headY` {#Entity.headY}

```zig
pub fn headY(self: Entity) core.Error!f64
```

`PIER_APROP_HEAD_Y`: (G) `Actor::getHeadPos()`.y

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.headZ` {#Entity.headZ}

```zig
pub fn headZ(self: Entity) core.Error!f64
```

`PIER_APROP_HEAD_Z`: (G) `Actor::getHeadPos()`.z

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.feetX` {#Entity.feetX}

```zig
pub fn feetX(self: Entity) core.Error!f64
```

`PIER_APROP_FEET_X`: (G) `Actor::getFeetPos()`.x

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.feetY` {#Entity.feetY}

```zig
pub fn feetY(self: Entity) core.Error!f64
```

`PIER_APROP_FEET_Y`: (G) `Actor::getFeetPos()`.y

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.feetZ` {#Entity.feetZ}

```zig
pub fn feetZ(self: Entity) core.Error!f64
```

`PIER_APROP_FEET_Z`: (G) `Actor::getFeetPos()`.z

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.fallDistance` {#Entity.fallDistance}

```zig
pub fn fallDistance(self: Entity) core.Error!f64
```

`PIER_APROP_FALL_DISTANCE`: (G) `Actor::getFallDistance`

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isPersistent` {#Entity.isPersistent}

```zig
pub fn isPersistent(self: Entity) core.Error!bool
```

`PIER_APROP_IS_PERSISTENT`: (G) `Actor::isPersistent`

- Return type: `core.Error!bool`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isLeashed` {#Entity.isLeashed}

```zig
pub fn isLeashed(self: Entity) core.Error!bool
```

`PIER_APROP_IS_LEASHED`: (G) `Actor::isLeashed`

- Return type: `core.Error!bool`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isInvulnerable` {#Entity.isInvulnerable}

```zig
pub fn isInvulnerable(self: Entity) core.Error!bool
```

`PIER_APROP_IS_INVULNERABLE`: (G) `Actor::isInvulnerable`

- Return type: `core.Error!bool`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.variant` {#Entity.variant}

```zig
pub fn variant(self: Entity) core.Error!f64
```

`PIER_APROP_VARIANT`: (G) `Actor::getVariant`

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.markVariant` {#Entity.markVariant}

```zig
pub fn markVariant(self: Entity) core.Error!f64
```

`PIER_APROP_MARK_VARIANT`: (G) `Actor::getMarkVariant`

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.scale` {#Entity.scale}

```zig
pub fn scale(self: Entity) core.Error!f64
```

`PIER_APROP_SCALE`: (G) `Actor::getScaleFactor`

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.brightness` {#Entity.brightness}

```zig
pub fn brightness(self: Entity) core.Error!f64
```

`PIER_APROP_BRIGHTNESS`: (G) `Actor::getBrightness`

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.radius` {#Entity.radius}

```zig
pub fn radius(self: Entity) core.Error!f64
```

`PIER_APROP_RADIUS`: (G) `Actor::getRadius`

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.hasTotem` {#Entity.hasTotem}

```zig
pub fn hasTotem(self: Entity) core.Error!bool
```

`PIER_APROP_HAS_TOTEM`: (G) `Actor::hasTotemEquipped`

- Return type: `core.Error!bool`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isInRain` {#Entity.isInRain}

```zig
pub fn isInRain(self: Entity) core.Error!bool
```

`PIER_APROP_IS_IN_RAIN`: (G) `Actor::isInRain`

- Return type: `core.Error!bool`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isInSnow` {#Entity.isInSnow}

```zig
pub fn isInSnow(self: Entity) core.Error!bool
```

`PIER_APROP_IS_IN_SNOW`: (G) `Actor::isInSnow`

- Return type: `core.Error!bool`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isInThunderstorm` {#Entity.isInThunderstorm}

```zig
pub fn isInThunderstorm(self: Entity) core.Error!bool
```

`PIER_APROP_IS_IN_THUNDERSTORM`: (G) `Actor::isInThunderstorm`

- Return type: `core.Error!bool`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isFrozen` {#Entity.isFrozen}

```zig
pub fn isFrozen(self: Entity) core.Error!bool
```

`PIER_APROP_IS_FROZEN`: (G) `Actor::isFrozen`

- Return type: `core.Error!bool`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isInLove` {#Entity.isInLove}

```zig
pub fn isInLove(self: Entity) core.Error!bool
```

`PIER_APROP_IS_IN_LOVE`: (G) unsupported since BDS 1.26.40: `Actor::isInLove` is gone

- Return type: `core.Error!bool`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.deathTime` {#Entity.deathTime}

```zig
pub fn deathTime(self: Entity) core.Error!f64
```

`PIER_APROP_DEATH_TIME`: (G) `Actor::getDeathTime`

- Return type: `core.Error!f64`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.hasPassenger` {#Entity.hasPassenger}

```zig
pub fn hasPassenger(self: Entity) core.Error!bool
```

`PIER_APROP_HAS_PASSENGER`: (G) `Actor::hasPassenger`

- Return type: `core.Error!bool`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.typeName` {#Entity.typeName}

```zig
pub fn typeName(self: Entity, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ASTR_TYPE_NAME`: `Actor::getTypeName`

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity.nameTag` {#Entity.nameTag}

```zig
pub fn nameTag(self: Entity, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ASTR_NAME_TAG`: `Actor::getNameTag`

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity.scoreTag` {#Entity.scoreTag}

```zig
pub fn scoreTag(self: Entity, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ASTR_SCORE_TAG`: `Actor::getScoreTag`

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity.filteredName` {#Entity.filteredName}

```zig
pub fn filteredName(self: Entity, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ASTR_FILTERED_NAME`: `Actor::getFilteredNameTag`

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity.kill` {#Entity.kill}

```zig
pub fn kill(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_KILL`: `Actor::kill`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.despawn` {#Entity.despawn}

```zig
pub fn despawn(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_DESPAWN`: `Actor::despawn`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.heal` {#Entity.heal}

```zig
pub fn heal(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_HEAL`: a=amount `Actor::heal`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setOnFire` {#Entity.setOnFire}

```zig
pub fn setOnFire(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_ON_FIRE`: a=seconds `Actor::setOnFire`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.teleport` {#Entity.teleport}

```zig
pub fn teleport(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_TELEPORT`: a,b,c=pos, sarg=dim ("0".."2") `Actor::teleport`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setNameTag` {#Entity.setNameTag}

```zig
pub fn setNameTag(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_NAME_TAG`: sarg=name `Actor::setNameTag`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.addTag` {#Entity.addTag}

```zig
pub fn addTag(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_ADD_TAG`: sarg=tag → out "0"/"1" `Actor::addTag`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.removeTag` {#Entity.removeTag}

```zig
pub fn removeTag(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_REMOVE_TAG`: sarg=tag → out "0"/"1" `Actor::removeTag`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.hasTag` {#Entity.hasTag}

```zig
pub fn hasTag(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_HAS_TAG`: sarg=tag → out "0"/"1" `Actor::hasTag`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.addEffect` {#Entity.addEffect}

```zig
pub fn addEffect(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_ADD_EFFECT`: sarg=effect name, a=ticks, b=amplifier, c=visible(0/1) `MobEffect::getByName` + `Actor::addEffect`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.removeEffect` {#Entity.removeEffect}

```zig
pub fn removeEffect(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_REMOVE_EFFECT`: sarg=effect name `Actor::removeEffect`(id)

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.clearEffects` {#Entity.clearEffects}

```zig
pub fn clearEffects(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_CLEAR_EFFECTS`: `Actor::removeAllEffects`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.hurt` {#Entity.hurt}

```zig
pub fn hurt(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_HURT`: a=damage (generic damage source) `Actor::hurt`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.attributeGet` {#Entity.attributeGet}

```zig
pub fn attributeGet(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_ATTRIBUTE_GET`: sarg=attribute name ("minecraft:health"…) → out value

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setVariant` {#Entity.setVariant}

```zig
pub fn setVariant(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_VARIANT`: a=variant `Actor::setVariant`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setMarkVariant` {#Entity.setMarkVariant}

```zig
pub fn setMarkVariant(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_MARK_VARIANT`: a=variant `Actor::setMarkVariant`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setPersistent` {#Entity.setPersistent}

```zig
pub fn setPersistent(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_PERSISTENT`: `Actor::setPersistent`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setLeashHolder` {#Entity.setLeashHolder}

```zig
pub fn setLeashHolder(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_LEASH_HOLDER`: a=holder ActorUniqueID `Actor::setLeashHolder`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setInvisible` {#Entity.setInvisible}

```zig
pub fn setInvisible(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_INVISIBLE`: a=0/1 `Actor::setInvisible`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setSneaking` {#Entity.setSneaking}

```zig
pub fn setSneaking(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_SNEAKING`: a=0/1 `Actor::setSneaking`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setNameTagVisible` {#Entity.setNameTagVisible}

```zig
pub fn setNameTagVisible(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_NAME_TAG_VISIBLE`: a=0/1 `Actor::setNameTagVisible`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setTarget` {#Entity.setTarget}

```zig
pub fn setTarget(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_TARGET`: a=target ActorUniqueID `Actor::setTarget`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setOwner` {#Entity.setOwner}

```zig
pub fn setOwner(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_OWNER`: a=owner ActorUniqueID `Actor::setOwner`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.burn` {#Entity.burn}

```zig
pub fn burn(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_BURN`: a=damage `Actor::burn`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.stopFire` {#Entity.stopFire}

```zig
pub fn stopFire(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_STOP_FIRE`: `Actor::extinguishFire`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setVelocity` {#Entity.setVelocity}

```zig
pub fn setVelocity(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_VELOCITY`: a,b,c=vel `Actor::setVelocity`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.applyImpulse` {#Entity.applyImpulse}

```zig
pub fn applyImpulse(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_APPLY_IMPULSE`: a,b,c=impulse `Actor::applyImpulse`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setScoreTag` {#Entity.setScoreTag}

```zig
pub fn setScoreTag(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_SCORE_TAG`: sarg=text `Actor::setScoreTag`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setSkinId` {#Entity.setSkinId}

```zig
pub fn setSkinId(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_SKIN_ID`: a=skin id `Actor::setSkinID`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setStrength` {#Entity.setStrength}

```zig
pub fn setStrength(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_STRENGTH`: a=strength `Actor::setStrength`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.removeAllPassengers` {#Entity.removeAllPassengers}

```zig
pub fn removeAllPassengers(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_REMOVE_ALL_PASSENGERS`: `Actor::removeAllPassengers`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.executeEvent` {#Entity.executeEvent}

```zig
pub fn executeEvent(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_EXECUTE_EVENT`: sarg=event name `Actor::executeEvent`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setRotation` {#Entity.setRotation}

```zig
pub fn setRotation(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_ROTATION`: a=pitch b=yaw `Actor::setRotationWrapped`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)
