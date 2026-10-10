# levilamina::packet · 数据包

原始数据包拦截。

这一层负责闭包的所有权、panic 围栏，以及把 `{ptr,len}` 收集成 `&[u8]`。包体它一个字节都不解读：版本差异、字段布局和编解码都在调用方那边。一个能跨版本使用的加载器，没办法同时理解每个版本的线上格式。

**线程：写任何状态之前请先读这一段**

收到的包，回调在泵送这个连接的线程上运行；发出的包，回调在发起发送的线程上运行。通常是服务器线程，但有异步刷新，所以并不保证。这就是闭包的约束是 `Send + Sync`、只有 `Send` 不够的原因：同一个闭包可能同时被几个线程进入。

有多个订阅者时，按注册顺序，每一个看到的是前一个的输出，第一个 Drop 生效，之后的全部跳过。分发前会给订阅表拍一个快照，所以在回调里注册和注销是安全的。

## `Packet` {#Packet}

```rust
pub struct Packet<'a> {
    // private fields
}
```

回调收到的数据包。只在回调期间有效。

### `Packet::direction` {#Packet.direction}

```rust
pub fn direction(&self) -> Direction
```

- 返回值类型：`Direction`

### `Packet::conn_id` {#Packet.conn_id}

```rust
pub fn conn_id(&self) -> u64
```

这个连接的 id。在多个数据包之间保持不变，可以用作按连接保存状态的键。

- 返回值类型：`u64`

### `Packet::address` {#Packet.address}

```rust
pub fn address(&self) -> &str
```

对端地址，形如 `ip:port`。用于诊断；当作键请用 `conn_id`。

- 返回值类型：`&str`

### `Packet::packet_id` {#Packet.packet_id}

```rust
pub fn packet_id(&self) -> i32
```

- 返回值类型：`i32`

### `Packet::sender_sub_id` {#Packet.sender_sub_id}

```rust
pub fn sender_sub_id(&self) -> u8
```

- 返回值类型：`u8`

### `Packet::target_sub_id` {#Packet.target_sub_id}

```rust
pub fn target_sub_id(&self) -> u8
```

- 返回值类型：`u8`

### `Packet::body` {#Packet.body}

```rust
pub fn body(&self) -> &[u8]
```

包体，不含包头，因为数据包 id 和子 id 已经解码好了。

这是有意的：改写方只提供新的包体，宿主根据 `edit` 重新编码包头，所以改数据包 id 只是给一个字段赋值，不用去拆变长整数。

- 返回值类型：`&[u8]`

### `Packet::set_body` {#Packet.set_body}

```rust
pub fn set_body(&mut self, bytes: &[u8])
```

替换包体。一次回调里可以调用多次，以最后一次为准。

- 参数：
    - bytes : `&[u8]`

### `Packet::set_packet_id` {#Packet.set_packet_id}

```rust
pub fn set_packet_id(&mut self, id: i32)
```

改写数据包 id，用于映射到另一个版本的编号上。

- 参数：
    - id : `i32`

### `Packet::set_sender_sub_id` {#Packet.set_sender_sub_id}

```rust
pub fn set_sender_sub_id(&mut self, id: u8)
```

- 参数：
    - id : `u8`

### `Packet::set_target_sub_id` {#Packet.set_target_sub_id}

```rust
pub fn set_target_sub_id(&mut self, id: u8)
```

- 参数：
    - id : `u8`

## `Packets` {#Packets}

```rust
pub struct Packets(/* private */);
```

数据包门面。

- 实现的 trait：`Clone`、`Copy`

### `Packets::get` {#Packets.get}

```rust
pub fn get() -> Packets
```

- 返回值类型：`Packets`

### `Packets::intercept` {#Packets.intercept}

```rust
pub fn intercept<F>(&self, dirs: Directions, f: F) -> Result<PacketHook>
    where
        F: Fn(&mut Packet<'_>) -> Verdict + Send + Sync + 'static,
```

注册一个数据包拦截器。

闭包需要 `Send + Sync`；见模块开头的线程一节。回调不保证在服务器线程上，也可能同时被几个线程进入。

