# Go：SNBT 与 NBT 值

## 函数 {#functions}

### `ParseSNBT` {#ParseSNBT}

```go
func ParseSNBT(text string) (*Nbt, error)
```

解析 SNBT 文本，规则和 Rust 绑定一样：不带后缀的数字依次尝试 int、long、double；后缀 b、s、l、f 或 d 指定类型；true 和 false 是字节；其他不带引号的单词是字符串。

- 参数：
    - text : `string`
- 返回值类型：`(*Nbt, error)`

### `NewCompound` {#NewCompound}

```go
func NewCompound() *Nbt
```

一个空的复合标签。

- 返回值类型：`*Nbt`

### `NbtByteOf` {#NbtByteOf}

```go
func NbtByteOf(v int8) *Nbt
```

一个 byte 值。

- 参数：
    - v : `int8`
- 返回值类型：`*Nbt`

### `NbtIntOf` {#NbtIntOf}

```go
func NbtIntOf(v int32) *Nbt
```

一个 int 值。

- 参数：
    - v : `int32`
- 返回值类型：`*Nbt`

### `NbtLongOf` {#NbtLongOf}

```go
func NbtLongOf(v int64) *Nbt
```

一个 long 值。

- 参数：
    - v : `int64`
- 返回值类型：`*Nbt`

### `NbtDoubleOf` {#NbtDoubleOf}

```go
func NbtDoubleOf(v float64) *Nbt
```

一个 double 值。

- 参数：
    - v : `float64`
- 返回值类型：`*Nbt`

### `NbtStringOf` {#NbtStringOf}

```go
func NbtStringOf(v string) *Nbt
```

一个字符串值。

- 参数：
    - v : `string`
- 返回值类型：`*Nbt`

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

一个 SNBT 值。各种宽度的整数都放在 `Int` 里，两种浮点数都放在 `Float` 里；复合标签把键按顺序放在 `Keys` 里，值放在 `Map` 里。

### `Nbt.Set` {#Nbt.Set}

```go
func (v *Nbt) Set(key string, value *Nbt)
```

在复合标签里把 `value` 放到 `key` 下，键保持它第一次出现的位置。

- 参数：
    - key : `string`
    - value : `*Nbt`

### `Nbt.Get` {#Nbt.Get}

```go
func (v *Nbt) Get(path string) *Nbt
```

沿着用点分隔的路径穿过复合标签；某一步不存在时为 nil。

- 参数：
    - path : `string`
- 返回值类型：`*Nbt`

### `Nbt.OptString` {#Nbt.OptString}

```go
func (v *Nbt) OptString(path string) (string, bool)
```

`path` 处的字符串；不存在或者不是字符串时为 false。

- 参数：
    - path : `string`
- 返回值类型：`(string, bool)`

### `Nbt.OptInt` {#Nbt.OptInt}

```go
func (v *Nbt) OptInt(path string) (int64, bool)
```

`path` 处任意宽度的整数；不存在或者不是整数时为 false。

- 参数：
    - path : `string`
- 返回值类型：`(int64, bool)`

### `Nbt.OptFloat` {#Nbt.OptFloat}

```go
func (v *Nbt) OptFloat(path string) (float64, bool)
```

`path` 处的数字，整数或浮点数都可以；不存在时为 false。

- 参数：
    - path : `string`
- 返回值类型：`(float64, bool)`

### `Nbt.OptBool` {#Nbt.OptBool}

```go
func (v *Nbt) OptBool(path string) (bool, bool)
```

把 `path` 处的字节当作布尔值读取；不存在，或者不是 0 或 1 的字节时，第二个返回值为 false。

- 参数：
    - path : `string`
- 返回值类型：`(bool, bool)`

### `Nbt.SNBT` {#Nbt.SNBT}

```go
func (v *Nbt) SNBT() (string, error)
```

把这个值写成 SNBT。不是有限数的浮点值会返回错误：SNBT 没有写法能表示它，而宿主的解析器遇到一个这样的值，会拒绝它后面的所有字段。

- 返回值类型：`(string, error)`

## `NbtKind` {#NbtKind}

```go
type NbtKind uint8
```

一个 NBT 值的类型。

| 名称 | 值 | 说明 |
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
