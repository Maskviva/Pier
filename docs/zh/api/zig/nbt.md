# Zig：SNBT 值

nbt.zig：Zig 模组的 SNBT 解析和写出，规则和 Rust、Go 绑定一样：不带后缀的数字依次尝试 int、long、double；后缀 b、s、l、f 或 d 指定类型；true 和 false 是字节；其他不带引号的单词是字符串。

## 函数 {#functions}

### `nbt.parse` {#nbt.parse}

```zig
pub fn parse(arena: std.mem.Allocator, text: []const u8) ParseError!Value
```

解析 SNBT。每个节点都用 `arena` 分配，由调用方整体释放；不带引号的字符串或键也可能直接指向 `text`，所以 `text` 必须比结果存活得更久。

- 参数：
    - arena : `std.mem.Allocator`
    - text : `[]const u8`
- 返回值类型：`ParseError!Value`

## 声明 {#declarations}

### `nbt.max_depth` {#nbt.max_depth}

```zig
pub const max_depth = 512;
```

接受的最大嵌套深度，和 Minecraft 对 NBT 的上限一样。更深的数据会被拒绝，以免耗尽宿主线程的栈。

## `nbt.Value` {#nbt.Value}

```zig
pub const Value = union(enum) {
    byte: i8,
    short: i16,
    int: i32,
    long: i64,
    float: f32,
    double: f64,
    string: []const u8,
    list: []Value,
    compound: []Entry,
    byte_array: []i64,
    int_array: []i64,
    long_array: []i64,
    // ...
};
```

一个 SNBT 值。带类型的数组不论元素多宽，都用 i64 保存数字。

### `nbt.Value.get` {#nbt.Value.get}

```zig
pub fn get(self: Value, path: []const u8) ?Value
```

沿着用点分隔的路径穿过复合标签；某一步不存在时返回 null。

- 参数：
    - path : `[]const u8`
- 返回值类型：`?Value`

### `nbt.Value.getString` {#nbt.Value.getString}

```zig
pub fn getString(self: Value, path: []const u8) ?[]const u8
```

`path` 处的字符串；不存在或者不是字符串时返回 null。

- 参数：
    - path : `[]const u8`
- 返回值类型：`?[]const u8`

### `nbt.Value.getInt` {#nbt.Value.getInt}

```zig
pub fn getInt(self: Value, path: []const u8) ?i64
```

`path` 处任意宽度的整数；不存在或者不是整数时返回 null。

- 参数：
    - path : `[]const u8`
- 返回值类型：`?i64`

### `nbt.Value.getFloat` {#nbt.Value.getFloat}

```zig
pub fn getFloat(self: Value, path: []const u8) ?f64
```

`path` 处的数字，整数或浮点数都可以；不存在时返回 null。

- 参数：
    - path : `[]const u8`
- 返回值类型：`?f64`

### `nbt.Value.getBool` {#nbt.Value.getBool}

```zig
pub fn getBool(self: Value, path: []const u8) ?bool
```

把 `path` 处的字节当作布尔值读取；不存在，或者不是 0 或 1 时返回 null。

- 参数：
    - path : `[]const u8`
- 返回值类型：`?bool`

## `nbt.Entry` {#nbt.Entry}

```zig
pub const Entry = struct {
    key: []const u8,
    value: Value,
    // ...
};
```

复合标签的一个键和它的值。

## `nbt.ParseError` {#nbt.ParseError}

```zig
pub const ParseError = error{ Malformed, TooDeep, OutOfMemory };
```
