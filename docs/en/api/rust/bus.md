# levilamina::bus

The cross-mod event bus: broadcast, with no return value.

Complementary to [`crate::service`](service.md): the bus is one to many, returns nothing and guarantees no
order, while a service is one to one, returns a value and holds its name exclusively.

**A mod does not receive its own publish**

A mod does not receive its own publish. Notifying yourself is a direct function call, and
publishing to yourself is the one cycle no depth limit can tell apart. A cross-mod cycle, A to B
to A, is caught by the depth cap, and hitting it discards the innermost publish with a log line.

**The whole family is thread safe and a callback runs on the publisher's thread**

A callback therefore must not touch world state; touching it means `Host::schedule` back onto
the server thread.

## Functions {#functions}

### `bus::subscribe` {#fn.subscribe}

```rust
pub fn subscribe(
    topic: &str,
    handler: impl FnMut(&str, &str) -> bool + Send + 'static,
) -> Result<Subscription>
```

Subscribes to a topic.

A `true` from the callback is a veto, which only [`publish_vetoable`](bus.md#fn.publish_vetoable) reads; an
ordinary [`publish`](bus.md#fn.publish) ignores the return value.

- Parameters:
    - topic : `&str`
    - handler : `impl FnMut(&str, &str) -> bool + Send + 'static,`
- Return type: `Result<Subscription>`
- Slots: [`bus_subscribe`](../cpp/crossmod.md#bus_subscribe)

### `bus::publish` {#fn.publish}

```rust
pub fn publish(topic: &str, payload: &str) -> Result<u32>
```

Broadcasts. It returns how many subscribers really ran, and 0 is a normal result
meaning nobody is listening.

- Parameters:
    - topic : `&str`
    - payload : `&str`
- Return type: `Result<u32>`
- Slots: [`bus_publish`](../cpp/crossmod.md#bus_publish)

### `bus::publish_vetoable` {#fn.publish_vetoable}

```rust
pub fn publish_vetoable(topic: &str, payload: &str) -> Result<Vetoable>
```

Broadcasts and collects the veto bit.

- Parameters:
    - topic : `&str`
    - payload : `&str`
- Return type: `Result<Vetoable>`
- Slots: [`bus_publish_vetoable`](../cpp/crossmod.md#bus_publish_vetoable)

### `bus::subscriber_count` {#fn.subscriber_count}

```rust
pub fn subscriber_count(topic: &str) -> u32
```

How many subscribers this topic currently has, across every mod.

For skipping the cost of assembling a payload nobody will read.

- Parameters:
    - topic : `&str`
- Return type: `u32`
- Slots: [`bus_subscriber_count`](../cpp/crossmod.md#bus_subscriber_count), [`bus_subscriber_count`](../cpp/crossmod.md#bus_subscriber_count)

## `Subscription` {#Subscription}

```rust
pub struct Subscription {
    // private fields
}
```

One subscription. Dropping it unsubscribes.

[`Subscription::forget`](bus.md#Subscription.forget) keeps it alive until the mod unloads, when the host clears it.

- Implements: `Drop`

### `Subscription::id` {#Subscription.id}

```rust
pub fn id(&self) -> u64
```

- Return type: `u64`

### `Subscription::topic` {#Subscription.topic}

```rust
pub fn topic(&self) -> &str
```

- Return type: `&str`

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

The result of one vetoable broadcast.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`
