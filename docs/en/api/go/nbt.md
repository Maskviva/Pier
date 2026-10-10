# Go: SNBT and NBT values

## Functions {#functions}

### `ParseSNBT` {#ParseSNBT}

```go
func ParseSNBT(text string) (*Nbt, error)
```

ParseSNBT parses SNBT text, with the same rules as the Rust binding: a bare number is an int, then a long, then a double; a suffix b, s, l, f or d fixes the type; true and false are bytes; any other bare word is a string.

- Parameters:
    - text : `string`
- Return type: `(*Nbt, error)`

### `NewCompound` {#NewCompound}

```go
func NewCompound() *Nbt
```

NewCompound is an empty compound.

- Return type: `*Nbt`

### `NbtByteOf` {#NbtByteOf}

```go
func NbtByteOf(v int8) *Nbt
```

NbtByteOf is a byte value.

- Parameters:
    - v : `int8`
- Return type: `*Nbt`

### `NbtIntOf` {#NbtIntOf}

```go
func NbtIntOf(v int32) *Nbt
```

NbtIntOf is an int value.

- Parameters:
    - v : `int32`
- Return type: `*Nbt`

### `NbtLongOf` {#NbtLongOf}

```go
func NbtLongOf(v int64) *Nbt
```

NbtLongOf is a long value.

- Parameters:
    - v : `int64`
- Return type: `*Nbt`

### `NbtDoubleOf` {#NbtDoubleOf}

```go
func NbtDoubleOf(v float64) *Nbt
```

NbtDoubleOf is a double value.

- Parameters:
    - v : `float64`
- Return type: `*Nbt`

### `NbtStringOf` {#NbtStringOf}

```go
func NbtStringOf(v string) *Nbt
```

NbtStringOf is a string value.

- Parameters:
    - v : `string`
- Return type: `*Nbt`

## `Nbt` {#Nbt}

```go
type Nbt struct {
    Kind  NbtKind
    Int   int64
    Float float64
    Str   string
    List  []*Nbt
    Keys  []string
    Map   map[string]*Nbt
    Ints  []int64
}
```

Nbt is one SNBT value. Integers of every width are in Int and both floating kinds in Float; a compound keeps its keys in order in Keys and its values in Map.

### `Nbt.Set` {#Nbt.Set}

```go
func (v *Nbt) Set(key string, value *Nbt)
```

Set puts value under key of a compound, keeping the key's first position.

- Parameters:
    - key : `string`
    - value : `*Nbt`

### `Nbt.Get` {#Nbt.Get}

```go
func (v *Nbt) Get(path string) *Nbt
```

Get follows a dotted path through compounds, and is nil when a step is missing.

- Parameters:
    - path : `string`
- Return type: `*Nbt`

### `Nbt.OptString` {#Nbt.OptString}

```go
func (v *Nbt) OptString(path string) (string, bool)
```

OptString is the string at path, and false when it is absent or not a string.

- Parameters:
    - path : `string`
- Return type: `(string, bool)`

### `Nbt.OptInt` {#Nbt.OptInt}

```go
func (v *Nbt) OptInt(path string) (int64, bool)
```

OptInt is the integer at path of any width, and false when it is absent or not one.

- Parameters:
    - path : `string`
- Return type: `(int64, bool)`

### `Nbt.OptFloat` {#Nbt.OptFloat}

```go
func (v *Nbt) OptFloat(path string) (float64, bool)
```

OptFloat is the number at path, integer or floating, and false when it is absent.

- Parameters:
    - path : `string`
- Return type: `(float64, bool)`

### `Nbt.OptBool` {#Nbt.OptBool}

```go
func (v *Nbt) OptBool(path string) (bool, bool)
```

OptBool is the byte at path read as a boolean, and false in the second result when it is absent or not a 0 or 1 byte.

- Parameters:
    - path : `string`
- Return type: `(bool, bool)`

### `Nbt.SNBT` {#Nbt.SNBT}

```go
func (v *Nbt) SNBT() (string, error)
```

SNBT writes the value as SNBT. A floating value that is not finite is an error: SNBT has no spelling for it, and the host's parser refuses every field after one.

- Return type: `(string, error)`

## `NbtKind` {#NbtKind}

```go
type NbtKind uint8
```

NbtKind is the type of one NBT value.

| Name | Value | Description |
|---|---|---|
| <span id="NbtByte"></span>`NbtByte` | `1` |  |
| <span id="NbtShort"></span>`NbtShort` | `2` |  |
| <span id="NbtInt"></span>`NbtInt` | `3` |  |
| <span id="NbtLong"></span>`NbtLong` | `4` |  |
| <span id="NbtFloat"></span>`NbtFloat` | `5` |  |
| <span id="NbtDouble"></span>`NbtDouble` | `6` |  |
| <span id="NbtString"></span>`NbtString` | `7` |  |
| <span id="NbtList"></span>`NbtList` | `8` |  |
| <span id="NbtCompound"></span>`NbtCompound` | `9` |  |
| <span id="NbtByteArray"></span>`NbtByteArray` | `10` |  |
| <span id="NbtIntArray"></span>`NbtIntArray` | `11` |  |
| <span id="NbtLongArray"></span>`NbtLongArray` | `12` |  |
