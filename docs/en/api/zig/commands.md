# Zig: Commands

## Functions {#functions}

### `registerCommand` {#registerCommand}

```zig
pub fn registerCommand(name: []const u8, description: []const u8, permission: Permission, comptime handler: fn (*const Invocation) void) Error!void
```

Registers /name, whose text after the name reaches handler as args. Call it from enable.

- Parameters:
    - name : `[]const u8`
    - description : `[]const u8`
    - permission : `Permission`
    - handler : `comptime fn (*const Invocation) void`
- Return type: `Error!void`
- Slots: [`register_command`](../cpp/commands.md#register_command)

### `executeCommand` {#executeCommand}

```zig
pub fn executeCommand(cmd: []const u8) Error!void
```

Runs cmd as the console. Server thread only. The output lines are logged at debug level; a false from the host, a level that is not ready, is Refused.

- Parameters:
    - cmd : `[]const u8`
- Return type: `Error!void`
- Slots: [`execute_command`](../cpp/commands.md#execute_command), [`log`](../cpp/core.md#log)

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

One run of a command; valid during the callback only.

### `Invocation.success` {#Invocation.success}

```zig
pub fn success(self: *const Invocation, msg: []const u8) void
```

Sends a line of output.

- Parameters:
    - msg : `[]const u8`
- Return type: `void`

### `Invocation.failure` {#Invocation.failure}

```zig
pub fn failure(self: *const Invocation, msg: []const u8) void
```

Sends a line of error output.

- Parameters:
    - msg : `[]const u8`
- Return type: `void`

## `Permission` {#Permission}

```zig
pub const Permission = enum(i32) {
    any = 0, game_directors = 1, admin = 2, host = 3, owner = 4
};
```

Who may run a command, mirroring CommandPermissionLevel.
