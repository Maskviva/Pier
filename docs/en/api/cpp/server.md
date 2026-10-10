# Server, ticks and system information

??? note "Section notes in abi.h"

    **the core slots, present since ABI v1**

    **§I NBT binary, KvDb (thread-safe), system & server info**

    **Appended: tick statistics**

## Slots {#slots}

### `get_current_tick` {#get_current_tick}

```c
uint64_t (*get_current_tick)(void);
```

Current server tick (the tickID from `Level::getCurrentTick()`). Returns 0 when the level is not ready. Server thread only.

- Call: `api->get_current_tick()`
- Return type: `uint64_t`
- Section of abi.h: the core slots, present since ABI v1
- Position in the table: slot 9, counting from 0
- Callers in each binding:
    - Rust: [`Host::current_tick`](../rust/host.md#Host.current_tick)
    - Go: [`CurrentTick`](../go/server.md#CurrentTick), [`Raw.GetCurrentTick`](../go/raw.md#Raw.GetCurrentTick)

### `get_tick_delta_time` {#get_tick_delta_time}

```c
double (*get_tick_delta_time)(void);
```

Wall-clock period of the last frame in seconds (mTickDeltaTime; 0.05 at 20 TPS). It includes the sleep the server inserts to hold 20 Hz, so it is not the time spent computing a tick and its reciprocal is not a tick rate: it is one noisy sample of the frame rate. For TPS and MSPT use `get_tps` and `get_mspt`. Returns -1.0 if unavailable. Server thread only.

- Call: `api->get_tick_delta_time()`
- Return type: `double`
- Section of abi.h: the core slots, present since ABI v1
- Position in the table: slot 10, counting from 0
- Callers in each binding:
    - Rust: [`Server::tick_delta_time`](../rust/server.md#Server.tick_delta_time)
    - Go: [`TickDeltaTime`](../go/server.md#TickDeltaTime), [`Raw.GetTickDeltaTime`](../go/raw.md#Raw.GetTickDeltaTime)

### `get_player_count` {#get_player_count}

```c
int32_t (*get_player_count)(void);
```

Number of currently connected players (`Level::getActivePlayerCount()`). Server thread only.

- Call: `api->get_player_count()`
- Return type: `int32_t`
- Section of abi.h: the core slots, present since ABI v1
- Position in the table: slot 11, counting from 0
- Callers in each binding:
    - Rust: [`Host::player_count`](../rust/host.md#Host.player_count)
    - Go: [`PlayerCount`](../go/server.md#PlayerCount), [`Raw.GetPlayerCount`](../go/raw.md#Raw.GetPlayerCount)

### `get_sim_paused` {#get_sim_paused}

```c
bool (*get_sim_paused)(void);
```

Whether the simulation is currently paused (`Level::getSimPaused()`). Server thread only.

- Call: `api->get_sim_paused()`
- Return type: `bool`
- Section of abi.h: the core slots, present since ABI v1
- Position in the table: slot 12, counting from 0
- Callers in each binding:
    - Rust: [`Server::is_sim_paused`](../rust/server.md#Server.is_sim_paused)
    - Go: [`SimPaused`](../go/server.md#SimPaused), [`Raw.GetSimPaused`](../go/raw.md#Raw.GetSimPaused)

### `sys_info_str` {#sys_info_str}

```c
bool (*sys_info_str)(int32_t prop, void* ctx, PierStrSink sink);
```

!!! note "Group note"

    System info: THREAD-SAFE (plain OS calls).

- Call: `api->sys_info_str(prop, ctx, sink)`
- Parameters:
    - prop : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 68, counting from 0
- Callers in each binding:
    - Rust: [`Host::sys_info`](../rust/host.md#Host.sys_info), [`Host::os_name`](../rust/host.md#Host.os_name), [`Host::os_version`](../rust/host.md#Host.os_version), [`Host::locale`](../rust/host.md#Host.locale), [`Host::local_time`](../rust/host.md#Host.local_time)
    - Go: [`SystemOsName`](../go/server.md#SystemOsName), [`SystemOsVersion`](../go/server.md#SystemOsVersion), [`SystemLocale`](../go/server.md#SystemLocale), [`SystemLocalTime`](../go/server.md#SystemLocalTime), [`Raw.SysInfoStr`](../go/raw.md#Raw.SysInfoStr)
    - Zig: [`props.sysOsName`](../zig/server.md#props.sysOsName), [`props.sysOsVersion`](../zig/server.md#props.sysOsVersion), [`props.sysLocale`](../zig/server.md#props.sysLocale), [`props.sysLocalTime`](../zig/server.md#props.sysLocalTime)

### `sys_get_env` {#sys_get_env}

```c
bool (*sys_get_env)(PierStr name, void* ctx, PierStrSink sink);
```

!!! note "Group note"

    System info: THREAD-SAFE (plain OS calls).

- Call: `api->sys_get_env(name, ctx, sink)`
- Parameters:
    - name : `PierStr`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 69, counting from 0
- Callers in each binding:
    - Rust: [`Host::env`](../rust/host.md#Host.env)
    - Go: [`Env`](../go/server.md#Env), [`Raw.SysGetEnv`](../go/raw.md#Raw.SysGetEnv)

### `sys_set_env` {#sys_set_env}

```c
bool (*sys_set_env)(PierStr name, PierStr value);
```

!!! note "Group note"

    System info: THREAD-SAFE (plain OS calls).

- Call: `api->sys_set_env(name, value)`
- Parameters:
    - name : `PierStr`
    - value : `PierStr`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 70, counting from 0
- Callers in each binding:
    - Rust: [`Host::set_env`](../rust/host.md#Host.set_env)
    - Go: [`SetEnv`](../go/server.md#SetEnv), [`Raw.SysSetEnv`](../go/raw.md#Raw.SysSetEnv)

### `sys_is_wine` {#sys_is_wine}

```c
bool (*sys_is_wine)(void);
```

!!! note "Group note"

    System info: THREAD-SAFE (plain OS calls).

- Call: `api->sys_is_wine()`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 71, counting from 0
- Callers in each binding:
    - Rust: [`Host::try_is_wine`](../rust/host.md#Host.try_is_wine), [`Host::is_wine`](../rust/host.md#Host.is_wine)
    - Go: [`Raw.SysIsWine`](../go/raw.md#Raw.SysIsWine)

### `server_info_str` {#server_info_str}

```c
bool (*server_info_str)(int32_t prop, void* ctx, PierStrSink sink);
```

!!! note "Group note"

    Server / world-level settings.

- Call: `api->server_info_str(prop, ctx, sink)`
- Parameters:
    - prop : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 77, counting from 0
- Callers in each binding:
    - Rust: [`Host::server_info`](../rust/host.md#Host.server_info), [`Host::protocol_version`](../rust/host.md#Host.protocol_version), [`Host::level_protocol_version`](../rust/host.md#Host.level_protocol_version), [`Host::game_sem_version`](../rust/host.md#Host.game_sem_version), [`Host::bds_version`](../rust/host.md#Host.bds_version)
    - Go: [`ServerBdsVersion`](../go/server.md#ServerBdsVersion), [`ServerProtocolVersion`](../go/server.md#ServerProtocolVersion), [`ServerLevelProtocolVersion`](../go/server.md#ServerLevelProtocolVersion), [`ServerGameSemVersion`](../go/server.md#ServerGameSemVersion), [`Raw.ServerInfoStr`](../go/raw.md#Raw.ServerInfoStr)
    - Zig: [`props.srvBdsVersion`](../zig/server.md#props.srvBdsVersion), [`props.srvProtocolVersion`](../zig/server.md#props.srvProtocolVersion), [`props.srvLevelProtocolVersion`](../zig/server.md#props.srvLevelProtocolVersion), [`props.srvGameSemVersion`](../zig/server.md#props.srvGameSemVersion)

### `tick_freeze` {#tick_freeze}

```c
bool (*tick_freeze)(bool on);
```

!!! note "Group note"

    Tick control (additive, gated by `struct_size`). Backed by a bridge-owned detour on `Level::tick`, installed lazily on the first control call and left in place (idle cost: one predictable branch per frame — a control call can arrive from a command handler that is executing INSIDE the tick, where unpatching would not be safe). Server thread only. While frozen, mobs/blocks/redstone/time stop; players can still move and chat (movement is client-authoritative, network runs outside the level tick).

- Call: `api->tick_freeze(on)`
- Parameters:
    - on : `bool`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 80, counting from 0
- Callers in each binding:
    - Rust: [`Server::set_tick_freeze`](../rust/server.md#Server.set_tick_freeze)
    - Go: [`Raw.TickFreeze`](../go/raw.md#Raw.TickFreeze)

### `tick_step` {#tick_step}

```c
bool (*tick_step)(uint32_t n);
```

!!! note "Group note"

    Tick control (additive, gated by `struct_size`). Backed by a bridge-owned detour on `Level::tick`, installed lazily on the first control call and left in place (idle cost: one predictable branch per frame — a control call can arrive from a command handler that is executing INSIDE the tick, where unpatching would not be safe). Server thread only. While frozen, mobs/blocks/redstone/time stop; players can still move and chat (movement is client-authoritative, network runs outside the level tick).

Only while frozen: queue exactly n extra frames. False if not frozen or n == 0.

- Call: `api->tick_step(n)`
- Parameters:
    - n : `uint32_t`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 81, counting from 0
- Callers in each binding:
    - Rust: [`Server::step_ticks`](../rust/server.md#Server.step_ticks)
    - Go: [`Raw.TickStep`](../go/raw.md#Raw.TickStep)

### `tick_warp` {#tick_warp}

```c
bool (*tick_warp)(double factor);
```

!!! note "Group note"

    Tick control (additive, gated by `struct_size`). Backed by a bridge-owned detour on `Level::tick`, installed lazily on the first control call and left in place (idle cost: one predictable branch per frame — a control call can arrive from a command handler that is executing INSIDE the tick, where unpatching would not be safe). Server thread only. While frozen, mobs/blocks/redstone/time stop; players can still move and chat (movement is client-authoritative, network runs outside the level tick).

0 &lt; factor &lt;= 100. Fractional = slow motion (accumulator), 1.0 restores normal.

- Call: `api->tick_warp(factor)`
- Parameters:
    - factor : `double`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 82, counting from 0
- Callers in each binding:
    - Rust: [`Server::set_tick_warp`](../rust/server.md#Server.set_tick_warp)
    - Go: [`Raw.TickWarp`](../go/raw.md#Raw.TickWarp)

### `profile_begin` {#profile_begin}

```c
bool (*profile_begin)(uint32_t ticks);
```

!!! note "Group note"

    Per-subsystem MSPT profiler (additive, gated by `struct_size`). Backed by five timing detours (Level/Dimension tick, redstone, chunk block ticks, block entities), installed lazily on the first `profile_begin` and left in place. One sampling window at a time. Server thread only.

Arm a window of `ticks` level ticks (1..12000). False if 0, too big, or already sampling.

- Call: `api->profile_begin(ticks)`
- Parameters:
    - ticks : `uint32_t`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 83, counting from 0
- Callers in each binding:
    - Rust: [`Server::begin_profile`](../rust/server.md#Server.begin_profile)
    - Go: [`Raw.ProfileBegin`](../go/raw.md#Raw.ProfileBegin)

### `profile_take` {#profile_take}

```c
bool (*profile_take)(void* ctx, PierStrSink sink);
```

!!! note "Group note"

    Per-subsystem MSPT profiler (additive, gated by `struct_size`). Backed by five timing detours (Level/Dimension tick, redstone, chunk block ticks, block entities), installed lazily on the first `profile_begin` and left in place. One sampling window at a time. Server thread only.

Poll for the finished report. False while sampling / nothing armed; true exactly once per window, sinking one SNBT report: {ticks:N, buckets:{`level_tick`:{us,calls}, `dimension_tick`:{…}, redstone:{…}, `chunk_blocks`:{…}, `block_entities`:{…}}}. Bucket times are INCLUSIVE (nested subsystems), report side by side, don't sum.

`chunk_blocks` is the drain of a chunk's pending block-tick queues, both the scheduled and the random one, summed over every chunk. Work a chunk does around that drain is outside the bucket, so the number is a floor on block ticking and not the whole of it. calls counts queue drains, of which a ticking chunk contributes two, not one.

- Call: `api->profile_take(ctx, sink)`
- Parameters:
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 84, counting from 0
- Callers in each binding:
    - Rust: [`Server::take_profile`](../rust/server.md#Server.take_profile)
    - Go: [`Raw.ProfileTake`](../go/raw.md#Raw.ProfileTake)

### `get_tps` {#get_tps}

```c
double (*get_tps)(int32_t window_seconds);
```

Ticks per second over the last `window_seconds` (1..60, clipped) of wall clock: the number of `Level::tick` calls that really ran divided by elapsed time. Stays correct under the tick warp (reads above 20), the tick freeze (reads 0) and lag (reads below 20). Returns -1.0 before the first frame has been sampled. Server thread only.

- Call: `api->get_tps(window_seconds)`
- Parameters:
    - window_seconds : `int32_t`
- Return type: `double`
- Section of abi.h: Appended: tick statistics
- Position in the table: slot 186, counting from 0
- Callers in each binding:
    - Rust: [`Server::tps`](../rust/server.md#Server.tps), [`Server::tps_over`](../rust/server.md#Server.tps_over)
    - Go: [`Tps`](../go/server.md#Tps), [`Raw.GetTps`](../go/raw.md#Raw.GetTps)

### `get_mspt` {#get_mspt}

```c
double (*get_mspt)(int32_t window_seconds);
```

Milliseconds spent inside `Level::tick` per tick, averaged over the last `window_seconds` (1..60, clipped). This is the time the server computes, excluding the idle sleep between frames; a healthy server reads a few milliseconds and only approaches 50 when saturated. Returns -1.0 when no tick ran in the window. Server thread only.

- Call: `api->get_mspt(window_seconds)`
- Parameters:
    - window_seconds : `int32_t`
- Return type: `double`
- Section of abi.h: Appended: tick statistics
- Position in the table: slot 187, counting from 0
- Callers in each binding:
    - Rust: [`Server::mspt`](../rust/server.md#Server.mspt), [`Server::mspt_over`](../rust/server.md#Server.mspt_over)
    - Go: [`Mspt`](../go/server.md#Mspt), [`Raw.GetMspt`](../go/raw.md#Raw.GetMspt)

## `PierSysInfoProp` {#PierSysInfoProp}

| Name | Value | Description |
|---|---|---|
| <span id="PIER_SYS_OS_NAME"></span>`PIER_SYS_OS_NAME` | `0` | `sys_utils::getSystemName` |
| <span id="PIER_SYS_OS_VERSION"></span>`PIER_SYS_OS_VERSION` | `1` | `sys_utils::getSystemVersion` → string |
| <span id="PIER_SYS_LOCALE"></span>`PIER_SYS_LOCALE` | `2` | `sys_utils::getSystemLocaleCode` |
| <span id="PIER_SYS_LOCAL_TIME"></span>`PIER_SYS_LOCAL_TIME` | `3` | `sys_utils::getLocalTime` → SNBT {year,month,day,hour,minute,second,ms} |

## `PierServerInfoProp` {#PierServerInfoProp}

`server_info_str` keys.

| Name | Value | Description |
|---|---|---|
| <span id="PIER_SRV_BDS_VERSION"></span>`PIER_SRV_BDS_VERSION` | `0` | `Common::getGameVersionString` |
| <span id="PIER_SRV_PROTOCOL_VERSION"></span>`PIER_SRV_PROTOCOL_VERSION` | `1` | `LevelData::mNetworkVersion`: the protocol the level was last written by. Says how old the save is, not what the server speaks. |
| <span id="PIER_SRV_LEVEL_PROTOCOL_VERSION"></span>`PIER_SRV_LEVEL_PROTOCOL_VERSION` | `2` | "major.minor.patch" of the running build, from CurrentGameSemVersion. Lets a mod carry its own version→protocol table instead of waiting for a Pier release. |
| <span id="PIER_SRV_GAME_SEM_VERSION"></span>`PIER_SRV_GAME_SEM_VERSION` | `3` |  |
