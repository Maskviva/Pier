# levilamina::server

Server runtime control: freezing, stepping and warping ticks, plus performance sampling broken
down by subsystem.
These do not live in [`crate::host`](host.md): that layer speaks about the host itself, meaning
scheduling, executing commands and the operating system, and holds for another game, while a
tick is the rhythm of the world simulation and is a game concept.

**A hook is installed and never removed**

The detour installs lazily on the first call and stays. The reason is that a control call may
come from a command handler executing inside the tick, where removing a hook is unsafe. The idle
cost is one predictable branch per frame.

**Players still move while frozen**

Freezing stops mobs, blocks, redstone and time. Movement is client authoritative and the network
runs outside the level tick, so players keep walking and chatting.

## `Server` {#Server}

```rust
pub struct Server(/* private */);
```

The server runtime facade. Zero sized.

- Implements: `Clone`, `Copy`

### `Server::get` {#Server.get}

```rust
pub fn get() -> Server
```

- Return type: `Server`

### `Server::set_tick_freeze` {#Server.set_tick_freeze}

```rust
pub fn set_tick_freeze(&self, on: bool) -> Result<()>
```

Freezes or unfreezes the world.

- Parameters:
    - on : `bool`
- Return type: `Result<()>`
- Slots: [`tick_freeze`](../cpp/server.md#tick_freeze)

### `Server::step_ticks` {#Server.step_ticks}

```rust
pub fn step_ticks(&self, n: u32) -> Result<()>
```

Valid only while frozen: lets `n` more frames through.

- Parameters:
    - n : `u32`
- Return type: `Result<()>`
- Slots: [`tick_step`](../cpp/server.md#tick_step)

### `Server::set_tick_warp` {#Server.set_tick_warp}

```rust
pub fn set_tick_warp(&self, factor: f64) -> Result<()>
```

The time warp. `0 < factor <= 100`, below 1 is slow motion and 1.0 is normal.

- Parameters:
    - factor : `f64`
- Return type: `Result<()>`
- Slots: [`tick_warp`](../cpp/server.md#tick_warp)

### `Server::begin_profile` {#Server.begin_profile}

```rust
pub fn begin_profile(&self, ticks: u32) -> Result<()>
```

Opens a sampling window of `ticks` frames, from 1 to 12000. Only one window at a time.

- Parameters:
    - ticks : `u32`
- Return type: `Result<()>`
- Slots: [`profile_begin`](../cpp/server.md#profile_begin)

### `Server::take_profile` {#Server.take_profile}

```rust
pub fn take_profile(&self) -> Result<Option<NbtValue>>
```

Takes the sampling report.

Sampling still running gives `Ok(None)`, and one window succeeds exactly once.

The per-item times are inclusive of nesting, so they are read side by side and not
summed.

- Return type: `Result<Option<NbtValue>>`
- Slots: [`profile_take`](../cpp/server.md#profile_take)

### `Server::is_sim_paused` {#Server.is_sim_paused}

```rust
pub fn is_sim_paused(&self) -> Result<bool>
```

Whether the simulation is paused.

- Return type: `Result<bool>`
- Slots: [`get_sim_paused`](../cpp/server.md#get_sim_paused)

### `Server::tick_delta_time` {#Server.tick_delta_time}

```rust
pub fn tick_delta_time(&self) -> Result<f64>
```

The wall-clock period of the last frame in seconds. At 20 TPS it is 0.05.

This is the frame period, sleep included, and not the time the server spent computing
the tick: on an idle server it reads 0.05 all the same. It is kept for callers that want
the raw engine value; [`Server::tps`](server.md#Server.tps) and [`Server::mspt`](server.md#Server.mspt) are the numbers a monitor
wants.

The host returns -1.0 when it cannot read it, translated here into an `Err`: a negative
frame duration would have a caller compute a negative TPS without noticing.

- Return type: `Result<f64>`
- Slots: [`get_tick_delta_time`](../cpp/server.md#get_tick_delta_time)

### `Server::tps` {#Server.tps}

```rust
pub fn tps(&self) -> Result<f64>
```

Ticks per second over the last 5 seconds of wall clock.

Measured by counting the `Level::tick` calls that really run and dividing by elapsed
time, so it stays right under the tick warp (reads above 20), the tick freeze (reads
0) and lag (reads below 20). The reciprocal of one frame period is not this number:
it reads above 20 on about half the frames of an idle server and reads 20 while the
world is frozen.

`Err` until the host has sampled its first frame, which takes two ticks after start.

- Return type: `Result<f64>`
- Slots: [`get_tps`](../cpp/server.md#get_tps)

### `Server::tps_over` {#Server.tps_over}

```rust
pub fn tps_over(&self, window_seconds: u32) -> Result<f64>
```

As [`Server::tps`](server.md#Server.tps), over a window of `window_seconds` (1..=60, clipped by the host).

- Parameters:
    - window_seconds : `u32`
- Return type: `Result<f64>`
- Slots: [`get_tps`](../cpp/server.md#get_tps)

### `Server::mspt` {#Server.mspt}

```rust
pub fn mspt(&self) -> Result<f64>
```

Milliseconds spent computing each tick, averaged over the last 5 seconds.

This excludes the idle sleep between frames: a healthy server reads a few
milliseconds and only approaches 50 when it is saturated. `tick_delta_time() * 1000`
is not this number, it is the frame period and reads about 50 on an idle server.

- Return type: `Result<f64>`
- Slots: [`get_mspt`](../cpp/server.md#get_mspt)

### `Server::mspt_over` {#Server.mspt_over}

```rust
pub fn mspt_over(&self, window_seconds: u32) -> Result<f64>
```

As [`Server::mspt`](server.md#Server.mspt), over a window of `window_seconds` (1..=60, clipped by the host).

- Parameters:
    - window_seconds : `u32`
- Return type: `Result<f64>`
- Slots: [`get_mspt`](../cpp/server.md#get_mspt)
