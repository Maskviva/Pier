# Go: NBT and the key-value database

## Functions {#functions}

### `SnbtToBinary` {#SnbtToBinary}

```go
func SnbtToBinary(snbt string, format int32) ([]byte, error)
```

SnbtToBinary encodes SNBT as binary NBT in format 0, the little-endian disk layout, or format 1, the network layout.

- Parameters:
    - snbt : `string`
    - format : `int32`
- Return type: `([]byte, error)`
- Slots: [`nbt_snbt_to_binary`](../cpp/data.md#nbt_snbt_to_binary)

### `OpenKvDb` {#OpenKvDb}

```go
func OpenKvDb(path string, createIfMissing bool) (*KvDb, error)
```

OpenKvDb opens the store at path, creating it when createIfMissing is set.

- Parameters:
    - path : `string`
    - createIfMissing : `bool`
- Return type: `(*KvDb, error)`
- Slots: [`kvdb_open`](../cpp/data.md#kvdb_open)

## `KvDb` {#KvDb}

```go
type KvDb struct {
    // unexported fields
}
```

KvDb is an open key-value store, kept by the host under the mod's data directory.

### `KvDb.Path` {#KvDb.Path}

```go
func (db *KvDb) Path() string
```

Path is where the store was opened.

- Return type: `string`

### `KvDb.Get` {#KvDb.Get}

```go
func (db *KvDb) Get(key string) (value string, found bool, err error)
```

Get reads one key; found is false for a key that does not exist. A store the host closed on its own, at an unload, also reads as not found: the ABI answers both alike.

- Parameters:
    - key : `string`
- Return type: `(value string, found bool, err error)`
- Slots: [`kvdb_get`](../cpp/data.md#kvdb_get)

### `KvDb.Set` {#KvDb.Set}

```go
func (db *KvDb) Set(key, value string) error
```

Set writes one key.

- Parameters:
    - key : `string`
    - value : `string`
- Return type: `error`
- Slots: [`kvdb_set`](../cpp/data.md#kvdb_set)

### `KvDb.Delete` {#KvDb.Delete}

```go
func (db *KvDb) Delete(key string) error
```

Delete removes one key.

- Parameters:
    - key : `string`
- Return type: `error`
- Slots: [`kvdb_del`](../cpp/data.md#kvdb_del)

### `KvDb.Has` {#KvDb.Has}

```go
func (db *KvDb) Has(key string) (bool, error)
```

Has reports whether a key exists.

- Parameters:
    - key : `string`
- Return type: `(bool, error)`
- Slots: [`kvdb_has`](../cpp/data.md#kvdb_has)

### `KvDb.IsEmpty` {#KvDb.IsEmpty}

```go
func (db *KvDb) IsEmpty() (bool, error)
```

IsEmpty reports whether the store holds no key.

- Return type: `(bool, error)`
- Slots: [`kvdb_is_empty`](../cpp/data.md#kvdb_is_empty)

### `KvDb.Iter` {#KvDb.Iter}

```go
func (db *KvDb) Iter() ([]KeyValue, error)
```

Iter lists every entry.

- Return type: `([]KeyValue, error)`
- Slots: [`kvdb_iter`](../cpp/data.md#kvdb_iter)

### `KvDb.Close` {#KvDb.Close}

```go
func (db *KvDb) Close() error
```

Close closes the store; using it afterwards fails.

- Return type: `error`
- Slots: [`kvdb_close`](../cpp/data.md#kvdb_close)

## `KvDbHandle` {#KvDbHandle}

```go
type KvDbHandle struct{ p unsafe.Pointer }
type KvDbHandle struct{ p unsafe.Pointer }
```

KvDbHandle is an open key-value store; the zero value is no store.

### `KvDbHandle.IsZero` {#KvDbHandle.IsZero}

```go
func (h KvDbHandle) IsZero() bool
```

IsZero reports whether the handle names nothing.

- Return type: `bool`

## `KeyValue` {#KeyValue}

```go
type KeyValue struct {
    Key   string
    Value string
}
```

KeyValue is one entry of a key-value store.
