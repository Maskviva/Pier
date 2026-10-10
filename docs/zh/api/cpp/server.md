# 服务器、刻与系统信息

??? note "abi.h 里的分节说明"

    **核心槽位，从 ABI v1 起就有（`the core slots, present since ABI v1`）**

    **§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）**

    **追加：刻的统计（`Appended: tick statistics`）**

## 槽位 {#slots}

### `get_current_tick` {#get_current_tick}

```c
uint64_t (*get_current_tick)(void);
```

当前的服务器刻（`Level::getCurrentTick()` 里的 tickID）。世界还没就绪时返回 0。只能在服务器线程调用。

- 调用形式：`api->get_current_tick()`
- 返回值类型：`uint64_t`
- 所在分节：核心槽位，从 ABI v1 起就有（`the core slots, present since ABI v1`）
- 表内序号：第 9 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Host::current_tick`](../rust/host.md#Host.current_tick)
    - Go：[`CurrentTick`](../go/server.md#CurrentTick)、[`Raw.GetCurrentTick`](../go/raw.md#Raw.GetCurrentTick)

### `get_tick_delta_time` {#get_tick_delta_time}

```c
double (*get_tick_delta_time)(void);
```

上一帧实际经过的时间，单位秒（`mTickDeltaTime`；20 TPS 时是 0.05）。它包含服务器为维持 20 Hz 插入的休眠，所以它和计算一刻所花的时间是两回事，它的倒数也不能当刻率用：它只是帧率的一个带噪声的样本。要 TPS 和 MSPT，请用 `get_tps` 和 `get_mspt`。取不到时返回 -1.0。只能在服务器线程调用。

- 调用形式：`api->get_tick_delta_time()`
- 返回值类型：`double`
- 所在分节：核心槽位，从 ABI v1 起就有（`the core slots, present since ABI v1`）
- 表内序号：第 10 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Server::tick_delta_time`](../rust/server.md#Server.tick_delta_time)
    - Go：[`TickDeltaTime`](../go/server.md#TickDeltaTime)、[`Raw.GetTickDeltaTime`](../go/raw.md#Raw.GetTickDeltaTime)

### `get_player_count` {#get_player_count}

```c
int32_t (*get_player_count)(void);
```

当前连接的玩家数（`Level::getActivePlayerCount()`）。只能在服务器线程调用。

- 调用形式：`api->get_player_count()`
- 返回值类型：`int32_t`
- 所在分节：核心槽位，从 ABI v1 起就有（`the core slots, present since ABI v1`）
- 表内序号：第 11 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Host::player_count`](../rust/host.md#Host.player_count)
    - Go：[`PlayerCount`](../go/server.md#PlayerCount)、[`Raw.GetPlayerCount`](../go/raw.md#Raw.GetPlayerCount)

### `get_sim_paused` {#get_sim_paused}

```c
bool (*get_sim_paused)(void);
```

模拟当前是否暂停（`Level::getSimPaused()`）。只能在服务器线程调用。

- 调用形式：`api->get_sim_paused()`
- 返回值类型：`bool`
- 所在分节：核心槽位，从 ABI v1 起就有（`the core slots, present since ABI v1`）
- 表内序号：第 12 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Server::is_sim_paused`](../rust/server.md#Server.is_sim_paused)
    - Go：[`SimPaused`](../go/server.md#SimPaused)、[`Raw.GetSimPaused`](../go/raw.md#Raw.GetSimPaused)

### `sys_info_str` {#sys_info_str}

```c
bool (*sys_info_str)(int32_t prop, void* ctx, PierStrSink sink);
```

!!! note "分组说明"

    系统信息：线程安全（只是普通的操作系统调用）。

- 调用形式：`api->sys_info_str(prop, ctx, sink)`
- 参数：
    - prop : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 68 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Host::sys_info`](../rust/host.md#Host.sys_info)、[`Host::os_name`](../rust/host.md#Host.os_name)、[`Host::os_version`](../rust/host.md#Host.os_version)、[`Host::locale`](../rust/host.md#Host.locale)、[`Host::local_time`](../rust/host.md#Host.local_time)
    - Go：[`SystemOsName`](../go/server.md#SystemOsName)、[`SystemOsVersion`](../go/server.md#SystemOsVersion)、[`SystemLocale`](../go/server.md#SystemLocale)、[`SystemLocalTime`](../go/server.md#SystemLocalTime)、[`Raw.SysInfoStr`](../go/raw.md#Raw.SysInfoStr)
    - Zig：[`props.sysOsName`](../zig/server.md#props.sysOsName)、[`props.sysOsVersion`](../zig/server.md#props.sysOsVersion)、[`props.sysLocale`](../zig/server.md#props.sysLocale)、[`props.sysLocalTime`](../zig/server.md#props.sysLocalTime)

### `sys_get_env` {#sys_get_env}

```c
bool (*sys_get_env)(PierStr name, void* ctx, PierStrSink sink);
```

!!! note "分组说明"

    系统信息：线程安全（只是普通的操作系统调用）。

- 调用形式：`api->sys_get_env(name, ctx, sink)`
- 参数：
    - name : `PierStr`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 69 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Host::env`](../rust/host.md#Host.env)
    - Go：[`Env`](../go/server.md#Env)、[`Raw.SysGetEnv`](../go/raw.md#Raw.SysGetEnv)

### `sys_set_env` {#sys_set_env}

```c
bool (*sys_set_env)(PierStr name, PierStr value);
```

!!! note "分组说明"

    系统信息：线程安全（只是普通的操作系统调用）。

- 调用形式：`api->sys_set_env(name, value)`
- 参数：
    - name : `PierStr`
    - value : `PierStr`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 70 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Host::set_env`](../rust/host.md#Host.set_env)
    - Go：[`SetEnv`](../go/server.md#SetEnv)、[`Raw.SysSetEnv`](../go/raw.md#Raw.SysSetEnv)

### `sys_is_wine` {#sys_is_wine}

```c
bool (*sys_is_wine)(void);
```

!!! note "分组说明"

    系统信息：线程安全（只是普通的操作系统调用）。

- 调用形式：`api->sys_is_wine()`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 71 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Host::try_is_wine`](../rust/host.md#Host.try_is_wine)、[`Host::is_wine`](../rust/host.md#Host.is_wine)
    - Go：[`Raw.SysIsWine`](../go/raw.md#Raw.SysIsWine)

### `server_info_str` {#server_info_str}

```c
bool (*server_info_str)(int32_t prop, void* ctx, PierStrSink sink);
```

!!! note "分组说明"

    服务器和世界级别的设置。

- 调用形式：`api->server_info_str(prop, ctx, sink)`
- 参数：
    - prop : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 77 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Host::server_info`](../rust/host.md#Host.server_info)、[`Host::protocol_version`](../rust/host.md#Host.protocol_version)、[`Host::level_protocol_version`](../rust/host.md#Host.level_protocol_version)、[`Host::game_sem_version`](../rust/host.md#Host.game_sem_version)、[`Host::bds_version`](../rust/host.md#Host.bds_version)
    - Go：[`ServerBdsVersion`](../go/server.md#ServerBdsVersion)、[`ServerProtocolVersion`](../go/server.md#ServerProtocolVersion)、[`ServerLevelProtocolVersion`](../go/server.md#ServerLevelProtocolVersion)、[`ServerGameSemVersion`](../go/server.md#ServerGameSemVersion)、[`Raw.ServerInfoStr`](../go/raw.md#Raw.ServerInfoStr)
    - Zig：[`props.srvBdsVersion`](../zig/server.md#props.srvBdsVersion)、[`props.srvProtocolVersion`](../zig/server.md#props.srvProtocolVersion)、[`props.srvLevelProtocolVersion`](../zig/server.md#props.srvLevelProtocolVersion)、[`props.srvGameSemVersion`](../zig/server.md#props.srvGameSemVersion)

### `tick_freeze` {#tick_freeze}

```c
bool (*tick_freeze)(bool on);
```

!!! note "分组说明"

    控制刻的推进（追加的槽位，受 `struct_size` 约束）。实现方式是 bridge 在 `Level::tick` 上挂一个钩子：第一次调用控制接口时才安装，装上以后一直留着，空闲时每帧多一次可以预测的分支。一直留着的原因是，控制调用可能来自正在刻**内部**执行的命令处理函数，在那里卸下钩子并不安全。只能在服务器线程调用。

    冻结期间，生物、方块、红石和时间都停下；玩家仍然可以移动和聊天，因为移动由客户端决定，网络在关卡的刻之外运行。

- 调用形式：`api->tick_freeze(on)`
- 参数：
    - on : `bool`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 80 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Server::set_tick_freeze`](../rust/server.md#Server.set_tick_freeze)
    - Go：[`Raw.TickFreeze`](../go/raw.md#Raw.TickFreeze)

### `tick_step` {#tick_step}

```c
bool (*tick_step)(uint32_t n);
```

!!! note "分组说明"

    控制刻的推进（追加的槽位，受 `struct_size` 约束）。实现方式是 bridge 在 `Level::tick` 上挂一个钩子：第一次调用控制接口时才安装，装上以后一直留着，空闲时每帧多一次可以预测的分支。一直留着的原因是，控制调用可能来自正在刻**内部**执行的命令处理函数，在那里卸下钩子并不安全。只能在服务器线程调用。

    冻结期间，生物、方块、红石和时间都停下；玩家仍然可以移动和聊天，因为移动由客户端决定，网络在关卡的刻之外运行。

只在冻结时有效：额外排入正好 n 帧。没有冻结或 n == 0 时返回 false。

- 调用形式：`api->tick_step(n)`
- 参数：
    - n : `uint32_t`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 81 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Server::step_ticks`](../rust/server.md#Server.step_ticks)
    - Go：[`Raw.TickStep`](../go/raw.md#Raw.TickStep)

### `tick_warp` {#tick_warp}

```c
bool (*tick_warp)(double factor);
```

!!! note "分组说明"

    控制刻的推进（追加的槽位，受 `struct_size` 约束）。实现方式是 bridge 在 `Level::tick` 上挂一个钩子：第一次调用控制接口时才安装，装上以后一直留着，空闲时每帧多一次可以预测的分支。一直留着的原因是，控制调用可能来自正在刻**内部**执行的命令处理函数，在那里卸下钩子并不安全。只能在服务器线程调用。

    冻结期间，生物、方块、红石和时间都停下；玩家仍然可以移动和聊天，因为移动由客户端决定，网络在关卡的刻之外运行。

0 < factor <= 100。小数表示慢动作（用累加器实现），1.0 恢复正常。

- 调用形式：`api->tick_warp(factor)`
- 参数：
    - factor : `double`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 82 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Server::set_tick_warp`](../rust/server.md#Server.set_tick_warp)
    - Go：[`Raw.TickWarp`](../go/raw.md#Raw.TickWarp)

### `profile_begin` {#profile_begin}

```c
bool (*profile_begin)(uint32_t ticks);
```

!!! note "分组说明"

    按子系统统计 MSPT 的分析器（追加的槽位，受 `struct_size` 约束）。实现是五个计时钩子（Level 和 Dimension 的刻、红石、区块的方块刻、方块实体），第一次调用 `profile_begin` 时才安装，之后一直留着。同一时间只有一个采样窗口。只能在服务器线程调用。

开启一个长度为 `ticks` 个关卡刻（1 到 12000）的采样窗口。为 0、太大，或者已经在采样时返回 false。

- 调用形式：`api->profile_begin(ticks)`
- 参数：
    - ticks : `uint32_t`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 83 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Server::begin_profile`](../rust/server.md#Server.begin_profile)
    - Go：[`Raw.ProfileBegin`](../go/raw.md#Raw.ProfileBegin)

### `profile_take` {#profile_take}

```c
bool (*profile_take)(void* ctx, PierStrSink sink);
```

!!! note "分组说明"

    按子系统统计 MSPT 的分析器（追加的槽位，受 `struct_size` 约束）。实现是五个计时钩子（Level 和 Dimension 的刻、红石、区块的方块刻、方块实体），第一次调用 `profile_begin` 时才安装，之后一直留着。同一时间只有一个采样窗口。只能在服务器线程调用。

取回已经完成的报告。还在采样或者没有开启窗口时返回 false；每个窗口正好返回一次 true，并通过输出回调给出一份 SNBT 报告：

```text
{ticks:N, buckets:{level_tick:{us,calls}, dimension_tick:{…}, redstone:{…},
 chunk_blocks:{…}, block_entities:{…}}}
```

各个桶的时间是**包含**关系（子系统之间有嵌套），请并排对照着看，不要相加。

`chunk_blocks` 统计的是清空区块待处理方块刻队列的时间，计划刻和随机刻两个队列都算，所有区块加在一起。区块在清空队列前后做的其他工作不在这个桶里，所以这个数是方块刻耗时的下限，并非全部。`calls` 统计的是清空队列的次数，一个正在运行刻的区块贡献两次。

- 调用形式：`api->profile_take(ctx, sink)`
- 参数：
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 84 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Server::take_profile`](../rust/server.md#Server.take_profile)
    - Go：[`Raw.ProfileTake`](../go/raw.md#Raw.ProfileTake)

### `get_tps` {#get_tps}

```c
double (*get_tps)(int32_t window_seconds);
```

最近 `window_seconds` 秒（1 到 60，超出会被截断）的实际时间里，每秒运行的刻数：真正运行过的 `Level::tick` 次数除以经过的时间。刻加速时（读数高于 20）、刻冻结时（读数为 0）和卡顿时（读数低于 20）都是准确的。第一帧还没采样时返回 -1.0。只能在服务器线程调用。

- 调用形式：`api->get_tps(window_seconds)`
- 参数：
    - window_seconds : `int32_t`
- 返回值类型：`double`
- 所在分节：追加：刻的统计（`Appended: tick statistics`）
- 表内序号：第 186 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Server::tps`](../rust/server.md#Server.tps)、[`Server::tps_over`](../rust/server.md#Server.tps_over)
    - Go：[`Tps`](../go/server.md#Tps)、[`Raw.GetTps`](../go/raw.md#Raw.GetTps)

### `get_mspt` {#get_mspt}

```c
double (*get_mspt)(int32_t window_seconds);
```

最近 `window_seconds` 秒（1 到 60，超出范围会被截断）里，每一刻在 `Level::tick` 内花费的毫秒数的平均值。这是服务器计算所用的时间，不含帧与帧之间空闲的休眠；健康的服务器读数是几毫秒，只有满负荷时才接近 50。窗口内一刻都没有运行时返回 -1.0。只能在服务器线程调用。

- 调用形式：`api->get_mspt(window_seconds)`
- 参数：
    - window_seconds : `int32_t`
- 返回值类型：`double`
- 所在分节：追加：刻的统计（`Appended: tick statistics`）
- 表内序号：第 187 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Server::mspt`](../rust/server.md#Server.mspt)、[`Server::mspt_over`](../rust/server.md#Server.mspt_over)
    - Go：[`Mspt`](../go/server.md#Mspt)、[`Raw.GetMspt`](../go/raw.md#Raw.GetMspt)

## `PierSysInfoProp` {#PierSysInfoProp}

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PIER_SYS_OS_NAME"></span>`PIER_SYS_OS_NAME` | `0` | 取自 `sys_utils::getSystemName` |
| <span id="PIER_SYS_OS_VERSION"></span>`PIER_SYS_OS_VERSION` | `1` | 取自 `sys_utils::getSystemVersion`，返回字符串 |
| <span id="PIER_SYS_LOCALE"></span>`PIER_SYS_LOCALE` | `2` | 取自 `sys_utils::getSystemLocaleCode` |
| <span id="PIER_SYS_LOCAL_TIME"></span>`PIER_SYS_LOCAL_TIME` | `3` | 取自 `sys_utils::getLocalTime`，返回 SNBT `{year,month,day,hour,minute,second,ms}` |

## `PierServerInfoProp` {#PierServerInfoProp}

`server_info_str` 的键。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PIER_SRV_BDS_VERSION"></span>`PIER_SRV_BDS_VERSION` | `0` | 取自 `Common::getGameVersionString` |
| <span id="PIER_SRV_PROTOCOL_VERSION"></span>`PIER_SRV_PROTOCOL_VERSION` | `1` | `LevelData::mNetworkVersion`：最后一次写这个存档时用的协议版本。它说明存档有多旧，和服务器现在使用的协议无关。 |
| <span id="PIER_SRV_LEVEL_PROTOCOL_VERSION"></span>`PIER_SRV_LEVEL_PROTOCOL_VERSION` | `2` | 正在运行的构建的版本号 `"major.minor.patch"`，取自 `CurrentGameSemVersion`。模组可以据此自带一张版本到协议的对照表，不必等 Pier 发新版。 |
| <span id="PIER_SRV_GAME_SEM_VERSION"></span>`PIER_SRV_GAME_SEM_VERSION` | `3` |  |
