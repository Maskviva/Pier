# levilamina::Host · 宿主与任务

宿主和系统本身：不涉及任何游戏概念的能力。

放在这里的标准，是它说的是宿主还是世界。运行状态、把任务交回服务器线程、执行命令、问操作系统叫什么名字，换一个游戏也都成立；玩家、方块和物品就不行。

## `Host` {#Host}

```rust
pub struct Host(/* private */);
```

宿主门面。零大小，可以随意 `Copy`。

- 实现的 trait：`Clone`、`Copy`

### `Host::get` {#Host.get}

```rust
pub fn get() -> Host
```

- 返回值类型：`Host`

### `Host::gaming_status` {#Host.gaming_status}

```rust
pub fn gaming_status(&self) -> GamingStatus
```

服务器处在哪个阶段。ABI 把它标为线程安全，所以任何线程都可以问。

- 返回值类型：`GamingStatus`
- 对应槽位：[`gaming_status`](../cpp/core.md#gaming_status)

### `Host::schedule` {#Host.schedule}

```rust
pub fn schedule(&self, f: impl FnOnce() + Send + 'static) -> Result<TaskId>
```

把一件工作交回服务器线程，尽快运行。线程安全。

闭包被装箱交给宿主，在回调里取回并运行，运行完立刻释放。所有权自始至终留在模组这一边，符合契约 §3：没有所有权跨过边界，传过去的是一个不透明的指针，宿主只是原样交回来。

它走的是带模组句柄的 `schedule_for`，没有用不带主人的 `schedule`。不带主人的槽位上的任务，在模组卸载以后照样会触发，跳进一段已经卸载的代码。带句柄的任务由宿主按模组记账，卸载时整批丢弃，并发出一条警告，建议你自己取消它们。返回的 [`TaskId`](root.md#TaskId) 可以交给 [`Host::cancel`](host.md#Host.cancel)。

- 参数：
    - f : `impl FnOnce() + Send + 'static`
- 返回值类型：`Result<TaskId>`
- 对应槽位：[`schedule_for`](../cpp/core.md#schedule_for)

### `Host::schedule_after` {#Host.schedule_after}

```rust
pub fn schedule_after(
        &self,
        delay: std::time::Duration,
        f: impl FnOnce() + Send + 'static,
    ) -> Result<TaskId>
```

同上，但在 `delay` 之后运行。线程安全。

它接收 `Duration`，不接收裸的毫秒数：`schedule_after(5, ...)` 里的 5 是五毫秒还是五秒，只有参数名能回答，而参数名在调用处看不见。把单位放进类型里，调用处自己就带着答案。

ABI 那一侧用毫秒，所以这里换算一次。超过 `u64` 毫秒的时长会被截断到最大值，不会回绕成一个小数字，否则「一年后运行」就成了「立刻运行」。

- 参数：
    - delay : `std::time::Duration`
    - f : `impl FnOnce() + Send + 'static`
- 返回值类型：`Result<TaskId>`
- 对应槽位：[`schedule_after_for`](../cpp/core.md#schedule_after_for)

### `Host::cancel` {#Host.cancel}

```rust
pub fn cancel(&self, task: TaskId) -> bool
```

取消一个还没运行的任务。已经运行过、已经取消过，或者不属于这个模组的票据返回 `false`。

注意取消只是作废票据。闭包本身既不会被调用，也不会在宿主拆除时被释放，要等进程结束才回收。想避免泄漏，就不要在热路径上大量地排任务再取消。

- 参数：
    - task : `TaskId`
- 返回值类型：`bool`
- 对应槽位：[`schedule_cancel`](../cpp/core.md#schedule_cancel)

### `Host::try_pending_tasks` {#Host.try_pending_tasks}

```rust
pub fn try_pending_tasks(&self) -> Result<u32>
```

这个模组名下还有多少任务没运行。适合在 `on_unload` 里断言它为 0：宿主数不出来时返回 `Err`，断言拿到的是一个错误，没有拿到 0。

- 返回值类型：`Result<u32>`
- 对应槽位：[`schedule_pending_count`](../cpp/core.md#schedule_pending_count)

### `Host::pending_tasks` {#Host.pending_tasks}

!!! warning "已弃用（自 26.51.2 起）"

    请用 `try_pending_tasks`：宿主数不出来时，这个函数回答 0，正好能通过它本该把关的那个断言

```rust
pub fn pending_tasks(&self) -> u32
```

这个模组名下还有多少任务没运行；宿主数不出来时返回 0。

- 返回值类型：`u32`
- 对应槽位：[`schedule_pending_count`](../cpp/core.md#schedule_pending_count)

### `Host::execute_command` {#Host.execute_command}

```rust
pub fn execute_command(&self, cmd: &str) -> Result<String>
```

以控制台的身份执行一条命令，返回它的输出。

两种 `Err` 是分开的：缺少槽位，意思是宿主太旧；命令本身执行失败，这时 `success == false`，输出里是错误信息。

- 参数：
    - cmd : `&str`
- 返回值类型：`Result<String>`
- 对应槽位：[`execute_command`](../cpp/commands.md#execute_command)

### `Host::list_events` {#Host.list_events}

```rust
pub fn list_events(&self) -> Vec<String>
```

列出宿主当前能解析的所有事件 id。

订阅失败时打印这份列表，比光秃秃的一句「订阅失败」有用得多（契约 §5.3：日志要回答该怎么办）。

- 返回值类型：`Vec<String>`
- 对应槽位：[`list_events`](../cpp/events.md#list_events)

### `Host::sys_info` {#Host.sys_info}

```rust
pub fn sys_info(&self, prop: i32) -> Result<String>
```

操作系统层面的信息。`prop` 取自 `sys::PIER_SYS_*`。

- 参数：
    - prop : `i32`
- 返回值类型：`Result<String>`
- 对应槽位：[`sys_info_str`](../cpp/server.md#sys_info_str)

### `Host::server_info` {#Host.server_info}

```rust
pub fn server_info(&self, prop: i32) -> Result<String>
```

服务器层面的信息。`prop` 取自 `sys::PIER_SRV_*`。

- 参数：
    - prop : `i32`
- 返回值类型：`Result<String>`
- 对应槽位：[`server_info_str`](../cpp/server.md#server_info_str)

### `Host::protocol_version` {#Host.protocol_version}

```rust
pub fn protocol_version(&self) -> Result<u32>
```

服务器的网络协议版本，根据正在运行的构建的游戏版本推出。

**宿主不认识这个版本，或者它是任何预发布构建时，宿主会让这个调用失败。** `Err` 表示无法确定，必须和协议版本为 0 分开（契约 §5.2），所以它返回 `Result`。

失败时，读 `Self::game_sem_version`，自己去映射。不要去拿 `Self::level_protocol_version`：那是存档的标记，在任何比服务器旧的世界上，它都是另一个数。

- 返回值类型：`Result<u32>`
- 对应槽位：[`server_info_str`](../cpp/server.md#server_info_str)

### `Host::level_protocol_version` {#Host.level_protocol_version}

```rust
pub fn level_protocol_version(&self) -> Result<u32>
```

**关卡**最后一次被写入时所用的协议（`LevelData::mNetworkVersion`）。

它说明存档有多旧，和服务器使用的协议无关；把它当成服务器的协议，正是拆出这一对访问函数要防止的错误。

- 返回值类型：`Result<u32>`
- 对应槽位：[`server_info_str`](../cpp/server.md#server_info_str)

### `Host::game_sem_version` {#Host.game_sem_version}

```rust
pub fn game_sem_version(&self) -> Result<String>
```

正在运行的构建的 `"major.minor.patch"`，取自 `CurrentGameSemVersion`。

`Self::protocol_version` 不认识某个构建时，以它为依据自带一张版本到协议的对照表：支持一个昨天才发布的 BDS，不应该卡在等 Pier 发新版上。

- 返回值类型：`Result<String>`
- 对应槽位：[`server_info_str`](../cpp/server.md#server_info_str)

### `Host::bds_version` {#Host.bds_version}

```rust
pub fn bds_version(&self) -> Result<String>
```

BDS 的版本字符串，取自 `Common::getGameVersionString`。

- 返回值类型：`Result<String>`
- 对应槽位：[`server_info_str`](../cpp/server.md#server_info_str)

### `Host::packets` {#Host.packets}

```rust
pub fn packets(&self) -> crate::packet::Packets
```

- 返回值类型：`crate::packet::Packets`

### `Host::current_tick` {#Host.current_tick}

```rust
pub fn current_tick(&self) -> Option<u64>
```

- 返回值类型：`Option<u64>`
- 对应槽位：[`get_current_tick`](../cpp/server.md#get_current_tick)

### `Host::player_count` {#Host.player_count}

```rust
pub fn player_count(&self) -> Option<i32>
```

- 返回值类型：`Option<i32>`
- 对应槽位：[`get_player_count`](../cpp/server.md#get_player_count)

### `Host::env` {#Host.env}

```rust
pub fn env(&self, name: &str) -> Result<String>
```

读取一个环境变量。

读取失败和读到空字符串，在这里都是同一个空字符串：这个槽位在 ABI 上的失败位只表示宿主没能执行读取，而进程环境里一个存在但为空的变量，和不存在是一样的。

- 参数：
    - name : `&str`
- 返回值类型：`Result<String>`
- 对应槽位：[`sys_get_env`](../cpp/server.md#sys_get_env)

### `Host::set_env` {#Host.set_env}

```rust
pub fn set_env(&self, name: &str, value: &str) -> Result<()>
```

设置一个环境变量。只影响这个进程。

- 参数：
    - name : `&str`
    - value : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`sys_set_env`](../cpp/server.md#sys_set_env)

### `Host::try_is_wine` {#Host.try_is_wine}

```rust
pub fn try_is_wine(&self) -> Result<bool>
```

这个宿主是否运行在 Wine 下；宿主分辨不出来时返回 `Err`。

这个问题值得单独问：有些 Windows API 在 Wine 下的行为和真正的 Windows 不同，症状通常出现在离原因很远的地方。

- 返回值类型：`Result<bool>`
- 对应槽位：[`sys_is_wine`](../cpp/server.md#sys_is_wine)

### `Host::is_wine` {#Host.is_wine}

!!! warning "已弃用（自 26.51.2 起）"

    请用 `try_is_wine`：宿主分辨不出来时，这个函数回答 false

```rust
pub fn is_wine(&self) -> bool
```

这个宿主是否运行在 Wine 下；宿主分辨不出来时返回 `false`。

- 返回值类型：`bool`
- 对应槽位：[`sys_is_wine`](../cpp/server.md#sys_is_wine)

### `Host::os_name` {#Host.os_name}

```rust
pub fn os_name(&self) -> Result<String>
```

- 返回值类型：`Result<String>`
- 对应槽位：[`sys_info_str`](../cpp/server.md#sys_info_str)

### `Host::os_version` {#Host.os_version}

```rust
pub fn os_version(&self) -> Result<String>
```

操作系统的版本字符串。

- 返回值类型：`Result<String>`
- 对应槽位：[`sys_info_str`](../cpp/server.md#sys_info_str)

### `Host::locale` {#Host.locale}

```rust
pub fn locale(&self) -> Result<String>
```

- 返回值类型：`Result<String>`
- 对应槽位：[`sys_info_str`](../cpp/server.md#sys_info_str)

### `Host::local_time` {#Host.local_time}

```rust
pub fn local_time(&self) -> Result<crate::types::LocalTime>
```

- 返回值类型：`Result<crate::types::LocalTime>`
- 对应槽位：[`sys_info_str`](../cpp/server.md#sys_info_str)

### `Host::world` {#Host.world}

```rust
pub fn world(&self) -> crate::World
```

世界门面。

这个访问函数和 `Host` 之间不构成循环：`world` 确实需要用 `Host::execute_command` 拼出 `/fill`，但两边的入口都只是零大小门面的 `get()`，不共享任何状态。分层检查明确声明了这条边。

- 返回值类型：`crate::World`

### `Host::server` {#Host.server}

```rust
pub fn server(&self) -> crate::Server
```

服务器运行时的控制：冻结刻、加速刻，以及性能采样。

- 返回值类型：`crate::Server`

## `GamingStatus` {#GamingStatus}

```rust
pub enum GamingStatus {
        Default,
        Starting,
        Running,
        Stopping,
        /// The host reported a value this side does not recognize. This is not an error, since
        /// the host may be newer than the mod (contract §2.2). `Unknown(i32)` is used rather than
        /// a panic or a silent collapse into Default, because unrecognized and Default are two
        /// different things (contract §5.2).
        Unknown(i32),
}
```

服务器的运行阶段，对应 `ll::GamingStatus`。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`、`From`
