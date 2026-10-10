# levilamina::command

Custom commands.
**Command registration is one way**
Bedrock offers no route to deregister a command, so a registered command lives until the server
stops. While a mod is disabled the host mutes the callback rather than removing it, and re-
enabling resumes it. There is therefore no `unregister` and no handle that deregisters on drop,
since such a handle would suggest it can be undone when it cannot.

It follows that registering a command inside `on_enable` has to survive being called several
times, as a hot reload does. A re-registration under the same name only swaps the callback and
does not rebuild the command.

**Two shapes**

[`register`](command.md#fn.register) takes the whole raw line, which suits parsing it yourself. [`CommandBuilder`](command.md#CommandBuilder)
declares typed overloads that the engine parses and completes, so a player sees argument hints
on the client.

## Functions {#functions}

### `command::register` {#fn.register}

```rust
pub fn register(
    name: &str,
    description: &str,
    permission: CommandPermission,
    handler: impl FnMut(&mut Invocation<'_>) + Send + 'static,
) -> Result<()>
```

Registers a command that takes the whole raw line.

[`Invocation::raw`](command.md#Invocation.raw) in the callback is everything after `/name`, parsed by you.

- Parameters:
    - name : `&str`
    - description : `&str`
    - permission : `CommandPermission`
    - handler : `impl FnMut(&mut Invocation<'_>) + Send + 'static`
- Return type: `Result<()>`
- Slots: [`register_command`](../cpp/commands.md#register_command)

### `command::builder` {#fn.builder}

```rust
pub fn builder(
    name: impl Into<String>,
    description: impl Into<String>,
    permission: CommandPermission,
) -> CommandBuilder
```

Begins declaring a command with overloads.

- Parameters:
    - name : `impl Into<String>`
    - description : `impl Into<String>`
    - permission : `CommandPermission`
- Return type: `CommandBuilder`

### `command::register_enum` {#fn.register_enum}

```rust
pub fn register_enum(name: &str, values: &[(&str, u64)]) -> Result<()>
```

Registers a static enum for a `ParamType::Enum` parameter to reference.

- Parameters:
    - name : `&str`
    - values : `&[(&str, u64)]`
- Return type: `Result<()>`
- Slots: [`register_command_enum`](../cpp/commands.md#register_command_enum)

### `command::register_soft_enum` {#fn.register_soft_enum}

```rust
pub fn register_soft_enum(name: &str, values: &[&str]) -> Result<()>
```

Registers a soft enum that can change at runtime, for a `ParamType::SoftEnum`
parameter to reference.

- Parameters:
    - name : `&str`
    - values : `&[&str]`
- Return type: `Result<()>`
- Slots: [`register_command_soft_enum`](../cpp/commands.md#register_command_soft_enum)

### `command::update_soft_enum` {#fn.update_soft_enum}

```rust
pub fn update_soft_enum(name: &str, op: SoftEnumOp, values: &[&str]) -> Result<()>
```

- Parameters:
    - name : `&str`
    - op : `SoftEnumOp`
    - values : `&[&str]`
- Return type: `Result<()>`
- Slots: [`update_command_soft_enum`](../cpp/commands.md#update_command_soft_enum)

## `Invocation` {#Invocation}

```rust
pub struct Invocation<'a> {
    // private fields
}
```

One command invocation.

### `Invocation::raw` {#Invocation.raw}

```rust
pub fn raw(&self) -> &str
```

The raw argument text. For a command registered through `CommandBuilder` this is the
argument SNBT.

- Return type: `&str`

### `Invocation::success` {#Invocation.success}

```rust
pub fn success(&self, msg: &str)
```

Returns one success output.

- Parameters:
    - msg : `&str`

### `Invocation::error` {#Invocation.error}

```rust
pub fn error(&self, msg: &str)
```

Returns one error output. It is a separate channel from success, the client colors it
differently, and a failed command must not use the success channel, which would make a
command block decide wrongly.

- Parameters:
    - msg : `&str`

### `Invocation::origin` {#Invocation.origin}

```rust
pub fn origin(&self) -> CommandOrigin
```

The origin.

A command from [`CommandBuilder`](command.md#CommandBuilder) carries `{name,type,dim,x,y,z}`. One from
`register` carries only the name, so `kind` is -1 there and [`CommandOrigin::is_console`](command.md#CommandOrigin.is_console)
and [`CommandOrigin::player_name`](command.md#CommandOrigin.player_name) cannot tell who sent it; a command that needs to
know is registered through `CommandBuilder`. The two shapes are told apart by
structure: a bare word such as `Steve` is itself valid SNBT, so a successful parse
alone does not mean the structured form.

- Return type: `CommandOrigin`

### `Invocation::overload` {#Invocation.overload}

```rust
pub fn overload(&mut self) -> Option<i32>
```

Which overload matched. Only a command registered through `CommandBuilder` has one.

- Return type: `Option<i32>`

### `Invocation::arg` {#Invocation.arg}

```rust
pub fn arg(&mut self, name: &str) -> Option<&NbtValue>
```

Reads a named argument. An omitted optional argument gives `None`.

- Parameters:
    - name : `&str`
- Return type: `Option<&NbtValue>`

### `Invocation::arg_str` {#Invocation.arg_str}

```rust
pub fn arg_str(&mut self, name: &str) -> Option<&str>
```

- Parameters:
    - name : `&str`
- Return type: `Option<&str>`

### `Invocation::arg_i64` {#Invocation.arg_i64}

```rust
pub fn arg_i64(&mut self, name: &str) -> Option<i64>
```

- Parameters:
    - name : `&str`
- Return type: `Option<i64>`

### `Invocation::arg_f64` {#Invocation.arg_f64}

```rust
pub fn arg_f64(&mut self, name: &str) -> Option<f64>
```

- Parameters:
    - name : `&str`
- Return type: `Option<f64>`

### `Invocation::arg_bool` {#Invocation.arg_bool}

```rust
pub fn arg_bool(&mut self, name: &str) -> Option<bool>
```

- Parameters:
    - name : `&str`
- Return type: `Option<bool>`

## `OverloadBuilder` {#OverloadBuilder}

```rust
pub struct OverloadBuilder {
    // private fields
}
```

The declaration of one overload.

- Implements: `Debug`, `Clone`, `Default`

### `OverloadBuilder::required` {#OverloadBuilder.required}

```rust
pub fn required(self, name: &str, kind: ParamType) -> OverloadBuilder
```

- Parameters:
    - name : `&str`
    - kind : `ParamType`
- Return type: `OverloadBuilder`

### `OverloadBuilder::optional` {#OverloadBuilder.optional}

```rust
pub fn optional(self, name: &str, kind: ParamType) -> OverloadBuilder
```

- Parameters:
    - name : `&str`
    - kind : `ParamType`
- Return type: `OverloadBuilder`

### `OverloadBuilder::text` {#OverloadBuilder.text}

```rust
pub fn text(self, name: &str, literal: &str) -> OverloadBuilder
```

A **literal**: a fixed word the player types, and a node in the command tree.

**Why this is not an enum with one value**

An `Enum` parameter carries `EnumAutocompleteExpansion`, so the client lists its
values, but its data type is `Enum` and not `ChainedSubcommand`: there is no tree
node, and the client has nothing to narrow as the player types. A literal is what
`/scoreboard objectives add` is made of, and it is the only form that filters.

The word comes back in the arguments under `name`, so the handler reads it the
same way as any other parameter, rather than keying on an overload index that
moves when an overload is inserted above it.

Sibling words that take the same arguments are one enum instead (CONTRACT.md 6.1).

- Parameters:
    - name : `&str`
    - literal : `&str`
- Return type: `OverloadBuilder`

### `OverloadBuilder::required_enum` {#OverloadBuilder.required_enum}

```rust
pub fn required_enum(self, name: &str, kind: ParamType, enum_name: &str) -> OverloadBuilder
```

- Parameters:
    - name : `&str`
    - kind : `ParamType`
    - enum_name : `&str`
- Return type: `OverloadBuilder`

### `OverloadBuilder::optional_enum` {#OverloadBuilder.optional_enum}

```rust
pub fn optional_enum(self, name: &str, kind: ParamType, enum_name: &str) -> OverloadBuilder
```

- Parameters:
    - name : `&str`
    - kind : `ParamType`
    - enum_name : `&str`
- Return type: `OverloadBuilder`

## `CommandOrigin` {#CommandOrigin}

```rust
pub struct CommandOrigin {
    /// The player name, or the name of the console.
    pub name: String,
    /// The `CommandOriginType`, where 0 is a player and 7 is the dedicated server console.
    /// -1 when the host did not say, which is always the case for a `register` command.
    pub kind: i32,
    /// Where the origin is. A console has no position and gives `None`.
    pub at: Option<(i32, f64, f64, f64)>,
}
```

Where a command came from.

- Implements: `Debug`, `Clone`, `PartialEq`

### `CommandOrigin::is_console` {#CommandOrigin.is_console}

```rust
pub fn is_console(&self) -> bool
```

- Return type: `bool`

### `CommandOrigin::player_name` {#CommandOrigin.player_name}

```rust
pub fn player_name(&self) -> Option<&str>
```

The player name when the origin is a player.

Note this is a name and not an identity: a permission decision uses `kind` plus a
own player table, for the reason [`crate::sel`](player.md#PlayerSel) gives.

- Return type: `Option<&str>`

## `CommandBuilder` {#CommandBuilder}

```rust
pub struct CommandBuilder {
    // private fields
}
```

A command with typed overloads.

Each overload parses a different input: two that accept the same words leave the engine
to pick one, and the handler cannot rely on which (contract §6.1). Sibling words with the
same arguments are one enum parameter; a word with arguments of its own is a literal.

```rust
command::register_enum("plot_simple", &[("menu", 0), ("help", 1), ("status", 2)])?;
command::builder("plot", "Plots", CommandPermission::Any)
    .overload(|o| o.required_enum("simple", ParamType::Enum, "plot_simple"))
    .overload(|o| o.text("verb", "rate").required("score", ParamType::Int))
    .register(handler)?;
```

### `CommandBuilder::overload` {#CommandBuilder.overload}

```rust
pub fn overload(
        mut self,
        build: impl FnOnce(OverloadBuilder) -> OverloadBuilder,
    ) -> CommandBuilder
```

- Parameters:
    - build : `impl FnOnce(OverloadBuilder) -> OverloadBuilder,`
- Return type: `CommandBuilder`

### `CommandBuilder::register` {#CommandBuilder.register}

```rust
pub fn register(self, handler: impl FnMut(&mut Invocation<'_>) + Send + 'static) -> Result<()>
```

Registers it. At least one overload is required, since the host refuses outright with
none, so this stops it earlier and says why rather than leaving a bare registration
failure to be guessed at.

- Parameters:
    - handler : `impl FnMut(&mut Invocation<'_>) + Send + 'static`
- Return type: `Result<()>`
- Slots: [`register_command_ex`](../cpp/commands.md#register_command_ex)

## `CommandPermission` {#CommandPermission}

```rust
pub enum CommandPermission {
        Any = 0,
        GameDirectors = 1,
        Admin = 2,
        Host = 3,
        Owner = 4,
}
```

The permission a command needs. The values mirror `CommandPermissionLevel`.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

### `CommandPermission::as_i32` {#CommandPermission.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- Return type: `i32`

## `ParamType` {#ParamType}

```rust
pub enum ParamType {
        Int,
        Bool,
        Float,
        String,
        Enum,
        SoftEnum,
        Actor,
        Player,
        BlockPos,
        Vec3,
        RawText,
        Message,
        Json,
        Item,
        BlockName,
        Effect,
        ActorType,
        Command,
        RelativeFloat,
        FilePath,
}
```

The type of one parameter inside one overload.

`Enum` and `SoftEnum` also need an enum name, declared through the
[`OverloadBuilder::required_enum`](command.md#OverloadBuilder.required_enum) pair of methods.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

### `ParamType::as_str` {#ParamType.as_str}

```rust
pub fn as_str(self) -> &'static str
```

The spelling on the ABI. The host dispatches on this string and a typo drops the whole
overload.

- Return type: `&'static str`

## `SoftEnumOp` {#SoftEnumOp}

```rust
pub enum SoftEnumOp {
        Set = 0,
        Add = 1,
        Remove = 2,
}
```

Changes the values of a soft enum.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`
