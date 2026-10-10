# Go: Bulk world editing

## Functions {#functions}

### `SetBlocks` {#SetBlocks}

```go
func SetBlocks(dim int32, palette []string, cells []BlockCell, updateFlags int32) (int64, error)
```

SetBlocks writes many blocks in one call: palette holds block specs and each cell names one by index. It returns how many cells changed. Server thread only.

- Parameters:
    - dim : `int32`
    - palette : `[]string`
    - cells : `[]BlockCell`
    - updateFlags : `int32`
- Return type: `(int64, error)`
- Slots: [`edit_set_blocks`](../cpp/edit.md#edit_set_blocks)

### `FillRegion` {#FillRegion}

```go
func FillRegion(dim, x1, y1, z1, x2, y2, z2 int32, blockSpec string, updateFlags int32) (int64, error)
```

FillRegion fills a box with one block and returns how many cells changed.

- Parameters:
    - dim : `int32`
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
