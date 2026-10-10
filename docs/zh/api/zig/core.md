# Zig：核心：入口、日志与任务

core.zig：Zig 绑定的握手、槽位关卡、字符串、日志、任务、事件、命令和跨模组通道。

Zig 直接调用宿主的函数指针，中间没有生成的那一层：`slot("name")` 就是那个槽位的函数指针，经过契约第 10 节的两道检查，这里的其他函数都建立在它之上。

## 函数 {#functions}

### `slot` {#slot}

```zig
pub fn slot(comptime name: []const u8) @FieldType(c.PierApi, name)
```

名为 `name` 的槽位的函数指针；宿主的表太短、放不下这个槽位，或者槽位为 NULL 时返回 null。abi.h 的每一个槽位都可以这样取到。

- 参数：
    - name : `comptime []const u8`
- 返回值类型：`@FieldType(c.PierApi, name)`

### `handle` {#handle}

```zig
pub fn handle() c.PierModHandle
```

调用方自己的模组句柄，供需要它的槽位使用。

- 返回值类型：`c.PierModHandle`

### `str` {#str}

```zig
pub fn str(text: []const u8) c.PierStr
```

把 Zig 切片当作 `PierStr`；宿主只在调用期间读取它。

- 参数：
    - text : `[]const u8`
- 返回值类型：`c.PierStr`

### `view` {#view}

```zig
pub fn view(s: c.PierStr) []const u8
```

把 `PierStr` 当作切片，只在收到它的那次回调期间有效。

- 参数：
    - s : `c.PierStr`
- 返回值类型：`[]const u8`

### `log` {#log}

```zig
pub fn log(level: Level, msg: []const u8) void
```

通过这个模组的 LeviLamina 日志器写一行。可以在任何线程调用。

- 参数：
    - level : `Level`
    - msg : `[]const u8`
