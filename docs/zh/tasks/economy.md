# 💰 经济 API

Pier 通过 **LegacyMoney** 提供经济功能。服务器没装 LegacyMoney 时，这些接口返回「不支持」的错误，Pier 本身照常运行。

!!! danger "账户请按 XUID 认人"

    玩家改名以后，旧名字可能被另一个人注册；账户如果按名字记，新来的这个人就会接手前一个人的余额。所以所有经济操作都用 XUID。

### 余额

#### 查询余额

Rust：`money::balance(xuid)`  
Go：`levilamina.Money(xuid)`  
Zig：`levilamina.slot("get_money")`  

- 参数：
    - xuid : 字符串  
      玩家的 XUID
- 返回值：余额
- 返回值类型：Rust `Result<i64>`，Go `(int64, error)`，Zig `i64`
    - 读不到的时候（XUID 为空、没有经济后端）返回错误，拿余额做加减的代码会在这一步停下，不会从 0 开始算。Zig 直接调用槽位时，负数表示读不到。
- 对应槽位：`get_money`
- 示例：
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

#### 增加余额

Rust：`money::add(xuid, delta)`  
Go：`levilamina.AddMoney(xuid, delta)`  
Zig：`levilamina.slot("add_money")`  

扣钱、直接设置：Rust `reduce`、`set`，Go `ReduceMoney`、`SetMoney`。余额不够时扣钱会失败。

- 参数：
    - xuid : 字符串  
      玩家的 XUID
    - delta : 整数  
      增加的数额
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `error`，Zig `bool`
- 对应槽位：`add_money`, `reduce_money`, `set_money`
- 示例：
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

#### 转账

Rust：`money::transfer(from, to, value, note)`  
Go：`levilamina.TransferMoney(from, to, value, note)`  
Zig：`levilamina.slot("trans_money")`  

- 参数：
    - from, to : 字符串  
      付款方和收款方的 XUID
    - value : 整数  
      金额
    - note : 字符串  
      备注，记进交易历史
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `error`，Zig `bool`
    - 收款方实际收到的金额可能扣了税，取决于 LegacyMoney 的配置。
- 对应槽位：`trans_money`
- 示例：
    - Rust

      ```rust title="Rust"
      money::transfer(from_xuid, to_xuid, 50, "买了一把钻石剑")?;
      ```

    - Go

      ```go title="Go"
      err := levilamina.TransferMoney(fromXuid, toXuid, 50, "买了一把钻石剑")
      ```

### 交易监听

#### 在交易发生前拦截

Rust：`money::on_before_with(handler)`  
Go：`levilamina.OnMoneyBefore(handler)`  
Zig：`levilamina.slot("money_listen_before_event")`  

- 参数：
    - handler : 函数  
      每笔交易发生前调用，返回 `false` 就拦下这笔交易
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `error`
    - 交易的种类（设置、加、减、转账）在事件的 kind 里。遇到不认识的种类，请拒绝。交易发生后再记录：Rust `on_after`，Go `OnMoneyAfter`。
- 对应槽位：`money_listen_before_event`, `money_listen_after_event`
- 示例：
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
