# Rust 绑定

Rust 是 Pier 的第一个官方绑定。这一节是写模组用的，不假设你了解底下的 ABI。

想知道 Pier 是什么、为什么长这样，看 [Pier 那一节](/zh/guide/what-is-pier)。
想绑定另一门语言，看 [加一门语言](/zh/guide/adding-a-language)。

## 两个 crate

| | |
|---|---|
| `pier-rs`，暴露出来的 crate 名是 **`levilamina`** | 你写代码面对的那个。安全，也是你唯一需要写出名字的。 |
| `pier-sys-rs`，暴露为 **`levilamina_sys`** | 头文件的裸 FFI 镜像，每个调用都是 `unsafe`。 |

你依赖 `pier-rs`，代码里写 `use levilamina::`。包名说明它属于哪条 ABI，
crate 名贴着你实际在写的东西——LeviLamina 的模组代码。

```toml
[lib]
crate-type = ["cdylib"]

[dependencies]
pier-rs = { git = "https://github.com/Maskviva/pier", tag = "26.20.1" }
```

## 最小的模组

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

`on_enable`、`on_disable`、`on_unload` 都是可选的，默认什么都不做。
`register_mod!` 生成宿主要找的那个入口符号，没有它模组会在装载时被拒绝。

## 这一层加了什么

`levilamina` 建在 `levilamina_sys` 之上，只做四件事，一件不多：

- **两道槽位闸。** 每个非核心调用都先查宿主的表覆盖得到这个槽、且槽非空，
  失败时翻成一条指明是哪一道的错误。
- **字符串收口。** 跨边界来的东西活不过它所在的那次调用，所以你拿到的已经是拷贝。
- **panic 围栏。** panic 穿回 C++ 是未定义行为，所以每个回调都被包住。
  各条路径分别往哪个方向降级，见 [错误与日志](./errors#panic)。
- **sink 内拷贝。** 宿主的指针在 sink 返回时就失效了。

它不缓存宿主状态，不假装同步，不替宿主兜底。失败就是一个说得清原因的 `Err`，
不会悄悄变成默认值。

## 该看哪里

- [第一个模组](./first-mod)：构建并装上服务器
- [模组生命周期](./lifecycle)：四个回调，各自该放什么
- [事件](./events) 与 [命令](./commands)：几乎每个模组都要做的两件事
- [错误与日志](./errors)：这个绑定赖以成立的那条纪律
- [线程](./threads)：你在哪条线程上，怎么回到服务器线程
- [API 地图](./api)：全景，细节在 rustdoc 里

[pier-mod-template](https://github.com/Maskviva/pier-rs-mod-template)
是一个能跑的起手模板，不是空壳。
