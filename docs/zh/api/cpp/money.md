# 经济

??? note "abi.h 里的分节说明"

    **经济（追加）（`Money (appended)`）**

    由 LegacyMoney 提供，它是延迟加载的。后端不在或被禁用时，这一组槽位整体降级，不会崩溃，各自返回失败值。下面的语义取自 LegacyMoney 的源码，没有一条是猜的：

    - 金额总是非负的。`val < 0` 由后端自己拒绝（`LLMoney_Trans` 的第一项检查）；`set_money` 传负数会失败，因为余额不可能减到那个目标值。
    - `trans_money` 拒绝 `from == to`，并按后端配置的 `pay_tax` 扣税：收款方收到的是 `val - val * pay_tax`，不是 `val`。要转出全额，请分别加钱和减钱。
    - `set_money` 的 `money` 是目标余额；后端在内部把它变成一笔转账。

## 槽位 {#slots}

### `get_money` {#get_money}

```c
int64_t (*get_money)(PierStr xuid);
```

余额。失败时返回 -1（xuid 为空、数据库出错，或者后端不在）；真实的余额不会是负数，所以小于 0 表示「说不出来」。注意：遇到没见过的 xuid，它会按配置的默认值开一个账户，所以这次读取是有副作用的。

- 调用形式：`api->get_money(xuid)`
- 参数：
    - xuid : `PierStr`