- 返回值类型：`void`
- 对应槽位：[`log`](../cpp/core.md#log)

### `logf` {#logf}

```zig
pub fn logf(level: Level, comptime fmt: []const u8, args: anytype) void
```

用 `std.fmt` 格式化的 `log`，写进一个 2048 字节的缓冲区。

- 参数：
    - level : `Level`
    - fmt : `comptime []const u8`
    - args : `anytype`
- 返回值类型：`void`
- 对应槽位：[`log`](../cpp/core.md#log)

### `exportMod` {#exportMod}

```zig
pub fn exportMod(comptime M: type, comptime name: []const u8) void
```

为模组类型 `M` 导出 `pier_main`，在日志里以 `name` 命名。在根文件里调用：

```zig
comptime { levilamina.exportMod(MyMod, "my-mod"); }
```

`M` 可以声明 `pub fn load`、`enable`、`disable` 和 `unload` 中的任意几个，每个都接收 `*levilamina.Context`，返回 `!void`；返回错误时会记录日志，并拒绝这一步。

- 参数：
    - M : `comptime type`
    - name : `comptime []const u8`
- 返回值类型：`void`
- 对应槽位：[`log`](../cpp/core.md#log)

### `schedule` {#schedule}

```zig
pub fn schedule(comptime task: fn () void) Error!TaskId
```

尽快在服务器线程上运行 `task`。可以在任何线程调用。任务属于这个模组：模组先卸载的话，宿主会丢掉这个任务，它永远不会运行。

- 参数：
    - task : `comptime fn () void`
- 返回值类型：`Error!TaskId`
- 对应槽位：[`schedule_for`](../cpp/core.md#schedule_for)

### `scheduleAfter` {#scheduleAfter}

```zig
pub fn scheduleAfter(delay_ms: u64, comptime task: fn () void) Error!TaskId
```

在 `delay_ms` 毫秒后于服务器线程上运行 `task`，归属规则和 `schedule` 一样。

- 参数：
    - delay_ms : `u64`
    - task : `comptime fn () void`
- 返回值类型：`Error!TaskId`
- 对应槽位：[`schedule_after_for`](../cpp/core.md#schedule_after_for)

### `cancel` {#cancel}

```zig
pub fn cancel(id: TaskId) Error!bool
```

作废一个还没运行的任务；已经运行过、已经取消过，或者不属于这个模组的任务返回 false。

- 参数：
    - id : `TaskId`
- 返回值类型：`Error!bool`
- 对应槽位：[`schedule_cancel`](../cpp/core.md#schedule_cancel)

### `pendingTasks` {#pendingTasks}

```zig
pub fn pendingTasks() Error!u32
```

这个模组还没运行的任务数。宿主数不出来时返回错误。

- 返回值类型：`Error!u32`
- 对应槽位：[`schedule_pending_count`](../cpp/core.md#schedule_pending_count)

## 声明 {#declarations}

### `c` {#c}

```zig
pub const c = @import("abi");
```

translate-c 从 abi.h 读出来的 ABI：所有的类型、槽位和常量。

### `TaskId` {#TaskId}

```zig
pub const TaskId = u64;
```

任务 id，供 `cancel` 使用。

## `Collector` {#Collector}

```zig
pub const Collector = struct {
    allocator: std.mem.Allocator,
    items: std.ArrayList([]u8) = .empty,
    failed: bool = false,
    // ...
};
```

收集一次调用期间 `PierStrSink` 收到的内容，每一段都用 `allocator` 复制一份。

### `Collector.init` {#Collector.init}

```zig
pub fn init(allocator: std.mem.Allocator) Collector
```

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`Collector`

### `Collector.deinit` {#Collector.deinit}

```zig
pub fn deinit(self: *Collector) void
```

- 返回值类型：`void`

### `Collector.sink` {#Collector.sink}

```zig
pub fn sink(ctx: ?*anyopaque, s: c.PierStr) callconv(.c) void
```

交给宿主的 `PierStrSink`，`ctx` 是一个 `*Collector`。

- 参数：
    - ctx : `?*anyopaque`
    - s : `c.PierStr`
- 返回值类型：`callconv(.c) void`

### `Collector.take` {#Collector.take}

```zig
pub fn take(self: *Collector) Error![]u8
```

最后一段，归调用方所有；输出回调什么都没收到时返回 `NoAnswer`。

- 返回值类型：`Error![]u8`

### `Collector.takeOrEmpty` {#Collector.takeOrEmpty}

```zig
pub fn takeOrEmpty(self: *Collector) Error![]u8
```

最后一段；输出回调什么都没收到时，返回一个归调用方所有的空切片。

- 返回值类型：`Error![]u8`

### `Collector.takeAll` {#Collector.takeAll}

```zig
pub fn takeAll(self: *Collector) Error![][]u8
```

所有的段，连同装着它们的切片，都归调用方所有。

- 返回值类型：`Error![][]u8`

## `Context` {#Context}

```zig
pub const Context = struct { ... };
```

生命周期的每一步收到的东西。

### `Context.info` {#Context.info}

```zig
pub fn info(_: *Context, msg: []const u8) void
```

- 参数：
    - _ : `*Context`
    - msg : `[]const u8`
- 返回值类型：`void`
- 对应槽位：[`log`](../cpp/core.md#log)

### `Context.warn` {#Context.warn}

```zig
pub fn warn(_: *Context, msg: []const u8) void
```

- 参数：
    - _ : `*Context`
    - msg : `[]const u8`
- 返回值类型：`void`
- 对应槽位：[`log`](../cpp/core.md#log)

### `Context.err` {#Context.err}

```zig
pub fn err(_: *Context, msg: []const u8) void
```

- 参数：
    - _ : `*Context`
    - msg : `[]const u8`
- 返回值类型：`void`
- 对应槽位：[`log`](../cpp/core.md#log)

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

调用失败的原因。宿主答不上来的读取返回 `NoAnswer`。

## `Level` {#Level}

```zig
pub const Level = enum(i32) {
    fatal = 0, err = 1, warn = 2, info = 3, debug = 4, trace = 5
};
```

`log` 槽位的日志级别，对应 `ll::io::LogLevel`。
