# Go：批量编辑世界

## 函数 {#functions}

### `SetBlocks` {#SetBlocks}

```go
func SetBlocks(dim int32, palette []string, cells []BlockCell, updateFlags int32) (int64, error)
```

一次调用写入很多方块：`palette` 里是方块描述，每一格用索引指定其中一个。返回改变了多少格。只能在服务器线程调用。

- 参数：
    - dim : `int32`
    - palette : `[]string`
    - cells : `[]BlockCell`
    - updateFlags : `int32`
- 返回值类型：`(int64, error)`
- 对应槽位：[`edit_set_blocks`](../cpp/edit.md#edit_set_blocks)

### `FillRegion` {#FillRegion}

```go
func FillRegion(dim, x1, y1, z1, x2, y2, z2 int32, blockSpec string, updateFlags int32) (int64, error)
```

用一种方块填满一个长方体，返回改变了多少格。

- 参数：
    - dim : `int32`
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
