# levilamina::entity · 实体

实体：所有用 `ActorUniqueID` 指定的东西，玩家也包括在内。

**id 是身份，不是指针**

一个 [`Entity`](entity.md#Entity) 只存一个 `i64`。宿主每次调用都重新查一遍活动实体表，所以 `Entity` 值可以跨刻保存：实体死掉以后，调用返回 `Err`，不会跳进已经释放的内存。代价是每次调用一次查找，所以热路径要自己缓存结果。

**玩家要经过这里才能用实体的能力**

`Player::as_entity()` 经 `player_resolve` 取得 id。反过来不成立：一个实体 id 不一定是玩家，也没有槽位能把 id 解析回选择器。

## `Entity` {#Entity}

```rust
pub struct Entity(/* private */);
```

一个实体。`ActorUniqueID` 的零开销包装。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`、`Hash`、`PartialOrd`、`Ord`、`Display`

### `Entity::from_id` {#Entity.from_id}

```rust
pub fn from_id(id: i64) -> Entity
```

- 参数：
    - id : `i64`
- 返回值类型：`Entity`

### `Entity::id` {#Entity.id}

```rust
pub fn id(&self) -> i64
```

- 返回值类型：`i64`

### `Entity::list` {#Entity.list}

```rust
pub fn list(dim: Option<i32>) -> Vec<ActorEntry>
```

列出活着的实体。`dim` 为 `None` 时覆盖所有维度。

这个槽位没有失败位，世界没就绪时什么都不报告，所以表为空，既可能是这个维度里没有实体，也可能是世界还没起来；调用方用 `Host::gaming_status()` 区分两者。

- 参数：
    - dim : `Option<i32>`
- 返回值类型：`Vec<ActorEntry>`
- 对应槽位：[`list_actors`](../cpp/entity.md#list_actors)、[`list_actors`](../cpp/entity.md#list_actors)

### `Entity::exists` {#Entity.exists}

```rust
pub fn exists(&self) -> bool
```

这个 id 是否仍然指向一个活着的实体。

判断依据是能不能读到类型名：每个能解析到的实体都有类型名，解析不到的会让 `actor_get_str` 返回 false。

- 返回值类型：`bool`
- 对应槽位：[`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity::snapshot` {#Entity.snapshot}

```rust
pub fn snapshot(&self) -> Result<NbtValue>
```

- 返回值类型：`Result<NbtValue>`
- 对应槽位：[`actor_snapshot`](../cpp/entity.md#actor_snapshot)

### `Entity::num` {#Entity.num}

```rust
pub fn num(&self, prop: i32) -> Result<f64>
```

读取一个 `PIER_APROP_*` 数值属性。

- 参数：
    - prop : `i32`
- 返回值类型：`Result<f64>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::text` {#Entity.text}

```rust
pub fn text(&self, prop: i32) -> Result<String>
```

读取一个 `PIER_ASTR_*` 字符串属性。

- 参数：
    - prop : `i32`
- 返回值类型：`Result<String>`
- 对应槽位：[`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity::pos` {#Entity.pos}

```rust
pub fn pos(&self) -> Result<PositionF64>
```

位置，取自 `Actor::getPosition`。玩家的脚下坐标见 [`Entity::feet_pos`](entity.md#Entity.feet_pos)。

- 返回值类型：`Result<PositionF64>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::feet_pos` {#Entity.feet_pos}

```rust
pub fn feet_pos(&self) -> Result<PositionF64>
```

- 返回值类型：`Result<PositionF64>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::head_pos` {#Entity.head_pos}

```rust
pub fn head_pos(&self) -> Result<PositionF64>
```

- 返回值类型：`Result<PositionF64>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::velocity` {#Entity.velocity}

```rust
pub fn velocity(&self) -> Result<PositionF64>
```

- 返回值类型：`Result<PositionF64>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::view_vector` {#Entity.view_vector}

```rust
pub fn view_vector(&self) -> Result<PositionF64>
```

视线方向的单位向量。

- 返回值类型：`Result<PositionF64>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::rotation` {#Entity.rotation}

```rust
pub fn rotation(&self) -> Result<(f64, f64)>
```

`(pitch, yaw)`，即俯仰角和偏航角。

- 返回值类型：`Result<(f64, f64)>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::act` {#Entity.act}

```rust
pub fn act(&self, action: i32, sarg: &str, a: f64, b: f64, c: f64) -> Result<String>
```

执行一个 `PIER_AACT_*` 动作，返回它的输出；大多数动作的输出是空字符串。

- 参数：
    - action : `i32`
    - sarg : `&str`
    - a : `f64`
    - b : `f64`
    - c : `f64`
- 返回值类型：`Result<String>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::kill` {#Entity.kill}

```rust
pub fn kill(&self) -> Result<()>
```

- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::despawn` {#Entity.despawn}

```rust
pub fn despawn(&self) -> Result<()>
```

- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::clear_effects` {#Entity.clear_effects}

```rust
pub fn clear_effects(&self) -> Result<()>
```

- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::stop_fire` {#Entity.stop_fire}

```rust
pub fn stop_fire(&self) -> Result<()>
```

- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::remove_all_passengers` {#Entity.remove_all_passengers}

```rust
pub fn remove_all_passengers(&self) -> Result<()>
```

- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::heal` {#Entity.heal}

```rust
pub fn heal(&self, amount: f64) -> Result<()>
```

- 参数：
    - amount : `f64`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::hurt` {#Entity.hurt}

```rust
pub fn hurt(&self, amount: f64) -> Result<()>
```

- 参数：
    - amount : `f64`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::burn` {#Entity.burn}

```rust
pub fn burn(&self, damage: f64) -> Result<()>
```

- 参数：
    - damage : `f64`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_on_fire` {#Entity.set_on_fire}

```rust
pub fn set_on_fire(&self, seconds: i32) -> Result<()>
```

- 参数：
    - seconds : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::teleport` {#Entity.teleport}

```rust
pub fn teleport(&self, x: f64, y: f64, z: f64) -> Result<()>
```

在同一个维度内传送到别处。

- 参数：
    - x : `f64`
    - y : `f64`
    - z : `f64`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)、[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::teleport_to` {#Entity.teleport_to}

```rust
pub fn teleport_to(&self, dim: i32, x: f64, y: f64, z: f64) -> Result<()>
```

传送到指定的维度。id 为 3 及以上的自定义维度也走这里。

- 参数：
    - dim : `i32`
    - x : `f64`
    - y : `f64`
    - z : `f64`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_rotation` {#Entity.set_rotation}

```rust
pub fn set_rotation(&self, pitch: f64, yaw: f64) -> Result<()>
```

- 参数：
    - pitch : `f64`
    - yaw : `f64`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_velocity` {#Entity.set_velocity}

```rust
pub fn set_velocity(&self, x: f64, y: f64, z: f64) -> Result<()>
```

- 参数：
    - x : `f64`
    - y : `f64`
    - z : `f64`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::apply_impulse` {#Entity.apply_impulse}

```rust
pub fn apply_impulse(&self, x: f64, y: f64, z: f64) -> Result<()>
```

- 参数：
    - x : `f64`
    - y : `f64`
    - z : `f64`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_name_tag` {#Entity.set_name_tag}

```rust
pub fn set_name_tag(&self, name: &str) -> Result<()>
```

- 参数：
    - name : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_name_tag_visible` {#Entity.set_name_tag_visible}

```rust
pub fn set_name_tag_visible(&self, on: bool) -> Result<()>
```

- 参数：
    - on : `bool`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_score_tag` {#Entity.set_score_tag}

```rust
pub fn set_score_tag(&self, text: &str) -> Result<()>
```

- 参数：
    - text : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::add_tag` {#Entity.add_tag}

```rust
pub fn add_tag(&self, tag: &str) -> Result<bool>
```

- 参数：
    - tag : `&str`
- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::remove_tag` {#Entity.remove_tag}

```rust
pub fn remove_tag(&self, tag: &str) -> Result<bool>
```

- 参数：
    - tag : `&str`
- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::has_tag` {#Entity.has_tag}

```rust
pub fn has_tag(&self, tag: &str) -> Result<bool>
```

- 参数：
    - tag : `&str`
- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

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

添加一个状态效果。

- 参数：
    - effect : `&str`
    - ticks : `i32`
    - amplifier : `i32`
    - visible : `bool`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::remove_effect` {#Entity.remove_effect}

```rust
pub fn remove_effect(&self, effect: &str) -> Result<()>
```

- 参数：
    - effect : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::attribute` {#Entity.attribute}

```rust
pub fn attribute(&self, name: &str) -> Result<f64>
```

读取一个属性的当前值，属性名的写法和 `minecraft:health` 一样。

- 参数：
    - name : `&str`
- 返回值类型：`Result<f64>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_variant` {#Entity.set_variant}

```rust
pub fn set_variant(&self, v: i32) -> Result<()>
```

- 参数：
    - v : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_mark_variant` {#Entity.set_mark_variant}

```rust
pub fn set_mark_variant(&self, v: i32) -> Result<()>
```

- 参数：
    - v : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_persistent` {#Entity.set_persistent}

```rust
pub fn set_persistent(&self) -> Result<()>
```

- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_invisible` {#Entity.set_invisible}

```rust
pub fn set_invisible(&self, on: bool) -> Result<()>
```

- 参数：
    - on : `bool`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_sneaking` {#Entity.set_sneaking}

```rust
pub fn set_sneaking(&self, on: bool) -> Result<()>
```

- 参数：
    - on : `bool`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_skin_id` {#Entity.set_skin_id}

```rust
pub fn set_skin_id(&self, id: i32) -> Result<()>
```

- 参数：
    - id : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_strength` {#Entity.set_strength}

```rust
pub fn set_strength(&self, v: i32) -> Result<()>
```

- 参数：
    - v : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_target` {#Entity.set_target}

```rust
pub fn set_target(&self, target: Entity) -> Result<()>
```

- 参数：
    - target : `Entity`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_owner` {#Entity.set_owner}

```rust
pub fn set_owner(&self, owner: Entity) -> Result<()>
```

- 参数：
    - owner : `Entity`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::set_leash_holder` {#Entity.set_leash_holder}

```rust
pub fn set_leash_holder(&self, holder: Entity) -> Result<()>
```

- 参数：
    - holder : `Entity`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::execute_event` {#Entity.execute_event}

```rust
pub fn execute_event(&self, event: &str) -> Result<()>
```

- 参数：
    - event : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Entity::type_name` {#Entity.type_name}

```rust
pub fn type_name(&self) -> Result<String>
```

- 返回值类型：`Result<String>`
- 对应槽位：[`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity::name_tag` {#Entity.name_tag}

```rust
pub fn name_tag(&self) -> Result<String>
```

显示在头顶上的名字。

- 返回值类型：`Result<String>`
- 对应槽位：[`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity::score_tag` {#Entity.score_tag}

```rust
pub fn score_tag(&self) -> Result<String>
```

名字下方的那一行。

- 返回值类型：`Result<String>`
- 对应槽位：[`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity::filtered_name` {#Entity.filtered_name}

```rust
pub fn filtered_name(&self) -> Result<String>
```

经过脏话过滤以后的名字。

- 返回值类型：`Result<String>`
- 对应槽位：[`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Entity::dimension` {#Entity.dimension}

```rust
pub fn dimension(&self) -> Result<i32>
```

- 返回值类型：`Result<i32>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::health` {#Entity.health}

```rust
pub fn health(&self) -> Result<f64>
```

- 返回值类型：`Result<f64>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::max_health` {#Entity.max_health}

```rust
pub fn max_health(&self) -> Result<f64>
```

- 返回值类型：`Result<f64>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::speed` {#Entity.speed}

```rust
pub fn speed(&self) -> Result<f64>
```

当前的移动速度，单位是方块每刻。

- 返回值类型：`Result<f64>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::fall_distance` {#Entity.fall_distance}

```rust
pub fn fall_distance(&self) -> Result<f64>
```

这一次下落已经落了多少格，着地时用来计算摔落伤害。

- 返回值类型：`Result<f64>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::scale` {#Entity.scale}

```rust
pub fn scale(&self) -> Result<f64>
```

体型缩放，1.0 是原始大小。

- 返回值类型：`Result<f64>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::variant` {#Entity.variant}

```rust
pub fn variant(&self) -> Result<i32>
```

`variant` 数据值，含义随实体种类而不同。

- 返回值类型：`Result<i32>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::mark_variant` {#Entity.mark_variant}

```rust
pub fn mark_variant(&self) -> Result<i32>
```

`mark_variant` 数据值，编号和 [`Entity::variant`](entity.md#Entity.variant) 是两套。

- 返回值类型：`Result<i32>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::death_time` {#Entity.death_time}

```rust
pub fn death_time(&self) -> Result<i32>
```

死亡动画已经播放了多少刻。

- 返回值类型：`Result<i32>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_alive` {#Entity.is_alive}

```rust
pub fn is_alive(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_on_ground` {#Entity.is_on_ground}

```rust
pub fn is_on_ground(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_in_water` {#Entity.is_in_water}

```rust
pub fn is_in_water(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_in_lava` {#Entity.is_in_lava}

```rust
pub fn is_in_lava(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_on_fire` {#Entity.is_on_fire}

```rust
pub fn is_on_fire(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_invisible` {#Entity.is_invisible}

```rust
pub fn is_invisible(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_sneaking` {#Entity.is_sneaking}

```rust
pub fn is_sneaking(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_baby` {#Entity.is_baby}

```rust
pub fn is_baby(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_riding` {#Entity.is_riding}

```rust
pub fn is_riding(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_tame` {#Entity.is_tame}

```rust
pub fn is_tame(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_persistent` {#Entity.is_persistent}

```rust
pub fn is_persistent(&self) -> Result<bool>
```

不会因为离玩家太远而被清除。

- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_leashed` {#Entity.is_leashed}

```rust
pub fn is_leashed(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_invulnerable` {#Entity.is_invulnerable}

```rust
pub fn is_invulnerable(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_frozen` {#Entity.is_frozen}

```rust
pub fn is_frozen(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_in_love` {#Entity.is_in_love}

```rust
pub fn is_in_love(&self) -> Result<bool>
```

处在繁殖状态。

- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_in_rain` {#Entity.is_in_rain}

```rust
pub fn is_in_rain(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_in_snow` {#Entity.is_in_snow}

```rust
pub fn is_in_snow(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::is_in_thunderstorm` {#Entity.is_in_thunderstorm}

```rust
pub fn is_in_thunderstorm(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::has_totem` {#Entity.has_totem}

```rust
pub fn has_totem(&self) -> Result<bool>
```

主手或副手拿着不死图腾。

- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::has_passenger` {#Entity.has_passenger}

```rust
pub fn has_passenger(&self) -> Result<bool>
```

- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Entity::vehicle` {#Entity.vehicle}

```rust
pub fn vehicle(&self) -> Result<Option<Entity>>
```

正在骑乘的载具。什么都没骑时返回 `Ok(None)`，只有缺少槽位才是 `Err`。

- 返回值类型：`Result<Option<Entity>>`
- 对应槽位：[`actor_get_vehicle`](../cpp/entity.md#actor_get_vehicle)

### `Entity::first_passenger` {#Entity.first_passenger}

```rust
pub fn first_passenger(&self) -> Result<Option<Entity>>
```

- 返回值类型：`Result<Option<Entity>>`
- 对应槽位：[`actor_get_first_passenger`](../cpp/entity.md#actor_get_first_passenger)

### `Entity::owner` {#Entity.owner}

```rust
pub fn owner(&self) -> Result<Option<Entity>>
```

- 返回值类型：`Result<Option<Entity>>`
- 对应槽位：[`actor_get_owner`](../cpp/entity.md#actor_get_owner)

### `Entity::target` {#Entity.target}

```rust
pub fn target(&self) -> Result<Option<Entity>>
```

- 返回值类型：`Result<Option<Entity>>`
- 对应槽位：[`actor_get_target`](../cpp/entity.md#actor_get_target)

### `Entity::distance_to` {#Entity.distance_to}

```rust
pub fn distance_to(&self, other: Entity) -> Result<f64>
```

两个实体之间的距离。跨维度时宿主返回失败。

- 参数：
    - other : `Entity`
- 返回值类型：`Result<f64>`
- 对应槽位：[`actor_distance_to`](../cpp/entity.md#actor_distance_to)

### `Entity::aabb` {#Entity.aabb}

```rust
pub fn aabb(&self) -> Result<Aabb>
```

碰撞箱。

- 返回值类型：`Result<Aabb>`
- 对应槽位：[`actor_get_aabb`](../cpp/entity.md#actor_get_aabb)

### `Entity::clone_at` {#Entity.clone_at}

```rust
pub fn clone_at(&self, dim: i32, x: f64, y: f64, z: f64) -> Result<Entity>
```

在指定的位置复制出一个。

- 参数：
    - dim : `i32`
    - x : `f64`
    - y : `f64`
    - z : `f64`
- 返回值类型：`Result<Entity>`
- 对应槽位：[`actor_clone`](../cpp/entity.md#actor_clone)

### `Entity::equipped_item` {#Entity.equipped_item}

```rust
pub fn equipped_item(&self, slot: EquipSlot) -> Result<ItemStack>
```

- 参数：
    - slot : `EquipSlot`
- 返回值类型：`Result<ItemStack>`
- 对应槽位：[`actor_get_equipped_item`](../cpp/entity.md#actor_get_equipped_item)

### `Entity::set_equipped_item` {#Entity.set_equipped_item}

```rust
pub fn set_equipped_item(&self, slot: EquipSlot, item: &ItemStack) -> Result<()>
```

- 参数：
    - slot : `EquipSlot`
    - item : `&ItemStack`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_set_equipped_item`](../cpp/entity.md#actor_set_equipped_item)

### `Entity::effects` {#Entity.effects}

```rust
pub fn effects(&self) -> Result<Vec<Effect>>
```

身上的全部状态效果。

- 返回值类型：`Result<Vec<Effect>>`
- 对应槽位：[`actor_get_effects`](../cpp/entity.md#actor_get_effects)

### `Entity::status_flag` {#Entity.status_flag}

```rust
pub fn status_flag(&self, flag_index: i32) -> Result<bool>
```

读取 `ActorFlags` 的一位。

在 ABI 上，这个槽位把「实体不在了」和「这一位为 false」合成了同一个 `false`，这正是契约 §5.2 反对的形状。这个签名已经发布，所以这里只是如实写明。需要区分两者时，先调用 [`Entity::exists`](entity.md#Entity.exists)。

- 参数：
    - flag_index : `i32`
- 返回值类型：`Result<bool>`
- 对应槽位：[`actor_get_status_flag`](../cpp/entity.md#actor_get_status_flag)

### `Entity::set_status_flag` {#Entity.set_status_flag}

```rust
pub fn set_status_flag(&self, flag_index: i32, value: bool) -> Result<()>
```

- 参数：
    - flag_index : `i32`
    - value : `bool`
- 返回值类型：`Result<()>`
- 对应槽位：[`actor_set_status_flag`](../cpp/entity.md#actor_set_status_flag)

### `Entity::trace_ray` {#Entity.trace_ray}

```rust
pub fn trace_ray(
        self,
        max_dist: f32,
        include_actors: bool,
        include_blocks: bool,
    ) -> Result<RayHit>
```

沿着这个实体的视线发射一条射线，以精确坐标报告命中点。

- 参数：
    - max_dist : `f32`
    - include_actors : `bool`
    - include_blocks : `bool`
- 返回值类型：`Result<RayHit>`
- 对应槽位：[`actor_trace_ray`](../cpp/entity.md#actor_trace_ray)

### `Entity::trace_ray_blocks` {#Entity.trace_ray_blocks}

```rust
pub fn trace_ray_blocks(
        self,
        max_dist: f32,
        include_actors: bool,
        include_blocks: bool,
    ) -> Result<RayHit>
```

同上，但以方块格报告命中点，并带上命中的面。

两个槽位都保留，因为它们回答的问题不同：放方块需要格子坐标和面，画粒子需要精确坐标，而把后者向下取整当作前者，在方块边界上会差一格。

- 参数：
    - max_dist : `f32`
    - include_actors : `bool`
    - include_blocks : `bool`
- 返回值类型：`Result<RayHit>`
- 对应槽位：[`edit_trace_ray`](../cpp/edit.md#edit_trace_ray)

## `Effect` {#Effect}

```rust
pub struct Effect {
    pub id: String,
    pub ticks: i32,
    pub amplifier: i32,
    pub visible: bool,
}
```

一个状态效果。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`、`Eq`

## `Aabb` {#Aabb}

```rust
pub struct Aabb {
    pub min: PositionF64,
    pub max: PositionF64,
}
```

实体的轴对齐碰撞箱。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`

## `ActorEntry` {#ActorEntry}

```rust
pub struct ActorEntry {
    pub id: i64,
    pub type_name: String,
}
```

`list_actors` 报告的一项。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`、`Eq`
