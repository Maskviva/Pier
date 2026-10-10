# Packets

??? note "Section notes in abi.h"

    **§I NBT binary, KvDb (thread-safe), system & server info**

    **Packet interception, appended and struct\_size-gated.**

    Raw wire-format interception in both directions. This is the primitive `send_packet` could not provide: it observes and rewrites bytes that already exist, instead of manufacturing new ones.

    Delivery unit is exactly ONE packet — the leading unsigned-varint header followed by the packet body. Batching and compression live further down the peer chain (BatchedNetworkPeer splits inbound batches and re-batches outbound ones), so a callback never sees a batch and never has to produce a length prefix.

    The bridge decodes the header: `packet_id` is its low 10 bits, `sender_sub_id` / `target_sub_id` the two 2-bit fields above it, and `body`/`body_len` point PAST the header. A REPLACE verdict supplies a new BODY only; the bridge re-encodes the header from `edit`, so a rewrite never reproduces varint framing and packet-id remapping is a field assignment rather than a byte-surgery exercise.

    Dispatch chains: with several subscribers, each one sees the output of the previous, in registration order. The first DROP wins and the rest are skipped. Subscriber lists are snapshotted before dispatch, so a callback may register or unregister (including itself) safely.

    Threading — read this before touching game state. Inbound callbacks run wherever the connection is pumped and outbound ones wherever the send originates. In practice that is the server thread, but async flush means it is not guaranteed. Treat these as "not necessarily the game thread": a handler stays short, guards its own state, and routes anything that touches the world through `schedule`.

    Detours install lazily on the first subscriber and are never unpatched (an unsubscribe can arrive from inside the hooked function). With no subscribers the hook bodies fast-path straight to origin.

    **Appended: packet interception filtered by id**

## Slots {#slots}

### `send_packet` {#send_packet}

```c
bool (*send_packet)(PierPlayerSel sel, int32_t packet_id, uint8_t const* body, size_t body_len);
```

!!! note "Group note"

    Raw per-connection packet send (additive, gated by `struct_size`) — the generic primitive `spawn_particle_for` derives from. `packet_id` is a MinecraftPacketIds value; `body`/`body_len` is the packet's wire-format body for the CURRENT game version. The bridge deserializes it into a real packet object (`MinecraftPackets::createPacket` + `Packet::read`) and delivers it to the resolved player's connection only. False if: player offline, unknown/unconstructible id, body fails to parse, or bytes are left over after parsing (wrong shape for this version). ESCAPE HATCH: the wire format is version-specific and is the caller's responsibility; prefer typed entries when one exists.

- Call: `api->send_packet(sel, packet_id, body, body_len)`
- Parameters:
    - sel : `PierPlayerSel`
    - packet_id : `int32_t`
    - body : `uint8_t const*`
    - body_len : `size_t`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 79, counting from 0
