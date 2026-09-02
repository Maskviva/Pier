//! `levilamina`: writing LeviLamina mods in safe Rust.
//!
//! ```ignore
//! struct Hello;
//! impl LeviMod for Hello {
//!     fn on_load(ctx: &ModContext) -> Result<Self> { Ok(Hello) }
//! }
//! levilamina::register_mod!(Hello);
//! ```
//!
//! On top of `levilamina_sys`, the cell-for-cell mirror of `sdk/abi.h`, it does four things
//! and no more: the two slot gates, string handling, the panic fence, and copying inside a
//! sink (contract §3).
//! It caches no host state, feigns no synchronization and covers for the host in nothing: a
//! failure is an `Err` that says why and never quietly becomes a default (§5.1).

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

/// The internal entry point macro expansion uses. It is not a stable API and mod code must
/// not touch it directly.
#[doc(hidden)]
pub mod __rt {
    pub use crate::rt::registration::mod_flags;
    pub use crate::rt::runtime::{api, has_slot, host_abi};
}

// Domain modules. Each domain is a safe facade over one group of slots in `PierApi`.
// Which may depend on which is guarded by the `rust-layering` check, whose graph is in
// tools/checks/rust_layering.py.
//
// Two rules: a constant is spelled in full, as `sys::PIER_BSTR_TYPE_NAME`, for the reason
// the sys-mirrors-abi script documents; and a non-core slot is always called through
// `require_slot!` or `has_slot!`, never by reading a field of `__rt::api()` directly
// (contract §2.2).

pub mod prelude {
    //! One `use levilamina::prelude::*;` brings in what writing a mod needs most.
    pub use crate::{event::names, service};
    pub use crate::{
        register_mod, Block, ConnectionState, Container, Direction, Directions, Entity, Error,
        Event, EventRef, GameMode, GamingStatus, Host, ItemStack, LeviMod, Listener, LogLevel,
        Logger, ModContext, NbtValue, Packet, PacketHook, Packets, Player, PlayerIdentity,
        PlayerSel, Priority, Result, Server, TaskId, Verdict, Wiring, World,
    };
}