- 返回值类型：`int64_t`
- 所在分节：经济（追加）（`Money (appended)`）
- 表内序号：第 92 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`money::balance`](../rust/money.md#fn.balance)
    - Go：[`Money`](../go/money.md#Money)、[`Raw.GetMoney`](../go/raw.md#Raw.GetMoney)

### `set_money` {#set_money}

```c
bool (*set_money)(PierStr xuid, int64_t money);
```

把余额设为 `money`。它是目标余额，不是差额。

- 调用形式：`api->set_money(xuid, money)`
- 参数：
    - xuid : `PierStr`
    - money : `int64_t`
- 返回值类型：`bool`
- 所在分节：经济（追加）（`Money (appended)`）
- 表内序号：第 93 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`money::set`](../rust/money.md#fn.set)
    - Go：[`SetMoney`](../go/money.md#SetMoney)、[`Raw.SetMoney`](../go/raw.md#Raw.SetMoney)

### `add_money` {#add_money}

```c
bool (*add_money)(PierStr xuid, int64_t money);
```

- 调用形式：`api->add_money(xuid, money)`
- 参数：
    - xuid : `PierStr`
    - money : `int64_t`
- 返回值类型：`bool`
- 所在分节：经济（追加）（`Money (appended)`）
- 表内序号：第 94 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`money::add`](../rust/money.md#fn.add)
    - Go：[`AddMoney`](../go/money.md#AddMoney)、[`Raw.AddMoney`](../go/raw.md#Raw.AddMoney)

### `reduce_money` {#reduce_money}

```c
bool (*reduce_money)(PierStr xuid, int64_t money);
```

- 调用形式：`api->reduce_money(xuid, money)`
- 参数：
    - xuid : `PierStr`
    - money : `int64_t`
- 返回值类型：`bool`
- 所在分节：经济（追加）（`Money (appended)`）
- 表内序号：第 95 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`money::reduce`](../rust/money.md#fn.reduce)
    - Go：[`ReduceMoney`](../go/money.md#ReduceMoney)、[`Raw.ReduceMoney`](../go/raw.md#Raw.ReduceMoney)

### `trans_money` {#trans_money}

```c
bool (*trans_money)(PierStr from, PierStr to, int64_t val, PierStr note);
```

`from` 或 `to` 为空，表示钱是凭空生出的，或者转出后就消失了。收款方收到的金额会按 `pay_tax` 扣减（见上面的说明）。`from == to` 会失败。

- 调用形式：`api->trans_money(from, to, val, note)`
- 参数：
    - from : `PierStr`
    - to : `PierStr`
    - val : `int64_t`
    - note : `PierStr`
- 返回值类型：`bool`
- 所在分节：经济（追加）（`Money (appended)`）
- 表内序号：第 96 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`money::transfer`](../rust/money.md#fn.transfer)
    - Go：[`TransferMoney`](../go/money.md#TransferMoney)、[`Raw.TransMoney`](../go/raw.md#Raw.TransMoney)

### `money_get_hist` {#money_get_hist}

```c
void (*money_get_hist)(PierStr xuid, int32_t timediff, void* ctx, PierStrSink sink);
```

- 调用形式：`api->money_get_hist(xuid, timediff, ctx, sink)`
- 参数：
    - xuid : `PierStr`
    - timediff : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- 所在分节：经济（追加）（`Money (appended)`）
- 表内序号：第 97 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`money::try_history`](../rust/money.md#fn.try_history)、[`money::history`](../rust/money.md#fn.history)
    - Go：[`Raw.MoneyGetHist`](../go/raw.md#Raw.MoneyGetHist)

### `money_clear_hist` {#money_clear_hist}

```c
void (*money_clear_hist)(int32_t difftime);
```

- 调用形式：`api->money_clear_hist(difftime)`
- 参数：
    - difftime : `int32_t`
- 所在分节：经济（追加）（`Money (appended)`）
- 表内序号：第 98 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`money::try_clear_history`](../rust/money.md#fn.try_clear_history)、[`money::clear_history`](../rust/money.md#fn.clear_history)
    - Go：[`Raw.MoneyClearHist`](../go/raw.md#Raw.MoneyClearHist)

### `money_listen_before_event` {#money_listen_before_event}

```c
void (*money_listen_before_event)(PierMoneyCb callback);
```

注册一个变动前的回调，它可以否决这次变动。多个模组可以各注册一个，互不覆盖；同一个函数指针注册两次只算一次。加载器把每个回调记在它所在的模块名下，那个模组卸载时就移除它；LegacyMoney 自己没有注销接口，所以这份记录由加载器负责。不在任何已加载的 Pier 模组里的回调无法归属：注册时会记一条日志，这样的回调一直留到进程退出。注册时不要求后端已经就绪：后端可用以后，加载器才装上转发用的跳板函数。

- 调用形式：`api->money_listen_before_event(callback)`
- 参数：
    - callback : `PierMoneyCb`
- 所在分节：经济（追加）（`Money (appended)`）
- 表内序号：第 99 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`money::on_before`](../rust/money.md#fn.on_before)、[`money::on_before_with`](../rust/money.md#fn.on_before_with)
    - Go：[`OnMoneyBefore`](../go/money.md#OnMoneyBefore)

### `money_listen_after_event` {#money_listen_after_event}

```c
void (*money_listen_after_event)(PierMoneyCb callback);
```

同上，但在变动发生之后调用；返回值会被忽略。

- 调用形式：`api->money_listen_after_event(callback)`
- 参数：
    - callback : `PierMoneyCb`
- 所在分节：经济（追加）（`Money (appended)`）
- 表内序号：第 100 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`money::on_after`](../rust/money.md#fn.on_after)、[`money::on_after_with`](../rust/money.md#fn.on_after_with)
    - Go：[`OnMoneyAfter`](../go/money.md#OnMoneyAfter)

### `money_ranking` {#money_ranking}

```c
void (*money_ranking)(uint16_t num, void* ctx, PierStrSink sink);
```

- 调用形式：`api->money_ranking(num, ctx, sink)`
- 参数：
    - num : `uint16_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- 所在分节：经济（追加）（`Money (appended)`）
- 表内序号：第 101 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`money::try_ranking`](../rust/money.md#fn.try_ranking)、[`money::ranking`](../rust/money.md#fn.ranking)
    - Go：[`Raw.MoneyRanking`](../go/raw.md#Raw.MoneyRanking)

## 类型 {#types}

### `PierMoneyCb` {#PierMoneyCb}

```c
typedef bool (*PierMoneyCb)(PierMoneyEvent type, PierStr from, PierStr to, int64_t value);
```

LegacyMoney 的事件回调。返回 false 否决这次变动；只有变动前回调的返回值有效，变动后回调的返回值会被忽略。

`from` 和 `to` 的含义没有名字看起来那么直白。这是 LegacyMoney 自己的形状，原样转发，没有做「修正」：

- `PIER_MONEY_TRANS`：`from` 是付款方，`to` 是收款方，两者都不为空。
- `PIER_MONEY_ADD` / `REDUCE` / `SET`：`from` 总是空字符串，`to` 是被操作的 xuid；`REDUCE` 也是这样，钱是从 `to` 那里扣的。想知道谁的余额变了，一律读 `to`。

`value` 在 ADD、REDUCE 和 TRANS 时是变化量，在 SET 时是目标余额（不是变化量）。

每个模组的回调都会被调用；前面的否决不会跳过后面的回调，所以结果和注册顺序无关。只要有一个回调返回 false，这次变动就被否决。

## `PierMoneyEvent` {#PierMoneyEvent}

LegacyMoney 的事件类型，供服务器侧的经济能力使用。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PIER_MONEY_SET"></span>`PIER_MONEY_SET` | `0` |  |
| <span id="PIER_MONEY_ADD"></span>`PIER_MONEY_ADD` | `1` |  |
| <span id="PIER_MONEY_REDUCE"></span>`PIER_MONEY_REDUCE` | `2` |  |
| <span id="PIER_MONEY_TRANS"></span>`PIER_MONEY_TRANS` | `3` |  |
