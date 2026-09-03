# 命令

## 原始文本

```rust
use levilamina::command::{self, CommandPermission};

command::register("hello", "Say hello", CommandPermission::Any, |inv| {
    let who = if inv.raw().is_empty() {
        inv.origin().name
    } else {
        inv.raw().to_owned()
    };
    inv.success(&format!("Hello, {who}!"));
})?;
```

`inv.raw()` 是 `/hello` 之后的全部内容，由你自己解析。

## 类型化 overload

把参数声明出来，引擎就会替你解析和补全，玩家在客户端能看到参数提示：

```rust
use levilamina::command::{self, CommandPermission, ParamType};

command::builder("plot", "Plot management", CommandPermission::Any)
    .overload(|o| {
        o.required("action", ParamType::String)
            .optional("target", ParamType::Player)
    })
    .register(|inv| {
        match inv.arg_str("action").unwrap_or("") {
            "claim" => inv.success("claimed"),
            "info" => inv.success("..."),
            _ => inv.error("unrecognized subcommand"),
        }
    })?;
```

`arg_str`、`arg_i64`、`arg_f64`、`arg_bool` 按名字取参数，`arg` 给出原始的 `NbtValue`。
没填的可选参数读出来是 `None`。

至少要有一个 overload。一个都没有的 builder 会被拒绝，并提示改用 `command::register`，
而不是丢下一个光秃秃的注册失败让人猜。

## 成功和失败是两条通道

```rust
inv.success("done");
inv.error("that plot is not yours");
```

客户端给它们不同的颜色，命令方块也读得出区别。**失败的命令不能走成功通道。**

## 注册是单向的

基岩版无法移除命令。你注册的命令活到服务器停止；模组被停用期间宿主把回调静音，
重新启用就恢复。

两个后果：

- 没有 `unregister`，也没有 drop 即注销的句柄。
- 在 `on_enable` 里注册必须能承受跑第二遍。同名重注册只是换回调，这部分本来就安全。

**改了形状**的重注册会被拒绝：基岩版的命令一旦建好就改不了。报错会说重启服务器
以采用新声明。

## origin 是名字，不是身份

```rust
let origin = inv.origin();
origin.name      // 玩家名，或者控制台
origin.kind      // CommandOriginType：0 是玩家，7 是专用服务器控制台
origin.at        // Option，控制台没有位置
```

权限判定用 `kind` 加上你自己的玩家表。名字会走显示名回退，所以它不是身份。
为什么这很要紧，见 [错误与日志](./errors#身份)。
