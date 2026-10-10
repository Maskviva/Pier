# Zig：命令

## 函数 {#functions}

### `registerCommand` {#registerCommand}

```zig
pub fn registerCommand(name: []const u8, description: []const u8, permission: Permission, comptime handler: fn (*const Invocation) void) Error!void
```

注册 `/name`，命令名之后的文本作为 `args` 交给 `handler`。请在 `enable` 里调用。

- 参数：
    - name : `[]const u8`
    - description : `[]const u8`
    - permission : `Permission`
    - handler : `comptime fn (*const Invocation) void`
- 返回值类型：`Error!void`
- 对应槽位：[`register_command`](../cpp/commands.md#register_command)

### `executeCommand` {#executeCommand}

```zig
pub fn executeCommand(cmd: []const u8) Error!void
```

以控制台的身份运行 `cmd`。只能在服务器线程调用。输出的每一行按 debug 级别记进日志；宿主返回 false（世界还没就绪）时得到 `Refused`。

- 参数：
    - cmd : `[]const u8`
- 返回值类型：`Error!void`
- 对应槽位：[`execute_command`](../cpp/commands.md#execute_command)、[`log`](../cpp/core.md#log)

## `Invocation` {#Invocation}

```zig
pub const Invocation = struct {
    args: []const u8,
    origin: []const u8,
    ctx: ?*anyopaque,
    ok: c.PierStrSink,
    fail: c.PierStrSink,
    // ...
};
```

一次命令的执行；只在回调期间有效。

### `Invocation.success` {#Invocation.success}

```zig
pub fn success(self: *const Invocation, msg: []const u8) void
```

发送一行输出。

- 参数：
    - msg : `[]const u8`
- 返回值类型：`void`

### `Invocation.failure` {#Invocation.failure}

```zig
pub fn failure(self: *const Invocation, msg: []const u8) void
```

发送一行错误输出。

- 参数：
    - msg : `[]const u8`
- 返回值类型：`void`

## `Permission` {#Permission}

```zig
pub const Permission = enum(i32) {
    any = 0, game_directors = 1, admin = 2, host = 3, owner = 4
};
```

谁可以运行一条命令，对应 `CommandPermissionLevel`。
