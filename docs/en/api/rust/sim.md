# levilamina::sim

Simulated players: a real `ServerPlayer` the server builds.

Once built, every by-name player API applies to it: teleporting, health, inventory and
kicking. Only the family of make-it-do-something actions unique to it lives here.

**Actions go through one multiplexed slot**

`sim_do` takes a verb plus argument SNBT, and the verb table grows on the host side
without taking a new table slot. The cost is that a misspelled verb is reported only at
runtime, so each verb is wrapped here as a named method.

**A real player is never driven**

The host gates on `isSimulatedPlayer()` and a call against a real player fails outright.

## Functions {#functions}

### `sim::spawn` {#fn.spawn}

```rust
pub fn spawn(name: &str, dim: i32, x: f64, y: f64, z: f64) -> Result<SimPlayer>
```

Builds a simulated player. It fails when the name is taken by a real player.

- Parameters:
    - name : `&str`
    - dim : `i32`
    - x : `f64`
    - y : `f64`
    - z : `f64`
- Return type: `Result<SimPlayer>`
- Slots: [`sim_spawn`](../cpp/sim.md#sim_spawn)

### `sim::try_list` {#fn.try_list}

```rust
pub fn try_list() -> Result<Vec<SimPlayer>>
```

The simulated players currently alive, and `Err` for a host without the slot, which an
empty list would hide.

A simulated player survives a restart with the save while an in-memory handle does not,
so this is how they are found again after a restart.

- Return type: `Result<Vec<SimPlayer>>`
- Slots: [`sim_list`](../cpp/sim.md#sim_list)

### `sim::list` {#fn.list}

!!! warning "Deprecated since 26.51.2"

    use try_list: this answers an empty list when the host has no sim_list slot

```rust
pub fn list() -> Vec<SimPlayer>
```

The simulated players currently alive, and an empty list when the host cannot list them.

- Return type: `Vec<SimPlayer>`
- Slots: [`sim_list`](../cpp/sim.md#sim_list)

## `SimPlayer` {#SimPlayer}

```rust
pub struct SimPlayer {
    // private fields
}
```

One simulated player.

- Implements: `Debug`, `Clone`, `PartialEq`, `Eq`, `Display`

### `SimPlayer::by_name` {#SimPlayer.by_name}

```rust
pub fn by_name(name: impl Into<String>) -> SimPlayer
```

Attaches by name to a simulated player that already exists. It does not check whether
it really exists; [`SimPlayer::try_is_simulated`](sim.md#SimPlayer.try_is_simulated) does that.

- Parameters:
    - name : `impl Into<String>`
- Return type: `SimPlayer`

### `SimPlayer::name` {#SimPlayer.name}

```rust
pub fn name(&self) -> &str
```

- Return type: `&str`

### `SimPlayer::player` {#SimPlayer.player}

```rust
pub fn player(&self) -> Player
```

Uses it as an ordinary player, giving the full player API.

- Return type: `Player`

### `SimPlayer::try_is_simulated` {#SimPlayer.try_is_simulated}

```rust
pub fn try_is_simulated(&self) -> Result<bool>
```

Whether this name currently points at a live simulated player, and `Err` when the
host cannot tell.

- Return type: `Result<bool>`
- Slots: [`sim_is`](../cpp/sim.md#sim_is)

### `SimPlayer::is_simulated` {#SimPlayer.is_simulated}

!!! warning "Deprecated since 26.51.2"

    use try_is_simulated: this answers false when the host cannot tell

```rust
pub fn is_simulated(&self) -> bool
```

Whether this name currently points at a live simulated player, and `false` when the
host cannot tell.

- Return type: `bool`
- Slots: [`sim_is`](../cpp/sim.md#sim_is)

### `SimPlayer::act` {#SimPlayer.act}

```rust
pub fn act(&self, verb: &str, args_snbt: &str) -> Result<()>
```

Runs one verb. The argument is SNBT, and `"{}"` is passed when there is none.

The verb table is on the host side and the named methods here are a facade over it. A
verb the host does not recognize, an argument of the wrong shape, and a target that is
not a simulated player are all an `Err`.

- Parameters:
    - verb : `&str`
    - args_snbt : `&str`
- Return type: `Result<()>`
- Slots: [`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::despawn` {#SimPlayer.despawn}

```rust
pub fn despawn(self) -> Result<()>
```

- Return type: `Result<()>`
- Slots: [`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::stop` {#SimPlayer.stop}

```rust
pub fn stop(&self) -> Result<()>
```

- Return type: `Result<()>`
- Slots: [`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::jump` {#SimPlayer.jump}

```rust
pub fn jump(&self) -> Result<()>
```

- Return type: `Result<()>`
- Slots: [`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::attack` {#SimPlayer.attack}

```rust
pub fn attack(&self) -> Result<()>
```

- Return type: `Result<()>`
- Slots: [`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::interact` {#SimPlayer.interact}

```rust
pub fn interact(&self) -> Result<()>
```

- Return type: `Result<()>`
- Slots: [`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::use_item` {#SimPlayer.use_item}

```rust
pub fn use_item(&self) -> Result<()>
```

- Return type: `Result<()>`
- Slots: [`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::drop_selected` {#SimPlayer.drop_selected}

```rust
pub fn drop_selected(&self) -> Result<()>
```

- Return type: `Result<()>`
- Slots: [`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::respawn` {#SimPlayer.respawn}

```rust
pub fn respawn(&self) -> Result<()>
```

- Return type: `Result<()>`
- Slots: [`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::stop_destroying` {#SimPlayer.stop_destroying}

```rust
pub fn stop_destroying(&self) -> Result<()>
```

- Return type: `Result<()>`
- Slots: [`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::move_to` {#SimPlayer.move_to}

```rust
pub fn move_to(&self, x: f64, y: f64, z: f64, speed: f64, face_target: bool) -> Result<()>
```

Walks straight there. `face_target` decides whether it faces the target while walking.

- Parameters:
    - x : `f64`
    - y : `f64`
    - z : `f64`
    - speed : `f64`
    - face_target : `bool`
- Return type: `Result<()>`
- Slots: [`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::navigate_to` {#SimPlayer.navigate_to}

```rust
pub fn navigate_to(&self, x: f64, y: f64, z: f64, speed: f64) -> Result<()>
```

Paths there. Unlike [`SimPlayer::move_to`](sim.md#SimPlayer.move_to) it goes around obstacles.

- Parameters:
    - x : `f64`
    - y : `f64`
    - z : `f64`
    - speed : `f64`
- Return type: `Result<()>`
- Slots: [`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::look_at` {#SimPlayer.look_at}

```rust
pub fn look_at(&self, x: f64, y: f64, z: f64) -> Result<()>
```

- Parameters:
    - x : `f64`
    - y : `f64`
    - z : `f64`
- Return type: `Result<()>`
- Slots: [`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::destroy_block` {#SimPlayer.destroy_block}

```rust
pub fn destroy_block(&self, x: i32, y: i32, z: i32, face: i32) -> Result<()>
```

Mines one block. `face` is the face, defaulting to 1, meaning up.

- Parameters:
    - x : `i32`
    - y : `i32`
    - z : `i32`
    - face : `i32`
- Return type: `Result<()>`
- Slots: [`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::destroy_look_at` {#SimPlayer.destroy_look_at}

```rust
pub fn destroy_look_at(&self, hand: f64) -> Result<()>
```

Mines the one in the line of sight. `hand` is the reach, in blocks.

- Parameters:
    - hand : `f64`
- Return type: `Result<()>`
- Slots: [`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::interact_block` {#SimPlayer.interact_block}

```rust
pub fn interact_block(&self, x: i32, y: i32, z: i32, face: i32) -> Result<()>
```

- Parameters:
    - x : `i32`
    - y : `i32`
    - z : `i32`
    - face : `i32`
- Return type: `Result<()>`
- Slots: [`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::set_sneaking` {#SimPlayer.set_sneaking}

```rust
pub fn set_sneaking(&self, on: bool) -> Result<()>
```

- Parameters:
    - on : `bool`
- Return type: `Result<()>`
- Slots: [`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::set_flying` {#SimPlayer.set_flying}

```rust
pub fn set_flying(&self, on: bool) -> Result<()>
```

- Parameters:
    - on : `bool`
- Return type: `Result<()>`
- Slots: [`sim_do`](../cpp/sim.md#sim_do)

### `SimPlayer::chat` {#SimPlayer.chat}

```rust
pub fn chat(&self, msg: &str) -> Result<()>
```

- Parameters:
    - msg : `&str`
- Return type: `Result<()>`
- Slots: [`sim_do`](../cpp/sim.md#sim_do)
