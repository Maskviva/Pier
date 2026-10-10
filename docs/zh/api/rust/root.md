# levilamina：生命周期、日志与错误

`levilamina`：用安全的 Rust 写 LeviLamina 模组。

```rust
struct Hello;
impl LeviMod for Hello {
    fn on_load(ctx: &ModContext) -> Result<Self> { Ok(Hello) }
}
levilamina::register_mod!(Hello);
```

它建立在 `levilamina_sys`（`sdk/abi.h` 的逐格镜像）之上，只做四件事：两道槽位关卡、字符串处理、panic 围栏，以及在输出回调里复制数据（契约 §3）。它不缓存宿主的状态，不假装做同步，也不替宿主掩盖任何问题：失败就是一个说明原因的 `Err`，永远不会悄悄变成默认值（§5.1）。

## `prelude` {#prelude}

```rust
pub mod prelude {
    pub use crate::{event::names, service};
    pub use crate::{
        register_mod, Block, ConnectionState, Container, Direction, Directions, Entity, Error,
        Event, EventRef, GameMode, GamingStatus, Host, ItemStack, LeviMod, Listener, LogLevel,
        Logger, ModContext, NbtValue, Packet, PacketHook, Packets, Player, PlayerIdentity,
        PlayerSel, Priority, Result, Server, TaskId, Verdict, Wiring, World,
    };
}
```

一句 `use levilamina::prelude::*;` 就能引入写模组最常用的东西。

## `Logger` {#Logger}

```rust
pub struct Logger(/* private */);
```

- 实现的 trait：`Clone`、`Copy`

### `Logger::get` {#Logger.get}

```rust
pub fn get() -> Logger
```

- 返回值类型：`Logger`

### `Logger::log` {#Logger.log}

```rust
pub fn log(&self, level: LogLevel, msg: &str)
```

- 参数：
    - level : `LogLevel`
    - msg : `&str`

### `Logger::fatal` {#Logger.fatal}

```rust
pub fn fatal(&self, msg: &str)
```

- 参数：
    - msg : `&str`

### `Logger::error` {#Logger.error}

```rust
pub fn error(&self, msg: &str)
```

- 参数：
    - msg : `&str`

### `Logger::warn` {#Logger.warn}

```rust
pub fn warn(&self, msg: &str)
```

- 参数：
    - msg : `&str`

### `Logger::info` {#Logger.info}

```rust
pub fn info(&self, msg: &str)
```

- 参数：
    - msg : `&str`

### `Logger::debug` {#Logger.debug}

```rust
pub fn debug(&self, msg: &str)
```

- 参数：
    - msg : `&str`

### `Logger::trace` {#Logger.trace}

```rust
pub fn trace(&self, msg: &str)
```

- 参数：
    - msg : `&str`

## `ModContext` {#ModContext}

```rust
pub struct ModContext(/* private */);
```

交给模组生命周期回调的上下文。

它自己不带任何状态，真正的状态在 `RUNTIME` 里。它的存在是为了给各个门面一个共同的入口，也让 `on_load(ctx)` 这样的签名读起来自然。

### `ModContext::logger` {#ModContext.logger}

```rust
pub fn logger(&self) -> crate::Logger
```

- 返回值类型：`crate::Logger`

### `ModContext::host` {#ModContext.host}

```rust
pub fn host(&self) -> crate::Host
```

宿主和系统层面的能力：运行阶段、调度、执行命令和协议版本。

- 返回值类型：`crate::Host`

### `ModContext::packets` {#ModContext.packets}

```rust
pub fn packets(&self) -> crate::Packets
```

数据包门面。

- 返回值类型：`crate::Packets`

### `ModContext::world` {#ModContext.world}

```rust
pub fn world(&self) -> crate::World
```

世界门面。

- 返回值类型：`crate::World`

### `ModContext::server` {#ModContext.server}

```rust
pub fn server(&self) -> crate::Server
```

服务器运行时的控制：冻结和加速刻，以及性能采样。

- 返回值类型：`crate::Server`

### `ModContext::host_is_client` {#ModContext.host_is_client}

```rust
pub fn host_is_client(&self) -> bool
```

宿主是不是为客户端目标构建的。

很少需要用到：模组被加载到错误的目标上时，宿主会在握手时拒绝它。它的用处是让同一份源码不靠编译期特性，就能在两个目标之间做一点行为上的区分。

