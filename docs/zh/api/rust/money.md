# levilamina::money · 经济

经济。

**后端是延迟加载的，整组接口在缺少后端时降级，不会崩溃**

没有安装经济后端，或者后端被禁用时，每个槽位返回各自的失败值。这一层把它们转成 `Err`，只有 [`balance`](money.md#fn.balance) 例外；见它自己的文档。

**金额从不为负，转账要扣税**

后端自己会拒绝负的金额。[`transfer`](money.md#fn.transfer) 按后端配置的 `pay_tax` 抽成，收款方收到的是 `val - val * pay_tax`，不是 `val`。要转出全额，就分别调用 [`add`](money.md#fn.add) 和 [`reduce`](money.md#fn.reduce)。[`set`](money.md#fn.set) 的参数是目标余额，不是差额。

## 函数 {#functions}

### `money::balance` {#fn.balance}

```rust
pub fn balance(xuid: &str) -> Result<i64>
```

余额。

它返回 `Err`，不返回 -1：真实的余额从不为负，所以负值只能说明这个问题答不上来，原因是 xuid 为空、数据库出错，或者后端不在。

注意这次读取有副作用：对一个没见过的 xuid，后端会按配置的默认值开一个账户。

- 参数：
    - xuid : `&str`
- 返回值类型：`Result<i64>`
- 对应槽位：[`get_money`](../cpp/money.md#get_money)

### `money::set` {#fn.set}

```rust
pub fn set(xuid: &str, amount: i64) -> Result<()>
```

设为一个目标余额。

- 参数：
    - xuid : `&str`
    - amount : `i64`
- 返回值类型：`Result<()>`
- 对应槽位：[`set_money`](../cpp/money.md#set_money)

### `money::add` {#fn.add}

```rust
pub fn add(xuid: &str, delta: i64) -> Result<()>
```

- 参数：
    - xuid : `&str`
    - delta : `i64`
- 返回值类型：`Result<()>`
- 对应槽位：[`add_money`](../cpp/money.md#add_money)

### `money::reduce` {#fn.reduce}

```rust
pub fn reduce(xuid: &str, delta: i64) -> Result<()>
```

- 参数：
    - xuid : `&str`
    - delta : `i64`
- 返回值类型：`Result<()>`
- 对应槽位：[`reduce_money`](../cpp/money.md#reduce_money)

### `money::transfer` {#fn.transfer}

```rust
pub fn transfer(from: &str, to: &str, value: i64, note: &str) -> Result<()>
```

转账。`from == to` 会被后端拒绝，收款方收到的金额已经扣过税；见模块文档。

- 参数：
    - from : `&str`
    - to : `&str`
    - value : `i64`
    - note : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`trans_money`](../cpp/money.md#trans_money)

### `money::try_history` {#fn.try_history}

```rust
pub fn try_history(xuid: &str, seconds: i32) -> Result<Vec<String>>
```

最近 `seconds` 秒内的交易，每行一笔；宿主没有 history 槽位时返回 `Err`，空列表会把这一点掩盖掉。

- 参数：
    - xuid : `&str`
    - seconds : `i32`
- 返回值类型：`Result<Vec<String>>`
- 对应槽位：[`money_get_hist`](../cpp/money.md#money_get_hist)

### `money::history` {#fn.history}

!!! warning "已弃用（自 26.51.2 起）"

    请用 `try_history`：宿主没有 history 槽位时，这个函数回答的是空列表

```rust
pub fn history(xuid: &str, seconds: i32) -> Vec<String>
```

最近 `seconds` 秒内的交易；宿主读不出来时返回空列表。

- 参数：
    - xuid : `&str`
    - seconds : `i32`
- 返回值类型：`Vec<String>`
- 对应槽位：[`money_get_hist`](../cpp/money.md#money_get_hist)

### `money::try_clear_history` {#fn.try_clear_history}

```rust
pub fn try_clear_history(seconds: i32) -> Result<()>
```

清除早于 `seconds` 秒的交易；宿主做不到时返回 `Err`。

- 参数：
    - seconds : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`money_clear_hist`](../cpp/money.md#money_clear_hist)

### `money::clear_history` {#fn.clear_history}

!!! warning "已弃用（自 26.51.2 起）"

    请用 `try_clear_history`：在不能清除的宿主上，这个函数什么都不做，也不提示

```rust
pub fn clear_history(seconds: i32)
```

清除早于 `seconds` 秒的交易；宿主做不到时什么都不做。

- 参数：
    - seconds : `i32`
- 对应槽位：[`money_clear_hist`](../cpp/money.md#money_clear_hist)

### `money::try_ranking` {#fn.try_ranking}

```rust
pub fn try_ranking(top_n: u16) -> Result<Vec<String>>
```

富豪榜的前 `top_n` 名，每行一个；宿主没有 ranking 槽位时返回 `Err`。

- 参数：
    - top_n : `u16`
- 返回值类型：`Result<Vec<String>>`
- 对应槽位：[`money_ranking`](../cpp/money.md#money_ranking)

### `money::ranking` {#fn.ranking}

!!! warning "已弃用（自 26.51.2 起）"

    请用 `try_ranking`：宿主没有 ranking 槽位时，这个函数回答的是空列表

```rust
pub fn ranking(top_n: u16) -> Vec<String>
```

富豪榜的前 `top_n` 名；宿主读不出来时返回空列表。

- 参数：
    - top_n : `u16`
- 返回值类型：`Vec<String>`
- 对应槽位：[`money_ranking`](../cpp/money.md#money_ranking)

### `money::on_before` {#fn.on_before}

```rust
pub fn on_before(callback: sys::PierMoneyCb) -> Result<()>
```

注册一个在事件之前运行的回调，返回 `false` 会否决这笔交易。

多个模组可以各注册自己的，互不覆盖；同一个函数指针注册两次只算一次。宿主按模块记账，模组卸载时移除它们。

**回调是全局的，不是每次调用一个**

ABI 在这里接收一个裸函数指针，没有 `user` 参数，所以它装不下一个捕获了环境的闭包。因此这一层接收 `fn`，不接收 `impl FnMut`：后者需要把状态藏在一个全局变量里，而模组卸载以后还会有人碰它。要带状态，就在模组自己里面用一个 `static`，并在 `on_unload` 里清空。

- 参数：
    - callback : `sys::PierMoneyCb`
- 返回值类型：`Result<()>`
- 对应槽位：[`money_listen_before_event`](../cpp/money.md#money_listen_before_event)

### `money::on_after` {#fn.on_after}

```rust
pub fn on_after(callback: sys::PierMoneyCb) -> Result<()>
```

注册一个在事件之后运行的回调。返回值被忽略。

- 参数：
    - callback : `sys::PierMoneyCb`
- 返回值类型：`Result<()>`
- 对应槽位：[`money_listen_after_event`](../cpp/money.md#money_listen_after_event)

### `money::on_before_with` {#fn.on_before_with}

```rust
pub fn on_before_with(f: impl FnMut(&MoneyEvent) -> bool + Send + 'static) -> Result<()>
```

注册一个可以捕获环境的事件前回调。返回 `false` 会否决这笔交易。

每个模组只有一个：再次调用会替换之前的那个，两个不会都运行。要运行好几个，就在闭包里自己分派。这样做有原因：ABI 没有 `user` 参数，所以「是哪个闭包」只能由这一侧的一个全局变量回答，而一个全局变量只装得下一个。

- 参数：
    - f : `impl FnMut(&MoneyEvent) -> bool + Send + 'static`
- 返回值类型：`Result<()>`
- 对应槽位：[`money_listen_before_event`](../cpp/money.md#money_listen_before_event)

### `money::on_after_with` {#fn.on_after_with}

```rust
pub fn on_after_with(f: impl FnMut(&MoneyEvent) + Send + 'static) -> Result<()>
```

同上，事件后的版本。返回值被忽略，所以闭包不返回任何东西。

- 参数：
    - f : `impl FnMut(&MoneyEvent) + Send + 'static`
- 返回值类型：`Result<()>`
- 对应槽位：[`money_listen_after_event`](../cpp/money.md#money_listen_after_event)

### `money::event_from_raw` {#fn.event_from_raw}

```rust
pub unsafe fn event_from_raw(
    kind: sys::PierMoneyEvent,
    from: sys::PierStr,
    to: sys::PierStr,
    value: i64,
) -> MoneyEvent
```

把 ABI 传来的四个参数组装成一个 [`MoneyEvent`](money.md#MoneyEvent)。

给自己写 `extern "C"` 回调的人用：在函数体第一行调用它，剩下的就都是安全的 Rust。

**安全性**

这四个参数必须是宿主在回调期间传来的那一组，`from` 和 `to` 只在回调期间有效。

- 参数：
    - kind : `sys::PierMoneyEvent`
    - from : `sys::PierStr`
    - to : `sys::PierStr`
    - value : `i64`
- 返回值类型：`MoneyEvent`

### `money::guard` {#fn.guard}

```rust
pub fn guard(f: impl FnOnce() -> bool) -> bool
```

截住经济回调里的 panic。

panic 跨过 `extern "C"` 是未定义行为，而经济回调是由宿主直接调用的。它把回调包起来，发生 panic 时视为不否决，并记录日志：否决是更强的动作，不应该由一个 bug 触发。

- 参数：
    - f : `impl FnOnce() -> bool`
- 返回值类型：`bool`

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

一次经济事件做了什么。`Unknown` 带着一个这个 SDK 没有列出的值，更新的经济后端可能会发出这样的值；否决回调遇到不认识的种类，应当拒绝，不要去猜。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

### `MoneyEventKind::from_raw` {#MoneyEventKind.from_raw}

```rust
pub fn from_raw(raw: sys::PierMoneyEvent) -> Self
```

映射 ABI 的值，列表之外的值保留为 `Unknown`。

- 参数：
    - raw : `sys::PierMoneyEvent`
- 返回值类型：`Self`

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

一次经济事件。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`、`Eq`

### `MoneyEvent::kind_checked` {#MoneyEvent.kind_checked}

```rust
pub fn kind_checked(&self) -> MoneyEventKind
```

事件的种类；这个 SDK 没有列出的值保留为 `MoneyEventKind::Unknown`。

- 返回值类型：`MoneyEventKind`
