# levilamina::kvdb

The key-value store: a mod's own persistent storage.

**This family is thread safe**

The ABI marks `kvdb_*` as internally locked, one of the exceptions of contract §4, so any thread
may call it. Almost every other domain works on the server thread alone, and this is one of the
few usable straight from a `std::thread::spawn`.

**Paths are confined to the mod's own data directory**

The host refuses `..` and an absolute path. The host owns the handle, force-closes it when the
mod unloads and warns, so forgetting to close loses no data while leaving a trace in the log.

## `KvDb` {#KvDb}

```rust
pub struct KvDb {
    // private fields
}
```

One open key-value store. Dropping it closes it.

- Implements: `Send`, `Sync`, `Drop`, `Debug`

### `KvDb::open` {#KvDb.open}

```rust
pub fn open(path: &str) -> Result<KvDb>
```

Opens it, creating it when it does not exist.

- Parameters:
    - path : `&str`
- Return type: `Result<KvDb>`
- Slots: [`kvdb_open`](../cpp/data.md#kvdb_open)

### `KvDb::open_existing` {#KvDb.open_existing}

```rust
pub fn open_existing(path: &str) -> Result<KvDb>
```

Opens an existing one only. A store that does not exist is an `Err` and no empty one is
quietly created: reading a store that should hold data and creating a new one are two
different things.

- Parameters:
    - path : `&str`
- Return type: `Result<KvDb>`
- Slots: [`kvdb_open`](../cpp/data.md#kvdb_open)

### `KvDb::path` {#KvDb.path}

```rust
pub fn path(&self) -> &str
```

- Return type: `&str`

### `KvDb::try_get` {#KvDb.try_get}

```rust
pub fn try_get(&self, key: &str) -> Result<Option<String>>
```

Reads one key: `Ok(None)` for a key that does not exist, `Err` for a host without
`kvdb_get`. A database the host closed on its own, at an unload, also reads as
`Ok(None)`: the slot answers both with the same false, and the ABI cannot tell them
apart.

Both gates apply even after `open` has succeeded: `kvdb_get` sits after `kvdb_open` in
the table at a larger offset, and the first being covered does not imply the second is.

- Parameters:
    - key : `&str`
- Return type: `Result<Option<String>>`
- Slots: [`kvdb_get`](../cpp/data.md#kvdb_get)

### `KvDb::get` {#KvDb.get}

```rust
pub fn get(&self, key: &str) -> Option<String>
```

Reads one key, with `None` for every case [`KvDb::try_get`](kvdb.md#KvDb.try_get) keeps apart.

- Parameters:
    - key : `&str`
- Return type: `Option<String>`
- Slots: [`kvdb_get`](../cpp/data.md#kvdb_get)

### `KvDb::set` {#KvDb.set}

```rust
pub fn set(&self, key: &str, value: &str) -> Result<()>
```

- Parameters:
    - key : `&str`
    - value : `&str`
- Return type: `Result<()>`
- Slots: [`kvdb_set`](../cpp/data.md#kvdb_set)

### `KvDb::del` {#KvDb.del}

```rust
pub fn del(&self, key: &str) -> Result<()>
```

Deletes one key. A key that never existed also counts as a success.

- Parameters:
    - key : `&str`
- Return type: `Result<()>`
- Slots: [`kvdb_del`](../cpp/data.md#kvdb_del)

### `KvDb::has` {#KvDb.has}

```rust
pub fn has(&self, key: &str) -> bool
```

- Parameters:
    - key : `&str`
- Return type: `bool`
- Slots: [`kvdb_has`](../cpp/data.md#kvdb_has), [`kvdb_has`](../cpp/data.md#kvdb_has)

### `KvDb::is_empty` {#KvDb.is_empty}

```rust
pub fn is_empty(&self) -> bool
```

- Return type: `bool`
- Slots: [`kvdb_is_empty`](../cpp/data.md#kvdb_is_empty), [`kvdb_is_empty`](../cpp/data.md#kvdb_is_empty)

### `KvDb::iter` {#KvDb.iter}

```rust
pub fn iter(&self) -> Vec<(String, String)>
```

Every key-value pair. The whole store is read into memory, so a large one needs care.

- Return type: `Vec<(String, String)>`
- Slots: [`kvdb_iter`](../cpp/data.md#kvdb_iter), [`kvdb_iter`](../cpp/data.md#kvdb_iter)
