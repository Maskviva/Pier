# 数据包

??? note "abi.h 里的分节说明"

    **§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）**

    **数据包拦截，追加的槽位，受 `struct_size` 约束（`Packet interception, appended and struct_size-gated.`）**

    双向拦截原始的线上格式。这是 `send_packet` 做不到的：它观察并改写已经存在的字节，不制造新的字节。

    传递的单位正好是**一个**数据包：开头是无符号变长整数的包头，后面跟着包体。分批和压缩在对端链更下层（`BatchedNetworkPeer` 拆开收到的批次，把发出的包重新分批），所以回调永远看不到一批数据包，也永远不需要自己写长度前缀。

    bridge 负责解码包头：`packet_id` 是它的低 10 位，`sender_sub_id` / `target_sub_id` 是再往上的两个 2 位字段，`body`/`body_len` 指向包头**之后**的位置。REPLACE 只提供新的**包体**；bridge 根据 `edit` 重新编码包头，所以改写时不需要重新拼变长整数的帧，改数据包 id 也只是给一个字段赋值，不需要去改字节。

    分发是链式的：有多个订阅者时，按注册顺序，每一个看到的是前一个的输出。第一个 DROP 生效，后面的被跳过。分发前会给订阅者列表拍一个快照，所以回调里可以安全地注册或注销（包括注销自己）。

    线程：碰游戏状态之前请先读这一段。收到的包，回调在连接被泵送的地方运行；发出的包，回调在发送发起的地方运行。实际上通常是服务器线程，但有异步刷新，所以并不保证。请把它们当作「不一定在游戏线程上」：处理函数要短，自己保护自己的状态，凡是碰世界的操作都通过 `schedule` 转过去。

    钩子在出现第一个订阅者时才安装，之后永不卸下（注销可能发生在被钩住的函数内部）。没有订阅者时，钩子函数直接快速转到原函数。

    **追加：按 id 过滤的数据包拦截（`Appended: packet interception filtered by id`）**

## 槽位 {#slots}

### `send_packet` {#send_packet}

```c
bool (*send_packet)(PierPlayerSel sel, int32_t packet_id, uint8_t const* body, size_t body_len);
```

!!! note "分组说明"

    按连接发送原始数据包（追加的槽位，受 `struct_size` 约束），`spawn_particle_for` 就是在它之上实现的。`packet_id` 是 `MinecraftPacketIds` 的值；`body`/`body_len` 是这个数据包在**当前**游戏版本下的线上格式。bridge 把它反序列化成真正的数据包对象（`MinecraftPackets::createPacket` 加 `Packet::read`），只发给解析出来的那名玩家的连接。

    以下情况返回 false：玩家不在线；id 不认识或构造不出来；包体解析失败；解析完以后还剩字节（形状不符合这个版本）。这是一个**逃生口**：线上格式随版本变化，正确与否由调用方负责；有带类型的接口时请优先用那些。

