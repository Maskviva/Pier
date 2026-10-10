# Commands

??? note "Section notes in abi.h"

    **the core slots, present since ABI v1**

    **§H parameterized commands & enums**

## Slots {#slots}

### `execute_command` {#execute_command}

```c
bool (*execute_command)(PierStr cmd, void* ctx, PierCmdOutputSink sink);
```

Execute a command as the server console (permission: Owner) and collect its output. Server thread only. Returns false if the level is not ready.

- Call: `api->execute_command(cmd, ctx, sink)`
- Parameters:
    - cmd : `PierStr`
    - ctx : `void*`
    - sink : `PierCmdOutputSink`
- Return type: `bool`
- Section of abi.h: the core slots, present since ABI v1
- Position in the table: slot 7, counting from 0
- Callers in each binding:
    - Rust: [`Host::execute_command`](../rust/host.md#Host.execute_command)
    - Go: [`ExecuteCommand`](../go/commands.md#ExecuteCommand)
    - Zig: [`executeCommand`](../zig/commands.md#executeCommand)

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

Register a custom command `/name [args: raw text]`.

```text
permission: 0=Any,1=GameDirectors,2=Admin,3=Host,4=Owner
            (mirrors CommandPermissionLevel).
```

Call during `on_enable`, on the server thread. The command stays registered for the lifetime of the server (Bedrock cannot unregister commands); callbacks for disabled mods are muted by the loader.

- Call: `api->register_command(mod, name, description, permission, cb, user)`
- Parameters:
    - mod : `PierModHandle`
    - name : `PierStr`
    - description : `PierStr`
    - permission : `int32_t`
    - cb : `PierCommandCb`
    - user : `void*`
- Return type: `bool`
- Section of abi.h: the core slots, present since ABI v1
- Position in the table: slot 8, counting from 0
- Callers in each binding:
    - Rust: [`command::register`](../rust/command.md#fn.register)
    - Go: [`RegisterCommand`](../go/commands.md#RegisterCommand)
    - Zig: [`registerCommand`](../zig/commands.md#registerCommand)

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

Like `register_command`, but with typed overloads. `overloads_snbt`:

```text
{overloads:[[{name:"target",kind:"player",optional:0b},…],…]}
```

kinds: int|bool|float|string|enum|`soft_enum`|actor|player|`block_pos`|vec3|

```text
raw_text|message|json|item|block_name|effect|actor_type|command|
relative_float|file_path (enum/soft_enum also need "enum":"Name").
```

The callback's `args` receives the parse result as SNBT

```text
{overload:N, args:{<name>:…}}   and `origin_name` becomes origin SNBT
{name,type,dim,x,y,z}.
```

- Call: `api->register_command_ex(mod, name, description, permission, overloads_snbt, cb, user)`
- Parameters:
    - mod : `PierModHandle`
    - name : `PierStr`
    - description : `PierStr`
    - permission : `int32_t`
    - overloads_snbt : `PierStr`
    - cb : `PierCommandCb`
    - user : `void*`
- Return type: `bool`
- Section of abi.h: §H parameterized commands & enums
- Position in the table: slot 54, counting from 0
- Callers in each binding:
    - Rust: [`CommandBuilder::register`](../rust/command.md#CommandBuilder.register)
    - Go: [`CommandBuilder.Register`](../go/commands.md#CommandBuilder.Register)

### `register_command_enum` {#register_command_enum}

```c
bool (*register_command_enum)(PierStr name, PierStr values_snbt);
```

`values_snbt` = {values:\[\["name",1L\],…\]}  → tryRegisterRuntimeEnum.

- Call: `api->register_command_enum(name, values_snbt)`
- Parameters:
    - name : `PierStr`
    - values_snbt : `PierStr`
- Return type: `bool`
- Section of abi.h: §H parameterized commands & enums
- Position in the table: slot 55, counting from 0
- Callers in each binding:
    - Rust: [`command::register_enum`](../rust/command.md#fn.register_enum)
    - Go: [`RegisterCommandEnum`](../go/commands.md#RegisterCommandEnum), [`Raw.RegisterCommandEnum`](../go/raw.md#Raw.RegisterCommandEnum)

### `register_command_soft_enum` {#register_command_soft_enum}

```c
bool (*register_command_soft_enum)(PierStr name, PierStr values_snbt);
```

`values_snbt` = {values:\["a","b"\]}         → tryRegisterSoftEnum.

- Call: `api->register_command_soft_enum(name, values_snbt)`
- Parameters:
    - name : `PierStr`
    - values_snbt : `PierStr`
- Return type: `bool`
- Section of abi.h: §H parameterized commands & enums
- Position in the table: slot 56, counting from 0
- Callers in each binding:
    - Rust: [`command::register_soft_enum`](../rust/command.md#fn.register_soft_enum)
    - Go: [`RegisterSoftEnum`](../go/commands.md#RegisterSoftEnum), [`Raw.RegisterCommandSoftEnum`](../go/raw.md#Raw.RegisterCommandSoftEnum)

### `update_command_soft_enum` {#update_command_soft_enum}

```c
bool (*update_command_soft_enum)(PierStr name, int32_t op, PierStr values_snbt);
```

op: 0=set 1=add 2=remove.

- Call: `api->update_command_soft_enum(name, op, values_snbt)`
- Parameters:
    - name : `PierStr`
    - op : `int32_t`
    - values_snbt : `PierStr`
- Return type: `bool`
- Section of abi.h: §H parameterized commands & enums
- Position in the table: slot 57, counting from 0
- Callers in each binding:
    - Rust: [`command::update_soft_enum`](../rust/command.md#fn.update_soft_enum)
    - Go: [`UpdateSoftEnum`](../go/commands.md#UpdateSoftEnum), [`Raw.UpdateCommandSoftEnum`](../go/raw.md#Raw.UpdateCommandSoftEnum)

## Types {#types}

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

Custom command callback.

```text
args        : raw text following the command name (may be empty).
origin_name : display name of the command origin (player name / "Server").
out_success / out_error : call any number of times to emit output lines.
```

### `PierCmdOutputSink` {#PierCmdOutputSink}

```c
typedef void (*PierCmdOutputSink)(void* ctx, bool success, PierStr output);
```

Output sink for `execute_command`: full command output + success flag.
