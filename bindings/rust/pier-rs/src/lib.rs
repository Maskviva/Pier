//! `levilamina` —— 用安全 Rust 写 LeviLamina 模组。
//!
//! ```ignore
//! struct Hello;
//! impl LeviMod for Hello {
//!     fn on_load(ctx: &ModContext) -> Result<Self> { Ok(Hello) }
//! }
//! levilamina::register_mod!(Hello);
//! ```
//!
//! 在 `levilamina_sys`（`sdk/abi.h` 的逐格镜像）之上做四件事，一件不多：
//! 两道槽位闸、字符串收口、panic 围栏、sink 内拷贝（契约 §三）。
//! 不缓存宿主状态，不假装同步，不替宿主兜底 —— 失败就是说得清原因的
//! `Err`，不会悄悄变成默认值（§5.1）。

pub use levilamina_sys as sys;

pub mod block;
pub mod bus;
pub mod client;
pub mod command;
pub mod container;
mod context;
pub mod dimensions;
pub mod entity;
pub mod event;
pub mod gui;
mod host;
pub mod item;
pub mod kvdb;
pub mod lane;
pub mod money;
pub mod nbt;
pub mod packet;
pub mod player;
mod rt;
pub mod scoreboard;
mod sel;
pub mod server;
pub mod service;
pub mod sim;
pub mod types;
pub mod world;

pub use block::{Block, BlockInfo};
pub use command::{CommandOrigin, CommandPermission, Invocation, ParamType, SoftEnumOp};
pub use container::{Container, ContainerKind};
pub use context::ModContext;
pub use entity::{Aabb, ActorEntry, Effect, Entity};
pub use event::{Event, EventPriority, EventRef, Listener, PlayerIdentity, Priority, Wiring};
pub use gui::{CustomForm, FormResponse, FormValue, ModalForm, SimpleForm};
pub use host::{GamingStatus, Host};
pub use item::{Enchant, ItemStack};
pub use kvdb::KvDb;
pub use nbt::{NbtError, NbtFormat, NbtResult, NbtValue};
pub use packet::{ConnectionState, Direction, Directions, Packet, PacketHook, Packets, Verdict};
pub use player::{Player, PlayerInfo};
pub use rt::error::{Error, Result};
pub use rt::logger::{LogLevel, Logger};
pub use rt::registration::{__init_runtime, __lifecycle, __load, LeviMod, ModSlot};
pub use rt::runtime::TaskId;
pub use scoreboard::{DisplaySlot, Objective, Scoreboard};
pub use sel::PlayerSel;
pub use server::Server;
pub use service::CallError;
pub use sim::SimPlayer;
pub use types::{
    Ability, BlockUpdate, Bounds, Difficulty, EquipSlot, GameMode, LocalTime, MessageType,
    PlayerPermission, PositionF64, PositionI32, RayHit, TitleKind, TitleTimes, Weather,
};
pub use world::{Box3D, Scan, World};

/// 宏展开后要用的内部入口。**不是稳定 API**，模组代码不要直接碰。
#[doc(hidden)]
pub mod __rt {
    pub use crate::rt::registration::mod_flags;
    pub use crate::rt::runtime::{api, has_slot, host_abi};
}

// ── 域模块 ────────────────────────────────────────────────────────
//
// 每个域是 `PierApi` 里一组槽的安全门面。谁能依赖谁由 `rust-layering` 机检
// 守着，那张图在 tools/checks/rust_layering.py 里。
//
// 两条纪律：常量用全名（`sys::PIER_BSTR_TYPE_NAME`，理由见 sys-mirrors-abi
// 的脚本文档）；调非核心槽一律走 `require_slot!` / `has_slot!`，不许直接读
// `__rt::api().字段`（契约 §2.2）。

pub mod prelude {
    //! 一行 `use levilamina::prelude::*;` 拿到写模组最常用的那些东西。
    pub use crate::{event::names, service};
    pub use crate::{
        register_mod, Block, ConnectionState, Container, Direction, Directions, Entity, Error,
        Event, EventRef, GameMode, GamingStatus, Host, ItemStack, LeviMod, Listener, LogLevel,
        Logger, ModContext, NbtValue, Packet, PacketHook, Packets, Player, PlayerIdentity,
        PlayerSel, Priority, Result, Server, TaskId, Verdict, Wiring, World,
    };
}
