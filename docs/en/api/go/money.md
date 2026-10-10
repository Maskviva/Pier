# Go: Economy

## Functions {#functions}

### `OnMoneyBefore` {#OnMoneyBefore}

```go
func OnMoneyBefore(fn func(MoneyEvent) bool) error
```

OnMoneyBefore calls fn before every economy transaction; returning false vetoes it.

- Parameters:
    - fn : `func(MoneyEvent) bool`
- Return type: `error`
- Slots: [`money_listen_before_event`](../cpp/money.md#money_listen_before_event)

### `OnMoneyAfter` {#OnMoneyAfter}

```go
func OnMoneyAfter(fn func(MoneyEvent)) error
```

OnMoneyAfter calls fn after every economy transaction.

- Parameters:
    - fn : `func(MoneyEvent)`
- Return type: `error`
- Slots: [`money_listen_after_event`](../cpp/money.md#money_listen_after_event)

### `Money` {#Money}

```go
func Money(xuid string) (int64, error)
```

Money is a balance. A negative answer means it cannot be read, an empty xuid or no economy backend, and is an error: a real balance is never negative.

- Parameters:
    - xuid : `string`
- Return type: `(int64, error)`
- Slots: [`get_money`](../cpp/money.md#get_money)

### `SetMoney` {#SetMoney}

```go
func SetMoney(xuid string, amount int64) error
```

SetMoney sets a balance.

- Parameters:
    - xuid : `string`
    - amount : `int64`
- Return type: `error`
- Slots: [`set_money`](../cpp/money.md#set_money)

### `AddMoney` {#AddMoney}

```go
func AddMoney(xuid string, delta int64) error
```

AddMoney adds to a balance.

- Parameters:
    - xuid : `string`
    - delta : `int64`
- Return type: `error`
- Slots: [`add_money`](../cpp/money.md#add_money)

### `ReduceMoney` {#ReduceMoney}

```go
func ReduceMoney(xuid string, delta int64) error
```

ReduceMoney takes from a balance.

- Parameters:
    - xuid : `string`
    - delta : `int64`
- Return type: `error`
- Slots: [`reduce_money`](../cpp/money.md#reduce_money)

### `TransferMoney` {#TransferMoney}

```go
func TransferMoney(from, to string, value int64, note string) error
```

TransferMoney moves value between two balances; the recipient receives it taxed.

- Parameters:
    - from : `string`
    - to : `string`
    - value : `int64`
    - note : `string`
- Return type: `error`
- Slots: [`trans_money`](../cpp/money.md#trans_money)

## `MoneyKind` {#MoneyKind}

```go
type MoneyKind int32
```

MoneyKind is what an economy event does, one of the Money\* kinds. A value outside them is one a newer economy backend sent; a veto listener should refuse it rather than guess.

| Name | Value | Description |
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

MoneyEvent is one economy transaction.
