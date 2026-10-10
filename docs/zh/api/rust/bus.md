# levilamina::bus · 事件总线

跨模组事件总线：广播，没有返回值。

和 [`crate::service`](service.md) 互补：总线是一对多，不返回任何东西，也不保证顺序；服务是一对一，有返回值，名字独占。

**模组收不到自己的发布**

模组收不到自己发布的消息。通知自己可以直接调用函数，而发布给自己是唯一一种任何深度上限都分辨不出的循环。跨模组的循环（A 到 B 再到 A）由深度上限截住，碰到上限时丢弃最内层的那次发布，并记一行日志。

**整组接口都是线程安全的，回调在发布方的线程上运行**

所以回调不能碰世界的状态；要碰，就用 `Host::schedule` 回到服务器线程。

## 函数 {#functions}

### `bus::subscribe` {#fn.subscribe}

```rust
pub fn subscribe(
    topic: &str,
    handler: impl FnMut(&str, &str) -> bool + Send + 'static,
) -> Result<Subscription>
```

订阅一个主题。

回调返回 `true` 表示否决，只有 [`publish_vetoable`](bus.md#fn.publish_vetoable) 会读它；普通的 [`publish`](bus.md#fn.publish) 忽略返回值。

- 参数：
    - topic : `&str`
    - handler : `impl FnMut(&str, &str) -> bool + Send + 'static,`
- 返回值类型：`Result<Subscription>`
- 对应槽位：[`bus_subscribe`](../cpp/crossmod.md#bus_subscribe)

### `bus::publish` {#fn.publish}

```rust
pub fn publish(topic: &str, payload: &str) -> Result<u32>
```

广播。返回实际运行了多少个订阅者，0 是正常结果，表示没有人在听。

- 参数：
    - topic : `&str`
    - payload : `&str`
- 返回值类型：`Result<u32>`
- 对应槽位：[`bus_publish`](../cpp/crossmod.md#bus_publish)

### `bus::publish_vetoable` {#fn.publish_vetoable}

```rust
pub fn publish_vetoable(topic: &str, payload: &str) -> Result<Vetoable>
```

广播，并收集否决位。

- 参数：
    - topic : `&str`
    - payload : `&str`
- 返回值类型：`Result<Vetoable>`
- 对应槽位：[`bus_publish_vetoable`](../cpp/crossmod.md#bus_publish_vetoable)

### `bus::subscriber_count` {#fn.subscriber_count}

```rust
pub fn subscriber_count(topic: &str) -> u32
```

这个主题当前有多少订阅者，所有模组加在一起。

用来在没人会读的时候，省掉拼装载荷的开销。

- 参数：
    - topic : `&str`
- 返回值类型：`u32`
- 对应槽位：[`bus_subscriber_count`](../cpp/crossmod.md#bus_subscriber_count)、[`bus_subscriber_count`](../cpp/crossmod.md#bus_subscriber_count)

## `Subscription` {#Subscription}

```rust
pub struct Subscription {
    // private fields
}
```

一个订阅。丢弃它就会取消订阅。

[`Subscription::forget`](bus.md#Subscription.forget) 让它一直存活到模组卸载，那时由宿主清掉。

- 实现的 trait：`Drop`

### `Subscription::id` {#Subscription.id}

```rust
pub fn id(&self) -> u64
```

- 返回值类型：`u64`

### `Subscription::topic` {#Subscription.topic}

```rust
pub fn topic(&self) -> &str
```

- 返回值类型：`&str`

### `Subscription::forget` {#Subscription.forget}

```rust
pub fn forget(mut self)
```

## `Vetoable` {#Vetoable}

```rust
pub struct Vetoable {
    /// A subscriber cast a veto.
    pub vetoed: bool,
    /// How many subscribers really ran. There is no short circuit: even after a veto the
    /// later observers still receive it, so they see a consistent stream.
    pub delivered: u32,
}
```

一次可否决广播的结果。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`
