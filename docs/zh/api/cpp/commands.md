# 命令

??? note "abi.h 里的分节说明"

    **核心槽位，从 ABI v1 起就有（`the core slots, present since ABI v1`）**

    **§H 带参数的命令与枚举（`§H parameterized commands & enums`）**

## 槽位 {#slots}

### `execute_command` {#execute_command}

```c
bool (*execute_command)(PierStr cmd, void* ctx, PierCmdOutputSink sink);
```

以服务器控制台的身份（权限：Owner）执行一条命令，并收集它的输出。只能在服务器线程调用。世界还没就绪时返回 false。

- 调用形式：`api->execute_command(cmd, ctx, sink)`
- 参数：
    - cmd : `PierStr`
    - ctx : `void*`
    - sink : `PierCmdOutputSink`
- 返回值类型：`bool`
- 所在分节：核心槽位，从 ABI v1 起就有（`the core slots, present since ABI v1`）
- 表内序号：第 7 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Host::execute_command`](../rust/host.md#Host.execute_command)
    - Go：[`ExecuteCommand`](../go/commands.md#ExecuteCommand)
    - Zig：[`executeCommand`](../zig/commands.md#executeCommand)

### `register_command` {#register_command}

```c
bool (*register_command)(
    PierModHandle mod,
    PierStr name,
    PierStr description,
    int32_t permission,
    PierCommandCb cb,
    void* user
);
```

注册一条自定义命令 `/name [args: 原始文本]`。

- `permission`：0=Any，1=GameDirectors，2=Admin，3=Host，4=Owner（对应 `CommandPermissionLevel`）。

在 `on_enable` 里、在服务器线程上调用。命令在服务器整个运行期间都保持注册（基岩版不能注销命令）；被禁用的模组，它的回调由加载器屏蔽。

- 调用形式：`api->register_command(mod, name, description, permission, cb, user)`
- 参数：
    - mod : `PierModHandle`
    - name : `PierStr`
    - description : `PierStr`
    - permission : `int32_t`
    - cb : `PierCommandCb`
    - user : `void*`
- 返回值类型：`bool`
- 所在分节：核心槽位，从 ABI v1 起就有（`the core slots, present since ABI v1`）
- 表内序号：第 8 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`command::register`](../rust/command.md#fn.register)
    - Go：[`RegisterCommand`](../go/commands.md#RegisterCommand)
    - Zig：[`registerCommand`](../zig/commands.md#registerCommand)

### `register_command_ex` {#register_command_ex}

```c
bool (*register_command_ex)(
    PierModHandle mod,
    PierStr name,
    PierStr description,
    int32_t permission,
    PierStr overloads_snbt,
    PierCommandCb cb,
    void* user
);
```

和 `register_command` 一样，但带有类型化的重载。`overloads_snbt` 的形状：

```text
{overloads:[[{name:"target",kind:"player",optional:0b},…],…]}
```

`kind` 可以是 `int|bool|float|string|enum|soft_enum|actor|player|block_pos|vec3|raw_text|message|json|item|block_name|effect|actor_type|command|relative_float|file_path`，其中 `enum` 和 `soft_enum` 还需要 `"enum":"Name"`。

回调的 `args` 收到 SNBT 形式的解析结果 `{overload:N, args:{<name>:…}}`，`origin_name` 则变成来源的 SNBT `{name,type,dim,x,y,z}`。

- 调用形式：`api->register_command_ex(mod, name, description, permission, overloads_snbt, cb, user)`
- 参数：
    - mod : `PierModHandle`
    - name : `PierStr`
    - description : `PierStr`
    - permission : `int32_t`
    - overloads_snbt : `PierStr`
    - cb : `PierCommandCb`
    - user : `void*`
- 返回值类型：`bool`
- 所在分节：§H 带参数的命令与枚举（`§H parameterized commands & enums`）
- 表内序号：第 54 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`CommandBuilder::register`](../rust/command.md#CommandBuilder.register)
    - Go：[`CommandBuilder.Register`](../go/commands.md#CommandBuilder.Register)

### `register_command_enum` {#register_command_enum}

```c
bool (*register_command_enum)(PierStr name, PierStr values_snbt);
```

`values_snbt` 形如 `{values:[["name",1L],…]}`，交给 `tryRegisterRuntimeEnum`。

- 调用形式：`api->register_command_enum(name, values_snbt)`
- 参数：
    - name : `PierStr`
    - values_snbt : `PierStr`
- 返回值类型：`bool`
- 所在分节：§H 带参数的命令与枚举（`§H parameterized commands & enums`）
- 表内序号：第 55 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`command::register_enum`](../rust/command.md#fn.register_enum)
    - Go：[`RegisterCommandEnum`](../go/commands.md#RegisterCommandEnum)、[`Raw.RegisterCommandEnum`](../go/raw.md#Raw.RegisterCommandEnum)

### `register_command_soft_enum` {#register_command_soft_enum}

```c
bool (*register_command_soft_enum)(PierStr name, PierStr values_snbt);
```

`values_snbt` 形如 `{values:["a","b"]}`，交给 `tryRegisterSoftEnum`。

- 调用形式：`api->register_command_soft_enum(name, values_snbt)`
- 参数：
    - name : `PierStr`
    - values_snbt : `PierStr`
- 返回值类型：`bool`
- 所在分节：§H 带参数的命令与枚举（`§H parameterized commands & enums`）
- 表内序号：第 56 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`command::register_soft_enum`](../rust/command.md#fn.register_soft_enum)
    - Go：[`RegisterSoftEnum`](../go/commands.md#RegisterSoftEnum)、[`Raw.RegisterCommandSoftEnum`](../go/raw.md#Raw.RegisterCommandSoftEnum)

### `update_command_soft_enum` {#update_command_soft_enum}

```c
bool (*update_command_soft_enum)(PierStr name, int32_t op, PierStr values_snbt);
```

`op`：0=设置，1=添加，2=移除。

- 调用形式：`api->update_command_soft_enum(name, op, values_snbt)`
- 参数：
    - name : `PierStr`
    - op : `int32_t`
    - values_snbt : `PierStr`
- 返回值类型：`bool`
- 所在分节：§H 带参数的命令与枚举（`§H parameterized commands & enums`）
- 表内序号：第 57 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`command::update_soft_enum`](../rust/command.md#fn.update_soft_enum)
    - Go：[`UpdateSoftEnum`](../go/commands.md#UpdateSoftEnum)、[`Raw.UpdateCommandSoftEnum`](../go/raw.md#Raw.UpdateCommandSoftEnum)

## 类型 {#types}

### `PierCommandCb` {#PierCommandCb}

```c
typedef void (*PierCommandCb)(
    void* user,
    PierStr args,
    PierStr origin_name,
    void* out_ctx,
    PierStrSink out_success,
    PierStrSink out_error
);
```

自定义命令的回调。

- `args`：命令名后面的原始文本，可能为空。
- `origin_name`：命令来源的显示名（玩家名或 `"Server"`）。
- `out_success` / `out_error`：每调用一次输出一行，次数不限。

### `PierCmdOutputSink` {#PierCmdOutputSink}

```c
typedef void (*PierCmdOutputSink)(void* ctx, bool success, PierStr output);
```

`execute_command` 的输出回调：完整的命令输出，加上是否成功的标志。
