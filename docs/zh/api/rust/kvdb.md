# levilamina::kvdb · 键值数据库

键值存储：模组自己的持久化存储。

**这一组接口是线程安全的**

ABI 把 `kvdb_*` 标为内部加锁，是契约 §4 的例外之一，所以任何线程都可以调用。几乎所有其他领域都只能在服务器线程上工作，这是少数几个可以直接在 `std::thread::spawn` 里用的。

**路径限定在模组自己的数据目录里**

宿主拒绝 `..` 和绝对路径。句柄归宿主所有，模组卸载时宿主会强制关闭它并发出警告，所以忘了关闭不会丢数据，但会在日志里留下记录。

## `KvDb` {#KvDb}

```rust
pub struct KvDb {
    // private fields
}
```

一个已打开的键值存储。丢弃它就会关闭。

- 实现的 trait：`Send`、`Sync`、`Drop`、`Debug`

### `KvDb::open` {#KvDb.open}

```rust
pub fn open(path: &str) -> Result<KvDb>
```

打开它，不存在时创建。

- 参数：
    - path : `&str`
- 返回值类型：`Result<KvDb>`
- 对应槽位：[`kvdb_open`](../cpp/data.md#kvdb_open)

### `KvDb::open_existing` {#KvDb.open_existing}

```rust
pub fn open_existing(path: &str) -> Result<KvDb>
```

只打开已存在的存储。存储不存在时返回 `Err`，不会悄悄建一个空的：读取一个本该有数据的存储，和新建一个存储，是两件不同的事。

- 参数：
    - path : `&str`
- 返回值类型：`Result<KvDb>`
- 对应槽位：[`kvdb_open`](../cpp/data.md#kvdb_open)

### `KvDb::path` {#KvDb.path}

```rust
pub fn path(&self) -> &str
```

- 返回值类型：`&str`

### `KvDb::try_get` {#KvDb.try_get}

```rust
pub fn try_get(&self, key: &str) -> Result<Option<String>>
```

读取一个键：键不存在时返回 `Ok(None)`，宿主没有 `kvdb_get` 时返回 `Err`。宿主自己关掉的数据库（卸载时）读起来也是 `Ok(None)`：槽位对这两种情况回答同一个 false，ABI 分不清它们。

即使 `open` 已经成功，两道关卡也照样要检查：`kvdb_get` 在表里排在 `kvdb_open` 之后，偏移更大，前者够得着并不代表后者也够得着。

- 参数：
    - key : `&str`
- 返回值类型：`Result<Option<String>>`
- 对应槽位：[`kvdb_get`](../cpp/data.md#kvdb_get)

### `KvDb::get` {#KvDb.get}

```rust
pub fn get(&self, key: &str) -> Option<String>
```

读取一个键，[`KvDb::try_get`](kvdb.md#KvDb.try_get) 区分开的所有情况在这里都是 `None`。

- 参数：
    - key : `&str`
- 返回值类型：`Option<String>`
- 对应槽位：[`kvdb_get`](../cpp/data.md#kvdb_get)

### `KvDb::set` {#KvDb.set}

```rust
pub fn set(&self, key: &str, value: &str) -> Result<()>
```

- 参数：
    - key : `&str`
    - value : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`kvdb_set`](../cpp/data.md#kvdb_set)

### `KvDb::del` {#KvDb.del}

```rust
pub fn del(&self, key: &str) -> Result<()>
```

删除一个键。从来不存在的键也算成功。

- 参数：
    - key : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`kvdb_del`](../cpp/data.md#kvdb_del)

### `KvDb::has` {#KvDb.has}

```rust
pub fn has(&self, key: &str) -> bool
```

- 参数：
    - key : `&str`
- 返回值类型：`bool`
- 对应槽位：[`kvdb_has`](../cpp/data.md#kvdb_has)、[`kvdb_has`](../cpp/data.md#kvdb_has)

### `KvDb::is_empty` {#KvDb.is_empty}

```rust
pub fn is_empty(&self) -> bool
```

- 返回值类型：`bool`
- 对应槽位：[`kvdb_is_empty`](../cpp/data.md#kvdb_is_empty)、[`kvdb_is_empty`](../cpp/data.md#kvdb_is_empty)

### `KvDb::iter` {#KvDb.iter}

```rust
pub fn iter(&self) -> Vec<(String, String)>
```

所有的键值对。整个存储都会读进内存，所以大的存储要当心。

- 返回值类型：`Vec<(String, String)>`
- 对应槽位：[`kvdb_iter`](../cpp/data.md#kvdb_iter)、[`kvdb_iter`](../cpp/data.md#kvdb_iter)
