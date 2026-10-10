# Go: Simulated players

## Functions {#functions}

### `SimSpawn` {#SimSpawn}

```go
func SimSpawn(name string, dim int32, x, y, z float64) error
```

SimSpawn spawns a simulated player.

- Parameters:
    - name : `string`
    - dim : `int32`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- Return type: `error`
- Slots: [`sim_spawn`](../cpp/sim.md#sim_spawn)

### `SimDo` {#SimDo}

```go
func SimDo(name, verb, args string) error
```

SimDo runs one verb on a simulated player; args is SNBT, "{}" when there is none.

- Parameters:
    - name : `string`
    - verb : `string`
    - args : `string`
- Return type: `error`
- Slots: [`sim_do`](../cpp/sim.md#sim_do)

### `IsSimulated` {#IsSimulated}

```go
func IsSimulated(name string) (bool, error)
```

IsSimulated reports whether name is a live simulated player.

- Parameters:
    - name : `string`
- Return type: `(bool, error)`
- Slots: [`sim_is`](../cpp/sim.md#sim_is)

### `SimList` {#SimList}

```go
func SimList() ([]string, error)
```

SimList lists the live simulated players.

- Return type: `([]string, error)`
- Slots: [`sim_list`](../cpp/sim.md#sim_list)
