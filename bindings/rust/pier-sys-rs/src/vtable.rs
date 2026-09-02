//! The table a mod hands to the host, and the signature of the entry symbol.
//!
//! This is the mod side of the handshake. The host half lives in
//! `pier-host/src/ModHost.cpp`, and the order matters: read the length first, then the
//! version, then compare the target flags.

use core::ffi::c_void;

use crate::api::PierApi;
use crate::types::PierModHandle;

/// The table a mod fills in inside `pier_main` and hands to the host.
///
/// It carries its own `struct_size`, so the vtable follows the append-only path too: the
/// host reads only the fields within the length the mod declared, and adding a callback
/// needs no ABI version bump and condemns no already-compiled mod.
///
/// Target matching goes through a separate `mod_flags` and no marker is hidden in the
/// high bits of the version number. Packed into one number, every test would have to
/// unpack it first, and such a marker protects no mod that was not rebuilt.
///
/// The host compares bit 0 of `mod_flags` against `PierApi::host_flags` and, on a
/// mismatch, refuses to load explicitly and says why. Being explicit is the point:
/// letting it load and then crashing on the first slot that exists on one side only
/// cannot be traced.
#[repr(C)]
pub struct PierModVTable {
    /// `size_of::<PierModVTable>()`, filled in by the mod from the table it compiled.
    pub struct_size: u32,
    /// The `PIER_ABI_VERSION` the mod was compiled against.
    pub abi_version: u32,
    /// The bitwise or of the `PIER_FLAG_*` values. Bit 0 means this is a client mod.
    pub mod_flags: u32,
    /// Reserved and always 0. It completes a 16-byte header and leaves room for a future
    /// header scalar.
    pub _reserved0: u32,
    /// The mod's own state pointer. The host passes it back to the three callbacks
    /// unchanged and does not interpret it.
    pub instance: *mut c_void,
    pub on_enable: Option<unsafe extern "C" fn(instance: *mut c_void) -> bool>,
    pub on_disable: Option<unsafe extern "C" fn(instance: *mut c_void) -> bool>,
    pub on_unload: Option<unsafe extern "C" fn(instance: *mut c_void) -> bool>,
}

/// The single entry point a mod must export. The host looks for the name `pier_main` and
/// nothing else, and refuses to load explicitly with no fallback when it is absent
/// (contract §2.4).
pub type PierMainFn = unsafe extern "C" fn(
    api: *const PierApi,
    self_: PierModHandle,
    out_vtable: *mut PierModVTable,
) -> bool;
