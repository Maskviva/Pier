# Go：数据包

## 函数 {#functions}

### `RegisterPacketHook` {#RegisterPacketHook}

```go
func RegisterPacketHook(dirMask int32, fn func(*PacketCall) PacketVerdict) (*PacketHook, error)
```

对 `dirMask` 指定方向上的每个数据包调用 `fn`，`dirMask` 用的是 abi.h 的 `PIER_PKT_MASK_*` 位：1 表示收到的包，2 表示发出的包。`fn` 在宿主泵送连接或发送数据包的地方运行，通常是服务器线程，但不总是，因为刷新可能是异步的；`fn` 只碰自己的状态，要碰世界就通过 `Schedule`。

- 参数：
    - dirMask : `int32`
    - fn : `func(*PacketCall) PacketVerdict`
- 返回值类型：`(*PacketHook, error)`
- 对应槽位：[`packet_hook_register`](../cpp/packet.md#packet_hook_register)

### `RegisterPacketHookIDs` {#RegisterPacketHookIDs}

```go
func RegisterPacketHookIDs(dirMask int32, ids []int32, fn func(*PacketCall) PacketVerdict) (*PacketHook, error)
```

只对列出的数据包 id 生效的 `RegisterPacketHook`：没有任何钩子列出其 id 的数据包，在 `fn` 运行、加锁之前就直接放行。

- 参数：
    - dirMask : `int32`
    - ids : `[]int32`
    - fn : `func(*PacketCall) PacketVerdict`
- 返回值类型：`(*PacketHook, error)`
- 对应槽位：[`packet_hook_register_ids`](../cpp/packet.md#packet_hook_register_ids)

### `RegisterConnHook` {#RegisterConnHook}

```go
func RegisterConnHook(fn func(connID uint64, address string, opened bool)) (*PacketHook, error)
```

在连接打开和关闭时调用 `fn`，线程规则和 `RegisterPacketHook` 写的一样。

- 参数：
    - fn : `func(connID uint64, address string, opened bool)`
- 返回值类型：`(*PacketHook, error)`
- 对应槽位：[`packet_conn_hook_register`](../cpp/packet.md#packet_conn_hook_register)

## `PacketCall` {#PacketCall}

```go
type PacketCall struct {
    Event PacketEvent
    // unexported fields
}
```

钩子里的一个数据包，以及修改它的方法。

### `PacketCall.SetPacketID` {#PacketCall.SetPacketID}

```go
func (c *PacketCall) SetPacketID(id int32)
```

让转发出去的数据包带上另一个 id。

- 参数：
    - id : `int32`

### `PacketCall.SetSubIDs` {#PacketCall.SetSubIDs}

```go
func (c *PacketCall) SetSubIDs(sender, target uint8)
```

修改转发出去的数据包的发送方和目标子客户端 id。

- 参数：
    - sender : `uint8`
    - target : `uint8`

### `PacketCall.Replace` {#PacketCall.Replace}

```go
func (c *PacketCall) Replace(body []byte)
```

交出要转发的包体；钩子返回 `PacketReplace` 时生效。

- 参数：
    - body : `[]byte`

## `PacketHook` {#PacketHook}

```go
type PacketHook struct {
    // unexported fields
}
```

一个已注册的数据包钩子或连接钩子，保存下来用来移除它。

### `PacketHook.Unregister` {#PacketHook.Unregister}

```go
func (p *PacketHook) Unregister() error
```

移除这个钩子。调用两次也没有问题。

- 返回值类型：`error`
- 对应槽位：[`packet_conn_hook_unregister`](../cpp/packet.md#packet_conn_hook_unregister)、[`packet_hook_unregister`](../cpp/packet.md#packet_hook_unregister)

## `PacketHookHandle` {#PacketHookHandle}

```go
type PacketHookHandle struct{ p unsafe.Pointer }
type PacketHookHandle struct{ p unsafe.Pointer }
```

一个已注册的数据包钩子。

### `PacketHookHandle.IsZero` {#PacketHookHandle.IsZero}

```go
func (h PacketHookHandle) IsZero() bool
```

判断这个句柄是否什么都不指向。

- 返回值类型：`bool`

## `PacketVerdict` {#PacketVerdict}

```go
type PacketVerdict int32
```

数据包钩子对一个数据包做出的决定。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PacketPass"></span>`PacketPass` | `0` |  |
| <span id="PacketReplace"></span>`PacketReplace` | `1` |  |
| <span id="PacketDrop"></span>`PacketDrop` | `2` |  |

## `PacketEvent` {#PacketEvent}

```go
type PacketEvent struct {
    Direction   int32
    ConnID      uint64
    Address     string
    PacketID    int32
    SenderSubID uint8
    TargetSubID uint8
    Body        []byte
}
```

钩子看到的一个数据包。`Body` 是一份副本。
