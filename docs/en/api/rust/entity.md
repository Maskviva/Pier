# levilamina::entity

Actors: everything addressed by an `ActorUniqueID`, players included.

**An id is an identity and not a pointer**

An [`Entity`](entity.md#Entity) holds one `i64`. The host looks the live table up again on every call, so
an `Entity` value can be kept across ticks: once the actor dies a call returns `Err`
rather than jumping into freed memory. The cost is one lookup per call, so a hot path
caches the result itself.

**A player passes through here to use actor capabilities**

`Player::as_entity()` goes through `player_resolve` for the id. The reverse does not
hold: an actor id is not necessarily a player, and there is no slot resolving an id back
into a selector.

## `Entity` {#Entity}

```rust
pub struct Entity(/* private */);
```

One actor. A zero-cost wrapper around an `ActorUniqueID`.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`, `Hash`, `PartialOrd`, `Ord`, `Display`

### `Entity::from_id` {#Entity.from_id}

```rust
pub fn from_id(id: i64) -> Entity
```

- Parameters:
    - id : `i64`
- Return type: `Entity`

### `Entity::id` {#Entity.id}

```rust
pub fn id(&self) -> i64
```

- Return type: `i64`

### `Entity::list` {#Entity.list}

```rust
pub fn list(dim: Option<i32>) -> Vec<ActorEntry>
```

Enumerates live actors. A `dim` of `None` spans every dimension.

This slot has no failure bit and reports nothing while the level is not ready, so an
empty table means either that the dimension holds no actor or that the level has not
come up, and a caller tells them apart with `Host::gaming_status()`.

- Parameters:
    - dim : `Option<i32>`
- Return type: `Vec<ActorEntry>`
- Slots: [`list_actors`](../cpp/entity.md#list_actors), [`list_actors`](../cpp/entity.md#list_actors)

### `Entity::exists` {#Entity.exists}

```rust
pub fn exists(&self) -> bool
```

Whether this id still points at a live actor.

The criterion is whether the type name can be read: every actor that resolves has one,
and one that does not makes `actor_get_str` return false.

- Return type: `bool`
- Slots: [`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity::snapshot` {#Entity.snapshot}

```rust
pub fn snapshot(&self) -> Result<NbtValue>
```

- Return type: `Result<NbtValue>`
- Slots: [`actor_snapshot`](../cpp/entity.md#actor_snapshot)

### `Entity::num` {#Entity.num}

```rust
pub fn num(&self, prop: i32) -> Result<f64>
```

Reads a `PIER_APROP_*` numeric property.

- Parameters:
    - prop : `i32`
- Return type: `Result<f64>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::text` {#Entity.text}

```rust
pub fn text(&self, prop: i32) -> Result<String>
```

Reads a `PIER_ASTR_*` string property.

- Parameters:
    - prop : `i32`
- Return type: `Result<String>`
- Slots: [`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity::pos` {#Entity.pos}

```rust
pub fn pos(&self) -> Result<PositionF64>
```

The position, from `Actor::getPosition`. For the feet coordinate of a player see
[`Entity::feet_pos`](entity.md#Entity.feet_pos).

- Return type: `Result<PositionF64>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::feet_pos` {#Entity.feet_pos}

```rust
pub fn feet_pos(&self) -> Result<PositionF64>
```

- Return type: `Result<PositionF64>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::head_pos` {#Entity.head_pos}

```rust
pub fn head_pos(&self) -> Result<PositionF64>
```

- Return type: `Result<PositionF64>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::velocity` {#Entity.velocity}

```rust
pub fn velocity(&self) -> Result<PositionF64>
```

- Return type: `Result<PositionF64>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::view_vector` {#Entity.view_vector}

```rust
pub fn view_vector(&self) -> Result<PositionF64>
```

The unit vector of the line of sight.

- Return type: `Result<PositionF64>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::rotation` {#Entity.rotation}

```rust
pub fn rotation(&self) -> Result<(f64, f64)>
```

`(pitch, yaw)`.

- Return type: `Result<(f64, f64)>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::act` {#Entity.act}

```rust
pub fn act(&self, action: i32, sarg: &str, a: f64, b: f64, c: f64) -> Result<String>
```

Runs one `PIER_AACT_*` action and returns its output, which is an empty string for most
actions.

- Parameters:
    - action : `i32`
    - sarg : `&str`
    - a : `f64`
    - b : `f64`
    - c : `f64`
- Return type: `Result<String>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::kill` {#Entity.kill}

```rust
pub fn kill(&self) -> Result<()>
```

- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::despawn` {#Entity.despawn}

```rust
pub fn despawn(&self) -> Result<()>
```

- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::clear_effects` {#Entity.clear_effects}

```rust
pub fn clear_effects(&self) -> Result<()>
```

- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::stop_fire` {#Entity.stop_fire}

```rust
pub fn stop_fire(&self) -> Result<()>
```

- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::remove_all_passengers` {#Entity.remove_all_passengers}

```rust
pub fn remove_all_passengers(&self) -> Result<()>
```

- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::heal` {#Entity.heal}

```rust
pub fn heal(&self, amount: f64) -> Result<()>
```

- Parameters:
    - amount : `f64`
- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::hurt` {#Entity.hurt}

```rust
pub fn hurt(&self, amount: f64) -> Result<()>
```

- Parameters:
    - amount : `f64`
- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::burn` {#Entity.burn}

```rust
pub fn burn(&self, damage: f64) -> Result<()>
```

- Parameters:
    - damage : `f64`
- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_on_fire` {#Entity.set_on_fire}

```rust
pub fn set_on_fire(&self, seconds: i32) -> Result<()>
```

- Parameters:
    - seconds : `i32`
- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::teleport` {#Entity.teleport}

```rust
pub fn teleport(&self, x: f64, y: f64, z: f64) -> Result<()>
```

Teleports elsewhere within the same dimension.

- Parameters:
    - x : `f64`
    - y : `f64`
    - z : `f64`
- Return type: `Result<()>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num), [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::teleport_to` {#Entity.teleport_to}

```rust
pub fn teleport_to(&self, dim: i32, x: f64, y: f64, z: f64) -> Result<()>
```

Teleports into a given dimension. A custom dimension, with an id of 3 or above, goes
through here too.

- Parameters:
    - dim : `i32`
    - x : `f64`
    - y : `f64`
    - z : `f64`
- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_rotation` {#Entity.set_rotation}

```rust
pub fn set_rotation(&self, pitch: f64, yaw: f64) -> Result<()>
```

- Parameters:
    - pitch : `f64`
    - yaw : `f64`
- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_velocity` {#Entity.set_velocity}

```rust
pub fn set_velocity(&self, x: f64, y: f64, z: f64) -> Result<()>
```

- Parameters:
    - x : `f64`
    - y : `f64`
    - z : `f64`
- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::apply_impulse` {#Entity.apply_impulse}

```rust
pub fn apply_impulse(&self, x: f64, y: f64, z: f64) -> Result<()>
```

- Parameters:
    - x : `f64`
    - y : `f64`
    - z : `f64`
- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_name_tag` {#Entity.set_name_tag}

```rust
pub fn set_name_tag(&self, name: &str) -> Result<()>
```

- Parameters:
    - name : `&str`
- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_name_tag_visible` {#Entity.set_name_tag_visible}

```rust
pub fn set_name_tag_visible(&self, on: bool) -> Result<()>
```

- Parameters:
    - on : `bool`
- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_score_tag` {#Entity.set_score_tag}

```rust
pub fn set_score_tag(&self, text: &str) -> Result<()>
```

- Parameters:
    - text : `&str`
- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::add_tag` {#Entity.add_tag}

```rust
pub fn add_tag(&self, tag: &str) -> Result<bool>
```

- Parameters:
    - tag : `&str`
- Return type: `Result<bool>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::remove_tag` {#Entity.remove_tag}

```rust
pub fn remove_tag(&self, tag: &str) -> Result<bool>
```

- Parameters:
    - tag : `&str`
- Return type: `Result<bool>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::has_tag` {#Entity.has_tag}

```rust
pub fn has_tag(&self, tag: &str) -> Result<bool>
```

- Parameters:
    - tag : `&str`
- Return type: `Result<bool>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::add_effect` {#Entity.add_effect}

```rust
pub fn add_effect(
        &self,
        effect: &str,
        ticks: i32,
        amplifier: i32,
        visible: bool,
    ) -> Result<()>
```

Adds one status effect.

- Parameters:
    - effect : `&str`
    - ticks : `i32`
    - amplifier : `i32`
    - visible : `bool`
- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::remove_effect` {#Entity.remove_effect}

```rust
pub fn remove_effect(&self, effect: &str) -> Result<()>
```

- Parameters:
    - effect : `&str`
- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::attribute` {#Entity.attribute}

```rust
pub fn attribute(&self, name: &str) -> Result<f64>
```

Reads the current value of one attribute, named as `minecraft:health` is.

- Parameters:
    - name : `&str`
- Return type: `Result<f64>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_variant` {#Entity.set_variant}

```rust
pub fn set_variant(&self, v: i32) -> Result<()>
```

- Parameters:
    - v : `i32`
- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_mark_variant` {#Entity.set_mark_variant}

```rust
pub fn set_mark_variant(&self, v: i32) -> Result<()>
```

- Parameters:
    - v : `i32`
- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_persistent` {#Entity.set_persistent}

```rust
pub fn set_persistent(&self) -> Result<()>
```

- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_invisible` {#Entity.set_invisible}

```rust
pub fn set_invisible(&self, on: bool) -> Result<()>
```

- Parameters:
    - on : `bool`
- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_sneaking` {#Entity.set_sneaking}

```rust
pub fn set_sneaking(&self, on: bool) -> Result<()>
```

- Parameters:
    - on : `bool`
- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_skin_id` {#Entity.set_skin_id}

```rust
pub fn set_skin_id(&self, id: i32) -> Result<()>
```

- Parameters:
    - id : `i32`
- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_strength` {#Entity.set_strength}

```rust
pub fn set_strength(&self, v: i32) -> Result<()>
```

- Parameters:
    - v : `i32`
- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_target` {#Entity.set_target}

```rust
pub fn set_target(&self, target: Entity) -> Result<()>
```

- Parameters:
    - target : `Entity`
- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_owner` {#Entity.set_owner}

```rust
pub fn set_owner(&self, owner: Entity) -> Result<()>
```

- Parameters:
    - owner : `Entity`
- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_leash_holder` {#Entity.set_leash_holder}

```rust
pub fn set_leash_holder(&self, holder: Entity) -> Result<()>
```

- Parameters:
    - holder : `Entity`
- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::execute_event` {#Entity.execute_event}

```rust
pub fn execute_event(&self, event: &str) -> Result<()>
```

- Parameters:
    - event : `&str`
- Return type: `Result<()>`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Entity::type_name` {#Entity.type_name}

```rust
pub fn type_name(&self) -> Result<String>
```

- Return type: `Result<String>`
- Slots: [`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity::name_tag` {#Entity.name_tag}

```rust
pub fn name_tag(&self) -> Result<String>
```

The name shown above the head.

- Return type: `Result<String>`
- Slots: [`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity::score_tag` {#Entity.score_tag}

```rust
pub fn score_tag(&self) -> Result<String>
```

The line below the name.

- Return type: `Result<String>`
- Slots: [`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity::filtered_name` {#Entity.filtered_name}

```rust
pub fn filtered_name(&self) -> Result<String>
```

The name after profanity filtering.

- Return type: `Result<String>`
- Slots: [`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity::dimension` {#Entity.dimension}

```rust
pub fn dimension(&self) -> Result<i32>
```

- Return type: `Result<i32>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::health` {#Entity.health}

```rust
pub fn health(&self) -> Result<f64>
```

- Return type: `Result<f64>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::max_health` {#Entity.max_health}

```rust
pub fn max_health(&self) -> Result<f64>
```

- Return type: `Result<f64>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::speed` {#Entity.speed}

```rust
pub fn speed(&self) -> Result<f64>
```

The current movement speed, in blocks per tick.

- Return type: `Result<f64>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::fall_distance` {#Entity.fall_distance}

```rust
pub fn fall_distance(&self) -> Result<f64>
```

How many blocks the current fall has covered, used to compute fall damage on landing.

- Return type: `Result<f64>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::scale` {#Entity.scale}

```rust
pub fn scale(&self) -> Result<f64>
```

The size scale, where 1.0 is the original size.

- Return type: `Result<f64>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::variant` {#Entity.variant}

```rust
pub fn variant(&self) -> Result<i32>
```

The `variant` data value, whose meaning differs per actor kind.

- Return type: `Result<i32>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::mark_variant` {#Entity.mark_variant}

```rust
pub fn mark_variant(&self) -> Result<i32>
```

The `mark_variant` data value, a different numbering from [`Entity::variant`](entity.md#Entity.variant).

- Return type: `Result<i32>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::death_time` {#Entity.death_time}

```rust
pub fn death_time(&self) -> Result<i32>
```

How many ticks of the death animation have played.

- Return type: `Result<i32>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_alive` {#Entity.is_alive}

```rust
pub fn is_alive(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_on_ground` {#Entity.is_on_ground}

```rust
pub fn is_on_ground(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_in_water` {#Entity.is_in_water}

```rust
pub fn is_in_water(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_in_lava` {#Entity.is_in_lava}

```rust
pub fn is_in_lava(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_on_fire` {#Entity.is_on_fire}

```rust
pub fn is_on_fire(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_invisible` {#Entity.is_invisible}

```rust
pub fn is_invisible(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_sneaking` {#Entity.is_sneaking}

```rust
pub fn is_sneaking(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_baby` {#Entity.is_baby}

```rust
pub fn is_baby(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_riding` {#Entity.is_riding}

```rust
pub fn is_riding(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_tame` {#Entity.is_tame}

```rust
pub fn is_tame(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_persistent` {#Entity.is_persistent}

```rust
pub fn is_persistent(&self) -> Result<bool>
```

It is not despawned for being too far from a player.

- Return type: `Result<bool>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_leashed` {#Entity.is_leashed}

```rust
pub fn is_leashed(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_invulnerable` {#Entity.is_invulnerable}

```rust
pub fn is_invulnerable(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_frozen` {#Entity.is_frozen}

```rust
pub fn is_frozen(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_in_love` {#Entity.is_in_love}

```rust
pub fn is_in_love(&self) -> Result<bool>
```

In the breeding state.

- Return type: `Result<bool>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_in_rain` {#Entity.is_in_rain}

```rust
pub fn is_in_rain(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_in_snow` {#Entity.is_in_snow}

```rust
pub fn is_in_snow(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_in_thunderstorm` {#Entity.is_in_thunderstorm}

```rust
pub fn is_in_thunderstorm(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::has_totem` {#Entity.has_totem}

```rust
pub fn has_totem(&self) -> Result<bool>
```

Holding a totem of undying in a hand or the off hand.

- Return type: `Result<bool>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::has_passenger` {#Entity.has_passenger}

```rust
pub fn has_passenger(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::vehicle` {#Entity.vehicle}

```rust
pub fn vehicle(&self) -> Result<Option<Entity>>
```

The vehicle being ridden. Riding nothing gives `Ok(None)` and only a missing slot is
an `Err`.

- Return type: `Result<Option<Entity>>`
- Slots: [`actor_get_vehicle`](../cpp/entity.md#actor_get_vehicle)

### `Entity::first_passenger` {#Entity.first_passenger}

```rust
pub fn first_passenger(&self) -> Result<Option<Entity>>
```

- Return type: `Result<Option<Entity>>`
- Slots: [`actor_get_first_passenger`](../cpp/entity.md#actor_get_first_passenger)

### `Entity::owner` {#Entity.owner}

```rust
pub fn owner(&self) -> Result<Option<Entity>>
```

- Return type: `Result<Option<Entity>>`
- Slots: [`actor_get_owner`](../cpp/entity.md#actor_get_owner)

### `Entity::target` {#Entity.target}

```rust
pub fn target(&self) -> Result<Option<Entity>>
```

- Return type: `Result<Option<Entity>>`
- Slots: [`actor_get_target`](../cpp/entity.md#actor_get_target)

### `Entity::distance_to` {#Entity.distance_to}

```rust
pub fn distance_to(&self, other: Entity) -> Result<f64>
```

The distance between two actors. The host returns a failure across dimensions.

- Parameters:
    - other : `Entity`
- Return type: `Result<f64>`
- Slots: [`actor_distance_to`](../cpp/entity.md#actor_distance_to)

### `Entity::aabb` {#Entity.aabb}

```rust
pub fn aabb(&self) -> Result<Aabb>
```

The bounding box.

- Return type: `Result<Aabb>`
- Slots: [`actor_get_aabb`](../cpp/entity.md#actor_get_aabb)

### `Entity::clone_at` {#Entity.clone_at}

```rust
pub fn clone_at(&self, dim: i32, x: f64, y: f64, z: f64) -> Result<Entity>
```

Clones one to a given position.

- Parameters:
    - dim : `i32`
    - x : `f64`
    - y : `f64`
    - z : `f64`
- Return type: `Result<Entity>`
- Slots: [`actor_clone`](../cpp/entity.md#actor_clone)

### `Entity::equipped_item` {#Entity.equipped_item}

```rust
pub fn equipped_item(&self, slot: EquipSlot) -> Result<ItemStack>
```

- Parameters:
    - slot : `EquipSlot`
- Return type: `Result<ItemStack>`
- Slots: [`actor_get_equipped_item`](../cpp/entity.md#actor_get_equipped_item)

### `Entity::set_equipped_item` {#Entity.set_equipped_item}

```rust
pub fn set_equipped_item(&self, slot: EquipSlot, item: &ItemStack) -> Result<()>
```

- Parameters:
    - slot : `EquipSlot`
    - item : `&ItemStack`
- Return type: `Result<()>`
- Slots: [`actor_set_equipped_item`](../cpp/entity.md#actor_set_equipped_item)

### `Entity::effects` {#Entity.effects}

```rust
pub fn effects(&self) -> Result<Vec<Effect>>
```

Every status effect on it.

- Return type: `Result<Vec<Effect>>`
- Slots: [`actor_get_effects`](../cpp/entity.md#actor_get_effects)

### `Entity::status_flag` {#Entity.status_flag}

```rust
pub fn status_flag(&self, flag_index: i32) -> Result<bool>
```

Reads one bit of `ActorFlags`.

On the ABI this slot collapses the actor being gone and the bit being false into the
same `false`, the shape contract §5.2 opposes. That signature is already released, so
this only states it truthfully.
Call [`Entity::exists`](entity.md#Entity.exists) first when the two must be told apart.

- Parameters:
    - flag_index : `i32`
- Return type: `Result<bool>`
- Slots: [`actor_get_status_flag`](../cpp/entity.md#actor_get_status_flag)

### `Entity::set_status_flag` {#Entity.set_status_flag}

```rust
pub fn set_status_flag(&self, flag_index: i32, value: bool) -> Result<()>
```

- Parameters:
    - flag_index : `i32`
    - value : `bool`
- Return type: `Result<()>`
- Slots: [`actor_set_status_flag`](../cpp/entity.md#actor_set_status_flag)

### `Entity::trace_ray` {#Entity.trace_ray}

```rust
pub fn trace_ray(
        self,
        max_dist: f32,
        include_actors: bool,
        include_blocks: bool,
    ) -> Result<RayHit>
```

Casts a ray along the line of sight of this actor, reporting the hit as an exact
coordinate.

- Parameters:
    - max_dist : `f32`
    - include_actors : `bool`
    - include_blocks : `bool`
- Return type: `Result<RayHit>`
- Slots: [`actor_trace_ray`](../cpp/entity.md#actor_trace_ray)

### `Entity::trace_ray_blocks` {#Entity.trace_ray_blocks}

```rust
pub fn trace_ray_blocks(
        self,
        max_dist: f32,
        include_actors: bool,
        include_blocks: bool,
    ) -> Result<RayHit>
```

As above, reporting the hit as a block cell and carrying the face hit.

Both slots are kept because they answer different questions: placing a block needs a
cell coordinate and a face while drawing a particle needs an exact coordinate, and
flooring the latter into the former is off by one cell at a block boundary.

- Parameters:
    - max_dist : `f32`
    - include_actors : `bool`
    - include_blocks : `bool`
- Return type: `Result<RayHit>`
- Slots: [`edit_trace_ray`](../cpp/edit.md#edit_trace_ray)

## `Effect` {#Effect}

```rust
pub struct Effect {
    pub id: String,
    pub ticks: i32,
    pub amplifier: i32,
    pub visible: bool,
}
```

One status effect.

- Implements: `Debug`, `Clone`, `PartialEq`, `Eq`

## `Aabb` {#Aabb}

```rust
pub struct Aabb {
    pub min: PositionF64,
    pub max: PositionF64,
}
```

The axis-aligned bounding box of an actor.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`

## `ActorEntry` {#ActorEntry}

```rust
pub struct ActorEntry {
    pub id: i64,
    pub type_name: String,
}
```

One entry `list_actors` reports.

- Implements: `Debug`, `Clone`, `PartialEq`, `Eq`
