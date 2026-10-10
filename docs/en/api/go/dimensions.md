# Go: Custom dimensions

## Functions {#functions}

### `AddGeneratedDimension` {#AddGeneratedDimension}

```go
func AddGeneratedDimension(name, spec string, materials, biomes []string, fill func(*ChunkRequest) bool) (int32, error)
```

AddGeneratedDimension registers a dimension whose terrain fill writes, and returns its id.

fill runs on the host's chunk worker threads, several at once: it must be safe to run concurrently, give the same answer for the same chunk forever, and call nothing on this package but the request itself. Returning false leaves the chunk to air. materials\[0\] must be "minecraft:air". The function is kept for the dimension's whole life.

- Parameters:
    - name : `string`
    - spec : `string`
    - materials : `[]string`
    - biomes : `[]string`
    - fill : `func(*ChunkRequest) bool`
- Return type: `(int32, error)`
- Slots: [`md_add_dimension_generated`](../cpp/dimensions.md#md_add_dimension_generated)

### `DimensionsAvailable` {#DimensionsAvailable}

```go
func DimensionsAvailable() (bool, error)
```

DimensionsAvailable reports whether this host can register custom dimensions.

- Return type: `(bool, error)`
- Slots: [`md_is_available`](../cpp/dimensions.md#md_is_available)

### `AddDimension` {#AddDimension}

```go
func AddDimension(name, spec string) (int32, error)
```

AddDimension registers a dimension from spec SNBT and returns its id.

- Parameters:
    - name : `string`
    - spec : `string`
- Return type: `(int32, error)`
- Slots: [`md_add_dimension`](../cpp/dimensions.md#md_add_dimension)

### `DimensionID` {#DimensionID}

```go
func DimensionID(name string) (int32, error)
```

DimensionID is the id of a registered dimension.

- Parameters:
    - name : `string`
- Return type: `(int32, error)`
- Slots: [`md_get_dimension_id`](../cpp/dimensions.md#md_get_dimension_id)

### `ListDimensions` {#ListDimensions}

```go
func ListDimensions() ([]string, error)
```

ListDimensions lists the registered custom dimensions.

- Return type: `([]string, error)`
- Slots: [`md_list_dimensions`](../cpp/dimensions.md#md_list_dimensions)

### `SetDimensionRule` {#SetDimensionRule}

```go
func SetDimensionRule(dim, rule int32, allow bool) error
```

SetDimensionRule sets one rule, a PIER\_DIMRULE\_\* value, for one dimension.

- Parameters:
    - dim : `int32`
    - rule : `int32`
    - allow : `bool`
- Return type: `error`
- Slots: [`md_set_dimension_rule`](../cpp/dimensions.md#md_set_dimension_rule)

### `DimensionRule` {#DimensionRule}

```go
func DimensionRule(dim, rule int32) (allow, set bool, err error)
```

DimensionRule reads one rule; set is false when the dimension follows vanilla for it, which a host older than the rule also answers.

- Parameters:
    - dim : `int32`
    - rule : `int32`
- Return type: `(allow, set bool, err error)`
- Slots: [`md_get_dimension_rule`](../cpp/dimensions.md#md_get_dimension_rule)

## `ChunkRequest` {#ChunkRequest}

```go
type ChunkRequest struct {
    Dim, ChunkX, ChunkZ int32
    MinY, Height        int32
    Materials           []uint16
    Biomes              []uint16
}
```

ChunkRequest is one chunk a supplied-terrain generator fills. Materials has 256\*Height entries indexed (x\*16+z)\*Height + (y-MinY), and Biomes 256, one per column; both hold indices into the palettes given at registration, and material 0 is air.
