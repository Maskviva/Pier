# Go: Scoreboard

## Functions {#functions}

### `Scoreboard` {#Scoreboard}

```go
func Scoreboard(op int32, a, b string, n int64) (string, error)
```

Scoreboard runs one PIER\_SB\_\* operation and returns its output, when it has one.

- Parameters:
    - op : `int32`
    - a : `string`
    - b : `string`
    - n : `int64`
- Return type: `(string, error)`
- Slots: [`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)
