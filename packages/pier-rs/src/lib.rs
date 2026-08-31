//! `levilamina` —— 用安全 Rust 写 LeviLamina 模组。
//!
//! ```ignore
//! use levilamina::prelude::*;
//!
//! struct Hello;
//!
//! impl LeviMod for Hello {
//!     fn on_load(ctx: &ModContext) -> Result<Self> {
//!         ctx.logger().info("你好，栈桥。");
//!         Ok(Hello)
//!     }
//! }
//!
//! levilamina::register_mod!(Hello);
//! ```
//!
//! # 这个 crate 和 `levilamina_sys` 的分工
//!
//! `levilamina_sys` 是 `sdk/abi.h` 的逐格镜像，每一次调用都是 `unsafe`。
//! 这个 crate 在它上面做四件事，一件都不多：
//!
//! 1. **两道槽位闸** —— 表够不够长、槽是不是空（见 `require_slot!`）；
//! 2. **字符串收口** —— `PierStr` ↔ `&str`，含 UTF-8 校验（见 `rt::ffi`）；
//! 3. **panic 围栏** —— 任何跨 `extern "C"` 的回调都包在 `catch_unwind` 里；
//! 4. **所有权** —— 契约 §三 说跨边界不移交所有权，所以这里的每个 sink
//!    都在回调内把数据拷走。
//!
//! 它**不做**的事：不缓存宿主状态、不假装同步、不替宿主兜底。一次调用失败
//! 就是一个说得清原因的 `Err`，不会悄悄变成默认值（契约 §5.1）。
//!
//! # v1 起没有会改 API 面的 feature
//!
//! v0 有 `server` / `client` / `more_dimensions` 三个，其中前两个互斥地裁剪
//! 模块树。ABI v1 的布局在所有目标下相同（契约 §2.1），所以：
//!
//! * 客户端专属的槽在服务端宿主上是 NULL，调用它返回
//!   「宿主不提供此能力」的 `Err`，而不是编译错误；
//! * 维度能力靠运行期的 `md_is_available()` 判断，不再有
//!   `more_dimensions` feature。
//!
//! 换来的是：**同一份模组源码在两个目标上都编得过**，装错目标由宿主在握手
//! 时明确拒绝，而不是让人对着一堆 `cfg` 报错猜自己开错了哪个 feature。

pub use levilamina_sys as sys;

pub mod event;
mod host;
pub mod nbt;
pub mod packet;
mod rt;
mod sel;
pub mod service;

pub use event::{Event, EventPriority, EventRef, Listener, PlayerIdentity, Priority, Wiring};
pub use host::{GamingStatus, Host};
pub use nbt::{NbtError, NbtResult, NbtValue};
pub use sel::PlayerSel;
pub use service::CallError;
pub use packet::{ConnectionState, Direction, Directions, Packet, PacketHook, Packets, Verdict};
pub use rt::error::{Error, Result};
pub use rt::logger::{LogLevel, Logger};
pub use rt::registration::{LeviMod, ModSlot, __init_runtime, __lifecycle, __load};
pub use rt::runtime::{ModContext, TaskId};

/// 宏展开后要用的内部入口。**不是稳定 API**，模组代码不要直接碰。
#[doc(hidden)]
pub mod __rt {
    pub use crate::rt::registration::mod_flags;
    pub use crate::rt::runtime::{api, has_slot, host_abi};
}

// ── 域模块 ────────────────────────────────────────────────────────
//
// 每个域是 `PierApi` 里一组槽的安全门面。它们互不依赖，只依赖 `rt`。
//
// **常量用全名**：`sys::PIER_BSTR_TYPE_NAME`，不是 v0 那种去了前缀的
// `sys::BSTR_TYPE_NAME`。理由是 `sys-mirrors-abi` 机检按**字面**比对
// `abi.h` 里的枚举成员名 —— 一旦 sys 那层再提供一套去前缀的别名，
// 同一个常量就有了两个拼法，而机检只守得住其中一个。
// 一个东西一个名字，这条比「打字短一点」重要。

pub mod prelude {
    //! 一行 `use levilamina::prelude::*;` 拿到写模组最常用的那些东西。
    pub use crate::{
        register_mod, ConnectionState, Direction, Directions, Error, Event, EventRef,
        GamingStatus, Host, LeviMod, Listener, LogLevel, Logger, ModContext, NbtValue, Packet,
        PacketHook, Packets, PlayerIdentity, PlayerSel, Priority, Result, TaskId, Verdict, Wiring,
    };
    pub use crate::{event::names, service};
}
