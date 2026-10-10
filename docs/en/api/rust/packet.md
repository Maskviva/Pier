# levilamina::packet

Raw packet interception.
This layer handles closure ownership, the panic fence, and gathering `{ptr,len}` into a `&[u8]`.
It interprets not one byte of the body: version differences, field layout and codecs all live on
the caller's side. A loader usable across versions cannot understand the wire format of every
version at once.

**Threads: read this before writing any state**

An inbound callback runs on the thread pumping that connection and an outbound one on the thread
that started the send. Usually that is the server thread, but an async flush means it is not
guaranteed, which is why the closure bound is `Send + Sync` and not `Send`: the same closure may
be entered by several threads at once.

With several subscribers, each sees the output of the previous one in registration order, and
the first Drop wins with everything after it skipped. The subscription table is snapshotted
before dispatch, so registering and deregistering inside a callback is safe.

## `Packet` {#Packet}

```rust
pub struct Packet<'a> {
    // private fields
}
```

The packet a callback receives. Valid only during the callback.

### `Packet::direction` {#Packet.direction}

```rust
pub fn direction(&self) -> Direction
```

- Return type: `Direction`

### `Packet::conn_id` {#Packet.conn_id}

```rust
pub fn conn_id(&self) -> u64
```

The id of this connection. Stable across packets and usable as the key of per-connection
state.

- Return type: `u64`

### `Packet::address` {#Packet.address}

```rust
pub fn address(&self) -> &str
```

The peer address as `ip:port`. For diagnostics; use `conn_id` as a key.

- Return type: `&str`

### `Packet::packet_id` {#Packet.packet_id}

```rust
pub fn packet_id(&self) -> i32
```

- Return type: `i32`

### `Packet::sender_sub_id` {#Packet.sender_sub_id}

```rust
pub fn sender_sub_id(&self) -> u8
```

- Return type: `u8`

### `Packet::target_sub_id` {#Packet.target_sub_id}

```rust
pub fn target_sub_id(&self) -> u8
```

- Return type: `u8`

### `Packet::body` {#Packet.body}

```rust
pub fn body(&self) -> &[u8]
```

The body, without the header, since the packet id and sub id are already decoded.

That is deliberate: a rewriter only supplies a new body and the host re-encodes the
header from `edit`, so changing a packet id is a field assignment rather than varint
surgery.

- Return type: `&[u8]`

### `Packet::set_body` {#Packet.set_body}

```rust
pub fn set_body(&mut self, bytes: &[u8])
```

Replaces the body. It may be called several times in one callback and the last one
counts.

- Parameters:
    - bytes : `&[u8]`

### `Packet::set_packet_id` {#Packet.set_packet_id}

```rust
pub fn set_packet_id(&mut self, id: i32)
```

Rewrites the packet id, for remapping onto the numbering of another version.

- Parameters:
    - id : `i32`

### `Packet::set_sender_sub_id` {#Packet.set_sender_sub_id}

```rust
pub fn set_sender_sub_id(&mut self, id: u8)
```

- Parameters:
    - id : `u8`

### `Packet::set_target_sub_id` {#Packet.set_target_sub_id}

```rust
pub fn set_target_sub_id(&mut self, id: u8)
```

- Parameters:
    - id : `u8`

## `Packets` {#Packets}

```rust
pub struct Packets(/* private */);
```

The packet facade.

- Implements: `Clone`, `Copy`

### `Packets::get` {#Packets.get}

```rust
pub fn get() -> Packets
```

- Return type: `Packets`

### `Packets::intercept` {#Packets.intercept}

```rust
pub fn intercept<F>(&self, dirs: Directions, f: F) -> Result<PacketHook>
    where
        F: Fn(&mut Packet<'_>) -> Verdict + Send + Sync + 'static,
```

Registers a packet interceptor.

The closure needs `Send + Sync`; see the thread section of the module header. A callback
is not guaranteed to be on the server thread and may be entered by several threads at
once.

- Parameters:
    - dirs : `Directions`
    - f : `F`
- Return type: `Result<PacketHook>`
- Slots: [`packet_hook_register`](../cpp/packet.md#packet_hook_register), [`packet_hook_unregister`](../cpp/packet.md#packet_hook_unregister)

### `Packets::intercept_ids` {#Packets.intercept_ids}

```rust
pub fn intercept_ids<F>(&self, dirs: Directions, ids: &[i32], f: F) -> Result<PacketHook>
    where
        F: Fn(&mut Packet<'_>) -> Verdict + Send + Sync + 'static,
```

As [`Packets::intercept`](packet.md#Packets.intercept), but the interceptor runs only for the listed packet ids.

This is the form to use whenever the ids are known, which is nearly always. The
unfiltered form costs one callback per packet in each direction it asked for, chunk
data included, while here the host passes a packet no subscriber listed through before
taking any lock. `ids` are `MinecraftPacketIds` values in `0..1024`; an id outside that
range is ignored by the host, and a list with nothing left is refused.

On a host without this slot the call falls back to the unfiltered form and filters the
id in the trampoline, so the interceptor still sees only the ids it asked for, at the
cost of one callback per packet.

- Parameters:
    - dirs : `Directions`
    - ids : `&[i32]`
    - f : `F`
- Return type: `Result<PacketHook>`
- Slots: [`packet_hook_unregister`](../cpp/packet.md#packet_hook_unregister), [`packet_hook_register_ids`](../cpp/packet.md#packet_hook_register_ids), [`packet_hook_register`](../cpp/packet.md#packet_hook_register)

### `Packets::on_connection` {#Packets.on_connection}

```rust
pub fn on_connection<F>(&self, f: F) -> Result<PacketHook>
    where
        F: Fn(u64, &str, ConnectionState) + Send + Sync + 'static,
```

Registers an observer of connection opens and closes.

- Parameters:
    - f : `F`
- Return type: `Result<PacketHook>`
- Slots: [`packet_conn_hook_register`](../cpp/packet.md#packet_conn_hook_register), [`packet_conn_hook_unregister`](../cpp/packet.md#packet_conn_hook_unregister)

## `PacketHook` {#PacketHook}

```rust
pub struct PacketHook {
    // private fields
}
```

A registered packet interceptor.

Dropping it deregisters. `forget()` keeps it alive until the mod unloads, and it is
explicit because registering and discarding looks identical in code to registering and
forgetting to keep the returned value, and the latter is a bug. The host clears what
remains when the mod unloads, at teardown stage 90.

- Implements: `Drop`

### `PacketHook::forget` {#PacketHook.forget}

```rust
pub fn forget(mut self)
```

Gives up deregistering on drop and keeps it alive until the mod unloads.

This step is explicit because registering and discarding looks identical in code to
registering and forgetting to keep the returned value, and the latter is a bug where the
interceptor disappears the moment it is installed.

## `Direction` {#Direction}

```rust
pub enum Direction {
        /// Client to server.
        Inbound,
        /// Server to client.
        Outbound,
}
```

The direction of a packet.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

## `Directions` {#Directions}

```rust
pub enum Directions {
        Inbound,
        Outbound,
        Both,
}
```

Which directions to select at registration.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

## `Verdict` {#Verdict}

```rust
pub enum Verdict {
        /// Forward unchanged. A prior `set_body` or `set_packet_id` takes effect.
        Forward,
        /// Consume the packet entirely.
        Drop,
}
```

What the callback does with this packet.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

## `ConnectionState` {#ConnectionState}

```rust
pub enum ConnectionState {
        Opened,
        Closed,
}
```

The two states of a connection.

A close is the only reliable signal to clear the state of a connection: one that never
completed the login handshake never becomes a Player and no player event covers it.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`
