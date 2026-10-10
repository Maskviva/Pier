# 📣 事件监听 API

服务器上发生的事情，比如玩家说话、方块被破坏、生物死亡，Pier 都会以「事件」的形式通知你。你订阅一个事件，事情发生时，你的函数就会被调用。

### 订阅事件

#### 订阅一个事件

Rust：`event::subscribe(id, handler)`  
Go：`levilamina.Subscribe(id, priority, handler)`  
Zig：`levilamina.subscribe(id, priority, handler)`  

- 参数：
    - id : 字符串  
      事件名。LeviLamina 的事件写完整名字，比如 `ll::event::PlayerChatEvent`；Pier 自己的事件直接写名字，比如 `PlayerAttackTargetEvent`。Rust 里用 `names::` 下的常量，拼错时编译器会报出来。
    - priority : 优先级  
      （Go、Zig）谁先收到事件：最高、高、普通、低、最低。Rust 用 `event::subscribe_with` 指定
    - handler : 函数  
      事件发生时被调用，运行在服务器主线程上
- 返回值：一个监听器，留着它，以后用来退订
- 返回值类型：Rust `Result<Listener>`，Go `(*Listener, error)`，Zig `levilamina.Error!Listener`
- 对应槽位：`subscribe_event`
- 示例：
    - Rust

      ```rust title="Rust"
      use levilamina::event::{self, names};

      let listener = event::subscribe(names::PLAYER_CHAT, |ev| {
          if let Ok(msg) = ev.str_at("message") {
              Logger::get().info(&format!("有人说：{msg}"));
          }
      })?;
      ```

    - Go

      ```go title="Go"
      listener, err := levilamina.Subscribe("ll::event::PlayerChatEvent", levilamina.PriorityNormal, func(ev *levilamina.Event) {
      	payload, err := ev.Payload()
      	if err != nil {
      		return
      	}
      	if msg, ok := payload.OptString("message"); ok {
      		levilamina.Logger{}.Info("有人说：" + msg)
      	}
      })
      ```

    - Zig

      ```zig title="Zig"
      const listener = try levilamina.subscribe("ll::event::PlayerChatEvent", .normal, onChat);

      fn onChat(ev: *const levilamina.Event) void {
          levilamina.logf(.info, "收到聊天事件：{s}", .{ev.snbt});
      }
      ```

!!! tip "订阅了却没有反应？"

    在服务器控制台运行 `/pier events`，看看当前服务器能订阅的事件名里有没有你写的那个。

#### 退订一个事件

Rust：`drop(listener)`  
Go：`listener.Unsubscribe()`  
Zig：`listener.unsubscribe()`  

Rust 里监听器被丢弃时就会自动退订；Go、Zig 调用退订函数。重复退订是安全的。

- 返回值：成功时没有返回值
- 返回值类型：Go `error`，Zig `levilamina.Error!void`
- 对应槽位：`unsubscribe_event`
- 示例：
    - Rust

      ```rust title="Rust"
      drop(listener);
      ```

    - Go

      ```go title="Go"
      err := listener.Unsubscribe()
      ```

    - Zig

      ```zig title="Zig"
      try listener.unsubscribe();
      ```

### 读取事件内容

事件的内容是一段 SNBT 文本。每个事件有哪些字段，见 [事件载荷参考](../guide/event-payloads.md)。

#### 读取一个字段

Rust：`ev.str_at(path)`  
Go：`ev.Payload()`  
Zig：`levilamina.nbt.parse(arena, ev.snbt)`  

- 参数：
    - path : 字符串  
      字段的路径，嵌套的字段用点号连接，比如 `_player.name`
- 返回值：字段的值。Go、Zig 先拿到解析好的完整内容，再按路径读取
- 返回值类型：Rust `Result<&str>`（另有 `bool_at`、`opt_str` 等），Go `(*Nbt, error)`，Zig `nbt.ParseError!nbt.Value`
- 示例：
    - Rust

      ```rust title="Rust"
      let name = ev.str_at("_player.name")?;
      ```

    - Go

      ```go title="Go"
      payload, err := ev.Payload()
      name, ok := payload.OptString("_player.name")
      ```

    - Zig

      ```zig title="Zig"
      const payload = try levilamina.nbt.parse(arena, ev.snbt);
      const name = payload.getString("_player.name");
      ```

#### 获取事件发生的维度

Rust：`ev.dim()`  
Go：`ev.Dim()`  
Zig：`payload.getInt("dim")`  

- 返回值：维度 id
- 返回值类型：Rust `Result<i32>`，Go `(int32, error)`，Zig `?i64`
    - 宿主没能识别事件里的某个实体时，内容里会有 `_unresolved` 字段，这时 `dim()`、`Dim()` 返回错误。基于这种内容做保护判断时，请拒绝操作。
- 示例：
    - Rust

      ```rust title="Rust"
      let dim = ev.dim()?;
      ```

    - Go

      ```go title="Go"
      dim, err := ev.Dim()
      ```

    - Zig

      ```zig title="Zig"
      const dim = payload.getInt("dim") orelse return;
      ```

!!! info "谁杀了谁"

    生物死亡、受伤这类事件，伤害来源里的 `attackerUid` 是负责的那个实体；被箭射死时，这里是**射手**。`attacker` 是它的详细信息，它还在时才有。详见 [事件载荷参考](../guide/event-payloads.md#谁杀了谁)。

### 改变事件

#### 取消一个事件

Rust：`ev.cancel()`  
Go：`ev.Cancel()`  
Zig：`ev.cancel()`  

在处理函数里取消，就能阻止这件事发生，比如不让玩家破坏某个方块。

- 返回值：Rust 在事件不能取消时返回错误并说明原因
- 返回值类型：Rust `Result<()>`，Go、Zig 无
    - 排在你前面的模组已经取消了这个事件的话，它会保持取消状态，你这边改不回来。
- 示例：
    - Rust

      ```rust title="Rust"
      event::subscribe(names::PLAYER_DESTROY_BLOCK, |ev| {
          if forbidden(ev) {
              let _ = ev.cancel();
          }
      })?;
      ```

    - Go

      ```go title="Go"
      levilamina.Subscribe("ll::event::PlayerDestroyBlockEvent", levilamina.PriorityNormal, func(ev *levilamina.Event) {
      	if forbidden(ev) {
      		ev.Cancel()
      	}
      })
      ```

    - Zig

      ```zig title="Zig"
      fn onDestroy(ev: *const levilamina.Event) void {
          if (forbidden(ev)) ev.cancel();
      }
      ```

#### 写回字段

Go：`ev.Set(key, value)`  
Zig：`ev.writeBack(snbt)`  

把修改过的字段写回事件。只写回改动的那几个键，宿主把它们合并进事件，所以两个模组改不同的键时，两边的修改都会保留。

- 参数：
    - key, value : 键和 NBT 值（Go）  
      要改的字段和新值
    - snbt : 字符串（Zig）  
      只含改动字段的 SNBT，比如 `{cancelled:1b}`
- 示例：
    - Go

      ```go title="Go"
      ev.Set("message", levilamina.NbtStringOf("（已过滤）"))
      ```

    - Zig

      ```zig title="Zig"
      ev.writeBack("{message:\"（已过滤）\"}");
      ```
