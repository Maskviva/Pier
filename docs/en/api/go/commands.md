# Go: Commands

## Functions {#functions}

### `ExecuteCommand` {#ExecuteCommand}

```go
func ExecuteCommand(cmd string) ([]CommandOutput, error)
```

ExecuteCommand runs cmd as the console and returns what it printed. Server thread only. An error means the level is not ready, or the host cannot run commands.

- Parameters:
    - cmd : `string`
- Return type: `([]CommandOutput, error)`
- Slots: [`execute_command`](../cpp/commands.md#execute_command)

### `RegisterCommand` {#RegisterCommand}

```go
func RegisterCommand(name, description string, permission Permission, fn func(*Invocation)) error
```

RegisterCommand registers /name, whose text after the name reaches fn as Args. Call it from Enable, on the server thread. A command stays registered while the server runs, since Bedrock cannot unregister one; the host mutes it while the mod is disabled.

- Parameters:
    - name : `string`
    - description : `string`
    - permission : `Permission`
    - fn : `func(*Invocation)`
- Return type: `error`
- Slots: [`register_command`](../cpp/commands.md#register_command)

### `NewOverload` {#NewOverload}

```go
func NewOverload() *Overload
```

NewOverload begins an overload.

- Return type: `*Overload`

### `NewCommand` {#NewCommand}

```go
func NewCommand(name, description string, permission Permission) *CommandBuilder
```

NewCommand begins declaring /name.

- Parameters:
    - name : `string`
    - description : `string`
    - permission : `Permission`
- Return type: `*CommandBuilder`

### `RegisterCommandEnum` {#RegisterCommandEnum}

```go
func RegisterCommandEnum(name string, values []EnumValue) error
```

RegisterCommandEnum registers a fixed enum for RequiredEnum and OptionalEnum.

- Parameters:
    - name : `string`
    - values : `[]EnumValue`
- Return type: `error`
- Slots: [`register_command_enum`](../cpp/commands.md#register_command_enum)

### `RegisterSoftEnum` {#RegisterSoftEnum}

```go
func RegisterSoftEnum(name string, values []string) error
```

RegisterSoftEnum registers an enum whose values can change while the server runs.

- Parameters:
    - name : `string`
    - values : `[]string`
- Return type: `error`
- Slots: [`register_command_soft_enum`](../cpp/commands.md#register_command_soft_enum)

### `UpdateSoftEnum` {#UpdateSoftEnum}

```go
func UpdateSoftEnum(name string, op SoftEnumOp, values []string) error
```

UpdateSoftEnum replaces, adds to or removes from the values of a soft enum.

- Parameters:
    - name : `string`
    - op : `SoftEnumOp`
    - values : `[]string`
- Return type: `error`
- Slots: [`update_command_soft_enum`](../cpp/commands.md#update_command_soft_enum)

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

CommandCall is one run of a command declared with CommandBuilder.

### `CommandCall.Arg` {#CommandCall.Arg}

```go
func (c *CommandCall) Arg(name string) (*Nbt, bool)
```

Arg is a named argument, and false for an optional one left out.

- Parameters:
    - name : `string`
- Return type: `(*Nbt, bool)`

### `CommandCall.ArgString` {#CommandCall.ArgString}

```go
func (c *CommandCall) ArgString(name string) (string, bool)
```

ArgString is a named argument as text.

- Parameters:
    - name : `string`
- Return type: `(string, bool)`

### `CommandCall.ArgInt` {#CommandCall.ArgInt}

```go
func (c *CommandCall) ArgInt(name string) (int64, bool)
```

ArgInt is a named integer argument.

- Parameters:
    - name : `string`
- Return type: `(int64, bool)`

### `CommandCall.ArgFloat` {#CommandCall.ArgFloat}

```go
func (c *CommandCall) ArgFloat(name string) (float64, bool)
```

ArgFloat is a named number argument.

- Parameters:
    - name : `string`
- Return type: `(float64, bool)`

### `CommandCall.IsConsole` {#CommandCall.IsConsole}

```go
func (c *CommandCall) IsConsole() bool
```

IsConsole reports whether the dedicated server console ran the command.

- Return type: `bool`

### `CommandCall.Success` {#CommandCall.Success}

```go
func (c *CommandCall) Success(msg string)
```

Success sends a line of output.

- Parameters:
    - msg : `string`

### `CommandCall.Error` {#CommandCall.Error}

```go
func (c *CommandCall) Error(msg string)
```

Error sends a line of error output.

- Parameters:
    - msg : `string`

## `Overload` {#Overload}

```go
type Overload struct {
    // unexported fields
}
```

Overload is one way a command parses: its parameters in order.

### `Overload.Required` {#Overload.Required}

```go
func (o *Overload) Required(name string, t ParamType) *Overload
```

Required adds a parameter that must be given.

- Parameters:
    - name : `string`
    - t : `ParamType`
- Return type: `*Overload`

### `Overload.Optional` {#Overload.Optional}

```go
func (o *Overload) Optional(name string, t ParamType) *Overload
```

Optional adds a parameter that may be left out; Arg answers false for it then.

- Parameters:
    - name : `string`
    - t : `ParamType`
- Return type: `*Overload`

### `Overload.RequiredEnum` {#Overload.RequiredEnum}

```go
func (o *Overload) RequiredEnum(name string, t ParamType, enum string) *Overload
```

RequiredEnum adds an enum parameter, which RegisterCommandEnum or RegisterSoftEnum registered first.

- Parameters:
    - name : `string`
    - t : `ParamType`
    - enum : `string`
- Return type: `*Overload`

### `Overload.OptionalEnum` {#Overload.OptionalEnum}

```go
func (o *Overload) OptionalEnum(name string, t ParamType, enum string) *Overload
```

OptionalEnum adds an enum parameter that may be left out.

- Parameters:
    - name : `string`
    - t : `ParamType`
    - enum : `string`
- Return type: `*Overload`

### `Overload.Text` {#Overload.Text}

```go
func (o *Overload) Text(name, literal string) *Overload
```

Text adds a literal word, the shape /scoreboard objectives add is made of. The word comes back among the arguments under name, so a handler reads it like any other parameter. Sibling words taking the same arguments are one enum instead (contract section 6.1).

- Parameters:
    - name : `string`
    - literal : `string`
- Return type: `*Overload`

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

Invocation is one run of a command. Success and Error may be called any number of times during the callback and not after it returns.

### `Invocation.Success` {#Invocation.Success}

```go
func (i *Invocation) Success(msg string)
```

Success sends a line of output.

- Parameters:
    - msg : `string`

### `Invocation.Error` {#Invocation.Error}

```go
func (i *Invocation) Error(msg string)
```

Error sends a line of error output.

- Parameters:
    - msg : `string`

## `CommandBuilder` {#CommandBuilder}

```go
type CommandBuilder struct {
    // unexported fields
}
```

CommandBuilder declares a command with typed overloads. Each overload must parse a different input: two that accept the same words leave the engine to pick one.

### `CommandBuilder.Overload` {#CommandBuilder.Overload}

```go
func (b *CommandBuilder) Overload(o *Overload) *CommandBuilder
```

Overload adds one way the command parses.

- Parameters:
    - o : `*Overload`
- Return type: `*CommandBuilder`

### `CommandBuilder.Register` {#CommandBuilder.Register}

```go
func (b *CommandBuilder) Register(fn func(*CommandCall)) error
```

Register registers the command. fn runs on the server thread for every run; a payload the binding cannot parse is reported to whoever ran the command and fn does not run.

- Parameters:
    - fn : `func(*CommandCall)`
- Return type: `error`
- Slots: [`register_command_ex`](../cpp/commands.md#register_command_ex)

## `CommandOutput` {#CommandOutput}

```go
type CommandOutput struct {
    Success bool
    Text    string
}
```

CommandOutput is one line a command printed.

## `Permission` {#Permission}

```go
type Permission int32
```

Permission is who may run a command; it mirrors CommandPermissionLevel.

| Name | Value | Description |
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

ParamType is the type of one command parameter, the kind words of `register_command_ex`.

| Name | Value | Description |
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

CommandOrigin is who ran a command and where.

## `EnumValue` {#EnumValue}

```go
type EnumValue struct {
    Name  string
    Value int64
}
```

EnumValue is one value of a command enum.

## `SoftEnumOp` {#SoftEnumOp}

```go
type SoftEnumOp int32
```

SoftEnumOp is how UpdateSoftEnum changes the values.

| Name | Value | Description |
|---|---|---|
| <span id="SoftEnumSet"></span>`SoftEnumSet` | `0` |  |
| <span id="SoftEnumAdd"></span>`SoftEnumAdd` | `1` |  |
| <span id="SoftEnumRemove"></span>`SoftEnumRemove` | `2` |  |
