# levilamina::nbt · NBT

NBT 值：跨越边界的所有结构化数据的载体。

Pier 的 ABI 上不传递任何结构体。事件载荷、表单、物品、方块状态、实体快照、命令参数、服务的请求和回答，全都是 SNBT 字符串，所以从一段 SNBT 里读出某个字段，是模组代码里最常做的事。

有两族访问函数，含义不同：

- `opt_*` 返回 `Option`：有就给我，剩下的我自己处理。
- `get_*` 返回 `Result`，缺少键和类型不符是两种不同的错误，消息里带着键名和实际的类型。保护判断（权限、地皮、经济）用这一族，遇到 `Err` 就拒绝。把两者合成一种回答会有什么后果，见 [`crate::event`](event.md)。

## 函数 {#functions}

### `nbt::binary::to_binary` {#fn.to_binary}

```rust
pub fn to_binary(snbt: &str, fmt: NbtFormat) -> Result<Vec<u8>>
```

把 SNBT 文本转成二进制。

- 参数：
    - snbt : `&str`
    - fmt : `NbtFormat`
- 返回值类型：`Result<Vec<u8>>`
- 对应槽位：[`nbt_snbt_to_binary`](../cpp/data.md#nbt_snbt_to_binary)

### `nbt::binary::from_binary` {#fn.from_binary}

```rust
pub fn from_binary(data: &[u8], fmt: NbtFormat) -> Result<String>
```

把二进制转成 SNBT 文本。

- 参数：
    - data : `&[u8]`
    - fmt : `NbtFormat`
- 返回值类型：`Result<String>`
- 对应槽位：[`nbt_binary_to_snbt`](../cpp/data.md#nbt_binary_to_snbt)

## `NbtValue` {#NbtValue}

```rust
pub enum NbtValue {
        Byte(i8),
        Short(i16),
        Int(i32),
        Long(i64),
        Float(f32),
        Double(f64),
        String(String),
        List(Vec<NbtValue>),
        Compound(BTreeMap<String, NbtValue>),
        ByteArray(Vec<i8>),
        IntArray(Vec<i32>),
        LongArray(Vec<i64>),
}
```

一个 NBT 值。各个变体和基岩版的标签类型一一对应。

变体的集合和早先的一代相同，所以现有的代码不用改就能编译。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`、`From`、`> From`、`Display`

### `NbtValue::compound` {#NbtValue.compound}

```rust
pub fn compound() -> NbtValue
```

一个空的复合标签。

- 返回值类型：`NbtValue`

### `NbtValue::obj` {#NbtValue.obj}

```rust
pub fn obj<K, I>(entries: I) -> NbtValue
    where
        K: Into<String>,
        I: IntoIterator<Item = (K, NbtValue)>,
```

直接用键值对构造一个复合标签。

```rust
let v = NbtValue::obj([
    ("x", 10.into()),
    ("name", "stone".into()),
]);
```

- 参数：
    - entries : `I`
- 返回值类型：`NbtValue`

### `NbtValue::list` {#NbtValue.list}

```rust
pub fn list<I: IntoIterator<Item = NbtValue>>(items: I) -> NbtValue
```

用一串值构造一个列表。

- 参数：
    - items : `I`
- 返回值类型：`NbtValue`

### `NbtValue::vec3` {#NbtValue.vec3}

```rust
pub fn vec3(x: f64, y: f64, z: f64) -> NbtValue
```

形如 `[x, y, z]` 的 double 列表，载荷里的坐标就是这个形状。

- 参数：
    - x : `f64`
    - y : `f64`
    - z : `f64`
- 返回值类型：`NbtValue`

### `NbtValue::parse` {#NbtValue.parse}

```rust
pub fn parse(text: &str) -> std::result::Result<NbtValue, ParseError>
```

解析一段 SNBT。

- 参数：
    - text : `&str`
- 返回值类型：`std::result::Result<NbtValue, ParseError>`

### `NbtValue::to_snbt` {#NbtValue.to_snbt}

```rust
pub fn to_snbt(&self) -> String
```

序列化成 SNBT。浮点数总是带 `d` 或 `f` 后缀，byte 带 `b`，long 带 `L`。没有后缀的话，另一边会把 `100.0` 读成 Int，写进去一个 double、读回来却失败，根源就在这里。

- 返回值类型：`String`

### `NbtValue::type_name` {#NbtValue.type_name}

```rust
pub fn type_name(&self) -> &'static str
```

这个值的类型名，只用在错误消息里。

- 返回值类型：`&'static str`

### `NbtValue::as_i64` {#NbtValue.as_i64}

```rust
pub fn as_i64(&self) -> Option<i64>
```

把任意整数类型转成 i64。浮点数不转换，因为 `3.7` 悄悄变成 `3` 会带来 bug。

- 返回值类型：`Option<i64>`

### `NbtValue::as_i32` {#NbtValue.as_i32}

```rust
pub fn as_i32(&self) -> Option<i32>
```

同上，收窄成 i32。超出范围时返回 `None`，不截断。

- 返回值类型：`Option<i32>`

### `NbtValue::as_f64` {#NbtValue.as_f64}

```rust
pub fn as_f64(&self) -> Option<f64>
```

把任意数值类型转成 f64，包括整数。

- 返回值类型：`Option<f64>`

### `NbtValue::as_bool` {#NbtValue.as_bool}

```rust
pub fn as_bool(&self) -> Option<bool>
```

布尔值。SNBT 里的布尔值是 `1b` 或 `0b`，所以接受任意整数类型，非零即为真。

- 返回值类型：`Option<bool>`

### `NbtValue::as_str` {#NbtValue.as_str}

```rust
pub fn as_str(&self) -> Option<&str>
```

- 返回值类型：`Option<&str>`

### `NbtValue::as_list` {#NbtValue.as_list}

```rust
pub fn as_list(&self) -> Option<&[NbtValue]>
```

- 返回值类型：`Option<&[NbtValue]>`

### `NbtValue::as_compound` {#NbtValue.as_compound}

```rust
pub fn as_compound(&self) -> Option<&BTreeMap<String, NbtValue>>
```

- 返回值类型：`Option<&BTreeMap<String, NbtValue>>`

### `NbtValue::as_compound_mut` {#NbtValue.as_compound_mut}

```rust
pub fn as_compound_mut(&mut self) -> Option<&mut BTreeMap<String, NbtValue>>
```

- 返回值类型：`Option<&mut BTreeMap<String, NbtValue>>`

### `NbtValue::as_vec3` {#NbtValue.as_vec3}

```rust
pub fn as_vec3(&self) -> Option<(f64, f64, f64)>
```

把 `[x, y, z]` 列表转成三元组。宿主表示坐标就用这个形状，比如 `edit_trace_ray` 的 `pos` 和 `block`，`actor_get_aabb` 的 `min` 和 `max`。

- 返回值类型：`Option<(f64, f64, f64)>`

### `NbtValue::as_block_pos` {#NbtValue.as_block_pos}

```rust
pub fn as_block_pos(&self) -> Option<(i32, i32, i32)>
```

同上，但用整数，用于方块坐标。

- 返回值类型：`Option<(i32, i32, i32)>`

### `NbtValue::is_compound` {#NbtValue.is_compound}

```rust
pub fn is_compound(&self) -> bool
```

- 返回值类型：`bool`

### `NbtValue::is_list` {#NbtValue.is_list}

```rust
pub fn is_list(&self) -> bool
```

- 返回值类型：`bool`

### `NbtValue::get` {#NbtValue.get}

```rust
pub fn get(&self, key: &str) -> Option<&NbtValue>
```

读取一个直接的子键。

- 参数：
    - key : `&str`
- 返回值类型：`Option<&NbtValue>`

### `NbtValue::get_mut` {#NbtValue.get_mut}

```rust
pub fn get_mut(&mut self, key: &str) -> Option<&mut NbtValue>
```

- 参数：
    - key : `&str`
- 返回值类型：`Option<&mut NbtValue>`

### `NbtValue::index` {#NbtValue.index}

```rust
pub fn index(&self, i: usize) -> Option<&NbtValue>
```

读取列表的第 i 项。

- 参数：
    - i : `usize`
- 返回值类型：`Option<&NbtValue>`

### `NbtValue::insert` {#NbtValue.insert}

```rust
pub fn insert(&mut self, key: impl Into<String>, value: NbtValue) -> bool
```

写入一个键。这个值不是复合标签时返回 false，永远不会悄悄把它变成复合标签。

- 参数：
    - key : `impl Into<String>`
    - value : `NbtValue`
- 返回值类型：`bool`

### `NbtValue::remove` {#NbtValue.remove}

```rust
pub fn remove(&mut self, key: &str) -> Option<NbtValue>
```

- 参数：
    - key : `&str`
- 返回值类型：`Option<NbtValue>`

### `NbtValue::contains` {#NbtValue.contains}

```rust
pub fn contains(&self, key: &str) -> bool
```

- 参数：
    - key : `&str`
- 返回值类型：`bool`

### `NbtValue::path` {#NbtValue.path}

```rust
pub fn path(&self, dotted: &str) -> Option<&NbtValue>
```

用点分隔、带数组下标的路径，例如 `"a.b[2].c"`。

早先的一代只支持纯粹的点，读一个碰撞箱 min 的 y 要写三行。

- 参数：
    - dotted : `&str`
- 返回值类型：`Option<&NbtValue>`

### `NbtValue::require` {#NbtValue.require}

```rust
pub fn require(&self, path: &str) -> NbtResult<&NbtValue>
```

按路径读取。缺少值时得到 `Missing`，里面带着完整的路径名。

- 参数：
    - path : `&str`
- 返回值类型：`NbtResult<&NbtValue>`

### `NbtValue::get_i64` {#NbtValue.get_i64}

```rust
pub fn get_i64(&self, path: &str) -> NbtResult<i64>
```

读取一个整数。缺少键得到 `Missing`，类型不符得到 `WrongType`。它从不拿 0 来充数。

- 参数：
    - path : `&str`
- 返回值类型：`NbtResult<i64>`

### `NbtValue::get_i32` {#NbtValue.get_i32}

```rust
pub fn get_i32(&self, path: &str) -> NbtResult<i32>
```

- 参数：
    - path : `&str`
- 返回值类型：`NbtResult<i32>`

### `NbtValue::get_f64` {#NbtValue.get_f64}

```rust
pub fn get_f64(&self, path: &str) -> NbtResult<f64>
```

- 参数：
    - path : `&str`
- 返回值类型：`NbtResult<f64>`

### `NbtValue::get_bool` {#NbtValue.get_bool}

```rust
pub fn get_bool(&self, path: &str) -> NbtResult<bool>
```

- 参数：
    - path : `&str`
- 返回值类型：`NbtResult<bool>`

### `NbtValue::get_str` {#NbtValue.get_str}

```rust
pub fn get_str(&self, path: &str) -> NbtResult<&str>
```

- 参数：
    - path : `&str`
- 返回值类型：`NbtResult<&str>`

### `NbtValue::get_list` {#NbtValue.get_list}

```rust
pub fn get_list(&self, path: &str) -> NbtResult<&[NbtValue]>
```

- 参数：
    - path : `&str`
- 返回值类型：`NbtResult<&[NbtValue]>`

### `NbtValue::get_vec3` {#NbtValue.get_vec3}

```rust
pub fn get_vec3(&self, path: &str) -> NbtResult<(f64, f64, f64)>
```

- 参数：
    - path : `&str`
- 返回值类型：`NbtResult<(f64, f64, f64)>`

### `NbtValue::get_block_pos` {#NbtValue.get_block_pos}

```rust
pub fn get_block_pos(&self, path: &str) -> NbtResult<(i32, i32, i32)>
```

- 参数：
    - path : `&str`
- 返回值类型：`NbtResult<(i32, i32, i32)>`

### `NbtValue::opt_i64` {#NbtValue.opt_i64}

```rust
pub fn opt_i64(&self, path: &str) -> Option<i64>
```

- 参数：
    - path : `&str`
- 返回值类型：`Option<i64>`

### `NbtValue::opt_i32` {#NbtValue.opt_i32}

```rust
pub fn opt_i32(&self, path: &str) -> Option<i32>
```

- 参数：
    - path : `&str`
- 返回值类型：`Option<i32>`

### `NbtValue::opt_f64` {#NbtValue.opt_f64}

```rust
pub fn opt_f64(&self, path: &str) -> Option<f64>
```

- 参数：
    - path : `&str`
- 返回值类型：`Option<f64>`

### `NbtValue::opt_bool` {#NbtValue.opt_bool}

```rust
pub fn opt_bool(&self, path: &str) -> Option<bool>
```

- 参数：
    - path : `&str`
- 返回值类型：`Option<bool>`

### `NbtValue::opt_str` {#NbtValue.opt_str}

```rust
pub fn opt_str(&self, path: &str) -> Option<&str>
```

- 参数：
    - path : `&str`
- 返回值类型：`Option<&str>`

### `NbtValue::first_str` {#NbtValue.first_str}

```rust
pub fn first_str(&self, paths: &[&str]) -> Option<&str>
```

按顺序尝试几条路径，返回第一个存在的字符串。

有这个方法的原因很具体：同一个概念在不同事件里的键名不一样，`player`、`_player.name` 或 `name`，业务代码已经在写这个循环了。

- 参数：
    - paths : `&[&str]`
- 返回值类型：`Option<&str>`

### `NbtValue::to_json` {#NbtValue.to_json}

```rust
pub fn to_json(&self) -> serde_json::Value
```

转成 `serde_json::Value`。类型后缀的信息会丢失，因为 JSON 不区分 byte 和 long，所以这条路适合保存、记日志和发给网页，不适合再转回来交给宿主。

- 返回值类型：`serde_json::Value`

### `NbtValue::from_json` {#NbtValue.from_json}

```rust
pub fn from_json(v: &serde_json::Value) -> NbtValue
```

从 `serde_json::Value` 转换。整数变成 `Long`，浮点数变成 `Double`，这是不丢信息的选择。更窄的类型要手工构造。

- 参数：
    - v : `&serde_json::Value`
- 返回值类型：`NbtValue`

### `NbtValue::to_binary` {#NbtValue.to_binary}

```rust
pub fn to_binary(&self, fmt: NbtFormat) -> Result<Vec<u8>>
```

把这棵树编码成二进制 NBT。

- 参数：
    - fmt : `NbtFormat`
- 返回值类型：`Result<Vec<u8>>`
- 对应槽位：[`nbt_snbt_to_binary`](../cpp/data.md#nbt_snbt_to_binary)

### `NbtValue::from_binary` {#NbtValue.from_binary}

```rust
pub fn from_binary(data: &[u8], fmt: NbtFormat) -> Result<NbtValue>
```

从二进制 NBT 解码出一棵树。

- 参数：
    - data : `&[u8]`
    - fmt : `NbtFormat`
- 返回值类型：`Result<NbtValue>`
- 对应槽位：[`nbt_binary_to_snbt`](../cpp/data.md#nbt_binary_to_snbt)

## `NbtFormat` {#NbtFormat}

```rust
pub enum NbtFormat {
        /// The little-endian form used in a save.
        LittleEndian = 0,
        /// The variable-length encoding used on the network.
        Network = 1,
}
```

二进制 NBT 的两种编码。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

### `NbtFormat::as_i32` {#NbtFormat.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- 返回值类型：`i32`

## `NbtError` {#NbtError}

```rust
pub enum NbtError {
        /// The key is simply not there.
        Missing { path: String },
        /// The key is there with a type other than the one requested.
        WrongType {
            path: String,
            want: &'static str,
            got: &'static str,
        },
        /// Reading a key on something that is not a compound tag, or an index on something that is
        /// not a list.
        NotIndexable { path: String, got: &'static str },
}
```

读取失败的原因。缺少键和类型不符是两回事，不能都合成 `None`。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`、`Eq`、`Display`、`Error`

## `NbtResult` {#NbtResult}

```rust
pub type NbtResult<T> = std::result::Result<T, NbtError>;
```

一次读取的结果。

## `ParseError` {#ParseError}

```rust
pub struct ParseError {
    pub at: usize,
    pub what: String,
}
```

解析失败。`at` 是出错处的字节偏移。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`、`Eq`、`Display`、`Error`
