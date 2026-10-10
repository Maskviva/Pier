# Go: Packets

## Functions {#functions}

### `RegisterPacketHook` {#RegisterPacketHook}

```go
func RegisterPacketHook(dirMask int32, fn func(*PacketCall) PacketVerdict) (*PacketHook, error)
```

RegisterPacketHook calls fn for every packet in the directions of dirMask, the PIER\_PKT\_MASK\_\* bits of abi.h: 1 for inbound, 2 for outbound. fn runs where the host pumps the connection or sends the packet, which is usually the server thread and not always, since a flush can be asynchronous; fn keeps to its own state and reaches the world through Schedule.

- Parameters:
    - dirMask : `int32`
    - fn : `func(*PacketCall) PacketVerdict`
- Return type: `(*PacketHook, error)`
- Slots: [`packet_hook_register`](../cpp/packet.md#packet_hook_register)

### `RegisterPacketHookIDs` {#RegisterPacketHookIDs}

```go
func RegisterPacketHookIDs(dirMask int32, ids []int32, fn func(*PacketCall) PacketVerdict) (*PacketHook, error)
```

RegisterPacketHookIDs is RegisterPacketHook for the listed packet ids only: a packet whose id no hook listed is passed on before fn runs or any lock is taken.

- Parameters:
    - dirMask : `int32`
    - ids : `[]int32`
    - fn : `func(*PacketCall) PacketVerdict`
- Return type: `(*PacketHook, error)`
- Slots: [`packet_hook_register_ids`](../cpp/packet.md#packet_hook_register_ids)

### `RegisterConnHook` {#RegisterConnHook}

```go
func RegisterConnHook(fn func(connID uint64, address string, opened bool)) (*PacketHook, error)
```

RegisterConnHook calls fn when a connection opens and when it closes, with the threading RegisterPacketHook describes.

- Parameters:
    - fn : `func(connID uint64, address string, opened bool)`
- Return type: `(*PacketHook, error)`
- Slots: [`packet_conn_hook_register`](../cpp/packet.md#packet_conn_hook_register)

## `PacketCall` {#PacketCall}

```go
type PacketCall struct {
    Event PacketEvent
    // unexported fields
}
```

PacketCall is one packet inside a hook, with the ways to change it.

### `PacketCall.SetPacketID` {#PacketCall.SetPacketID}

```go
func (c *PacketCall) SetPacketID(id int32)
```

SetPacketID makes the forwarded packet carry another id.

- Parameters:
    - id : `int32`

### `PacketCall.SetSubIDs` {#PacketCall.SetSubIDs}

```go
func (c *PacketCall) SetSubIDs(sender, target uint8)
```

SetSubIDs changes the sender and target sub-client ids of the forwarded packet.

- Parameters:
    - sender : `uint8`
    - target : `uint8`

### `PacketCall.Replace` {#PacketCall.Replace}

```go
func (c *PacketCall) Replace(body []byte)
```

Replace hands over the body to forward; it takes effect when the hook returns PacketReplace.

- Parameters:
    - body : `[]byte`

## `PacketHook` {#PacketHook}

```go
type PacketHook struct {
    // unexported fields
}
```

PacketHook is a registered packet or connection hook, kept to remove it.

### `PacketHook.Unregister` {#PacketHook.Unregister}

```go
func (p *PacketHook) Unregister() error
```

Unregister removes the hook. Calling it twice is harmless.

- Return type: `error`
- Slots: [`packet_conn_hook_unregister`](../cpp/packet.md#packet_conn_hook_unregister), [`packet_hook_unregister`](../cpp/packet.md#packet_hook_unregister)

## `PacketHookHandle` {#PacketHookHandle}

```go
type PacketHookHandle struct{ p unsafe.Pointer }
type PacketHookHandle struct{ p unsafe.Pointer }
```

PacketHookHandle is a registered packet hook.

### `PacketHookHandle.IsZero` {#PacketHookHandle.IsZero}

```go
func (h PacketHookHandle) IsZero() bool
```

IsZero reports whether the handle names nothing.

- Return type: `bool`

## `PacketVerdict` {#PacketVerdict}

```go
type PacketVerdict int32
```

PacketVerdict is what a packet hook decides about one packet.

| Name | Value | Description |
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

PacketEvent is one packet a hook sees. Body is a copy.
