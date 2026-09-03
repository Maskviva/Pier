# 第一个模组

这一节是 Rust 绑定。服务器上得先有 Pier，那部分见 [安装](/zh/guide/installation)。

## 从模板开始

```bash
cargo generate --git https://github.com/Maskviva/pier-rs-mod-template
```

模板是一个能跑的模组，不是空壳：它打日志、订阅聊天并拦下一条消息、注册一条命令、
排一个延迟任务。不需要的删掉就行。

## 或者从零开始

```toml
# Cargo.toml
[package]
name = "my-mod"
version = "0.1.0"
edition = "2021"

[lib]
crate-type = ["cdylib"]

[dependencies]
pier-rs = { git = "https://github.com/Maskviva/pier", tag = "26.20.1" }
```

包名是 `pier-rs`，它暴露出来的 crate 是 `levilamina`，所以 import 写的是
`use levilamina::`。包名说明属于哪条 ABI，crate 名贴着你实际在写的东西。

```rust
// src/lib.rs
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

`register_mod!` 生成宿主要找的入口符号。没有它，模组在装载时被拒绝。

## manifest

```json
{
  "name": "my-mod",
  "entry": "my_mod.dll",
  "type": "pier",
  "version": "0.1.0",
  "dependencies": [{ "name": "pier" }]
}
```

三处必须互相对得上：

- `name` 和 `mods/` 下的目录名一致。
- `entry` 和 cargo 的产出一致。cargo 会把连字符变成下划线，所以 crate `my-mod`
  产出的是 `my_mod.dll`。
- `type` 正好是 `pier`。

## 构建与安装

```bash
cargo build --release
```

把 `target/release/my_mod.dll` 和 `manifest.json` 一起放进 `<服务器>/mods/my-mod/`，
然后启动服务器。

## 什么都没发生的时候

按这个顺序排查，每一条排除一种可能。

**模组没出现在 `/pier list` 里。** 多半是 manifest：检查 `"type": "pier"`，
再检查 `name` 和目录名一致。type 写错意味着模组根本不会被扫到，而且什么都不报。

**服务器拒绝装载并说明了原因。** 读那行话。宿主会说三道握手里失败的是哪一道：
表长度、ABI 版本区间，还是目标标志。目标不匹配就是把客户端模组装到了服务端构建上，
或者反过来。

**装上了，但订阅一次都不触发。** 事件 id 几乎肯定写错了。跑 `/pier events`
看这个构建能解析哪些 id，并且改用 `names` 里的常量，这样拼错在编译期就报。

**某个调用返回「宿主不提供」。** 那个能力包没有编进这个构建。错误信息里有槽位名；
`ctx.host_abi()` 给出版本号和表长度，报问题时带上。

## 接下来

- [模组生命周期](./lifecycle)：四个回调，各自能做什么
- [事件](./events)：订阅、读载荷、取消
- [错误与日志](./errors)：这个绑定赖以成立的那条纪律
