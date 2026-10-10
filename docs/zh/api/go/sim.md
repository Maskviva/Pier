# Go：模拟玩家

## 函数 {#functions}

### `SimSpawn` {#SimSpawn}

```go
func SimSpawn(name string, dim int32, x, y, z float64) error
```

生成一个模拟玩家。

- 参数：
    - name : `string`
    - dim : `int32`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- 返回值类型：`error`
- 对应槽位：[`sim_spawn`](../cpp/sim.md#sim_spawn)

### `SimDo` {#SimDo}

```go
func SimDo(name, verb, args string) error
```

让模拟玩家执行一个动作；`args` 是 SNBT，没有参数时为 `"{}"`。

- 参数：
    - name : `string`
    - verb : `string`
    - args : `string`
- 返回值类型：`error`
- 对应槽位：[`sim_do`](../cpp/sim.md#sim_do)

### `IsSimulated` {#IsSimulated}

```go
func IsSimulated(name string) (bool, error)
```

判断 `name` 是不是一个活着的模拟玩家。

- 参数：
    - name : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`sim_is`](../cpp/sim.md#sim_is)

### `SimList` {#SimList}

```go
func SimList() ([]string, error)
```

列出活着的模拟玩家。

- 返回值类型：`([]string, error)`
- 对应槽位：[`sim_list`](../cpp/sim.md#sim_list)
