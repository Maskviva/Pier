# levilamina::sim · 模拟玩家

模拟玩家：由服务器构造的一个真正的 `ServerPlayer`。

构造好以后，所有按名字操作玩家的接口都适用于它：传送、生命值、物品栏、踢出。只有它特有的「让它做点什么」这一族动作放在这里。

**动作经过一个复用的槽位**

`sim_do` 接收一个动作名加上参数 SNBT，动作表在宿主那边扩展，不占新的表槽位。代价是动作名拼错只能在运行时发现，所以这里把每个动作包成了一个具名方法。

**永远不会操纵真实玩家**

宿主以 `isSimulatedPlayer()` 把关，对真实玩家的调用会直接失败。

## 函数 {#functions}

### `sim::spawn` {#fn.spawn}

```rust
pub fn spawn(name: &str, dim: i32, x: f64, y: f64, z: f64) -> Result<SimPlayer>
```

构造一个模拟玩家。名字被一个真实玩家占用时失败。

- 参数：
    - name : `&str`
    - dim : `i32`
    - x : `f64`
    - y : `f64`
    - z : `f64`
- 返回值类型：`Result<SimPlayer>`
- 对应槽位：[`sim_spawn`](../cpp/sim.md#sim_spawn)

### `sim::try_list` {#fn.try_list}

```rust
pub fn try_list() -> Result<Vec<SimPlayer>>
```

当前活着的模拟玩家；宿主没有这个槽位时返回 `Err`，空列表会把这一点掩盖掉。

模拟玩家会随存档在重启后保留，内存里的句柄不会，所以重启后靠这个找回它们。

- 返回值类型：`Result<Vec<SimPlayer>>`
- 对应槽位：[`sim_list`](../cpp/sim.md#sim_list)

### `sim::list` {#fn.list}

!!! warning "已弃用（自 26.51.2 起）"

    请用 `try_list`：宿主没有 `sim_list` 槽位时，这个函数回答的是空列表

```rust
pub fn list() -> Vec<SimPlayer>
```

当前活着的模拟玩家；宿主列不出来时返回空列表。

- 返回值类型：`Vec<SimPlayer>`
- 对应槽位：[`sim_list`](../cpp/sim.md#sim_list)

## `SimPlayer` {#SimPlayer}

```rust
pub struct SimPlayer {
    // private fields
}
```

一个模拟玩家。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`、`Eq`、`Display`

### `SimPlayer::by_name` {#SimPlayer.by_name}

```rust
pub fn by_name(name: impl Into<String>) -> SimPlayer
```

按名字接上一个已经存在的模拟玩家。它不检查是否真的存在；要检查请用 [`SimPlayer::try_is_simulated`](sim.md#SimPlayer.try_is_simulated)。

- 参数：
    - name : `impl Into<String>`
- 返回值类型：`SimPlayer`

### `SimPlayer::name` {#SimPlayer.name}

```rust
pub fn name(&self) -> &str
```

- 返回值类型：`&str`

### `SimPlayer::player` {#SimPlayer.player}

```rust
pub fn player(&self) -> Player
```

把它当作普通玩家使用，获得完整的玩家接口。

- 返回值类型：`Player`

### `SimPlayer::try_is_simulated` {#SimPlayer.try_is_simulated}

```rust
pub fn try_is_simulated(&self) -> Result<bool>
```

这个名字当前是否指向一个活着的模拟玩家；宿主分辨不出来时返回 `Err`。

- 返回值类型：`Result<bool>`
- 对应槽位：[`sim_is`](../cpp/sim.md#sim_is)

### `SimPlayer::is_simulated` {#SimPlayer.is_simulated}

!!! warning "已弃用（自 26.51.2 起）"

    请用 `try_is_simulated`：宿主分辨不出来时，这个函数回答 false

```rust
pub fn is_simulated(&self) -> bool
```

这个名字当前是否指向一个活着的模拟玩家；宿主分辨不出来时返回 `false`。

- 返回值类型：`bool`
- 对应槽位：[`sim_is`](../cpp/sim.md#sim_is)

### `SimPlayer::act` {#SimPlayer.act}

```rust
pub fn act(&self, verb: &str, args_snbt: &str) -> Result<()>
```

执行一个动作。参数是 SNBT，没有参数时传 `"{}"`。

动作表在宿主那边，这里的具名方法是它的门面。宿主不认识的动作、形状不对的参数，以及目标不是模拟玩家，都会返回 `Err`。

- 参数：
    - verb : `&str`
    - args_snbt : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::despawn` {#SimPlayer.despawn}

```rust
pub fn despawn(self) -> Result<()>
```

- 返回值类型：`Result<()>`
- 对应槽位：[`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::stop` {#SimPlayer.stop}

```rust
pub fn stop(&self) -> Result<()>
```

- 返回值类型：`Result<()>`
- 对应槽位：[`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::jump` {#SimPlayer.jump}

```rust
pub fn jump(&self) -> Result<()>
```

- 返回值类型：`Result<()>`
- 对应槽位：[`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::attack` {#SimPlayer.attack}

```rust
pub fn attack(&self) -> Result<()>
```

- 返回值类型：`Result<()>`
- 对应槽位：[`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::interact` {#SimPlayer.interact}

```rust
pub fn interact(&self) -> Result<()>
```

- 返回值类型：`Result<()>`
- 对应槽位：[`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::use_item` {#SimPlayer.use_item}

```rust
pub fn use_item(&self) -> Result<()>
```

- 返回值类型：`Result<()>`
- 对应槽位：[`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::drop_selected` {#SimPlayer.drop_selected}

```rust
pub fn drop_selected(&self) -> Result<()>
```

- 返回值类型：`Result<()>`
- 对应槽位：[`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::respawn` {#SimPlayer.respawn}

```rust
pub fn respawn(&self) -> Result<()>
```

- 返回值类型：`Result<()>`
- 对应槽位：[`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::stop_destroying` {#SimPlayer.stop_destroying}

```rust
pub fn stop_destroying(&self) -> Result<()>
```

- 返回值类型：`Result<()>`
- 对应槽位：[`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::move_to` {#SimPlayer.move_to}

```rust
pub fn move_to(&self, x: f64, y: f64, z: f64, speed: f64, face_target: bool) -> Result<()>
```

径直走过去。`face_target` 决定走的时候是否面向目标。

- 参数：
    - x : `f64`
    - y : `f64`
    - z : `f64`
    - speed : `f64`
    - face_target : `bool`
- 返回值类型：`Result<()>`
- 对应槽位：[`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::navigate_to` {#SimPlayer.navigate_to}

```rust
pub fn navigate_to(&self, x: f64, y: f64, z: f64, speed: f64) -> Result<()>
```

寻路过去。和 [`SimPlayer::move_to`](sim.md#SimPlayer.move_to) 不同，它会绕开障碍物。

- 参数：
    - x : `f64`
    - y : `f64`
    - z : `f64`
    - speed : `f64`
- 返回值类型：`Result<()>`
- 对应槽位：[`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::look_at` {#SimPlayer.look_at}

```rust
pub fn look_at(&self, x: f64, y: f64, z: f64) -> Result<()>
```

- 参数：
    - x : `f64`
    - y : `f64`
    - z : `f64`
- 返回值类型：`Result<()>`
- 对应槽位：[`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::destroy_block` {#SimPlayer.destroy_block}

```rust
pub fn destroy_block(&self, x: i32, y: i32, z: i32, face: i32) -> Result<()>
```

挖掉一个方块。`face` 是面，默认为 1，也就是上面。

- 参数：
    - x : `i32`
    - y : `i32`
    - z : `i32`
    - face : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::destroy_look_at` {#SimPlayer.destroy_look_at}

```rust
pub fn destroy_look_at(&self, hand: f64) -> Result<()>
```

挖掉视线上的那个方块。`hand` 是触及距离，单位是方块。

- 参数：
    - hand : `f64`
- 返回值类型：`Result<()>`
- 对应槽位：[`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::interact_block` {#SimPlayer.interact_block}

```rust
pub fn interact_block(&self, x: i32, y: i32, z: i32, face: i32) -> Result<()>
```

- 参数：
    - x : `i32`
    - y : `i32`
    - z : `i32`
    - face : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::set_sneaking` {#SimPlayer.set_sneaking}

```rust
pub fn set_sneaking(&self, on: bool) -> Result<()>
```

- 参数：
    - on : `bool`
- 返回值类型：`Result<()>`
- 对应槽位：[`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::set_flying` {#SimPlayer.set_flying}

```rust
pub fn set_flying(&self, on: bool) -> Result<()>
```

- 参数：
    - on : `bool`
- 返回值类型：`Result<()>`
- 对应槽位：[`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::chat` {#SimPlayer.chat}

```rust
pub fn chat(&self, msg: &str) -> Result<()>
```

- 参数：
    - msg : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`sim_do`](../cpp/sim.md#sim_do)