- 调用形式：`api->send_packet(sel, packet_id, body, body_len)`
- 参数：
    - sel : `PierPlayerSel`
    - packet_id : `int32_t`
    - body : `uint8_t const*`
    - body_len : `size_t`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 79 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::send_packet`](../rust/player.md#Player.send_packet)
    - Go：[`Raw.SendPacket`](../go/raw.md#Raw.SendPacket)

### `packet_hook_register` {#packet_hook_register}

```c
PierPacketHookHandle (*packet_hook_register)(
    PierModHandle mod,
    int32_t dir_mask,
    PierPacketCb cb,
    void* user
);
```

注册一个原始数据包拦截器。`dir_mask` 是 `PIER_PKT_MASK_INBOUND | PIER_PKT_MASK_OUTBOUND` 的组合（为 0 时什么都不注册，返回 NULL）。失败时返回 NULL。

- 调用形式：`api->packet_hook_register(mod, dir_mask, cb, user)`
- 参数：
    - mod : `PierModHandle`
    - dir_mask : `int32_t`
    - cb : `PierPacketCb`
    - user : `void*`
- 返回值类型：`PierPacketHookHandle`
- 所在分节：数据包拦截，追加的槽位，受 `struct_size` 约束（`Packet interception, appended and struct_size-gated.`）
- 表内序号：第 136 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Packets::intercept`](../rust/packet.md#Packets.intercept)、[`Packets::intercept_ids`](../rust/packet.md#Packets.intercept_ids)
    - Go：[`RegisterPacketHook`](../go/packet.md#RegisterPacketHook)

### `packet_hook_unregister` {#packet_hook_unregister}

```c
bool (*packet_hook_unregister)(PierModHandle mod, PierPacketHookHandle handle);
```

注销。在回调内部调用也是安全的。

- 调用形式：`api->packet_hook_unregister(mod, handle)`
- 参数：
    - mod : `PierModHandle`
    - handle : `PierPacketHookHandle`
- 返回值类型：`bool`
- 所在分节：数据包拦截，追加的槽位，受 `struct_size` 约束（`Packet interception, appended and struct_size-gated.`）
- 表内序号：第 137 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Packets::intercept`](../rust/packet.md#Packets.intercept)、[`Packets::intercept_ids`](../rust/packet.md#Packets.intercept_ids)
    - Go：[`PacketHook.Unregister`](../go/packet.md#PacketHook.Unregister)、[`Raw.PacketHookUnregister`](../go/raw.md#Raw.PacketHookUnregister)

### `packet_conn_hook_register` {#packet_conn_hook_register}

```c
PierPacketHookHandle (*packet_conn_hook_register)(PierModHandle mod, PierConnCb cb, void* user);
```

注册一个观察连接打开和关闭的回调。失败时返回 NULL。关闭通知是丢弃按连接保存的状态时唯一可靠的信号：没有完成登录握手的连接永远不会成为 Player，任何玩家事件都覆盖不到它。

- 调用形式：`api->packet_conn_hook_register(mod, cb, user)`
- 参数：
    - mod : `PierModHandle`
    - cb : `PierConnCb`
    - user : `void*`
- 返回值类型：`PierPacketHookHandle`
- 所在分节：数据包拦截，追加的槽位，受 `struct_size` 约束（`Packet interception, appended and struct_size-gated.`）
- 表内序号：第 138 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Packets::on_connection`](../rust/packet.md#Packets.on_connection)
    - Go：[`RegisterConnHook`](../go/packet.md#RegisterConnHook)

### `packet_conn_hook_unregister` {#packet_conn_hook_unregister}

```c
bool (*packet_conn_hook_unregister)(PierModHandle mod, PierPacketHookHandle handle);
```

注销。在回调内部调用也是安全的。

- 调用形式：`api->packet_conn_hook_unregister(mod, handle)`
- 参数：
    - mod : `PierModHandle`
    - handle : `PierPacketHookHandle`
- 返回值类型：`bool`
- 所在分节：数据包拦截，追加的槽位，受 `struct_size` 约束（`Packet interception, appended and struct_size-gated.`）
- 表内序号：第 139 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Packets::on_connection`](../rust/packet.md#Packets.on_connection)
    - Go：[`PacketHook.Unregister`](../go/packet.md#PacketHook.Unregister)、[`Raw.PacketConnHookUnregister`](../go/raw.md#Raw.PacketConnHookUnregister)

### `packet_hook_register_ids` {#packet_hook_register_ids}

```c
PierPacketHookHandle (*packet_hook_register_ids)(
    PierModHandle mod, int32_t dir_mask, int32_t const* ids, size_t count,
    PierPacketCb cb, void* user);
```

和 `packet_hook_register` 一样，但回调只对列出的数据包 id（`MinecraftPacketIds` 的值，0 到 1023）触发。`packet_hook_register` 订阅所有 id，那个方向上每个数据包都要调用一次回调，区块数据也算在内；只关心几个 id 的模组应当用这个槽位，因为没有任何订阅者列出的数据包，会在加锁之前直接放行。列表为空，或者所有 id 都超出范围时会被拒绝。注销用同一个槽位。可以在任何线程调用。

- 调用形式：`api->packet_hook_register_ids(mod, dir_mask, ids, count, cb, user)`
- 参数：
    - mod : `PierModHandle`
    - dir_mask : `int32_t`
    - ids : `int32_t const*`
    - count : `size_t`
    - cb : `PierPacketCb`
    - user : `void*`
- 返回值类型：`PierPacketHookHandle`
- 所在分节：追加：按 id 过滤的数据包拦截（`Appended: packet interception filtered by id`）
- 表内序号：第 188 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Packets::intercept_ids`](../rust/packet.md#Packets.intercept_ids)
    - Go：[`RegisterPacketHookIDs`](../go/packet.md#RegisterPacketHookIDs)

## 类型 {#types}

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

一个被拦截的数据包。里面的每个指针都是借来的，只在回调期间有效。回调之后还要用的东西必须复制一份。

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

可以修改的包头字段，预先填好了事件里的值。只有回调返回 `PIER_PKT_REPLACE` 时，这里的赋值才生效。

### `PierPacketHookHandle` {#PierPacketHookHandle}

```c
typedef void* PierPacketHookHandle;
```

用 `packet_hook_unregister` / `packet_conn_hook_unregister` 释放。

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

数据包拦截器。要改写，就用**新的包体**（不含包头）调用 `replace(replace_ctx, bytes, len)`，并返回 `PIER_PKT_REPLACE`。多次调用 `replace`，以最后一次的包体为准；返回 REPLACE 却一次都没调用 `replace`，表示「包体为空」。

### `PierConnCb` {#PierConnCb}

```c
typedef void (*PierConnCb)(void* user, uint64_t conn_id, PierStr address, bool opened);
```

连接的生命周期：接受连接时 `opened` 为 true，关闭时为 false。

## `PIER_PKT_*` {#PIER_PKT_INBOUND-group}

`PierPacketEvent::direction` 的取值，也是 `dir_mask` 用到的位的位置。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PIER_PKT_INBOUND"></span>`PIER_PKT_INBOUND` | `0` | 客户端 -> 服务器 |
| <span id="PIER_PKT_OUTBOUND"></span>`PIER_PKT_OUTBOUND` | `1` | 服务器 -> 客户端 |

## `PIER_PKT_MASK_*` {#PIER_PKT_MASK_INBOUND-group}

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PIER_PKT_MASK_INBOUND"></span>`PIER_PKT_MASK_INBOUND` | `(1 << PIER_PKT_INBOUND)` |  |
| <span id="PIER_PKT_MASK_OUTBOUND"></span>`PIER_PKT_MASK_OUTBOUND` | `(1 << PIER_PKT_OUTBOUND)` |  |

## `PIER_PKT_*` {#PIER_PKT_PASS-group}

`PierPacketCb` 的返回值。其他任何值都按 PASS 处理。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PIER_PKT_PASS"></span>`PIER_PKT_PASS` | `0` | 原样转发；`replace` 的输出被忽略 |
| <span id="PIER_PKT_REPLACE"></span>`PIER_PKT_REPLACE` | `1` | 转发交给 `replace` 的包体 |
| <span id="PIER_PKT_DROP"></span>`PIER_PKT_DROP` | `2` | 整个吞掉这个数据包 |
