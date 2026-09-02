//! The families of integer constants in the ABI, split into three modules by topic.
//!
//! The split is for reading only and is not a semantic boundary: everything is flattened
//! under `levilamina_sys::` through `pub use`, and a caller need not know which file
//! `PIER_PPROP_HEALTH` lives in.
//!
//! The agreement of the three modules is guarded by the `sys-mirrors-abi` check: every
//! `enum` member in `abi.h` needs a constant of the same name and value here. Reality
//! forced that check into existence, since `PIER_AACT_ADD_EFFECT` was dropped whole
//! during manual transcription because its comment wrapped in `abi.h`, and a missing
//! constant shows up as calling add_effect while remove_effect runs.

pub mod actor;
pub mod player;
pub mod world;

pub use actor::*;
pub use player::*;
pub use world::*;
