# Zig: Core: entry point, logging and tasks

core.zig: the handshake, the slot gate, strings, logging, tasks, events, commands and the cross-mod channels of the Zig binding.

Zig calls the host's function pointers directly, so there is no generated layer between: `slot("name")` is the function pointer of that slot, gated by both checks of contract section 10, and every other function here is built on it.

## Functions {#functions}

### `slot` {#slot}

```zig
pub fn slot(comptime name: []const u8) @FieldType(c.PierApi, name)
```

The function pointer of slot `name`, or null when the host's table is too short to hold it or the slot is NULL. Every slot of abi.h is reachable this way.

- Parameters:
    - name : `comptime []const u8`
- Return type: `@FieldType(c.PierApi, name)`

### `handle` {#handle}

```zig
pub fn handle() c.PierModHandle
```

The caller's own mod handle, for a slot that takes one.

- Return type: `c.PierModHandle`

### `str` {#str}

```zig
pub fn str(text: []const u8) c.PierStr
```

A Zig slice as a PierStr; the host reads it during the call only.

- Parameters:
    - text : `[]const u8`
- Return type: `c.PierStr`

### `view` {#view}

```zig
pub fn view(s: c.PierStr) []const u8
```

A PierStr as a slice, valid only during the callback that received it.

- Parameters:
    - s : `c.PierStr`
- Return type: `[]const u8`

### `log` {#log}

```zig
pub fn log(level: Level, msg: []const u8) void
```

Writes one line through this mod's LeviLamina logger. Safe from any thread.

- Parameters:
    - level : `Level`
    - msg : `[]const u8`
- Return type: `void`
- Slots: [`log`](../cpp/core.md#log)

### `logf` {#logf}

```zig
pub fn logf(level: Level, comptime fmt: []const u8, args: anytype) void
```

log with std.fmt formatting, into a buffer of 2048 bytes.

- Parameters:
    - level : `Level`
    - fmt : `comptime []const u8`
    - args : `anytype`
- Return type: `void`
- Slots: [`log`](../cpp/core.md#log)

### `exportMod` {#exportMod}

```zig
pub fn exportMod(comptime M: type, comptime name: []const u8) void
```

Exports `pier_main` for mod type M, named `name` in the log. Call it from the root file:

```text
comptime { levilamina.exportMod(MyMod, "my-mod"); }
```

M declares any of `pub fn load`, `enable`, `disable` and `unload`, each taking a `*levilamina.Context` and returning `!void`; an error is logged and refuses the step.

- Parameters:
    - M : `comptime type`
    - name : `comptime []const u8`
- Return type: `void`
- Slots: [`log`](../cpp/core.md#log)

### `schedule` {#schedule}

```zig
pub fn schedule(comptime task: fn () void) Error!TaskId
```

Runs `task` on the server thread as soon as it can. Safe from any thread. The task belongs to this mod: if the mod unloads first, the host drops it and it never runs.

- Parameters:
    - task : `comptime fn () void`
- Return type: `Error!TaskId`
- Slots: [`schedule_for`](../cpp/core.md#schedule_for)

### `scheduleAfter` {#scheduleAfter}

```zig
pub fn scheduleAfter(delay_ms: u64, comptime task: fn () void) Error!TaskId
```

Runs `task` on the server thread after `delay_ms`, under the ownership of schedule.

- Parameters:
    - delay_ms : `u64`
    - task : `comptime fn () void`
- Return type: `Error!TaskId`
- Slots: [`schedule_after_for`](../cpp/core.md#schedule_after_for)

### `cancel` {#cancel}

```zig
pub fn cancel(id: TaskId) Error!bool
```

Voids a task that has not run; false for one that ran, was cancelled or is not this mod's.

- Parameters:
    - id : `TaskId`
- Return type: `Error!bool`
- Slots: [`schedule_cancel`](../cpp/core.md#schedule_cancel)

### `pendingTasks` {#pendingTasks}

```zig
pub fn pendingTasks() Error!u32
```

This mod's tasks that have not run. A host that cannot count returns an error.

- Return type: `Error!u32`
- Slots: [`schedule_pending_count`](../cpp/core.md#schedule_pending_count)

## Declarations {#declarations}

### `c` {#c}

```zig
pub const c = @import("abi");
```

The ABI as translate-c read it from abi.h: every type, slot and constant.

### `TaskId` {#TaskId}

```zig
pub const TaskId = u64;
```

A task id, for cancel.

## `Collector` {#Collector}

```zig
pub const Collector = struct {
    allocator: std.mem.Allocator,
    items: std.ArrayList([]u8) = .empty,
    failed: bool = false,
    // ...
};
```

Gathers what a PierStrSink receives during one call, each piece copied with allocator.

### `Collector.init` {#Collector.init}

```zig
pub fn init(allocator: std.mem.Allocator) Collector
```

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `Collector`

### `Collector.deinit` {#Collector.deinit}

```zig
pub fn deinit(self: *Collector) void
```

- Return type: `void`

### `Collector.sink` {#Collector.sink}

```zig
pub fn sink(ctx: ?*anyopaque, s: c.PierStr) callconv(.c) void
```

The PierStrSink to hand the host, with a \*Collector as its ctx.

- Parameters:
    - ctx : `?*anyopaque`
    - s : `c.PierStr`
- Return type: `callconv(.c) void`

### `Collector.take` {#Collector.take}

```zig
pub fn take(self: *Collector) Error![]u8
```

The last piece, owned by the caller; NoAnswer when the sink received nothing.

- Return type: `Error![]u8`

### `Collector.takeOrEmpty` {#Collector.takeOrEmpty}

```zig
pub fn takeOrEmpty(self: *Collector) Error![]u8
```

The last piece, or an empty owned slice when the sink received nothing.

- Return type: `Error![]u8`

### `Collector.takeAll` {#Collector.takeAll}

```zig
pub fn takeAll(self: *Collector) Error![][]u8
```

Every piece, owned by the caller with the slice holding them.

- Return type: `Error![][]u8`

## `Context` {#Context}

```zig
pub const Context = struct { ... };
```

What a lifecycle step receives.

### `Context.info` {#Context.info}

```zig
pub fn info(_: *Context, msg: []const u8) void
```

- Parameters:
    - _ : `*Context`
    - msg : `[]const u8`
- Return type: `void`
- Slots: [`log`](../cpp/core.md#log)

### `Context.warn` {#Context.warn}

```zig
pub fn warn(_: *Context, msg: []const u8) void
```

- Parameters:
    - _ : `*Context`
    - msg : `[]const u8`
- Return type: `void`
- Slots: [`log`](../cpp/core.md#log)

### `Context.err` {#Context.err}

```zig
pub fn err(_: *Context, msg: []const u8) void
```

- Parameters:
    - _ : `*Context`
    - msg : `[]const u8`
- Return type: `void`
- Slots: [`log`](../cpp/core.md#log)

## `Error` {#Error}

```zig
pub const Error = error{
    /// The host has no such slot: it is older than this binding or lacks the capability.
    NotProvided,
    /// The host ran the call and refused it.
    Refused,
    /// The host has no answer for the read.
    NoAnswer,
    OutOfMemory,
};
```

Why a call failed. A read the host cannot answer returns NoAnswer.

## `Level` {#Level}

```zig
pub const Level = enum(i32) {
    fatal = 0, err = 1, warn = 2, info = 3, debug = 4, trace = 5
};
```

The levels of the log slot, which mirror `ll::io::LogLevel`.
