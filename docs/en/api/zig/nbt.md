# Zig: SNBT values

nbt.zig: SNBT parsing and writing for Zig mods, with the rules of the Rust and Go bindings: a bare number is an int, then a long, then a double; a suffix b, s, l, f or d fixes the type; true and false are bytes; any other bare word is a string.

## Functions {#functions}

### `nbt.parse` {#nbt.parse}

```zig
pub fn parse(arena: std.mem.Allocator, text: []const u8) ParseError!Value
```

Parses SNBT. Every node is allocated with `arena`, which the caller frees as a whole; a bare string or key may also point into `text`, which must outlive the result.

- Parameters:
    - arena : `std.mem.Allocator`
    - text : `[]const u8`
- Return type: `ParseError!Value`

## Declarations {#declarations}

### `nbt.max_depth` {#nbt.max_depth}

```zig
pub const max_depth = 512;
```

The deepest nesting accepted, the cap Minecraft puts on NBT. A deeper payload is refused instead of exhausting the stack of a host thread.

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

One SNBT value. Typed arrays hold their numbers as i64 whatever the element width.

### `nbt.Value.get` {#nbt.Value.get}

```zig
pub fn get(self: Value, path: []const u8) ?Value
```

Follows a dotted path through compounds; null when a step is missing.

- Parameters:
    - path : `[]const u8`
- Return type: `?Value`

### `nbt.Value.getString` {#nbt.Value.getString}

```zig
pub fn getString(self: Value, path: []const u8) ?[]const u8
```

The string at path, or null when it is absent or not a string.

- Parameters:
    - path : `[]const u8`
- Return type: `?[]const u8`

### `nbt.Value.getInt` {#nbt.Value.getInt}

```zig
pub fn getInt(self: Value, path: []const u8) ?i64
```

The integer at path of any width, or null when it is absent or not one.

- Parameters:
    - path : `[]const u8`
- Return type: `?i64`

### `nbt.Value.getFloat` {#nbt.Value.getFloat}

```zig
pub fn getFloat(self: Value, path: []const u8) ?f64
```

The number at path, integer or floating, or null when it is absent.

- Parameters:
    - path : `[]const u8`
- Return type: `?f64`

### `nbt.Value.getBool` {#nbt.Value.getBool}

```zig
pub fn getBool(self: Value, path: []const u8) ?bool
```

The byte at path read as a boolean, or null when it is absent or not 0 or 1.

- Parameters:
    - path : `[]const u8`
- Return type: `?bool`

## `nbt.Entry` {#nbt.Entry}

```zig
pub const Entry = struct {
    key: []const u8,
    value: Value,
    // ...
};
```

One key of a compound and its value.

## `nbt.ParseError` {#nbt.ParseError}

```zig
pub const ParseError = error{ Malformed, TooDeep, OutOfMemory };
```
