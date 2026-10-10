# Go：客户端

## 函数 {#functions}

### `RegisterKey` {#RegisterKey}

```go
func RegisterKey(name string, keyCodes []int32, allowRemap bool, fn func(pressed bool, impact int32)) (*KeyBinding, error)
```

在客户端上绑定按键：按下时以 `pressed` 为 true 调用 `fn`，松开时为 false，同时传入当前的焦点影响。只有客户端宿主支持；服务器会回答「没有提供」。

- 参数：
    - name : `string`
    - keyCodes : `[]int32`
    - allowRemap : `bool`
    - fn : `func(pressed bool, impact int32)`
- 返回值类型：`(*KeyBinding, error)`
- 对应槽位：[`client_register_key`](../cpp/client.md#client_register_key)

## `KeyBinding` {#KeyBinding}

```go
type KeyBinding struct {
    // unexported fields
}
```

一个已注册的客户端按键绑定。

### `KeyBinding.Unregister` {#KeyBinding.Unregister}

```go
func (k *KeyBinding) Unregister() error
```

停止这个绑定的回调。

- 返回值类型：`error`
- 对应槽位：[`client_unregister_key`](../cpp/client.md#client_unregister_key)

## `KeyHandle` {#KeyHandle}

```go
type KeyHandle struct{ p unsafe.Pointer }
type KeyHandle struct{ p unsafe.Pointer }
```

一个已注册的客户端按键绑定。

### `KeyHandle.IsZero` {#KeyHandle.IsZero}

```go
func (h KeyHandle) IsZero() bool
```

判断这个句柄是否什么都不指向。

- 返回值类型：`bool`
