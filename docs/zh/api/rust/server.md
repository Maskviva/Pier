# levilamina::server · 服务器

服务器运行时的控制：冻结、单步和加速刻，以及按子系统拆分的性能采样。

这些不放在 [`crate::host`](host.md) 里：那一层说的是宿主本身，即调度、执行命令和操作系统，换一个游戏也成立；而刻是世界模拟的节奏，是游戏里的概念。

**钩子装上以后永不卸下**

钩子在第一次调用时才安装，之后一直留着。原因是控制调用可能来自正在刻内部执行的命令处理函数，在那里卸下钩子并不安全。空闲时的开销是每帧一次可以预测的分支。

**冻结时玩家仍然能动**

冻结会停下生物、方块、红石和时间。移动由客户端决定，网络在关卡的刻之外运行，所以玩家照样能走动、聊天。

## `Server` {#Server}

```rust
pub struct Server(/* private */);
```

服务器运行时的门面。零大小。

- 实现的 trait：`Clone`、`Copy`

### `Server::get` {#Server.get}

```rust
pub fn get() -> Server
```

- 返回值类型：`Server`

### `Server::set_tick_freeze` {#Server.set_tick_freeze}

```rust
pub fn set_tick_freeze(&self, on: bool) -> Result<()>
```

冻结或解冻世界。

- 参数：
    - on : `bool`
- 返回值类型：`Result<()>`
- 对应槽位：[`tick_freeze`](../cpp/server.md#tick_freeze)

### `Server::step_ticks` {#Server.step_ticks}

```rust
pub fn step_ticks(&self, n: u32) -> Result<()>
```

只在冻结时有效：再放过 `n` 帧。

- 参数：
    - n : `u32`
- 返回值类型：`Result<()>`
- 对应槽位：[`tick_step`](../cpp/server.md#tick_step)

### `Server::set_tick_warp` {#Server.set_tick_warp}

```rust
pub fn set_tick_warp(&self, factor: f64) -> Result<()>
```

时间加速。`0 < factor <= 100`，小于 1 是慢动作，1.0 是正常速度。

- 参数：
    - factor : `f64`
- 返回值类型：`Result<()>`
- 对应槽位：[`tick_warp`](../cpp/server.md#tick_warp)

### `Server::begin_profile` {#Server.begin_profile}

```rust
pub fn begin_profile(&self, ticks: u32) -> Result<()>
```

开启一个 `ticks` 帧的采样窗口，1 到 12000。同一时间只有一个窗口。

- 参数：
    - ticks : `u32`
- 返回值类型：`Result<()>`
- 对应槽位：[`profile_begin`](../cpp/server.md#profile_begin)

### `Server::take_profile` {#Server.take_profile}

```rust
pub fn take_profile(&self) -> Result<Option<NbtValue>>
```

取回采样报告。

还在采样时返回 `Ok(None)`，一个窗口只会成功一次。

各项时间包含了嵌套的部分，所以要并排对照着看，不要相加。

- 返回值类型：`Result<Option<NbtValue>>`
- 对应槽位：[`profile_take`](../cpp/server.md#profile_take)

### `Server::is_sim_paused` {#Server.is_sim_paused}

```rust
pub fn is_sim_paused(&self) -> Result<bool>
```

模拟是否暂停。

- 返回值类型：`Result<bool>`
- 对应槽位：[`get_sim_paused`](../cpp/server.md#get_sim_paused)

### `Server::tick_delta_time` {#Server.tick_delta_time}

```rust
pub fn tick_delta_time(&self) -> Result<f64>
```

上一帧实际经过的时间，单位秒。20 TPS 时是 0.05。

这是帧的周期，包含休眠，和服务器计算这一刻所花的时间是两回事：空闲的服务器上它一样读到 0.05。保留它，是给想要引擎原始值的调用方用的；监控要的数字是 [`Server::tps`](server.md#Server.tps) 和 [`Server::mspt`](server.md#Server.mspt)。

宿主读不出来时返回 -1.0，这里转成 `Err`：负的帧时长会让调用方在不知不觉中算出负的 TPS。

- 返回值类型：`Result<f64>`
- 对应槽位：[`get_tick_delta_time`](../cpp/server.md#get_tick_delta_time)

### `Server::tps` {#Server.tps}

```rust
pub fn tps(&self) -> Result<f64>
```

最近 5 秒实际时间里每秒运行的刻数。

测量方法是数真正运行过的 `Level::tick` 调用次数，再除以经过的时间，所以在刻加速（读数高于 20）、刻冻结（读数为 0）和卡顿（读数低于 20）时都准确。一帧周期的倒数和这个数是两回事：空闲的服务器上，大约一半的帧算出来高于 20，世界冻结时它仍然是 20。

宿主采样到第一帧之前返回 `Err`，启动后要等两刻。

- 返回值类型：`Result<f64>`
- 对应槽位：[`get_tps`](../cpp/server.md#get_tps)

### `Server::tps_over` {#Server.tps_over}

```rust
pub fn tps_over(&self, window_seconds: u32) -> Result<f64>
```

和 [`Server::tps`](server.md#Server.tps) 一样，但窗口是 `window_seconds` 秒（1 到 60，超出由宿主截断）。

- 参数：
    - window_seconds : `u32`
- 返回值类型：`Result<f64>`
- 对应槽位：[`get_tps`](../cpp/server.md#get_tps)

### `Server::mspt` {#Server.mspt}

```rust
pub fn mspt(&self) -> Result<f64>
```

最近 5 秒里，每一刻计算所花的毫秒数的平均值。

它不包括帧与帧之间空闲的休眠：健康的服务器读数是几毫秒，只有满负荷时才接近 50。`tick_delta_time() * 1000` 和这个数是两回事，那是帧的周期，空闲的服务器上大约是 50。

- 返回值类型：`Result<f64>`
- 对应槽位：[`get_mspt`](../cpp/server.md#get_mspt)

### `Server::mspt_over` {#Server.mspt_over}

```rust
pub fn mspt_over(&self, window_seconds: u32) -> Result<f64>
```

和 [`Server::mspt`](server.md#Server.mspt) 一样，但窗口是 `window_seconds` 秒（1 到 60，超出由宿主截断）。

- 参数：
    - window_seconds : `u32`
- 返回值类型：`Result<f64>`
- 对应槽位：[`get_mspt`](../cpp/server.md#get_mspt)
