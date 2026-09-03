# 事件

```rust
use levilamina::prelude::*;
use levilamina::event;

let listener = event::subscribe(names::PLAYER_CHAT, |ev| {
    let Ok(message) = ev.str_at("message") else { return };
    Logger::get().info(&format!("[chat] {message}"));
})?;
```

接住返回的 `Listener`；丢掉它就退订。

## 用常量，不要写字面量

`event::subscribe` 收的是字符串，而**拼错的 id 会订阅成功，然后一次都不触发**。
`names` 模块把这个错挪到编译期：

```rust
event::subscribe(names::PLAYER_CHAT, ...)     // 好
event::subscribe("PlayerChatEvent", ...)      // 能用，直到你哪天打错一个字母
```

每个常量都注明了这个事件能不能取消、载荷里有什么。`/pier events` 列出某个构建能解析的全部 id。

## 两类事件

**注册表事件**来自 LeviLamina，写全名 `ll::event::PlayerChatEvent`。唯一后缀也能解析，
但上游哪天加一个同名事件，后缀就有歧义了。

**合成事件**是 Pier 用原生 detour 造的，因为 LeviLamina 没有对应物。它们是裸名字，
比如 `BlockDestroyEvent`。一共 29 个，补的正是保护场景里要紧的那些缺口：
非玩家造成的方块破坏、爆炸、水流跨界、活塞伸进邻居地皮、跨界大箱子合并。

`names::ALL_SYNTHETIC` 是完整清单，方便做启动自检。

## 读载荷

载荷是 SNBT。类型化取值把「缺键」和「类型不符」分开，也把这两者和「值恰好是 0」分开：

```rust
let dim = ev.dim()?;                    // 宿主解析不出来时是 Err
let pos = ev.pos()?;                    // (x, y, z)
let who = ev.player();                  // Option<PlayerIdentity>
let n   = ev.i32_at("count")?;          // Err 里带键名和实际类型
let s   = ev.opt_str("reason");         // Option，用于「没有也没关系」
```

`ev.dim()` 会返回错误，这正是重点。一个把解析不到的维度读成 `0` 的土地保护模组，
会在主世界里拒绝、在别的所有地方放行，而且悄无声息。请处理那个错误——
**不知道的时候就拒绝**。

宿主在解析不了载荷的一部分时会注入 `_unresolved`。`ev.unresolved()` 返回那些字段名，
非空就说明载荷不完整。保护判定在这种时候应该拒绝，而不是猜。

## 取消

```rust
if should_block {
    ev.cancel()?;
}
```

`cancel()` 返回 `Result`，因为不是每个事件都能取消。不能取消的会说清楚，
并指出该拦哪一个：

```
event `PlayerStartDestroyBlockEvent` cannot be cancelled: emitted before origin only to
record who started mining which cell; use PlayerDestroyBlockEvent or BlockDestroyEvent
to block it
```

这里返回 `()` 会让保护模组以为自己拦住了。那种「以为」比崩溃更糟，因为崩溃看得见。

`Ok` 只表示取消位送达了宿主，不表示引擎停了。有几个钩点处在半更新状态，
宿主在那里根本不接受取消；适用的事件常量里写明了。

## 改载荷

```rust
ev.edit(|v| {
    v.insert("message", NbtValue::from("edited"));
});
```

只改一个字段时用 `ev.set("message", value)` 更省事。

写回是**差量**的：只有你真正碰过的键才写回去，所以两个模组挂同一个事件不会互相抹掉对方的修改。

## 一次订一批

`Wiring` 把一批订阅攒起来，句柄也一起管：

```rust
let wiring = Wiring::new("my-mod")
    .on("chat", names::PLAYER_CHAT, |ev| { ... })
    .on("break", names::PLAYER_DESTROY_BLOCK, |ev| { ... })
    .at("explode", names::EXPLOSION, Priority::High, |ev| { ... })
    .arm()?;
```

`arm()` 是整体成败：任何一条订阅失败，它会把已经挂上的那些退订掉再返回错误。
**挂了一半的保护比没有更糟**——有些点拦得住有些拦不住，而且没人知道是哪些。
另一个选择是 `arm_lenient()`，适合可有可无的功能，失败的记录在 `failures()` 里。
