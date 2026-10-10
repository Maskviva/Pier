# Go：自定义维度

## 函数 {#functions}

### `AddGeneratedDimension` {#AddGeneratedDimension}

```go
func AddGeneratedDimension(name, spec string, materials, biomes []string, fill func(*ChunkRequest) bool) (int32, error)
```

注册一个由 `fill` 写入地形的维度，返回它的 id。

`fill` 在宿主的区块工作线程上运行，同时可能有好几个：它必须能并发运行，对同一个区块永远给出同样的结果，并且除了请求本身，不调用这个包里的任何东西。返回 false 时，这个区块留给空气。`materials[0]` 必须是 `"minecraft:air"`。这个函数在维度的整个生命期里都会被保留。

- 参数：
    - name : `string`
    - spec : `string`
    - materials : `[]string`
    - biomes : `[]string`
    - fill : `func(*ChunkRequest) bool`
- 返回值类型：`(int32, error)`
- 对应槽位：[`md_add_dimension_generated`](../cpp/dimensions.md#md_add_dimension_generated)

### `DimensionsAvailable` {#DimensionsAvailable}

```go
func DimensionsAvailable() (bool, error)
```

判断这个宿主能不能注册自定义维度。

- 返回值类型：`(bool, error)`
- 对应槽位：[`md_is_available`](../cpp/dimensions.md#md_is_available)

### `AddDimension` {#AddDimension}

```go
func AddDimension(name, spec string) (int32, error)
```

用描述 SNBT 注册一个维度，返回它的 id。

- 参数：
    - name : `string`
    - spec : `string`
- 返回值类型：`(int32, error)`
- 对应槽位：[`md_add_dimension`](../cpp/dimensions.md#md_add_dimension)

### `DimensionID` {#DimensionID}

```go
func DimensionID(name string) (int32, error)
```

一个已注册维度的 id。

- 参数：
    - name : `string`
- 返回值类型：`(int32, error)`
- 对应槽位：[`md_get_dimension_id`](../cpp/dimensions.md#md_get_dimension_id)

### `ListDimensions` {#ListDimensions}

```go
func ListDimensions() ([]string, error)
```

列出已注册的自定义维度。

- 返回值类型：`([]string, error)`
- 对应槽位：[`md_list_dimensions`](../cpp/dimensions.md#md_list_dimensions)

### `SetDimensionRule` {#SetDimensionRule}

```go
func SetDimensionRule(dim, rule int32, allow bool) error
```

为一个维度设置一条规则，规则是某个 `PIER_DIMRULE_*` 值。

- 参数：
    - dim : `int32`
    - rule : `int32`
    - allow : `bool`
- 返回值类型：`error`
- 对应槽位：[`md_set_dimension_rule`](../cpp/dimensions.md#md_set_dimension_rule)

### `DimensionRule` {#DimensionRule}

```go
func DimensionRule(dim, rule int32) (allow, set bool, err error)
```

读取一条规则；这个维度在这条规则上沿用原版行为时 `set` 为 false，比这条规则更旧的宿主也这样回答。

- 参数：
    - dim : `int32`
    - rule : `int32`
- 返回值类型：`(allow, set bool, err error)`
- 对应槽位：[`md_get_dimension_rule`](../cpp/dimensions.md#md_get_dimension_rule)

## `ChunkRequest` {#ChunkRequest}

```go
type ChunkRequest struct {
    Dim, ChunkX, ChunkZ int32
    MinY, Height        int32
    Materials           []uint16
    Biomes              []uint16
}
```

自供地形的生成器要填充的一个区块。`Materials` 有 `256*Height` 项，下标是 `(x*16+z)*Height + (y-MinY)`；`Biomes` 有 256 项，每列一项；两者存的都是注册时给出的调色板里的索引，材料 0 是空气。
