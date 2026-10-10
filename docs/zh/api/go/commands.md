# Go：命令

## 函数 {#functions}

### `ExecuteCommand` {#ExecuteCommand}

```go
func ExecuteCommand(cmd string) ([]CommandOutput, error)
```

以控制台的身份运行 `cmd`，返回它输出的内容。只能在服务器线程调用。返回错误表示世界还没就绪，或者宿主不能运行命令。

- 参数：
    - cmd : `string`
- 返回值类型：`([]CommandOutput, error)`
- 对应槽位：[`execute_command`](../cpp/commands.md#execute_command)

### `RegisterCommand` {#RegisterCommand}

```go
func RegisterCommand(name, description string, permission Permission, fn func(*Invocation)) error
```

注册 `/name`，命令名之后的文本作为 `Args` 交给 `fn`。在 `Enable` 里、在服务器线程上调用。命令在服务器运行期间一直保持注册，因为基岩版不能注销命令；模组被禁用时，宿主会屏蔽它。

- 参数：
    - name : `string`
    - description : `string`
    - permission : `Permission`
    - fn : `func(*Invocation)`
- 返回值类型：`error`
- 对应槽位：[`register_command`](../cpp/commands.md#register_command)

### `NewOverload` {#NewOverload}

```go
func NewOverload() *Overload
```

开始一个重载。

- 返回值类型：`*Overload`

### `NewCommand` {#NewCommand}

```go
func NewCommand(name, description string, permission Permission) *CommandBuilder
```

开始声明 `/name`。

- 参数：
    - name : `string`
    - description : `string`
    - permission : `Permission`
- 返回值类型：`*CommandBuilder`

### `RegisterCommandEnum` {#RegisterCommandEnum}

```go
func RegisterCommandEnum(name string, values []EnumValue) error
```

注册一个固定的枚举，供 `RequiredEnum` 和 `OptionalEnum` 使用。

- 参数：
    - name : `string`
    - values : `[]EnumValue`
- 返回值类型：`error`
- 对应槽位：[`register_command_enum`](../cpp/commands.md#register_command_enum)

### `RegisterSoftEnum` {#RegisterSoftEnum}

```go
func RegisterSoftEnum(name string, values []string) error
```

注册一个值可以在服务器运行时改变的枚举。

- 参数：
    - name : `string`
    - values : `[]string`
- 返回值类型：`error`
- 对应槽位：[`register_command_soft_enum`](../cpp/commands.md#register_command_soft_enum)

### `UpdateSoftEnum` {#UpdateSoftEnum}

```go
func UpdateSoftEnum(name string, op SoftEnumOp, values []string) error
```

替换一个软枚举的值，或者向它添加值、从它移除值。

- 参数：
    - name : `string`
    - op : `SoftEnumOp`
    - values : `[]string`
- 返回值类型：`error`
- 对应槽位：[`update_command_soft_enum`](../cpp/commands.md#update_command_soft_enum)

## `CommandCall` {#CommandCall}

```go
type CommandCall struct {
    // Overload is the index of the overload that parsed.
    Overload int
    Args     *Nbt
    Origin   CommandOrigin
    // unexported fields
}
```

用 `CommandBuilder` 声明的命令的一次执行。

### `CommandCall.Arg` {#CommandCall.Arg}

```go
func (c *CommandCall) Arg(name string) (*Nbt, bool)
```

按名字取一个参数；可选参数没有给出时返回 false。

- 参数：
    - name : `string`
- 返回值类型：`(*Nbt, bool)`

### `CommandCall.ArgString` {#CommandCall.ArgString}

```go
func (c *CommandCall) ArgString(name string) (string, bool)
```

按名字取一个参数，作为文本。

- 参数：
    - name : `string`
- 返回值类型：`(string, bool)`

### `CommandCall.ArgInt` {#CommandCall.ArgInt}

```go
func (c *CommandCall) ArgInt(name string) (int64, bool)
```

按名字取一个整数参数。

- 参数：
    - name : `string`
- 返回值类型：`(int64, bool)`

### `CommandCall.ArgFloat` {#CommandCall.ArgFloat}

```go
func (c *CommandCall) ArgFloat(name string) (float64, bool)
```

按名字取一个数值参数。

- 参数：
    - name : `string`
- 返回值类型：`(float64, bool)`

### `CommandCall.IsConsole` {#CommandCall.IsConsole}

```go
func (c *CommandCall) IsConsole() bool
```

判断这条命令是不是由专用服务器的控制台运行的。

- 返回值类型：`bool`

### `CommandCall.Success` {#CommandCall.Success}

```go
func (c *CommandCall) Success(msg string)
```

发送一行输出。

- 参数：
    - msg : `string`

### `CommandCall.Error` {#CommandCall.Error}

```go
func (c *CommandCall) Error(msg string)
```

发送一行错误输出。

- 参数：
    - msg : `string`

## `Overload` {#Overload}

```go
type Overload struct {
    // unexported fields
}
```

命令的一种解析方式：按顺序排列的参数。

### `Overload.Required` {#Overload.Required}

```go
func (o *Overload) Required(name string, t ParamType) *Overload
```

添加一个必须给出的参数。

- 参数：
    - name : `string`
    - t : `ParamType`
- 返回值类型：`*Overload`

### `Overload.Optional` {#Overload.Optional}

```go
func (o *Overload) Optional(name string, t ParamType) *Overload
```

添加一个可以省略的参数；省略时 `Arg` 对它返回 false。

- 参数：
    - name : `string`
    - t : `ParamType`
- 返回值类型：`*Overload`

### `Overload.RequiredEnum` {#Overload.RequiredEnum}

```go
func (o *Overload) RequiredEnum(name string, t ParamType, enum string) *Overload
```

添加一个枚举参数，枚举要先用 `RegisterCommandEnum` 或 `RegisterSoftEnum` 注册。

- 参数：
    - name : `string`
    - t : `ParamType`
    - enum : `string`
- 返回值类型：`*Overload`

### `Overload.OptionalEnum` {#Overload.OptionalEnum}

```go
func (o *Overload) OptionalEnum(name string, t ParamType, enum string) *Overload
```

添加一个可以省略的枚举参数。

- 参数：
    - name : `string`
    - t : `ParamType`
    - enum : `string`
- 返回值类型：`*Overload`

### `Overload.Text` {#Overload.Text}

```go
func (o *Overload) Text(name, literal string) *Overload
```

添加一个字面单词，`/scoreboard objectives add` 就是由这样的单词组成的。这个单词会以 `name` 为名出现在参数里，处理函数读它和读其他参数一样。接收相同参数的几个并列单词，应当合成一个枚举（契约 6.1 节）。

- 参数：
    - name : `string`
    - literal : `string`
- 返回值类型：`*Overload`

## `Invocation` {#Invocation}

```go
type Invocation struct {
    // Args is the raw text after the command name, possibly empty.
    Args string
    // Origin is the display name of whoever ran it: a player's name, or "Server".
    Origin string
    // unexported fields
}
```

命令的一次执行。回调期间可以任意多次调用 `Success` 和 `Error`，回调返回以后就不能再调用了。

### `Invocation.Success` {#Invocation.Success}

```go
func (i *Invocation) Success(msg string)
```

发送一行输出。

- 参数：
    - msg : `string`

### `Invocation.Error` {#Invocation.Error}

```go
func (i *Invocation) Error(msg string)
```

发送一行错误输出。

- 参数：
    - msg : `string`

## `CommandBuilder` {#CommandBuilder}

```go
type CommandBuilder struct {
    // unexported fields
}
```

声明一个带类型化重载的命令。每个重载解析的输入必须不同：两个重载接受同样的单词时，引擎会自己从中挑一个。

### `CommandBuilder.Overload` {#CommandBuilder.Overload}

```go
func (b *CommandBuilder) Overload(o *Overload) *CommandBuilder
```

添加命令的一种解析方式。

- 参数：
    - o : `*Overload`
- 返回值类型：`*CommandBuilder`

### `CommandBuilder.Register` {#CommandBuilder.Register}

```go
func (b *CommandBuilder) Register(fn func(*CommandCall)) error
```

注册这条命令。每次执行时 `fn` 都在服务器线程上运行；绑定解析不了的载荷会报告给执行命令的人，这时 `fn` 不会运行。

- 参数：
    - fn : `func(*CommandCall)`
- 返回值类型：`error`
- 对应槽位：[`register_command_ex`](../cpp/commands.md#register_command_ex)

## `CommandOutput` {#CommandOutput}

```go
type CommandOutput struct {
    Success bool
    Text    string
}
```

命令输出的一行。

## `Permission` {#Permission}

```go
type Permission int32
```

谁可以运行一条命令，对应 `CommandPermissionLevel`。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PermissionAny"></span>`PermissionAny` | `0` |  |
| <span id="PermissionGameDirectors"></span>`PermissionGameDirectors` | `1` |  |
| <span id="PermissionAdmin"></span>`PermissionAdmin` | `2` |  |
| <span id="PermissionHost"></span>`PermissionHost` | `3` |  |
| <span id="PermissionOwner"></span>`PermissionOwner` | `4` |  |

## `ParamType` {#ParamType}

```go
type ParamType string
```

一个命令参数的类型，也就是 `register_command_ex` 的 kind 单词。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="ParamInt"></span>`ParamInt` | `"int"` |  |
| <span id="ParamBool"></span>`ParamBool` | `"bool"` |  |
| <span id="ParamFloat"></span>`ParamFloat` | `"float"` |  |
| <span id="ParamString"></span>`ParamString` | `"string"` |  |
| <span id="ParamEnum"></span>`ParamEnum` | `"enum"` |  |
| <span id="ParamSoftEnum"></span>`ParamSoftEnum` | `"soft_enum"` |  |
| <span id="ParamActor"></span>`ParamActor` | `"actor"` |  |
| <span id="ParamPlayer"></span>`ParamPlayer` | `"player"` |  |
| <span id="ParamBlockPos"></span>`ParamBlockPos` | `"block_pos"` |  |
| <span id="ParamVec3"></span>`ParamVec3` | `"vec3"` |  |
| <span id="ParamRawText"></span>`ParamRawText` | `"raw_text"` |  |
| <span id="ParamMessage"></span>`ParamMessage` | `"message"` |  |
| <span id="ParamJSON"></span>`ParamJSON` | `"json"` |  |
| <span id="ParamItem"></span>`ParamItem` | `"item"` |  |
| <span id="ParamBlockName"></span>`ParamBlockName` | `"block_name"` |  |
| <span id="ParamEffect"></span>`ParamEffect` | `"effect"` |  |
| <span id="ParamActorType"></span>`ParamActorType` | `"actor_type"` |  |
| <span id="ParamCommand"></span>`ParamCommand` | `"command"` |  |
| <span id="ParamRelativeFloat"></span>`ParamRelativeFloat` | `"relative_float"` |  |
| <span id="ParamFilePath"></span>`ParamFilePath` | `"file_path"` |  |

## `CommandOrigin` {#CommandOrigin}

```go
type CommandOrigin struct {
    Name string
    // Kind is the CommandOriginType: 0 a player, 7 the server console, -1 not said.
    Kind    int32
    Dim     int32
    X, Y, Z float64
    HasPos  bool
}
```

谁在什么地方运行了这条命令。

## `EnumValue` {#EnumValue}

```go
type EnumValue struct {
    Name  string
    Value int64
}
```

命令枚举的一个值。

## `SoftEnumOp` {#SoftEnumOp}

```go
type SoftEnumOp int32
```

`UpdateSoftEnum` 怎样改变枚举的值。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="SoftEnumSet"></span>`SoftEnumSet` | `0` |  |
| <span id="SoftEnumAdd"></span>`SoftEnumAdd` | `1` |  |
| <span id="SoftEnumRemove"></span>`SoftEnumRemove` | `2` |  |
