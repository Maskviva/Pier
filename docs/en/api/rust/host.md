# levilamina::Host

The host and the system themselves: the capabilities that involve no game concept.

What belongs here is decided by whether it speaks about the host or the world. Run
state, handing a task back to the server thread, executing a command and asking the
operating system its name all hold for another game as well; players, blocks and items
do not.

## `Host` {#Host}

```rust
pub struct Host(/* private */);
```

The host facade. Zero sized and freely `Copy`.

- Implements: `Clone`, `Copy`

### `Host::get` {#Host.get}

```rust
pub fn get() -> Host
```

- Return type: `Host`

### `Host::gaming_status` {#Host.gaming_status}

```rust
pub fn gaming_status(&self) -> GamingStatus
```

Which stage the server is in. The ABI marks it thread safe, so any thread may ask.

- Return type: `GamingStatus`
- Slots: [`gaming_status`](../cpp/core.md#gaming_status)

### `Host::schedule` {#Host.schedule}

```rust
pub fn schedule(&self, f: impl FnOnce() + Send + 'static) -> Result<TaskId>
```

Hands a piece of work back to the server thread to run as soon as possible. Thread
safe.

The closure is boxed and handed to the host, taken back and run inside the callback,
and freed immediately afterwards. Ownership stays on the mod side throughout, which
follows contract §3: no ownership crosses the boundary, what is passed is an opaque
pointer and the host only hands it back unchanged.

It goes through `schedule_for`, which carries a mod handle, and not the ownerless
`schedule`. A task on the ownerless slot still fires after the mod unloads and jumps
into an already unmapped code segment. A task with a handle is accounted per mod by the
host and the whole batch is discarded at unload, with a warning suggesting you cancel
them yourself. The returned [`TaskId`](root.md#TaskId) works with [`Host::cancel`](host.md#Host.cancel).

- Parameters:
    - f : `impl FnOnce() + Send + 'static`
- Return type: `Result<TaskId>`
- Slots: [`schedule_for`](../cpp/core.md#schedule_for)

### `Host::schedule_after` {#Host.schedule_after}

```rust
pub fn schedule_after(
        &self,
        delay: std::time::Duration,
        f: impl FnOnce() + Send + 'static,
    ) -> Result<TaskId>
```

As above, but runs after `delay`. Thread safe.

It takes a `Duration` rather than a bare millisecond count: whether the 5 in
`schedule_after(5, ...)` means five milliseconds or five seconds is answered only by the
parameter name, which is invisible at the call site. Putting the unit in the type makes
the call site carry the answer.

The ABI side is in milliseconds, so this converts once. A duration exceeding `u64`
milliseconds is clamped to the maximum rather than wrapping to a small number, since
wrapping would turn run in a year into run immediately.

- Parameters:
    - delay : `std::time::Duration`
    - f : `impl FnOnce() + Send + 'static`
- Return type: `Result<TaskId>`
- Slots: [`schedule_after_for`](../cpp/core.md#schedule_after_for)

### `Host::cancel` {#Host.cancel}

```rust
pub fn cancel(&self, task: TaskId) -> bool
```

Cancels a task that has not run yet. A ticket that already ran, was already cancelled,
or does not belong to this mod returns `false`.

Note that cancelling only voids the ticket. The closure itself is neither called nor
freed when the host tears down and is reclaimed when the process ends. Avoiding a leak
means not scheduling and cancelling in bulk on a hot path.

- Parameters:
    - task : `TaskId`
- Return type: `bool`
- Slots: [`schedule_cancel`](../cpp/core.md#schedule_cancel)

### `Host::try_pending_tasks` {#Host.try_pending_tasks}

```rust
pub fn try_pending_tasks(&self) -> Result<u32>
```

How many tasks under this mod have not run. Suited to asserting 0 in `on_unload`: a
host that cannot count returns `Err`, which the assertion receives in place of a 0.

- Return type: `Result<u32>`
- Slots: [`schedule_pending_count`](../cpp/core.md#schedule_pending_count)

### `Host::pending_tasks` {#Host.pending_tasks}

!!! warning "Deprecated since 26.51.2"

    use try_pending_tasks: this answers 0 when the host cannot count, which passes the assertion it exists for

```rust
pub fn pending_tasks(&self) -> u32
```

How many tasks under this mod have not run, and 0 when the host cannot count.

- Return type: `u32`
- Slots: [`schedule_pending_count`](../cpp/core.md#schedule_pending_count)

### `Host::execute_command` {#Host.execute_command}

```rust
pub fn execute_command(&self, cmd: &str) -> Result<String>
```

Executes a command as the console and returns its output.

The two `Err` cases stay apart: a missing slot, meaning the host is too old, and the
command itself failing, where `success == false` and the output holds the error.

- Parameters:
    - cmd : `&str`
- Return type: `Result<String>`
- Slots: [`execute_command`](../cpp/commands.md#execute_command)

### `Host::list_events` {#Host.list_events}

```rust
pub fn list_events(&self) -> Vec<String>
```

Lists every event id the host can currently resolve.

Printing this list when a subscription fails is far more useful than a bare subscribe
failed (contract §5.3: a log line has to answer what to do about it).

- Return type: `Vec<String>`
- Slots: [`list_events`](../cpp/events.md#list_events)

### `Host::sys_info` {#Host.sys_info}

```rust
pub fn sys_info(&self, prop: i32) -> Result<String>
```

Information at the operating-system level. `prop` comes from `sys::PIER_SYS_*`.

- Parameters:
    - prop : `i32`
- Return type: `Result<String>`
- Slots: [`sys_info_str`](../cpp/server.md#sys_info_str)

### `Host::server_info` {#Host.server_info}

```rust
pub fn server_info(&self, prop: i32) -> Result<String>
```

Information at the server level. `prop` comes from `sys::PIER_SRV_*`.

- Parameters:
    - prop : `i32`
- Return type: `Result<String>`
- Slots: [`server_info_str`](../cpp/server.md#server_info_str)

### `Host::protocol_version` {#Host.protocol_version}

```rust
pub fn protocol_version(&self) -> Result<u32>
```

The network protocol version of the server, derived from the running build's
game version.

**The host fails this on a version it does not know and on any pre-release
build.** An `Err` means cannot-be-determined, which must stay apart from a
protocol version of 0 (contract §5.2) — that is why this returns a `Result`.

When it fails, read `Self::game_sem_version` and map it yourself. Do not reach
for `Self::level_protocol_version`: that is the save's tag, and on any world
older than the server it is a different number.

- Return type: `Result<u32>`
- Slots: [`server_info_str`](../cpp/server.md#server_info_str)

### `Host::level_protocol_version` {#Host.level_protocol_version}

```rust
pub fn level_protocol_version(&self) -> Result<u32>
```

The protocol the **level** was last written by (`LevelData::mNetworkVersion`).

This says how old the save is. It is not what the server speaks, and using it as
such is the bug this pair of accessors was split to prevent.

- Return type: `Result<u32>`
- Slots: [`server_info_str`](../cpp/server.md#server_info_str)

### `Host::game_sem_version` {#Host.game_sem_version}

```rust
pub fn game_sem_version(&self) -> Result<String>
```

`"major.minor.patch"` of the running build, from `CurrentGameSemVersion`.

Carry your own version→protocol table off this when `Self::protocol_version`
does not know a build: a Pier release should not be on the critical path for
supporting a BDS that shipped yesterday.

- Return type: `Result<String>`
- Slots: [`server_info_str`](../cpp/server.md#server_info_str)

### `Host::bds_version` {#Host.bds_version}

```rust
pub fn bds_version(&self) -> Result<String>
```

The BDS version string, from `Common::getGameVersionString`.

- Return type: `Result<String>`
- Slots: [`server_info_str`](../cpp/server.md#server_info_str)

### `Host::packets` {#Host.packets}

```rust
pub fn packets(&self) -> crate::packet::Packets
```

- Return type: `crate::packet::Packets`

### `Host::current_tick` {#Host.current_tick}

```rust
pub fn current_tick(&self) -> Option<u64>
```

- Return type: `Option<u64>`
- Slots: [`get_current_tick`](../cpp/server.md#get_current_tick)

### `Host::player_count` {#Host.player_count}

```rust
pub fn player_count(&self) -> Option<i32>
```

- Return type: `Option<i32>`
- Slots: [`get_player_count`](../cpp/server.md#get_player_count)

### `Host::env` {#Host.env}

```rust
pub fn env(&self, name: &str) -> Result<String>
```

Reads an environment variable.

Failing to read and reading an empty string are the same empty string here: the failure
bit of this slot on the ABI means only that the host could not perform the read, and an
existing empty variable in a process environment is the same as an absent one.

- Parameters:
    - name : `&str`
- Return type: `Result<String>`
- Slots: [`sys_get_env`](../cpp/server.md#sys_get_env)

### `Host::set_env` {#Host.set_env}

```rust
pub fn set_env(&self, name: &str, value: &str) -> Result<()>
```

Sets an environment variable. It affects this process only.

- Parameters:
    - name : `&str`
    - value : `&str`
- Return type: `Result<()>`
- Slots: [`sys_set_env`](../cpp/server.md#sys_set_env)

### `Host::try_is_wine` {#Host.try_is_wine}

```rust
pub fn try_is_wine(&self) -> Result<bool>
```

Whether this host runs under Wine, and `Err` when the host cannot tell.

Worth asking on its own: some Windows APIs behave differently under Wine than on real
Windows, and the symptom usually appears far from the cause.

- Return type: `Result<bool>`
- Slots: [`sys_is_wine`](../cpp/server.md#sys_is_wine)

### `Host::is_wine` {#Host.is_wine}

!!! warning "Deprecated since 26.51.2"

    use try_is_wine: this answers false when the host cannot tell

```rust
pub fn is_wine(&self) -> bool
```

Whether this host runs under Wine, and `false` when the host cannot tell.

- Return type: `bool`
- Slots: [`sys_is_wine`](../cpp/server.md#sys_is_wine)

### `Host::os_name` {#Host.os_name}

```rust
pub fn os_name(&self) -> Result<String>
```

- Return type: `Result<String>`
- Slots: [`sys_info_str`](../cpp/server.md#sys_info_str)

### `Host::os_version` {#Host.os_version}

```rust
pub fn os_version(&self) -> Result<String>
```

The operating system version string.

- Return type: `Result<String>`
- Slots: [`sys_info_str`](../cpp/server.md#sys_info_str)

### `Host::locale` {#Host.locale}

```rust
pub fn locale(&self) -> Result<String>
```

- Return type: `Result<String>`
- Slots: [`sys_info_str`](../cpp/server.md#sys_info_str)

### `Host::local_time` {#Host.local_time}

```rust
pub fn local_time(&self) -> Result<crate::types::LocalTime>
```

- Return type: `Result<crate::types::LocalTime>`
- Slots: [`sys_info_str`](../cpp/server.md#sys_info_str)

### `Host::world` {#Host.world}

```rust
pub fn world(&self) -> crate::World
```

The world facade.

This accessor forms no cycle with `Host`: `world` really does need
`Host::execute_command` to assemble a `/fill`, but the entry point on each side is only
the `get()` of a zero-sized facade and no state is shared. The layering check declares
this edge explicitly.

- Return type: `crate::World`

### `Host::server` {#Host.server}

```rust
pub fn server(&self) -> crate::Server
```

Server runtime control: freezing ticks, warping them, and performance sampling.

- Return type: `crate::Server`

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

The run stage of the server, mirroring `ll::GamingStatus`.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`, `From`
