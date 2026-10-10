# levilamina::command · 命令

自定义命令。

**命令注册是单向的**

基岩版没有注销命令的途径，所以注册过的命令会一直存在到服务器停止。模组被禁用时，宿主屏蔽它的回调，不移除命令；重新启用后恢复。所以这里没有 `unregister`，也没有在丢弃时注销的句柄，这样的句柄会让人以为可以撤销，而实际上撤销不了。

因此，在 `on_enable` 里注册命令必须能承受被调用多次，热重载就会这样调用。用同一个名字再次注册只会换掉回调，不会重建命令。

**两种形状**

[`register`](command.md#fn.register) 接收整行原始文本，适合自己解析。[`CommandBuilder`](command.md#CommandBuilder) 声明带类型的重载，由引擎解析并补全，玩家在客户端能看到参数提示。

## 函数 {#functions}

### `command::register` {#fn.register}

```rust
pub fn register(
    name: &str,
    description: &str,
    permission: CommandPermission,
    handler: impl FnMut(&mut Invocation<'_>) + Send + 'static,
) -> Result<()>
```

注册一条接收整行原始文本的命令。

回调里的 [`Invocation::raw`](command.md#Invocation.raw) 是 `/name` 之后的全部内容，由你自己解析。

- 参数：
    - name : `&str`
    - description : `&str`
    - permission : `CommandPermission`
    - handler : `impl FnMut(&mut Invocation<'_>) + Send + 'static`
- 返回值类型：`Result<()>`
- 对应槽位：[`register_command`](../cpp/commands.md#register_command)

### `command::builder` {#fn.builder}

```rust
pub fn builder(
    name: impl Into<String>,
    description: impl Into<String>,
    permission: CommandPermission,
) -> CommandBuilder
```

开始声明一条带重载的命令。

- 参数：
    - name : `impl Into<String>`
    - description : `impl Into<String>`
    - permission : `CommandPermission`
- 返回值类型：`CommandBuilder`

### `command::register_enum` {#fn.register_enum}

```rust
pub fn register_enum(name: &str, values: &[(&str, u64)]) -> Result<()>
```

注册一个静态枚举，供 `ParamType::Enum` 参数引用。

- 参数：
    - name : `&str`
    - values : `&[(&str, u64)]`
- 返回值类型：`Result<()>`
- 对应槽位：[`register_command_enum`](../cpp/commands.md#register_command_enum)

### `command::register_soft_enum` {#fn.register_soft_enum}

```rust
pub fn register_soft_enum(name: &str, values: &[&str]) -> Result<()>
```

注册一个可以在运行时改变的软枚举，供 `ParamType::SoftEnum` 参数引用。

- 参数：
    - name : `&str`
    - values : `&[&str]`
- 返回值类型：`Result<()>`
- 对应槽位：[`register_command_soft_enum`](../cpp/commands.md#register_command_soft_enum)

### `command::update_soft_enum` {#fn.update_soft_enum}

```rust
pub fn update_soft_enum(name: &str, op: SoftEnumOp, values: &[&str]) -> Result<()>
```

- 参数：
    - name : `&str`
    - op : `SoftEnumOp`
    - values : `&[&str]`
- 返回值类型：`Result<()>`
- 对应槽位：[`update_command_soft_enum`](../cpp/commands.md#update_command_soft_enum)

## `Invocation` {#Invocation}

```rust
pub struct Invocation<'a> {
    // private fields
}
```

一次命令调用。

### `Invocation::raw` {#Invocation.raw}

```rust
pub fn raw(&self) -> &str
```

原始的参数文本。对通过 `CommandBuilder` 注册的命令，这是参数的 SNBT。

- 返回值类型：`&str`

### `Invocation::success` {#Invocation.success}

```rust
pub fn success(&self, msg: &str)
```

返回一条成功输出。

- 参数：
    - msg : `&str`

### `Invocation::error` {#Invocation.error}

```rust
pub fn error(&self, msg: &str)
```

返回一条错误输出。它和成功输出是两个通道，客户端用不同的颜色显示；执行失败的命令不能用成功通道，否则命令方块会做出错误的判断。

- 参数：
    - msg : `&str`

### `Invocation::origin` {#Invocation.origin}

```rust
pub fn origin(&self) -> CommandOrigin
```

命令的来源。

来自 [`CommandBuilder`](command.md#CommandBuilder) 的命令带着 `{name,type,dim,x,y,z}`。来自 `register` 的命令只带名字，这时 `kind` 为 -1，[`CommandOrigin::is_console`](command.md#CommandOrigin.is_console) 和 [`CommandOrigin::player_name`](command.md#CommandOrigin.player_name) 都无法判断是谁发的；需要知道是谁的命令，要通过 `CommandBuilder` 注册。两种形状按结构区分：`Steve` 这样的单词本身就是合法的 SNBT，所以光是解析成功，并不说明是结构化的那种。

- 返回值类型：`CommandOrigin`

### `Invocation::overload` {#Invocation.overload}

```rust
pub fn overload(&mut self) -> Option<i32>
```

匹配上的是哪个重载。只有通过 `CommandBuilder` 注册的命令才有。

- 返回值类型：`Option<i32>`

### `Invocation::arg` {#Invocation.arg}

```rust
pub fn arg(&mut self, name: &str) -> Option<&NbtValue>
```

读取一个命名参数。省略了的可选参数返回 `None`。

- 参数：
    - name : `&str`
- 返回值类型：`Option<&NbtValue>`

### `Invocation::arg_str` {#Invocation.arg_str}

```rust
pub fn arg_str(&mut self, name: &str) -> Option<&str>
```

- 参数：
    - name : `&str`
- 返回值类型：`Option<&str>`

### `Invocation::arg_i64` {#Invocation.arg_i64}

```rust
pub fn arg_i64(&mut self, name: &str) -> Option<i64>
```

- 参数：
    - name : `&str`
- 返回值类型：`Option<i64>`

### `Invocation::arg_f64` {#Invocation.arg_f64}

```rust
pub fn arg_f64(&mut self, name: &str) -> Option<f64>
```

- 参数：
    - name : `&str`
- 返回值类型：`Option<f64>`

### `Invocation::arg_bool` {#Invocation.arg_bool}

```rust
pub fn arg_bool(&mut self, name: &str) -> Option<bool>
```

- 参数：
    - name : `&str`
- 返回值类型：`Option<bool>`

## `OverloadBuilder` {#OverloadBuilder}

```rust
pub struct OverloadBuilder {
    // private fields
}
```

一个重载的声明。

- 实现的 trait：`Debug`、`Clone`、`Default`

### `OverloadBuilder::required` {#OverloadBuilder.required}

```rust
pub fn required(self, name: &str, kind: ParamType) -> OverloadBuilder
```

- 参数：
    - name : `&str`
    - kind : `ParamType`
- 返回值类型：`OverloadBuilder`

### `OverloadBuilder::optional` {#OverloadBuilder.optional}

```rust
pub fn optional(self, name: &str, kind: ParamType) -> OverloadBuilder
```

- 参数：
    - name : `&str`
    - kind : `ParamType`
- 返回值类型：`OverloadBuilder`

### `OverloadBuilder::text` {#OverloadBuilder.text}

```rust
pub fn text(self, name: &str, literal: &str) -> OverloadBuilder
```

一个**字面量**：玩家要输入的固定单词，也是命令树上的一个节点。

**为什么不用只有一个值的枚举**

`Enum` 参数带有 `EnumAutocompleteExpansion`，客户端会列出它的值，但它的数据类型是 `Enum`，并非 `ChainedSubcommand`：没有树节点，玩家输入时客户端没有东西可以缩小范围。`/scoreboard objectives add` 就是由字面量组成的，只有这种形式能随输入过滤。

这个单词会以 `name` 为名回到参数里，所以处理函数读它的方式和读其他参数一样，不用依赖重载的下标；在上面插入一个重载时，下标就会变。

接收相同参数的几个并列单词，应当合成一个枚举（CONTRACT.md 6.1）。

- 参数：
    - name : `&str`
    - literal : `&str`
- 返回值类型：`OverloadBuilder`

### `OverloadBuilder::required_enum` {#OverloadBuilder.required_enum}

```rust
pub fn required_enum(self, name: &str, kind: ParamType, enum_name: &str) -> OverloadBuilder
```

- 参数：
    - name : `&str`
    - kind : `ParamType`
    - enum_name : `&str`
- 返回值类型：`OverloadBuilder`

### `OverloadBuilder::optional_enum` {#OverloadBuilder.optional_enum}

```rust
pub fn optional_enum(self, name: &str, kind: ParamType, enum_name: &str) -> OverloadBuilder
```

- 参数：
    - name : `&str`
    - kind : `ParamType`
    - enum_name : `&str`
- 返回值类型：`OverloadBuilder`

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

命令从哪里来。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`

### `CommandOrigin::is_console` {#CommandOrigin.is_console}

```rust
pub fn is_console(&self) -> bool
```

- 返回值类型：`bool`

### `CommandOrigin::player_name` {#CommandOrigin.player_name}

```rust
pub fn player_name(&self) -> Option<&str>
```

来源是玩家时，玩家的名字。

注意这是名字，不是身份：权限判断要用 `kind` 加上自己的玩家表，原因见 [`crate::sel`](player.md#PlayerSel)。

- 返回值类型：`Option<&str>`

## `CommandBuilder` {#CommandBuilder}

```rust
pub struct CommandBuilder {
    // private fields
}
```

带类型化重载的命令。

每个重载解析的输入必须不同：两个重载接受同样的单词时，由引擎从中挑一个，处理函数不能依赖挑的是哪一个（契约 §6.1）。接收相同参数的并列单词合成一个枚举参数；带有自己参数的单词用字面量。

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

- 参数：
    - build : `impl FnOnce(OverloadBuilder) -> OverloadBuilder,`
- 返回值类型：`CommandBuilder`

### `CommandBuilder::register` {#CommandBuilder.register}

```rust
pub fn register(self, handler: impl FnMut(&mut Invocation<'_>) + Send + 'static) -> Result<()>
```

注册这条命令。至少要有一个重载：没有重载时宿主会直接拒绝，所以这里提前拦下并说明原因，免得只留下一个光秃秃的注册失败让人去猜。

- 参数：
    - handler : `impl FnMut(&mut Invocation<'_>) + Send + 'static`
- 返回值类型：`Result<()>`
- 对应槽位：[`register_command_ex`](../cpp/commands.md#register_command_ex)

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

命令需要的权限。取值对应 `CommandPermissionLevel`。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

### `CommandPermission::as_i32` {#CommandPermission.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- 返回值类型：`i32`

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

一个重载里一个参数的类型。

`Enum` 和 `SoftEnum` 还需要一个枚举名，通过 [`OverloadBuilder::required_enum`](command.md#OverloadBuilder.required_enum) 这一对方法声明。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

### `ParamType::as_str` {#ParamType.as_str}

```rust
pub fn as_str(self) -> &'static str
```

ABI 上的写法。宿主按这个字符串分派，拼错会让整个重载被丢弃。

- 返回值类型：`&'static str`

## `SoftEnumOp` {#SoftEnumOp}

```rust
pub enum SoftEnumOp {
        Set = 0,
        Add = 1,
        Remove = 2,
}
```

修改软枚举的值。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`