- 返回值类型：`bool`

### `ModContext::host_abi` {#ModContext.host_abi}

```rust
pub fn host_abi(&self) -> (u32, usize)
```

宿主的 ABI 版本和表长。用于诊断：报告「pier 太旧，不支持某个功能」时，附上这两个数，看的人才知道要升级到什么程度。

- 返回值类型：`(u32, usize)`

## `LeviMod` {#LeviMod}

```rust
pub trait LeviMod: Sized + Send + 'static { /* ... */ }
```

一个 Pier 模组。

`Send` 让 `ModSlot<T>`（一个 `Mutex<Option<T>>`）真正满足 `Sync`：生命周期回调可能在另一个线程上进入，因为宿主允许 `unload` 和跨模组的服务调用来自别的线程。

### `LeviMod::on_load` {#LeviMod.on_load}

```rust
fn on_load(ctx: &ModContext) -> Result<Self>
```

加载。返回 `Err` 时，宿主把这次加载视为失败并回滚，拆除的步骤照常运行。

- 参数：
    - ctx : `&ModContext`
- 返回值类型：`Result<Self>`

### `LeviMod::on_enable` {#LeviMod.on_enable}

```rust
fn on_enable(&mut self, _ctx: &ModContext) -> Result<()>
```

- 参数：
    - _ctx : `&ModContext`
- 返回值类型：`Result<()>`

### `LeviMod::on_disable` {#LeviMod.on_disable}

```rust
fn on_disable(&mut self, _ctx: &ModContext) -> Result<()>
```

- 参数：
    - _ctx : `&ModContext`
- 返回值类型：`Result<()>`

### `LeviMod::on_unload` {#LeviMod.on_unload}

```rust
fn on_unload(&mut self, _ctx: &ModContext) -> Result<()>
```

- 参数：
    - _ctx : `&ModContext`
- 返回值类型：`Result<()>`

## `TaskId` {#TaskId}

```rust
pub struct TaskId(pub(crate) u64);
```

一个定时任务的票据。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`、`Hash`

### `TaskId::is_valid` {#TaskId.is_valid}

```rust
pub fn is_valid(self) -> bool
```

- 返回值类型：`bool`

### `TaskId::raw` {#TaskId.raw}

```rust
pub fn raw(self) -> u64
```

- 返回值类型：`u64`

## `Error` {#Error}

```rust
pub struct Error(pub String);
```

一次 ABI 调用失败的原因。消息是写给人看的，可以直接记进日志。

- 实现的 trait：`Debug`、`From`、`Display`、`Error`

### `Error::new` {#Error.new}

```rust
pub fn new(msg: impl core::fmt::Display) -> Self
```

- 参数：
    - msg : `impl core::fmt::Display`
- 返回值类型：`Self`

## `Result` {#Result}

```rust
pub type Result<T> = core::result::Result<T, Error>;
```

## `LogLevel` {#LogLevel}

```rust
pub enum LogLevel {
        Fatal = 0,
        Error = 1,
        Warn = 2,
        Info = 3,
        Debug = 4,
        Trace = 5,
}
```

对应 `ll::io::LogLevel`。取值属于 ABI。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

## 宏 {#macros}

### `register_mod!` {#macro.register_mod}

```rust
register_mod! ($ty:ty)
```

生成 `pier_main` 入口点。每个模组写一次。

```rust
struct MyMod;
impl LeviMod for MyMod { /* ... */ }
levilamina::register_mod!(MyMod);
```

### `require_slot!` {#macro.require_slot}

```rust
require_slot! ($field:ident, $what:expr)
require_slot! (f)
```

两道关卡都检查。缺任何一道，都返回一个说明缺了什么的 `Err`。

用法：写在函数体开头，`require_slot!(md_add_dimension, "creating a dimension");`

消息里不带任何历史产品名（契约 §7）。以前有一版写着「...Update levilamina-rust-loader」，而这个名字的模组早就不存在了，照着去找的人什么都找不到；§5.3 要求日志回答「该怎么办」，反对的正是这种写法。

### `has_slot!` {#macro.has_slot}

```rust
has_slot! ($field:ident)
```

只问槽位在不在，不返回 `Err`。用于有就用、没有就降级的代码。

同样检查两道关卡：表够长并且不为空，才算存在。
