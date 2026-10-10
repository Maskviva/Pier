# Zig：实体

## `Entity` {#Entity}

```zig
pub const Entity = struct {
    actor_id: i64,
    // ...
};
```

一个实体，由它的唯一 id 指定，也就是事件载荷里的 uid。

### `Entity.of` {#Entity.of}

```zig
pub fn of(actor_id_value: i64) Entity
```

- 参数：
    - actor_id_value : `i64`
- 返回值类型：`Entity`

### `Entity.posX` {#Entity.posX}

```zig
pub fn posX(self: Entity) core.Error!f64
```

`PIER_APROP_POS_X`：(G) `Actor::getPosition().x`（玩家的脚下位置要用 `getFeetPos`；`POS_*` 用的是 `getPosition`）

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.posY` {#Entity.posY}

```zig
pub fn posY(self: Entity) core.Error!f64
```

`PIER_APROP_POS_Y`：(G)

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.posZ` {#Entity.posZ}

```zig
pub fn posZ(self: Entity) core.Error!f64
```

`PIER_APROP_POS_Z`：(G)

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.rotPitch` {#Entity.rotPitch}

```zig
pub fn rotPitch(self: Entity) core.Error!f64
```

`PIER_APROP_ROT_PITCH`：(G) `Actor::getRotation().x`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.rotYaw` {#Entity.rotYaw}

```zig
pub fn rotYaw(self: Entity) core.Error!f64
```

`PIER_APROP_ROT_YAW`：(G) `Actor::getRotation().y`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.dimension` {#Entity.dimension}

```zig
pub fn dimension(self: Entity) core.Error!f64
```

`PIER_APROP_DIMENSION`：(G) `Actor::getDimensionId`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.health` {#Entity.health}

```zig
pub fn health(self: Entity) core.Error!f64
```

`PIER_APROP_HEALTH`：(G) `Actor::getHealth`；治疗和伤害用动作完成

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.maxHealth` {#Entity.maxHealth}

```zig
pub fn maxHealth(self: Entity) core.Error!f64
```

`PIER_APROP_MAX_HEALTH`：(G) `Actor::getMaxHealth`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isAlive` {#Entity.isAlive}

```zig
pub fn isAlive(self: Entity) core.Error!bool
```

`PIER_APROP_IS_ALIVE`：(G) `Actor::isAlive`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isOnGround` {#Entity.isOnGround}

```zig
pub fn isOnGround(self: Entity) core.Error!bool
```

`PIER_APROP_IS_ON_GROUND`：(G) `Actor::isOnGround`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isInWater` {#Entity.isInWater}

```zig
pub fn isInWater(self: Entity) core.Error!bool
```

`PIER_APROP_IS_IN_WATER`：(G) `Actor::isInWater`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isInLava` {#Entity.isInLava}

```zig
pub fn isInLava(self: Entity) core.Error!bool
```

`PIER_APROP_IS_IN_LAVA`：(G) `Actor::isInLava`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isOnFire` {#Entity.isOnFire}

```zig
pub fn isOnFire(self: Entity) core.Error!bool
```

`PIER_APROP_IS_ON_FIRE`：(G) `Actor::isOnFire`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isInvisible` {#Entity.isInvisible}

```zig
pub fn isInvisible(self: Entity) core.Error!bool
```

`PIER_APROP_IS_INVISIBLE`：(G) `Actor::isInvisible`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isSneaking` {#Entity.isSneaking}

```zig
pub fn isSneaking(self: Entity) core.Error!bool
```

`PIER_APROP_IS_SNEAKING`：(G) `Actor::isSneaking`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isBaby` {#Entity.isBaby}

```zig
pub fn isBaby(self: Entity) core.Error!bool
```

`PIER_APROP_IS_BABY`：(G) `Actor::isBaby`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isRiding` {#Entity.isRiding}

```zig
pub fn isRiding(self: Entity) core.Error!bool
```

`PIER_APROP_IS_RIDING`：(G) `Actor::isRiding`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isTame` {#Entity.isTame}

```zig
pub fn isTame(self: Entity) core.Error!bool
```

`PIER_APROP_IS_TAME`：(G) `Actor::isTame`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.speed` {#Entity.speed}

```zig
pub fn speed(self: Entity) core.Error!f64
```

`PIER_APROP_SPEED`：(G) `Actor::getSpeedInMetersPerSecond`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.viewX` {#Entity.viewX}

```zig
pub fn viewX(self: Entity) core.Error!f64
```

`PIER_APROP_VIEW_X`：(G) `Actor::getViewVector().x`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.viewY` {#Entity.viewY}

```zig
pub fn viewY(self: Entity) core.Error!f64
```

`PIER_APROP_VIEW_Y`：(G) `Actor::getViewVector().y`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.viewZ` {#Entity.viewZ}

```zig
pub fn viewZ(self: Entity) core.Error!f64
```

`PIER_APROP_VIEW_Z`：(G) `Actor::getViewVector().z`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.velX` {#Entity.velX}

```zig
pub fn velX(self: Entity) core.Error!f64
```

`PIER_APROP_VEL_X`：(G) `Actor::getVelocity().x`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.velY` {#Entity.velY}

```zig
pub fn velY(self: Entity) core.Error!f64
```

`PIER_APROP_VEL_Y`：(G) `Actor::getVelocity().y`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.velZ` {#Entity.velZ}

```zig
pub fn velZ(self: Entity) core.Error!f64
```

`PIER_APROP_VEL_Z`：(G) `Actor::getVelocity().z`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.headX` {#Entity.headX}

```zig
pub fn headX(self: Entity) core.Error!f64
```

`PIER_APROP_HEAD_X`：(G) `Actor::getHeadPos().x`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.headY` {#Entity.headY}

```zig
pub fn headY(self: Entity) core.Error!f64
```

`PIER_APROP_HEAD_Y`：(G) `Actor::getHeadPos().y`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.headZ` {#Entity.headZ}

```zig
pub fn headZ(self: Entity) core.Error!f64
```

`PIER_APROP_HEAD_Z`：(G) `Actor::getHeadPos().z`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.feetX` {#Entity.feetX}

```zig
pub fn feetX(self: Entity) core.Error!f64
```

`PIER_APROP_FEET_X`：(G) `Actor::getFeetPos().x`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.feetY` {#Entity.feetY}

```zig
pub fn feetY(self: Entity) core.Error!f64
```

`PIER_APROP_FEET_Y`：(G) `Actor::getFeetPos().y`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.feetZ` {#Entity.feetZ}

```zig
pub fn feetZ(self: Entity) core.Error!f64
```

`PIER_APROP_FEET_Z`：(G) `Actor::getFeetPos().z`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.fallDistance` {#Entity.fallDistance}

```zig
pub fn fallDistance(self: Entity) core.Error!f64
```

`PIER_APROP_FALL_DISTANCE`：(G) `Actor::getFallDistance`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isPersistent` {#Entity.isPersistent}

```zig
pub fn isPersistent(self: Entity) core.Error!bool
```

`PIER_APROP_IS_PERSISTENT`：(G) `Actor::isPersistent`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isLeashed` {#Entity.isLeashed}

```zig
pub fn isLeashed(self: Entity) core.Error!bool
```

`PIER_APROP_IS_LEASHED`：(G) `Actor::isLeashed`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isInvulnerable` {#Entity.isInvulnerable}

```zig
pub fn isInvulnerable(self: Entity) core.Error!bool
```

`PIER_APROP_IS_INVULNERABLE`：(G) `Actor::isInvulnerable`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.variant` {#Entity.variant}

```zig
pub fn variant(self: Entity) core.Error!f64
```

`PIER_APROP_VARIANT`：(G) `Actor::getVariant`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.markVariant` {#Entity.markVariant}

```zig
pub fn markVariant(self: Entity) core.Error!f64
```

`PIER_APROP_MARK_VARIANT`：(G) `Actor::getMarkVariant`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.scale` {#Entity.scale}

```zig
pub fn scale(self: Entity) core.Error!f64
```

`PIER_APROP_SCALE`：(G) `Actor::getScaleFactor`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.brightness` {#Entity.brightness}

```zig
pub fn brightness(self: Entity) core.Error!f64
```

`PIER_APROP_BRIGHTNESS`：(G) `Actor::getBrightness`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.radius` {#Entity.radius}

```zig
pub fn radius(self: Entity) core.Error!f64
```

`PIER_APROP_RADIUS`：(G) `Actor::getRadius`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.hasTotem` {#Entity.hasTotem}

```zig
pub fn hasTotem(self: Entity) core.Error!bool
```

`PIER_APROP_HAS_TOTEM`：(G) `Actor::hasTotemEquipped`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isInRain` {#Entity.isInRain}

```zig
pub fn isInRain(self: Entity) core.Error!bool
```

`PIER_APROP_IS_IN_RAIN`：(G) `Actor::isInRain`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isInSnow` {#Entity.isInSnow}

```zig
pub fn isInSnow(self: Entity) core.Error!bool
```

`PIER_APROP_IS_IN_SNOW`：(G) `Actor::isInSnow`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isInThunderstorm` {#Entity.isInThunderstorm}

```zig
pub fn isInThunderstorm(self: Entity) core.Error!bool
```

`PIER_APROP_IS_IN_THUNDERSTORM`：(G) `Actor::isInThunderstorm`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isFrozen` {#Entity.isFrozen}

```zig
pub fn isFrozen(self: Entity) core.Error!bool
```

`PIER_APROP_IS_FROZEN`：(G) `Actor::isFrozen`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.isInLove` {#Entity.isInLove}

```zig
pub fn isInLove(self: Entity) core.Error!bool
```

`PIER_APROP_IS_IN_LOVE`：(G) 从 BDS 1.26.40 起不再支持：`Actor::isInLove` 已被移除

- 返回值类型：`core.Error!bool`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.deathTime` {#Entity.deathTime}

```zig
pub fn deathTime(self: Entity) core.Error!f64
```

`PIER_APROP_DEATH_TIME`：(G) `Actor::getDeathTime`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.hasPassenger` {#Entity.hasPassenger}

```zig
pub fn hasPassenger(self: Entity) core.Error!bool
```

`PIER_APROP_HAS_PASSENGER`：(G) `Actor::hasPassenger`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.typeName` {#Entity.typeName}

```zig
pub fn typeName(self: Entity, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ASTR_TYPE_NAME`：取自 `Actor::getTypeName`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity.nameTag` {#Entity.nameTag}

```zig
pub fn nameTag(self: Entity, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ASTR_NAME_TAG`：取自 `Actor::getNameTag`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity.scoreTag` {#Entity.scoreTag}

```zig
pub fn scoreTag(self: Entity, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ASTR_SCORE_TAG`：取自 `Actor::getScoreTag`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity.filteredName` {#Entity.filteredName}

```zig
pub fn filteredName(self: Entity, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_ASTR_FILTERED_NAME`：取自 `Actor::getFilteredNameTag`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity.kill` {#Entity.kill}

```zig
pub fn kill(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_KILL`：调用 `Actor::kill`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.despawn` {#Entity.despawn}

```zig
pub fn despawn(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_DESPAWN`：调用 `Actor::despawn`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.heal` {#Entity.heal}

```zig
pub fn heal(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_HEAL`：`a` 为治疗量，调用 `Actor::heal`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setOnFire` {#Entity.setOnFire}

```zig
pub fn setOnFire(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_ON_FIRE`：`a` 为秒数，调用 `Actor::setOnFire`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.teleport` {#Entity.teleport}

```zig
pub fn teleport(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_TELEPORT`：`a`、`b`、`c` 为坐标，`sarg` 为维度（`"0"` 到 `"2"`），调用 `Actor::teleport`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setNameTag` {#Entity.setNameTag}

```zig
pub fn setNameTag(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_NAME_TAG`：`sarg` 为名字，调用 `Actor::setNameTag`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.addTag` {#Entity.addTag}

```zig
pub fn addTag(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_ADD_TAG`：`sarg` 为标签，输出 `"0"` 或 `"1"`，调用 `Actor::addTag`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.removeTag` {#Entity.removeTag}

```zig
pub fn removeTag(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_REMOVE_TAG`：`sarg` 为标签，输出 `"0"` 或 `"1"`，调用 `Actor::removeTag`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.hasTag` {#Entity.hasTag}

```zig
pub fn hasTag(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_HAS_TAG`：`sarg` 为标签，输出 `"0"` 或 `"1"`，调用 `Actor::hasTag`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.addEffect` {#Entity.addEffect}

```zig
pub fn addEffect(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_ADD_EFFECT`：`sarg` 为效果名，`a` 为刻数，`b` 为效果等级，`c` 为是否显示粒子（0 或 1），经 `MobEffect::getByName` 和 `Actor::addEffect` 完成

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.removeEffect` {#Entity.removeEffect}

```zig
pub fn removeEffect(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_REMOVE_EFFECT`：`sarg` 为效果名，调用 `Actor::removeEffect(id)`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.clearEffects` {#Entity.clearEffects}

```zig
pub fn clearEffects(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_CLEAR_EFFECTS`：调用 `Actor::removeAllEffects`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.hurt` {#Entity.hurt}

```zig
pub fn hurt(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_HURT`：`a` 为伤害值（通用伤害来源），调用 `Actor::hurt`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.attributeGet` {#Entity.attributeGet}

```zig
pub fn attributeGet(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_ATTRIBUTE_GET`：`sarg` 为属性名（`"minecraft:health"` 等），输出属性值

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setVariant` {#Entity.setVariant}

```zig
pub fn setVariant(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_VARIANT`：`a` 为变种值，调用 `Actor::setVariant`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setMarkVariant` {#Entity.setMarkVariant}

```zig
pub fn setMarkVariant(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_MARK_VARIANT`：`a` 为变种值，调用 `Actor::setMarkVariant`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setPersistent` {#Entity.setPersistent}

```zig
pub fn setPersistent(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_PERSISTENT`：调用 `Actor::setPersistent`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setLeashHolder` {#Entity.setLeashHolder}

```zig
pub fn setLeashHolder(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_LEASH_HOLDER`：`a` 为牵引者的 `ActorUniqueID`，调用 `Actor::setLeashHolder`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setInvisible` {#Entity.setInvisible}

```zig
pub fn setInvisible(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_INVISIBLE`：`a` 为 0 或 1，调用 `Actor::setInvisible`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setSneaking` {#Entity.setSneaking}

```zig
pub fn setSneaking(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_SNEAKING`：`a` 为 0 或 1，调用 `Actor::setSneaking`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setNameTagVisible` {#Entity.setNameTagVisible}

```zig
pub fn setNameTagVisible(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_NAME_TAG_VISIBLE`：`a` 为 0 或 1，调用 `Actor::setNameTagVisible`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setTarget` {#Entity.setTarget}

```zig
pub fn setTarget(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_TARGET`：`a` 为目标的 `ActorUniqueID`，调用 `Actor::setTarget`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setOwner` {#Entity.setOwner}

```zig
pub fn setOwner(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_OWNER`：`a` 为主人的 `ActorUniqueID`，调用 `Actor::setOwner`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.burn` {#Entity.burn}

```zig
pub fn burn(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_BURN`：`a` 为伤害值，调用 `Actor::burn`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.stopFire` {#Entity.stopFire}

```zig
pub fn stopFire(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_STOP_FIRE`：调用 `Actor::extinguishFire`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setVelocity` {#Entity.setVelocity}

```zig
pub fn setVelocity(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_VELOCITY`：`a`、`b`、`c` 为速度，调用 `Actor::setVelocity`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.applyImpulse` {#Entity.applyImpulse}

```zig
pub fn applyImpulse(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_APPLY_IMPULSE`：`a`、`b`、`c` 为冲量，调用 `Actor::applyImpulse`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setScoreTag` {#Entity.setScoreTag}

```zig
pub fn setScoreTag(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_SCORE_TAG`：`sarg` 为文本，调用 `Actor::setScoreTag`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setSkinId` {#Entity.setSkinId}

```zig
pub fn setSkinId(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_SKIN_ID`：`a` 为皮肤 id，调用 `Actor::setSkinID`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setStrength` {#Entity.setStrength}

```zig
pub fn setStrength(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_STRENGTH`：`a` 为强度，调用 `Actor::setStrength`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.removeAllPassengers` {#Entity.removeAllPassengers}

```zig
pub fn removeAllPassengers(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_REMOVE_ALL_PASSENGERS`：调用 `Actor::removeAllPassengers`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.executeEvent` {#Entity.executeEvent}

```zig
pub fn executeEvent(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_EXECUTE_EVENT`：`sarg` 为事件名，调用 `Actor::executeEvent`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.setRotation` {#Entity.setRotation}

```zig
pub fn setRotation(self: Entity, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_AACT_SET_ROTATION`：`a` 为俯仰角，`b` 为偏航角，调用 `Actor::setRotationWrapped`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)
