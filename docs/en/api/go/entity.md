# Go: Actors

## Functions {#functions}

### `ListActors` {#ListActors}

```go
func ListActors(dim int32) ([]ActorInfo, error)
```

ListActors lists every actor in a dimension. Server thread only.

- Parameters:
    - dim : `int32`
- Return type: `([]ActorInfo, error)`
- Slots: [`list_actors`](../cpp/entity.md#list_actors)

### `EntityByID` {#EntityByID}

```go
func EntityByID(id ActorID) Entity
```

EntityByID is the actor with this unique id, the uid of event payloads.

- Parameters:
    - id : `ActorID`
- Return type: `Entity`

### `SpawnMob` {#SpawnMob}

```go
func SpawnMob(dim int32, typeName string, x, y, z float64) (Entity, error)
```

SpawnMob spawns a mob and returns it.

- Parameters:
    - dim : `int32`
    - typeName : `string`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- Return type: `(Entity, error)`
- Slots: [`spawn_mob`](../cpp/entity.md#spawn_mob)

## `Entity` {#Entity}

```go
type Entity struct {
    ID ActorID
}
```

Entity is one actor, named by its unique id.

### `Entity.Snapshot` {#Entity.Snapshot}

```go
func (e Entity) Snapshot() (string, error)
```

Snapshot is the actor's full NBT as SNBT.

- Return type: `(string, error)`
- Slots: [`actor_snapshot`](../cpp/entity.md#actor_snapshot)

### `Entity.Vehicle` {#Entity.Vehicle}

```go
func (e Entity) Vehicle() (Entity, error)
```

Vehicle is what the actor rides.

- Return type: `(Entity, error)`
- Slots: [`actor_get_vehicle`](../cpp/entity.md#actor_get_vehicle)

### `Entity.Owner` {#Entity.Owner}

```go
func (e Entity) Owner() (Entity, error)
```

Owner is the actor's owner, for a tamed or summoned one.

- Return type: `(Entity, error)`
- Slots: [`actor_get_owner`](../cpp/entity.md#actor_get_owner)

### `Entity.Target` {#Entity.Target}

```go
func (e Entity) Target() (Entity, error)
```

Target is the actor's current target.

- Return type: `(Entity, error)`
- Slots: [`actor_get_target`](../cpp/entity.md#actor_get_target)

### `Entity.DistanceTo` {#Entity.DistanceTo}

```go
func (e Entity) DistanceTo(other Entity) (float64, error)
```

DistanceTo is the distance to another actor.

- Parameters:
    - other : `Entity`
- Return type: `(float64, error)`
- Slots: [`actor_distance_to`](../cpp/entity.md#actor_distance_to)

### `Entity.Clone` {#Entity.Clone}

```go
func (e Entity) Clone(dim int32, x, y, z float64) (Entity, error)
```

Clone copies the actor to a position and returns the copy.

- Parameters:
    - dim : `int32`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- Return type: `(Entity, error)`
- Slots: [`actor_clone`](../cpp/entity.md#actor_clone)

### `Entity.PosX` {#Entity.PosX}

```go
func (e Entity) PosX() (float64, error)
```

PosX reads `PIER_APROP_POS_X`: (G) `Actor::getPosition()`.x (feet: getFeetPos for players; POS\_\* uses getPosition)

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.PosY` {#Entity.PosY}

```go
func (e Entity) PosY() (float64, error)
```

PosY reads `PIER_APROP_POS_Y`: (G)

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.PosZ` {#Entity.PosZ}

```go
func (e Entity) PosZ() (float64, error)
```

PosZ reads `PIER_APROP_POS_Z`: (G)

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.RotPitch` {#Entity.RotPitch}

```go
func (e Entity) RotPitch() (float64, error)
```

RotPitch reads `PIER_APROP_ROT_PITCH`: (G) `Actor::getRotation()`.x

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.RotYaw` {#Entity.RotYaw}

```go
func (e Entity) RotYaw() (float64, error)
```

RotYaw reads `PIER_APROP_ROT_YAW`: (G) `Actor::getRotation()`.y

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.Dimension` {#Entity.Dimension}

```go
func (e Entity) Dimension() (float64, error)
```

Dimension reads `PIER_APROP_DIMENSION`: (G) `Actor::getDimensionId`

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.Health` {#Entity.Health}

```go
func (e Entity) Health() (float64, error)
```

Health reads `PIER_APROP_HEALTH`: (G) `Actor::getHealth`; heal/hurt via actions

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.MaxHealth` {#Entity.MaxHealth}

```go
func (e Entity) MaxHealth() (float64, error)
```

MaxHealth reads `PIER_APROP_MAX_HEALTH`: (G) `Actor::getMaxHealth`

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsAlive` {#Entity.IsAlive}

```go
func (e Entity) IsAlive() (bool, error)
```

IsAlive reads `PIER_APROP_IS_ALIVE`: (G) `Actor::isAlive`

- Return type: `(bool, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsOnGround` {#Entity.IsOnGround}

```go
func (e Entity) IsOnGround() (bool, error)
```

IsOnGround reads `PIER_APROP_IS_ON_GROUND`: (G) `Actor::isOnGround`

- Return type: `(bool, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsInWater` {#Entity.IsInWater}

```go
func (e Entity) IsInWater() (bool, error)
```

IsInWater reads `PIER_APROP_IS_IN_WATER`: (G) `Actor::isInWater`

- Return type: `(bool, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsInLava` {#Entity.IsInLava}

```go
func (e Entity) IsInLava() (bool, error)
```

IsInLava reads `PIER_APROP_IS_IN_LAVA`: (G) `Actor::isInLava`

- Return type: `(bool, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsOnFire` {#Entity.IsOnFire}

```go
func (e Entity) IsOnFire() (bool, error)
```

IsOnFire reads `PIER_APROP_IS_ON_FIRE`: (G) `Actor::isOnFire`

- Return type: `(bool, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsInvisible` {#Entity.IsInvisible}

```go
func (e Entity) IsInvisible() (bool, error)
```

IsInvisible reads `PIER_APROP_IS_INVISIBLE`: (G) `Actor::isInvisible`

- Return type: `(bool, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsSneaking` {#Entity.IsSneaking}

```go
func (e Entity) IsSneaking() (bool, error)
```

IsSneaking reads `PIER_APROP_IS_SNEAKING`: (G) `Actor::isSneaking`

- Return type: `(bool, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsBaby` {#Entity.IsBaby}

```go
func (e Entity) IsBaby() (bool, error)
```

IsBaby reads `PIER_APROP_IS_BABY`: (G) `Actor::isBaby`

- Return type: `(bool, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsRiding` {#Entity.IsRiding}

```go
func (e Entity) IsRiding() (bool, error)
```

IsRiding reads `PIER_APROP_IS_RIDING`: (G) `Actor::isRiding`

- Return type: `(bool, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsTame` {#Entity.IsTame}

```go
func (e Entity) IsTame() (bool, error)
```

IsTame reads `PIER_APROP_IS_TAME`: (G) `Actor::isTame`

- Return type: `(bool, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.Speed` {#Entity.Speed}

```go
func (e Entity) Speed() (float64, error)
```

Speed reads `PIER_APROP_SPEED`: (G) `Actor::getSpeedInMetersPerSecond`

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.ViewX` {#Entity.ViewX}

```go
func (e Entity) ViewX() (float64, error)
```

ViewX reads `PIER_APROP_VIEW_X`: (G) `Actor::getViewVector()`.x

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.ViewY` {#Entity.ViewY}

```go
func (e Entity) ViewY() (float64, error)
```

ViewY reads `PIER_APROP_VIEW_Y`: (G) `Actor::getViewVector()`.y

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.ViewZ` {#Entity.ViewZ}

```go
func (e Entity) ViewZ() (float64, error)
```

ViewZ reads `PIER_APROP_VIEW_Z`: (G) `Actor::getViewVector()`.z

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.VelX` {#Entity.VelX}

```go
func (e Entity) VelX() (float64, error)
```

VelX reads `PIER_APROP_VEL_X`: (G) `Actor::getVelocity()`.x

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.VelY` {#Entity.VelY}

```go
func (e Entity) VelY() (float64, error)
```

VelY reads `PIER_APROP_VEL_Y`: (G) `Actor::getVelocity()`.y

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.VelZ` {#Entity.VelZ}

```go
func (e Entity) VelZ() (float64, error)
```

VelZ reads `PIER_APROP_VEL_Z`: (G) `Actor::getVelocity()`.z

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.HeadX` {#Entity.HeadX}

```go
func (e Entity) HeadX() (float64, error)
```

HeadX reads `PIER_APROP_HEAD_X`: (G) `Actor::getHeadPos()`.x

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.HeadY` {#Entity.HeadY}

```go
func (e Entity) HeadY() (float64, error)
```

HeadY reads `PIER_APROP_HEAD_Y`: (G) `Actor::getHeadPos()`.y

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.HeadZ` {#Entity.HeadZ}

```go
func (e Entity) HeadZ() (float64, error)
```

HeadZ reads `PIER_APROP_HEAD_Z`: (G) `Actor::getHeadPos()`.z

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.FeetX` {#Entity.FeetX}

```go
func (e Entity) FeetX() (float64, error)
```

FeetX reads `PIER_APROP_FEET_X`: (G) `Actor::getFeetPos()`.x

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.FeetY` {#Entity.FeetY}

```go
func (e Entity) FeetY() (float64, error)
```

FeetY reads `PIER_APROP_FEET_Y`: (G) `Actor::getFeetPos()`.y

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.FeetZ` {#Entity.FeetZ}

```go
func (e Entity) FeetZ() (float64, error)
```

FeetZ reads `PIER_APROP_FEET_Z`: (G) `Actor::getFeetPos()`.z

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.FallDistance` {#Entity.FallDistance}

```go
func (e Entity) FallDistance() (float64, error)
```

FallDistance reads `PIER_APROP_FALL_DISTANCE`: (G) `Actor::getFallDistance`

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsPersistent` {#Entity.IsPersistent}

```go
func (e Entity) IsPersistent() (bool, error)
```

IsPersistent reads `PIER_APROP_IS_PERSISTENT`: (G) `Actor::isPersistent`

- Return type: `(bool, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsLeashed` {#Entity.IsLeashed}

```go
func (e Entity) IsLeashed() (bool, error)
```

IsLeashed reads `PIER_APROP_IS_LEASHED`: (G) `Actor::isLeashed`

- Return type: `(bool, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsInvulnerable` {#Entity.IsInvulnerable}

```go
func (e Entity) IsInvulnerable() (bool, error)
```

IsInvulnerable reads `PIER_APROP_IS_INVULNERABLE`: (G) `Actor::isInvulnerable`

- Return type: `(bool, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.Variant` {#Entity.Variant}

```go
func (e Entity) Variant() (float64, error)
```

Variant reads `PIER_APROP_VARIANT`: (G) `Actor::getVariant`

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.MarkVariant` {#Entity.MarkVariant}

```go
func (e Entity) MarkVariant() (float64, error)
```

MarkVariant reads `PIER_APROP_MARK_VARIANT`: (G) `Actor::getMarkVariant`

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.Scale` {#Entity.Scale}

```go
func (e Entity) Scale() (float64, error)
```

Scale reads `PIER_APROP_SCALE`: (G) `Actor::getScaleFactor`

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.Brightness` {#Entity.Brightness}

```go
func (e Entity) Brightness() (float64, error)
```

Brightness reads `PIER_APROP_BRIGHTNESS`: (G) `Actor::getBrightness`

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.Radius` {#Entity.Radius}

```go
func (e Entity) Radius() (float64, error)
```

Radius reads `PIER_APROP_RADIUS`: (G) `Actor::getRadius`

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.HasTotem` {#Entity.HasTotem}

```go
func (e Entity) HasTotem() (bool, error)
```

HasTotem reads `PIER_APROP_HAS_TOTEM`: (G) `Actor::hasTotemEquipped`

- Return type: `(bool, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsInRain` {#Entity.IsInRain}

```go
func (e Entity) IsInRain() (bool, error)
```

IsInRain reads `PIER_APROP_IS_IN_RAIN`: (G) `Actor::isInRain`

- Return type: `(bool, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsInSnow` {#Entity.IsInSnow}

```go
func (e Entity) IsInSnow() (bool, error)
```

IsInSnow reads `PIER_APROP_IS_IN_SNOW`: (G) `Actor::isInSnow`

- Return type: `(bool, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsInThunderstorm` {#Entity.IsInThunderstorm}

```go
func (e Entity) IsInThunderstorm() (bool, error)
```

IsInThunderstorm reads `PIER_APROP_IS_IN_THUNDERSTORM`: (G) `Actor::isInThunderstorm`

- Return type: `(bool, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsFrozen` {#Entity.IsFrozen}

```go
func (e Entity) IsFrozen() (bool, error)
```

IsFrozen reads `PIER_APROP_IS_FROZEN`: (G) `Actor::isFrozen`

- Return type: `(bool, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.IsInLove` {#Entity.IsInLove}

```go
func (e Entity) IsInLove() (bool, error)
```

IsInLove reads `PIER_APROP_IS_IN_LOVE`: (G) unsupported since BDS 1.26.40: `Actor::isInLove` is gone

The host answers no on every current engine version: (G) unsupported since BDS 1.26.40: `Actor::isInLove` is gone

- Return type: `(bool, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.DeathTime` {#Entity.DeathTime}

```go
func (e Entity) DeathTime() (float64, error)
```

DeathTime reads `PIER_APROP_DEATH_TIME`: (G) `Actor::getDeathTime`

- Return type: `(float64, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.HasPassenger` {#Entity.HasPassenger}

```go
func (e Entity) HasPassenger() (bool, error)
```

HasPassenger reads `PIER_APROP_HAS_PASSENGER`: (G) `Actor::hasPassenger`

- Return type: `(bool, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity.TypeName` {#Entity.TypeName}

```go
func (e Entity) TypeName() (string, error)
```

TypeName reads `PIER_ASTR_TYPE_NAME`: `Actor::getTypeName`

- Return type: `(string, error)`
- Slots: [`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity.NameTag` {#Entity.NameTag}

```go
func (e Entity) NameTag() (string, error)
```

NameTag reads `PIER_ASTR_NAME_TAG`: `Actor::getNameTag`

- Return type: `(string, error)`
- Slots: [`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity.ScoreTag` {#Entity.ScoreTag}

```go
func (e Entity) ScoreTag() (string, error)
```

ScoreTag reads `PIER_ASTR_SCORE_TAG`: `Actor::getScoreTag`

- Return type: `(string, error)`
- Slots: [`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity.FilteredName` {#Entity.FilteredName}

```go
func (e Entity) FilteredName() (string, error)
```

FilteredName reads `PIER_ASTR_FILTERED_NAME`: `Actor::getFilteredNameTag`

- Return type: `(string, error)`
- Slots: [`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity.Kill` {#Entity.Kill}

```go
func (e Entity) Kill(sarg string, a, b, c float64) (string, error)
```

Kill runs `PIER_AACT_KILL`: `Actor::kill`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.Despawn` {#Entity.Despawn}

```go
func (e Entity) Despawn(sarg string, a, b, c float64) (string, error)
```

Despawn runs `PIER_AACT_DESPAWN`: `Actor::despawn`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.Heal` {#Entity.Heal}

```go
func (e Entity) Heal(sarg string, a, b, c float64) (string, error)
```

Heal runs `PIER_AACT_HEAL`: a=amount `Actor::heal`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetOnFire` {#Entity.SetOnFire}

```go
func (e Entity) SetOnFire(sarg string, a, b, c float64) (string, error)
```

SetOnFire runs `PIER_AACT_SET_ON_FIRE`: a=seconds `Actor::setOnFire`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.Teleport` {#Entity.Teleport}

```go
func (e Entity) Teleport(sarg string, a, b, c float64) (string, error)
```

Teleport runs `PIER_AACT_TELEPORT`: a,b,c=pos, sarg=dim ("0".."2") `Actor::teleport`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetNameTag` {#Entity.SetNameTag}

```go
func (e Entity) SetNameTag(sarg string, a, b, c float64) (string, error)
```

SetNameTag runs `PIER_AACT_SET_NAME_TAG`: sarg=name `Actor::setNameTag`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.AddTag` {#Entity.AddTag}

```go
func (e Entity) AddTag(sarg string, a, b, c float64) (string, error)
```

AddTag runs `PIER_AACT_ADD_TAG`: sarg=tag → out "0"/"1" `Actor::addTag`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.RemoveTag` {#Entity.RemoveTag}

```go
func (e Entity) RemoveTag(sarg string, a, b, c float64) (string, error)
```

RemoveTag runs `PIER_AACT_REMOVE_TAG`: sarg=tag → out "0"/"1" `Actor::removeTag`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.HasTag` {#Entity.HasTag}

```go
func (e Entity) HasTag(sarg string, a, b, c float64) (string, error)
```

HasTag runs `PIER_AACT_HAS_TAG`: sarg=tag → out "0"/"1" `Actor::hasTag`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.AddEffect` {#Entity.AddEffect}

```go
func (e Entity) AddEffect(sarg string, a, b, c float64) (string, error)
```

AddEffect runs `PIER_AACT_ADD_EFFECT`: sarg=effect name, a=ticks, b=amplifier, c=visible(0/1) `MobEffect::getByName` + `Actor::addEffect`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.RemoveEffect` {#Entity.RemoveEffect}

```go
func (e Entity) RemoveEffect(sarg string, a, b, c float64) (string, error)
```

RemoveEffect runs `PIER_AACT_REMOVE_EFFECT`: sarg=effect name `Actor::removeEffect`(id)

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.ClearEffects` {#Entity.ClearEffects}

```go
func (e Entity) ClearEffects(sarg string, a, b, c float64) (string, error)
```

ClearEffects runs `PIER_AACT_CLEAR_EFFECTS`: `Actor::removeAllEffects`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.Hurt` {#Entity.Hurt}

```go
func (e Entity) Hurt(sarg string, a, b, c float64) (string, error)
```

Hurt runs `PIER_AACT_HURT`: a=damage (generic damage source) `Actor::hurt`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.AttributeGet` {#Entity.AttributeGet}

```go
func (e Entity) AttributeGet(sarg string, a, b, c float64) (string, error)
```

AttributeGet runs `PIER_AACT_ATTRIBUTE_GET`: sarg=attribute name ("minecraft:health"…) → out value

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetVariant` {#Entity.SetVariant}

```go
func (e Entity) SetVariant(sarg string, a, b, c float64) (string, error)
```

SetVariant runs `PIER_AACT_SET_VARIANT`: a=variant `Actor::setVariant`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetMarkVariant` {#Entity.SetMarkVariant}

```go
func (e Entity) SetMarkVariant(sarg string, a, b, c float64) (string, error)
```

SetMarkVariant runs `PIER_AACT_SET_MARK_VARIANT`: a=variant `Actor::setMarkVariant`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetPersistent` {#Entity.SetPersistent}

```go
func (e Entity) SetPersistent(sarg string, a, b, c float64) (string, error)
```

SetPersistent runs `PIER_AACT_SET_PERSISTENT`: `Actor::setPersistent`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetLeashHolder` {#Entity.SetLeashHolder}

```go
func (e Entity) SetLeashHolder(sarg string, a, b, c float64) (string, error)
```

SetLeashHolder runs `PIER_AACT_SET_LEASH_HOLDER`: a=holder ActorUniqueID `Actor::setLeashHolder`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetInvisible` {#Entity.SetInvisible}

```go
func (e Entity) SetInvisible(sarg string, a, b, c float64) (string, error)
```

SetInvisible runs `PIER_AACT_SET_INVISIBLE`: a=0/1 `Actor::setInvisible`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetSneaking` {#Entity.SetSneaking}

```go
func (e Entity) SetSneaking(sarg string, a, b, c float64) (string, error)
```

SetSneaking runs `PIER_AACT_SET_SNEAKING`: a=0/1 `Actor::setSneaking`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetNameTagVisible` {#Entity.SetNameTagVisible}

```go
func (e Entity) SetNameTagVisible(sarg string, a, b, c float64) (string, error)
```

SetNameTagVisible runs `PIER_AACT_SET_NAME_TAG_VISIBLE`: a=0/1 `Actor::setNameTagVisible`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetTarget` {#Entity.SetTarget}

```go
func (e Entity) SetTarget(sarg string, a, b, c float64) (string, error)
```

SetTarget runs `PIER_AACT_SET_TARGET`: a=target ActorUniqueID `Actor::setTarget`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetOwner` {#Entity.SetOwner}

```go
func (e Entity) SetOwner(sarg string, a, b, c float64) (string, error)
```

SetOwner runs `PIER_AACT_SET_OWNER`: a=owner ActorUniqueID `Actor::setOwner`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.Burn` {#Entity.Burn}

```go
func (e Entity) Burn(sarg string, a, b, c float64) (string, error)
```

Burn runs `PIER_AACT_BURN`: a=damage `Actor::burn`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.StopFire` {#Entity.StopFire}

```go
func (e Entity) StopFire(sarg string, a, b, c float64) (string, error)
```

StopFire runs `PIER_AACT_STOP_FIRE`: `Actor::extinguishFire`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetVelocity` {#Entity.SetVelocity}

```go
func (e Entity) SetVelocity(sarg string, a, b, c float64) (string, error)
```

SetVelocity runs `PIER_AACT_SET_VELOCITY`: a,b,c=vel `Actor::setVelocity`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.ApplyImpulse` {#Entity.ApplyImpulse}

```go
func (e Entity) ApplyImpulse(sarg string, a, b, c float64) (string, error)
```

ApplyImpulse runs `PIER_AACT_APPLY_IMPULSE`: a,b,c=impulse `Actor::applyImpulse`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetScoreTag` {#Entity.SetScoreTag}

```go
func (e Entity) SetScoreTag(sarg string, a, b, c float64) (string, error)
```

SetScoreTag runs `PIER_AACT_SET_SCORE_TAG`: sarg=text `Actor::setScoreTag`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetSkinId` {#Entity.SetSkinId}

```go
func (e Entity) SetSkinId(sarg string, a, b, c float64) (string, error)
```

SetSkinId runs `PIER_AACT_SET_SKIN_ID`: a=skin id `Actor::setSkinID`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetStrength` {#Entity.SetStrength}

```go
func (e Entity) SetStrength(sarg string, a, b, c float64) (string, error)
```

SetStrength runs `PIER_AACT_SET_STRENGTH`: a=strength `Actor::setStrength`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.RemoveAllPassengers` {#Entity.RemoveAllPassengers}

```go
func (e Entity) RemoveAllPassengers(sarg string, a, b, c float64) (string, error)
```

RemoveAllPassengers runs `PIER_AACT_REMOVE_ALL_PASSENGERS`: `Actor::removeAllPassengers`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.ExecuteEvent` {#Entity.ExecuteEvent}

```go
func (e Entity) ExecuteEvent(sarg string, a, b, c float64) (string, error)
```

ExecuteEvent runs `PIER_AACT_EXECUTE_EVENT`: sarg=event name `Actor::executeEvent`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity.SetRotation` {#Entity.SetRotation}

```go
func (e Entity) SetRotation(sarg string, a, b, c float64) (string, error)
```

SetRotation runs `PIER_AACT_SET_ROTATION`: a=pitch b=yaw `Actor::setRotationWrapped`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

## `ActorInfo` {#ActorInfo}

```go
type ActorInfo struct {
    ID   ActorID
    Type string
}
```

ActorInfo is one actor of a dimension: its unique id and its type name.

## `ActorID` {#ActorID}

```go
type ActorID int64
```

ActorID is an actor's unique id, the `uid` of event payloads. 0 never resolves.

## `AProp*` {#AProp}

| Name | Value | Description |
|---|---|---|
| <span id="APropPosX"></span>`APropPosX` | `0` | APropPosX is `PIER_APROP_POS_X`: (G) `Actor::getPosition()`.x (feet: getFeetPos for players; POS\_\* uses getPosition) |
| <span id="APropPosY"></span>`APropPosY` | `1` | APropPosY is `PIER_APROP_POS_Y`: (G) |
| <span id="APropPosZ"></span>`APropPosZ` | `2` | APropPosZ is `PIER_APROP_POS_Z`: (G) |
| <span id="APropRotPitch"></span>`APropRotPitch` | `3` | APropRotPitch is `PIER_APROP_ROT_PITCH`: (G) `Actor::getRotation()`.x |
| <span id="APropRotYaw"></span>`APropRotYaw` | `4` | APropRotYaw is `PIER_APROP_ROT_YAW`: (G) `Actor::getRotation()`.y |
| <span id="APropDimension"></span>`APropDimension` | `5` | APropDimension is `PIER_APROP_DIMENSION`: (G) `Actor::getDimensionId` |
| <span id="APropHealth"></span>`APropHealth` | `6` | APropHealth is `PIER_APROP_HEALTH`: (G) `Actor::getHealth`; heal/hurt via actions |
| <span id="APropMaxHealth"></span>`APropMaxHealth` | `7` | APropMaxHealth is `PIER_APROP_MAX_HEALTH`: (G) `Actor::getMaxHealth` |
| <span id="APropIsAlive"></span>`APropIsAlive` | `8` | APropIsAlive is `PIER_APROP_IS_ALIVE`: (G) `Actor::isAlive` |
| <span id="APropIsOnGround"></span>`APropIsOnGround` | `9` | APropIsOnGround is `PIER_APROP_IS_ON_GROUND`: (G) `Actor::isOnGround` |
| <span id="APropIsInWater"></span>`APropIsInWater` | `10` | APropIsInWater is `PIER_APROP_IS_IN_WATER`: (G) `Actor::isInWater` |
| <span id="APropIsInLava"></span>`APropIsInLava` | `11` | APropIsInLava is `PIER_APROP_IS_IN_LAVA`: (G) `Actor::isInLava` |
| <span id="APropIsOnFire"></span>`APropIsOnFire` | `12` | APropIsOnFire is `PIER_APROP_IS_ON_FIRE`: (G) `Actor::isOnFire` |
| <span id="APropIsInvisible"></span>`APropIsInvisible` | `13` | APropIsInvisible is `PIER_APROP_IS_INVISIBLE`: (G) `Actor::isInvisible` |
| <span id="APropIsSneaking"></span>`APropIsSneaking` | `14` | APropIsSneaking is `PIER_APROP_IS_SNEAKING`: (G) `Actor::isSneaking` |
| <span id="APropIsBaby"></span>`APropIsBaby` | `15` | APropIsBaby is `PIER_APROP_IS_BABY`: (G) `Actor::isBaby` |
| <span id="APropIsRiding"></span>`APropIsRiding` | `16` | APropIsRiding is `PIER_APROP_IS_RIDING`: (G) `Actor::isRiding` |
| <span id="APropIsTame"></span>`APropIsTame` | `17` | APropIsTame is `PIER_APROP_IS_TAME`: (G) `Actor::isTame` |
| <span id="APropSpeed"></span>`APropSpeed` | `18` | APropSpeed is `PIER_APROP_SPEED`: (G) `Actor::getSpeedInMetersPerSecond` |
| <span id="APropViewX"></span>`APropViewX` | `19` | APropViewX is `PIER_APROP_VIEW_X`: (G) `Actor::getViewVector()`.x |
| <span id="APropViewY"></span>`APropViewY` | `20` | APropViewY is `PIER_APROP_VIEW_Y`: (G) `Actor::getViewVector()`.y |
| <span id="APropViewZ"></span>`APropViewZ` | `21` | APropViewZ is `PIER_APROP_VIEW_Z`: (G) `Actor::getViewVector()`.z |
| <span id="APropVelX"></span>`APropVelX` | `22` | APropVelX is `PIER_APROP_VEL_X`: (G) `Actor::getVelocity()`.x |
| <span id="APropVelY"></span>`APropVelY` | `23` | APropVelY is `PIER_APROP_VEL_Y`: (G) `Actor::getVelocity()`.y |
| <span id="APropVelZ"></span>`APropVelZ` | `24` | APropVelZ is `PIER_APROP_VEL_Z`: (G) `Actor::getVelocity()`.z |
| <span id="APropHeadX"></span>`APropHeadX` | `25` | APropHeadX is `PIER_APROP_HEAD_X`: (G) `Actor::getHeadPos()`.x |
| <span id="APropHeadY"></span>`APropHeadY` | `26` | APropHeadY is `PIER_APROP_HEAD_Y`: (G) `Actor::getHeadPos()`.y |
| <span id="APropHeadZ"></span>`APropHeadZ` | `27` | APropHeadZ is `PIER_APROP_HEAD_Z`: (G) `Actor::getHeadPos()`.z |
| <span id="APropFeetX"></span>`APropFeetX` | `28` | APropFeetX is `PIER_APROP_FEET_X`: (G) `Actor::getFeetPos()`.x |
| <span id="APropFeetY"></span>`APropFeetY` | `29` | APropFeetY is `PIER_APROP_FEET_Y`: (G) `Actor::getFeetPos()`.y |
| <span id="APropFeetZ"></span>`APropFeetZ` | `30` | APropFeetZ is `PIER_APROP_FEET_Z`: (G) `Actor::getFeetPos()`.z |
| <span id="APropFallDistance"></span>`APropFallDistance` | `31` | APropFallDistance is `PIER_APROP_FALL_DISTANCE`: (G) `Actor::getFallDistance` |
| <span id="APropIsPersistent"></span>`APropIsPersistent` | `32` | APropIsPersistent is `PIER_APROP_IS_PERSISTENT`: (G) `Actor::isPersistent` |
| <span id="APropIsLeashed"></span>`APropIsLeashed` | `33` | APropIsLeashed is `PIER_APROP_IS_LEASHED`: (G) `Actor::isLeashed` |
| <span id="APropIsInvulnerable"></span>`APropIsInvulnerable` | `34` | APropIsInvulnerable is `PIER_APROP_IS_INVULNERABLE`: (G) `Actor::isInvulnerable` |
| <span id="APropVariant"></span>`APropVariant` | `35` | APropVariant is `PIER_APROP_VARIANT`: (G) `Actor::getVariant` |
| <span id="APropMarkVariant"></span>`APropMarkVariant` | `36` | APropMarkVariant is `PIER_APROP_MARK_VARIANT`: (G) `Actor::getMarkVariant` |
| <span id="APropScale"></span>`APropScale` | `37` | APropScale is `PIER_APROP_SCALE`: (G) `Actor::getScaleFactor` |
| <span id="APropBrightness"></span>`APropBrightness` | `38` | APropBrightness is `PIER_APROP_BRIGHTNESS`: (G) `Actor::getBrightness` |
| <span id="APropRadius"></span>`APropRadius` | `39` | APropRadius is `PIER_APROP_RADIUS`: (G) `Actor::getRadius` |
| <span id="APropHasTotem"></span>`APropHasTotem` | `40` | APropHasTotem is `PIER_APROP_HAS_TOTEM`: (G) `Actor::hasTotemEquipped` |
| <span id="APropIsInRain"></span>`APropIsInRain` | `41` | APropIsInRain is `PIER_APROP_IS_IN_RAIN`: (G) `Actor::isInRain` |
| <span id="APropIsInSnow"></span>`APropIsInSnow` | `42` | APropIsInSnow is `PIER_APROP_IS_IN_SNOW`: (G) `Actor::isInSnow` |
| <span id="APropIsInThunderstorm"></span>`APropIsInThunderstorm` | `43` | APropIsInThunderstorm is `PIER_APROP_IS_IN_THUNDERSTORM`: (G) `Actor::isInThunderstorm` |
| <span id="APropIsFrozen"></span>`APropIsFrozen` | `44` | APropIsFrozen is `PIER_APROP_IS_FROZEN`: (G) `Actor::isFrozen` |
| <span id="APropIsInLove"></span>`APropIsInLove` | `45` | **the current engine version gives no value for this constant**: APropIsInLove is `PIER_APROP_IS_IN_LOVE`: (G) unsupported since BDS 1.26.40: `Actor::isInLove` is gone |
| <span id="APropDeathTime"></span>`APropDeathTime` | `46` | APropDeathTime is `PIER_APROP_DEATH_TIME`: (G) `Actor::getDeathTime` |
| <span id="APropHasPassenger"></span>`APropHasPassenger` | `47` | APropHasPassenger is `PIER_APROP_HAS_PASSENGER`: (G) `Actor::hasPassenger` |

## `AStr*` {#AStr}

| Name | Value | Description |
|---|---|---|
| <span id="AStrTypeName"></span>`AStrTypeName` | `0` | AStrTypeName is `PIER_ASTR_TYPE_NAME`: `Actor::getTypeName` |
| <span id="AStrNameTag"></span>`AStrNameTag` | `1` | AStrNameTag is `PIER_ASTR_NAME_TAG`: `Actor::getNameTag` |
| <span id="AStrScoreTag"></span>`AStrScoreTag` | `2` | AStrScoreTag is `PIER_ASTR_SCORE_TAG`: `Actor::getScoreTag` |
| <span id="AStrFilteredName"></span>`AStrFilteredName` | `3` | AStrFilteredName is `PIER_ASTR_FILTERED_NAME`: `Actor::getFilteredNameTag` |

## `AAct*` {#AAct}

| Name | Value | Description |
|---|---|---|
| <span id="AActKill"></span>`AActKill` | `0` | AActKill is `PIER_AACT_KILL`: `Actor::kill` |
| <span id="AActDespawn"></span>`AActDespawn` | `1` | AActDespawn is `PIER_AACT_DESPAWN`: `Actor::despawn` |
| <span id="AActHeal"></span>`AActHeal` | `2` | AActHeal is `PIER_AACT_HEAL`: a=amount `Actor::heal` |
| <span id="AActSetOnFire"></span>`AActSetOnFire` | `3` | AActSetOnFire is `PIER_AACT_SET_ON_FIRE`: a=seconds `Actor::setOnFire` |
| <span id="AActTeleport"></span>`AActTeleport` | `4` | AActTeleport is `PIER_AACT_TELEPORT`: a,b,c=pos, sarg=dim ("0".."2") `Actor::teleport` |
| <span id="AActSetNameTag"></span>`AActSetNameTag` | `5` | AActSetNameTag is `PIER_AACT_SET_NAME_TAG`: sarg=name `Actor::setNameTag` |
| <span id="AActAddTag"></span>`AActAddTag` | `6` | AActAddTag is `PIER_AACT_ADD_TAG`: sarg=tag → out "0"/"1" `Actor::addTag` |
| <span id="AActRemoveTag"></span>`AActRemoveTag` | `7` | AActRemoveTag is `PIER_AACT_REMOVE_TAG`: sarg=tag → out "0"/"1" `Actor::removeTag` |
| <span id="AActHasTag"></span>`AActHasTag` | `8` | AActHasTag is `PIER_AACT_HAS_TAG`: sarg=tag → out "0"/"1" `Actor::hasTag` |
| <span id="AActAddEffect"></span>`AActAddEffect` | `9` | AActAddEffect is `PIER_AACT_ADD_EFFECT`: sarg=effect name, a=ticks, b=amplifier, c=visible(0/1) `MobEffect::getByName` + `Actor::addEffect` |
| <span id="AActRemoveEffect"></span>`AActRemoveEffect` | `10` | AActRemoveEffect is `PIER_AACT_REMOVE_EFFECT`: sarg=effect name `Actor::removeEffect`(id) |
| <span id="AActClearEffects"></span>`AActClearEffects` | `11` | AActClearEffects is `PIER_AACT_CLEAR_EFFECTS`: `Actor::removeAllEffects` |
| <span id="AActHurt"></span>`AActHurt` | `12` | AActHurt is `PIER_AACT_HURT`: a=damage (generic damage source) `Actor::hurt` |
| <span id="AActAttributeGet"></span>`AActAttributeGet` | `13` | AActAttributeGet is `PIER_AACT_ATTRIBUTE_GET`: sarg=attribute name ("minecraft:health"…) → out value |
| <span id="AActSetVariant"></span>`AActSetVariant` | `14` | AActSetVariant is `PIER_AACT_SET_VARIANT`: a=variant `Actor::setVariant` |
| <span id="AActSetMarkVariant"></span>`AActSetMarkVariant` | `15` | AActSetMarkVariant is `PIER_AACT_SET_MARK_VARIANT`: a=variant `Actor::setMarkVariant` |
| <span id="AActSetPersistent"></span>`AActSetPersistent` | `16` | AActSetPersistent is `PIER_AACT_SET_PERSISTENT`: `Actor::setPersistent` |
| <span id="AActSetLeashHolder"></span>`AActSetLeashHolder` | `17` | AActSetLeashHolder is `PIER_AACT_SET_LEASH_HOLDER`: a=holder ActorUniqueID `Actor::setLeashHolder` |
| <span id="AActSetInvisible"></span>`AActSetInvisible` | `18` | AActSetInvisible is `PIER_AACT_SET_INVISIBLE`: a=0/1 `Actor::setInvisible` |
| <span id="AActSetSneaking"></span>`AActSetSneaking` | `19` | AActSetSneaking is `PIER_AACT_SET_SNEAKING`: a=0/1 `Actor::setSneaking` |
| <span id="AActSetNameTagVisible"></span>`AActSetNameTagVisible` | `20` | AActSetNameTagVisible is `PIER_AACT_SET_NAME_TAG_VISIBLE`: a=0/1 `Actor::setNameTagVisible` |
| <span id="AActSetTarget"></span>`AActSetTarget` | `21` | AActSetTarget is `PIER_AACT_SET_TARGET`: a=target ActorUniqueID `Actor::setTarget` |
| <span id="AActSetOwner"></span>`AActSetOwner` | `22` | AActSetOwner is `PIER_AACT_SET_OWNER`: a=owner ActorUniqueID `Actor::setOwner` |
| <span id="AActBurn"></span>`AActBurn` | `23` | AActBurn is `PIER_AACT_BURN`: a=damage `Actor::burn` |
| <span id="AActStopFire"></span>`AActStopFire` | `24` | AActStopFire is `PIER_AACT_STOP_FIRE`: `Actor::extinguishFire` |
| <span id="AActSetVelocity"></span>`AActSetVelocity` | `25` | AActSetVelocity is `PIER_AACT_SET_VELOCITY`: a,b,c=vel `Actor::setVelocity` |
| <span id="AActApplyImpulse"></span>`AActApplyImpulse` | `26` | AActApplyImpulse is `PIER_AACT_APPLY_IMPULSE`: a,b,c=impulse `Actor::applyImpulse` |
| <span id="AActSetScoreTag"></span>`AActSetScoreTag` | `27` | AActSetScoreTag is `PIER_AACT_SET_SCORE_TAG`: sarg=text `Actor::setScoreTag` |
| <span id="AActSetSkinId"></span>`AActSetSkinId` | `28` | AActSetSkinId is `PIER_AACT_SET_SKIN_ID`: a=skin id `Actor::setSkinID` |
| <span id="AActSetStrength"></span>`AActSetStrength` | `29` | AActSetStrength is `PIER_AACT_SET_STRENGTH`: a=strength `Actor::setStrength` |
| <span id="AActRemoveAllPassengers"></span>`AActRemoveAllPassengers` | `30` | AActRemoveAllPassengers is `PIER_AACT_REMOVE_ALL_PASSENGERS`: `Actor::removeAllPassengers` |
| <span id="AActExecuteEvent"></span>`AActExecuteEvent` | `31` | AActExecuteEvent is `PIER_AACT_EXECUTE_EVENT`: sarg=event name `Actor::executeEvent` |
| <span id="AActSetRotation"></span>`AActSetRotation` | `32` | AActSetRotation is `PIER_AACT_SET_ROTATION`: a=pitch b=yaw `Actor::setRotationWrapped` |
