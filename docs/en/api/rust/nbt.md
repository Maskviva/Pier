# levilamina::nbt

NBT values: the carrier of all structured data across the boundary.

No struct is passed on the Pier ABI. Event payloads, forms, items, block states, actor
snapshots, command arguments, service requests and replies are all SNBT strings, which
makes reading a field out of some SNBT the most frequent operation in mod code.

Two families of accessors, with different meanings:

* `opt_*` returns an `Option`: give it to me if it is there and I will handle the rest.
* `get_*` returns a `Result`, where a missing key and a type mismatch are two different
  errors and the message carries the key name and the actual type. A protection
  decision, for permissions, plots or economy, uses this family and fails closed on an
  `Err`. [`crate::event`](event.md) gives the consequence of collapsing them into one answer.

## Functions {#functions}

### `nbt::binary::to_binary` {#fn.to_binary}

```rust
pub fn to_binary(snbt: &str, fmt: NbtFormat) -> Result<Vec<u8>>
```

Converts SNBT text into binary.

- Parameters:
    - snbt : `&str`
    - fmt : `NbtFormat`
- Return type: `Result<Vec<u8>>`
- Slots: [`nbt_snbt_to_binary`](../cpp/data.md#nbt_snbt_to_binary)

### `nbt::binary::from_binary` {#fn.from_binary}

```rust
pub fn from_binary(data: &[u8], fmt: NbtFormat) -> Result<String>
```

Converts binary into SNBT text.

- Parameters:
    - data : `&[u8]`
    - fmt : `NbtFormat`
- Return type: `Result<String>`
- Slots: [`nbt_binary_to_snbt`](../cpp/data.md#nbt_binary_to_snbt)

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

One NBT value. The variants correspond one to one with the Bedrock tag types.

The variant set matches the earlier generation, so existing code compiles unchanged.

- Implements: `Debug`, `Clone`, `PartialEq`, `From`, `> From`, `Display`

### `NbtValue::compound` {#NbtValue.compound}

```rust
pub fn compound() -> NbtValue
```

An empty compound tag.

- Return type: `NbtValue`

### `NbtValue::obj` {#NbtValue.obj}

```rust
pub fn obj<K, I>(entries: I) -> NbtValue
    where
        K: Into<String>,
        I: IntoIterator<Item = (K, NbtValue)>,
```

Builds a compound tag straight from key-value pairs.

```rust
let v = NbtValue::obj([
    ("x", 10.into()),
    ("name", "stone".into()),
]);
```

- Parameters:
    - entries : `I`
- Return type: `NbtValue`

### `NbtValue::list` {#NbtValue.list}

```rust
pub fn list<I: IntoIterator<Item = NbtValue>>(items: I) -> NbtValue
```

Builds a list from a sequence of values.

- Parameters:
    - items : `I`
- Return type: `NbtValue`

### `NbtValue::vec3` {#NbtValue.vec3}

```rust
pub fn vec3(x: f64, y: f64, z: f64) -> NbtValue
```

A double list shaped `[x, y, z]`, which is how a coordinate appears in a payload.

- Parameters:
    - x : `f64`
    - y : `f64`
    - z : `f64`
- Return type: `NbtValue`

### `NbtValue::parse` {#NbtValue.parse}

```rust
pub fn parse(text: &str) -> std::result::Result<NbtValue, ParseError>
```

Parses a piece of SNBT.

- Parameters:
    - text : `&str`
- Return type: `std::result::Result<NbtValue, ParseError>`

### `NbtValue::to_snbt` {#NbtValue.to_snbt}

```rust
pub fn to_snbt(&self) -> String
```

Serializes to SNBT. A float always carries a `d` or `f` suffix, a byte a `b` and a long
an `L`. Without the suffix the other side reads `100.0` as an Int, which is the source
of a double being written and then failing to read back.

- Return type: `String`

### `NbtValue::type_name` {#NbtValue.type_name}

```rust
pub fn type_name(&self) -> &'static str
```

The type name of this value, used only in error messages.

- Return type: `&'static str`

### `NbtValue::as_i64` {#NbtValue.as_i64}

```rust
pub fn as_i64(&self) -> Option<i64>
```

Any integer type into an i64. A float does not convert, since `3.7` quietly becoming
`3` breeds bugs.

- Return type: `Option<i64>`

### `NbtValue::as_i32` {#NbtValue.as_i32}

```rust
pub fn as_i32(&self) -> Option<i32>
```

As above, narrowed to an i32. Out of range returns `None` rather than truncating.

- Return type: `Option<i32>`

### `NbtValue::as_f64` {#NbtValue.as_f64}

```rust
pub fn as_f64(&self) -> Option<f64>
```

Any numeric type into an f64, integers included.

- Return type: `Option<f64>`

### `NbtValue::as_bool` {#NbtValue.as_bool}

```rust
pub fn as_bool(&self) -> Option<bool>
```

A boolean. In SNBT a boolean is `1b` or `0b`, so any integer type is accepted and
non-zero is true.

- Return type: `Option<bool>`

### `NbtValue::as_str` {#NbtValue.as_str}

```rust
pub fn as_str(&self) -> Option<&str>
```

- Return type: `Option<&str>`

### `NbtValue::as_list` {#NbtValue.as_list}

```rust
pub fn as_list(&self) -> Option<&[NbtValue]>
```

- Return type: `Option<&[NbtValue]>`

### `NbtValue::as_compound` {#NbtValue.as_compound}

```rust
pub fn as_compound(&self) -> Option<&BTreeMap<String, NbtValue>>
```

- Return type: `Option<&BTreeMap<String, NbtValue>>`

### `NbtValue::as_compound_mut` {#NbtValue.as_compound_mut}

```rust
pub fn as_compound_mut(&mut self) -> Option<&mut BTreeMap<String, NbtValue>>
```

- Return type: `Option<&mut BTreeMap<String, NbtValue>>`

### `NbtValue::as_vec3` {#NbtValue.as_vec3}

```rust
pub fn as_vec3(&self) -> Option<(f64, f64, f64)>
```

An `[x, y, z]` list into a triple. This is the shape the host uses for coordinates, as
in `pos` and `block` of `edit_trace_ray` and `min` and `max` of `actor_get_aabb`.

- Return type: `Option<(f64, f64, f64)>`

### `NbtValue::as_block_pos` {#NbtValue.as_block_pos}

```rust
pub fn as_block_pos(&self) -> Option<(i32, i32, i32)>
```

As above but as integers, for a block coordinate.

- Return type: `Option<(i32, i32, i32)>`

### `NbtValue::is_compound` {#NbtValue.is_compound}

```rust
pub fn is_compound(&self) -> bool
```

- Return type: `bool`

### `NbtValue::is_list` {#NbtValue.is_list}

```rust
pub fn is_list(&self) -> bool
```

- Return type: `bool`

### `NbtValue::get` {#NbtValue.get}

```rust
pub fn get(&self, key: &str) -> Option<&NbtValue>
```

Reads a direct child key.

- Parameters:
    - key : `&str`
- Return type: `Option<&NbtValue>`

### `NbtValue::get_mut` {#NbtValue.get_mut}

```rust
pub fn get_mut(&mut self, key: &str) -> Option<&mut NbtValue>
```

- Parameters:
    - key : `&str`
- Return type: `Option<&mut NbtValue>`

### `NbtValue::index` {#NbtValue.index}

```rust
pub fn index(&self, i: usize) -> Option<&NbtValue>
```

Reads item i of a list.

- Parameters:
    - i : `usize`
- Return type: `Option<&NbtValue>`

### `NbtValue::insert` {#NbtValue.insert}

```rust
pub fn insert(&mut self, key: impl Into<String>, value: NbtValue) -> bool
```

Writes a key. It returns false when this is not a compound tag and never quietly turns
it into one.

- Parameters:
    - key : `impl Into<String>`
    - value : `NbtValue`
- Return type: `bool`

### `NbtValue::remove` {#NbtValue.remove}

```rust
pub fn remove(&mut self, key: &str) -> Option<NbtValue>
```

- Parameters:
    - key : `&str`
- Return type: `Option<NbtValue>`

### `NbtValue::contains` {#NbtValue.contains}

```rust
pub fn contains(&self, key: &str) -> bool
```

- Parameters:
    - key : `&str`
- Return type: `bool`

### `NbtValue::path` {#NbtValue.path}

```rust
pub fn path(&self, dotted: &str) -> Option<&NbtValue>
```

A dotted path with array indices, such as `"a.b[2].c"`.

An earlier generation supported plain dots only, so reading the y of the min of an aabb
took three lines.

- Parameters:
    - dotted : `&str`
- Return type: `Option<&NbtValue>`

### `NbtValue::require` {#NbtValue.require}

```rust
pub fn require(&self, path: &str) -> NbtResult<&NbtValue>
```

Reads by path. A missing value is a `Missing` carrying the full path name.

- Parameters:
    - path : `&str`
- Return type: `NbtResult<&NbtValue>`

### `NbtValue::get_i64` {#NbtValue.get_i64}

```rust
pub fn get_i64(&self, path: &str) -> NbtResult<i64>
```

Reads an integer. A missing key is a `Missing` and a mismatched type a `WrongType`. It
never gives back a 0.

- Parameters:
    - path : `&str`
- Return type: `NbtResult<i64>`

### `NbtValue::get_i32` {#NbtValue.get_i32}

```rust
pub fn get_i32(&self, path: &str) -> NbtResult<i32>
```

- Parameters:
    - path : `&str`
- Return type: `NbtResult<i32>`

### `NbtValue::get_f64` {#NbtValue.get_f64}

```rust
pub fn get_f64(&self, path: &str) -> NbtResult<f64>
```

- Parameters:
    - path : `&str`
- Return type: `NbtResult<f64>`

### `NbtValue::get_bool` {#NbtValue.get_bool}

```rust
pub fn get_bool(&self, path: &str) -> NbtResult<bool>
```

- Parameters:
    - path : `&str`
- Return type: `NbtResult<bool>`

### `NbtValue::get_str` {#NbtValue.get_str}

```rust
pub fn get_str(&self, path: &str) -> NbtResult<&str>
```

- Parameters:
    - path : `&str`
- Return type: `NbtResult<&str>`

### `NbtValue::get_list` {#NbtValue.get_list}

```rust
pub fn get_list(&self, path: &str) -> NbtResult<&[NbtValue]>
```

- Parameters:
    - path : `&str`
- Return type: `NbtResult<&[NbtValue]>`

### `NbtValue::get_vec3` {#NbtValue.get_vec3}

```rust
pub fn get_vec3(&self, path: &str) -> NbtResult<(f64, f64, f64)>
```

- Parameters:
    - path : `&str`
- Return type: `NbtResult<(f64, f64, f64)>`

### `NbtValue::get_block_pos` {#NbtValue.get_block_pos}

```rust
pub fn get_block_pos(&self, path: &str) -> NbtResult<(i32, i32, i32)>
```

- Parameters:
    - path : `&str`
- Return type: `NbtResult<(i32, i32, i32)>`

### `NbtValue::opt_i64` {#NbtValue.opt_i64}

```rust
pub fn opt_i64(&self, path: &str) -> Option<i64>
```

- Parameters:
    - path : `&str`
- Return type: `Option<i64>`

### `NbtValue::opt_i32` {#NbtValue.opt_i32}

```rust
pub fn opt_i32(&self, path: &str) -> Option<i32>
```

- Parameters:
    - path : `&str`
- Return type: `Option<i32>`

### `NbtValue::opt_f64` {#NbtValue.opt_f64}

```rust
pub fn opt_f64(&self, path: &str) -> Option<f64>
```

- Parameters:
    - path : `&str`
- Return type: `Option<f64>`

### `NbtValue::opt_bool` {#NbtValue.opt_bool}

```rust
pub fn opt_bool(&self, path: &str) -> Option<bool>
```

- Parameters:
    - path : `&str`
- Return type: `Option<bool>`

### `NbtValue::opt_str` {#NbtValue.opt_str}

```rust
pub fn opt_str(&self, path: &str) -> Option<&str>
```

- Parameters:
    - path : `&str`
- Return type: `Option<&str>`

### `NbtValue::first_str` {#NbtValue.first_str}

```rust
pub fn first_str(&self, paths: &[&str]) -> Option<&str>
```

Tries several paths in order and returns the first string that exists.

The reason for this method is concrete: the same concept has different key names across
events, `player`, `_player.name` or `name`, and business code was already writing this
loop.

- Parameters:
    - paths : `&[&str]`
- Return type: `Option<&str>`

### `NbtValue::to_json` {#NbtValue.to_json}

```rust
pub fn to_json(&self) -> serde_json::Value
```

Converts into a `serde_json::Value`. The type suffix information is lost, since JSON
does not distinguish byte from long, so this route suits saving, logging and sending to
the web, and not converting back to feed the host.

- Return type: `serde_json::Value`

### `NbtValue::from_json` {#NbtValue.from_json}

```rust
pub fn from_json(v: &serde_json::Value) -> NbtValue
```

Converts from a `serde_json::Value`. An integer becomes a `Long` and a float a
`Double`, which is the lossless choice. A narrower type has to be constructed by hand.

- Parameters:
    - v : `&serde_json::Value`
- Return type: `NbtValue`

### `NbtValue::to_binary` {#NbtValue.to_binary}

```rust
pub fn to_binary(&self, fmt: NbtFormat) -> Result<Vec<u8>>
```

Encodes this tree into binary NBT.

- Parameters:
    - fmt : `NbtFormat`
- Return type: `Result<Vec<u8>>`
- Slots: [`nbt_snbt_to_binary`](../cpp/data.md#nbt_snbt_to_binary)

### `NbtValue::from_binary` {#NbtValue.from_binary}

```rust
pub fn from_binary(data: &[u8], fmt: NbtFormat) -> Result<NbtValue>
```

Decodes a tree from binary NBT.

- Parameters:
    - data : `&[u8]`
    - fmt : `NbtFormat`
- Return type: `Result<NbtValue>`
- Slots: [`nbt_binary_to_snbt`](../cpp/data.md#nbt_binary_to_snbt)

## `NbtFormat` {#NbtFormat}

```rust
pub enum NbtFormat {
        /// The little-endian form used in a save.
        LittleEndian = 0,
        /// The variable-length encoding used on the network.
        Network = 1,
}
```

The two encodings of binary NBT.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

### `NbtFormat::as_i32` {#NbtFormat.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- Return type: `i32`

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

Why a read failed. A missing key and a type mismatch are two different things and must
not both collapse into `None`.

- Implements: `Debug`, `Clone`, `PartialEq`, `Eq`, `Display`, `Error`

## `NbtResult` {#NbtResult}

```rust
pub type NbtResult<T> = std::result::Result<T, NbtError>;
```

The result of a read.

## `ParseError` {#ParseError}

```rust
pub struct ParseError {
    pub at: usize,
    pub what: String,
}
```

A parse failure. `at` is the byte offset of the fault.

- Implements: `Debug`, `Clone`, `PartialEq`, `Eq`, `Display`, `Error`
