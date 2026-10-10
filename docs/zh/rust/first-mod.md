# ✍️ 写你的第一个 Rust 模组

这篇教程带你从零写一个 Rust 模组：先让它在服务器上打出第一行日志，再让它在玩家进服时打个招呼。
你需要会一点 Rust，但不用很熟。

## 准备工作

- **Rust 工具链**：装好 [rustup](https://rustup.rs) 就行，Windows 上默认的 MSVC 目标正合适。
- **一台装好 Pier 的服务器**，用来测试。还没装的话，先看 [安装](../guide/installation.md)。

## 第一步：建一个项目

最快的办法是用模板。在它的 [GitHub 页面](https://github.com/Maskviva/pier-mod-template) 上点 *Use this template*，
就能得到一个属于你自己的仓库；也可以直接在本地生成一份：

```bash
cargo generate --git https://github.com/Maskviva/pier-mod-template
```

模板生成的是一个能直接跑的模组：它会打日志、订阅聊天并拦下一条消息、注册一条命令、排一个延迟任务。
你不需要的部分删掉就行。

想自己从零搭，就新建一个库项目，把 `Cargo.toml` 写成这样：

```toml
[package]
name = "my-mod"
version = "0.1.0"
edition = "2021"

[lib]
crate-type = ["cdylib"]

[dependencies]
pier-rs = { git = "https://github.com/Maskviva/pier", tag = "26.51.2" }
```

`crate-type = ["cdylib"]` 让 cargo 编出一个 DLL，服务器加载的就是它。

依赖的包名是 `pier-rs`，但代码里写的是 `use levilamina::...`：这个包对外暴露的 crate 名叫 `levilamina`。

## 第二步：写下模组的入口

把 `src/lib.rs` 改成这样：

```rust
use levilamina::prelude::*;

struct MyMod;

impl LeviMod for MyMod {
    fn on_load(ctx: &ModContext) -> Result<Self> {
        ctx.logger().info("hello from Rust");
        Ok(MyMod)
    }
}

levilamina::register_mod!(MyMod);
```

服务器加载模组时，会调用 `on_load`，你的模组就在日志里打出 `hello from Rust`。

最后一行的 `register_mod!` 会生成服务器要找的入口函数。少了这一行，模组在加载时会被拒绝。

## 第三步：写 manifest

在项目根目录建一个 `manifest.json`：

```json
{
  "name": "my-mod",
  "entry": "my_mod.dll",
  "type": "pier",
  "version": "0.1.0",
  "dependencies": [{ "name": "pier" }]
}
```

有三处要对得上：

- `name` 要和服务器 `plugins/` 下放模组的文件夹同名；
- `entry` 要和 cargo 编出来的文件同名。cargo 会把包名里的连字符换成下划线，所以 `my-mod` 编出来是 `my_mod.dll`；
- `type` 必须是 `pier`，Pier 才会接管它。

## 第四步：编译、安装、看效果

```bash
cargo build --release
```

编译好以后，在服务器的 `plugins/` 下建一个 `my-mod` 文件夹，把 `target/release/my_mod.dll` 和 `manifest.json` 放进去：

```
plugins/
  Pier/
  my-mod/
    my_mod.dll
    manifest.json
```

启动服务器，在日志里找 `hello from Rust`。看到它，你的第一个模组就跑起来了。在控制台输入 `/pier list`，也能看到 `my-mod`。

## 第五步：玩家进服时打个招呼

现在让模组做点事：有玩家进服，就在日志里写下他的名字。

事件订阅要放在**启用**阶段，并且把返回的监听器存起来：监听器被丢弃时，订阅就自动取消了。

```rust
use levilamina::prelude::*;
use levilamina::event::{self, names};

struct MyMod {
    join: Option<Listener>,
}

impl LeviMod for MyMod {
    fn on_load(ctx: &ModContext) -> Result<Self> {
        ctx.logger().info("hello from Rust");
        Ok(MyMod { join: None })
    }

    fn on_enable(&mut self, _ctx: &ModContext) -> Result<()> {
        self.join = Some(event::subscribe(names::PLAYER_JOIN, |ev| {
            if let Ok(name) = ev.str_at("_player.name") {
                Logger::get().info(&format!("{name} 进服了"));
            }
        })?);
        Ok(())
    }

    fn on_disable(&mut self, _ctx: &ModContext) -> Result<()> {
        self.join = None; // 监听器在这里被丢弃，订阅随之取消
        Ok(())
    }
}

levilamina::register_mod!(MyMod);
```

重新编译、替换 DLL、重启服务器，然后进服试试，日志里会出现你的名字。

`_player.name` 是 Pier 补充进事件内容的字段，每个事件有哪些字段，见 [事件载荷参考](../guide/event-payloads.md)。

## 什么都没发生的时候

按这个顺序一项项查，每查一项就排除一种可能：

1. **`/pier list` 里没有你的模组**：先看 `manifest.json` 里是不是 `"type": "pier"`，再看 `name` 和文件夹名是不是一样。
   type 写错时，模组不会被扫到，日志里也不会有任何提示。
2. **日志里有一行拒绝加载的原因**：照着读。Pier 会说明是哪一项检查没通过：函数表长度、ABI 版本，还是目标不匹配。
   目标不匹配，说明把客户端模组装到了服务端，或者反过来。
3. **加载了，但订阅一次都没触发**：多半是事件名写错了。在控制台运行 `/pier events` 看看有哪些事件名，并改用 `names::` 下的常量，拼错时编译器会报出来。
4. **某个调用返回「宿主不提供」**：说明这台服务器上的 Pier 没有编进这项功能。错误信息里写着槽位名；
   `ctx.host_abi()` 能拿到 ABI 版本和函数表长度，报告问题时请一起带上。

## 接下来

- [🔄 生命周期与日志](../tasks/lifecycle.md)：四个阶段各在什么时候发生、适合做什么
- [📣 事件监听](../tasks/events.md)：读取事件内容、取消事件
- [⌨️ 命令](../tasks/commands.md)：给你的模组加一条命令
