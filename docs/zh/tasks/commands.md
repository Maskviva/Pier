# ⌨️ 命令 API

想让玩家或服主在聊天框里输入 `/hello` 就能用上你的功能？注册一个命令。

### 注册命令

#### 注册一个命令

Rust：`command::register(name, description, permission, handler)`  
Go：`levilamina.RegisterCommand(name, description, permission, handler)`  
Zig：`levilamina.registerCommand(name, description, permission, handler)`  

- 参数：
    - name : 字符串  
      命令名，不带斜杠
    - description : 字符串  
      在游戏的命令提示里显示
    - permission : 权限  
      谁能用：任何人、游戏管理员、管理员、主机、服主，从低到高
    - handler : 函数  
      有人执行命令时调用，运行在服务器主线程。命令后面的文字：Rust `inv.raw()`，Go `inv.Args`，Zig `inv.args`
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `error`，Zig `levilamina.Error!void`
    - 请在启用阶段注册。Bedrock 不支持注销命令，命令会一直存在到关服；模组被禁用时，Pier 会让它的命令暂时静音。
- 对应槽位：`register_command`
- 示例：
    - Rust

      ```rust title="Rust"
      use levilamina::command::{self, CommandPermission};

      command::register("hello", "打个招呼", CommandPermission::Any, |inv| {
          inv.success(&format!("你好，{}!", inv.origin().name));
      })?;
      ```

    - Go

      ```go title="Go"
      err := levilamina.RegisterCommand("hello", "打个招呼", levilamina.PermissionAny, func(inv *levilamina.Invocation) {
      	inv.Success("你好，" + inv.Origin + "!")
      })
      ```

    - Zig

      ```zig title="Zig"
      try levilamina.registerCommand("hello", "打个招呼", .any, onHello);

      fn onHello(inv: *const levilamina.Invocation) void {
          inv.success("你好！");
      }
      ```

#### 注册带参数的命令

Rust：`command::builder(name, description, permission).overload(..).register(handler)`  
Go：`levilamina.NewCommand(name, description, permission).Overload(..).Register(handler)`  
Zig：`levilamina.slot("register_command_ex")`  

让游戏帮你解析参数，聊天框里还能自动补全。每个重载是一种参数组合。

- 参数：
    - overload : 重载  
      依次加上必填参数、可选参数、字面词、枚举参数
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `error`
- 对应槽位：`register_command_ex`
- 示例：
    - Rust

      ```rust title="Rust"
      command::builder("plot", "地皮管理", CommandPermission::Any)
          .overload(|o| o.required("action", ParamType::String).optional("target", ParamType::Player))
          .register(|inv| {
              match inv.arg_str("action").unwrap_or("") {
                  "claim" => inv.success("已认领"),
                  _ => inv.error("不认识的操作"),
              }
          })?;
      ```

    - Go

      ```go title="Go"
      err := levilamina.NewCommand("plot", "地皮管理", levilamina.PermissionAny).
      	Overload(levilamina.NewOverload().Required("action", levilamina.ParamString).Optional("target", levilamina.ParamPlayer)).
      	Register(func(call *levilamina.CommandCall) {
      		if action, _ := call.ArgString("action"); action == "claim" {
      			call.Success("已认领")
      			return
      		}
      		call.Error("不认识的操作")
      	})
      ```

!!! warning "两个重载不能接受同样的输入"

    两个重载都能匹配玩家输入的同一串文字时，游戏会随便挑一个。意思不同但参数相同的几个单词（比如 `menu`、`help`、`status`），请合并成**一个枚举参数**。

#### 注册一个命令枚举

Rust：`command::register_enum(name, values)`  
Go：`levilamina.RegisterCommandEnum(name, values)`  
Zig：`levilamina.slot("register_command_enum")`  

- 参数：
    - name : 字符串  
      枚举名，重载里用它引用这个枚举
    - values : 名字和数值的列表  
      枚举的每一个值
- 返回值：成功时没有返回值
- 返回值类型：Rust `Result<()>`，Go `error`
- 对应槽位：`register_command_enum`
- 示例：
    - Rust

      ```rust title="Rust"
      command::register_enum("plot_simple", &[("menu", 0), ("help", 1), ("status", 2)])?;
      ```

    - Go

      ```go title="Go"
      err := levilamina.RegisterCommandEnum("plot_simple", []levilamina.EnumValue{{Name: "menu", Value: 0}, {Name: "help", Value: 1}, {Name: "status", Value: 2}})
      ```

### 执行命令

#### 以控制台身份执行一条命令

Rust：`ctx.host().execute_command(cmd)`  
Go：`levilamina.ExecuteCommand(cmd)`  
Zig：`levilamina.executeCommand(cmd)`  

- 参数：
    - cmd : 字符串  
      要执行的命令，不带斜杠
- 返回值：命令的输出。Zig 把输出以 debug 级别写进日志
- 返回值类型：Rust `Result<String>`，Go `([]CommandOutput, error)`，Zig `levilamina.Error!void`
    - 只能在服务器主线程调用；世界还没加载好时返回错误。
- 对应槽位：`execute_command`
- 示例：
    - Rust

      ```rust title="Rust"
      let output = ctx.host().execute_command("time set day")?;
      ```

    - Go

      ```go title="Go"
      lines, err := levilamina.ExecuteCommand("time set day")
      ```

    - Zig

      ```zig title="Zig"
      try levilamina.executeCommand("time set day");
      ```
