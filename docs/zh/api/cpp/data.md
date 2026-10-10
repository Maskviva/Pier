# NBT 与键值数据库

??? note "abi.h 里的分节说明"

    **§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）**

## 槽位 {#slots}

### `nbt_snbt_to_binary` {#nbt_snbt_to_binary}

```c
bool (*nbt_snbt_to_binary)(PierStr snbt, int32_t fmt, void* ctx, PierBytesSink sink);
```

`fmt`：0=磁盘格式（小端），1=网络格式。

- 调用形式：`api->nbt_snbt_to_binary(snbt, fmt, ctx, sink)`
- 参数：
    - snbt : `PierStr`
    - fmt : `int32_t`
    - ctx : `void*`
    - sink : `PierBytesSink`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 58 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`nbt::binary::to_binary`](../rust/nbt.md#fn.to_binary)、[`NbtValue::to_binary`](../rust/nbt.md#NbtValue.to_binary)
    - Go：[`SnbtToBinary`](../go/data.md#SnbtToBinary)

### `nbt_binary_to_snbt` {#nbt_binary_to_snbt}

```c
bool (*nbt_binary_to_snbt)(uint8_t const* data, size_t len, int32_t fmt, void* ctx, PierStrSink sink);
```

- 调用形式：`api->nbt_binary_to_snbt(data, len, fmt, ctx, sink)`
- 参数：
    - data : `uint8_t const*`
    - len : `size_t`
    - fmt : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 59 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`nbt::binary::from_binary`](../rust/nbt.md#fn.from_binary)、[`NbtValue::from_binary`](../rust/nbt.md#NbtValue.from_binary)
    - Go：[`Raw.NbtBinaryToSnbt`](../go/raw.md#Raw.NbtBinaryToSnbt)

### `kvdb_open` {#kvdb_open}

```c
PierKvDbHandle (*kvdb_open)(PierModHandle mod, PierStr path, bool create_if_missing);
```

!!! note "分组说明"

    KvDb：线程安全（内部有互斥锁）。路径限定在模组自己的数据目录里，带 `..` 的路径和绝对路径会被拒绝。句柄归加载器所有，模组卸载时会被强制关闭，并记一条警告。

- 调用形式：`api->kvdb_open(mod, path, create_if_missing)`
- 参数：
    - mod : `PierModHandle`
    - path : `PierStr`
    - create_if_missing : `bool`
- 返回值类型：`PierKvDbHandle`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 60 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`KvDb::open`](../rust/kvdb.md#KvDb.open)、[`KvDb::open_existing`](../rust/kvdb.md#KvDb.open_existing)
    - Go：[`OpenKvDb`](../go/data.md#OpenKvDb)、[`Raw.KvdbOpen`](../go/raw.md#Raw.KvdbOpen)

### `kvdb_close` {#kvdb_close}

```c
void (*kvdb_close)(PierKvDbHandle h);
```

!!! note "分组说明"

    KvDb：线程安全（内部有互斥锁）。路径限定在模组自己的数据目录里，带 `..` 的路径和绝对路径会被拒绝。句柄归加载器所有，模组卸载时会被强制关闭，并记一条警告。

- 调用形式：`api->kvdb_close(h)`
- 参数：
    - h : `PierKvDbHandle`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 61 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Go：[`KvDb.Close`](../go/data.md#KvDb.Close)、[`Raw.KvdbClose`](../go/raw.md#Raw.KvdbClose)

### `kvdb_get` {#kvdb_get}

```c
bool (*kvdb_get)(PierKvDbHandle h, PierStr key, void* ctx, PierStrSink sink);
```

!!! note "分组说明"

    KvDb：线程安全（内部有互斥锁）。路径限定在模组自己的数据目录里，带 `..` 的路径和绝对路径会被拒绝。句柄归加载器所有，模组卸载时会被强制关闭，并记一条警告。

- 调用形式：`api->kvdb_get(h, key, ctx, sink)`
- 参数：
    - h : `PierKvDbHandle`
    - key : `PierStr`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 62 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`KvDb::try_get`](../rust/kvdb.md#KvDb.try_get)、[`KvDb::get`](../rust/kvdb.md#KvDb.get)
    - Go：[`KvDb.Get`](../go/data.md#KvDb.Get)、[`Raw.KvdbGet`](../go/raw.md#Raw.KvdbGet)

### `kvdb_set` {#kvdb_set}

```c
bool (*kvdb_set)(PierKvDbHandle h, PierStr key, PierStr value);
```

!!! note "分组说明"

    KvDb：线程安全（内部有互斥锁）。路径限定在模组自己的数据目录里，带 `..` 的路径和绝对路径会被拒绝。句柄归加载器所有，模组卸载时会被强制关闭，并记一条警告。

- 调用形式：`api->kvdb_set(h, key, value)`
- 参数：
    - h : `PierKvDbHandle`
    - key : `PierStr`
    - value : `PierStr`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 63 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`KvDb::set`](../rust/kvdb.md#KvDb.set)
    - Go：[`KvDb.Set`](../go/data.md#KvDb.Set)、[`Raw.KvdbSet`](../go/raw.md#Raw.KvdbSet)

### `kvdb_del` {#kvdb_del}

```c
bool (*kvdb_del)(PierKvDbHandle h, PierStr key);
```

!!! note "分组说明"

    KvDb：线程安全（内部有互斥锁）。路径限定在模组自己的数据目录里，带 `..` 的路径和绝对路径会被拒绝。句柄归加载器所有，模组卸载时会被强制关闭，并记一条警告。

- 调用形式：`api->kvdb_del(h, key)`
- 参数：
    - h : `PierKvDbHandle`
    - key : `PierStr`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 64 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`KvDb::del`](../rust/kvdb.md#KvDb.del)
    - Go：[`KvDb.Delete`](../go/data.md#KvDb.Delete)、[`Raw.KvdbDel`](../go/raw.md#Raw.KvdbDel)

### `kvdb_has` {#kvdb_has}

```c
bool (*kvdb_has)(PierKvDbHandle h, PierStr key);
```

!!! note "分组说明"

    KvDb：线程安全（内部有互斥锁）。路径限定在模组自己的数据目录里，带 `..` 的路径和绝对路径会被拒绝。句柄归加载器所有，模组卸载时会被强制关闭，并记一条警告。

- 调用形式：`api->kvdb_has(h, key)`
- 参数：
    - h : `PierKvDbHandle`
    - key : `PierStr`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 65 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`KvDb::has`](../rust/kvdb.md#KvDb.has)、[`KvDb::has`](../rust/kvdb.md#KvDb.has)
    - Go：[`KvDb.Has`](../go/data.md#KvDb.Has)、[`Raw.KvdbHas`](../go/raw.md#Raw.KvdbHas)

### `kvdb_is_empty` {#kvdb_is_empty}

```c
bool (*kvdb_is_empty)(PierKvDbHandle h);
```

!!! note "分组说明"

    KvDb：线程安全（内部有互斥锁）。路径限定在模组自己的数据目录里，带 `..` 的路径和绝对路径会被拒绝。句柄归加载器所有，模组卸载时会被强制关闭，并记一条警告。

- 调用形式：`api->kvdb_is_empty(h)`
- 参数：
    - h : `PierKvDbHandle`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 66 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`KvDb::is_empty`](../rust/kvdb.md#KvDb.is_empty)、[`KvDb::is_empty`](../rust/kvdb.md#KvDb.is_empty)
    - Go：[`KvDb.IsEmpty`](../go/data.md#KvDb.IsEmpty)、[`Raw.KvdbIsEmpty`](../go/raw.md#Raw.KvdbIsEmpty)

### `kvdb_iter` {#kvdb_iter}

```c
void (*kvdb_iter)(PierKvDbHandle h, void* ctx, PierKvSink sink);
```

!!! note "分组说明"

    KvDb：线程安全（内部有互斥锁）。路径限定在模组自己的数据目录里，带 `..` 的路径和绝对路径会被拒绝。句柄归加载器所有，模组卸载时会被强制关闭，并记一条警告。

- 调用形式：`api->kvdb_iter(h, ctx, sink)`
- 参数：
    - h : `PierKvDbHandle`
    - ctx : `void*`
    - sink : `PierKvSink`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 67 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`KvDb::iter`](../rust/kvdb.md#KvDb.iter)、[`KvDb::iter`](../rust/kvdb.md#KvDb.iter)
    - Go：[`KvDb.Iter`](../go/data.md#KvDb.Iter)

## 类型 {#types}

### `PierBytesSink` {#PierBytesSink}

```c
typedef void (*PierBytesSink)(void* ctx, uint8_t const* data, size_t len);
```

原始字节的输出回调（二进制 NBT）。字节只在当前调用帧内有效。

### `PierKvSink` {#PierKvSink}

```c
typedef void (*PierKvSink)(void* ctx, PierStr key, PierStr value);
```

键值对的输出回调（`kvdb_iter`）。两个视图只在当前调用帧内有效。

### `PierKvDbHandle` {#PierKvDbHandle}

```c
typedef void* PierKvDbHandle;
```

指向一个已打开的键值数据库的不透明句柄，数据库归加载器所有。
