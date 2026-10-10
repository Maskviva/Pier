# levilamina::money

Economy.

**The backend is delay-loaded, and the whole family degrades rather than crashing**

Without an economy backend installed, or with it disabled, each slot returns its own failure
value. This layer translates those into `Err`, with [`balance`](money.md#fn.balance) the one exception; see its own
documentation.

**Amounts are never negative and a transfer is taxed**

The backend refuses a negative amount itself. [`transfer`](money.md#fn.transfer) takes a cut according to the
`pay_tax` the backend is configured with, so the recipient receives `val - val * pay_tax` and
not `val`. Moving the full amount means separate [`add`](money.md#fn.add) and [`reduce`](money.md#fn.reduce) calls. The argument of
[`set`](money.md#fn.set) is the target balance and not a delta.

## Functions {#functions}

### `money::balance` {#fn.balance}

```rust
pub fn balance(xuid: &str) -> Result<i64>
```

The balance.

It returns `Err` and not -1: a real balance is never negative, so a negative value can
only mean the question cannot be answered, because the xuid is empty, the database
failed, or the backend is absent.

Note this read is not free of side effects: the backend opens an account at the
configured default for an xuid it has not seen.

- Parameters:
    - xuid : `&str`
- Return type: `Result<i64>`
- Slots: [`get_money`](../cpp/money.md#get_money)

### `money::set` {#fn.set}

```rust
pub fn set(xuid: &str, amount: i64) -> Result<()>
```

Sets it to a target balance.

- Parameters:
    - xuid : `&str`
    - amount : `i64`
- Return type: `Result<()>`
- Slots: [`set_money`](../cpp/money.md#set_money)

### `money::add` {#fn.add}

```rust
pub fn add(xuid: &str, delta: i64) -> Result<()>
```

- Parameters:
    - xuid : `&str`
    - delta : `i64`
- Return type: `Result<()>`
- Slots: [`add_money`](../cpp/money.md#add_money)

### `money::reduce` {#fn.reduce}

```rust
pub fn reduce(xuid: &str, delta: i64) -> Result<()>
```

- Parameters:
    - xuid : `&str`
    - delta : `i64`
- Return type: `Result<()>`
- Slots: [`reduce_money`](../cpp/money.md#reduce_money)

### `money::transfer` {#fn.transfer}

```rust
pub fn transfer(from: &str, to: &str, value: i64, note: &str) -> Result<()>
```

Transfers. A `from == to` is refused by the backend, and the amount the recipient
receives is already taxed; see the module documentation.

- Parameters:
    - from : `&str`
    - to : `&str`
    - value : `i64`
    - note : `&str`
- Return type: `Result<()>`
- Slots: [`trans_money`](../cpp/money.md#trans_money)

### `money::try_history` {#fn.try_history}

```rust
pub fn try_history(xuid: &str, seconds: i32) -> Result<Vec<String>>
```

The transactions of the last `seconds` seconds, one per line, and `Err` for a host
without the history slot, which an empty list would hide.

- Parameters:
    - xuid : `&str`
    - seconds : `i32`
- Return type: `Result<Vec<String>>`
- Slots: [`money_get_hist`](../cpp/money.md#money_get_hist)

### `money::history` {#fn.history}

!!! warning "Deprecated since 26.51.2"

    use try_history: this answers an empty list when the host has no history slot

```rust
pub fn history(xuid: &str, seconds: i32) -> Vec<String>
```

The transactions of the last `seconds` seconds, and an empty list when the host cannot
read them.

- Parameters:
    - xuid : `&str`
    - seconds : `i32`
- Return type: `Vec<String>`
- Slots: [`money_get_hist`](../cpp/money.md#money_get_hist)

### `money::try_clear_history` {#fn.try_clear_history}

```rust
pub fn try_clear_history(seconds: i32) -> Result<()>
```

Clears transactions older than `seconds` seconds, and `Err` for a host that cannot.

- Parameters:
    - seconds : `i32`
- Return type: `Result<()>`
- Slots: [`money_clear_hist`](../cpp/money.md#money_clear_hist)

### `money::clear_history` {#fn.clear_history}

!!! warning "Deprecated since 26.51.2"

    use try_clear_history: this does nothing, silently, on a host that cannot clear

```rust
pub fn clear_history(seconds: i32)
```

Clears transactions older than `seconds` seconds, doing nothing on a host that cannot.

- Parameters:
    - seconds : `i32`
- Slots: [`money_clear_hist`](../cpp/money.md#money_clear_hist)

### `money::try_ranking` {#fn.try_ranking}

```rust
pub fn try_ranking(top_n: u16) -> Result<Vec<String>>
```

The top `top_n` of the rich list, one per line, and `Err` for a host without the
ranking slot.

- Parameters:
    - top_n : `u16`
- Return type: `Result<Vec<String>>`
- Slots: [`money_ranking`](../cpp/money.md#money_ranking)

### `money::ranking` {#fn.ranking}

!!! warning "Deprecated since 26.51.2"

    use try_ranking: this answers an empty list when the host has no ranking slot

```rust
pub fn ranking(top_n: u16) -> Vec<String>
```

The top `top_n` of the rich list, and an empty list when the host cannot read it.

- Parameters:
    - top_n : `u16`
- Return type: `Vec<String>`
- Slots: [`money_ranking`](../cpp/money.md#money_ranking)

### `money::on_before` {#fn.on_before}

```rust
pub fn on_before(callback: sys::PierMoneyCb) -> Result<()>
```

Registers a callback that runs before the event, where returning `false` vetoes the
transaction.

Several mods may each register their own without overwriting one another, and
registering the same function pointer twice is idempotent.
The host accounts per module and removes them when the mod unloads.

**The callback is global and not one per call**

The ABI takes a raw function pointer here with no `user` parameter, so it cannot hold a
closure that captured its environment. This layer therefore takes an `fn` and not an
`impl FnMut`, which would need the state hidden in a global that is still touched after
the mod unloads. Carrying state means a `static` inside the mod itself, cleared in
`on_unload`.

- Parameters:
    - callback : `sys::PierMoneyCb`
- Return type: `Result<()>`
- Slots: [`money_listen_before_event`](../cpp/money.md#money_listen_before_event)

### `money::on_after` {#fn.on_after}

```rust
pub fn on_after(callback: sys::PierMoneyCb) -> Result<()>
```

Registers a callback that runs after the event. The return value is ignored.

- Parameters:
    - callback : `sys::PierMoneyCb`
- Return type: `Result<()>`
- Slots: [`money_listen_after_event`](../cpp/money.md#money_listen_after_event)

### `money::on_before_with` {#fn.on_before_with}

```rust
pub fn on_before_with(f: impl FnMut(&MoneyEvent) -> bool + Send + 'static) -> Result<()>
```

Registers a before-callback that can capture its environment. Returning `false` vetoes
the transaction.

There is one per mod: calling again replaces the previous one rather than running both.
Running several means dispatching inside the closure yourself. This is not laziness:
the ABI has no `user` parameter, so which closure can only be answered by a global on
this side, and one global holds one.

- Parameters:
    - f : `impl FnMut(&MoneyEvent) -> bool + Send + 'static`
- Return type: `Result<()>`
- Slots: [`money_listen_before_event`](../cpp/money.md#money_listen_before_event)

### `money::on_after_with` {#fn.on_after_with}

```rust
pub fn on_after_with(f: impl FnMut(&MoneyEvent) + Send + 'static) -> Result<()>
```

As above, the after version. The return value is ignored, so the closure returns
nothing.

- Parameters:
    - f : `impl FnMut(&MoneyEvent) + Send + 'static`
- Return type: `Result<()>`
- Slots: [`money_listen_after_event`](../cpp/money.md#money_listen_after_event)

### `money::event_from_raw` {#fn.event_from_raw}

```rust
pub unsafe fn event_from_raw(
    kind: sys::PierMoneyEvent,
    from: sys::PierStr,
    to: sys::PierStr,
    value: i64,
) -> MoneyEvent
```

Assembles the four arguments the ABI passes into a [`MoneyEvent`](money.md#MoneyEvent).

For anyone writing their own `extern "C"` callback: call it on the first line of the
body and the rest is safe Rust.

**Safety**
The four arguments must be the set the host passed during the callback, and `from` and
`to` are valid only for its duration.

- Parameters:
    - kind : `sys::PierMoneyEvent`
    - from : `sys::PierStr`
    - to : `sys::PierStr`
    - value : `i64`
- Return type: `MoneyEvent`

### `money::guard` {#fn.guard}

```rust
pub fn guard(f: impl FnOnce() -> bool) -> bool
```

Catches a panic inside an economy callback.

A panic crossing `extern "C"` is undefined behavior, and an economy callback is called
directly by the host.
This wraps it, treating a panic as no veto and logging: a veto is the stronger action
and should not be triggered by a bug.

- Parameters:
    - f : `impl FnOnce() -> bool`
- Return type: `bool`

## `MoneyEventKind` {#MoneyEventKind}

```rust
pub enum MoneyEventKind {
        Set,
        Add,
        Reduce,
        Trans,
        Unknown(i32),
}
```

What an economy event does. `Unknown` carries a value this SDK does not list, which a
newer economy backend may send; a veto callback deciding on an unknown kind should
refuse rather than guess.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

### `MoneyEventKind::from_raw` {#MoneyEventKind.from_raw}

```rust
pub fn from_raw(raw: sys::PierMoneyEvent) -> Self
```

Maps the ABI value, keeping one outside the list as `Unknown`.

- Parameters:
    - raw : `sys::PierMoneyEvent`
- Return type: `Self`

## `MoneyEvent` {#MoneyEvent}

```rust
pub struct MoneyEvent {
    /// The raw kind as the host sent it. [`MoneyEvent::kind_checked`] names it.
    pub kind: sys::PierMoneyEvent,
    /// The xuid of the payer. An empty string means created out of nothing.
    pub from: String,
    /// The xuid of the recipient. An empty string means destroyed into nothing.
    pub to: String,
    pub value: i64,
}
```

One economy event.

- Implements: `Debug`, `Clone`, `PartialEq`, `Eq`

### `MoneyEvent::kind_checked` {#MoneyEvent.kind_checked}

```rust
pub fn kind_checked(&self) -> MoneyEventKind
```

The kind, with a value this SDK does not list kept as `MoneyEventKind::Unknown`.

- Return type: `MoneyEventKind`
