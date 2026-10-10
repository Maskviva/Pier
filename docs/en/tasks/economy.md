# 💰 Economy API

Pier offers economy functions through **LegacyMoney**. Without it these APIs return "unavailable" errors, and Pier itself keeps running.

!!! danger "Identify accounts by XUID"

    After a player renames, someone else can register the old name; an account kept by name would pass its balance to that newcomer. Every economy operation uses the XUID.

### Balances

#### Reading a balance

Rust: `money::balance(xuid)`  
Go: `levilamina.Money(xuid)`  
Zig: `levilamina.slot("get_money")`  

- Parameters:
    - xuid : string  
      the player's XUID
- Return value: the balance
- Return type: Rust `Result<i64>`, Go `(int64, error)`, Zig `i64`
    - When it cannot be read, an empty XUID or no backend, the call returns an error, and code that adds to the balance stops there instead of starting from 0. Calling the slot from Zig, a negative value means unreadable.
- Slot: `get_money`
- Example:
    - Rust

      ```rust title="Rust"
      let balance = money::balance(xuid)?;
      ```

    - Go

      ```go title="Go"
      balance, err := levilamina.Money(xuid)
      ```

    - Zig

      ```zig title="Zig"
      const get_money = levilamina.slot("get_money") orelse return error.NotProvided;
      const balance = get_money(levilamina.str(xuid));
      if (balance < 0) return error.NoAnswer;
      ```

#### Adding to a balance

Rust: `money::add(xuid, delta)`  
Go: `levilamina.AddMoney(xuid, delta)`  
Zig: `levilamina.slot("add_money")`  

Taking and setting: Rust `reduce`, `set`, Go `ReduceMoney`, `SetMoney`. Taking more than the balance fails.

- Parameters:
    - xuid : string  
      the player's XUID
    - delta : integer  
      the amount to add
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `error`, Zig `bool`
- Slot: `add_money`, `reduce_money`, `set_money`
- Example:
    - Rust

      ```rust title="Rust"
      money::add(xuid, 100)?;
      ```

    - Go

      ```go title="Go"
      err := levilamina.AddMoney(xuid, 100)
      ```

    - Zig

      ```zig title="Zig"
      const add = levilamina.slot("add_money") orelse return error.NotProvided;
      if (!add(levilamina.str(xuid), 100)) return error.Refused;
      ```

#### Transferring

Rust: `money::transfer(from, to, value, note)`  
Go: `levilamina.TransferMoney(from, to, value, note)`  
Zig: `levilamina.slot("trans_money")`  

- Parameters:
    - from, to : string  
      the payer's and recipient's XUID
    - value : integer  
      the amount
    - note : string  
      a note for the history
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `error`, Zig `bool`
    - What the recipient receives may be taxed, depending on LegacyMoney's settings.
- Slot: `trans_money`
- Example:
    - Rust

      ```rust title="Rust"
      money::transfer(from_xuid, to_xuid, 50, "bought a diamond sword")?;
      ```

    - Go

      ```go title="Go"
      err := levilamina.TransferMoney(fromXuid, toXuid, 50, "bought a diamond sword")
      ```

### Transaction listeners

#### Before a transaction

Rust: `money::on_before_with(handler)`  
Go: `levilamina.OnMoneyBefore(handler)`  
Zig: `levilamina.slot("money_listen_before_event")`  

- Parameters:
    - handler : function  
      called before every transaction; returning `false` stops it
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `error`
    - The kind of transaction, set, add, reduce or transfer, is in the event's kind; refuse a kind you do not know. To record after: Rust `on_after`, Go `OnMoneyAfter`.
- Slot: `money_listen_before_event`, `money_listen_after_event`
- Example:
    - Rust

      ```rust title="Rust"
      money::on_before_with(|ev| ev.value <= 10_000)?;
      ```

    - Go

      ```go title="Go"
      err := levilamina.OnMoneyBefore(func(ev levilamina.MoneyEvent) bool {
      	return ev.Value <= 10_000
      })
      ```
