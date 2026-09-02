//! `levilamina_sys`: the raw FFI declarations of Pier ABI v1.
//! This crate is a cell-for-cell mirror of `packages/pier-abi/include/sdk/abi.h` and the reference
//! implementation of the four steps contract §10 gives for adding a language:
//! 1. read `sdk/abi.h` only;
//! 2. declare the `PierApi` mirror, declaring every field unconditionally, with no
//!    target branch anywhere;
//! 3. export `pier_main` and fill the vtable with `struct_size`, `abi_version`,
//!    `mod_flags` and the three lifecycle callbacks;
//! 4. before calling a non-core slot, check that `struct_size` covers it and that the
//!    slot is not NULL.
//!    Another language follows those four steps rather than copying this crate: the
//!    contract is that header.
//!
//! This layer offers no safety. Every call is `unsafe` and string lifetimes rest on the caller. The
//! safe wrapper is `pier-rs`, whose crate name is `levilamina`, and `types.rs` gives the reason for
//! the split.

#![no_std]
#![allow(non_camel_case_types)]

pub mod api;
pub mod consts;
pub mod types;
pub mod vtable;

pub use api::PierApi;
pub use consts::*;
pub use types::*;
pub use vtable::{PierMainFn, PierModVTable};

/// The ABI version this crate is compiled against. Goes into `PierModVTable::abi_version`.
pub const PIER_ABI_VERSION: u32 = 1;

/// The oldest mod ABI the host accepts. Compatibility is a range and not an equality:
/// `MIN_SUPPORTED <= mod_abi <= VERSION` (contract §2.2).
///
/// A copy lives in the mirror so that a mod can state for itself, when loading fails,
/// that it was built against one version while the host wants a given range, rather than
/// reporting only that the version does not match.
pub const PIER_ABI_MIN_SUPPORTED: u32 = 1;

/// The entry symbol name a mod must export.
pub const PIER_MAIN_SYMBOL: &str = "pier_main";

/// Bit 0 of `host_flags` and `mod_flags`: this side is a client build.
///
/// Bit 0 must be equal on both sides, otherwise the host refuses to load and says why.
/// The remaining bits are reserved and must be 0.
pub const PIER_FLAG_CLIENT: u32 = 0x1;

// Result codes for a cross-mod service call.
//
// Note that `OK` is 0. That has a practical consequence for error handling: a fallback
// returning zero on error reports a failure as a success, which is exactly the shape
// contract §5.1 forbids. `PIER_API_GUARD_END_VAL` on the C++ side therefore has to give
// an explicit failure value for this family of entry points.

pub const PIER_SERVICE_OK: i32 = 0;
/// Nobody provides this name, because the mod is not installed, or is disabled, or has
/// been unloaded.
pub const PIER_SERVICE_NOT_FOUND: i32 = 1;
/// The provider ran and returned a failure, and `reply` holds its error message.
pub const PIER_SERVICE_ERROR: i32 = 2;
/// The name is invalid, the call is to itself, or the call depth is exceeded.
pub const PIER_SERVICE_REFUSED: i32 = 3;

// The same-toolchain fast lane.

/// The lane protocol version. `PierLaneDesc::protocol` must equal it or publishing is refused.
pub const PIER_LANE_PROTOCOL: u32 = 1;

pub const PIER_LANE_OK: i32 = 0;
/// Nobody publishes this name, because the mod is not installed.
pub const PIER_LANE_NOT_FOUND: i32 = 1;
/// It is published with a different fingerprint. Fall back to `service_call` and pass no
/// pointer.
pub const PIER_LANE_FINGERPRINT: i32 = 2;
/// The name is invalid, the lane is its own, the provider is disabled, or the protocol
/// does not match.
pub const PIER_LANE_REFUSED: i32 = 3;

// Packet direction and disposition.

pub const PIER_PKT_INBOUND: i32 = 0;
pub const PIER_PKT_OUTBOUND: i32 = 1;
pub const PIER_PKT_MASK_INBOUND: i32 = 1 << PIER_PKT_INBOUND;
pub const PIER_PKT_MASK_OUTBOUND: i32 = 1 << PIER_PKT_OUTBOUND;

/// Forward unchanged; the output of `replace` is ignored.
pub const PIER_PKT_PASS: i32 = 0;
/// Forward the body handed to `replace`.
pub const PIER_PKT_REPLACE: i32 = 1;
/// Consume the packet entirely.
pub const PIER_PKT_DROP: i32 = 2;
