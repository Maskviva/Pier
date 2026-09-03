# 模组生命周期

模组实现 `LeviMod`。只有 `on_load` 是必须的。

```rust
impl LeviMod for MyMod {
    fn on_load(ctx: &ModContext) -> Result<Self> { ... }
    fn on_enable(&mut self, ctx: &ModContext) -> Result<()> { Ok(()) }
    fn on_disable(&mut self, ctx: &ModContext) -> Result<()> { Ok(()) }
    fn on_unload(&mut self, ctx: &ModContext) -> Result<()> { Ok(()) }
}
```

## 各自该放什么

| 回调 | 什么时候 | 该放什么 |
|---|---|---|
| `on_load` | 模组正在被拉起来 | 建你的状态。返回 `Err` 会让装载失败，宿主随即回滚。 |
| `on_enable` | 所有依赖都已装载 | 订阅事件、注册命令、开工。**这个可能跑不止一次。** |
| `on_disable` | 模组正在被停用 | 丢掉订阅，取消排期任务。 |
| `on_unload` | 模组要走了 | 剩下的收尾。 |

## enable 会跑不止一次

一次 reload 是停用再启用，不卸载，所以 `on_enable` 必须能安全地跑第二遍。
同名命令重复注册只是换掉回调，那部分本来就幂等；你自己的东西如果不幂等，得自己加闸。

## 订阅丢掉就退订

`Listener` 在 drop 时退订，所以把它放进结构体，就把订阅的寿命绑到了你的模组上：

```rust
struct MyMod {
    chat: Option<Listener>,
}

fn on_enable(&mut self, _ctx: &ModContext) -> Result<()> {
    self.chat = Some(event::subscribe(names::PLAYER_CHAT, |ev| { ... })?);
    Ok(())
}

fn on_disable(&mut self, _ctx: &ModContext) -> Result<()> {
    self.chat = None;   // 在这里退订
    Ok(())
}
```

订阅完随手把返回值丢掉，会立刻退订——而这在代码上和「忘了接住返回值」长得一模一样。
如果你真的想让订阅活到卸载，请写 `.forget()` 明说，而不是让它自然出作用域。

`service::Registration`、`bus::Subscription`、`PacketHook` 同理。

## 命令是单向的

基岩版没有移除命令的通道，所以你注册的命令会活到服务器停止。模组被停用期间，
宿主把回调静音而不是移除它，重新启用就恢复。

两个后果：

- 没有 `unregister`，也没有 drop 即注销的句柄。给一个句柄会让人以为它能撤销。
- 在 `on_enable` 里注册必须能承受跑第二遍。同名重注册只是换回调，这部分本来就安全。

**改了形状**的重注册会被拒绝：基岩版的命令一旦建好就改不了。报错会说重启服务器
以采用新声明。

## 自己排的任务自己取消

排期任务记在你的模组名下，卸载时宿主会把剩下的丢掉并打一条告警。
请在 `on_disable` 里取消自己的定时器：

```rust
let id = ctx.host().schedule_after(Duration::from_secs(30), || { ... })?;
// 之后
ctx.host().cancel(id);
```

想确认干净了，就在 `on_unload` 里断言 `ctx.host().pending_tasks()`。
