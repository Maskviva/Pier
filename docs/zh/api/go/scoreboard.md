# Go：计分板

## 函数 {#functions}

### `Scoreboard` {#Scoreboard}

```go
func Scoreboard(op int32, a, b string, n int64) (string, error)
```

执行一个 `PIER_SB_*` 操作，有输出时返回输出。

- 参数：
    - op : `int32`
    - a : `string`
    - b : `string`
    - n : `int64`
- 返回值类型：`(string, error)`
- 对应槽位：[`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)
