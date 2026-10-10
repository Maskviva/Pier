# Go：世界

## 函数 {#functions}

### `ScanRegion` {#ScanRegion}

```go
func ScanRegion(dim, x1, y1, z1, x2, y2, z2 int32) ([]BlockInfo, []EntityInfo, error)
```

读取一个长方体里的每个方块和实体。只能在服务器线程调用。

- 参数：
    - dim : `int32`
    - x1 : `int32`
    - y1 : `int32`
    - z1 : `int32`
    - x2 : `int32`
    - y2 : `int32`
    - z2 : `int32`
- 返回值类型：`([]BlockInfo, []EntityInfo, error)`
- 对应槽位：[`scan_region`](../cpp/world.md#scan_region)

### `ScanRegionIndexed` {#ScanRegionIndexed}

```go
func ScanRegionIndexed(dim, x1, y1, z1, x2, y2, z2 int32) ([]PaletteEntry, []BlockCell, error)
```

把一个长方体读成由不同方块状态组成的调色板，加上引用调色板的格子；长方体很大时，结果比 `ScanRegion` 小得多。只能在服务器线程调用。

- 参数：
    - dim : `int32`
    - x1 : `int32`
    - y1 : `int32`
    - z1 : `int32`
    - x2 : `int32`
    - y2 : `int32`
    - z2 : `int32`
- 返回值类型：`([]PaletteEntry, []BlockCell, error)`
- 对应槽位：[`scan_region_indexed`](../cpp/world.md#scan_region_indexed)

### `GetBlock` {#GetBlock}

```go
func GetBlock(dim, x, y, z int32) (BlockInfo, error)
```

读取一格的方块。只能在服务器线程调用。

- 参数：
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
- 返回值类型：`(BlockInfo, error)`
- 对应槽位：[`get_block`](../cpp/world.md#get_block)

### `Seed` {#Seed}

```go
func Seed() (int64, error)
```

世界种子。

- 返回值类型：`(int64, error)`
- 对应槽位：[`get_seed`](../cpp/world.md#get_seed)

### `Difficulty` {#Difficulty}

```go
func Difficulty() (int32, error)
```

世界难度，从 0 和平到 3 困难。

- 返回值类型：`(int32, error)`
- 对应槽位：[`get_difficulty`](../cpp/world.md#get_difficulty)

### `SetDifficulty` {#SetDifficulty}

```go
func SetDifficulty(d int32) error
```

设置世界难度。

- 参数：
    - d : `int32`
- 返回值类型：`error`
- 对应槽位：[`set_difficulty`](../cpp/world.md#set_difficulty)

### `GameRule` {#GameRule}

```go
func GameRule(name string) (string, error)
```

以文本形式读取一条游戏规则。

- 参数：
    - name : `string`
- 返回值类型：`(string, error)`
- 对应槽位：[`game_rule_get`](../cpp/world.md#game_rule_get)

### `SetGameRule` {#SetGameRule}

```go
func SetGameRule(name, value string) error
```

以文本形式写入一条游戏规则。

- 参数：
    - name : `string`
    - value : `string`
- 返回值类型：`error`
- 对应槽位：[`game_rule_set`](../cpp/world.md#game_rule_set)

### `Time` {#Time}

```go
func Time() (int64, error)
```

一天中的时间，单位是刻。

- 返回值类型：`(int64, error)`
- 对应槽位：[`get_time`](../cpp/world.md#get_time)

### `SetTime` {#SetTime}

```go
func SetTime(t int64) error
```

设置一天中的时间，单位是刻。

- 参数：
    - t : `int64`
- 返回值类型：`error`
- 对应槽位：[`set_time`](../cpp/world.md#set_time)

### `SetWeather` {#SetWeather}

```go
func SetWeather(weather int32) error
```

设置天气：0 晴，1 雨，2 雷雨。

- 参数：
    - weather : `int32`
- 返回值类型：`error`
- 对应槽位：[`set_weather`](../cpp/world.md#set_weather)

### `SaveLevel` {#SaveLevel}

```go
func SaveLevel() error
```

立刻保存世界。

- 返回值类型：`error`
- 对应槽位：[`level_save`](../cpp/world.md#level_save)

### `SpawnParticle` {#SpawnParticle}

```go
func SpawnParticle(dim int32, effect string, x, y, z float64) error
```

向附近的所有人显示一个粒子效果。

- 参数：
    - dim : `int32`
    - effect : `string`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- 返回值类型：`error`
- 对应槽位：[`spawn_particle`](../cpp/world.md#spawn_particle)

### `Explode` {#Explode}

```go
func Explode(dim int32, x, y, z float64, radius float32, o ExplodeOptions) error
```

按给定的半径制造一次爆炸。

- 参数：
    - dim : `int32`
    - x : `float64`
    - y : `float64`
    - z : `float64`
    - radius : `float32`
    - o : `ExplodeOptions`
- 返回值类型：`error`
- 对应槽位：[`explode`](../cpp/world.md#explode)

### `DefaultSpawn` {#DefaultSpawn}

```go
func DefaultSpawn() (x, y, z int32, err error)
```

世界的出生点。

- 返回值类型：`(x, y, z int32, err error)`
- 对应槽位：[`level_get_default_spawn`](../cpp/world.md#level_get_default_spawn)

### `SetDefaultSpawn` {#SetDefaultSpawn}

```go
func SetDefaultSpawn(x, y, z int32) error
```

移动世界的出生点。

- 参数：
    - x : `int32`
    - y : `int32`
    - z : `int32`
- 返回值类型：`error`
- 对应槽位：[`level_set_default_spawn`](../cpp/world.md#level_set_default_spawn)

### `SetBiome` {#SetBiome}

```go
func SetBiome(dim, minX, minZ, maxX, maxZ int32, biome string) (int32, error)
```

设置一个矩形内每一列的生物群系，返回改变了多少列。

- 参数：
    - dim : `int32`
    - minX : `int32`
    - minZ : `int32`
    - maxX : `int32`
    - maxZ : `int32`
    - biome : `string`
- 返回值类型：`(int32, error)`
- 对应槽位：[`level_set_biome`](../cpp/world.md#level_set_biome)

## `EntityInfo` {#EntityInfo}

```go
type EntityInfo struct {
    X, Y, Z int32
    Type    string
    SNBT    string
}
```

区域扫描找到的一个实体：它所在的格子、它的类型和它的 SNBT。

## `PaletteEntry` {#PaletteEntry}

```go
type PaletteEntry struct {
    Index uint32
    Name  string
    SNBT  string
}
```

带索引扫描里的一种不同的方块状态。

## `BlockCell` {#BlockCell}

```go
type BlockCell struct {
    X, Y, Z int32
    Index   uint32
}
```

一个格子加上调色板里的一个索引，供 `ScanRegionIndexed` 和 `SetBlocks` 使用。

## `ExplodeOptions` {#ExplodeOptions}

```go
type ExplodeOptions struct {
    MaxResistance   float32
    Source          ActorID
    Fire            bool
    BreaksBlocks    bool
    AllowUnderwater bool
}
```

爆炸的可选部分。