- 参数：
    - dirs : `Directions`
    - f : `F`
- 返回值类型：`Result<PacketHook>`
- 对应槽位：[`packet_hook_register`](../cpp/packet.md#packet_hook_register)、[`packet_hook_unregister`](../cpp/packet.md#packet_hook_unregister)

### `Packets::intercept_ids` {#Packets.intercept_ids}

```rust
pub fn intercept_ids<F>(&self, dirs: Directions, ids: &[i32], f: F) -> Result<PacketHook>
    where
        F: Fn(&mut Packet<'_>) -> Verdict + Send + Sync + 'static,
```

和 [`Packets::intercept`](packet.md#Packets.intercept) 一样，但拦截器只对列出的数据包 id 运行。

只要知道 id，就该用这种形式，而这几乎总是成立的。不过滤的形式在它要求的每个方向上，每个数据包都要调用一次回调，区块数据也算在内；这里，没有任何订阅者列出的数据包，宿主在加锁之前就直接放行。`ids` 是 `0..1024` 范围内的 `MinecraftPacketIds` 值；超出范围的 id 会被宿主忽略，过滤后一个不剩的列表会被拒绝。

在没有这个槽位的宿主上，调用会退回到不过滤的形式，并在跳板函数里按 id 过滤，所以拦截器看到的仍然只是它要的那些 id，代价是每个数据包一次回调。

- 参数：
    - dirs : `Directions`
    - ids : `&[i32]`
    - f : `F`
- 返回值类型：`Result<PacketHook>`
- 对应槽位：[`packet_hook_unregister`](../cpp/packet.md#packet_hook_unregister)、[`packet_hook_register_ids`](../cpp/packet.md#packet_hook_register_ids)、[`packet_hook_register`](../cpp/packet.md#packet_hook_register)

### `Packets::on_connection` {#Packets.on_connection}

```rust
pub fn on_connection<F>(&self, f: F) -> Result<PacketHook>
    where
        F: Fn(u64, &str, ConnectionState) + Send + Sync + 'static,
```

注册一个观察连接打开和关闭的回调。

- 参数：
    - f : `F`
- 返回值类型：`Result<PacketHook>`
- 对应槽位：[`packet_conn_hook_register`](../cpp/packet.md#packet_conn_hook_register)、[`packet_conn_hook_unregister`](../cpp/packet.md#packet_conn_hook_unregister)

## `PacketHook` {#PacketHook}

```rust
pub struct PacketHook {
    // private fields
}
```

一个已注册的数据包拦截器。

丢弃它就会注销。`forget()` 让它一直存活到模组卸载；要明确调用它，是因为「注册后丢弃」和「注册后忘了保存返回值」在代码里看起来一模一样，而后者是个 bug。模组卸载时，宿主会在拆除的第 90 阶段清掉剩下的拦截器。

- 实现的 trait：`Drop`

### `PacketHook::forget` {#PacketHook.forget}

```rust
pub fn forget(mut self)
```

放弃丢弃时的注销，让它一直存活到模组卸载。

这一步要明确调用，因为「注册后丢弃」和「注册后忘了保存返回值」在代码里看起来一模一样，而后者是一个 bug：拦截器刚装上就没了。

## `Direction` {#Direction}

```rust
pub enum Direction {
        /// Client to server.
        Inbound,
        /// Server to client.
        Outbound,
}
```

数据包的方向。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

## `Directions` {#Directions}

```rust
pub enum Directions {
        Inbound,
        Outbound,
        Both,
}
```

注册时选择哪些方向。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

## `Verdict` {#Verdict}

```rust
pub enum Verdict {
        /// Forward unchanged. A prior `set_body` or `set_packet_id` takes effect.
        Forward,
        /// Consume the packet entirely.
        Drop,
}
```

回调对这个数据包怎么处理。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

## `ConnectionState` {#ConnectionState}

```rust
pub enum ConnectionState {
        Opened,
        Closed,
}
```

连接的两种状态。

关闭是清除一个连接的状态时唯一可靠的信号：没有完成登录握手的连接永远不会成为 Player，任何玩家事件都覆盖不到它。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`
