# Go: Client

## Functions {#functions}

### `RegisterKey` {#RegisterKey}

```go
func RegisterKey(name string, keyCodes []int32, allowRemap bool, fn func(pressed bool, impact int32)) (*KeyBinding, error)
```

RegisterKey binds keys on the client: fn runs with pressed true on press and false on release, and the current focus impact. Client hosts only; a server answers not provided.

- Parameters:
    - name : `string`
    - keyCodes : `[]int32`
    - allowRemap : `bool`
    - fn : `func(pressed bool, impact int32)`
- Return type: `(*KeyBinding, error)`
- Slots: [`client_register_key`](../cpp/client.md#client_register_key)

## `KeyBinding` {#KeyBinding}

```go
type KeyBinding struct {
    // unexported fields
}
```

KeyBinding is a registered client key binding.

### `KeyBinding.Unregister` {#KeyBinding.Unregister}

```go
func (k *KeyBinding) Unregister() error
```

Unregister stops the binding's callbacks.

- Return type: `error`
- Slots: [`client_unregister_key`](../cpp/client.md#client_unregister_key)

## `KeyHandle` {#KeyHandle}

```go
type KeyHandle struct{ p unsafe.Pointer }
type KeyHandle struct{ p unsafe.Pointer }
```

KeyHandle is a registered client key binding.

### `KeyHandle.IsZero` {#KeyHandle.IsZero}

```go
func (h KeyHandle) IsZero() bool
```

IsZero reports whether the handle names nothing.

- Return type: `bool`
