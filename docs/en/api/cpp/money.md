# Economy

??? note "Section notes in abi.h"

    **Money (appended)**

    Backed by LegacyMoney, which is delay-loaded. The whole family degrades rather than crashing when the backend is absent or disabled, returning each slot's failure value. The semantics below come from LegacyMoney's source, not from guesswork:

    ```text
    - Amounts are always non-negative. val < 0 is rejected by the backend
      itself (the first check in LLMoney_Trans), and a negative set_money
      fails because the balance cannot be reduced to that target.
    - trans_money rejects from == to and applies the backend's configured
      pay_tax: the payee receives val - val * pay_tax, not val. To hand
      over the full amount, use add and reduce separately.
    - set_money's money is a target balance; the backend turns it into a
      single transfer internally.
    ```

## Slots {#slots}

### `get_money` {#get_money}

```c
int64_t (*get_money)(PierStr xuid);
```

Balance. Returns -1 on failure (empty xuid, database error, or absent backend); a real balance is never negative, so &lt; 0 means "cannot say". Note that it opens an account at the configured default for an unseen xuid, so this is not a side-effect-free read.

- Call: `api->get_money(xuid)`
- Parameters:
    - xuid : `PierStr`
- Return type: `int64_t`
- Section of abi.h: Money (appended)
- Position in the table: slot 92, counting from 0
- Callers in each binding:
    - Rust: [`money::balance`](../rust/money.md#fn.balance)
    - Go: [`Money`](../go/money.md#Money), [`Raw.GetMoney`](../go/raw.md#Raw.GetMoney)

### `set_money` {#set_money}

```c
bool (*set_money)(PierStr xuid, int64_t money);
```

Set to money, which is a target balance rather than a delta.

- Call: `api->set_money(xuid, money)`
- Parameters:
    - xuid : `PierStr`
    - money : `int64_t`
- Return type: `bool`
- Section of abi.h: Money (appended)
- Position in the table: slot 93, counting from 0
- Callers in each binding:
    - Rust: [`money::set`](../rust/money.md#fn.set)
    - Go: [`SetMoney`](../go/money.md#SetMoney), [`Raw.SetMoney`](../go/raw.md#Raw.SetMoney)

### `add_money` {#add_money}

```c
bool (*add_money)(PierStr xuid, int64_t money);
```

- Call: `api->add_money(xuid, money)`
- Parameters:
    - xuid : `PierStr`
    - money : `int64_t`
- Return type: `bool`
- Section of abi.h: Money (appended)
- Position in the table: slot 94, counting from 0
- Callers in each binding:
    - Rust: [`money::add`](../rust/money.md#fn.add)
    - Go: [`AddMoney`](../go/money.md#AddMoney), [`Raw.AddMoney`](../go/raw.md#Raw.AddMoney)

### `reduce_money` {#reduce_money}

```c
bool (*reduce_money)(PierStr xuid, int64_t money);
```

- Call: `api->reduce_money(xuid, money)`
- Parameters:
    - xuid : `PierStr`
    - money : `int64_t`
- Return type: `bool`
- Section of abi.h: Money (appended)
- Position in the table: slot 95, counting from 0
- Callers in each binding:
    - Rust: [`money::reduce`](../rust/money.md#fn.reduce)
    - Go: [`ReduceMoney`](../go/money.md#ReduceMoney), [`Raw.ReduceMoney`](../go/raw.md#Raw.ReduceMoney)

### `trans_money` {#trans_money}

```c
bool (*trans_money)(PierStr from, PierStr to, int64_t val, PierStr note);
```

An empty from or to means created from or destroyed into nothing. What the payee receives is reduced by `pay_tax` (see above). from == to fails.

- Call: `api->trans_money(from, to, val, note)`
- Parameters:
    - from : `PierStr`
    - to : `PierStr`
    - val : `int64_t`
    - note : `PierStr`
- Return type: `bool`
- Section of abi.h: Money (appended)
- Position in the table: slot 96, counting from 0
- Callers in each binding:
    - Rust: [`money::transfer`](../rust/money.md#fn.transfer)
    - Go: [`TransferMoney`](../go/money.md#TransferMoney), [`Raw.TransMoney`](../go/raw.md#Raw.TransMoney)

### `money_get_hist` {#money_get_hist}

```c
void (*money_get_hist)(PierStr xuid, int32_t timediff, void* ctx, PierStrSink sink);
```

- Call: `api->money_get_hist(xuid, timediff, ctx, sink)`
- Parameters:
    - xuid : `PierStr`
    - timediff : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- Section of abi.h: Money (appended)
- Position in the table: slot 97, counting from 0
- Callers in each binding:
    - Rust: [`money::try_history`](../rust/money.md#fn.try_history), [`money::history`](../rust/money.md#fn.history)
    - Go: [`Raw.MoneyGetHist`](../go/raw.md#Raw.MoneyGetHist)

### `money_clear_hist` {#money_clear_hist}

```c
void (*money_clear_hist)(int32_t difftime);
```

- Call: `api->money_clear_hist(difftime)`
- Parameters:
    - difftime : `int32_t`
- Section of abi.h: Money (appended)
- Position in the table: slot 98, counting from 0
- Callers in each binding:
    - Rust: [`money::try_clear_history`](../rust/money.md#fn.try_clear_history), [`money::clear_history`](../rust/money.md#fn.clear_history)
    - Go: [`Raw.MoneyClearHist`](../go/raw.md#Raw.MoneyClearHist)

### `money_listen_before_event` {#money_listen_before_event}

```c
void (*money_listen_before_event)(PierMoneyCb callback);
```

Register a before callback, which may veto. Several mods may each register one without overwriting the others, and registering the same function pointer twice is idempotent. The loader attributes each callback to its module and removes it when that mod unloads; LegacyMoney itself has no unregister interface, so this bookkeeping is the loader's. A callback outside every loaded pier mod cannot be attributed: that is logged at registration, and such a callback stays until the process exits. Registration does not require the backend to be ready yet: the loader installs the forwarding trampoline once it becomes available.

- Call: `api->money_listen_before_event(callback)`
- Parameters:
    - callback : `PierMoneyCb`
- Section of abi.h: Money (appended)
- Position in the table: slot 99, counting from 0
- Callers in each binding:
    - Rust: [`money::on_before`](../rust/money.md#fn.on_before), [`money::on_before_with`](../rust/money.md#fn.on_before_with)
    - Go: [`OnMoneyBefore`](../go/money.md#OnMoneyBefore)

### `money_listen_after_event` {#money_listen_after_event}

```c
void (*money_listen_after_event)(PierMoneyCb callback);
```

As above, but invoked after the change has happened; the return value is ignored.

- Call: `api->money_listen_after_event(callback)`
- Parameters:
    - callback : `PierMoneyCb`
- Section of abi.h: Money (appended)
- Position in the table: slot 100, counting from 0
- Callers in each binding:
    - Rust: [`money::on_after`](../rust/money.md#fn.on_after), [`money::on_after_with`](../rust/money.md#fn.on_after_with)
    - Go: [`OnMoneyAfter`](../go/money.md#OnMoneyAfter)

### `money_ranking` {#money_ranking}

```c
void (*money_ranking)(uint16_t num, void* ctx, PierStrSink sink);
```

- Call: `api->money_ranking(num, ctx, sink)`
- Parameters:
    - num : `uint16_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- Section of abi.h: Money (appended)
- Position in the table: slot 101, counting from 0
- Callers in each binding:
    - Rust: [`money::try_ranking`](../rust/money.md#fn.try_ranking), [`money::ranking`](../rust/money.md#fn.ranking)
    - Go: [`Raw.MoneyRanking`](../go/raw.md#Raw.MoneyRanking)

## Types {#types}

### `PierMoneyCb` {#PierMoneyCb}

```c
typedef bool (*PierMoneyCb)(PierMoneyEvent type, PierStr from, PierStr to, int64_t value);
```

legacymoney event callback. Return false to veto the change; only the before callback's return value is honoured, an after callback's is ignored.

from and to are less obvious than their names suggest. This is LegacyMoney's own shape, forwarded as-is rather than "corrected":

```text
- PIER_MONEY_TRANS: from is the payer, to the payee, both non-empty.
- PIER_MONEY_ADD / REDUCE / SET: from is always the empty string and to is
  the xuid being operated on, including for REDUCE, where the money is
  taken from to. To learn whose balance changed, always read to.
```

value is the delta for ADD, REDUCE and TRANS, and the target balance (not a delta) for SET.

Every mod's callback is invoked; an earlier veto does not skip the rest, so the outcome does not depend on registration order. The change is vetoed if any callback returns false.

## `PierMoneyEvent` {#PierMoneyEvent}

legacymoney event types, used by the server-side economy capability.

| Name | Value | Description |
|---|---|---|
| <span id="PIER_MONEY_SET"></span>`PIER_MONEY_SET` | `0` |  |
| <span id="PIER_MONEY_ADD"></span>`PIER_MONEY_ADD` | `1` |  |
| <span id="PIER_MONEY_REDUCE"></span>`PIER_MONEY_REDUCE` | `2` |  |
| <span id="PIER_MONEY_TRANS"></span>`PIER_MONEY_TRANS` | `3` |  |
