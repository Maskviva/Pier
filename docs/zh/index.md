---
hide:
  - navigation
  - toc
---

<div class="pier-hero" markdown>
<div markdown>

# 用你喜欢的语言写 <span class="pier-accent">LeviLamina</span> 模组

<p class="pier-tagline">Rust、Go、Zig、C++ 都可以。装上 Pier，写一个动态库，放进 plugins/，服务器就会加载它；不同语言写的模组之间还能互相调用。</p>

[:material-rocket-launch: 快速开始](guide/installation.md){ .md-button .md-button--primary }
[:material-book-open-variant: 接口文档](api/index.md){ .md-button }
[:material-pencil: 写第一个模组](rust/first-mod.md){ .md-button }

</div>
<div markdown>

```rust title="src/lib.rs"
use levilamina::prelude::*;
use levilamina::event::{self, names};

struct Hello {
    join: Option<Listener>,
}

impl LeviMod for Hello {
    fn on_load(_ctx: &ModContext) -> Result<Self> {
        Ok(Hello { join: None })
    }

    fn on_enable(&mut self, _ctx: &ModContext) -> Result<()> {
        self.join = Some(event::subscribe(names::PLAYER_JOIN, |ev| {
            if let Ok(name) = ev.str_at("_player.name") {
                Logger::get().info(&format!("{name} 进服了"));
            }
        })?);
        Ok(())
    }
}

levilamina::register_mod!(Hello);
```

</div>
</div>

<div class="grid cards" markdown>

-   :material-translate:{ .lg .middle } **用你会的语言**

    ---

    Rust、C++、Go、Zig 都有官方绑定。不在名单上的语言，只要能调用 C 函数，也能自己接上。

    [:octicons-arrow-right-24: Pier 是什么](guide/what-is-pier.md)

-   :material-transit-connection-variant:{ .lg .middle } **模组之间能对话**

    ---

    Go 模组调用 Rust 模组提供的服务时，请求和回复由 Pier 转交，两边都不用知道对方是什么语言写的。

    [:octicons-arrow-right-24: 跨模组通信](tasks/crossmod.md)

-   :material-shield-check:{ .lg .middle } **读不出来时会报错**

    ---

    宿主读不出某个值时，调用返回一个错误，里面写着读的是哪一项。你的代码可以据此拒绝这次操作。

    [:octicons-arrow-right-24: 接口文档](api/index.md)

-   :material-update:{ .lg .middle } **升级不用重新编译**

    ---

    只要 ABI 版本没变，按旧版 Pier 编译的模组照常加载；真不兼容时，会在加载时写明原因。

    [:octicons-arrow-right-24: 兼容性](guide/compatibility.md)

-   :material-dog-side:{ .lg .middle } **卡住时看门狗会出手**

    ---

    某个模组占住服务器线程太久，看门狗先在日志里写下是哪个模组、卡在哪里，再结束进程，让守护程序把服务器拉起来。

    [:octicons-arrow-right-24: 看门狗](guide/configuration.md#watchdog)

-   :material-puzzle:{ .lg .middle } **和 LeviLamina 无缝衔接**

    ---

    LeviLamina 的事件、命令、玩家、世界都能在模组里直接用；原生插件也能通过 bridge 调用你的服务。

    [:octicons-arrow-right-24: bridge](guide/bridge.md)

</div>
