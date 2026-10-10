# Go: World

## Functions {#functions}

### `ScanRegion` {#ScanRegion}

```go
func ScanRegion(dim, x1, y1, z1, x2, y2, z2 int32) ([]BlockInfo, []EntityInfo, error)
```

ScanRegion reads every block and entity in a box. Server thread only.

- Parameters:
    - dim : `int32`
    - x1 : `int32`
    - y1 : `int32`
    - z1 : `int32`
    - x2 : `int32`
    - y2 : `int32`
    - z2 : `int32`
- Return type: `([]BlockInfo, []EntityInfo, error)`
- Slots: [`scan_region`](../cpp/world.md#scan_region)

### `ScanRegionIndexed` {#ScanRegionIndexed}

```go
func ScanRegionIndexed(dim, x1, y1, z1, x2, y2, z2 int32) ([]PaletteEntry, []BlockCell, error)
```

ScanRegionIndexed reads a box as a palette of distinct block states and the cells that index it, which is far smaller than ScanRegion for a large box. Server thread only.

- Parameters:
    - dim : `int32`
    - x1 : `int32`
    - y1 : `int32`
    - z1 : `int32`
    - x2 : `int32`
    - y2 : `int32`
    - z2 : `int32`
- Return type: `([]PaletteEntry, []BlockCell, error)`
- Slots: [`scan_region_indexed`](../cpp/world.md#scan_region_indexed)

### `GetBlock` {#GetBlock}

```go
func GetBlock(dim, x, y, z int32) (BlockInfo, error)
```

GetBlock reads the block at a cell. Server thread only.

- Parameters:
    - dim : `int32`
    - x : `int32`
    - y : `int32`
    - z : `int32`
- Return type: `(BlockInfo, error)`
- Slots: [`get_block`](../cpp/world.md#get_block)

### `Seed` {#Seed}

```go
func Seed() (int64, error)
```

Seed is the level seed.

- Return type: `(int64, error)`
- Slots: [`get_seed`](../cpp/world.md#get_seed)

### `Difficulty` {#Difficulty}

```go
func Difficulty() (int32, error)
```

Difficulty is the level difficulty, 0 peaceful to 3 hard.

- Return type: `(int32, error)`
- Slots: [`get_difficulty`](../cpp/world.md#get_difficulty)

### `SetDifficulty` {#SetDifficulty}

```go
func SetDifficulty(d int32) error
```

SetDifficulty sets the level difficulty.

- Parameters:
    - d : `int32`
- Return type: `error`
- Slots: [`set_difficulty`](../cpp/world.md#set_difficulty)

### `GameRule` {#GameRule}

```go
func GameRule(name string) (string, error)
```

GameRule reads a game rule as text.

- Parameters:
    - name : `string`
- Return type: `(string, error)`
- Slots: [`game_rule_get`](../cpp/world.md#game_rule_get)

### `SetGameRule` {#SetGameRule}

```go
func SetGameRule(name, value string) error
```

SetGameRule writes a game rule from text.

- Parameters:
    - name : `string`
    - value : `string`
- Return type: `error`
- Slots: [`game_rule_set`](../cpp/world.md#game_rule_set)

### `Time` {#Time}

```go
func Time() (int64, error)
```

Time is the time of day in ticks.

- Return type: `(int64, error)`
- Slots: [`get_time`](../cpp/world.md#get_time)

### `SetTime` {#SetTime}

```go
func SetTime(t int64) error
```

SetTime sets the time of day in ticks.

- Parameters:
    - t : `int64`
- Return type: `error`
- Slots: [`set_time`](../cpp/world.md#set_time)

### `SetWeather` {#SetWeather}

```go
func SetWeather(weather int32) error
```

SetWeather sets the weather: 0 clear, 1 rain, 2 thunder.

- Parameters:
    - weather : `int32`
- Return type: `error`
- Slots: [`set_weather`](../cpp/world.md#set_weather)

### `SaveLevel` {#SaveLevel}

```go
func SaveLevel() error
```

SaveLevel saves the level now.

- Return type: `error`
- Slots: [`level_save`](../cpp/world.md#level_save)

### `SpawnParticle` {#SpawnParticle}

```go
func SpawnParticle(dim int32, effect string, x, y, z float64) error
```

SpawnParticle shows a particle effect to everyone near it.

- Parameters:
    - dim : `int32`
    - effect : `string`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- Return type: `error`
- Slots: [`spawn_particle`](../cpp/world.md#spawn_particle)

### `Explode` {#Explode}

```go
func Explode(dim int32, x, y, z float64, radius float32, o ExplodeOptions) error
```

Explode makes an explosion of the given radius.

- Parameters:
    - dim : `int32`
    - x : `float64`
    - y : `float64`
    - z : `float64`
    - radius : `float32`
    - o : `ExplodeOptions`
- Return type: `error`
- Slots: [`explode`](../cpp/world.md#explode)

### `DefaultSpawn` {#DefaultSpawn}

```go
func DefaultSpawn() (x, y, z int32, err error)
```

DefaultSpawn is the level's world spawn.

- Return type: `(x, y, z int32, err error)`
- Slots: [`level_get_default_spawn`](../cpp/world.md#level_get_default_spawn)

### `SetDefaultSpawn` {#SetDefaultSpawn}

```go
func SetDefaultSpawn(x, y, z int32) error
```

SetDefaultSpawn moves the level's world spawn.

- Parameters:
    - x : `int32`
    - y : `int32`
    - z : `int32`
- Return type: `error`
- Slots: [`level_set_default_spawn`](../cpp/world.md#level_set_default_spawn)

### `SetBiome` {#SetBiome}

```go
func SetBiome(dim, minX, minZ, maxX, maxZ int32, biome string) (int32, error)
```

SetBiome sets the biome of every column in a rectangle and returns how many changed.

- Parameters:
    - dim : `int32`
    - minX : `int32`
    - minZ : `int32`
    - maxX : `int32`
    - maxZ : `int32`
    - biome : `string`
- Return type: `(int32, error)`
- Slots: [`level_set_biome`](../cpp/world.md#level_set_biome)

## `EntityInfo` {#EntityInfo}

```go
type EntityInfo struct {
    X, Y, Z int32
    Type    string
    SNBT    string
}
```

EntityInfo is one entity a region scan found: the cell holding it, its type and its SNBT.

## `PaletteEntry` {#PaletteEntry}

```go
type PaletteEntry struct {
    Index uint32
    Name  string
    SNBT  string
}
```

PaletteEntry is one distinct block state of an indexed scan.

## `BlockCell` {#BlockCell}

```go
type BlockCell struct {
    X, Y, Z int32
    Index   uint32
}
```

BlockCell is one cell and an index into a palette, for ScanRegionIndexed and SetBlocks.

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

ExplodeOptions are the optional parts of an explosion.
