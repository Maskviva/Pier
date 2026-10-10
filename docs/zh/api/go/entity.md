# Go：实体

## 函数 {#functions}

### `ListActors` {#ListActors}

```go
func ListActors(dim int32) ([]ActorInfo, error)
```

列出一个维度里的所有实体。只能在服务器线程调用。

- 参数：
    - dim : `int32`
- 返回值类型：`([]ActorInfo, error)`
- 对应槽位：[`list_actors`](../cpp/entity.md#list_actors)

### `EntityByID` {#EntityByID}

```go
func EntityByID(id ActorID) Entity
```

这个唯一 id 的实体，id 就是事件载荷里的 uid。

- 参数：
    - id : `ActorID`
- 返回值类型：`Entity`

### `SpawnMob` {#SpawnMob}

```go
func SpawnMob(dim int32, typeName string, x, y, z float64) (Entity, error)
```

生成一个生物，并返回它。

- 参数：
    - dim : `int32`
    - typeName : `string`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- 返回值类型：`(Entity, error)`
- 对应槽位：[`spawn_mob`](../cpp/entity.md#spawn_mob)

## `Entity` {#Entity}

```go
type Entity struct {
    ID ActorID
}
```

一个实体，由它的唯一 id 指定。

### `Entity.Snapshot` {#Entity.Snapshot}

```go
func (e Entity) Snapshot() (string, error)
```

这个实体完整的 NBT，以 SNBT 给出。

- 返回值类型：`(string, error)`
- 对应槽位：[`actor_snapshot`](../cpp/entity.md#actor_snapshot)

### `Entity.Vehicle` {#Entity.Vehicle}

```go
func (e Entity) Vehicle() (Entity, error)
```

这个实体骑着的东西。

- 返回值类型：`(Entity, error)`
- 对应槽位：[`actor_get_vehicle`](../cpp/entity.md#actor_get_vehicle)

### `Entity.Owner` {#Entity.Owner}

```go
func (e Entity) Owner() (Entity, error)
```

这个实体的主人，用于驯服的或召唤出来的实体。

- 返回值类型：`(Entity, error)`
- 对应槽位：[`actor_get_owner`](../cpp/entity.md#actor_get_owner)

### `Entity.Target` {#Entity.Target}

```go
func (e Entity) Target() (Entity, error)
```

这个实体当前的目标。

- 返回值类型：`(Entity, error)`
- 对应槽位：[`actor_get_target`](../cpp/entity.md#actor_get_target)

### `Entity.DistanceTo` {#Entity.DistanceTo}

```go
func (e Entity) DistanceTo(other Entity) (float64, error)
```

到另一个实体的距离。

- 参数：
    - other : `Entity`
- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_distance_to`](../cpp/entity.md#actor_distance_to)

### `Entity.Clone` {#Entity.Clone}

```go
func (e Entity) Clone(dim int32, x, y, z float64) (Entity, error)
```

把这个实体复制到一个位置，返回复制出来的实体。

- 参数：
    - dim : `int32`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- 返回值类型：`(Entity, error)`
- 对应槽位：[`actor_clone`](../cpp/entity.md#actor_clone)

### `Entity.PosX` {#Entity.PosX}

```go
func (e Entity) PosX() (float64, error)
```

读取 `PIER_APROP_POS_X`：(G) `Actor::getPosition().x`（玩家的脚下位置要用 `getFeetPos`；`POS_*` 用的是 `getPosition`）

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.PosY` {#Entity.PosY}

```go
func (e Entity) PosY() (float64, error)
```

读取 `PIER_APROP_POS_Y`：(G)

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.PosZ` {#Entity.PosZ}

```go
func (e Entity) PosZ() (float64, error)
```

读取 `PIER_APROP_POS_Z`：(G)

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.RotPitch` {#Entity.RotPitch}

```go
func (e Entity) RotPitch() (float64, error)
```

读取 `PIER_APROP_ROT_PITCH`：(G) `Actor::getRotation().x`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.RotYaw` {#Entity.RotYaw}

```go
func (e Entity) RotYaw() (float64, error)
```

读取 `PIER_APROP_ROT_YAW`：(G) `Actor::getRotation().y`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.Dimension` {#Entity.Dimension}

```go
func (e Entity) Dimension() (float64, error)
```

读取 `PIER_APROP_DIMENSION`：(G) `Actor::getDimensionId`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.Health` {#Entity.Health}

```go
func (e Entity) Health() (float64, error)
```

读取 `PIER_APROP_HEALTH`：(G) `Actor::getHealth`；治疗和伤害用动作完成

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.MaxHealth` {#Entity.MaxHealth}

```go
func (e Entity) MaxHealth() (float64, error)
```

读取 `PIER_APROP_MAX_HEALTH`：(G) `Actor::getMaxHealth`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsAlive` {#Entity.IsAlive}

```go
func (e Entity) IsAlive() (bool, error)
```

读取 `PIER_APROP_IS_ALIVE`：(G) `Actor::isAlive`

- 返回值类型：`(bool, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsOnGround` {#Entity.IsOnGround}

```go
func (e Entity) IsOnGround() (bool, error)
```

读取 `PIER_APROP_IS_ON_GROUND`：(G) `Actor::isOnGround`

- 返回值类型：`(bool, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsInWater` {#Entity.IsInWater}

```go
func (e Entity) IsInWater() (bool, error)
```

读取 `PIER_APROP_IS_IN_WATER`：(G) `Actor::isInWater`

- 返回值类型：`(bool, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsInLava` {#Entity.IsInLava}

```go
func (e Entity) IsInLava() (bool, error)
```

读取 `PIER_APROP_IS_IN_LAVA`：(G) `Actor::isInLava`

- 返回值类型：`(bool, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsOnFire` {#Entity.IsOnFire}

```go
func (e Entity) IsOnFire() (bool, error)
```

读取 `PIER_APROP_IS_ON_FIRE`：(G) `Actor::isOnFire`

- 返回值类型：`(bool, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsInvisible` {#Entity.IsInvisible}

```go
func (e Entity) IsInvisible() (bool, error)
```

读取 `PIER_APROP_IS_INVISIBLE`：(G) `Actor::isInvisible`

- 返回值类型：`(bool, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsSneaking` {#Entity.IsSneaking}

```go
func (e Entity) IsSneaking() (bool, error)
```

读取 `PIER_APROP_IS_SNEAKING`：(G) `Actor::isSneaking`

- 返回值类型：`(bool, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsBaby` {#Entity.IsBaby}

```go
func (e Entity) IsBaby() (bool, error)
```

读取 `PIER_APROP_IS_BABY`：(G) `Actor::isBaby`

- 返回值类型：`(bool, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsRiding` {#Entity.IsRiding}

```go
func (e Entity) IsRiding() (bool, error)
```

读取 `PIER_APROP_IS_RIDING`：(G) `Actor::isRiding`

- 返回值类型：`(bool, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsTame` {#Entity.IsTame}

```go
func (e Entity) IsTame() (bool, error)
```

读取 `PIER_APROP_IS_TAME`：(G) `Actor::isTame`

- 返回值类型：`(bool, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.Speed` {#Entity.Speed}

```go
func (e Entity) Speed() (float64, error)
```

读取 `PIER_APROP_SPEED`：(G) `Actor::getSpeedInMetersPerSecond`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.ViewX` {#Entity.ViewX}

```go
func (e Entity) ViewX() (float64, error)
```

读取 `PIER_APROP_VIEW_X`：(G) `Actor::getViewVector().x`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.ViewY` {#Entity.ViewY}

```go
func (e Entity) ViewY() (float64, error)
```

读取 `PIER_APROP_VIEW_Y`：(G) `Actor::getViewVector().y`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.ViewZ` {#Entity.ViewZ}

```go
func (e Entity) ViewZ() (float64, error)
```

读取 `PIER_APROP_VIEW_Z`：(G) `Actor::getViewVector().z`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.VelX` {#Entity.VelX}

```go
func (e Entity) VelX() (float64, error)
```

读取 `PIER_APROP_VEL_X`：(G) `Actor::getVelocity().x`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.VelY` {#Entity.VelY}

```go
func (e Entity) VelY() (float64, error)
```

读取 `PIER_APROP_VEL_Y`：(G) `Actor::getVelocity().y`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.VelZ` {#Entity.VelZ}

```go
func (e Entity) VelZ() (float64, error)
```

读取 `PIER_APROP_VEL_Z`：(G) `Actor::getVelocity().z`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.HeadX` {#Entity.HeadX}

```go
func (e Entity) HeadX() (float64, error)
```

读取 `PIER_APROP_HEAD_X`：(G) `Actor::getHeadPos().x`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.HeadY` {#Entity.HeadY}

```go
func (e Entity) HeadY() (float64, error)
```

读取 `PIER_APROP_HEAD_Y`：(G) `Actor::getHeadPos().y`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.HeadZ` {#Entity.HeadZ}

```go
func (e Entity) HeadZ() (float64, error)
```

读取 `PIER_APROP_HEAD_Z`：(G) `Actor::getHeadPos().z`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.FeetX` {#Entity.FeetX}

```go
func (e Entity) FeetX() (float64, error)
```

读取 `PIER_APROP_FEET_X`：(G) `Actor::getFeetPos().x`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.FeetY` {#Entity.FeetY}

```go
func (e Entity) FeetY() (float64, error)
```

读取 `PIER_APROP_FEET_Y`：(G) `Actor::getFeetPos().y`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.FeetZ` {#Entity.FeetZ}

```go
func (e Entity) FeetZ() (float64, error)
```

读取 `PIER_APROP_FEET_Z`：(G) `Actor::getFeetPos().z`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.FallDistance` {#Entity.FallDistance}

```go
func (e Entity) FallDistance() (float64, error)
```

读取 `PIER_APROP_FALL_DISTANCE`：(G) `Actor::getFallDistance`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsPersistent` {#Entity.IsPersistent}

```go
func (e Entity) IsPersistent() (bool, error)
```

读取 `PIER_APROP_IS_PERSISTENT`：(G) `Actor::isPersistent`

- 返回值类型：`(bool, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsLeashed` {#Entity.IsLeashed}

```go
func (e Entity) IsLeashed() (bool, error)
```

读取 `PIER_APROP_IS_LEASHED`：(G) `Actor::isLeashed`

- 返回值类型：`(bool, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsInvulnerable` {#Entity.IsInvulnerable}

```go
func (e Entity) IsInvulnerable() (bool, error)
```

读取 `PIER_APROP_IS_INVULNERABLE`：(G) `Actor::isInvulnerable`

- 返回值类型：`(bool, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.Variant` {#Entity.Variant}

```go
func (e Entity) Variant() (float64, error)
```

读取 `PIER_APROP_VARIANT`：(G) `Actor::getVariant`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.MarkVariant` {#Entity.MarkVariant}

```go
func (e Entity) MarkVariant() (float64, error)
```

读取 `PIER_APROP_MARK_VARIANT`：(G) `Actor::getMarkVariant`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.Scale` {#Entity.Scale}

```go
func (e Entity) Scale() (float64, error)
```

读取 `PIER_APROP_SCALE`：(G) `Actor::getScaleFactor`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.Brightness` {#Entity.Brightness}

```go
func (e Entity) Brightness() (float64, error)
```

读取 `PIER_APROP_BRIGHTNESS`：(G) `Actor::getBrightness`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.Radius` {#Entity.Radius}

```go
func (e Entity) Radius() (float64, error)
```

读取 `PIER_APROP_RADIUS`：(G) `Actor::getRadius`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.HasTotem` {#Entity.HasTotem}

```go
func (e Entity) HasTotem() (bool, error)
```

读取 `PIER_APROP_HAS_TOTEM`：(G) `Actor::hasTotemEquipped`

- 返回值类型：`(bool, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsInRain` {#Entity.IsInRain}

```go
func (e Entity) IsInRain() (bool, error)
```

读取 `PIER_APROP_IS_IN_RAIN`：(G) `Actor::isInRain`

- 返回值类型：`(bool, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsInSnow` {#Entity.IsInSnow}

```go
func (e Entity) IsInSnow() (bool, error)
```

读取 `PIER_APROP_IS_IN_SNOW`：(G) `Actor::isInSnow`

- 返回值类型：`(bool, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsInThunderstorm` {#Entity.IsInThunderstorm}

```go
func (e Entity) IsInThunderstorm() (bool, error)
```

读取 `PIER_APROP_IS_IN_THUNDERSTORM`：(G) `Actor::isInThunderstorm`

- 返回值类型：`(bool, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsFrozen` {#Entity.IsFrozen}

```go
func (e Entity) IsFrozen() (bool, error)
```

读取 `PIER_APROP_IS_FROZEN`：(G) `Actor::isFrozen`

- 返回值类型：`(bool, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsInLove` {#Entity.IsInLove}

```go
func (e Entity) IsInLove() (bool, error)
```

读取 `PIER_APROP_IS_IN_LOVE`：(G) 从 BDS 1.26.40 起不再支持：`Actor::isInLove` 已被移除

在当前所有的引擎版本上，宿主读这一项都没有结果：(G) 从 BDS 1.26.40 起不再支持：`Actor::isInLove` 已被移除

- 返回值类型：`(bool, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.DeathTime` {#Entity.DeathTime}

```go
func (e Entity) DeathTime() (float64, error)
```

读取 `PIER_APROP_DEATH_TIME`：(G) `Actor::getDeathTime`

- 返回值类型：`(float64, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.HasPassenger` {#Entity.HasPassenger}

```go
func (e Entity) HasPassenger() (bool, error)
```

读取 `PIER_APROP_HAS_PASSENGER`：(G) `Actor::hasPassenger`

- 返回值类型：`(bool, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.TypeName` {#Entity.TypeName}

```go
func (e Entity) TypeName() (string, error)
```

读取 `PIER_ASTR_TYPE_NAME`：取自 `Actor::getTypeName`

- 返回值类型：`(string, error)`
- 对应槽位：[`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity.NameTag` {#Entity.NameTag}

```go
func (e Entity) NameTag() (string, error)
```

读取 `PIER_ASTR_NAME_TAG`：取自 `Actor::getNameTag`

- 返回值类型：`(string, error)`
- 对应槽位：[`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity.ScoreTag` {#Entity.ScoreTag}

```go
func (e Entity) ScoreTag() (string, error)
```

读取 `PIER_ASTR_SCORE_TAG`：取自 `Actor::getScoreTag`

- 返回值类型：`(string, error)`
- 对应槽位：[`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity.FilteredName` {#Entity.FilteredName}

```go
func (e Entity) FilteredName() (string, error)
```

读取 `PIER_ASTR_FILTERED_NAME`：取自 `Actor::getFilteredNameTag`

- 返回值类型：`(string, error)`
- 对应槽位：[`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity.Kill` {#Entity.Kill}

```go
func (e Entity) Kill(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_KILL`：调用 `Actor::kill`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.Despawn` {#Entity.Despawn}

```go
func (e Entity) Despawn(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_DESPAWN`：调用 `Actor::despawn`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.Heal` {#Entity.Heal}

```go
func (e Entity) Heal(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_HEAL`：`a` 为治疗量，调用 `Actor::heal`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetOnFire` {#Entity.SetOnFire}

```go
func (e Entity) SetOnFire(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_SET_ON_FIRE`：`a` 为秒数，调用 `Actor::setOnFire`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.Teleport` {#Entity.Teleport}

```go
func (e Entity) Teleport(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_TELEPORT`：`a`、`b`、`c` 为坐标，`sarg` 为维度（`"0"` 到 `"2"`），调用 `Actor::teleport`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetNameTag` {#Entity.SetNameTag}

```go
func (e Entity) SetNameTag(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_SET_NAME_TAG`：`sarg` 为名字，调用 `Actor::setNameTag`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.AddTag` {#Entity.AddTag}

```go
func (e Entity) AddTag(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_ADD_TAG`：`sarg` 为标签，输出 `"0"` 或 `"1"`，调用 `Actor::addTag`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.RemoveTag` {#Entity.RemoveTag}

```go
func (e Entity) RemoveTag(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_REMOVE_TAG`：`sarg` 为标签，输出 `"0"` 或 `"1"`，调用 `Actor::removeTag`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.HasTag` {#Entity.HasTag}

```go
func (e Entity) HasTag(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_HAS_TAG`：`sarg` 为标签，输出 `"0"` 或 `"1"`，调用 `Actor::hasTag`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.AddEffect` {#Entity.AddEffect}

```go
func (e Entity) AddEffect(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_ADD_EFFECT`：`sarg` 为效果名，`a` 为刻数，`b` 为效果等级，`c` 为是否显示粒子（0 或 1），经 `MobEffect::getByName` 和 `Actor::addEffect` 完成

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.RemoveEffect` {#Entity.RemoveEffect}

```go
func (e Entity) RemoveEffect(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_REMOVE_EFFECT`：`sarg` 为效果名，调用 `Actor::removeEffect(id)`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.ClearEffects` {#Entity.ClearEffects}

```go
func (e Entity) ClearEffects(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_CLEAR_EFFECTS`：调用 `Actor::removeAllEffects`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.Hurt` {#Entity.Hurt}

```go
func (e Entity) Hurt(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_HURT`：`a` 为伤害值（通用伤害来源），调用 `Actor::hurt`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.AttributeGet` {#Entity.AttributeGet}

```go
func (e Entity) AttributeGet(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_ATTRIBUTE_GET`：`sarg` 为属性名（`"minecraft:health"` 等），输出属性值

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetVariant` {#Entity.SetVariant}

```go
func (e Entity) SetVariant(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_SET_VARIANT`：`a` 为变种值，调用 `Actor::setVariant`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetMarkVariant` {#Entity.SetMarkVariant}

```go
func (e Entity) SetMarkVariant(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_SET_MARK_VARIANT`：`a` 为变种值，调用 `Actor::setMarkVariant`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetPersistent` {#Entity.SetPersistent}

```go
func (e Entity) SetPersistent(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_SET_PERSISTENT`：调用 `Actor::setPersistent`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetLeashHolder` {#Entity.SetLeashHolder}

```go
func (e Entity) SetLeashHolder(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_SET_LEASH_HOLDER`：`a` 为牵引者的 `ActorUniqueID`，调用 `Actor::setLeashHolder`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetInvisible` {#Entity.SetInvisible}

```go
func (e Entity) SetInvisible(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_SET_INVISIBLE`：`a` 为 0 或 1，调用 `Actor::setInvisible`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetSneaking` {#Entity.SetSneaking}

```go
func (e Entity) SetSneaking(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_SET_SNEAKING`：`a` 为 0 或 1，调用 `Actor::setSneaking`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetNameTagVisible` {#Entity.SetNameTagVisible}

```go
func (e Entity) SetNameTagVisible(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_SET_NAME_TAG_VISIBLE`：`a` 为 0 或 1，调用 `Actor::setNameTagVisible`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetTarget` {#Entity.SetTarget}

```go
func (e Entity) SetTarget(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_SET_TARGET`：`a` 为目标的 `ActorUniqueID`，调用 `Actor::setTarget`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetOwner` {#Entity.SetOwner}

```go
func (e Entity) SetOwner(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_SET_OWNER`：`a` 为主人的 `ActorUniqueID`，调用 `Actor::setOwner`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.Burn` {#Entity.Burn}

```go
func (e Entity) Burn(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_BURN`：`a` 为伤害值，调用 `Actor::burn`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.StopFire` {#Entity.StopFire}

```go
func (e Entity) StopFire(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_STOP_FIRE`：调用 `Actor::extinguishFire`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetVelocity` {#Entity.SetVelocity}

```go
func (e Entity) SetVelocity(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_SET_VELOCITY`：`a`、`b`、`c` 为速度，调用 `Actor::setVelocity`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.ApplyImpulse` {#Entity.ApplyImpulse}

```go
func (e Entity) ApplyImpulse(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_APPLY_IMPULSE`：`a`、`b`、`c` 为冲量，调用 `Actor::applyImpulse`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetScoreTag` {#Entity.SetScoreTag}

```go
func (e Entity) SetScoreTag(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_SET_SCORE_TAG`：`sarg` 为文本，调用 `Actor::setScoreTag`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetSkinId` {#Entity.SetSkinId}

```go
func (e Entity) SetSkinId(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_SET_SKIN_ID`：`a` 为皮肤 id，调用 `Actor::setSkinID`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetStrength` {#Entity.SetStrength}

```go
func (e Entity) SetStrength(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_SET_STRENGTH`：`a` 为强度，调用 `Actor::setStrength`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.RemoveAllPassengers` {#Entity.RemoveAllPassengers}

```go
func (e Entity) RemoveAllPassengers(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_REMOVE_ALL_PASSENGERS`：调用 `Actor::removeAllPassengers`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.ExecuteEvent` {#Entity.ExecuteEvent}

```go
func (e Entity) ExecuteEvent(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_EXECUTE_EVENT`：`sarg` 为事件名，调用 `Actor::executeEvent`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetRotation` {#Entity.SetRotation}

```go
func (e Entity) SetRotation(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_AACT_SET_ROTATION`：`a` 为俯仰角，`b` 为偏航角，调用 `Actor::setRotationWrapped`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

## `ActorInfo` {#ActorInfo}

```go
type ActorInfo struct {
    ID   ActorID
    Type string
}
```

一个维度里的一个实体：它的唯一 id 和类型名。

## `ActorID` {#ActorID}

```go
type ActorID int64
```

实体的唯一 id，即事件载荷里的 `uid`。0 永远解析不到。

## `AProp*` {#AProp}

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="APropPosX"></span>`APropPosX` | `0` | 即 `PIER_APROP_POS_X`：(G) `Actor::getPosition().x`（玩家的脚下位置要用 `getFeetPos`；`POS_*` 用的是 `getPosition`） |
| <span id="APropPosY"></span>`APropPosY` | `1` | 即 `PIER_APROP_POS_Y`：(G) |
| <span id="APropPosZ"></span>`APropPosZ` | `2` | 即 `PIER_APROP_POS_Z`：(G) |
| <span id="APropRotPitch"></span>`APropRotPitch` | `3` | 即 `PIER_APROP_ROT_PITCH`：(G) `Actor::getRotation().x` |
| <span id="APropRotYaw"></span>`APropRotYaw` | `4` | 即 `PIER_APROP_ROT_YAW`：(G) `Actor::getRotation().y` |
| <span id="APropDimension"></span>`APropDimension` | `5` | 即 `PIER_APROP_DIMENSION`：(G) `Actor::getDimensionId` |
| <span id="APropHealth"></span>`APropHealth` | `6` | 即 `PIER_APROP_HEALTH`：(G) `Actor::getHealth`；治疗和伤害用动作完成 |
| <span id="APropMaxHealth"></span>`APropMaxHealth` | `7` | 即 `PIER_APROP_MAX_HEALTH`：(G) `Actor::getMaxHealth` |
| <span id="APropIsAlive"></span>`APropIsAlive` | `8` | 即 `PIER_APROP_IS_ALIVE`：(G) `Actor::isAlive` |
| <span id="APropIsOnGround"></span>`APropIsOnGround` | `9` | 即 `PIER_APROP_IS_ON_GROUND`：(G) `Actor::isOnGround` |
| <span id="APropIsInWater"></span>`APropIsInWater` | `10` | 即 `PIER_APROP_IS_IN_WATER`：(G) `Actor::isInWater` |
| <span id="APropIsInLava"></span>`APropIsInLava` | `11` | 即 `PIER_APROP_IS_IN_LAVA`：(G) `Actor::isInLava` |
| <span id="APropIsOnFire"></span>`APropIsOnFire` | `12` | 即 `PIER_APROP_IS_ON_FIRE`：(G) `Actor::isOnFire` |
| <span id="APropIsInvisible"></span>`APropIsInvisible` | `13` | 即 `PIER_APROP_IS_INVISIBLE`：(G) `Actor::isInvisible` |
| <span id="APropIsSneaking"></span>`APropIsSneaking` | `14` | 即 `PIER_APROP_IS_SNEAKING`：(G) `Actor::isSneaking` |
| <span id="APropIsBaby"></span>`APropIsBaby` | `15` | 即 `PIER_APROP_IS_BABY`：(G) `Actor::isBaby` |
| <span id="APropIsRiding"></span>`APropIsRiding` | `16` | 即 `PIER_APROP_IS_RIDING`：(G) `Actor::isRiding` |
| <span id="APropIsTame"></span>`APropIsTame` | `17` | 即 `PIER_APROP_IS_TAME`：(G) `Actor::isTame` |
| <span id="APropSpeed"></span>`APropSpeed` | `18` | 即 `PIER_APROP_SPEED`：(G) `Actor::getSpeedInMetersPerSecond` |
| <span id="APropViewX"></span>`APropViewX` | `19` | 即 `PIER_APROP_VIEW_X`：(G) `Actor::getViewVector().x` |
| <span id="APropViewY"></span>`APropViewY` | `20` | 即 `PIER_APROP_VIEW_Y`：(G) `Actor::getViewVector().y` |
| <span id="APropViewZ"></span>`APropViewZ` | `21` | 即 `PIER_APROP_VIEW_Z`：(G) `Actor::getViewVector().z` |
| <span id="APropVelX"></span>`APropVelX` | `22` | 即 `PIER_APROP_VEL_X`：(G) `Actor::getVelocity().x` |
| <span id="APropVelY"></span>`APropVelY` | `23` | 即 `PIER_APROP_VEL_Y`：(G) `Actor::getVelocity().y` |
| <span id="APropVelZ"></span>`APropVelZ` | `24` | 即 `PIER_APROP_VEL_Z`：(G) `Actor::getVelocity().z` |
| <span id="APropHeadX"></span>`APropHeadX` | `25` | 即 `PIER_APROP_HEAD_X`：(G) `Actor::getHeadPos().x` |
| <span id="APropHeadY"></span>`APropHeadY` | `26` | 即 `PIER_APROP_HEAD_Y`：(G) `Actor::getHeadPos().y` |
| <span id="APropHeadZ"></span>`APropHeadZ` | `27` | 即 `PIER_APROP_HEAD_Z`：(G) `Actor::getHeadPos().z` |
| <span id="APropFeetX"></span>`APropFeetX` | `28` | 即 `PIER_APROP_FEET_X`：(G) `Actor::getFeetPos().x` |
| <span id="APropFeetY"></span>`APropFeetY` | `29` | 即 `PIER_APROP_FEET_Y`：(G) `Actor::getFeetPos().y` |
| <span id="APropFeetZ"></span>`APropFeetZ` | `30` | 即 `PIER_APROP_FEET_Z`：(G) `Actor::getFeetPos().z` |
| <span id="APropFallDistance"></span>`APropFallDistance` | `31` | 即 `PIER_APROP_FALL_DISTANCE`：(G) `Actor::getFallDistance` |
| <span id="APropIsPersistent"></span>`APropIsPersistent` | `32` | 即 `PIER_APROP_IS_PERSISTENT`：(G) `Actor::isPersistent` |
| <span id="APropIsLeashed"></span>`APropIsLeashed` | `33` | 即 `PIER_APROP_IS_LEASHED`：(G) `Actor::isLeashed` |
| <span id="APropIsInvulnerable"></span>`APropIsInvulnerable` | `34` | 即 `PIER_APROP_IS_INVULNERABLE`：(G) `Actor::isInvulnerable` |
| <span id="APropVariant"></span>`APropVariant` | `35` | 即 `PIER_APROP_VARIANT`：(G) `Actor::getVariant` |
| <span id="APropMarkVariant"></span>`APropMarkVariant` | `36` | 即 `PIER_APROP_MARK_VARIANT`：(G) `Actor::getMarkVariant` |
| <span id="APropScale"></span>`APropScale` | `37` | 即 `PIER_APROP_SCALE`：(G) `Actor::getScaleFactor` |
| <span id="APropBrightness"></span>`APropBrightness` | `38` | 即 `PIER_APROP_BRIGHTNESS`：(G) `Actor::getBrightness` |
| <span id="APropRadius"></span>`APropRadius` | `39` | 即 `PIER_APROP_RADIUS`：(G) `Actor::getRadius` |
| <span id="APropHasTotem"></span>`APropHasTotem` | `40` | 即 `PIER_APROP_HAS_TOTEM`：(G) `Actor::hasTotemEquipped` |
| <span id="APropIsInRain"></span>`APropIsInRain` | `41` | 即 `PIER_APROP_IS_IN_RAIN`：(G) `Actor::isInRain` |
| <span id="APropIsInSnow"></span>`APropIsInSnow` | `42` | 即 `PIER_APROP_IS_IN_SNOW`：(G) `Actor::isInSnow` |
| <span id="APropIsInThunderstorm"></span>`APropIsInThunderstorm` | `43` | 即 `PIER_APROP_IS_IN_THUNDERSTORM`：(G) `Actor::isInThunderstorm` |
| <span id="APropIsFrozen"></span>`APropIsFrozen` | `44` | 即 `PIER_APROP_IS_FROZEN`：(G) `Actor::isFrozen` |
| <span id="APropIsInLove"></span>`APropIsInLove` | `45` | **这个常量在当前的引擎版本上取不到值**：即 `PIER_APROP_IS_IN_LOVE`：(G) 从 BDS 1.26.40 起不再支持：`Actor::isInLove` 已被移除 |
| <span id="APropDeathTime"></span>`APropDeathTime` | `46` | 即 `PIER_APROP_DEATH_TIME`：(G) `Actor::getDeathTime` |
| <span id="APropHasPassenger"></span>`APropHasPassenger` | `47` | 即 `PIER_APROP_HAS_PASSENGER`：(G) `Actor::hasPassenger` |

## `AStr*` {#AStr}

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="AStrTypeName"></span>`AStrTypeName` | `0` | 即 `PIER_ASTR_TYPE_NAME`：取自 `Actor::getTypeName` |
| <span id="AStrNameTag"></span>`AStrNameTag` | `1` | 即 `PIER_ASTR_NAME_TAG`：取自 `Actor::getNameTag` |
| <span id="AStrScoreTag"></span>`AStrScoreTag` | `2` | 即 `PIER_ASTR_SCORE_TAG`：取自 `Actor::getScoreTag` |
| <span id="AStrFilteredName"></span>`AStrFilteredName` | `3` | 即 `PIER_ASTR_FILTERED_NAME`：取自 `Actor::getFilteredNameTag` |

## `AAct*` {#AAct}

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="AActKill"></span>`AActKill` | `0` | 即 `PIER_AACT_KILL`：调用 `Actor::kill` |
| <span id="AActDespawn"></span>`AActDespawn` | `1` | 即 `PIER_AACT_DESPAWN`：调用 `Actor::despawn` |
| <span id="AActHeal"></span>`AActHeal` | `2` | 即 `PIER_AACT_HEAL`：`a` 为治疗量，调用 `Actor::heal` |
| <span id="AActSetOnFire"></span>`AActSetOnFire` | `3` | 即 `PIER_AACT_SET_ON_FIRE`：`a` 为秒数，调用 `Actor::setOnFire` |
| <span id="AActTeleport"></span>`AActTeleport` | `4` | 即 `PIER_AACT_TELEPORT`：`a`、`b`、`c` 为坐标，`sarg` 为维度（`"0"` 到 `"2"`），调用 `Actor::teleport` |
| <span id="AActSetNameTag"></span>`AActSetNameTag` | `5` | 即 `PIER_AACT_SET_NAME_TAG`：`sarg` 为名字，调用 `Actor::setNameTag` |
| <span id="AActAddTag"></span>`AActAddTag` | `6` | 即 `PIER_AACT_ADD_TAG`：`sarg` 为标签，输出 `"0"` 或 `"1"`，调用 `Actor::addTag` |
| <span id="AActRemoveTag"></span>`AActRemoveTag` | `7` | 即 `PIER_AACT_REMOVE_TAG`：`sarg` 为标签，输出 `"0"` 或 `"1"`，调用 `Actor::removeTag` |
| <span id="AActHasTag"></span>`AActHasTag` | `8` | 即 `PIER_AACT_HAS_TAG`：`sarg` 为标签，输出 `"0"` 或 `"1"`，调用 `Actor::hasTag` |
| <span id="AActAddEffect"></span>`AActAddEffect` | `9` | 即 `PIER_AACT_ADD_EFFECT`：`sarg` 为效果名，`a` 为刻数，`b` 为效果等级，`c` 为是否显示粒子（0 或 1），经 `MobEffect::getByName` 和 `Actor::addEffect` 完成 |
| <span id="AActRemoveEffect"></span>`AActRemoveEffect` | `10` | 即 `PIER_AACT_REMOVE_EFFECT`：`sarg` 为效果名，调用 `Actor::removeEffect(id)` |
| <span id="AActClearEffects"></span>`AActClearEffects` | `11` | 即 `PIER_AACT_CLEAR_EFFECTS`：调用 `Actor::removeAllEffects` |
| <span id="AActHurt"></span>`AActHurt` | `12` | 即 `PIER_AACT_HURT`：`a` 为伤害值（通用伤害来源），调用 `Actor::hurt` |
| <span id="AActAttributeGet"></span>`AActAttributeGet` | `13` | 即 `PIER_AACT_ATTRIBUTE_GET`：`sarg` 为属性名（`"minecraft:health"` 等），输出属性值 |
| <span id="AActSetVariant"></span>`AActSetVariant` | `14` | 即 `PIER_AACT_SET_VARIANT`：`a` 为变种值，调用 `Actor::setVariant` |
| <span id="AActSetMarkVariant"></span>`AActSetMarkVariant` | `15` | 即 `PIER_AACT_SET_MARK_VARIANT`：`a` 为变种值，调用 `Actor::setMarkVariant` |
| <span id="AActSetPersistent"></span>`AActSetPersistent` | `16` | 即 `PIER_AACT_SET_PERSISTENT`：调用 `Actor::setPersistent` |
| <span id="AActSetLeashHolder"></span>`AActSetLeashHolder` | `17` | 即 `PIER_AACT_SET_LEASH_HOLDER`：`a` 为牵引者的 `ActorUniqueID`，调用 `Actor::setLeashHolder` |
| <span id="AActSetInvisible"></span>`AActSetInvisible` | `18` | 即 `PIER_AACT_SET_INVISIBLE`：`a` 为 0 或 1，调用 `Actor::setInvisible` |
| <span id="AActSetSneaking"></span>`AActSetSneaking` | `19` | 即 `PIER_AACT_SET_SNEAKING`：`a` 为 0 或 1，调用 `Actor::setSneaking` |
| <span id="AActSetNameTagVisible"></span>`AActSetNameTagVisible` | `20` | 即 `PIER_AACT_SET_NAME_TAG_VISIBLE`：`a` 为 0 或 1，调用 `Actor::setNameTagVisible` |
| <span id="AActSetTarget"></span>`AActSetTarget` | `21` | 即 `PIER_AACT_SET_TARGET`：`a` 为目标的 `ActorUniqueID`，调用 `Actor::setTarget` |
| <span id="AActSetOwner"></span>`AActSetOwner` | `22` | 即 `PIER_AACT_SET_OWNER`：`a` 为主人的 `ActorUniqueID`，调用 `Actor::setOwner` |
| <span id="AActBurn"></span>`AActBurn` | `23` | 即 `PIER_AACT_BURN`：`a` 为伤害值，调用 `Actor::burn` |
| <span id="AActStopFire"></span>`AActStopFire` | `24` | 即 `PIER_AACT_STOP_FIRE`：调用 `Actor::extinguishFire` |
| <span id="AActSetVelocity"></span>`AActSetVelocity` | `25` | 即 `PIER_AACT_SET_VELOCITY`：`a`、`b`、`c` 为速度，调用 `Actor::setVelocity` |
| <span id="AActApplyImpulse"></span>`AActApplyImpulse` | `26` | 即 `PIER_AACT_APPLY_IMPULSE`：`a`、`b`、`c` 为冲量，调用 `Actor::applyImpulse` |
| <span id="AActSetScoreTag"></span>`AActSetScoreTag` | `27` | 即 `PIER_AACT_SET_SCORE_TAG`：`sarg` 为文本，调用 `Actor::setScoreTag` |
| <span id="AActSetSkinId"></span>`AActSetSkinId` | `28` | 即 `PIER_AACT_SET_SKIN_ID`：`a` 为皮肤 id，调用 `Actor::setSkinID` |
| <span id="AActSetStrength"></span>`AActSetStrength` | `29` | 即 `PIER_AACT_SET_STRENGTH`：`a` 为强度，调用 `Actor::setStrength` |
| <span id="AActRemoveAllPassengers"></span>`AActRemoveAllPassengers` | `30` | 即 `PIER_AACT_REMOVE_ALL_PASSENGERS`：调用 `Actor::removeAllPassengers` |
| <span id="AActExecuteEvent"></span>`AActExecuteEvent` | `31` | 即 `PIER_AACT_EXECUTE_EVENT`：`sarg` 为事件名，调用 `Actor::executeEvent` |
| <span id="AActSetRotation"></span>`AActSetRotation` | `32` | 即 `PIER_AACT_SET_ROTATION`：`a` 为俯仰角，`b` 为偏航角，调用 `Actor::setRotationWrapped` |
