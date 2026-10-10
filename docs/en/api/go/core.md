# Go: Core: entry point, logging and tasks

Package levilamina writes LeviLamina mods in Go, on Pier's C ABI.

A mod is a DLL built with `go build -buildmode=c-shared`. It implements Mod, calls Register from an init function, and has an empty main. The host calls `pier_main` after the Go runtime has run every init, so the registered mod is there when the handshake asks.

Everything that reaches the host goes through the slot gates of contract section 10: a call to a slot the host does not have returns a \*NotProvidedError without calling anything.

## Functions {#functions}

### `Status` {#Status}

```go
func Status() (GamingStatus, error)
```

Status reports the server's life stage. Safe from any goroutine.

- Return type: `(GamingStatus, error)`
- Slots: [`gaming_status`](../cpp/core.md#gaming_status)

### `Schedule` {#Schedule}

```go
func Schedule(fn func()) (TaskID, error)
```

Schedule runs fn on the server thread as soon as it can. Safe from any goroutine: most of the host is server-thread only, and this is how a goroutine gets back onto that thread.

The task belongs to this mod: if the mod unloads first, the host drops it and fn never runs, so nothing calls into a DLL that is gone.

- Parameters:
    - fn : `func()`
- Return type: `(TaskID, error)`
- Slots: [`schedule_for`](../cpp/core.md#schedule_for)

### `ScheduleAfter` {#ScheduleAfter}

```go
func ScheduleAfter(d time.Duration, fn func()) (TaskID, error)
```

ScheduleAfter runs fn on the server thread once d has passed, under the same ownership as Schedule. Safe from any goroutine.

- Parameters:
    - d : `time.Duration`
    - fn : `func()`
- Return type: `(TaskID, error)`
- Slots: [`schedule_after_for`](../cpp/core.md#schedule_after_for)

### `Cancel` {#Cancel}

```go
func Cancel(task TaskID) (bool, error)
```

Cancel voids a task that has not run. It reports false for a task that already ran, was cancelled, or is not this mod's. The function itself is released when the process ends, so cancelling in bulk on a hot path leaks.

- Parameters:
    - task : `TaskID`
- Return type: `(bool, error)`
- Slots: [`schedule_cancel`](../cpp/core.md#schedule_cancel)

### `PendingTasks` {#PendingTasks}

```go
func PendingTasks() (uint32, error)
```

PendingTasks counts this mod's tasks that have not run, which an Unload can check is 0. A host that cannot count returns an error, which that check receives in place of a 0.

- Return type: `(uint32, error)`
- Slots: [`schedule_pending_count`](../cpp/core.md#schedule_pending_count)

### `Register` {#Register}

```go
func Register(name string, m Mod)
```

Register names the mod this DLL is. Call it once, from an init function; a DLL with nothing registered refuses to load, and a second call panics, since a DLL is one mod.

- Parameters:
    - name : `string`
    - m : `Mod`

### `IsNotProvided` {#IsNotProvided}

```go
func IsNotProvided(err error) bool
```

IsNotProvided reports whether err says the host lacks a slot.

- Parameters:
    - err : `error`
- Return type: `bool`

## `Logger` {#Logger}

```go
type Logger struct{}
type Logger struct{}
```

Logger writes to the server log under this mod's name. The zero value is ready, and it is safe from any goroutine.

### `Logger.Error` {#Logger.Error}

```go
func (Logger) Error(msg string)
```

Error logs at error level.

- Parameters:
    - msg : `string`
- Slots: [`log`](../cpp/core.md#log)

### `Logger.Warn` {#Logger.Warn}

```go
func (Logger) Warn(msg string)
```

Warn logs at warning level.

- Parameters:
    - msg : `string`
- Slots: [`log`](../cpp/core.md#log)

### `Logger.Info` {#Logger.Info}

```go
func (Logger) Info(msg string)
```

Info logs at info level.

- Parameters:
    - msg : `string`
- Slots: [`log`](../cpp/core.md#log)

### `Logger.Debug` {#Logger.Debug}

```go
func (Logger) Debug(msg string)
```

Debug logs at debug level.

- Parameters:
    - msg : `string`
- Slots: [`log`](../cpp/core.md#log)

## `Context` {#Context}

```go
type Context struct{}
type Context struct{}
```

Context is what a lifecycle step receives.

### `Context.Logger` {#Context.Logger}

```go
func (c *Context) Logger() Logger
```

Logger returns the logger of this mod.

- Return type: `Logger`

## `NotProvidedError` {#NotProvidedError}

```go
type NotProvidedError struct {
    What      string
    HostABI   uint32
    TableSize uint32
}
```

NotProvidedError is the error of a call whose slot this host does not have: the host is older than this binding, or the capability package was not built into it.

### `NotProvidedError.Error` {#NotProvidedError.Error}

```go
func (e *NotProvidedError) Error() string
```

- Return type: `string`

## `GamingStatus` {#GamingStatus}

```go
type GamingStatus int32
```

GamingStatus is the server's life stage, as LeviLamina reports it.

| Name | Value | Description |
|---|---|---|
| <span id="StatusDefault"></span>`StatusDefault` | `0` |  |
| <span id="StatusStarting"></span>`StatusStarting` | `1` |  |
| <span id="StatusRunning"></span>`StatusRunning` | `2` |  |
| <span id="StatusStopping"></span>`StatusStopping` | `3` |  |

## `TaskID` {#TaskID}

```go
type TaskID uint64
```

TaskID names a scheduled task, for Cancel.

## `Mod` {#Mod}

```go
type Mod interface {
    Enable(ctx *Context) error
    Disable(ctx *Context) error
}
```

Mod is the lifecycle a Go mod implements. Both steps run on the server thread, and a returned error is logged and refuses the step.

## `Loader` {#Loader}

```go
type Loader interface {
    Load(ctx *Context) error
}
```

Loader is implemented by a mod with work to do at load, before it is enabled.

## `Unloader` {#Unloader}

```go
type Unloader interface {
    Unload(ctx *Context) error
}
```

Unloader is implemented by a mod with work to do at unload, after it is disabled.
