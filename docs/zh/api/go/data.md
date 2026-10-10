# Go：NBT 与键值数据库

## 函数 {#functions}

### `SnbtToBinary` {#SnbtToBinary}

```go
func SnbtToBinary(snbt string, format int32) ([]byte, error)
```

把 SNBT 编码成二进制 NBT：格式 0 是小端的磁盘布局，格式 1 是网络布局。

- 参数：
    - snbt : `string`
    - format : `int32`
- 返回值类型：`([]byte, error)`
- 对应槽位：[`nbt_snbt_to_binary`](../cpp/data.md#nbt_snbt_to_binary)

### `OpenKvDb` {#OpenKvDb}

```go
func OpenKvDb(path string, createIfMissing bool) (*KvDb, error)
```

打开 `path` 处的存储；设置了 `createIfMissing` 时，不存在就创建。

- 参数：
    - path : `string`
    - createIfMissing : `bool`
- 返回值类型：`(*KvDb, error)`
- 对应槽位：[`kvdb_open`](../cpp/data.md#kvdb_open)

## `KvDb` {#KvDb}

```go
type KvDb struct {
    // unexported fields
}
```

一个已打开的键值存储，由宿主保存在模组的数据目录下。

### `KvDb.Path` {#KvDb.Path}

```go
func (db *KvDb) Path() string
```

打开存储时用的路径。

- 返回值类型：`string`

### `KvDb.Get` {#KvDb.Get}

```go
func (db *KvDb) Get(key string) (value string, found bool, err error)
```

读取一个键；键不存在时 `found` 为 false。宿主自己关掉的存储（卸载时）读起来也是不存在：ABI 对这两种情况给出同样的回答。

- 参数：
    - key : `string`
- 返回值类型：`(value string, found bool, err error)`
- 对应槽位：[`kvdb_get`](../cpp/data.md#kvdb_get)

### `KvDb.Set` {#KvDb.Set}

```go
func (db *KvDb) Set(key, value string) error
```

写入一个键。

- 参数：
    - key : `string`
    - value : `string`
- 返回值类型：`error`
- 对应槽位：[`kvdb_set`](../cpp/data.md#kvdb_set)

### `KvDb.Delete` {#KvDb.Delete}

```go
func (db *KvDb) Delete(key string) error
```

删除一个键。

- 参数：
    - key : `string`
- 返回值类型：`error`
- 对应槽位：[`kvdb_del`](../cpp/data.md#kvdb_del)

### `KvDb.Has` {#KvDb.Has}

```go
func (db *KvDb) Has(key string) (bool, error)
```

判断一个键是否存在。

- 参数：
    - key : `string`
- 返回值类型：`(bool, error)`
- 对应槽位：[`kvdb_has`](../cpp/data.md#kvdb_has)

### `KvDb.IsEmpty` {#KvDb.IsEmpty}

```go
func (db *KvDb) IsEmpty() (bool, error)
```

判断存储里是否一个键都没有。

- 返回值类型：`(bool, error)`
- 对应槽位：[`kvdb_is_empty`](../cpp/data.md#kvdb_is_empty)

### `KvDb.Iter` {#KvDb.Iter}

```go
func (db *KvDb) Iter() ([]KeyValue, error)
```

列出所有条目。

- 返回值类型：`([]KeyValue, error)`
- 对应槽位：[`kvdb_iter`](../cpp/data.md#kvdb_iter)

### `KvDb.Close` {#KvDb.Close}

```go
func (db *KvDb) Close() error
```

关闭存储；之后再使用会失败。

- 返回值类型：`error`
- 对应槽位：[`kvdb_close`](../cpp/data.md#kvdb_close)

## `KvDbHandle` {#KvDbHandle}

```go
type KvDbHandle struct{ p unsafe.Pointer }
type KvDbHandle struct{ p unsafe.Pointer }
```

一个已打开的键值存储；零值表示没有存储。

### `KvDbHandle.IsZero` {#KvDbHandle.IsZero}

```go
func (h KvDbHandle) IsZero() bool
```

判断这个句柄是否什么都不指向。

- 返回值类型：`bool`

## `KeyValue` {#KeyValue}

```go
type KeyValue struct {
    Key   string
    Value string
}
```

键值存储里的一个条目。
