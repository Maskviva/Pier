# NBT and the key-value database

??? note "Section notes in abi.h"

    **§I NBT binary, KvDb (thread-safe), system & server info**

## Slots {#slots}

### `nbt_snbt_to_binary` {#nbt_snbt_to_binary}

```c
bool (*nbt_snbt_to_binary)(PierStr snbt, int32_t fmt, void* ctx, PierBytesSink sink);
```

fmt: 0=disk little-endian, 1=network.

- Call: `api->nbt_snbt_to_binary(snbt, fmt, ctx, sink)`
- Parameters:
    - snbt : `PierStr`
    - fmt : `int32_t`
    - ctx : `void*`
    - sink : `PierBytesSink`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 58, counting from 0
- Callers in each binding:
    - Rust: [`nbt::binary::to_binary`](../rust/nbt.md#fn.to_binary), [`NbtValue::to_binary`](../rust/nbt.md#NbtValue.to_binary)
    - Go: [`SnbtToBinary`](../go/data.md#SnbtToBinary)

### `nbt_binary_to_snbt` {#nbt_binary_to_snbt}

```c
bool (*nbt_binary_to_snbt)(uint8_t const* data, size_t len, int32_t fmt, void* ctx, PierStrSink sink);
```

- Call: `api->nbt_binary_to_snbt(data, len, fmt, ctx, sink)`
- Parameters:
    - data : `uint8_t const*`
    - len : `size_t`
    - fmt : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 59, counting from 0
- Callers in each binding:
    - Rust: [`nbt::binary::from_binary`](../rust/nbt.md#fn.from_binary), [`NbtValue::from_binary`](../rust/nbt.md#NbtValue.from_binary)
    - Go: [`Raw.NbtBinaryToSnbt`](../go/raw.md#Raw.NbtBinaryToSnbt)

### `kvdb_open` {#kvdb_open}

```c
PierKvDbHandle (*kvdb_open)(PierModHandle mod, PierStr path, bool create_if_missing);
```

!!! note "Group note"

    KvDb: THREAD-SAFE (internal mutex). Paths are confined to the mod's own data directory; ".." and absolute paths are rejected. Handles are owned by the loader and force-closed (with a warning) at mod unload.

- Call: `api->kvdb_open(mod, path, create_if_missing)`
- Parameters:
    - mod : `PierModHandle`
    - path : `PierStr`
    - create_if_missing : `bool`
- Return type: `PierKvDbHandle`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 60, counting from 0
- Callers in each binding:
    - Rust: [`KvDb::open`](../rust/kvdb.md#KvDb.open), [`KvDb::open_existing`](../rust/kvdb.md#KvDb.open_existing)
    - Go: [`OpenKvDb`](../go/data.md#OpenKvDb), [`Raw.KvdbOpen`](../go/raw.md#Raw.KvdbOpen)

### `kvdb_close` {#kvdb_close}

```c
void (*kvdb_close)(PierKvDbHandle h);
```

!!! note "Group note"

    KvDb: THREAD-SAFE (internal mutex). Paths are confined to the mod's own data directory; ".." and absolute paths are rejected. Handles are owned by the loader and force-closed (with a warning) at mod unload.

- Call: `api->kvdb_close(h)`
- Parameters:
    - h : `PierKvDbHandle`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 61, counting from 0
- Callers in each binding:
    - Go: [`KvDb.Close`](../go/data.md#KvDb.Close), [`Raw.KvdbClose`](../go/raw.md#Raw.KvdbClose)

### `kvdb_get` {#kvdb_get}

```c
bool (*kvdb_get)(PierKvDbHandle h, PierStr key, void* ctx, PierStrSink sink);
```

!!! note "Group note"

    KvDb: THREAD-SAFE (internal mutex). Paths are confined to the mod's own data directory; ".." and absolute paths are rejected. Handles are owned by the loader and force-closed (with a warning) at mod unload.

- Call: `api->kvdb_get(h, key, ctx, sink)`
- Parameters:
    - h : `PierKvDbHandle`
    - key : `PierStr`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 62, counting from 0
- Callers in each binding:
    - Rust: [`KvDb::try_get`](../rust/kvdb.md#KvDb.try_get), [`KvDb::get`](../rust/kvdb.md#KvDb.get)
    - Go: [`KvDb.Get`](../go/data.md#KvDb.Get), [`Raw.KvdbGet`](../go/raw.md#Raw.KvdbGet)

### `kvdb_set` {#kvdb_set}

```c
bool (*kvdb_set)(PierKvDbHandle h, PierStr key, PierStr value);
```

!!! note "Group note"

    KvDb: THREAD-SAFE (internal mutex). Paths are confined to the mod's own data directory; ".." and absolute paths are rejected. Handles are owned by the loader and force-closed (with a warning) at mod unload.

- Call: `api->kvdb_set(h, key, value)`
- Parameters:
    - h : `PierKvDbHandle`
    - key : `PierStr`
    - value : `PierStr`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 63, counting from 0
- Callers in each binding:
    - Rust: [`KvDb::set`](../rust/kvdb.md#KvDb.set)
    - Go: [`KvDb.Set`](../go/data.md#KvDb.Set), [`Raw.KvdbSet`](../go/raw.md#Raw.KvdbSet)

### `kvdb_del` {#kvdb_del}

```c
bool (*kvdb_del)(PierKvDbHandle h, PierStr key);
```

!!! note "Group note"

    KvDb: THREAD-SAFE (internal mutex). Paths are confined to the mod's own data directory; ".." and absolute paths are rejected. Handles are owned by the loader and force-closed (with a warning) at mod unload.

- Call: `api->kvdb_del(h, key)`
- Parameters:
    - h : `PierKvDbHandle`
    - key : `PierStr`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 64, counting from 0
- Callers in each binding:
    - Rust: [`KvDb::del`](../rust/kvdb.md#KvDb.del)
    - Go: [`KvDb.Delete`](../go/data.md#KvDb.Delete), [`Raw.KvdbDel`](../go/raw.md#Raw.KvdbDel)

### `kvdb_has` {#kvdb_has}

```c
bool (*kvdb_has)(PierKvDbHandle h, PierStr key);
```

!!! note "Group note"

    KvDb: THREAD-SAFE (internal mutex). Paths are confined to the mod's own data directory; ".." and absolute paths are rejected. Handles are owned by the loader and force-closed (with a warning) at mod unload.

- Call: `api->kvdb_has(h, key)`
- Parameters:
    - h : `PierKvDbHandle`
    - key : `PierStr`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 65, counting from 0
- Callers in each binding:
    - Rust: [`KvDb::has`](../rust/kvdb.md#KvDb.has), [`KvDb::has`](../rust/kvdb.md#KvDb.has)
    - Go: [`KvDb.Has`](../go/data.md#KvDb.Has), [`Raw.KvdbHas`](../go/raw.md#Raw.KvdbHas)

### `kvdb_is_empty` {#kvdb_is_empty}

```c
bool (*kvdb_is_empty)(PierKvDbHandle h);
```

!!! note "Group note"

    KvDb: THREAD-SAFE (internal mutex). Paths are confined to the mod's own data directory; ".." and absolute paths are rejected. Handles are owned by the loader and force-closed (with a warning) at mod unload.

- Call: `api->kvdb_is_empty(h)`
- Parameters:
    - h : `PierKvDbHandle`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 66, counting from 0
- Callers in each binding:
    - Rust: [`KvDb::is_empty`](../rust/kvdb.md#KvDb.is_empty), [`KvDb::is_empty`](../rust/kvdb.md#KvDb.is_empty)
    - Go: [`KvDb.IsEmpty`](../go/data.md#KvDb.IsEmpty), [`Raw.KvdbIsEmpty`](../go/raw.md#Raw.KvdbIsEmpty)

### `kvdb_iter` {#kvdb_iter}

```c
void (*kvdb_iter)(PierKvDbHandle h, void* ctx, PierKvSink sink);
```

!!! note "Group note"

    KvDb: THREAD-SAFE (internal mutex). Paths are confined to the mod's own data directory; ".." and absolute paths are rejected. Handles are owned by the loader and force-closed (with a warning) at mod unload.

- Call: `api->kvdb_iter(h, ctx, sink)`
- Parameters:
    - h : `PierKvDbHandle`
    - ctx : `void*`
    - sink : `PierKvSink`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 67, counting from 0
- Callers in each binding:
    - Rust: [`KvDb::iter`](../rust/kvdb.md#KvDb.iter), [`KvDb::iter`](../rust/kvdb.md#KvDb.iter)
    - Go: [`KvDb.Iter`](../go/data.md#KvDb.Iter)

## Types {#types}

### `PierBytesSink` {#PierBytesSink}

```c
typedef void (*PierBytesSink)(void* ctx, uint8_t const* data, size_t len);
```

Raw byte sink (binary NBT). Bytes valid only within the call frame.

### `PierKvSink` {#PierKvSink}

```c
typedef void (*PierKvSink)(void* ctx, PierStr key, PierStr value);
```

Key/value sink (`kvdb_iter`). Views valid only within the call frame.

### `PierKvDbHandle` {#PierKvDbHandle}

```c
typedef void* PierKvDbHandle;
```

Opaque handle to an open key-value database owned by the loader.
