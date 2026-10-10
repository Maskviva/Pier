# levilamina: lifecycle, logging and errors

`levilamina`: writing LeviLamina mods in safe Rust.

```rust
struct Hello;
impl LeviMod for Hello {
    fn on_load(ctx: &ModContext) -> Result<Self> { Ok(Hello) }
}
levilamina::register_mod!(Hello);
```

On top of `levilamina_sys`, the cell-for-cell mirror of `sdk/abi.h`, it does four things
and no more: the two slot gates, string handling, the panic fence, and copying inside a
sink (contract §3).
It caches no host state, feigns no synchronization and covers for the host in nothing: a
failure is an `Err` that says why and never quietly becomes a default (§5.1).

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

One `use levilamina::prelude::*;` brings in what writing a mod needs most.

## `Logger` {#Logger}

```rust
pub struct Logger(/* private */);
```

- Implements: `Clone`, `Copy`

### `Logger::get` {#Logger.get}

```rust
pub fn get() -> Logger
```

- Return type: `Logger`

### `Logger::log` {#Logger.log}

```rust
pub fn log(&self, level: LogLevel, msg: &str)
```

- Parameters:
    - level : `LogLevel`
    - msg : `&str`

### `Logger::fatal` {#Logger.fatal}

```rust
pub fn fatal(&self, msg: &str)
```

- Parameters:
    - msg : `&str`

### `Logger::error` {#Logger.error}

```rust
pub fn error(&self, msg: &str)
```

- Parameters:
    - msg : `&str`

### `Logger::warn` {#Logger.warn}

```rust
pub fn warn(&self, msg: &str)
```

- Parameters:
    - msg : `&str`

### `Logger::info` {#Logger.info}

```rust
pub fn info(&self, msg: &str)
```

- Parameters:
    - msg : `&str`

### `Logger::debug` {#Logger.debug}

```rust
pub fn debug(&self, msg: &str)
```

- Parameters:
    - msg : `&str`

### `Logger::trace` {#Logger.trace}

```rust
pub fn trace(&self, msg: &str)
```

- Parameters:
    - msg : `&str`

## `ModContext` {#ModContext}

```rust
pub struct ModContext(/* private */);
```

The context handed to a mod's lifecycle callbacks.

It carries no state of its own, since the real state lives in `RUNTIME`. It exists to
give the facades one common entry point, and to make a signature such as
`on_load(ctx)` read sensibly.

### `ModContext::logger` {#ModContext.logger}

```rust
pub fn logger(&self) -> crate::Logger
```

- Return type: `crate::Logger`

### `ModContext::host` {#ModContext.host}

```rust
pub fn host(&self) -> crate::Host
```

Capabilities at the host and system level: the run stage, scheduling, executing
commands and the protocol version.

- Return type: `crate::Host`

### `ModContext::packets` {#ModContext.packets}

```rust
pub fn packets(&self) -> crate::Packets
```

The packet facade.

- Return type: `crate::Packets`

### `ModContext::world` {#ModContext.world}

```rust
pub fn world(&self) -> crate::World
```

The world facade.

- Return type: `crate::World`

### `ModContext::server` {#ModContext.server}

```rust
pub fn server(&self) -> crate::Server
```

Server runtime control: freezing and warping ticks, and performance sampling.

- Return type: `crate::Server`

### `ModContext::host_is_client` {#ModContext.host_is_client}

```rust
pub fn host_is_client(&self) -> bool
```

Whether the host was built for the client target.

Rarely needed, since a mod loaded onto the wrong target is refused by the host during
the handshake. It exists so that one source can make a small behavioral distinction
between the two targets without a compile-time feature.

- Return type: `bool`

### `ModContext::host_abi` {#ModContext.host_abi}

```rust
pub fn host_abi(&self) -> (u32, usize)
```

The ABI version and table length of the host. For diagnostics: reporting that a pier is
too old for a feature only tells someone how far to upgrade when these two numbers come
with it.

- Return type: `(u32, usize)`

## `LeviMod` {#LeviMod}

```rust
pub trait LeviMod: Sized + Send + 'static { /* ... */ }
```

One Pier mod.

`Send` is what makes `ModSlot<T>`, a `Mutex<Option<T>>`, genuinely `Sync`: a lifecycle
callback may be entered on a different thread, since the host allows `unload` and a
cross-mod service call to come from another thread.

### `LeviMod::on_load` {#LeviMod.on_load}

```rust
fn on_load(ctx: &ModContext) -> Result<Self>
```

Load. An `Err` makes the host treat loading as failed and roll back, with the teardown
steps running as usual.

- Parameters:
    - ctx : `&ModContext`
- Return type: `Result<Self>`

### `LeviMod::on_enable` {#LeviMod.on_enable}

```rust
fn on_enable(&mut self, _ctx: &ModContext) -> Result<()>
```

- Parameters:
    - _ctx : `&ModContext`
- Return type: `Result<()>`

### `LeviMod::on_disable` {#LeviMod.on_disable}

```rust
fn on_disable(&mut self, _ctx: &ModContext) -> Result<()>
```

- Parameters:
    - _ctx : `&ModContext`
- Return type: `Result<()>`

### `LeviMod::on_unload` {#LeviMod.on_unload}

```rust
fn on_unload(&mut self, _ctx: &ModContext) -> Result<()>
```

- Parameters:
    - _ctx : `&ModContext`
- Return type: `Result<()>`

## `TaskId` {#TaskId}

```rust
pub struct TaskId(pub(crate) u64);
```

The ticket of a scheduled task.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`, `Hash`

### `TaskId::is_valid` {#TaskId.is_valid}

```rust
pub fn is_valid(self) -> bool
```

- Return type: `bool`

### `TaskId::raw` {#TaskId.raw}

```rust
pub fn raw(self) -> u64
```

- Return type: `u64`

## `Error` {#Error}

```rust
pub struct Error(pub String);
```

Why one ABI call failed. The message is meant for a human and can go straight into a
log.

- Implements: `Debug`, `From`, `Display`, `Error`

### `Error::new` {#Error.new}

```rust
pub fn new(msg: impl core::fmt::Display) -> Self
```

- Parameters:
    - msg : `impl core::fmt::Display`
- Return type: `Self`

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

Mirrors `ll::io::LogLevel`. The values are part of the ABI.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

## Macros {#macros}

### `register_mod!` {#macro.register_mod}

```rust
register_mod! ($ty:ty)
```

Generates the `pier_main` entry point. Written once per mod.

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

Both gates. Missing either returns an `Err` that says what is missing.

Usage: at the top of a function body,
`require_slot!(md_add_dimension, "creating a dimension");`

The message carries no historical product name (contract §7). An earlier one read
"...Update levilamina-rust-loader" while no mod of that name exists any more, so anyone
following it finds nothing, which is exactly the shape §5.3 opposes when it says a log
line has to answer what to do about it.

### `has_slot!` {#macro.has_slot}

```rust
has_slot! ($field:ident)
```

Only asks whether it exists and returns no `Err`. For code that uses it when present
and degrades otherwise.

Both gates again: long enough and non-null counts as present.
