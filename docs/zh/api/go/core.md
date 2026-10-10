# Go：核心：入口、日志与任务

包 levilamina 用来在 Pier 的 C ABI 上用 Go 写 LeviLamina 模组。

一个模组是一个用 `go build -buildmode=c-shared` 构建的 DLL。它实现 `Mod`，在 init 函数里调用 `Register`，`main` 留空。宿主在 Go 运行时执行完所有 init 之后才调用 `pier_main`，所以握手的时候，注册好的模组已经在了。

所有到达宿主的调用都要经过契约第 10 节的槽位关卡：调用宿主没有的槽位，会返回一个 `*NotProvidedError`，不会调用任何东西。

## 函数 {#functions}

### `Status` {#Status}

```go
func Status() (GamingStatus, error)
```

报告服务器所处的生命阶段。可以在任何 goroutine 里调用。

- 返回值类型：`(GamingStatus, error)`
- 对应槽位：[`gaming_status`](../cpp/core.md#gaming_status)

### `Schedule` {#Schedule}

```go
func Schedule(fn func()) (TaskID, error)
```

尽快在服务器线程上运行 `fn`。可以在任何 goroutine 里调用：宿主的大部分接口只能在服务器线程调用，goroutine 就是靠它回到那个线程上的。

任务属于这个模组：模组先卸载的话，宿主会丢掉这个任务，`fn` 永远不会运行，所以不会有调用进入一个已经不在的 DLL。

- 参数：
    - fn : `func()`
- 返回值类型：`(TaskID, error)`
- 对应槽位：[`schedule_for`](../cpp/core.md#schedule_for)

### `ScheduleAfter` {#ScheduleAfter}

```go
func ScheduleAfter(d time.Duration, fn func()) (TaskID, error)
```

经过 `d` 之后在服务器线程上运行 `fn`，归属规则和 `Schedule` 一样。可以在任何 goroutine 里调用。

- 参数：
    - d : `time.Duration`
    - fn : `func()`
- 返回值类型：`(TaskID, error)`
- 对应槽位：[`schedule_after_for`](../cpp/core.md#schedule_after_for)

### `Cancel` {#Cancel}

```go
func Cancel(task TaskID) (bool, error)
```

作废一个还没运行的任务。已经运行过、已经取消过，或者不属于这个模组的任务返回 false。函数本身要到进程结束才释放，所以在热路径上大量取消会造成泄漏。

- 参数：
    - task : `TaskID`
- 返回值类型：`(bool, error)`
- 对应槽位：[`schedule_cancel`](../cpp/core.md#schedule_cancel)

### `PendingTasks` {#PendingTasks}

```go
func PendingTasks() (uint32, error)
```

这个模组还没运行的任务数，`Unload` 可以检查它是否为 0。宿主数不出来时返回错误，这时那项检查拿到的是一个错误，没有拿到 0。

- 返回值类型：`(uint32, error)`
- 对应槽位：[`schedule_pending_count`](../cpp/core.md#schedule_pending_count)

### `Register` {#Register}

```go
func Register(name string, m Mod)
```

声明这个 DLL 是哪个模组。在 init 函数里调用一次；没有注册任何模组的 DLL 会拒绝加载，调用第二次会 panic，因为一个 DLL 就是一个模组。

- 参数：
    - name : `string`
    - m : `Mod`

### `IsNotProvided` {#IsNotProvided}

```go
func IsNotProvided(err error) bool
```

判断 `err` 是否表示宿主缺少某个槽位。

- 参数：
    - err : `error`
- 返回值类型：`bool`

## `Logger` {#Logger}

```go
type Logger struct{}
type Logger struct{}
```

以这个模组的名义写服务器日志。零值就能直接用，可以在任何 goroutine 里调用。

### `Logger.Error` {#Logger.Error}

```go
func (Logger) Error(msg string)
```

以 error 级别记录日志。

- 参数：
    - msg : `string`
- 对应槽位：[`log`](../cpp/core.md#log)

### `Logger.Warn` {#Logger.Warn}

```go
func (Logger) Warn(msg string)
```

以 warning 级别记录日志。

- 参数：
    - msg : `string`
- 对应槽位：[`log`](../cpp/core.md#log)

### `Logger.Info` {#Logger.Info}

```go
func (Logger) Info(msg string)
```

以 info 级别记录日志。

- 参数：
    - msg : `string`
- 对应槽位：[`log`](../cpp/core.md#log)

### `Logger.Debug` {#Logger.Debug}

```go
func (Logger) Debug(msg string)
```

以 debug 级别记录日志。

- 参数：
    - msg : `string`
- 对应槽位：[`log`](../cpp/core.md#log)

## `Context` {#Context}

```go
type Context struct{}
type Context struct{}
```

生命周期的每一步收到的东西。

### `Context.Logger` {#Context.Logger}

```go
func (c *Context) Logger() Logger
```

返回这个模组的日志器。

- 返回值类型：`Logger`

## `NotProvidedError` {#NotProvidedError}

```go
type NotProvidedError struct {
    What      string
    HostABI   uint32
    TableSize uint32
}
```

调用的槽位这个宿主没有时返回的错误：宿主比这个绑定旧，或者没有编入对应的能力包。

### `NotProvidedError.Error` {#NotProvidedError.Error}

```go
func (e *NotProvidedError) Error() string
```

- 返回值类型：`string`

## `GamingStatus` {#GamingStatus}

```go
type GamingStatus int32
```

LeviLamina 报告的服务器生命阶段。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="StatusDefault"></span>`StatusDefault` | `0` |  |
| <span id="StatusStarting"></span>`StatusStarting` | `1` |  |
| <span id="StatusRunning"></span>`StatusRunning` | `2` |  |
| <span id="StatusStopping"></span>`StatusStopping` | `3` |  |

## `TaskID` {#TaskID}

```go
type TaskID uint64
```

一个定时任务的名字，供 `Cancel` 使用。

## `Mod` {#Mod}

```go
type Mod interface {
    Enable(ctx *Context) error
    Disable(ctx *Context) error
}
```

Go 模组要实现的生命周期。两步都在服务器线程上运行；返回错误时会记录日志，并拒绝这一步。

## `Loader` {#Loader}

```go
type Loader interface {
    Load(ctx *Context) error
}
```

加载时（启用之前）有事要做的模组实现它。

## `Unloader` {#Unloader}

```go
type Unloader interface {
    Unload(ctx *Context) error
}
```

卸载时（禁用之后）有事要做的模组实现它。