- Callers in each binding:
    - Rust: [`Player::send_packet`](../rust/player.md#Player.send_packet)
    - Go: [`Raw.SendPacket`](../go/raw.md#Raw.SendPacket)

### `packet_hook_register` {#packet_hook_register}

```c
PierPacketHookHandle (*packet_hook_register)(
    PierModHandle mod,
    int32_t dir_mask,
    PierPacketCb cb,
    void* user
);
```

Register a raw packet interceptor. `dir_mask` is `PIER_PKT_MASK_INBOUND` | `PIER_PKT_MASK_OUTBOUND` (a zero mask registers nothing and returns NULL). Returns NULL on failure.

- Call: `api->packet_hook_register(mod, dir_mask, cb, user)`
- Parameters:
    - mod : `PierModHandle`
    - dir_mask : `int32_t`
    - cb : `PierPacketCb`
    - user : `void*`
- Return type: `PierPacketHookHandle`
- Section of abi.h: Packet interception, appended and struct\_size-gated.
- Position in the table: slot 136, counting from 0
- Callers in each binding:
    - Rust: [`Packets::intercept`](../rust/packet.md#Packets.intercept), [`Packets::intercept_ids`](../rust/packet.md#Packets.intercept_ids)
    - Go: [`RegisterPacketHook`](../go/packet.md#RegisterPacketHook)

### `packet_hook_unregister` {#packet_hook_unregister}

```c
bool (*packet_hook_unregister)(PierModHandle mod, PierPacketHookHandle handle);
```

Unregister. Safe to call from inside the callback.

- Call: `api->packet_hook_unregister(mod, handle)`
- Parameters:
    - mod : `PierModHandle`
    - handle : `PierPacketHookHandle`
- Return type: `bool`
- Section of abi.h: Packet interception, appended and struct\_size-gated.
- Position in the table: slot 137, counting from 0
- Callers in each binding:
    - Rust: [`Packets::intercept`](../rust/packet.md#Packets.intercept), [`Packets::intercept_ids`](../rust/packet.md#Packets.intercept_ids)
    - Go: [`PacketHook.Unregister`](../go/packet.md#PacketHook.Unregister), [`Raw.PacketHookUnregister`](../go/raw.md#Raw.PacketHookUnregister)

### `packet_conn_hook_register` {#packet_conn_hook_register}

```c
PierPacketHookHandle (*packet_conn_hook_register)(PierModHandle mod, PierConnCb cb, void* user);
```

Register a connection open/close observer. Returns NULL on failure. The close notification is the only reliable signal for dropping per-connection state: a connection that never finishes the login handshake never becomes a Player, so no player event covers it.

- Call: `api->packet_conn_hook_register(mod, cb, user)`
- Parameters:
    - mod : `PierModHandle`
    - cb : `PierConnCb`
    - user : `void*`
- Return type: `PierPacketHookHandle`
- Section of abi.h: Packet interception, appended and struct\_size-gated.
- Position in the table: slot 138, counting from 0
- Callers in each binding:
    - Rust: [`Packets::on_connection`](../rust/packet.md#Packets.on_connection)
    - Go: [`RegisterConnHook`](../go/packet.md#RegisterConnHook)

### `packet_conn_hook_unregister` {#packet_conn_hook_unregister}

```c
bool (*packet_conn_hook_unregister)(PierModHandle mod, PierPacketHookHandle handle);
```

Unregister. Safe to call from inside the callback.

- Call: `api->packet_conn_hook_unregister(mod, handle)`
- Parameters:
    - mod : `PierModHandle`
    - handle : `PierPacketHookHandle`
- Return type: `bool`
- Section of abi.h: Packet interception, appended and struct\_size-gated.
- Position in the table: slot 139, counting from 0
- Callers in each binding:
    - Rust: [`Packets::on_connection`](../rust/packet.md#Packets.on_connection)
    - Go: [`PacketHook.Unregister`](../go/packet.md#PacketHook.Unregister), [`Raw.PacketConnHookUnregister`](../go/raw.md#Raw.PacketConnHookUnregister)

### `packet_hook_register_ids` {#packet_hook_register_ids}

```c
PierPacketHookHandle (*packet_hook_register_ids)(
    PierModHandle mod, int32_t dir_mask, int32_t const* ids, size_t count,
    PierPacketCb cb, void* user);
```

As `packet_hook_register`, but the callback fires only for the listed packet ids (MinecraftPacketIds values, 0..1023). `packet_hook_register` subscribes to every id and therefore costs one callback per packet in that direction, chunk data included; a mod watching a few ids should use this slot, since a packet no subscriber listed is passed through before any lock is taken. An empty list, or one whose ids are all out of range, is refused. The same unregister slot applies. Any thread.

- Call: `api->packet_hook_register_ids(mod, dir_mask, ids, count, cb, user)`
- Parameters:
    - mod : `PierModHandle`
    - dir_mask : `int32_t`
    - ids : `int32_t const*`
    - count : `size_t`
    - cb : `PierPacketCb`
    - user : `void*`
- Return type: `PierPacketHookHandle`
- Section of abi.h: Appended: packet interception filtered by id
- Position in the table: slot 188, counting from 0
- Callers in each binding:
    - Rust: [`Packets::intercept_ids`](../rust/packet.md#Packets.intercept_ids)
    - Go: [`RegisterPacketHookIDs`](../go/packet.md#RegisterPacketHookIDs)

## Types {#types}

### `PierPacketEvent` {#PierPacketEvent}

```c
typedef struct PierPacketEvent
{
    /** sizeof(PierPacketEvent) as the LOADER knows it. Check before reading
     *  trailing fields, same discipline as PierApi::struct_size. */
    uint32_t struct_size;
    /** PIER_PKT_INBOUND / PIER_PKT_OUTBOUND. */
    int32_t direction;
    /** NetworkIdentifier::getHash() — stable for the connection's lifetime and
     *  available before the player exists, which is exactly when a login-phase
     *  rewrite needs to key its state. */
    uint64_t conn_id;
    /** "host:port" (NetworkIdentifier::getIPAndPort). */
    PierStr address;
    /** MinecraftPacketIds value decoded from the header. */
    int32_t packet_id;
    uint8_t sender_sub_id;
    uint8_t target_sub_id;
    /** Packet body, header excluded. NULL only when body_len is 0. */
    uint8_t const* body;
    size_t body_len;
} PierPacketEvent;
```

One intercepted packet. Every pointer inside is borrowed and valid only for the duration of the callback. Anything kept past it must be copied.

### `PierPacketEdit` {#PierPacketEdit}

```c
typedef struct PierPacketEdit
{
    uint32_t struct_size;
    int32_t packet_id;
    uint8_t sender_sub_id;
    uint8_t target_sub_id;
} PierPacketEdit;
```

Mutable header fields, pre-filled from the event. Assignments here only take effect when the callback returns `PIER_PKT_REPLACE`.

### `PierPacketHookHandle` {#PierPacketHookHandle}

```c
typedef void* PierPacketHookHandle;
```

Drop via `packet_hook_unregister` / `packet_conn_hook_unregister`.

### `PierPacketCb` {#PierPacketCb}

```c
typedef int32_t (*PierPacketCb)(
    void* user,
    PierPacketEvent const* ev,
    PierPacketEdit* edit,
    void* replace_ctx,
    PierBytesSink replace
);
```

Packet interceptor. To rewrite, call `replace(replace_ctx, bytes, len)` with the NEW BODY (header excluded) and return `PIER_PKT_REPLACE`. Calling `replace` more than once keeps the last body; returning REPLACE without ever calling it means "empty body".

### `PierConnCb` {#PierConnCb}

```c
typedef void (*PierConnCb)(void* user, uint64_t conn_id, PierStr address, bool opened);
```

Connection lifecycle: `opened` is true on accept, false on close.

## `PIER_PKT_*` {#PIER_PKT_INBOUND-group}

`PierPacketEvent::direction`, and the bit positions used by `dir_mask`.

| Name | Value | Description |
|---|---|---|
| <span id="PIER_PKT_INBOUND"></span>`PIER_PKT_INBOUND` | `0` | client -&gt; server |
| <span id="PIER_PKT_OUTBOUND"></span>`PIER_PKT_OUTBOUND` | `1` | server -&gt; client |

## `PIER_PKT_MASK_*` {#PIER_PKT_MASK_INBOUND-group}

| Name | Value | Description |
|---|---|---|
| <span id="PIER_PKT_MASK_INBOUND"></span>`PIER_PKT_MASK_INBOUND` | `(1 << PIER_PKT_INBOUND)` |  |
| <span id="PIER_PKT_MASK_OUTBOUND"></span>`PIER_PKT_MASK_OUTBOUND` | `(1 << PIER_PKT_OUTBOUND)` |  |

## `PIER_PKT_*` {#PIER_PKT_PASS-group}

PierPacketCb return value. Anything else is treated as PASS.

| Name | Value | Description |
|---|---|---|
| <span id="PIER_PKT_PASS"></span>`PIER_PKT_PASS` | `0` | forward unchanged; `replace` output ignored |
| <span id="PIER_PKT_REPLACE"></span>`PIER_PKT_REPLACE` | `1` | forward the body handed to `replace` |
| <span id="PIER_PKT_DROP"></span>`PIER_PKT_DROP` | `2` | swallow the packet entirely |
