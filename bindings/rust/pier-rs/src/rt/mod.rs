//! The runtime foundation: the handshake, the slot gates, string handling, logging and the
//! panic fence.
//!
//! This layer knows no game concept. The domain modules, `player`, `block` and the rest,
//! are all built on it while it knows only `levilamina_sys`.

#[macro_use]
pub(crate) mod accessors;
pub(crate) mod error;
pub(crate) mod ffi;
pub(crate) mod handle;
pub(crate) mod logger;
pub(crate) mod registration;
pub(crate) mod runtime;
