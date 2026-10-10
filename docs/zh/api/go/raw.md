# Go：Raw：每个槽位一个带类型的方法

## 变量 {#variables}

### `Raw` {#Raw}

```go
var Raw RawAPI
```

带类型的槽位层。这个包里有对应的门面函数时，优先用门面函数。

## `RawAPI` {#RawAPI}

```go
type RawAPI struct{}
type RawAPI struct{}
```

`RawAPI` 用 Go 的类型、经过关卡，取到每一个不带回调的槽位：宿主缺少的槽位返回 `*NotProvidedError`。结果的含义就是 abi.h 写的含义；这个包的其余部分在它之上构建解释过的接口。通过 `Raw` 使用它。

## 核心：入口、日志与任务 {#raw-core}

### `Raw.Log` {#Raw.Log}

```go
func (RawAPI) Log(level int32, msg string) error
```

调用 `log` 槽位。

通过模组自己的 LeviLamina 日志器写一条日志。`level`：-1=Off，0=Fatal，1=Error，2=Warn，3=Info，4=Debug，5=Trace（对应 `ll::io::LogLevel`）。线程安全。

- 参数：
    - level : `int32`
    - msg : `string`
- 返回值类型：`error`
- 对应槽位：[`log`](../cpp/core.md#log)

### `Raw.GamingStatus` {#Raw.GamingStatus}

```go
func (RawAPI) GamingStatus() (int32, error)
```

调用 `gaming_status` 槽位。

当前的运行状态：0=Default，1=Starting，2=Running，3=Stopping（对应 `ll::GamingStatus`）。线程安全。

- 返回值类型：`(int32, error)`
- 对应槽位：[`gaming_status`](../cpp/core.md#gaming_status)

### `Raw.ScheduleCancel` {#Raw.ScheduleCancel}

```go
func (RawAPI) ScheduleCancel(taskId uint64) (bool, error)
```

调用 `schedule_cancel` 槽位。

这个模组排的某个任务如果还没运行，就把它丢掉。真的丢掉了一个待执行的任务时返回 true。可以从任何线程调用，也可以在另一个任务里调用。取消同样会泄漏 `user`，原因和上面一样，所以短任务最好让它跑完。

- 参数：
    - taskId : `uint64`
- 返回值类型：`(bool, error)`
- 对应槽位：[`schedule_cancel`](../cpp/core.md#schedule_cancel)

### `Raw.SchedulePendingCount` {#Raw.SchedulePendingCount}

```go
func (RawAPI) SchedulePendingCount() (uint32, error)
```

调用 `schedule_pending_count` 槽位。

这个模组还有多少个待执行的任务。供模组在 `on_disable` / `on_unload` 里确认自己的工作已经清空，这是在清单里标记 `"reload_safe"` 的前提。

- 返回值类型：`(uint32, error)`
- 对应槽位：[`schedule_pending_count`](../cpp/core.md#schedule_pending_count)

## 事件 {#raw-events}

### `Raw.UnsubscribeEvent` {#Raw.UnsubscribeEvent}

```go
func (RawAPI) UnsubscribeEvent(listener ListenerHandle) (bool, error)
```

调用 `unsubscribe_event` 槽位。

移除之前由 `subscribe_event` 返回的监听器。只能在服务器线程调用。

- 参数：
    - listener : `ListenerHandle`
- 返回值类型：`(bool, error)`
- 对应槽位：[`unsubscribe_event`](../cpp/events.md#unsubscribe_event)

### `Raw.ListEvents` {#Raw.ListEvents}

```go
func (RawAPI) ListEvents() ([]string, error)
```

调用 `list_events` 槽位。

列出当前已注册的所有事件 id。只能在服务器线程调用。

- 返回值类型：`([]string, error)`
- 对应槽位：[`list_events`](../cpp/events.md#list_events)

## 命令 {#raw-commands}

### `Raw.RegisterCommandEnum` {#Raw.RegisterCommandEnum}

```go
func (RawAPI) RegisterCommandEnum(name string, valuesSnbt string) (bool, error)
```

调用 `register_command_enum` 槽位。

`values_snbt` 形如 `{values:[["name",1L],…]}`，交给 `tryRegisterRuntimeEnum`。

- 参数：
    - name : `string`
    - valuesSnbt : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`register_command_enum`](../cpp/commands.md#register_command_enum)

### `Raw.RegisterCommandSoftEnum` {#Raw.RegisterCommandSoftEnum}

```go
func (RawAPI) RegisterCommandSoftEnum(name string, valuesSnbt string) (bool, error)
```

调用 `register_command_soft_enum` 槽位。

`values_snbt` 形如 `{values:["a","b"]}`，交给 `tryRegisterSoftEnum`。

- 参数：
    - name : `string`
    - valuesSnbt : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`register_command_soft_enum`](../cpp/commands.md#register_command_soft_enum)

### `Raw.UpdateCommandSoftEnum` {#Raw.UpdateCommandSoftEnum}

```go
func (RawAPI) UpdateCommandSoftEnum(name string, op int32, valuesSnbt string) (bool, error)
```

调用 `update_command_soft_enum` 槽位。

`op`：0=设置，1=添加，2=移除。

- 参数：
    - name : `string`
    - op : `int32`
    - valuesSnbt : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`update_command_soft_enum`](../cpp/commands.md#update_command_soft_enum)

## 服务器、刻与系统信息 {#raw-server}

### `Raw.GetCurrentTick` {#Raw.GetCurrentTick}

```go
func (RawAPI) GetCurrentTick() (uint64, error)
```

调用 `get_current_tick` 槽位。

当前的服务器刻（`Level::getCurrentTick()` 里的 tickID）。世界还没就绪时返回 0。只能在服务器线程调用。

- 返回值类型：`(uint64, error)`
- 对应槽位：[`get_current_tick`](../cpp/server.md#get_current_tick)

### `Raw.GetTickDeltaTime` {#Raw.GetTickDeltaTime}

```go
func (RawAPI) GetTickDeltaTime() (float64, error)
```

调用 `get_tick_delta_time` 槽位。

上一帧实际经过的时间，单位秒（`mTickDeltaTime`；20 TPS 时是 0.05）。它包含服务器为维持 20 Hz 插入的休眠，所以它和计算一刻所花的时间是两回事，它的倒数也不能当刻率用：它只是帧率的一个带噪声的样本。要 TPS 和 MSPT，请用 `get_tps` 和 `get_mspt`。取不到时返回 -1.0。只能在服务器线程调用。

- 返回值类型：`(float64, error)`
- 对应槽位：[`get_tick_delta_time`](../cpp/server.md#get_tick_delta_time)

### `Raw.GetPlayerCount` {#Raw.GetPlayerCount}

```go
func (RawAPI) GetPlayerCount() (int32, error)
```

调用 `get_player_count` 槽位。

当前连接的玩家数（`Level::getActivePlayerCount()`）。只能在服务器线程调用。

- 返回值类型：`(int32, error)`
- 对应槽位：[`get_player_count`](../cpp/server.md#get_player_count)

### `Raw.GetSimPaused` {#Raw.GetSimPaused}

```go
func (RawAPI) GetSimPaused() (bool, error)
```

调用 `get_sim_paused` 槽位。

模拟当前是否暂停（`Level::getSimPaused()`）。只能在服务器线程调用。

- 返回值类型：`(bool, error)`
- 对应槽位：[`get_sim_paused`](../cpp/server.md#get_sim_paused)

### `Raw.SysInfoStr` {#Raw.SysInfoStr}

```go
func (RawAPI) SysInfoStr(prop int32) ([]string, bool, error)
```

调用 `sys_info_str` 槽位。

系统信息：线程安全（只是普通的操作系统调用）。

- 参数：
    - prop : `int32`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`sys_info_str`](../cpp/server.md#sys_info_str)

### `Raw.SysGetEnv` {#Raw.SysGetEnv}

```go
func (RawAPI) SysGetEnv(name string) ([]string, bool, error)
```

调用 `sys_get_env` 槽位。

- 参数：
    - name : `string`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`sys_get_env`](../cpp/server.md#sys_get_env)

### `Raw.SysSetEnv` {#Raw.SysSetEnv}

```go
func (RawAPI) SysSetEnv(name string, value string) (bool, error)
```

调用 `sys_set_env` 槽位。

- 参数：
    - name : `string`
    - value : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`sys_set_env`](../cpp/server.md#sys_set_env)

### `Raw.SysIsWine` {#Raw.SysIsWine}

```go
func (RawAPI) SysIsWine() (bool, error)
```

调用 `sys_is_wine` 槽位。

- 返回值类型：`(bool, error)`
- 对应槽位：[`sys_is_wine`](../cpp/server.md#sys_is_wine)

### `Raw.ServerInfoStr` {#Raw.ServerInfoStr}

```go
func (RawAPI) ServerInfoStr(prop int32) ([]string, bool, error)
```

调用 `server_info_str` 槽位。

- 参数：
    - prop : `int32`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`server_info_str`](../cpp/server.md#server_info_str)

### `Raw.TickFreeze` {#Raw.TickFreeze}

```go
func (RawAPI) TickFreeze(on bool) (bool, error)
```

调用 `tick_freeze` 槽位。

控制刻的推进（追加的槽位，受 `struct_size` 约束）。实现方式是 bridge 在 `Level::tick` 上挂一个钩子：第一次调用控制接口时才安装，装上以后一直留着，空闲时每帧多一次可以预测的分支。一直留着的原因是，控制调用可能来自正在刻**内部**执行的命令处理函数，在那里卸下钩子并不安全。只能在服务器线程调用。

冻结期间，生物、方块、红石和时间都停下；玩家仍然可以移动和聊天，因为移动由客户端决定，网络在关卡的刻之外运行。

- 参数：
    - on : `bool`
- 返回值类型：`(bool, error)`
- 对应槽位：[`tick_freeze`](../cpp/server.md#tick_freeze)

### `Raw.TickStep` {#Raw.TickStep}

```go
func (RawAPI) TickStep(n uint32) (bool, error)
```

调用 `tick_step` 槽位。

只在冻结时有效：额外排入正好 n 帧。没有冻结或 n == 0 时返回 false。

- 参数：
    - n : `uint32`
- 返回值类型：`(bool, error)`
- 对应槽位：[`tick_step`](../cpp/server.md#tick_step)

### `Raw.TickWarp` {#Raw.TickWarp}

```go
func (RawAPI) TickWarp(factor float64) (bool, error)
```

调用 `tick_warp` 槽位。

0 < factor <= 100。小数表示慢动作（用累加器实现），1.0 恢复正常。

- 参数：
    - factor : `float64`
- 返回值类型：`(bool, error)`
- 对应槽位：[`tick_warp`](../cpp/server.md#tick_warp)

### `Raw.ProfileBegin` {#Raw.ProfileBegin}

```go
func (RawAPI) ProfileBegin(ticks uint32) (bool, error)
```

调用 `profile_begin` 槽位。

开启一个长度为 `ticks` 个关卡刻（1 到 12000）的采样窗口。为 0、太大，或者已经在采样时返回 false。

- 参数：
    - ticks : `uint32`
- 返回值类型：`(bool, error)`
- 对应槽位：[`profile_begin`](../cpp/server.md#profile_begin)

### `Raw.ProfileTake` {#Raw.ProfileTake}

```go
func (RawAPI) ProfileTake() ([]string, bool, error)
```

调用 `profile_take` 槽位。

取回已经完成的报告。还在采样或者没有开启窗口时返回 false；每个窗口正好返回一次 true，并通过输出回调给出一份 SNBT 报告：`{ticks:N, buckets:{level_tick:{us,calls}, dimension_tick:{…}, redstone:{…}, chunk_blocks:{…}, block_entities:{…}}}`。各个桶的时间是**包含**关系（子系统之间有嵌套），请并排对照着看，不要相加。

- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`profile_take`](../cpp/server.md#profile_take)

### `Raw.GetTps` {#Raw.GetTps}

```go
func (RawAPI) GetTps(windowSeconds int32) (float64, error)
```

调用 `get_tps` 槽位。

最近 `window_seconds` 秒（1 到 60，超出会被截断）的实际时间里，每秒运行的刻数：真正运行过的 `Level::tick` 次数除以经过的时间。刻加速时（读数高于 20）、刻冻结时（读数为 0）和卡顿时（读数低于 20）都是准确的。第一帧还没采样时返回 -1.0。只能在服务器线程调用。

- 参数：
    - windowSeconds : `int32`
- 返回值类型：`(float64, error)`
- 对应槽位：[`get_tps`](../cpp/server.md#get_tps)

### `Raw.GetMspt` {#Raw.GetMspt}

```go
func (RawAPI) GetMspt(windowSeconds int32) (float64, error)
```

调用 `get_mspt` 槽位。

最近 `window_seconds` 秒（1 到 60，超出范围会被截断）里，每一刻在 `Level::tick` 内花费的毫秒数的平均值。这是服务器计算所用的时间，不含帧与帧之间空闲的休眠；健康的服务器读数是几毫秒，只有满负荷时才接近 50。窗口内一刻都没有运行时返回 -1.0。只能在服务器线程调用。

- 参数：
    - windowSeconds : `int32`
- 返回值类型：`(float64, error)`
- 对应槽位：[`get_mspt`](../cpp/server.md#get_mspt)

## 世界 {#raw-world}

### `Raw.SpawnParticle` {#Raw.SpawnParticle}

```go
func (RawAPI) SpawnParticle(dimension int32, effectName string, x float64, y float64, z float64) (bool, error)
```

调用 `spawn_particle` 槽位。

在一个世界坐标上生成粒子效果，可以用来逐条边地勾出选区的轮廓。只能在服务器线程调用。世界或维度还没就绪时返回 false。

- `dimension`：0 为主世界，1 为下界，2 为末地。
- `effect_name`：例如 `"minecraft:basic_flame_particle"` 或 `"minecraft:redstone_wire_dust_particle"`。

- 参数：
    - dimension : `int32`
    - effectName : `string`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- 返回值类型：`(bool, error)`
- 对应槽位：[`spawn_particle`](../cpp/world.md#spawn_particle)

### `Raw.SetBlock` {#Raw.SetBlock}

```go
func (RawAPI) SetBlock(dim int32, x int32, y int32, z int32, blockSpec string) (bool, error)
```

调用 `set_block` 槽位。

原生放置方块（`BlockSource::setBlock`，默认的更新标志）。`block_spec` 可以是 `"minecraft:stone"` 或 `"stone"`（使用默认状态），也可以是完整的 `{name,states,...}` SNBT。名字认不出来时调用失败，不会放一个占位方块。

- 参数：
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
    - blockSpec : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`set_block`](../cpp/world.md#set_block)

### `Raw.GetTime` {#Raw.GetTime}

```go
func (RawAPI) GetTime() (int64, bool, error)
```

调用 `get_time` 槽位。

世界时间（`Level::getTime`）。

- 返回值类型：`(int64, bool, error)`
- 对应槽位：[`get_time`](../cpp/world.md#get_time)

### `Raw.SetTime` {#Raw.SetTime}

```go
func (RawAPI) SetTime(t int64) (bool, error)
```

调用 `set_time` 槽位。

原生设置世界时间（`Level::setTime`）。

- 参数：
    - t : `int64`
- 返回值类型：`(bool, error)`
- 对应槽位：[`set_time`](../cpp/world.md#set_time)

### `Raw.SetWeather` {#Raw.SetWeather}

```go
func (RawAPI) SetWeather(weather int32) (bool, error)
```

调用 `set_weather` 槽位。

0=晴，1=雨，2=雷雨，原生调用（`Level::updateWeather`）。

- 参数：
    - weather : `int32`
- 返回值类型：`(bool, error)`
- 对应槽位：[`set_weather`](../cpp/world.md#set_weather)

### `Raw.Explode` {#Raw.Explode}

```go
func (RawAPI) Explode(dim int32, x float64, y float64, z float64, radius float32, maxResistance float32, source ActorID, fire bool, breaksBlocks bool, allowUnderwater bool) (bool, error)
```

调用 `explode` 槽位。

调用 `Level::explode`。`source` 可以为 0，表示没有来源实体。

- 参数：
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
- 返回值类型：`(bool, error)`
- 对应槽位：[`explode`](../cpp/world.md#explode)

### `Raw.GetDifficulty` {#Raw.GetDifficulty}

```go
func (RawAPI) GetDifficulty() (int32, bool, error)
```

调用 `get_difficulty` 槽位。

服务器和世界级别的设置。

- 返回值类型：`(int32, bool, error)`
- 对应槽位：[`get_difficulty`](../cpp/world.md#get_difficulty)

### `Raw.SetDifficulty` {#Raw.SetDifficulty}

```go
func (RawAPI) SetDifficulty(d int32) (bool, error)
```

调用 `set_difficulty` 槽位。

- 参数：
    - d : `int32`
- 返回值类型：`(bool, error)`
- 对应槽位：[`set_difficulty`](../cpp/world.md#set_difficulty)

### `Raw.GetSeed` {#Raw.GetSeed}

```go
func (RawAPI) GetSeed() (int64, bool, error)
```

调用 `get_seed` 槽位。

- 返回值类型：`(int64, bool, error)`
- 对应槽位：[`get_seed`](../cpp/world.md#get_seed)

### `Raw.GameRuleGet` {#Raw.GameRuleGet}

```go
func (RawAPI) GameRuleGet(name string) ([]string, bool, error)
```

调用 `game_rule_get` 槽位。

输出回调收到 SNBT `{type:"bool"|"int"|"float", value:…}`；规则不存在时返回 false。

- 参数：
    - name : `string`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`game_rule_get`](../cpp/world.md#game_rule_get)

### `Raw.GameRuleSet` {#Raw.GameRuleSet}

```go
func (RawAPI) GameRuleSet(name string, value string) (bool, error)
```

调用 `game_rule_set` 槽位。

- 参数：
    - name : `string`
    - value : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`game_rule_set`](../cpp/world.md#game_rule_set)

### `Raw.SpawnParticleFor` {#Raw.SpawnParticleFor}

```go
func (RawAPI) SpawnParticleFor(sel PlayerSel, dimension int32, effectName string, x float64, y float64, z float64) (bool, error)
```

调用 `spawn_particle_for` 槽位。

只给一名玩家发送粒子包（追加的槽位，受 `struct_size` 约束）。`SpawnParticleEffectPacket` **只**发给解析出来的那名玩家（`Player::sendNetworkPacket`）；`Level::spawnParticleEffect` 会向整个维度广播，这里其他客户端收不到这个包。`dimension` 是包里带的原版维度 id，传坐标所在的维度，通常就是这名玩家所在的维度，因为客户端不渲染别的维度的粒子。玩家不在线或解析不到时返回 false。

- 参数：
    - sel : `PlayerSel`
    - dimension : `int32`
    - effectName : `string`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- 返回值类型：`(bool, error)`
- 对应槽位：[`spawn_particle_for`](../cpp/world.md#spawn_particle_for)

### `Raw.Villages` {#Raw.Villages}

```go
func (RawAPI) Villages(dimension int32) ([]string, error)
```

调用 `villages` 槽位。

列出一个维度里的村庄。每一项是 `{uuid, center:[x,y,z], bounds:{min,max}, poi_count}`。

- 参数：
    - dimension : `int32`
- 返回值类型：`([]string, error)`
- 对应槽位：[`villages`](../cpp/world.md#villages)

### `Raw.StructuresNear` {#Raw.StructuresNear}

```go
func (RawAPI) StructuresNear(dimension int32, x int32, y int32, z int32, radius int32) ([]string, error)
```

调用 `structures_near` 槽位。

以 (x,y,z) 为中心的一个半径内，所在区块与之相交的硬编码生成区：下界要塞、女巫小屋、海底神殿、掠夺者前哨站。每一项是 `{type, bounds:{min,max}}`。只检查**已加载**的区块，这个只读查询从不强制加载区块。

- 参数：
    - dimension : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
    - radius : `int32`
- 返回值类型：`([]string, error)`
- 对应槽位：[`structures_near`](../cpp/world.md#structures_near)

### `Raw.LevelGetBiome` {#Raw.LevelGetBiome}

```go
func (RawAPI) LevelGetBiome(dim int32, x int32, y int32, z int32) ([]string, bool, error)
```

调用 `level_get_biome` 槽位。

关卡：生物群系、出生点、保存、天气、寻路、睡眠（专用函数）

- 参数：
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`level_get_biome`](../cpp/world.md#level_get_biome)

### `Raw.LevelGetDefaultSpawn` {#Raw.LevelGetDefaultSpawn}

```go
func (RawAPI) LevelGetDefaultSpawn() (int32, int32, int32, bool, error)
```

调用 `level_get_default_spawn` 槽位。

- 返回值类型：`(int32, int32, int32, bool, error)`
- 对应槽位：[`level_get_default_spawn`](../cpp/world.md#level_get_default_spawn)

### `Raw.LevelSetDefaultSpawn` {#Raw.LevelSetDefaultSpawn}

```go
func (RawAPI) LevelSetDefaultSpawn(x int32, y int32, z int32) (bool, error)
```

调用 `level_set_default_spawn` 槽位。

- 参数：
    - x : `int32`
    - y : `int32`
    - z : `int32`
- 返回值类型：`(bool, error)`
- 对应槽位：[`level_set_default_spawn`](../cpp/world.md#level_set_default_spawn)

### `Raw.LevelSave` {#Raw.LevelSave}

```go
func (RawAPI) LevelSave() (bool, error)
```

调用 `level_save` 槽位。

- 返回值类型：`(bool, error)`
- 对应槽位：[`level_save`](../cpp/world.md#level_save)

### `Raw.LevelGetSleepStatus` {#Raw.LevelGetSleepStatus}

```go
func (RawAPI) LevelGetSleepStatus() ([]string, bool, error)
```

调用 `level_get_sleep_status` 槽位。

SNBT `{sleeping, total_players, active_sleeping}`

- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`level_get_sleep_status`](../cpp/world.md#level_get_sleep_status)

### `Raw.LevelUpdateWeather` {#Raw.LevelUpdateWeather}

```go
func (RawAPI) LevelUpdateWeather(rainLevel float32, rainTime int32, lightningLevel float32, lightningTime int32) (bool, error)
```

调用 `level_update_weather` 槽位。

- 参数：
    - rainLevel : `float32`
    - rainTime : `int32`
    - lightningLevel : `float32`
    - lightningTime : `int32`
- 返回值类型：`(bool, error)`
- 对应槽位：[`level_update_weather`](../cpp/world.md#level_update_weather)

### `Raw.LevelFindPath` {#Raw.LevelFindPath}

```go
func (RawAPI) LevelFindPath(id ActorID, x int32, y int32, z int32) ([]string, bool, error)
```

调用 `level_find_path` 槽位。

SNBT `{nodes:[{x,y,z},…], reached:1b/0b}`。当前所有宿主上这个槽位都是 NULL：寻路还没有实现。

- 参数：
    - id : `ActorID`
    - x : `int32`
    - y : `int32`
    - z : `int32`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`level_find_path`](../cpp/world.md#level_find_path)

### `Raw.LevelDeleteChunkKeys` {#Raw.LevelDeleteChunkKeys}

```go
func (RawAPI) LevelDeleteChunkKeys(dim int32, chunkX int32, chunkZ int32) (int32, error)
```

调用 `level_delete_chunk_keys` 槽位。

删除属于一个区块的所有存档键，下次加载时引擎会用生成器重新生成这个区块。

- 参数：
    - dim : `int32`
    - chunkX : `int32`
    - chunkZ : `int32`
- 返回值类型：`(int32, error)`
- 对应槽位：[`level_delete_chunk_keys`](../cpp/world.md#level_delete_chunk_keys)

### `Raw.LevelChunksLoaded` {#Raw.LevelChunksLoaded}

```go
func (RawAPI) LevelChunksLoaded(dim int32, minX int32, minZ int32, maxX int32, maxZ int32) (int32, error)
```

调用 `level_chunks_loaded` 槽位。

覆盖 [min..max] 的区块当前是否都已加载到内存里。

- 参数：
    - dim : `int32`
    - minX : `int32`
    - minZ : `int32`
    - maxX : `int32`
    - maxZ : `int32`
- 返回值类型：`(int32, error)`
- 对应槽位：[`level_chunks_loaded`](../cpp/world.md#level_chunks_loaded)

### `Raw.LevelChunkKeys` {#Raw.LevelChunkKeys}

```go
func (RawAPI) LevelChunkKeys(dim int32, chunkX int32, chunkZ int32) ([]string, int32, error)
```

调用 `level_chunk_keys` 槽位。

列出属于一个区块的所有存档键，每个键调用一次回调。

- 参数：
    - dim : `int32`
    - chunkX : `int32`
    - chunkZ : `int32`
- 返回值类型：`([]string, int32, error)`
- 对应槽位：[`level_chunk_keys`](../cpp/world.md#level_chunk_keys)

### `Raw.LevelDeleteKey` {#Raw.LevelDeleteKey}

```go
func (RawAPI) LevelDeleteKey(key string) (bool, error)
```

调用 `level_delete_key` 槽位。

原样删除一个区块类的键。

- 参数：
    - key : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`level_delete_key`](../cpp/world.md#level_delete_key)

### `Raw.LevelSetBiome` {#Raw.LevelSetBiome}

```go
func (RawAPI) LevelSetBiome(dim int32, minX int32, minZ int32, maxX int32, maxZ int32, biome string) (int32, error)
```

调用 `level_set_biome` 槽位。

设置一片区域的生物群系。

- 参数：
    - dim : `int32`
    - minX : `int32`
    - minZ : `int32`
    - maxX : `int32`
    - maxZ : `int32`
    - biome : `string`
- 返回值类型：`(int32, error)`
- 对应槽位：[`level_set_biome`](../cpp/world.md#level_set_biome)

### `Raw.GetExtraBlock` {#Raw.GetExtraBlock}

```go
func (RawAPI) GetExtraBlock(dim int32, x int32, y int32, z int32) ([]string, bool, error)
```

调用 `get_extra_block` 槽位。

读取液体层。输出回调收到一个方块名，例如 `"minecraft:water"`；这一层为空时给出空气。

- 参数：
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`get_extra_block`](../cpp/world.md#get_extra_block)

### `Raw.SetExtraBlock` {#Raw.SetExtraBlock}

```go
func (RawAPI) SetExtraBlock(dim int32, x int32, y int32, z int32, blockSpec string, updateFlags int32) (bool, error)
```

调用 `set_extra_block` 槽位。

写入液体层。`block_spec` 可以是单纯的方块名，也可以是完整的 SNBT；写入 `"minecraft:air"` 就是清空。`update_flags` 的含义和 `edit_set_block_nbt` 一样：第 1 位通知相邻方块，第 2 位同步给客户端。

- 参数：
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
    - blockSpec : `string`
    - updateFlags : `int32`
- 返回值类型：`(bool, error)`
- 对应槽位：[`set_extra_block`](../cpp/world.md#set_extra_block)

## 批量编辑世界 {#raw-edit}

### `Raw.EditSetBlockNbt` {#Raw.EditSetBlockNbt}

```go
func (RawAPI) EditSetBlockNbt(dim int32, x int32, y int32, z int32, snbt string, updateFlags int32) (bool, error)
```

调用 `edit_set_block_nbt` 槽位。

用序列化的 NBT 写入一个方块，格式是 `{name,states,version}`，也就是 `get_block` 给出的形状。

- 参数：
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
    - snbt : `string`
    - updateFlags : `int32`
- 返回值类型：`(bool, error)`
- 对应槽位：[`edit_set_block_nbt`](../cpp/edit.md#edit_set_block_nbt)

### `Raw.EditSetBlockStates` {#Raw.EditSetBlockStates}

```go
func (RawAPI) EditSetBlockStates(dim int32, x int32, y int32, z int32, name string, statesSnbt string, updateFlags int32) (bool, error)
```

调用 `edit_set_block_states` 槽位。

用方块名加上可选的部分状态写入一个方块。`states_snbt` 为空表示全部用默认状态；版本号取自加载器一侧的默认状态，调用方不能自己提供。

- 参数：
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
    - name : `string`
    - statesSnbt : `string`
    - updateFlags : `int32`
- 返回值类型：`(bool, error)`
- 对应槽位：[`edit_set_block_states`](../cpp/edit.md#edit_set_block_states)

### `Raw.EditSetBlockEntity` {#Raw.EditSetBlockEntity}

```go
func (RawAPI) EditSetBlockEntity(dim int32, x int32, y int32, z int32, snbt string) (bool, error)
```

调用 `edit_set_block_entity` 槽位。

把一个方块实体的 NBT 写回去（`BlockActor::load`）。这一格里必须已经是对应的方块。

- 参数：
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
    - snbt : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`edit_set_block_entity`](../cpp/edit.md#edit_set_block_entity)

### `Raw.EditSpawnEntityNbt` {#Raw.EditSpawnEntityNbt}

```go
func (RawAPI) EditSpawnEntityNbt(dim int32, snbt string, usePos bool, x float64, y float64, z float64) (ActorID, bool, error)
```

调用 `edit_spawn_entity_nbt` 槽位。

用完整的 NBT 生成一个实体，是 `actor_snapshot` 的反向操作。`use_pos` 为 true 时，(x,y,z) 覆盖 `Pos` 标签；引擎会重新分配 UniqueID，并通过 `out` 返回。

当前所有宿主上这个槽位都是 NULL：给读入的实体分配新 UniqueID 的那个引擎函数被内联掉了，而沿用快照里原来的 id 会让引擎把两个实体当成同一个。

- 参数：
    - dim : `int32`
    - snbt : `string`
    - usePos : `bool`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- 返回值类型：`(ActorID, bool, error)`
- 对应槽位：[`edit_spawn_entity_nbt`](../cpp/edit.md#edit_spawn_entity_nbt)

### `Raw.EditTraceRay` {#Raw.EditTraceRay}

```go
func (RawAPI) EditTraceRay(id ActorID, maxDist float32, includeActors bool, includeBlocks bool) ([]string, bool, error)
```

调用 `edit_trace_ray` 槽位。

射线检测，给出命中的**方块**坐标和命中的面：`{type, block:[x,y,z], facing, pos:[x,y,z], entity}`。

- 参数：
    - id : `ActorID`
    - maxDist : `float32`
    - includeActors : `bool`
    - includeBlocks : `bool`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`edit_trace_ray`](../cpp/edit.md#edit_trace_ray)

### `Raw.EditFillRegion` {#Raw.EditFillRegion}

```go
func (RawAPI) EditFillRegion(dimension int32, x1 int32, y1 int32, z1 int32, x2 int32, y2 int32, z2 int32, blockSpec string, updateFlags int32) (int64, error)
```

调用 `edit_fill_region` 槽位。

用同一种方块填满一个长方体。`block_spec` 可以是 `"minecraft:stone"` 这样的方块名，也可以是完整的 SNBT，整个长方体只解析一次。`update_flags` 的含义和 `edit_set_block_nbt` 一样：第 1 位通知相邻方块，第 2 位同步给客户端；传 0 最快，客户端要等下一次发送区块时才看到变化。返回写入的格子数；维度没有准备好、`block_spec` 解析不出来，或者长方体超过 2^24 个格子时返回 -1。只能在服务器线程调用。

- 参数：
    - dimension : `int32`
    - x1 : `int32`
    - y1 : `int32`
    - z1 : `int32`
    - x2 : `int32`
    - y2 : `int32`
    - z2 : `int32`
    - blockSpec : `string`
    - updateFlags : `int32`
- 返回值类型：`(int64, error)`
- 对应槽位：[`edit_fill_region`](../cpp/edit.md#edit_fill_region)

## 玩家 {#raw-player}

### `Raw.GetPlayerPosition` {#Raw.GetPlayerPosition}

```go
func (RawAPI) GetPlayerPosition(name string) (PlayerPos, error)
```

调用 `get_player_position` 槽位。

按名字查找一名已连接玩家的脚下位置和所在维度。用于根据玩家站的位置选取选区的角。只能在服务器线程调用。

- 参数：
    - name : `string`
- 返回值类型：`(PlayerPos, error)`
- 对应槽位：[`get_player_position`](../cpp/player.md#get_player_position)

### `Raw.ListPlayers` {#Raw.ListPlayers}

```go
func (RawAPI) ListPlayers() ([]string, error)
```

调用 `list_players` 槽位。

每个在线玩家一段 SNBT：`{name,xuid,uuid,dim,x,y,z}`。

- 返回值类型：`([]string, error)`
- 对应槽位：[`list_players`](../cpp/player.md#list_players)

### `Raw.PlayerResolve` {#Raw.PlayerResolve}

```go
func (RawAPI) PlayerResolve(sel PlayerSel) (ActorID, bool, error)
```

调用 `player_resolve` 槽位。

把玩家选择器解析成这名玩家的 `ActorUniqueID`，由此接到 `actor_*` 这组接口上。

- 参数：
    - sel : `PlayerSel`
- 返回值类型：`(ActorID, bool, error)`
- 对应槽位：[`player_resolve`](../cpp/player.md#player_resolve)

### `Raw.PlayerSendMessage` {#Raw.PlayerSendMessage}

```go
func (RawAPI) PlayerSendMessage(sel PlayerSel, msg string) (bool, error)
```

调用 `player_send_message` 槽位。

- 参数：
    - sel : `PlayerSel`
    - msg : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`player_send_message`](../cpp/player.md#player_send_message)

### `Raw.PlayerDisconnect` {#Raw.PlayerDisconnect}

```go
func (RawAPI) PlayerDisconnect(sel PlayerSel, reason string) (bool, error)
```

调用 `player_disconnect` 槽位。

- 参数：
    - sel : `PlayerSel`
    - reason : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`player_disconnect`](../cpp/player.md#player_disconnect)

### `Raw.BroadcastMessage` {#Raw.BroadcastMessage}

```go
func (RawAPI) BroadcastMessage(msg string) error
```

调用 `broadcast_message` 槽位。

对每个在线玩家调用 `sendMessage`。

- 参数：
    - msg : `string`
- 返回值类型：`error`
- 对应槽位：[`broadcast_message`](../cpp/player.md#broadcast_message)

### `Raw.PlayerSetGamemode` {#Raw.PlayerSetGamemode}

```go
func (RawAPI) PlayerSetGamemode(sel PlayerSel, mode int32) (bool, error)
```

调用 `player_set_gamemode` 槽位。

0=生存，1=创造，2=冒险，6=旁观，原生调用（`Player::setPlayerGameType`）。

- 参数：
    - sel : `PlayerSel`
    - mode : `int32`
- 返回值类型：`(bool, error)`
- 对应槽位：[`player_set_gamemode`](../cpp/player.md#player_set_gamemode)

### `Raw.PlayerTeleport` {#Raw.PlayerTeleport}

```go
func (RawAPI) PlayerTeleport(sel PlayerSel, dim int32, x float64, y float64, z float64) (bool, error)
```

调用 `player_teleport` 槽位。

原生传送（`Actor::teleport`）。可以传送到自定义维度（id >= 3），但维度桥接必须产出 id 一致的引擎实例；对不上时调用失败，不会把玩家送进一个对不上的维度。

- 参数：
    - sel : `PlayerSel`
    - dim : `int32`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- 返回值类型：`(bool, error)`
- 对应槽位：[`player_teleport`](../cpp/player.md#player_teleport)

### `Raw.PlayerGetNum` {#Raw.PlayerGetNum}

```go
func (RawAPI) PlayerGetNum(sel PlayerSel, prop int32) (float64, bool, error)
```

调用 `player_get_num` 槽位。

四个 `*_get_num` 槽位遵守同一个约定。返回值表示宿主有没有答案，`*out` 是答案；返回 false 时 `*out` 不会被改动。

- 参数：
    - sel : `PlayerSel`
    - prop : `int32`
- 返回值类型：`(float64, bool, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Raw.PlayerGetStr` {#Raw.PlayerGetStr}

```go
func (RawAPI) PlayerGetStr(sel PlayerSel, prop int32) ([]string, bool, error)
```

调用 `player_get_str` 槽位。

- 参数：
    - sel : `PlayerSel`
    - prop : `int32`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Raw.PlayerSetNum` {#Raw.PlayerSetNum}

```go
func (RawAPI) PlayerSetNum(sel PlayerSel, prop int32, v float64) (bool, error)
```

调用 `player_set_num` 槽位。

- 参数：
    - sel : `PlayerSel`
    - prop : `int32`
    - v : `float64`
- 返回值类型：`(bool, error)`
- 对应槽位：[`player_set_num`](../cpp/player.md#player_set_num)

### `Raw.PlayerAction` {#Raw.PlayerAction}

```go
func (RawAPI) PlayerAction(sel PlayerSel, action int32, sarg string, a float64, b float64, c float64) ([]string, bool, error)
```

调用 `player_action` 槽位。

- 参数：
    - sel : `PlayerSel`
    - action : `int32`
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Raw.PlayerSendMessageTyped` {#Raw.PlayerSendMessageTyped}

```go
func (RawAPI) PlayerSendMessageTyped(sel PlayerSel, msg string, typeArg int32) (bool, error)
```

调用 `player_send_message_typed` 槽位。

给一名玩家发送指定 `TextPacketType` 的消息（追加的槽位，受 `struct_size` 约束）。`type` 是 `TextPacketType` 的值：0 Raw · 1 Chat · 2 Translate · 3 Popup · 4 JukeboxPopup · 5 Tip · 6 SystemMessage · 7 Whisper · 8 Announcement · 9 TextObjectWhisper · 10 TextObject · 11 TextObjectAnnouncement。超出范围时按 Raw 处理。消息体只有一个字符串（和 LSE 的 tell 一样）：需要作者或参数的类型（Chat、Whisper、Translate）收到的是纯文本。普通的 `player_send_message` 仍然是发 Raw 或 Chat 消息的便捷路径。

- 参数：
    - sel : `PlayerSel`
    - msg : `string`
    - typeArg : `int32`
- 返回值类型：`(bool, error)`
- 对应槽位：[`player_send_message_typed`](../cpp/player.md#player_send_message_typed)

### `Raw.PlayerGetCarriedItem` {#Raw.PlayerGetCarriedItem}

```go
func (RawAPI) PlayerGetCarriedItem(sel PlayerSel) ([]string, bool, error)
```

调用 `player_get_carried_item` 槽位。

玩家：装备、冷却、网络（专用函数）

- 参数：
    - sel : `PlayerSel`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`player_get_carried_item`](../cpp/player.md#player_get_carried_item)

### `Raw.PlayerGetItem` {#Raw.PlayerGetItem}

```go
func (RawAPI) PlayerGetItem(sel PlayerSel, slot int32) ([]string, bool, error)
```

调用 `player_get_item` 槽位。

- 参数：
    - sel : `PlayerSel`
    - slot : `int32`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`player_get_item`](../cpp/player.md#player_get_item)

### `Raw.PlayerSetItem` {#Raw.PlayerSetItem}

```go
func (RawAPI) PlayerSetItem(sel PlayerSel, slot int32, itemSnbt string) (bool, error)
```

调用 `player_set_item` 槽位。

- 参数：
    - sel : `PlayerSel`
    - slot : `int32`
    - itemSnbt : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`player_set_item`](../cpp/player.md#player_set_item)

### `Raw.PlayerGetEquipment` {#Raw.PlayerGetEquipment}

```go
func (RawAPI) PlayerGetEquipment(sel PlayerSel) ([]string, bool, error)
```

调用 `player_get_equipment` 槽位。

全部装备，以 SNBT `[{slot, item_snbt},…]` 给出。`slot`：0 为主手，1 为副手，2 到 5 为盔甲。

- 参数：
    - sel : `PlayerSel`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`player_get_equipment`](../cpp/player.md#player_get_equipment)

### `Raw.PlayerGetCooldown` {#Raw.PlayerGetCooldown}

```go
func (RawAPI) PlayerGetCooldown(sel PlayerSel, itemName string) (int32, error)
```

调用 `player_get_cooldown` 槽位。

一个物品的冷却还剩多少刻（不在冷却中或者玩家不在线时为 -1）。

- 参数：
    - sel : `PlayerSel`
    - itemName : `string`
- 返回值类型：`(int32, error)`
- 对应槽位：[`player_get_cooldown`](../cpp/player.md#player_get_cooldown)

### `Raw.PlayerStartCooldown` {#Raw.PlayerStartCooldown}

```go
func (RawAPI) PlayerStartCooldown(sel PlayerSel, itemName string, ticks int32) (bool, error)
```

调用 `player_start_cooldown` 槽位。

- 参数：
    - sel : `PlayerSel`
    - itemName : `string`
    - ticks : `int32`
- 返回值类型：`(bool, error)`
- 对应槽位：[`player_start_cooldown`](../cpp/player.md#player_start_cooldown)

### `Raw.PlayerGetNetworkStatus` {#Raw.PlayerGetNetworkStatus}

```go
func (RawAPI) PlayerGetNetworkStatus(sel PlayerSel) ([]string, bool, error)
```

调用 `player_get_network_status` 槽位。

- 参数：
    - sel : `PlayerSel`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`player_get_network_status`](../cpp/player.md#player_get_network_status)

### `Raw.PlayerSendTitle` {#Raw.PlayerSendTitle}

```go
func (RawAPI) PlayerSendTitle(sel PlayerSel, typeArg int32, text string, fadeInTicks int32, stayTicks int32, fadeOutTicks int32) (bool, error)
```

调用 `player_send_title` 槽位。

标题

`PACT_SET_TITLE`（`player_action` 的第 6 号操作）是通过执行控制台命令 `title "<name>" title <text>` 送到客户端的。这样做有三个问题，三个都会真的发生：

- 文本不加引号就拼进命令行，名叫 `He said "hi"` 的地皮会把命令截断；
- `title` 的文本参数类型是 `message`，会展开选择器，名叫 `@e` 的地皮就成了一次命令注入；
- `/title` 没法在同一次调用里设置淡入和停留时间，计时用的是客户端上一次存下的值。

这个槽位改为构造一个真正的 `SetTitlePacket`。没有任何线上格式跨过 FFI（数据包在这一侧逐个字段构造），所以它和 `spawn_particle_for` 一样，协议升级以后照样能用。

- 参数：
    - sel : `PlayerSel`
    - typeArg : `int32`
    - text : `string`
    - fadeInTicks : `int32`
    - stayTicks : `int32`
    - fadeOutTicks : `int32`
- 返回值类型：`(bool, error)`
- 对应槽位：[`player_send_title`](../cpp/player.md#player_send_title)

### `Raw.PlayerConnId` {#Raw.PlayerConnId}

```go
func (RawAPI) PlayerConnId(who PlayerSel) (uint64, error)
```

调用 `player_conn_id` 槽位。

这名玩家的连接 id，和数据包拦截器在数据包上下文里看到的是同一个数。

- 参数：
    - who : `PlayerSel`
- 返回值类型：`(uint64, error)`
- 对应槽位：[`player_conn_id`](../cpp/player.md#player_conn_id)

## 实体 {#raw-entity}

### `Raw.ActorSnapshot` {#Raw.ActorSnapshot}

```go
func (RawAPI) ActorSnapshot(id ActorID) ([]string, bool, error)
```

调用 `actor_snapshot` 槽位。

完整的 `Actor::save` NBT，以 SNBT 给出。

- 参数：
    - id : `ActorID`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`actor_snapshot`](../cpp/entity.md#actor_snapshot)

### `Raw.ActorGetNum` {#Raw.ActorGetNum}

```go
func (RawAPI) ActorGetNum(id ActorID, prop int32) (float64, bool, error)
```

调用 `actor_get_num` 槽位。

- 参数：
    - id : `ActorID`
    - prop : `int32`
- 返回值类型：`(float64, bool, error)`
- 对应槽位：[`actor_get_num`](../cpp/entity.md#actor_get_num)

### `Raw.ActorGetStr` {#Raw.ActorGetStr}

```go
func (RawAPI) ActorGetStr(id ActorID, prop int32) ([]string, bool, error)
```

调用 `actor_get_str` 槽位。

- 参数：
    - id : `ActorID`
    - prop : `int32`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`actor_get_str`](../cpp/entity.md#actor_get_str)

### `Raw.ActorAction` {#Raw.ActorAction}

```go
func (RawAPI) ActorAction(id ActorID, action int32, sarg string, a float64, b float64, c float64) ([]string, bool, error)
```

调用 `actor_action` 槽位。

- 参数：
    - id : `ActorID`
    - action : `int32`
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`actor_action`](../cpp/entity.md#actor_action)

### `Raw.SpawnMob` {#Raw.SpawnMob}

```go
func (RawAPI) SpawnMob(dim int32, typeName string, x float64, y float64, z float64) (ActorID, bool, error)
```

调用 `spawn_mob` 槽位。

生成一个生物（`Spawner::spawnMob`）；成功时 `*out` 是它的 `ActorUniqueID`。

- 参数：
    - dim : `int32`
    - typeName : `string`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- 返回值类型：`(ActorID, bool, error)`
- 对应槽位：[`spawn_mob`](../cpp/entity.md#spawn_mob)

### `Raw.ActorGetVehicle` {#Raw.ActorGetVehicle}

```go
func (RawAPI) ActorGetVehicle(id ActorID) (ActorID, bool, error)
```

调用 `actor_get_vehicle` 槽位。

实体：关系、装备、效果、几何（专用函数）

- 参数：
    - id : `ActorID`
- 返回值类型：`(ActorID, bool, error)`
- 对应槽位：[`actor_get_vehicle`](../cpp/entity.md#actor_get_vehicle)

### `Raw.ActorGetFirstPassenger` {#Raw.ActorGetFirstPassenger}

```go
func (RawAPI) ActorGetFirstPassenger(id ActorID) (ActorID, bool, error)
```

调用 `actor_get_first_passenger` 槽位。

- 参数：
    - id : `ActorID`
- 返回值类型：`(ActorID, bool, error)`
- 对应槽位：[`actor_get_first_passenger`](../cpp/entity.md#actor_get_first_passenger)

### `Raw.ActorGetOwner` {#Raw.ActorGetOwner}

```go
func (RawAPI) ActorGetOwner(id ActorID) (ActorID, bool, error)
```

调用 `actor_get_owner` 槽位。

- 参数：
    - id : `ActorID`
- 返回值类型：`(ActorID, bool, error)`
- 对应槽位：[`actor_get_owner`](../cpp/entity.md#actor_get_owner)

### `Raw.ActorGetTarget` {#Raw.ActorGetTarget}

```go
func (RawAPI) ActorGetTarget(id ActorID) (ActorID, bool, error)
```

调用 `actor_get_target` 槽位。

- 参数：
    - id : `ActorID`
- 返回值类型：`(ActorID, bool, error)`
- 对应槽位：[`actor_get_target`](../cpp/entity.md#actor_get_target)

### `Raw.ActorGetEquippedItem` {#Raw.ActorGetEquippedItem}

```go
func (RawAPI) ActorGetEquippedItem(id ActorID, slot int32) ([]string, bool, error)
```

调用 `actor_get_equipped_item` 槽位。

`slot`：0=主手，1=副手，2=头盔，3=胸甲，4=护腿，5=靴子

- 参数：
    - id : `ActorID`
    - slot : `int32`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`actor_get_equipped_item`](../cpp/entity.md#actor_get_equipped_item)

### `Raw.ActorSetEquippedItem` {#Raw.ActorSetEquippedItem}

```go
func (RawAPI) ActorSetEquippedItem(id ActorID, slot int32, itemSnbt string) (bool, error)
```

调用 `actor_set_equipped_item` 槽位。

- 参数：
    - id : `ActorID`
    - slot : `int32`
    - itemSnbt : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`actor_set_equipped_item`](../cpp/entity.md#actor_set_equipped_item)

### `Raw.ActorGetEffects` {#Raw.ActorGetEffects}

```go
func (RawAPI) ActorGetEffects(id ActorID) ([]string, bool, error)
```

调用 `actor_get_effects` 槽位。

SNBT `[{id, ticks, amplifier, visible},…]`

- 参数：
    - id : `ActorID`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`actor_get_effects`](../cpp/entity.md#actor_get_effects)

### `Raw.ActorGetStatusFlag` {#Raw.ActorGetStatusFlag}

```go
func (RawAPI) ActorGetStatusFlag(id ActorID, flagIndex int32) (bool, error)
```

调用 `actor_get_status_flag` 槽位。

`flag_index`：`ActorFlags` 枚举的值（从 0 开始）。

- 参数：
    - id : `ActorID`
    - flagIndex : `int32`
- 返回值类型：`(bool, error)`
- 对应槽位：[`actor_get_status_flag`](../cpp/entity.md#actor_get_status_flag)

### `Raw.ActorSetStatusFlag` {#Raw.ActorSetStatusFlag}

```go
func (RawAPI) ActorSetStatusFlag(id ActorID, flagIndex int32, value bool) (bool, error)
```

调用 `actor_set_status_flag` 槽位。

- 参数：
    - id : `ActorID`
    - flagIndex : `int32`
    - value : `bool`
- 返回值类型：`(bool, error)`
- 对应槽位：[`actor_set_status_flag`](../cpp/entity.md#actor_set_status_flag)

### `Raw.ActorTraceRay` {#Raw.ActorTraceRay}

```go
func (RawAPI) ActorTraceRay(id ActorID, maxDist float32, includeActors bool, includeBlocks bool) ([]string, bool, error)
```

调用 `actor_trace_ray` 槽位。

SNBT `{type:"entity"|"block"|"none", pos:[x,y,z], entity_id?, block_name?}`

- 参数：
    - id : `ActorID`
    - maxDist : `float32`
    - includeActors : `bool`
    - includeBlocks : `bool`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`actor_trace_ray`](../cpp/entity.md#actor_trace_ray)

### `Raw.ActorDistanceTo` {#Raw.ActorDistanceTo}

```go
func (RawAPI) ActorDistanceTo(id ActorID, other ActorID) (float64, bool, error)
```

调用 `actor_distance_to` 槽位。

- 参数：
    - id : `ActorID`
    - other : `ActorID`
- 返回值类型：`(float64, bool, error)`
- 对应槽位：[`actor_distance_to`](../cpp/entity.md#actor_distance_to)

### `Raw.ActorGetAabb` {#Raw.ActorGetAabb}

```go
func (RawAPI) ActorGetAabb(id ActorID) ([]string, bool, error)
```

调用 `actor_get_aabb` 槽位。

SNBT `{min:[x,y,z], max:[x,y,z]}`

- 参数：
    - id : `ActorID`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`actor_get_aabb`](../cpp/entity.md#actor_get_aabb)

### `Raw.ActorClone` {#Raw.ActorClone}

```go
func (RawAPI) ActorClone(id ActorID, dim int32, x float64, y float64, z float64) (ActorID, bool, error)
```

调用 `actor_clone` 槽位。

- 参数：
    - id : `ActorID`
    - dim : `int32`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- 返回值类型：`(ActorID, bool, error)`
- 对应槽位：[`actor_clone`](../cpp/entity.md#actor_clone)

## 方块 {#raw-block}

### `Raw.BlockGetNum` {#Raw.BlockGetNum}

```go
func (RawAPI) BlockGetNum(dim int32, x int32, y int32, z int32, prop int32) (float64, bool, error)
```

调用 `block_get_num` 槽位。

§D 方块与方块实体

- 参数：
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
    - prop : `int32`
- 返回值类型：`(float64, bool, error)`
- 对应槽位：[`block_get_num`](../cpp/block.md#block_get_num)

### `Raw.BlockGetStr` {#Raw.BlockGetStr}

```go
func (RawAPI) BlockGetStr(dim int32, x int32, y int32, z int32, prop int32) ([]string, bool, error)
```

调用 `block_get_str` 槽位。

- 参数：
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
    - prop : `int32`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`block_get_str`](../cpp/block.md#block_get_str)

### `Raw.BlockAction` {#Raw.BlockAction}

```go
func (RawAPI) BlockAction(dim int32, x int32, y int32, z int32, action int32, sarg string) ([]string, bool, error)
```

调用 `block_action` 槽位。

- 参数：
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
    - action : `int32`
    - sarg : `string`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`block_action`](../cpp/block.md#block_action)

### `Raw.BlockEntitySnbt` {#Raw.BlockEntitySnbt}

```go
func (RawAPI) BlockEntitySnbt(dim int32, x int32, y int32, z int32) ([]string, bool, error)
```

调用 `block_entity_snbt` 槽位。

`BlockActor::save`（使用默认的 `SaveContext`）的结果，以 SNBT 给出；那个位置没有方块实体时返回 false。

- 参数：
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`block_entity_snbt`](../cpp/block.md#block_entity_snbt)

### `Raw.BlockGetState` {#Raw.BlockGetState}

```go
func (RawAPI) BlockGetState(dim int32, x int32, y int32, z int32, stateName string) ([]string, bool, error)
```

调用 `block_get_state` 槽位。

方块：读写状态、碰撞形状（专用函数）

- 参数：
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
    - stateName : `string`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`block_get_state`](../cpp/block.md#block_get_state)

### `Raw.BlockSetState` {#Raw.BlockSetState}

```go
func (RawAPI) BlockSetState(dim int32, x int32, y int32, z int32, stateName string, value string) (bool, error)
```

调用 `block_set_state` 槽位。

- 参数：
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
    - stateName : `string`
    - value : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`block_set_state`](../cpp/block.md#block_set_state)

### `Raw.BlockGetCollisionShape` {#Raw.BlockGetCollisionShape}

```go
func (RawAPI) BlockGetCollisionShape(dim int32, x int32, y int32, z int32) ([]string, bool, error)
```

调用 `block_get_collision_shape` 槽位。

- 参数：
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`block_get_collision_shape`](../cpp/block.md#block_get_collision_shape)

## 物品与容器 {#raw-item}

### `Raw.ItemGetNum` {#Raw.ItemGetNum}

```go
func (RawAPI) ItemGetNum(itemSnbt string, prop int32) (float64, bool, error)
```

调用 `item_get_num` 槽位。

§E 物品（SNBT 值对象）与容器

- 参数：
    - itemSnbt : `string`
    - prop : `int32`
- 返回值类型：`(float64, bool, error)`
- 对应槽位：[`item_get_num`](../cpp/item.md#item_get_num)

### `Raw.ItemGetStr` {#Raw.ItemGetStr}

```go
func (RawAPI) ItemGetStr(itemSnbt string, prop int32) ([]string, bool, error)
```

调用 `item_get_str` 槽位。

- 参数：
    - itemSnbt : `string`
    - prop : `int32`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`item_get_str`](../cpp/item.md#item_get_str)

### `Raw.ItemTransform` {#Raw.ItemTransform}

```go
func (RawAPI) ItemTransform(itemSnbt string, op int32, sarg string, narg float64) ([]string, bool, error)
```

调用 `item_transform` 槽位。

先重建物品，再修改，最后序列化；`out` 收到的是**新的**物品 SNBT。

- 参数：
    - itemSnbt : `string`
    - op : `int32`
    - sarg : `string`
    - narg : `float64`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`item_transform`](../cpp/item.md#item_transform)

### `Raw.ContainerSize` {#Raw.ContainerSize}

```go
func (RawAPI) ContainerSize(ref ContainerRef) (int32, bool, error)
```

调用 `container_size` 槽位。

- 参数：
    - ref : `ContainerRef`
- 返回值类型：`(int32, bool, error)`
- 对应槽位：[`container_size`](../cpp/item.md#container_size)

### `Raw.ContainerGetItem` {#Raw.ContainerGetItem}

```go
func (RawAPI) ContainerGetItem(ref ContainerRef, slot int32) ([]string, bool, error)
```

调用 `container_get_item` 槽位。

这一格的内容，以物品 SNBT 给出（空格子给出空气物品的 SNBT）。

- 参数：
    - ref : `ContainerRef`
    - slot : `int32`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`container_get_item`](../cpp/item.md#container_get_item)

### `Raw.ContainerSetItem` {#Raw.ContainerSetItem}

```go
func (RawAPI) ContainerSetItem(ref ContainerRef, slot int32, itemSnbt string) (bool, error)
```

调用 `container_set_item` 槽位。

- 参数：
    - ref : `ContainerRef`
    - slot : `int32`
    - itemSnbt : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`container_set_item`](../cpp/item.md#container_set_item)

### `Raw.ContainerAddItem` {#Raw.ContainerAddItem}

```go
func (RawAPI) ContainerAddItem(ref ContainerRef, itemSnbt string) (bool, error)
```

调用 `container_add_item` 槽位。

- 参数：
    - ref : `ContainerRef`
    - itemSnbt : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`container_add_item`](../cpp/item.md#container_add_item)

### `Raw.ContainerRemoveItem` {#Raw.ContainerRemoveItem}

```go
func (RawAPI) ContainerRemoveItem(ref ContainerRef, slot int32, count int32) (bool, error)
```

调用 `container_remove_item` 槽位。

- 参数：
    - ref : `ContainerRef`
    - slot : `int32`
    - count : `int32`
- 返回值类型：`(bool, error)`
- 对应槽位：[`container_remove_item`](../cpp/item.md#container_remove_item)

### `Raw.ContainerClear` {#Raw.ContainerClear}

```go
func (RawAPI) ContainerClear(ref ContainerRef) (bool, error)
```

调用 `container_clear` 槽位。

- 参数：
    - ref : `ContainerRef`
- 返回值类型：`(bool, error)`
- 对应槽位：[`container_clear`](../cpp/item.md#container_clear)

### `Raw.ItemGetEnchants` {#Raw.ItemGetEnchants}

```go
func (RawAPI) ItemGetEnchants(itemSnbt string) ([]string, bool, error)
```

调用 `item_get_enchants` 槽位。

SNBT `[{id, level},…]`

- 参数：
    - itemSnbt : `string`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`item_get_enchants`](../cpp/item.md#item_get_enchants)

### `Raw.ItemSetEnchants` {#Raw.ItemSetEnchants}

```go
func (RawAPI) ItemSetEnchants(itemSnbt string, enchantsSnbt string) ([]string, bool, error)
```

调用 `item_set_enchants` 槽位。

`enchants_snbt` 形如 `[{id, level},…]`；`out` 收到新的物品 SNBT。当前所有宿主上这个槽位都是 NULL：写附魔还没有实现，读取一侧是 `item_get_enchants`。

- 参数：
    - itemSnbt : `string`
    - enchantsSnbt : `string`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`item_set_enchants`](../cpp/item.md#item_set_enchants)

### `Raw.ItemMatches` {#Raw.ItemMatches}

```go
func (RawAPI) ItemMatches(a string, b string) (bool, error)
```

调用 `item_matches` 槽位。

- 参数：
    - a : `string`
    - b : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`item_matches`](../cpp/item.md#item_matches)

### `Raw.ItemGetUserData` {#Raw.ItemGetUserData}

```go
func (RawAPI) ItemGetUserData(itemSnbt string) ([]string, bool, error)
```

调用 `item_get_user_data` 槽位。

- 参数：
    - itemSnbt : `string`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`item_get_user_data`](../cpp/item.md#item_get_user_data)

### `Raw.ContainerRefresh` {#Raw.ContainerRefresh}

```go
func (RawAPI) ContainerRefresh(ref ContainerRef) (bool, error)
```

调用 `container_refresh` 槽位。

把玩家自己的容器（`which` 为 0 到 3）重新发给它的主人。方块容器（`which == 4`）返回 false：箱子没有唯一的主人可以重发，正在看它的玩家由引擎自己的容器事务流程刷新。

- 参数：
    - ref : `ContainerRef`
- 返回值类型：`(bool, error)`
- 对应槽位：[`container_refresh`](../cpp/item.md#container_refresh)

## 计分板 {#raw-scoreboard}

### `Raw.ScoreboardOp` {#Raw.ScoreboardOp}

```go
func (RawAPI) ScoreboardOp(op int32, a string, b string, n int64) ([]string, bool, error)
```

调用 `scoreboard_op` 槽位。

§F 计分板

- 参数：
    - op : `int32`
    - a : `string`
    - b : `string`
    - n : `int64`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)

## NBT 与键值数据库 {#raw-data}

### `Raw.NbtBinaryToSnbt` {#Raw.NbtBinaryToSnbt}

```go
func (RawAPI) NbtBinaryToSnbt(data []byte, fmt int32) ([]string, bool, error)
```

调用 `nbt_binary_to_snbt` 槽位。

- 参数：
    - data : `[]byte`
    - fmt : `int32`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`nbt_binary_to_snbt`](../cpp/data.md#nbt_binary_to_snbt)

### `Raw.KvdbOpen` {#Raw.KvdbOpen}

```go
func (RawAPI) KvdbOpen(path string, createIfMissing bool) (KvDbHandle, error)
```

调用 `kvdb_open` 槽位。

KvDb：线程安全（内部有互斥锁）。路径限定在模组自己的数据目录里，带 `..` 的路径和绝对路径会被拒绝。句柄归加载器所有，模组卸载时会被强制关闭，并记一条警告。

- 参数：
    - path : `string`
    - createIfMissing : `bool`
- 返回值类型：`(KvDbHandle, error)`
- 对应槽位：[`kvdb_open`](../cpp/data.md#kvdb_open)

### `Raw.KvdbClose` {#Raw.KvdbClose}

```go
func (RawAPI) KvdbClose(hArg KvDbHandle) error
```

调用 `kvdb_close` 槽位。

- 参数：
    - hArg : `KvDbHandle`
- 返回值类型：`error`
- 对应槽位：[`kvdb_close`](../cpp/data.md#kvdb_close)

### `Raw.KvdbGet` {#Raw.KvdbGet}

```go
func (RawAPI) KvdbGet(hArg KvDbHandle, key string) ([]string, bool, error)
```

调用 `kvdb_get` 槽位。

- 参数：
    - hArg : `KvDbHandle`
    - key : `string`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`kvdb_get`](../cpp/data.md#kvdb_get)

### `Raw.KvdbSet` {#Raw.KvdbSet}

```go
func (RawAPI) KvdbSet(hArg KvDbHandle, key string, value string) (bool, error)
```

调用 `kvdb_set` 槽位。

- 参数：
    - hArg : `KvDbHandle`
    - key : `string`
    - value : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`kvdb_set`](../cpp/data.md#kvdb_set)

### `Raw.KvdbDel` {#Raw.KvdbDel}

```go
func (RawAPI) KvdbDel(hArg KvDbHandle, key string) (bool, error)
```

调用 `kvdb_del` 槽位。

- 参数：
    - hArg : `KvDbHandle`
    - key : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`kvdb_del`](../cpp/data.md#kvdb_del)

### `Raw.KvdbHas` {#Raw.KvdbHas}

```go
func (RawAPI) KvdbHas(hArg KvDbHandle, key string) (bool, error)
```

调用 `kvdb_has` 槽位。

- 参数：
    - hArg : `KvDbHandle`
    - key : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`kvdb_has`](../cpp/data.md#kvdb_has)

### `Raw.KvdbIsEmpty` {#Raw.KvdbIsEmpty}

```go
func (RawAPI) KvdbIsEmpty(hArg KvDbHandle) (bool, error)
```

调用 `kvdb_is_empty` 槽位。

- 参数：
    - hArg : `KvDbHandle`
- 返回值类型：`(bool, error)`
- 对应槽位：[`kvdb_is_empty`](../cpp/data.md#kvdb_is_empty)

## 经济 {#raw-money}

### `Raw.GetMoney` {#Raw.GetMoney}

```go
func (RawAPI) GetMoney(xuid string) (int64, error)
```

调用 `get_money` 槽位。

余额。失败时返回 -1（xuid 为空、数据库出错，或者后端不在）；真实的余额不会是负数，所以小于 0 表示「说不出来」。注意：遇到没见过的 xuid，它会按配置的默认值开一个账户，所以这次读取是有副作用的。

- 参数：
    - xuid : `string`
- 返回值类型：`(int64, error)`
- 对应槽位：[`get_money`](../cpp/money.md#get_money)

### `Raw.SetMoney` {#Raw.SetMoney}

```go
func (RawAPI) SetMoney(xuid string, money int64) (bool, error)
```

调用 `set_money` 槽位。

把余额设为 `money`。它是目标余额，不是差额。

- 参数：
    - xuid : `string`
    - money : `int64`
- 返回值类型：`(bool, error)`
- 对应槽位：[`set_money`](../cpp/money.md#set_money)

### `Raw.AddMoney` {#Raw.AddMoney}

```go
func (RawAPI) AddMoney(xuid string, money int64) (bool, error)
```

调用 `add_money` 槽位。

- 参数：
    - xuid : `string`
    - money : `int64`
- 返回值类型：`(bool, error)`
- 对应槽位：[`add_money`](../cpp/money.md#add_money)

### `Raw.ReduceMoney` {#Raw.ReduceMoney}

```go
func (RawAPI) ReduceMoney(xuid string, money int64) (bool, error)
```

调用 `reduce_money` 槽位。

- 参数：
    - xuid : `string`
    - money : `int64`
- 返回值类型：`(bool, error)`
- 对应槽位：[`reduce_money`](../cpp/money.md#reduce_money)

### `Raw.TransMoney` {#Raw.TransMoney}

```go
func (RawAPI) TransMoney(from string, to string, val int64, note string) (bool, error)
```

调用 `trans_money` 槽位。

`from` 或 `to` 为空，表示钱是凭空生出的，或者转出后就消失了。收款方收到的金额会按 `pay_tax` 扣减（见上面的说明）。`from == to` 会失败。

- 参数：
    - from : `string`
    - to : `string`
    - val : `int64`
    - note : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`trans_money`](../cpp/money.md#trans_money)

### `Raw.MoneyGetHist` {#Raw.MoneyGetHist}

```go
func (RawAPI) MoneyGetHist(xuid string, timediff int32) ([]string, error)
```

调用 `money_get_hist` 槽位。

- 参数：
    - xuid : `string`
    - timediff : `int32`
- 返回值类型：`([]string, error)`
- 对应槽位：[`money_get_hist`](../cpp/money.md#money_get_hist)

### `Raw.MoneyClearHist` {#Raw.MoneyClearHist}

```go
func (RawAPI) MoneyClearHist(difftime int32) error
```

调用 `money_clear_hist` 槽位。

- 参数：
    - difftime : `int32`
- 返回值类型：`error`
- 对应槽位：[`money_clear_hist`](../cpp/money.md#money_clear_hist)

### `Raw.MoneyRanking` {#Raw.MoneyRanking}

```go
func (RawAPI) MoneyRanking(num uint16) ([]string, error)
```

调用 `money_ranking` 槽位。

- 参数：
    - num : `uint16`
- 返回值类型：`([]string, error)`
- 对应槽位：[`money_ranking`](../cpp/money.md#money_ranking)

## 数据包 {#raw-packet}

### `Raw.SendPacket` {#Raw.SendPacket}

```go
func (RawAPI) SendPacket(sel PlayerSel, packetId int32, body []byte) (bool, error)
```

调用 `send_packet` 槽位。

按连接发送原始数据包（追加的槽位，受 `struct_size` 约束），`spawn_particle_for` 就是在它之上实现的。`packet_id` 是 `MinecraftPacketIds` 的值；`body`/`body_len` 是这个数据包在**当前**游戏版本下的线上格式。bridge 把它反序列化成真正的数据包对象（`MinecraftPackets::createPacket` 加 `Packet::read`），只发给解析出来的那名玩家的连接。

以下情况返回 false：玩家不在线；id 不认识或构造不出来；包体解析失败；解析完以后还剩字节（形状不符合这个版本）。这是一个**逃生口**：线上格式随版本变化，正确与否由调用方负责；有带类型的接口时请优先用那些。

- 参数：
    - sel : `PlayerSel`
    - packetId : `int32`
    - body : `[]byte`
- 返回值类型：`(bool, error)`
- 对应槽位：[`send_packet`](../cpp/packet.md#send_packet)

### `Raw.PacketHookUnregister` {#Raw.PacketHookUnregister}

```go
func (RawAPI) PacketHookUnregister(handleArg PacketHookHandle) (bool, error)
```

调用 `packet_hook_unregister` 槽位。

注销。在回调内部调用也是安全的。

- 参数：
    - handleArg : `PacketHookHandle`
- 返回值类型：`(bool, error)`
- 对应槽位：[`packet_hook_unregister`](../cpp/packet.md#packet_hook_unregister)

### `Raw.PacketConnHookUnregister` {#Raw.PacketConnHookUnregister}

```go
func (RawAPI) PacketConnHookUnregister(handleArg PacketHookHandle) (bool, error)
```

调用 `packet_conn_hook_unregister` 槽位。

注销。在回调内部调用也是安全的。

- 参数：
    - handleArg : `PacketHookHandle`
- 返回值类型：`(bool, error)`
- 对应槽位：[`packet_conn_hook_unregister`](../cpp/packet.md#packet_conn_hook_unregister)

## 模拟玩家 {#raw-sim}

### `Raw.SimSpawn` {#Raw.SimSpawn}

```go
func (RawAPI) SimSpawn(name string, dimension int32, x float64, y float64, z float64) (bool, error)
```

调用 `sim_spawn` 槽位。

模拟（「假」）玩家（追加的槽位，受 `struct_size` 约束）。`sim_spawn` 用那个名字创建一个真正的 `ServerPlayer`，现有的每一个按玩家操作的接口（传送、生命值、物品栏、踢出……）都能通过普通的名字选择器作用在它身上。`sim_do` 复用 `simulate*` 这一族动作：动作的种类在 bridge 一侧增加，不需要新的表槽位（动作有 `despawn stop jump attack interact use_item drop respawn move_to navigate_to look_at destroy_block destroy_look stop_destroy interact_block sneak fly chat`，参数用 SNBT，见文档）。以 `isSimulatedPlayer()` 把关：真实玩家永远不会被操纵。动作不认识、参数格式不对、目标不在线或者不是模拟玩家时返回 false。

- 参数：
    - name : `string`
    - dimension : `int32`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- 返回值类型：`(bool, error)`
- 对应槽位：[`sim_spawn`](../cpp/sim.md#sim_spawn)

### `Raw.SimDo` {#Raw.SimDo}

```go
func (RawAPI) SimDo(sel PlayerSel, action string, argsSnbt string) (bool, error)
```

调用 `sim_do` 槽位。

- 参数：
    - sel : `PlayerSel`
    - action : `string`
    - argsSnbt : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`sim_do`](../cpp/sim.md#sim_do)

### `Raw.SimIs` {#Raw.SimIs}

```go
func (RawAPI) SimIs(sel PlayerSel) (bool, error)
```

调用 `sim_is` 槽位。

选择器能解析到一个活着的模拟玩家时返回 true。模组可以借此在重启后重新确认一个机器人：`SimulatedPlayer` 会保存在世界里，内存里的句柄不会。

- 参数：
    - sel : `PlayerSel`
- 返回值类型：`(bool, error)`
- 对应槽位：[`sim_is`](../cpp/sim.md#sim_is)

### `Raw.SimList` {#Raw.SimList}

```go
func (RawAPI) SimList() ([]string, error)
```

调用 `sim_list` 槽位。

列出所有活着的模拟玩家的名字（输出回调逐个收到名字）。用名字重建句柄，就能操纵一个比生成它的那次会话活得更久的机器人。

- 返回值类型：`([]string, error)`
- 对应槽位：[`sim_list`](../cpp/sim.md#sim_list)

## 客户端 {#raw-client}

### `Raw.ClientGetLocalPlayer` {#Raw.ClientGetLocalPlayer}

```go
func (RawAPI) ClientGetLocalPlayer() ([]string, bool, error)
```

调用 `client_get_local_player` 槽位。

通过 `ll::service::getClientInstance()->getLocalPlayer()` 取本地玩家的名字。输出回调收到名字；不在世界里时调用返回 false。

- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`client_get_local_player`](../cpp/client.md#client_get_local_player)

### `Raw.ClientIsInLevel` {#Raw.ClientIsInLevel}

```go
func (RawAPI) ClientIsInLevel() (bool, error)
```

调用 `client_is_in_level` 槽位。

客户端在世界里（已经加载了一个世界）时为 true。

- 返回值类型：`(bool, error)`
- 对应槽位：[`client_is_in_level`](../cpp/client.md#client_is_in_level)

### `Raw.ClientGetScreenName` {#Raw.ClientGetScreenName}

```go
func (RawAPI) ClientGetScreenName() ([]string, bool, error)
```

调用 `client_get_screen_name` 槽位。

当前界面的名字（例如 `"hud_screen"`、`"pause_screen"`）。当前所有宿主上这个槽位都是 NULL，客户端构建也一样：引擎没有提供稳定的访问方式。

- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`client_get_screen_name`](../cpp/client.md#client_get_screen_name)

### `Raw.ClientUnregisterKey` {#Raw.ClientUnregisterKey}

```go
func (RawAPI) ClientUnregisterKey(handleArg KeyHandle) (bool, error)
```

调用 `client_unregister_key` 槽位。

取消一个按键绑定：这个模组的处理函数不再触发，句柄被释放。`ll::input::KeyHandle` 本身不会被销毁，因为按键注册表每个名字只保留一个按键，也没有提供删除的办法；用同一个名字再注册时会复用那个按键。

- 参数：
    - handleArg : `KeyHandle`
- 返回值类型：`(bool, error)`
- 对应槽位：[`client_unregister_key`](../cpp/client.md#client_unregister_key)

### `Raw.ClientGetKeyCodes` {#Raw.ClientGetKeyCodes}

```go
func (RawAPI) ClientGetKeyCodes(handleArg KeyHandle) ([]string, bool, error)
```

调用 `client_get_key_codes` 槽位。

当前分配的按键码（被玩家重新映射过时可能和默认值不同）。输出回调收到一个 JSON 风格的数组字符串 `"[1,2,3]"`。

- 参数：
    - handleArg : `KeyHandle`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`client_get_key_codes`](../cpp/client.md#client_get_key_codes)

## 自定义维度 {#raw-dimensions}

### `Raw.MdIsAvailable` {#Raw.MdIsAvailable}

```go
func (RawAPI) MdIsAvailable() (bool, error)
```

调用 `md_is_available` 槽位。

这个宿主能不能注册自定义维度。没有编入 pier-dimensions 时这个槽位是 NULL。有这个槽位时，它的回答来自对引擎维度定义表的一次探测，所以在内存布局和这次构建对不上的引擎上可能回答 false；第一次能给出回答之后，结果会被缓存。

- 返回值类型：`(bool, error)`
- 对应槽位：[`md_is_available`](../cpp/dimensions.md#md_is_available)

### `Raw.MdSetDimensionRule` {#Raw.MdSetDimensionRule}

```go
func (RawAPI) MdSetDimensionRule(dimension int32, rule int32, allow bool) error
```

调用 `md_set_dimension_rule` 槽位。

按维度设置的规则，由加载器自己的钩子查询。

- 参数：
    - dimension : `int32`
    - rule : `int32`
    - allow : `bool`
- 返回值类型：`error`
- 对应槽位：[`md_set_dimension_rule`](../cpp/dimensions.md#md_set_dimension_rule)

### `Raw.MdGetDimensionRule` {#Raw.MdGetDimensionRule}

```go
func (RawAPI) MdGetDimensionRule(dimension int32, rule int32) (bool, bool, error)
```

调用 `md_get_dimension_rule` 槽位。

读回一条规则。只有这个维度对这条规则有显式设置时才写入 `outAllow`，否则返回 false。宿主不认识的规则编号也回答 false，而 `md_set_dimension_rule` 会忽略这样的编号，所以比宿主新的绑定在这里分不清「没有设置」和「不支持」。

- 参数：
    - dimension : `int32`
    - rule : `int32`
- 返回值类型：`(bool, bool, error)`
- 对应槽位：[`md_get_dimension_rule`](../cpp/dimensions.md#md_get_dimension_rule)

### `Raw.MdClearDimensionRules` {#Raw.MdClearDimensionRules}

```go
func (RawAPI) MdClearDimensionRules(dimension int32) error
```

调用 `md_clear_dimension_rules` 槽位。

删除一个维度的所有规则（删除世界时使用）。

- 参数：
    - dimension : `int32`
- 返回值类型：`error`
- 对应槽位：[`md_clear_dimension_rules`](../cpp/dimensions.md#md_clear_dimension_rules)

### `Raw.MdGetDimensionId` {#Raw.MdGetDimensionId}

```go
func (RawAPI) MdGetDimensionId(name string) (int32, error)
```

调用 `md_get_dimension_id` 槽位。

把维度名解析成维度 id。找不到时返回 -1。

- 参数：
    - name : `string`
- 返回值类型：`(int32, error)`
- 对应槽位：[`md_get_dimension_id`](../cpp/dimensions.md#md_get_dimension_id)

### `Raw.MdSetPlotMerges` {#Raw.MdSetPlotMerges}

```go
func (RawAPI) MdSetPlotMerges(dimension int32, entries []int32) error
```

调用 `md_set_plot_merges` 槽位。

整体替换一个维度的合并标记。`entries` 是 `count` 个三元组 `(x, z, mask)`，也就是 `count * 3` 个 int32；`mask` 是位集合，1=北，2=东，4=南，8=西，和插件的 `merged[]` 下标对应。只需要发送确实带有标记的地皮。

- 参数：
    - dimension : `int32`
    - entries : `[]int32`
- 返回值类型：`error`
- 对应槽位：[`md_set_plot_merges`](../cpp/dimensions.md#md_set_plot_merges)

### `Raw.MdListDimensions` {#Raw.MdListDimensions}

```go
func (RawAPI) MdListDimensions() ([]string, error)
```

调用 `md_list_dimensions` 槽位。

以 JSON 数组列出所有已注册的自定义维度：`[{"name":"plot_world","dim":1000,"snbt":"{…}"}]`。

- 返回值类型：`([]string, error)`
- 对应槽位：[`md_list_dimensions`](../cpp/dimensions.md#md_list_dimensions)

### `Raw.MdAddDimension` {#Raw.MdAddDimension}

```go
func (RawAPI) MdAddDimension(name string, specSnbt string) (int32, error)
```

调用 `md_add_dimension` 槽位。

用一份声明式的描述，添加一个使用原生地形的自定义维度。

- 参数：
    - name : `string`
    - specSnbt : `string`
- 返回值类型：`(int32, error)`
- 对应槽位：[`md_add_dimension`](../cpp/dimensions.md#md_add_dimension)

### `Raw.MdAddDimensionPack` {#Raw.MdAddDimensionPack}

```go
func (RawAPI) MdAddDimensionPack(name string, configPath string, specSnbt string) (int32, error)
```

调用 `md_add_dimension_pack` 槽位。

添加一个地形来自地形包的自定义维度。地形包是一个目录，里面有一个配置文件和一个由 tools/pier-pack 构建的二进制文件。`config_path` 指定配置文件，相对于服务器根目录，用正斜杠，不能有 `..`，也不能是绝对路径；二进制文件的名字写在配置里，相对于配置文件。

- 参数：
    - name : `string`
    - configPath : `string`
    - specSnbt : `string`
- 返回值类型：`(int32, error)`
- 对应槽位：[`md_add_dimension_pack`](../cpp/dimensions.md#md_add_dimension_pack)

### `Raw.MdPackInspect` {#Raw.MdPackInspect}

```go
func (RawAPI) MdPackInspect(configPath string) ([]string, int32, error)
```

调用 `md_pack_inspect` 槽位。

查看一个包要求什么，不注册任何东西。输出回调收到一个 JSON 文档，形状和 `pier-pack inspect` 打印的一样：

```text
{"ok":true,"kind":"template","pack":"...","sha256":"...","name":"...",
"biome":"...","height":{"min":-512,"max":320,"fixed":false},
"params":[{"name":"plot_size","kind":"free","default":64,"min":4,
"max":512,"step":1},
{"name":"wall_style","kind":"choice","default":0,"choices":[0,1]},
{"name":"plot_depth","kind":"derived"},
{"name":"size","kind":"fixed","value":256}],
"roles":[{"name":"floor","default":"minecraft:grass_block"}],
"zones":["interior","border","road"],
"constraints":["..."], "shapes":23, "picks":1,
"voxels":[{"size":[5,4,3],"palette":5}], "confine":true}
```

模组按 `"params"` 构造表单：`free` 用滑块，`choice` 用列表，`fixed` 是只读的值，`derived` 不显示。被拒绝时，输出回调收到 `{"ok":false,"status":<code>,"problems":["..."]}`，并返回这个代码。返回 0 或某个 `PIER_PACK_*` 代码。只能在服务器线程调用。

- 参数：
    - configPath : `string`
- 返回值类型：`([]string, int32, error)`
- 对应槽位：[`md_pack_inspect`](../cpp/dimensions.md#md_pack_inspect)

### `Raw.MdRetireDimension` {#Raw.MdRetireDimension}

```go
func (RawAPI) MdRetireDimension(name string) (bool, error)
```

调用 `md_retire_dimension` 槽位。

让一个自定义维度退役：把它从 dimension_config.json、宿主自己的表和维度工厂里去掉，下次启动时没有任何东西会注册它，它也不再出现在 `md_list_dimensions` 里。

- 参数：
    - name : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`md_retire_dimension`](../cpp/dimensions.md#md_retire_dimension)

### `Raw.MdSetDimensionCells` {#Raw.MdSetDimensionCells}

```go
func (RawAPI) MdSetDimensionCells(dimId int32, cell int32, gap int32) (bool, error)
```

调用 `md_set_dimension_cells` 槽位。

给一个生成式维度设置约束规则所用的单元几何。

- 参数：
    - dimId : `int32`
    - cell : `int32`
    - gap : `int32`
- 返回值类型：`(bool, error)`
- 对应槽位：[`md_set_dimension_cells`](../cpp/dimensions.md#md_set_dimension_cells)

## 跨模组：总线、服务与快速通道 {#raw-crossmod}

### `Raw.BusUnsubscribe` {#Raw.BusUnsubscribe}

```go
func (RawAPI) BusUnsubscribe(subId uint64) (bool, error)
```

调用 `bus_unsubscribe` 槽位。

删除这个模组的一个订阅。只作用于调用方自己，一个模组不能取消另一个模组的订阅。真的删除了一个订阅时返回 true。在回调内部（包括自己的回调里）调用也是安全的。

- 参数：
    - subId : `uint64`
- 返回值类型：`(bool, error)`
- 对应槽位：[`bus_unsubscribe`](../cpp/crossmod.md#bus_unsubscribe)

### `Raw.BusPublish` {#Raw.BusPublish}

```go
func (RawAPI) BusPublish(topic string, payload string) (uint32, error)
```

调用 `bus_publish` 槽位。

把 `payload` 投递给订阅了 `topic` 的每一个**其他**模组。返回实际运行了多少个订阅者（0 是正常的，表示没有人在听）。订阅者的返回值被忽略。

- 参数：
    - topic : `string`
    - payload : `string`
- 返回值类型：`(uint32, error)`
- 对应槽位：[`bus_publish`](../cpp/crossmod.md#bus_publish)

### `Raw.BusPublishVetoable` {#Raw.BusPublishVetoable}

```go
func (RawAPI) BusPublishVetoable(topic string, payload string) (uint32, bool, error)
```

调用 `bus_publish_vetoable` 槽位。

同上，但会收集否决位：任何一个订阅者返回 true，就返回 true。每个订阅者照样都会运行，不会提前结束，所以无论前面有没有订阅者拒绝，观察者看到的都是一样完整的消息流。`out_delivered` 可以为 NULL。

- 参数：
    - topic : `string`
    - payload : `string`
- 返回值类型：`(uint32, bool, error)`
- 对应槽位：[`bus_publish_vetoable`](../cpp/crossmod.md#bus_publish_vetoable)

### `Raw.BusSubscriberCount` {#Raw.BusSubscriberCount}

```go
func (RawAPI) BusSubscriberCount(topic string) (uint32, error)
```

调用 `bus_subscriber_count` 槽位。

一个主题当前有多少订阅者，所有模组加在一起。用来在没人会读的时候，省掉构造载荷的开销。

- 参数：
    - topic : `string`
- 返回值类型：`(uint32, error)`
- 对应槽位：[`bus_subscriber_count`](../cpp/crossmod.md#bus_subscriber_count)

### `Raw.ServiceUnregister` {#Raw.ServiceUnregister}

```go
func (RawAPI) ServiceUnregister(regId uint64) (bool, error)
```

调用 `service_unregister` 槽位。

删除这个模组的一项注册。只作用于调用方自己，一个模组不能注销另一个模组的服务。

- 参数：
    - regId : `uint64`
- 返回值类型：`(bool, error)`
- 对应槽位：[`service_unregister`](../cpp/crossmod.md#service_unregister)

### `Raw.ServiceCall` {#Raw.ServiceCall}

```go
func (RawAPI) ServiceCall(name string, request string) ([]string, int32, error)
```

调用 `service_call` 槽位。

用 `request` 调用名为 `name` 的服务，提供方的回答通过 `reply` 送回。返回某个 `PIER_SERVICE_*`。模组不能调用自己的服务：它可以直接调用自己的函数，而自己调用自己是最难看清的一种循环。

- 参数：
    - name : `string`
    - request : `string`
- 返回值类型：`([]string, int32, error)`
- 对应槽位：[`service_call`](../cpp/crossmod.md#service_call)

### `Raw.ServiceList` {#Raw.ServiceList}

```go
func (RawAPI) ServiceList() ([]string, error)
```

调用 `service_list` 槽位。

所有已注册的服务，以 JSON 数组给出，每一项是 `{"name":…,"mod":…}`。用于诊断，也方便调用方在构造请求之前先确认有没有人能回答。

- 返回值类型：`([]string, error)`
- 对应槽位：[`service_list`](../cpp/crossmod.md#service_list)

### `Raw.LaneUnpublish` {#Raw.LaneUnpublish}

```go
func (RawAPI) LaneUnpublish(pubId uint64) (bool, error)
```

调用 `lane_unpublish` 槽位。

撤回这个模组拥有的一个快速通道。对每一个还没归还的租约调用 `release`，并清除存活标记，这样使用方下一次检查就会发现通道已经没了，不会再通过一个失效的指针跳转。

- 参数：
    - pubId : `uint64`
- 返回值类型：`(bool, error)`
- 对应槽位：[`lane_unpublish`](../cpp/crossmod.md#lane_unpublish)

### `Raw.LaneRelease` {#Raw.LaneRelease}

```go
func (RawAPI) LaneRelease(lease uint64) (bool, error)
```

调用 `lane_release` 槽位。

归还一个租约，只能是这个模组持有的租约。提供方已经不在时返回 false：那时加载器已经替它调用过 `release`，再调用一次就是重复释放。

- 参数：
    - lease : `uint64`
- 返回值类型：`(bool, error)`
- 对应槽位：[`lane_release`](../cpp/crossmod.md#lane_release)

### `Raw.LaneList` {#Raw.LaneList}

```go
func (RawAPI) LaneList() ([]string, error)
```

调用 `lane_list` 槽位。

所有快速通道，以 JSON 数组给出：`[{"name":…,"mod":…,"fingerprint":"0x…","protocol":1,"leases":N,"alive":true}]`

- 返回值类型：`([]string, error)`
- 对应槽位：[`lane_list`](../cpp/crossmod.md#lane_list)

### `Raw.ServiceCaller` {#Raw.ServiceCaller}

```go
func (RawAPI) ServiceCaller() ([]string, error)
```

调用 `service_caller` 槽位。

正在运行的这个服务回调，是谁调用的。

- 返回值类型：`([]string, error)`
- 对应槽位：[`service_caller`](../cpp/crossmod.md#service_caller)

## 注册表 {#raw-registry}

### `Raw.RegistryList` {#Raw.RegistryList}

```go
func (RawAPI) RegistryList(kind int32) ([]string, bool, error)
```

调用 `registry_list` 槽位。

通过 `sink` 列出引擎的某一个注册表，每一项一个 JSON 对象，顺序不定。`kind` 是某个 `PIER_REGISTRY_*` 值。

- 参数：
    - kind : `int32`
- 返回值类型：`([]string, bool, error)`
- 对应槽位：[`registry_list`](../cpp/registry.md#registry_list)
