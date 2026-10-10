# Go: Raw: one typed method per slot

## Variables {#variables}

### `Raw` {#Raw}

```go
var Raw RawAPI
```

Raw is the typed slot layer. Prefer the facades of this package where one exists.

## `RawAPI` {#RawAPI}

```go
type RawAPI struct{}
type RawAPI struct{}
```

RawAPI reaches every slot without a callback, typed in Go and gated: a slot the host lacks is a \*NotProvidedError. A result means what abi.h says it means; the rest of the package builds the interpreted API on top of it. Use it through Raw.

## Core: entry point, logging and tasks {#raw-core}

### `Raw.Log` {#Raw.Log}

```go
func (RawAPI) Log(level int32, msg string) error
```

Log calls the log slot.

Log a message through the mod's own LeviLamina logger. level: -1=Off, 0=Fatal, 1=Error, 2=Warn, 3=Info, 4=Debug, 5=Trace (mirrors `ll::io::LogLevel`). Thread-safe.

- Parameters:
    - level : `int32`
    - msg : `string`
- Return type: `error`
- Slots: [`log`](../cpp/core.md#log)

### `Raw.GamingStatus` {#Raw.GamingStatus}

```go
func (RawAPI) GamingStatus() (int32, error)
```

GamingStatus calls the `gaming_status` slot.

Current gaming status: 0=Default, 1=Starting, 2=Running, 3=Stopping (mirrors `ll::GamingStatus`). Thread-safe.

- Return type: `(int32, error)`
- Slots: [`gaming_status`](../cpp/core.md#gaming_status)

### `Raw.ScheduleCancel` {#Raw.ScheduleCancel}

```go
func (RawAPI) ScheduleCancel(taskId uint64) (bool, error)
```

ScheduleCancel calls the `schedule_cancel` slot.

Drop a task scheduled by this mod if it has not run yet. Returns true if a pending task was actually dropped. Safe to call from any thread and from inside another task. Cancelling leaks `user` for the same reason as above, so prefer letting short tasks run.

- Parameters:
    - taskId : `uint64`
- Return type: `(bool, error)`
- Slots: [`schedule_cancel`](../cpp/core.md#schedule_cancel)

### `Raw.SchedulePendingCount` {#Raw.SchedulePendingCount}

```go
func (RawAPI) SchedulePendingCount() (uint32, error)
```

SchedulePendingCount calls the `schedule_pending_count` slot.

Number of tasks this mod still has pending. Intended for a mod to assert it has drained its own work in `on_disable` / `on_unload`, which is a precondition for being marked "`reload_safe`" in its manifest.

- Return type: `(uint32, error)`
- Slots: [`schedule_pending_count`](../cpp/core.md#schedule_pending_count)

## Events {#raw-events}

### `Raw.UnsubscribeEvent` {#Raw.UnsubscribeEvent}

```go
func (RawAPI) UnsubscribeEvent(listener ListenerHandle) (bool, error)
```

UnsubscribeEvent calls the `unsubscribe_event` slot.

Remove a listener previously returned by `subscribe_event`. Server thread only.

- Parameters:
    - listener : `ListenerHandle`
- Return type: `(bool, error)`
- Slots: [`unsubscribe_event`](../cpp/events.md#unsubscribe_event)

### `Raw.ListEvents` {#Raw.ListEvents}

```go
func (RawAPI) ListEvents() ([]string, error)
```

ListEvents calls the `list_events` slot.

Enumerate all currently registered event ids. Server thread only.

- Return type: `([]string, error)`
- Slots: [`list_events`](../cpp/events.md#list_events)

## Commands {#raw-commands}

### `Raw.RegisterCommandEnum` {#Raw.RegisterCommandEnum}

```go
func (RawAPI) RegisterCommandEnum(name string, valuesSnbt string) (bool, error)
```

RegisterCommandEnum calls the `register_command_enum` slot.

`values_snbt` = {values:\[\["name",1L\],…\]}  → tryRegisterRuntimeEnum.

- Parameters:
    - name : `string`
    - valuesSnbt : `string`
- Return type: `(bool, error)`
- Slots: [`register_command_enum`](../cpp/commands.md#register_command_enum)

### `Raw.RegisterCommandSoftEnum` {#Raw.RegisterCommandSoftEnum}

```go
func (RawAPI) RegisterCommandSoftEnum(name string, valuesSnbt string) (bool, error)
```

RegisterCommandSoftEnum calls the `register_command_soft_enum` slot.

`values_snbt` = {values:\["a","b"\]}         → tryRegisterSoftEnum.

- Parameters:
    - name : `string`
    - valuesSnbt : `string`
- Return type: `(bool, error)`
- Slots: [`register_command_soft_enum`](../cpp/commands.md#register_command_soft_enum)

### `Raw.UpdateCommandSoftEnum` {#Raw.UpdateCommandSoftEnum}

```go
func (RawAPI) UpdateCommandSoftEnum(name string, op int32, valuesSnbt string) (bool, error)
```

UpdateCommandSoftEnum calls the `update_command_soft_enum` slot.

op: 0=set 1=add 2=remove.

- Parameters:
    - name : `string`
    - op : `int32`
    - valuesSnbt : `string`
- Return type: `(bool, error)`
- Slots: [`update_command_soft_enum`](../cpp/commands.md#update_command_soft_enum)

## Server, ticks and system information {#raw-server}

### `Raw.GetCurrentTick` {#Raw.GetCurrentTick}

```go
func (RawAPI) GetCurrentTick() (uint64, error)
```

GetCurrentTick calls the `get_current_tick` slot.

Current server tick (the tickID from `Level::getCurrentTick()`). Returns 0 when the level is not ready. Server thread only.

- Return type: `(uint64, error)`
- Slots: [`get_current_tick`](../cpp/server.md#get_current_tick)

### `Raw.GetTickDeltaTime` {#Raw.GetTickDeltaTime}

```go
func (RawAPI) GetTickDeltaTime() (float64, error)
```

GetTickDeltaTime calls the `get_tick_delta_time` slot.

Wall-clock period of the last frame in seconds (mTickDeltaTime; 0.05 at 20 TPS). It includes the sleep the server inserts to hold 20 Hz, so it is not the time spent computing a tick and its reciprocal is not a tick rate: it is one noisy sample of the frame rate. For TPS and MSPT use `get_tps` and `get_mspt`. Returns -1.0 if unavailable. Server thread only.

- Return type: `(float64, error)`
- Slots: [`get_tick_delta_time`](../cpp/server.md#get_tick_delta_time)

### `Raw.GetPlayerCount` {#Raw.GetPlayerCount}

```go
func (RawAPI) GetPlayerCount() (int32, error)
```

GetPlayerCount calls the `get_player_count` slot.

Number of currently connected players (`Level::getActivePlayerCount()`). Server thread only.

- Return type: `(int32, error)`
- Slots: [`get_player_count`](../cpp/server.md#get_player_count)

### `Raw.GetSimPaused` {#Raw.GetSimPaused}

```go
func (RawAPI) GetSimPaused() (bool, error)
```

GetSimPaused calls the `get_sim_paused` slot.

Whether the simulation is currently paused (`Level::getSimPaused()`). Server thread only.

- Return type: `(bool, error)`
- Slots: [`get_sim_paused`](../cpp/server.md#get_sim_paused)

### `Raw.SysInfoStr` {#Raw.SysInfoStr}

```go
func (RawAPI) SysInfoStr(prop int32) ([]string, bool, error)
```

SysInfoStr calls the `sys_info_str` slot.

System info: THREAD-SAFE (plain OS calls).

- Parameters:
    - prop : `int32`
- Return type: `([]string, bool, error)`
- Slots: [`sys_info_str`](../cpp/server.md#sys_info_str)

### `Raw.SysGetEnv` {#Raw.SysGetEnv}

```go
func (RawAPI) SysGetEnv(name string) ([]string, bool, error)
```

SysGetEnv calls the `sys_get_env` slot.

- Parameters:
    - name : `string`
- Return type: `([]string, bool, error)`
- Slots: [`sys_get_env`](../cpp/server.md#sys_get_env)

### `Raw.SysSetEnv` {#Raw.SysSetEnv}

```go
func (RawAPI) SysSetEnv(name string, value string) (bool, error)
```

SysSetEnv calls the `sys_set_env` slot.

- Parameters:
    - name : `string`
    - value : `string`
- Return type: `(bool, error)`
- Slots: [`sys_set_env`](../cpp/server.md#sys_set_env)

### `Raw.SysIsWine` {#Raw.SysIsWine}

```go
func (RawAPI) SysIsWine() (bool, error)
```

SysIsWine calls the `sys_is_wine` slot.

- Return type: `(bool, error)`
- Slots: [`sys_is_wine`](../cpp/server.md#sys_is_wine)

### `Raw.ServerInfoStr` {#Raw.ServerInfoStr}

```go
func (RawAPI) ServerInfoStr(prop int32) ([]string, bool, error)
```

ServerInfoStr calls the `server_info_str` slot.

- Parameters:
    - prop : `int32`
- Return type: `([]string, bool, error)`
- Slots: [`server_info_str`](../cpp/server.md#server_info_str)

### `Raw.TickFreeze` {#Raw.TickFreeze}

```go
func (RawAPI) TickFreeze(on bool) (bool, error)
```

TickFreeze calls the `tick_freeze` slot.

Tick control (additive, gated by `struct_size`). Backed by a bridge-owned detour on `Level::tick`, installed lazily on the first control call and left in place (idle cost: one predictable branch per frame — a control call can arrive from a command handler that is executing INSIDE the tick, where unpatching would not be safe). Server thread only. While frozen, mobs/blocks/redstone/time stop; players can still move and chat (movement is client-authoritative, network runs outside the level tick).

- Parameters:
    - on : `bool`
- Return type: `(bool, error)`
- Slots: [`tick_freeze`](../cpp/server.md#tick_freeze)

### `Raw.TickStep` {#Raw.TickStep}

```go
func (RawAPI) TickStep(n uint32) (bool, error)
```

TickStep calls the `tick_step` slot.

Only while frozen: queue exactly n extra frames. False if not frozen or n == 0.

- Parameters:
    - n : `uint32`
- Return type: `(bool, error)`
- Slots: [`tick_step`](../cpp/server.md#tick_step)

### `Raw.TickWarp` {#Raw.TickWarp}

```go
func (RawAPI) TickWarp(factor float64) (bool, error)
```

TickWarp calls the `tick_warp` slot.

0 &lt; factor &lt;= 100. Fractional = slow motion (accumulator), 1.0 restores normal.

- Parameters:
    - factor : `float64`
- Return type: `(bool, error)`
- Slots: [`tick_warp`](../cpp/server.md#tick_warp)

### `Raw.ProfileBegin` {#Raw.ProfileBegin}

```go
func (RawAPI) ProfileBegin(ticks uint32) (bool, error)
```

ProfileBegin calls the `profile_begin` slot.

Arm a window of `ticks` level ticks (1..12000). False if 0, too big, or already sampling.

- Parameters:
    - ticks : `uint32`
- Return type: `(bool, error)`
- Slots: [`profile_begin`](../cpp/server.md#profile_begin)

### `Raw.ProfileTake` {#Raw.ProfileTake}

```go
func (RawAPI) ProfileTake() ([]string, bool, error)
```

ProfileTake calls the `profile_take` slot.

Poll for the finished report. False while sampling / nothing armed; true exactly once per window, sinking one SNBT report: {ticks:N, buckets:{`level_tick`:{us,calls}, `dimension_tick`:{…}, redstone:{…}, `chunk_blocks`:{…}, `block_entities`:{…}}}. Bucket times are INCLUSIVE (nested subsystems), report side by side, don't sum.

- Return type: `([]string, bool, error)`
- Slots: [`profile_take`](../cpp/server.md#profile_take)

### `Raw.GetTps` {#Raw.GetTps}

```go
func (RawAPI) GetTps(windowSeconds int32) (float64, error)
```

GetTps calls the `get_tps` slot.

Ticks per second over the last `window_seconds` (1..60, clipped) of wall clock: the number of `Level::tick` calls that really ran divided by elapsed time. Stays correct under the tick warp (reads above 20), the tick freeze (reads 0) and lag (reads below 20). Returns -1.0 before the first frame has been sampled. Server thread only.

- Parameters:
    - windowSeconds : `int32`
- Return type: `(float64, error)`
- Slots: [`get_tps`](../cpp/server.md#get_tps)

### `Raw.GetMspt` {#Raw.GetMspt}

```go
func (RawAPI) GetMspt(windowSeconds int32) (float64, error)
```

GetMspt calls the `get_mspt` slot.

Milliseconds spent inside `Level::tick` per tick, averaged over the last `window_seconds` (1..60, clipped). This is the time the server computes, excluding the idle sleep between frames; a healthy server reads a few milliseconds and only approaches 50 when saturated. Returns -1.0 when no tick ran in the window. Server thread only.

- Parameters:
    - windowSeconds : `int32`
- Return type: `(float64, error)`
- Slots: [`get_mspt`](../cpp/server.md#get_mspt)

## World {#raw-world}

### `Raw.SpawnParticle` {#Raw.SpawnParticle}

```go
func (RawAPI) SpawnParticle(dimension int32, effectName string, x float64, y float64, z float64) (bool, error)
```

SpawnParticle calls the `spawn_particle` slot.

Spawn a particle effect at a world coordinate. Used to outline a selection box edge-by-edge. Server thread only. Returns false if the level/dimension is not ready. dimension   : 0 = overworld, 1 = nether, 2 = the end. `effect_name` : e.g. "minecraft:`basic_flame_particle`" or "minecraft:`redstone_wire_dust_particle`".

- Parameters:
    - dimension : `int32`
    - effectName : `string`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- Return type: `(bool, error)`
- Slots: [`spawn_particle`](../cpp/world.md#spawn_particle)

### `Raw.SetBlock` {#Raw.SetBlock}

```go
func (RawAPI) SetBlock(dim int32, x int32, y int32, z int32, blockSpec string) (bool, error)
```

SetBlock calls the `set_block` slot.

Place a block natively (`BlockSource::setBlock`, DEFAULT update flags). `block_spec` = "minecraft:stone" / "stone" (default state) or a full {name,states,...} SNBT. Unknown names fail instead of placing a placeholder.

- Parameters:
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
    - blockSpec : `string`
- Return type: `(bool, error)`
- Slots: [`set_block`](../cpp/world.md#set_block)

### `Raw.GetTime` {#Raw.GetTime}

```go
func (RawAPI) GetTime() (int64, bool, error)
```

GetTime calls the `get_time` slot.

World time (`Level::getTime`).

- Return type: `(int64, bool, error)`
- Slots: [`get_time`](../cpp/world.md#get_time)

### `Raw.SetTime` {#Raw.SetTime}

```go
func (RawAPI) SetTime(t int64) (bool, error)
```

SetTime calls the `set_time` slot.

Set world time natively (`Level::setTime`).

- Parameters:
    - t : `int64`
- Return type: `(bool, error)`
- Slots: [`set_time`](../cpp/world.md#set_time)

### `Raw.SetWeather` {#Raw.SetWeather}

```go
func (RawAPI) SetWeather(weather int32) (bool, error)
```

SetWeather calls the `set_weather` slot.

0=clear 1=rain 2=thunder, native (`Level::updateWeather`).

- Parameters:
    - weather : `int32`
- Return type: `(bool, error)`
- Slots: [`set_weather`](../cpp/world.md#set_weather)

### `Raw.Explode` {#Raw.Explode}

```go
func (RawAPI) Explode(dim int32, x float64, y float64, z float64, radius float32, maxResistance float32, source ActorID, fire bool, breaksBlocks bool, allowUnderwater bool) (bool, error)
```

Explode calls the explode slot.

`Level::explode`. source may be 0 (no source actor).

- Parameters:
    - dim : `int32`
    - x : `float64`
    - y : `float64`
    - z : `float64`
    - radius : `float32`
    - maxResistance : `float32`
    - source : `ActorID`
    - fire : `bool`
    - breaksBlocks : `bool`
    - allowUnderwater : `bool`
- Return type: `(bool, error)`
- Slots: [`explode`](../cpp/world.md#explode)

### `Raw.GetDifficulty` {#Raw.GetDifficulty}

```go
func (RawAPI) GetDifficulty() (int32, bool, error)
```

GetDifficulty calls the `get_difficulty` slot.

Server / world-level settings.

- Return type: `(int32, bool, error)`
- Slots: [`get_difficulty`](../cpp/world.md#get_difficulty)

### `Raw.SetDifficulty` {#Raw.SetDifficulty}

```go
func (RawAPI) SetDifficulty(d int32) (bool, error)
```

SetDifficulty calls the `set_difficulty` slot.

- Parameters:
    - d : `int32`
- Return type: `(bool, error)`
- Slots: [`set_difficulty`](../cpp/world.md#set_difficulty)

### `Raw.GetSeed` {#Raw.GetSeed}

```go
func (RawAPI) GetSeed() (int64, bool, error)
```

GetSeed calls the `get_seed` slot.

- Return type: `(int64, bool, error)`
- Slots: [`get_seed`](../cpp/world.md#get_seed)

### `Raw.GameRuleGet` {#Raw.GameRuleGet}

```go
func (RawAPI) GameRuleGet(name string) ([]string, bool, error)
```

GameRuleGet calls the `game_rule_get` slot.

out sink receives SNBT {type:"bool"|"int"|"float", value:…}; false if unknown rule.

- Parameters:
    - name : `string`
- Return type: `([]string, bool, error)`
- Slots: [`game_rule_get`](../cpp/world.md#game_rule_get)

### `Raw.GameRuleSet` {#Raw.GameRuleSet}

```go
func (RawAPI) GameRuleSet(name string, value string) (bool, error)
```

GameRuleSet calls the `game_rule_set` slot.

- Parameters:
    - name : `string`
    - value : `string`
- Return type: `(bool, error)`
- Slots: [`game_rule_set`](../cpp/world.md#game_rule_set)

### `Raw.SpawnParticleFor` {#Raw.SpawnParticleFor}

```go
func (RawAPI) SpawnParticleFor(sel PlayerSel, dimension int32, effectName string, x float64, y float64, z float64) (bool, error)
```

SpawnParticleFor calls the `spawn_particle_for` slot.

Per-player particle packet (additive, gated by `struct_size`). Sends a SpawnParticleEffectPacket ONLY to the resolved player (`Player::sendNetworkPacket`) instead of `Level::spawnParticleEffect`'s dimension-wide broadcast — other clients never receive it. `dimension` is the vanilla dimension id carried in the packet; pass the dimension the coordinates refer to (normally the player's own — clients don't render particles for another dimension). False if the player is offline / can't be resolved.

- Parameters:
    - sel : `PlayerSel`
    - dimension : `int32`
    - effectName : `string`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- Return type: `(bool, error)`
- Slots: [`spawn_particle_for`](../cpp/world.md#spawn_particle_for)

### `Raw.Villages` {#Raw.Villages}

```go
func (RawAPI) Villages(dimension int32) ([]string, error)
```

Villages calls the villages slot.

Enumerate villages in a dimension. Each: {uuid, center:\[x,y,z\], bounds:{min,max}, `poi_count`}.

- Parameters:
    - dimension : `int32`
- Return type: `([]string, error)`
- Slots: [`villages`](../cpp/world.md#villages)

### `Raw.StructuresNear` {#Raw.StructuresNear}

```go
func (RawAPI) StructuresNear(dimension int32, x int32, y int32, z int32, radius int32) ([]string, error)
```

StructuresNear calls the `structures_near` slot.

Hardcoded spawn areas (nether fortress / witch hut / ocean monument / pillager outpost) whose chunks intersect a radius around (x,y,z). Each: {type, bounds:{min,max}}. Only LOADED chunks are inspected — a read-only query never force-loads.

- Parameters:
    - dimension : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
    - radius : `int32`
- Return type: `([]string, error)`
- Slots: [`structures_near`](../cpp/world.md#structures_near)

### `Raw.LevelGetBiome` {#Raw.LevelGetBiome}

```go
func (RawAPI) LevelGetBiome(dim int32, x int32, y int32, z int32) ([]string, bool, error)
```

LevelGetBiome calls the `level_get_biome` slot.

Level: biome, spawn, save, weather, path, sleep (dedicated fns)

- Parameters:
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
- Return type: `([]string, bool, error)`
- Slots: [`level_get_biome`](../cpp/world.md#level_get_biome)

### `Raw.LevelGetDefaultSpawn` {#Raw.LevelGetDefaultSpawn}

```go
func (RawAPI) LevelGetDefaultSpawn() (int32, int32, int32, bool, error)
```

LevelGetDefaultSpawn calls the `level_get_default_spawn` slot.

- Return type: `(int32, int32, int32, bool, error)`
- Slots: [`level_get_default_spawn`](../cpp/world.md#level_get_default_spawn)

### `Raw.LevelSetDefaultSpawn` {#Raw.LevelSetDefaultSpawn}

```go
func (RawAPI) LevelSetDefaultSpawn(x int32, y int32, z int32) (bool, error)
```

LevelSetDefaultSpawn calls the `level_set_default_spawn` slot.

- Parameters:
    - x : `int32`
    - y : `int32`
    - z : `int32`
- Return type: `(bool, error)`
- Slots: [`level_set_default_spawn`](../cpp/world.md#level_set_default_spawn)

### `Raw.LevelSave` {#Raw.LevelSave}

```go
func (RawAPI) LevelSave() (bool, error)
```

LevelSave calls the `level_save` slot.

- Return type: `(bool, error)`
- Slots: [`level_save`](../cpp/world.md#level_save)

### `Raw.LevelGetSleepStatus` {#Raw.LevelGetSleepStatus}

```go
func (RawAPI) LevelGetSleepStatus() ([]string, bool, error)
```

LevelGetSleepStatus calls the `level_get_sleep_status` slot.

SNBT {sleeping, `total_players`, `active_sleeping`}

- Return type: `([]string, bool, error)`
- Slots: [`level_get_sleep_status`](../cpp/world.md#level_get_sleep_status)

### `Raw.LevelUpdateWeather` {#Raw.LevelUpdateWeather}

```go
func (RawAPI) LevelUpdateWeather(rainLevel float32, rainTime int32, lightningLevel float32, lightningTime int32) (bool, error)
```

LevelUpdateWeather calls the `level_update_weather` slot.

- Parameters:
    - rainLevel : `float32`
    - rainTime : `int32`
    - lightningLevel : `float32`
    - lightningTime : `int32`
- Return type: `(bool, error)`
- Slots: [`level_update_weather`](../cpp/world.md#level_update_weather)

### `Raw.LevelFindPath` {#Raw.LevelFindPath}

```go
func (RawAPI) LevelFindPath(id ActorID, x int32, y int32, z int32) ([]string, bool, error)
```

LevelFindPath calls the `level_find_path` slot.

SNBT {nodes:\[{x,y,z},…\], reached:1b/0b}. NULL on every current host: pathfinding is not implemented.

- Parameters:
    - id : `ActorID`
    - x : `int32`
    - y : `int32`
    - z : `int32`
- Return type: `([]string, bool, error)`
- Slots: [`level_find_path`](../cpp/world.md#level_find_path)

### `Raw.LevelDeleteChunkKeys` {#Raw.LevelDeleteChunkKeys}

```go
func (RawAPI) LevelDeleteChunkKeys(dim int32, chunkX int32, chunkZ int32) (int32, error)
```

LevelDeleteChunkKeys calls the `level_delete_chunk_keys` slot.

Delete every save-file key belonging to one chunk, so the engine regenerates it from the generator on next load.

- Parameters:
    - dim : `int32`
    - chunkX : `int32`
    - chunkZ : `int32`
- Return type: `(int32, error)`
- Slots: [`level_delete_chunk_keys`](../cpp/world.md#level_delete_chunk_keys)

### `Raw.LevelChunksLoaded` {#Raw.LevelChunksLoaded}

```go
func (RawAPI) LevelChunksLoaded(dim int32, minX int32, minZ int32, maxX int32, maxZ int32) (int32, error)
```

LevelChunksLoaded calls the `level_chunks_loaded` slot.

Are the chunks covering \[min..max\] currently loaded in memory?

- Parameters:
    - dim : `int32`
    - minX : `int32`
    - minZ : `int32`
    - maxX : `int32`
    - maxZ : `int32`
- Return type: `(int32, error)`
- Slots: [`level_chunks_loaded`](../cpp/world.md#level_chunks_loaded)

### `Raw.LevelChunkKeys` {#Raw.LevelChunkKeys}

```go
func (RawAPI) LevelChunkKeys(dim int32, chunkX int32, chunkZ int32) ([]string, int32, error)
```

LevelChunkKeys calls the `level_chunk_keys` slot.

List every save-file key belonging to one chunk. One callback per key.

- Parameters:
    - dim : `int32`
    - chunkX : `int32`
    - chunkZ : `int32`
- Return type: `([]string, int32, error)`
- Slots: [`level_chunk_keys`](../cpp/world.md#level_chunk_keys)

### `Raw.LevelDeleteKey` {#Raw.LevelDeleteKey}

```go
func (RawAPI) LevelDeleteKey(key string) (bool, error)
```

LevelDeleteKey calls the `level_delete_key` slot.

Delete one chunk-category key, verbatim.

- Parameters:
    - key : `string`
- Return type: `(bool, error)`
- Slots: [`level_delete_key`](../cpp/world.md#level_delete_key)

### `Raw.LevelSetBiome` {#Raw.LevelSetBiome}

```go
func (RawAPI) LevelSetBiome(dim int32, minX int32, minZ int32, maxX int32, maxZ int32, biome string) (int32, error)
```

LevelSetBiome calls the `level_set_biome` slot.

Set the biome over an area.

- Parameters:
    - dim : `int32`
    - minX : `int32`
    - minZ : `int32`
    - maxX : `int32`
    - maxZ : `int32`
    - biome : `string`
- Return type: `(int32, error)`
- Slots: [`level_set_biome`](../cpp/world.md#level_set_biome)

### `Raw.GetExtraBlock` {#Raw.GetExtraBlock}

```go
func (RawAPI) GetExtraBlock(dim int32, x int32, y int32, z int32) ([]string, bool, error)
```

GetExtraBlock calls the `get_extra_block` slot.

Read the liquid layer. The sink receives a block name such as "minecraft:water"; an empty layer gives air.

- Parameters:
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
- Return type: `([]string, bool, error)`
- Slots: [`get_extra_block`](../cpp/world.md#get_extra_block)

### `Raw.SetExtraBlock` {#Raw.SetExtraBlock}

```go
func (RawAPI) SetExtraBlock(dim int32, x int32, y int32, z int32, blockSpec string, updateFlags int32) (bool, error)
```

SetExtraBlock calls the `set_extra_block` slot.

Write the liquid layer. `block_spec` takes a bare block name or full SNBT; write "minecraft:air" to clear it. `update_flags` is as in `edit_set_block_nbt`: bit 1 notifies neighbours, bit 2 syncs the client.

- Parameters:
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
    - blockSpec : `string`
    - updateFlags : `int32`
- Return type: `(bool, error)`
- Slots: [`set_extra_block`](../cpp/world.md#set_extra_block)

## Bulk world editing {#raw-edit}

### `Raw.EditSetBlockNbt` {#Raw.EditSetBlockNbt}

```go
func (RawAPI) EditSetBlockNbt(dim int32, x int32, y int32, z int32, snbt string, updateFlags int32) (bool, error)
```

EditSetBlockNbt calls the `edit_set_block_nbt` slot.

Write a block from serialized NBT ({name,states,version}, i.e. the shape `get_block` produces).

- Parameters:
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
    - snbt : `string`
    - updateFlags : `int32`
- Return type: `(bool, error)`
- Slots: [`edit_set_block_nbt`](../cpp/edit.md#edit_set_block_nbt)

### `Raw.EditSetBlockStates` {#Raw.EditSetBlockStates}

```go
func (RawAPI) EditSetBlockStates(dim int32, x int32, y int32, z int32, name string, statesSnbt string, updateFlags int32) (bool, error)
```

EditSetBlockStates calls the `edit_set_block_states` slot.

Write a block from a name + optional partial states. An empty `states_snbt` means all-default states; the version is taken from the default state on the loader side — the caller must not supply one.

- Parameters:
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
    - name : `string`
    - statesSnbt : `string`
    - updateFlags : `int32`
- Return type: `(bool, error)`
- Slots: [`edit_set_block_states`](../cpp/edit.md#edit_set_block_states)

### `Raw.EditSetBlockEntity` {#Raw.EditSetBlockEntity}

```go
func (RawAPI) EditSetBlockEntity(dim int32, x int32, y int32, z int32, snbt string) (bool, error)
```

EditSetBlockEntity calls the `edit_set_block_entity` slot.

Write a block entity's NBT back (`BlockActor::load`). The cell must already hold the matching block.

- Parameters:
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
    - snbt : `string`
- Return type: `(bool, error)`
- Slots: [`edit_set_block_entity`](../cpp/edit.md#edit_set_block_entity)

### `Raw.EditSpawnEntityNbt` {#Raw.EditSpawnEntityNbt}

```go
func (RawAPI) EditSpawnEntityNbt(dim int32, snbt string, usePos bool, x float64, y float64, z float64) (ActorID, bool, error)
```

EditSpawnEntityNbt calls the `edit_spawn_entity_nbt` slot.

Spawn an entity from full NBT (the inverse of `actor_snapshot`). When `use_pos` is true, (x,y,z) overrides the Pos tag; the UniqueID is reassigned by the engine and returned via out. NULL on every current host: the engine helper that gives a loaded actor a fresh UniqueID is inlined, and reusing the snapshot's own id makes the engine treat two actors as one.

- Parameters:
    - dim : `int32`
    - snbt : `string`
    - usePos : `bool`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- Return type: `(ActorID, bool, error)`
- Slots: [`edit_spawn_entity_nbt`](../cpp/edit.md#edit_spawn_entity_nbt)

### `Raw.EditTraceRay` {#Raw.EditTraceRay}

```go
func (RawAPI) EditTraceRay(id ActorID, maxDist float32, includeActors bool, includeBlocks bool) ([]string, bool, error)
```

EditTraceRay calls the `edit_trace_ray` slot.

Ray trace yielding the BLOCK coordinate and hit face: {type, block:\[x,y,z\], facing, pos:\[x,y,z\], entity}.

- Parameters:
    - id : `ActorID`
    - maxDist : `float32`
    - includeActors : `bool`
    - includeBlocks : `bool`
- Return type: `([]string, bool, error)`
- Slots: [`edit_trace_ray`](../cpp/edit.md#edit_trace_ray)

### `Raw.EditFillRegion` {#Raw.EditFillRegion}

```go
func (RawAPI) EditFillRegion(dimension int32, x1 int32, y1 int32, z1 int32, x2 int32, y2 int32, z2 int32, blockSpec string, updateFlags int32) (int64, error)
```

EditFillRegion calls the `edit_fill_region` slot.

Fills a box with one block. `block_spec` is a bare name such as "minecraft:stone" or full SNBT; it is resolved once for the whole box. `update_flags` is as in `edit_set_block_nbt`: bit 1 notifies neighbours, bit 2 syncs the client; 0 is the fastest and leaves the client to catch up on the next chunk send. Returns the number of cells written, or -1 if the dimension is not ready, the spec does not resolve, or the box exceeds 2^24 cells. Server thread only.

- Parameters:
    - dimension : `int32`
    - x1 : `int32`
    - y1 : `int32`
    - z1 : `int32`
    - x2 : `int32`
    - y2 : `int32`
    - z2 : `int32`
    - blockSpec : `string`
    - updateFlags : `int32`
- Return type: `(int64, error)`
- Slots: [`edit_fill_region`](../cpp/edit.md#edit_fill_region)

## Players {#raw-player}

### `Raw.GetPlayerPosition` {#Raw.GetPlayerPosition}

```go
func (RawAPI) GetPlayerPosition(name string) (PlayerPos, error)
```

GetPlayerPosition calls the `get_player_position` slot.

Look up a connected player's feet position and dimension by name. Used to pick selection corners from where the player is standing. Server thread only.

- Parameters:
    - name : `string`
- Return type: `(PlayerPos, error)`
- Slots: [`get_player_position`](../cpp/player.md#get_player_position)

### `Raw.ListPlayers` {#Raw.ListPlayers}

```go
func (RawAPI) ListPlayers() ([]string, error)
```

ListPlayers calls the `list_players` slot.

One SNBT per online player: {name,xuid,uuid,dim,x,y,z}.

- Return type: `([]string, error)`
- Slots: [`list_players`](../cpp/player.md#list_players)

### `Raw.PlayerResolve` {#Raw.PlayerResolve}

```go
func (RawAPI) PlayerResolve(sel PlayerSel) (ActorID, bool, error)
```

PlayerResolve calls the `player_resolve` slot.

Resolve a player selector to their ActorUniqueID (bridges into the actor\_\* API).

- Parameters:
    - sel : `PlayerSel`
- Return type: `(ActorID, bool, error)`
- Slots: [`player_resolve`](../cpp/player.md#player_resolve)

### `Raw.PlayerSendMessage` {#Raw.PlayerSendMessage}

```go
func (RawAPI) PlayerSendMessage(sel PlayerSel, msg string) (bool, error)
```

PlayerSendMessage calls the `player_send_message` slot.

- Parameters:
    - sel : `PlayerSel`
    - msg : `string`
- Return type: `(bool, error)`
- Slots: [`player_send_message`](../cpp/player.md#player_send_message)

### `Raw.PlayerDisconnect` {#Raw.PlayerDisconnect}

```go
func (RawAPI) PlayerDisconnect(sel PlayerSel, reason string) (bool, error)
```

PlayerDisconnect calls the `player_disconnect` slot.

- Parameters:
    - sel : `PlayerSel`
    - reason : `string`
- Return type: `(bool, error)`
- Slots: [`player_disconnect`](../cpp/player.md#player_disconnect)

### `Raw.BroadcastMessage` {#Raw.BroadcastMessage}

```go
func (RawAPI) BroadcastMessage(msg string) error
```

BroadcastMessage calls the `broadcast_message` slot.

sendMessage to every online player.

- Parameters:
    - msg : `string`
- Return type: `error`
- Slots: [`broadcast_message`](../cpp/player.md#broadcast_message)

### `Raw.PlayerSetGamemode` {#Raw.PlayerSetGamemode}

```go
func (RawAPI) PlayerSetGamemode(sel PlayerSel, mode int32) (bool, error)
```

PlayerSetGamemode calls the `player_set_gamemode` slot.

0=survival 1=creative 2=adventure 6=spectator, native (`Player::setPlayerGameType`).

- Parameters:
    - sel : `PlayerSel`
    - mode : `int32`
- Return type: `(bool, error)`
- Slots: [`player_set_gamemode`](../cpp/player.md#player_set_gamemode)

### `Raw.PlayerTeleport` {#Raw.PlayerTeleport}

```go
func (RawAPI) PlayerTeleport(sel PlayerSel, dim int32, x float64, y float64, z float64) (bool, error)
```

PlayerTeleport calls the `player_teleport` slot.

Teleport natively (`Actor::teleport`). Custom dimensions (id &gt;= 3) are allowed; the dimension bridge must produce an engine instance whose id matches, or the call fails instead of sending the player into a mismatched dimension.

- Parameters:
    - sel : `PlayerSel`
    - dim : `int32`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- Return type: `(bool, error)`
- Slots: [`player_teleport`](../cpp/player.md#player_teleport)

### `Raw.PlayerGetNum` {#Raw.PlayerGetNum}

```go
func (RawAPI) PlayerGetNum(sel PlayerSel, prop int32) (float64, bool, error)
```

PlayerGetNum calls the `player_get_num` slot.

The four \*\_get\_num slots share one convention. The return is whether the host has an answer, and \*out is the answer; a false leaves \*out untouched.

- Parameters:
    - sel : `PlayerSel`
    - prop : `int32`
- Return type: `(float64, bool, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Raw.PlayerGetStr` {#Raw.PlayerGetStr}

```go
func (RawAPI) PlayerGetStr(sel PlayerSel, prop int32) ([]string, bool, error)
```

PlayerGetStr calls the `player_get_str` slot.

- Parameters:
    - sel : `PlayerSel`
    - prop : `int32`
- Return type: `([]string, bool, error)`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Raw.PlayerSetNum` {#Raw.PlayerSetNum}

```go
func (RawAPI) PlayerSetNum(sel PlayerSel, prop int32, v float64) (bool, error)
```

PlayerSetNum calls the `player_set_num` slot.

- Parameters:
    - sel : `PlayerSel`
    - prop : `int32`
    - v : `float64`
- Return type: `(bool, error)`
- Slots: [`player_set_num`](../cpp/player.md#player_set_num)

### `Raw.PlayerAction` {#Raw.PlayerAction}

```go
func (RawAPI) PlayerAction(sel PlayerSel, action int32, sarg string, a float64, b float64, c float64) ([]string, bool, error)
```

PlayerAction calls the `player_action` slot.

- Parameters:
    - sel : `PlayerSel`
    - action : `int32`
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `([]string, bool, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Raw.PlayerSendMessageTyped` {#Raw.PlayerSendMessageTyped}

```go
func (RawAPI) PlayerSendMessageTyped(sel PlayerSel, msg string, typeArg int32) (bool, error)
```

PlayerSendMessageTyped calls the `player_send_message_typed` slot.

Send a message of a specific TextPacketType to one player (additive, gated by `struct_size`). `type` is a TextPacketType value: 0 Raw · 1 Chat · 2 Translate · 3 Popup · 4 JukeboxPopup · 5 Tip · 6 SystemMessage · 7 Whisper · 8 Announcement · 9 TextObjectWhisper · 10 TextObject · 11 TextObjectAnnouncement. Out-of-range falls back to Raw. Single-string body (like LSE tell): the author/param kinds (Chat/Whisper/Translate) arrive as plain text. plain `player_send_message` remains the Raw/Chat convenience path.

- Parameters:
    - sel : `PlayerSel`
    - msg : `string`
    - typeArg : `int32`
- Return type: `(bool, error)`
- Slots: [`player_send_message_typed`](../cpp/player.md#player_send_message_typed)

### `Raw.PlayerGetCarriedItem` {#Raw.PlayerGetCarriedItem}

```go
func (RawAPI) PlayerGetCarriedItem(sel PlayerSel) ([]string, bool, error)
```

PlayerGetCarriedItem calls the `player_get_carried_item` slot.

Player: equipment, cooldown, network (dedicated fns)

- Parameters:
    - sel : `PlayerSel`
- Return type: `([]string, bool, error)`
- Slots: [`player_get_carried_item`](../cpp/player.md#player_get_carried_item)

### `Raw.PlayerGetItem` {#Raw.PlayerGetItem}

```go
func (RawAPI) PlayerGetItem(sel PlayerSel, slot int32) ([]string, bool, error)
```

PlayerGetItem calls the `player_get_item` slot.

- Parameters:
    - sel : `PlayerSel`
    - slot : `int32`
- Return type: `([]string, bool, error)`
- Slots: [`player_get_item`](../cpp/player.md#player_get_item)

### `Raw.PlayerSetItem` {#Raw.PlayerSetItem}

```go
func (RawAPI) PlayerSetItem(sel PlayerSel, slot int32, itemSnbt string) (bool, error)
```

PlayerSetItem calls the `player_set_item` slot.

- Parameters:
    - sel : `PlayerSel`
    - slot : `int32`
    - itemSnbt : `string`
- Return type: `(bool, error)`
- Slots: [`player_set_item`](../cpp/player.md#player_set_item)

### `Raw.PlayerGetEquipment` {#Raw.PlayerGetEquipment}

```go
func (RawAPI) PlayerGetEquipment(sel PlayerSel) ([]string, bool, error)
```

PlayerGetEquipment calls the `player_get_equipment` slot.

All equipment as SNBT: \[{slot, `item_snbt`},…\] slot: 0=mainhand 1=offhand 2-5=armor

- Parameters:
    - sel : `PlayerSel`
- Return type: `([]string, bool, error)`
- Slots: [`player_get_equipment`](../cpp/player.md#player_get_equipment)

### `Raw.PlayerGetCooldown` {#Raw.PlayerGetCooldown}

```go
func (RawAPI) PlayerGetCooldown(sel PlayerSel, itemName string) (int32, error)
```

PlayerGetCooldown calls the `player_get_cooldown` slot.

Ticks remaining for an item cooldown (-1 if not on cooldown / player offline).

- Parameters:
    - sel : `PlayerSel`
    - itemName : `string`
- Return type: `(int32, error)`
- Slots: [`player_get_cooldown`](../cpp/player.md#player_get_cooldown)

### `Raw.PlayerStartCooldown` {#Raw.PlayerStartCooldown}

```go
func (RawAPI) PlayerStartCooldown(sel PlayerSel, itemName string, ticks int32) (bool, error)
```

PlayerStartCooldown calls the `player_start_cooldown` slot.

- Parameters:
    - sel : `PlayerSel`
    - itemName : `string`
    - ticks : `int32`
- Return type: `(bool, error)`
- Slots: [`player_start_cooldown`](../cpp/player.md#player_start_cooldown)

### `Raw.PlayerGetNetworkStatus` {#Raw.PlayerGetNetworkStatus}

```go
func (RawAPI) PlayerGetNetworkStatus(sel PlayerSel) ([]string, bool, error)
```

PlayerGetNetworkStatus calls the `player_get_network_status` slot.

- Parameters:
    - sel : `PlayerSel`
- Return type: `([]string, bool, error)`
- Slots: [`player_get_network_status`](../cpp/player.md#player_get_network_status)

### `Raw.PlayerSendTitle` {#Raw.PlayerSendTitle}

```go
func (RawAPI) PlayerSendTitle(sel PlayerSel, typeArg int32, text string, fadeInTicks int32, stayTicks int32, fadeOutTicks int32) (bool, error)
```

PlayerSendTitle calls the `player_send_title` slot.

Titles `PACT_SET_TITLE` (`player_action` opcode 6) reaches the client by running the console command `title "<name>" title <text>`. Three things are wrong with that and none of them are theoretical: - the text is pasted into a command line unquoted, so a plot named `He said "hi"` truncates the command; - `title`'s text parameter is a `message`, which expands selectors — a plot named `@e` is a command injection, not a name; - `/title` has no way to set fade/stay for the same call, so timing is whatever the client last stored. This slot builds a real SetTitlePacket instead. No wire format crosses the FFI (the packet is constructed field-by-field on this side), so it survives protocol bumps the way `spawn_particle_for` does.

- Parameters:
    - sel : `PlayerSel`
    - typeArg : `int32`
    - text : `string`
    - fadeInTicks : `int32`
    - stayTicks : `int32`
    - fadeOutTicks : `int32`
- Return type: `(bool, error)`
- Slots: [`player_send_title`](../cpp/player.md#player_send_title)

### `Raw.PlayerConnId` {#Raw.PlayerConnId}

```go
func (RawAPI) PlayerConnId(who PlayerSel) (uint64, error)
```

PlayerConnId calls the `player_conn_id` slot.

This player's connection id — the same number packet interceptors see in the packet context.

- Parameters:
    - who : `PlayerSel`
- Return type: `(uint64, error)`
- Slots: [`player_conn_id`](../cpp/player.md#player_conn_id)

## Actors {#raw-entity}

### `Raw.ActorSnapshot` {#Raw.ActorSnapshot}

```go
func (RawAPI) ActorSnapshot(id ActorID) ([]string, bool, error)
```

ActorSnapshot calls the `actor_snapshot` slot.

Full `Actor::save` NBT as SNBT.

- Parameters:
    - id : `ActorID`
- Return type: `([]string, bool, error)`
- Slots: [`actor_snapshot`](../cpp/entity.md#actor_snapshot)

### `Raw.ActorGetNum` {#Raw.ActorGetNum}

```go
func (RawAPI) ActorGetNum(id ActorID, prop int32) (float64, bool, error)
```

ActorGetNum calls the `actor_get_num` slot.

- Parameters:
    - id : `ActorID`
    - prop : `int32`
- Return type: `(float64, bool, error)`
- Slots: [`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Raw.ActorGetStr` {#Raw.ActorGetStr}

```go
func (RawAPI) ActorGetStr(id ActorID, prop int32) ([]string, bool, error)
```

ActorGetStr calls the `actor_get_str` slot.

- Parameters:
    - id : `ActorID`
    - prop : `int32`
- Return type: `([]string, bool, error)`
- Slots: [`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Raw.ActorAction` {#Raw.ActorAction}

```go
func (RawAPI) ActorAction(id ActorID, action int32, sarg string, a float64, b float64, c float64) ([]string, bool, error)
```

ActorAction calls the `actor_action` slot.

- Parameters:
    - id : `ActorID`
    - action : `int32`
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `([]string, bool, error)`
- Slots: [`actor_action`](../cpp/entity.md#actor_action)

### `Raw.SpawnMob` {#Raw.SpawnMob}

```go
func (RawAPI) SpawnMob(dim int32, typeName string, x float64, y float64, z float64) (ActorID, bool, error)
```

SpawnMob calls the `spawn_mob` slot.

Spawn a mob (`Spawner::spawnMob`); on success \*out = its ActorUniqueID.

- Parameters:
    - dim : `int32`
    - typeName : `string`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- Return type: `(ActorID, bool, error)`
- Slots: [`spawn_mob`](../cpp/entity.md#spawn_mob)

### `Raw.ActorGetVehicle` {#Raw.ActorGetVehicle}

```go
func (RawAPI) ActorGetVehicle(id ActorID) (ActorID, bool, error)
```

ActorGetVehicle calls the `actor_get_vehicle` slot.

Actor: relationships, equipment, effects, geometry (dedicated fns)

- Parameters:
    - id : `ActorID`
- Return type: `(ActorID, bool, error)`
- Slots: [`actor_get_vehicle`](../cpp/entity.md#actor_get_vehicle)

### `Raw.ActorGetFirstPassenger` {#Raw.ActorGetFirstPassenger}

```go
func (RawAPI) ActorGetFirstPassenger(id ActorID) (ActorID, bool, error)
```

ActorGetFirstPassenger calls the `actor_get_first_passenger` slot.

- Parameters:
    - id : `ActorID`
- Return type: `(ActorID, bool, error)`
- Slots: [`actor_get_first_passenger`](../cpp/entity.md#actor_get_first_passenger)

### `Raw.ActorGetOwner` {#Raw.ActorGetOwner}

```go
func (RawAPI) ActorGetOwner(id ActorID) (ActorID, bool, error)
```

ActorGetOwner calls the `actor_get_owner` slot.

- Parameters:
    - id : `ActorID`
- Return type: `(ActorID, bool, error)`
- Slots: [`actor_get_owner`](../cpp/entity.md#actor_get_owner)

### `Raw.ActorGetTarget` {#Raw.ActorGetTarget}

```go
func (RawAPI) ActorGetTarget(id ActorID) (ActorID, bool, error)
```

ActorGetTarget calls the `actor_get_target` slot.

- Parameters:
    - id : `ActorID`
- Return type: `(ActorID, bool, error)`
- Slots: [`actor_get_target`](../cpp/entity.md#actor_get_target)

### `Raw.ActorGetEquippedItem` {#Raw.ActorGetEquippedItem}

```go
func (RawAPI) ActorGetEquippedItem(id ActorID, slot int32) ([]string, bool, error)
```

ActorGetEquippedItem calls the `actor_get_equipped_item` slot.

slot: 0=mainhand 1=offhand 2=helmet 3=chestplate 4=leggings 5=boots

- Parameters:
    - id : `ActorID`
    - slot : `int32`
- Return type: `([]string, bool, error)`
- Slots: [`actor_get_equipped_item`](../cpp/entity.md#actor_get_equipped_item)

### `Raw.ActorSetEquippedItem` {#Raw.ActorSetEquippedItem}

```go
func (RawAPI) ActorSetEquippedItem(id ActorID, slot int32, itemSnbt string) (bool, error)
```

ActorSetEquippedItem calls the `actor_set_equipped_item` slot.

- Parameters:
    - id : `ActorID`
    - slot : `int32`
    - itemSnbt : `string`
- Return type: `(bool, error)`
- Slots: [`actor_set_equipped_item`](../cpp/entity.md#actor_set_equipped_item)

### `Raw.ActorGetEffects` {#Raw.ActorGetEffects}

```go
func (RawAPI) ActorGetEffects(id ActorID) ([]string, bool, error)
```

ActorGetEffects calls the `actor_get_effects` slot.

SNBT \[{id, ticks, amplifier, visible},…\]

- Parameters:
    - id : `ActorID`
- Return type: `([]string, bool, error)`
- Slots: [`actor_get_effects`](../cpp/entity.md#actor_get_effects)

### `Raw.ActorGetStatusFlag` {#Raw.ActorGetStatusFlag}

```go
func (RawAPI) ActorGetStatusFlag(id ActorID, flagIndex int32) (bool, error)
```

ActorGetStatusFlag calls the `actor_get_status_flag` slot.

`flag_index`: ActorFlags enum value (0-based).

- Parameters:
    - id : `ActorID`
    - flagIndex : `int32`
- Return type: `(bool, error)`
- Slots: [`actor_get_status_flag`](../cpp/entity.md#actor_get_status_flag)

### `Raw.ActorSetStatusFlag` {#Raw.ActorSetStatusFlag}

```go
func (RawAPI) ActorSetStatusFlag(id ActorID, flagIndex int32, value bool) (bool, error)
```

ActorSetStatusFlag calls the `actor_set_status_flag` slot.

- Parameters:
    - id : `ActorID`
    - flagIndex : `int32`
    - value : `bool`
- Return type: `(bool, error)`
- Slots: [`actor_set_status_flag`](../cpp/entity.md#actor_set_status_flag)

### `Raw.ActorTraceRay` {#Raw.ActorTraceRay}

```go
func (RawAPI) ActorTraceRay(id ActorID, maxDist float32, includeActors bool, includeBlocks bool) ([]string, bool, error)
```

ActorTraceRay calls the `actor_trace_ray` slot.

SNBT {type:"entity"|"block"|"none", pos:\[x,y,z\], `entity_id`?, `block_name`?}

- Parameters:
    - id : `ActorID`
    - maxDist : `float32`
    - includeActors : `bool`
    - includeBlocks : `bool`
- Return type: `([]string, bool, error)`
- Slots: [`actor_trace_ray`](../cpp/entity.md#actor_trace_ray)

### `Raw.ActorDistanceTo` {#Raw.ActorDistanceTo}

```go
func (RawAPI) ActorDistanceTo(id ActorID, other ActorID) (float64, bool, error)
```

ActorDistanceTo calls the `actor_distance_to` slot.

- Parameters:
    - id : `ActorID`
    - other : `ActorID`
- Return type: `(float64, bool, error)`
- Slots: [`actor_distance_to`](../cpp/entity.md#actor_distance_to)

### `Raw.ActorGetAabb` {#Raw.ActorGetAabb}

```go
func (RawAPI) ActorGetAabb(id ActorID) ([]string, bool, error)
```

ActorGetAabb calls the `actor_get_aabb` slot.

SNBT {min:\[x,y,z\], max:\[x,y,z\]}

- Parameters:
    - id : `ActorID`
- Return type: `([]string, bool, error)`
- Slots: [`actor_get_aabb`](../cpp/entity.md#actor_get_aabb)

### `Raw.ActorClone` {#Raw.ActorClone}

```go
func (RawAPI) ActorClone(id ActorID, dim int32, x float64, y float64, z float64) (ActorID, bool, error)
```

ActorClone calls the `actor_clone` slot.

- Parameters:
    - id : `ActorID`
    - dim : `int32`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- Return type: `(ActorID, bool, error)`
- Slots: [`actor_clone`](../cpp/entity.md#actor_clone)

## Blocks {#raw-block}

### `Raw.BlockGetNum` {#Raw.BlockGetNum}

```go
func (RawAPI) BlockGetNum(dim int32, x int32, y int32, z int32, prop int32) (float64, bool, error)
```

BlockGetNum calls the `block_get_num` slot.

§D blocks & block entities

- Parameters:
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
    - prop : `int32`
- Return type: `(float64, bool, error)`
- Slots: [`block_get_num`](../cpp/block.md#block_get_num)

### `Raw.BlockGetStr` {#Raw.BlockGetStr}

```go
func (RawAPI) BlockGetStr(dim int32, x int32, y int32, z int32, prop int32) ([]string, bool, error)
```

BlockGetStr calls the `block_get_str` slot.

- Parameters:
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
    - prop : `int32`
- Return type: `([]string, bool, error)`
- Slots: [`block_get_str`](../cpp/block.md#block_get_str)

### `Raw.BlockAction` {#Raw.BlockAction}

```go
func (RawAPI) BlockAction(dim int32, x int32, y int32, z int32, action int32, sarg string) ([]string, bool, error)
```

BlockAction calls the `block_action` slot.

- Parameters:
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
    - action : `int32`
    - sarg : `string`
- Return type: `([]string, bool, error)`
- Slots: [`block_action`](../cpp/block.md#block_action)

### `Raw.BlockEntitySnbt` {#Raw.BlockEntitySnbt}

```go
func (RawAPI) BlockEntitySnbt(dim int32, x int32, y int32, z int32) ([]string, bool, error)
```

BlockEntitySnbt calls the `block_entity_snbt` slot.

`BlockActor::save` (with default SaveContext) as SNBT; false if none there.

- Parameters:
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
- Return type: `([]string, bool, error)`
- Slots: [`block_entity_snbt`](../cpp/block.md#block_entity_snbt)

### `Raw.BlockGetState` {#Raw.BlockGetState}

```go
func (RawAPI) BlockGetState(dim int32, x int32, y int32, z int32, stateName string) ([]string, bool, error)
```

BlockGetState calls the `block_get_state` slot.

Block: state get/set, collision shape (dedicated fns)

- Parameters:
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
    - stateName : `string`
- Return type: `([]string, bool, error)`
- Slots: [`block_get_state`](../cpp/block.md#block_get_state)

### `Raw.BlockSetState` {#Raw.BlockSetState}

```go
func (RawAPI) BlockSetState(dim int32, x int32, y int32, z int32, stateName string, value string) (bool, error)
```

BlockSetState calls the `block_set_state` slot.

- Parameters:
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
    - stateName : `string`
    - value : `string`
- Return type: `(bool, error)`
- Slots: [`block_set_state`](../cpp/block.md#block_set_state)

### `Raw.BlockGetCollisionShape` {#Raw.BlockGetCollisionShape}

```go
func (RawAPI) BlockGetCollisionShape(dim int32, x int32, y int32, z int32) ([]string, bool, error)
```

BlockGetCollisionShape calls the `block_get_collision_shape` slot.

- Parameters:
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
- Return type: `([]string, bool, error)`
- Slots: [`block_get_collision_shape`](../cpp/block.md#block_get_collision_shape)

## Items and containers {#raw-item}

### `Raw.ItemGetNum` {#Raw.ItemGetNum}

```go
func (RawAPI) ItemGetNum(itemSnbt string, prop int32) (float64, bool, error)
```

ItemGetNum calls the `item_get_num` slot.

§E items (SNBT value objects) & containers

- Parameters:
    - itemSnbt : `string`
    - prop : `int32`
- Return type: `(float64, bool, error)`
- Slots: [`item_get_num`](../cpp/item.md#item_get_num)

### `Raw.ItemGetStr` {#Raw.ItemGetStr}

```go
func (RawAPI) ItemGetStr(itemSnbt string, prop int32) ([]string, bool, error)
```

ItemGetStr calls the `item_get_str` slot.

- Parameters:
    - itemSnbt : `string`
    - prop : `int32`
- Return type: `([]string, bool, error)`
- Slots: [`item_get_str`](../cpp/item.md#item_get_str)

### `Raw.ItemTransform` {#Raw.ItemTransform}

```go
func (RawAPI) ItemTransform(itemSnbt string, op int32, sarg string, narg float64) ([]string, bool, error)
```

ItemTransform calls the `item_transform` slot.

Rebuild → mutate → serialize; out receives the NEW item SNBT.

- Parameters:
    - itemSnbt : `string`
    - op : `int32`
    - sarg : `string`
    - narg : `float64`
- Return type: `([]string, bool, error)`
- Slots: [`item_transform`](../cpp/item.md#item_transform)

### `Raw.ContainerSize` {#Raw.ContainerSize}

```go
func (RawAPI) ContainerSize(ref ContainerRef) (int32, bool, error)
```

ContainerSize calls the `container_size` slot.

- Parameters:
    - ref : `ContainerRef`
- Return type: `(int32, bool, error)`
- Slots: [`container_size`](../cpp/item.md#container_size)

### `Raw.ContainerGetItem` {#Raw.ContainerGetItem}

```go
func (RawAPI) ContainerGetItem(ref ContainerRef, slot int32) ([]string, bool, error)
```

ContainerGetItem calls the `container_get_item` slot.

Slot content as item SNBT (empty slots yield the air item's SNBT).

- Parameters:
    - ref : `ContainerRef`
    - slot : `int32`
- Return type: `([]string, bool, error)`
- Slots: [`container_get_item`](../cpp/item.md#container_get_item)

### `Raw.ContainerSetItem` {#Raw.ContainerSetItem}

```go
func (RawAPI) ContainerSetItem(ref ContainerRef, slot int32, itemSnbt string) (bool, error)
```

ContainerSetItem calls the `container_set_item` slot.

- Parameters:
    - ref : `ContainerRef`
    - slot : `int32`
    - itemSnbt : `string`
- Return type: `(bool, error)`
- Slots: [`container_set_item`](../cpp/item.md#container_set_item)

### `Raw.ContainerAddItem` {#Raw.ContainerAddItem}

```go
func (RawAPI) ContainerAddItem(ref ContainerRef, itemSnbt string) (bool, error)
```

ContainerAddItem calls the `container_add_item` slot.

- Parameters:
    - ref : `ContainerRef`
    - itemSnbt : `string`
- Return type: `(bool, error)`
- Slots: [`container_add_item`](../cpp/item.md#container_add_item)

### `Raw.ContainerRemoveItem` {#Raw.ContainerRemoveItem}

```go
func (RawAPI) ContainerRemoveItem(ref ContainerRef, slot int32, count int32) (bool, error)
```

ContainerRemoveItem calls the `container_remove_item` slot.

- Parameters:
    - ref : `ContainerRef`
    - slot : `int32`
    - count : `int32`
- Return type: `(bool, error)`
- Slots: [`container_remove_item`](../cpp/item.md#container_remove_item)

### `Raw.ContainerClear` {#Raw.ContainerClear}

```go
func (RawAPI) ContainerClear(ref ContainerRef) (bool, error)
```

ContainerClear calls the `container_clear` slot.

- Parameters:
    - ref : `ContainerRef`
- Return type: `(bool, error)`
- Slots: [`container_clear`](../cpp/item.md#container_clear)

### `Raw.ItemGetEnchants` {#Raw.ItemGetEnchants}

```go
func (RawAPI) ItemGetEnchants(itemSnbt string) ([]string, bool, error)
```

ItemGetEnchants calls the `item_get_enchants` slot.

SNBT \[{id, level},…\]

- Parameters:
    - itemSnbt : `string`
- Return type: `([]string, bool, error)`
- Slots: [`item_get_enchants`](../cpp/item.md#item_get_enchants)

### `Raw.ItemSetEnchants` {#Raw.ItemSetEnchants}

```go
func (RawAPI) ItemSetEnchants(itemSnbt string, enchantsSnbt string) ([]string, bool, error)
```

ItemSetEnchants calls the `item_set_enchants` slot.

`enchants_snbt` = \[{id, level},…\]; out = new item SNBT. NULL on every current host: writing enchantments is not implemented, and `item_get_enchants` is the read side.

- Parameters:
    - itemSnbt : `string`
    - enchantsSnbt : `string`
- Return type: `([]string, bool, error)`
- Slots: [`item_set_enchants`](../cpp/item.md#item_set_enchants)

### `Raw.ItemMatches` {#Raw.ItemMatches}

```go
func (RawAPI) ItemMatches(a string, b string) (bool, error)
```

ItemMatches calls the `item_matches` slot.

- Parameters:
    - a : `string`
    - b : `string`
- Return type: `(bool, error)`
- Slots: [`item_matches`](../cpp/item.md#item_matches)

### `Raw.ItemGetUserData` {#Raw.ItemGetUserData}

```go
func (RawAPI) ItemGetUserData(itemSnbt string) ([]string, bool, error)
```

ItemGetUserData calls the `item_get_user_data` slot.

- Parameters:
    - itemSnbt : `string`
- Return type: `([]string, bool, error)`
- Slots: [`item_get_user_data`](../cpp/item.md#item_get_user_data)

### `Raw.ContainerRefresh` {#Raw.ContainerRefresh}

```go
func (RawAPI) ContainerRefresh(ref ContainerRef) (bool, error)
```

ContainerRefresh calls the `container_refresh` slot.

Resend a player-owned container (which 0..3) to its owner. Returns false for block containers (which == 4) — a chest has no single owner to resend to; its viewers are refreshed by the engine's own container transaction path.

- Parameters:
    - ref : `ContainerRef`
- Return type: `(bool, error)`
- Slots: [`container_refresh`](../cpp/item.md#container_refresh)

## Scoreboard {#raw-scoreboard}

### `Raw.ScoreboardOp` {#Raw.ScoreboardOp}

```go
func (RawAPI) ScoreboardOp(op int32, a string, b string, n int64) ([]string, bool, error)
```

ScoreboardOp calls the `scoreboard_op` slot.

§F scoreboard

- Parameters:
    - op : `int32`
    - a : `string`
    - b : `string`
    - n : `int64`
- Return type: `([]string, bool, error)`
- Slots: [`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)

## NBT and the key-value database {#raw-data}

### `Raw.NbtBinaryToSnbt` {#Raw.NbtBinaryToSnbt}

```go
func (RawAPI) NbtBinaryToSnbt(data []byte, fmt int32) ([]string, bool, error)
```

NbtBinaryToSnbt calls the `nbt_binary_to_snbt` slot.

- Parameters:
    - data : `[]byte`
    - fmt : `int32`
- Return type: `([]string, bool, error)`
- Slots: [`nbt_binary_to_snbt`](../cpp/data.md#nbt_binary_to_snbt)

### `Raw.KvdbOpen` {#Raw.KvdbOpen}

```go
func (RawAPI) KvdbOpen(path string, createIfMissing bool) (KvDbHandle, error)
```

KvdbOpen calls the `kvdb_open` slot.

KvDb: THREAD-SAFE (internal mutex). Paths are confined to the mod's own data directory; ".." and absolute paths are rejected. Handles are owned by the loader and force-closed (with a warning) at mod unload.

- Parameters:
    - path : `string`
    - createIfMissing : `bool`
- Return type: `(KvDbHandle, error)`
- Slots: [`kvdb_open`](../cpp/data.md#kvdb_open)

### `Raw.KvdbClose` {#Raw.KvdbClose}

```go
func (RawAPI) KvdbClose(hArg KvDbHandle) error
```

KvdbClose calls the `kvdb_close` slot.

- Parameters:
    - hArg : `KvDbHandle`
- Return type: `error`
- Slots: [`kvdb_close`](../cpp/data.md#kvdb_close)

### `Raw.KvdbGet` {#Raw.KvdbGet}

```go
func (RawAPI) KvdbGet(hArg KvDbHandle, key string) ([]string, bool, error)
```

KvdbGet calls the `kvdb_get` slot.

- Parameters:
    - hArg : `KvDbHandle`
    - key : `string`
- Return type: `([]string, bool, error)`
- Slots: [`kvdb_get`](../cpp/data.md#kvdb_get)

### `Raw.KvdbSet` {#Raw.KvdbSet}

```go
func (RawAPI) KvdbSet(hArg KvDbHandle, key string, value string) (bool, error)
```

KvdbSet calls the `kvdb_set` slot.

- Parameters:
    - hArg : `KvDbHandle`
    - key : `string`
    - value : `string`
- Return type: `(bool, error)`
- Slots: [`kvdb_set`](../cpp/data.md#kvdb_set)

### `Raw.KvdbDel` {#Raw.KvdbDel}

```go
func (RawAPI) KvdbDel(hArg KvDbHandle, key string) (bool, error)
```

KvdbDel calls the `kvdb_del` slot.

- Parameters:
    - hArg : `KvDbHandle`
    - key : `string`
- Return type: `(bool, error)`
- Slots: [`kvdb_del`](../cpp/data.md#kvdb_del)

### `Raw.KvdbHas` {#Raw.KvdbHas}

```go
func (RawAPI) KvdbHas(hArg KvDbHandle, key string) (bool, error)
```

KvdbHas calls the `kvdb_has` slot.

- Parameters:
    - hArg : `KvDbHandle`
    - key : `string`
- Return type: `(bool, error)`
- Slots: [`kvdb_has`](../cpp/data.md#kvdb_has)

### `Raw.KvdbIsEmpty` {#Raw.KvdbIsEmpty}

```go
func (RawAPI) KvdbIsEmpty(hArg KvDbHandle) (bool, error)
```

KvdbIsEmpty calls the `kvdb_is_empty` slot.

- Parameters:
    - hArg : `KvDbHandle`
- Return type: `(bool, error)`
- Slots: [`kvdb_is_empty`](../cpp/data.md#kvdb_is_empty)

## Economy {#raw-money}

### `Raw.GetMoney` {#Raw.GetMoney}

```go
func (RawAPI) GetMoney(xuid string) (int64, error)
```

GetMoney calls the `get_money` slot.

Balance. Returns -1 on failure (empty xuid, database error, or absent backend); a real balance is never negative, so &lt; 0 means "cannot say". Note that it opens an account at the configured default for an unseen xuid, so this is not a side-effect-free read.

- Parameters:
    - xuid : `string`
- Return type: `(int64, error)`
- Slots: [`get_money`](../cpp/money.md#get_money)

### `Raw.SetMoney` {#Raw.SetMoney}

```go
func (RawAPI) SetMoney(xuid string, money int64) (bool, error)
```

SetMoney calls the `set_money` slot.

Set to money, which is a target balance rather than a delta.

- Parameters:
    - xuid : `string`
    - money : `int64`
- Return type: `(bool, error)`
- Slots: [`set_money`](../cpp/money.md#set_money)

### `Raw.AddMoney` {#Raw.AddMoney}

```go
func (RawAPI) AddMoney(xuid string, money int64) (bool, error)
```

AddMoney calls the `add_money` slot.

- Parameters:
    - xuid : `string`
    - money : `int64`
- Return type: `(bool, error)`
- Slots: [`add_money`](../cpp/money.md#add_money)

### `Raw.ReduceMoney` {#Raw.ReduceMoney}

```go
func (RawAPI) ReduceMoney(xuid string, money int64) (bool, error)
```

ReduceMoney calls the `reduce_money` slot.

- Parameters:
    - xuid : `string`
    - money : `int64`
- Return type: `(bool, error)`
- Slots: [`reduce_money`](../cpp/money.md#reduce_money)

### `Raw.TransMoney` {#Raw.TransMoney}

```go
func (RawAPI) TransMoney(from string, to string, val int64, note string) (bool, error)
```

TransMoney calls the `trans_money` slot.

An empty from or to means created from or destroyed into nothing. What the payee receives is reduced by `pay_tax` (see above). from == to fails.

- Parameters:
    - from : `string`
    - to : `string`
    - val : `int64`
    - note : `string`
- Return type: `(bool, error)`
- Slots: [`trans_money`](../cpp/money.md#trans_money)

### `Raw.MoneyGetHist` {#Raw.MoneyGetHist}

```go
func (RawAPI) MoneyGetHist(xuid string, timediff int32) ([]string, error)
```

MoneyGetHist calls the `money_get_hist` slot.

- Parameters:
    - xuid : `string`
    - timediff : `int32`
- Return type: `([]string, error)`
- Slots: [`money_get_hist`](../cpp/money.md#money_get_hist)

### `Raw.MoneyClearHist` {#Raw.MoneyClearHist}

```go
func (RawAPI) MoneyClearHist(difftime int32) error
```

MoneyClearHist calls the `money_clear_hist` slot.

- Parameters:
    - difftime : `int32`
- Return type: `error`
- Slots: [`money_clear_hist`](../cpp/money.md#money_clear_hist)

### `Raw.MoneyRanking` {#Raw.MoneyRanking}

```go
func (RawAPI) MoneyRanking(num uint16) ([]string, error)
```

MoneyRanking calls the `money_ranking` slot.

- Parameters:
    - num : `uint16`
- Return type: `([]string, error)`
- Slots: [`money_ranking`](../cpp/money.md#money_ranking)

## Packets {#raw-packet}

### `Raw.SendPacket` {#Raw.SendPacket}

```go
func (RawAPI) SendPacket(sel PlayerSel, packetId int32, body []byte) (bool, error)
```

SendPacket calls the `send_packet` slot.

Raw per-connection packet send (additive, gated by `struct_size`) — the generic primitive `spawn_particle_for` derives from. `packet_id` is a MinecraftPacketIds value; `body`/`body_len` is the packet's wire-format body for the CURRENT game version. The bridge deserializes it into a real packet object (`MinecraftPackets::createPacket` + `Packet::read`) and delivers it to the resolved player's connection only. False if: player offline, unknown/unconstructible id, body fails to parse, or bytes are left over after parsing (wrong shape for this version). ESCAPE HATCH: the wire format is version-specific and is the caller's responsibility; prefer typed entries when one exists.

- Parameters:
    - sel : `PlayerSel`
    - packetId : `int32`
    - body : `[]byte`
- Return type: `(bool, error)`
- Slots: [`send_packet`](../cpp/packet.md#send_packet)

### `Raw.PacketHookUnregister` {#Raw.PacketHookUnregister}

```go
func (RawAPI) PacketHookUnregister(handleArg PacketHookHandle) (bool, error)
```

PacketHookUnregister calls the `packet_hook_unregister` slot.

Unregister. Safe to call from inside the callback.

- Parameters:
    - handleArg : `PacketHookHandle`
- Return type: `(bool, error)`
- Slots: [`packet_hook_unregister`](../cpp/packet.md#packet_hook_unregister)

### `Raw.PacketConnHookUnregister` {#Raw.PacketConnHookUnregister}

```go
func (RawAPI) PacketConnHookUnregister(handleArg PacketHookHandle) (bool, error)
```

PacketConnHookUnregister calls the `packet_conn_hook_unregister` slot.

Unregister. Safe to call from inside the callback.

- Parameters:
    - handleArg : `PacketHookHandle`
- Return type: `(bool, error)`
- Slots: [`packet_conn_hook_unregister`](../cpp/packet.md#packet_conn_hook_unregister)

## Simulated players {#raw-sim}

### `Raw.SimSpawn` {#Raw.SimSpawn}

```go
func (RawAPI) SimSpawn(name string, dimension int32, x float64, y float64, z float64) (bool, error)
```

SimSpawn calls the `sim_spawn` slot.

Simulated ("fake") players (additive, gated by `struct_size`). `sim_spawn` creates a real ServerPlayer with that name — every existing per-player entry (teleport, health, inventory, kick,…) works on it via the usual name selector. `sim_do` multiplexes the simulate\* verb family: the action vocabulary grows bridge-side without new table slots (verbs: despawn stop jump attack interact `use_item` drop respawn `move_to` `navigate_to` `look_at` `destroy_block` `destroy_look` `stop_destroy` `interact_block` sneak fly chat — args as SNBT, see docs). Gated on isSimulatedPlayer(): a real player can never be puppeted. False on unknown verb, malformed args, offline/non-sim target.

- Parameters:
    - name : `string`
    - dimension : `int32`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- Return type: `(bool, error)`
- Slots: [`sim_spawn`](../cpp/sim.md#sim_spawn)

### `Raw.SimDo` {#Raw.SimDo}

```go
func (RawAPI) SimDo(sel PlayerSel, action string, argsSnbt string) (bool, error)
```

SimDo calls the `sim_do` slot.

- Parameters:
    - sel : `PlayerSel`
    - action : `string`
    - argsSnbt : `string`
- Return type: `(bool, error)`
- Slots: [`sim_do`](../cpp/sim.md#sim_do)

### `Raw.SimIs` {#Raw.SimIs}

```go
func (RawAPI) SimIs(sel PlayerSel) (bool, error)
```

SimIs calls the `sim_is` slot.

True if the selector resolves to a live simulated player. Lets a mod re-validate a bot after a restart (the SimulatedPlayer persists in the world, but in-memory handles don't).

- Parameters:
    - sel : `PlayerSel`
- Return type: `(bool, error)`
- Slots: [`sim_is`](../cpp/sim.md#sim_is)

### `Raw.SimList` {#Raw.SimList}

```go
func (RawAPI) SimList() ([]string, error)
```

SimList calls the `sim_list` slot.

Enumerate the names of all live simulated players (sink receives each name). Rebuild a handle from a name to drive a bot that outlived the session that spawned it.

- Return type: `([]string, error)`
- Slots: [`sim_list`](../cpp/sim.md#sim_list)

## Client {#raw-client}

### `Raw.ClientGetLocalPlayer` {#Raw.ClientGetLocalPlayer}

```go
func (RawAPI) ClientGetLocalPlayer() ([]string, bool, error)
```

ClientGetLocalPlayer calls the `client_get_local_player` slot.

Local player's name via `ll::service::getClientInstance()`-&gt;getLocalPlayer(). sink receives the name, or the call returns false if not in a level.

- Return type: `([]string, bool, error)`
- Slots: [`client_get_local_player`](../cpp/client.md#client_get_local_player)

### `Raw.ClientIsInLevel` {#Raw.ClientIsInLevel}

```go
func (RawAPI) ClientIsInLevel() (bool, error)
```

ClientIsInLevel calls the `client_is_in_level` slot.

True when the client is inside a level (a world is loaded).

- Return type: `(bool, error)`
- Slots: [`client_is_in_level`](../cpp/client.md#client_is_in_level)

### `Raw.ClientGetScreenName` {#Raw.ClientGetScreenName}

```go
func (RawAPI) ClientGetScreenName() ([]string, bool, error)
```

ClientGetScreenName calls the `client_get_screen_name` slot.

Current screen / UI name (e.g. "`hud_screen`", "`pause_screen`"). NULL on every current host, client builds included: the engine exposes no stable accessor for it.

- Return type: `([]string, bool, error)`
- Slots: [`client_get_screen_name`](../cpp/client.md#client_get_screen_name)

### `Raw.ClientUnregisterKey` {#Raw.ClientUnregisterKey}

```go
func (RawAPI) ClientUnregisterKey(handleArg KeyHandle) (bool, error)
```

ClientUnregisterKey calls the `client_unregister_key` slot.

Unregister a key binding: this mod's handler stops firing and the handle is freed. The `ll::input::KeyHandle` itself is not destroyed, since the key registry keeps one key per name and offers no removal; registering the same name again reuses that key.

- Parameters:
    - handleArg : `KeyHandle`
- Return type: `(bool, error)`
- Slots: [`client_unregister_key`](../cpp/client.md#client_unregister_key)

### `Raw.ClientGetKeyCodes` {#Raw.ClientGetKeyCodes}

```go
func (RawAPI) ClientGetKeyCodes(handleArg KeyHandle) ([]string, bool, error)
```

ClientGetKeyCodes calls the `client_get_key_codes` slot.

Currently assigned key codes (may differ from defaults if remapped). sink receives a JSON-style array string "\[1,2,3\]".

- Parameters:
    - handleArg : `KeyHandle`
- Return type: `([]string, bool, error)`
- Slots: [`client_get_key_codes`](../cpp/client.md#client_get_key_codes)

## Custom dimensions {#raw-dimensions}

### `Raw.MdIsAvailable` {#Raw.MdIsAvailable}

```go
func (RawAPI) MdIsAvailable() (bool, error)
```

MdIsAvailable calls the `md_is_available` slot.

Whether this host can register custom dimensions. NULL when pier-dimensions was not built in. Filled in, it answers from a probe of the engine's dimension definition table, so it can be false on an engine whose layout this build does not match; the answer is cached after the first call that can make it.

- Return type: `(bool, error)`
- Slots: [`md_is_available`](../cpp/dimensions.md#md_is_available)

### `Raw.MdSetDimensionRule` {#Raw.MdSetDimensionRule}

```go
func (RawAPI) MdSetDimensionRule(dimension int32, rule int32, allow bool) error
```

MdSetDimensionRule calls the `md_set_dimension_rule` slot.

Per-dimension rules, consulted by the loader's own hooks.

- Parameters:
    - dimension : `int32`
    - rule : `int32`
    - allow : `bool`
- Return type: `error`
- Slots: [`md_set_dimension_rule`](../cpp/dimensions.md#md_set_dimension_rule)

### `Raw.MdGetDimensionRule` {#Raw.MdGetDimensionRule}

```go
func (RawAPI) MdGetDimensionRule(dimension int32, rule int32) (bool, bool, error)
```

MdGetDimensionRule calls the `md_get_dimension_rule` slot.

Read back a rule. `outAllow` is only written when the dimension has an explicit entry for that rule; returns false otherwise. A rule number this host does not know is answered false as well, and `md_set_dimension_rule` ignores one, so a binding newer than the host cannot tell "not set" from "not supported" here.

- Parameters:
    - dimension : `int32`
    - rule : `int32`
- Return type: `(bool, bool, error)`
- Slots: [`md_get_dimension_rule`](../cpp/dimensions.md#md_get_dimension_rule)

### `Raw.MdClearDimensionRules` {#Raw.MdClearDimensionRules}

```go
func (RawAPI) MdClearDimensionRules(dimension int32) error
```

MdClearDimensionRules calls the `md_clear_dimension_rules` slot.

Drop every rule for a dimension (used when a world is deleted).

- Parameters:
    - dimension : `int32`
- Return type: `error`
- Slots: [`md_clear_dimension_rules`](../cpp/dimensions.md#md_clear_dimension_rules)

### `Raw.MdGetDimensionId` {#Raw.MdGetDimensionId}

```go
func (RawAPI) MdGetDimensionId(name string) (int32, error)
```

MdGetDimensionId calls the `md_get_dimension_id` slot.

Resolve a dimension name to its id. Returns -1 if not found.

- Parameters:
    - name : `string`
- Return type: `(int32, error)`
- Slots: [`md_get_dimension_id`](../cpp/dimensions.md#md_get_dimension_id)

### `Raw.MdSetPlotMerges` {#Raw.MdSetPlotMerges}

```go
func (RawAPI) MdSetPlotMerges(dimension int32, entries []int32) error
```

MdSetPlotMerges calls the `md_set_plot_merges` slot.

Replace a dimension's merge markers wholesale. `entries` is `count` triples `(x, z, mask)`, i.e. `count * 3` int32s; `mask` is a bitset of 1=north, 2=east, 4=south, 8=west matching the plugin's `merged[]` indices. Only plots that actually carry a marker need to be sent.

- Parameters:
    - dimension : `int32`
    - entries : `[]int32`
- Return type: `error`
- Slots: [`md_set_plot_merges`](../cpp/dimensions.md#md_set_plot_merges)

### `Raw.MdListDimensions` {#Raw.MdListDimensions}

```go
func (RawAPI) MdListDimensions() ([]string, error)
```

MdListDimensions calls the `md_list_dimensions` slot.

List every registered custom dimension as a JSON array: \[{"name":"`plot_world`","dim":1000,"snbt":"{…}"}\].

- Return type: `([]string, error)`
- Slots: [`md_list_dimensions`](../cpp/dimensions.md#md_list_dimensions)

### `Raw.MdAddDimension` {#Raw.MdAddDimension}

```go
func (RawAPI) MdAddDimension(name string, specSnbt string) (int32, error)
```

MdAddDimension calls the `md_add_dimension` slot.

Add a custom dimension with a native terrain from one declarative spec.

- Parameters:
    - name : `string`
    - specSnbt : `string`
- Return type: `(int32, error)`
- Slots: [`md_add_dimension`](../cpp/dimensions.md#md_add_dimension)

### `Raw.MdAddDimensionPack` {#Raw.MdAddDimensionPack}

```go
func (RawAPI) MdAddDimensionPack(name string, configPath string, specSnbt string) (int32, error)
```

MdAddDimensionPack calls the `md_add_dimension_pack` slot.

Add a custom dimension whose terrain is a terrain pack: a directory with a config file and a binary built by tools/pier-pack. `config_path` names the config relative to the server root, forward slashes, no ".." and not absolute; the binary is named inside the config relative to it.

- Parameters:
    - name : `string`
    - configPath : `string`
    - specSnbt : `string`
- Return type: `(int32, error)`
- Slots: [`md_add_dimension_pack`](../cpp/dimensions.md#md_add_dimension_pack)

### `Raw.MdPackInspect` {#Raw.MdPackInspect}

```go
func (RawAPI) MdPackInspect(configPath string) ([]string, int32, error)
```

MdPackInspect calls the `md_pack_inspect` slot.

What a pack asks for, without registering anything: the sink receives one JSON document, the same shape `pier-pack inspect` prints: {"ok":true,"kind":"template","pack":"...","sha256":"...","name":"...", "biome":"...","height":{"min":-512,"max":320,"fixed":false}, "params":\[{"name":"`plot_size`","kind":"free","default":64,"min":4, "max":512,"step":1}, {"name":"`wall_style`","kind":"choice","default":0,"choices":\[0,1\]}, {"name":"`plot_depth`","kind":"derived"}, {"name":"size","kind":"fixed","value":256}\], "roles":\[{"name":"floor","default":"minecraft:`grass_block`"}\], "zones":\["interior","border","road"\], "constraints":\["..."\], "shapes":23, "picks":1, "voxels":\[{"size":\[5,4,3\],"palette":5}\], "confine":true} A mod builds its form from "params": free is a slider, choice a list, fixed a read-only value, derived is not shown. On a refusal the sink receives {"ok":false,"status":&lt;code&gt;,"problems":\["..."\]} and the code is returned. Returns 0 or a PIER\_PACK\_\* code. Server thread only.

- Parameters:
    - configPath : `string`
- Return type: `([]string, int32, error)`
- Slots: [`md_pack_inspect`](../cpp/dimensions.md#md_pack_inspect)

### `Raw.MdRetireDimension` {#Raw.MdRetireDimension}

```go
func (RawAPI) MdRetireDimension(name string) (bool, error)
```

MdRetireDimension calls the `md_retire_dimension` slot.

Retire a custom dimension: drop it from `dimension_config`.json, from the host's own tables and from the dimension factory, so nothing registers it on the next boot and it stops appearing in `md_list_dimensions`.

- Parameters:
    - name : `string`
- Return type: `(bool, error)`
- Slots: [`md_retire_dimension`](../cpp/dimensions.md#md_retire_dimension)

### `Raw.MdSetDimensionCells` {#Raw.MdSetDimensionCells}

```go
func (RawAPI) MdSetDimensionCells(dimId int32, cell int32, gap int32) (bool, error)
```

MdSetDimensionCells calls the `md_set_dimension_cells` slot.

Give a generated dimension the cell geometry its confinement rules use.

- Parameters:
    - dimId : `int32`
    - cell : `int32`
    - gap : `int32`
- Return type: `(bool, error)`
- Slots: [`md_set_dimension_cells`](../cpp/dimensions.md#md_set_dimension_cells)

## Cross-mod: bus, services and lanes {#raw-crossmod}

### `Raw.BusUnsubscribe` {#Raw.BusUnsubscribe}

```go
func (RawAPI) BusUnsubscribe(subId uint64) (bool, error)
```

BusUnsubscribe calls the `bus_unsubscribe` slot.

Drop one of this mod's subscriptions. Scoped to the caller — a mod cannot unsubscribe another mod. Returns true if one was removed. Safe to call from inside a callback (including one's own).

- Parameters:
    - subId : `uint64`
- Return type: `(bool, error)`
- Slots: [`bus_unsubscribe`](../cpp/crossmod.md#bus_unsubscribe)

### `Raw.BusPublish` {#Raw.BusPublish}

```go
func (RawAPI) BusPublish(topic string, payload string) (uint32, error)
```

BusPublish calls the `bus_publish` slot.

Deliver `payload` to every \*other\* mod subscribed to `topic`. Returns how many subscribers actually ran (0 is normal — nobody is listening). Return values from subscribers are ignored.

- Parameters:
    - topic : `string`
    - payload : `string`
- Return type: `(uint32, error)`
- Slots: [`bus_publish`](../cpp/crossmod.md#bus_publish)

### `Raw.BusPublishVetoable` {#Raw.BusPublishVetoable}

```go
func (RawAPI) BusPublishVetoable(topic string, payload string) (uint32, bool, error)
```

BusPublishVetoable calls the `bus_publish_vetoable` slot.

As above, but collects the veto bit: returns true when any subscriber returned true. Every subscriber still runs — no short-circuit — so observers see a consistent stream whether or not an earlier one refused. `out_delivered` may be NULL.

- Parameters:
    - topic : `string`
    - payload : `string`
- Return type: `(uint32, bool, error)`
- Slots: [`bus_publish_vetoable`](../cpp/crossmod.md#bus_publish_vetoable)

### `Raw.BusSubscriberCount` {#Raw.BusSubscriberCount}

```go
func (RawAPI) BusSubscriberCount(topic string) (uint32, error)
```

BusSubscriberCount calls the `bus_subscriber_count` slot.

How many subscribers a topic has right now, across all mods. Intended for skipping the cost of building a payload nobody will read.

- Parameters:
    - topic : `string`
- Return type: `(uint32, error)`
- Slots: [`bus_subscriber_count`](../cpp/crossmod.md#bus_subscriber_count)

### `Raw.ServiceUnregister` {#Raw.ServiceUnregister}

```go
func (RawAPI) ServiceUnregister(regId uint64) (bool, error)
```

ServiceUnregister calls the `service_unregister` slot.

Drop one of this mod's registrations. Scoped to the caller — a mod cannot unregister another mod's service.

- Parameters:
    - regId : `uint64`
- Return type: `(bool, error)`
- Slots: [`service_unregister`](../cpp/crossmod.md#service_unregister)

### `Raw.ServiceCall` {#Raw.ServiceCall}

```go
func (RawAPI) ServiceCall(name string, request string) ([]string, int32, error)
```

ServiceCall calls the `service_call` slot.

Call `name` with `request`; the provider's answer arrives through `reply`. Returns one of PIER\_SERVICE\_\*. A mod cannot call its own service (it has a direct function call, and self-calls are the least legible loop shape).

- Parameters:
    - name : `string`
    - request : `string`
- Return type: `([]string, int32, error)`
- Slots: [`service_call`](../cpp/crossmod.md#service_call)

### `Raw.ServiceList` {#Raw.ServiceList}

```go
func (RawAPI) ServiceList() ([]string, error)
```

ServiceList calls the `service_list` slot.

Every registered service as a JSON array of `{"name":…,"mod":…}`. For diagnostics and for a caller deciding whether to build a request nobody can answer.

- Return type: `([]string, error)`
- Slots: [`service_list`](../cpp/crossmod.md#service_list)

### `Raw.LaneUnpublish` {#Raw.LaneUnpublish}

```go
func (RawAPI) LaneUnpublish(pubId uint64) (bool, error)
```

LaneUnpublish calls the `lane_unpublish` slot.

Withdraw a lane owned by this mod. Calls release for every outstanding lease and clears the liveness flag, so a consumer's next check sees the lane gone instead of jumping through a dead pointer.

- Parameters:
    - pubId : `uint64`
- Return type: `(bool, error)`
- Slots: [`lane_unpublish`](../cpp/crossmod.md#lane_unpublish)

### `Raw.LaneRelease` {#Raw.LaneRelease}

```go
func (RawAPI) LaneRelease(lease uint64) (bool, error)
```

LaneRelease calls the `lane_release` slot.

Return a lease. Only a lease held by this mod. Returns false when the provider is already gone, since the loader has called release for it by then and calling again would be a double free.

- Parameters:
    - lease : `uint64`
- Return type: `(bool, error)`
- Slots: [`lane_release`](../cpp/crossmod.md#lane_release)

### `Raw.LaneList` {#Raw.LaneList}

```go
func (RawAPI) LaneList() ([]string, error)
```

LaneList calls the `lane_list` slot.

Every lane, as a JSON array: \[{"name":…,"mod":…,"fingerprint":"0x…","protocol":1,"leases":N,"alive":true}\]

- Return type: `([]string, error)`
- Slots: [`lane_list`](../cpp/crossmod.md#lane_list)

### `Raw.ServiceCaller` {#Raw.ServiceCaller}

```go
func (RawAPI) ServiceCaller() ([]string, error)
```

ServiceCaller calls the `service_caller` slot.

Who is calling the service callback that is running right now.

- Return type: `([]string, error)`
- Slots: [`service_caller`](../cpp/crossmod.md#service_caller)

## Registries {#raw-registry}

### `Raw.RegistryList` {#Raw.RegistryList}

```go
func (RawAPI) RegistryList(kind int32) ([]string, bool, error)
```

RegistryList calls the `registry_list` slot.

Lists one of the engine's registries through `sink`, one JSON object per entry, in no particular order. `kind` is a PIER\_REGISTRY\_\* value.

- Parameters:
    - kind : `int32`
- Return type: `([]string, bool, error)`
- Slots: [`registry_list`](../cpp/registry.md#registry_list)
