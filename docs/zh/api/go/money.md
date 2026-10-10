# Go：经济

## 函数 {#functions}

### `OnMoneyBefore` {#OnMoneyBefore}

```go
func OnMoneyBefore(fn func(MoneyEvent) bool) error
```

在每一笔经济交易之前调用 `fn`；返回 false 否决这笔交易。

- 参数：
    - fn : `func(MoneyEvent) bool`
- 返回值类型：`error`
- 对应槽位：[`money_listen_before_event`](../cpp/money.md#money_listen_before_event)

### `OnMoneyAfter` {#OnMoneyAfter}

```go
func OnMoneyAfter(fn func(MoneyEvent)) error
```

在每一笔经济交易之后调用 `fn`。

- 参数：
    - fn : `func(MoneyEvent)`
- 返回值类型：`error`
- 对应槽位：[`money_listen_after_event`](../cpp/money.md#money_listen_after_event)

### `Money` {#Money}

```go
func Money(xuid string) (int64, error)
```

一个余额。读不出来（xuid 为空，或者没有经济后端）时宿主给出负数，这里把它作为错误返回：真实的余额不会是负数。

- 参数：
    - xuid : `string`
- 返回值类型：`(int64, error)`
- 对应槽位：[`get_money`](../cpp/money.md#get_money)

### `SetMoney` {#SetMoney}

```go
func SetMoney(xuid string, amount int64) error
```

设置余额。

- 参数：
    - xuid : `string`
    - amount : `int64`
- 返回值类型：`error`
- 对应槽位：[`set_money`](../cpp/money.md#set_money)

### `AddMoney` {#AddMoney}

```go
func AddMoney(xuid string, delta int64) error
```

增加余额。

- 参数：
    - xuid : `string`
    - delta : `int64`
- 返回值类型：`error`
- 对应槽位：[`add_money`](../cpp/money.md#add_money)

### `ReduceMoney` {#ReduceMoney}

```go
func ReduceMoney(xuid string, delta int64) error
```

减少余额。

- 参数：
    - xuid : `string`
    - delta : `int64`
- 返回值类型：`error`
- 对应槽位：[`reduce_money`](../cpp/money.md#reduce_money)

### `TransferMoney` {#TransferMoney}

```go
func TransferMoney(from, to string, value int64, note string) error
```

在两个余额之间转移 `value`；收款方收到的是扣税后的金额。

- 参数：
    - from : `string`
    - to : `string`
    - value : `int64`
    - note : `string`
- 返回值类型：`error`
- 对应槽位：[`trans_money`](../cpp/money.md#trans_money)

## `MoneyKind` {#MoneyKind}

```go
type MoneyKind int32
```

一次经济事件做了什么，是 `Money*` 几种之一。超出这几种的值来自更新的经济后端；否决监听器应当拒绝它，不要去猜。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="MoneySet"></span>`MoneySet` | `0` |  |
| <span id="MoneyAdd"></span>`MoneyAdd` | `1` |  |
| <span id="MoneyReduce"></span>`MoneyReduce` | `2` |  |
| <span id="MoneyTrans"></span>`MoneyTrans` | `3` |  |

## `MoneyEvent` {#MoneyEvent}

```go
type MoneyEvent struct {
    Kind  MoneyKind
    From  string
    To    string
    Value int64
}
```

一笔经济交易。
