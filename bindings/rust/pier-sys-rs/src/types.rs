//! The fundamental types and callback signatures of the contract, one by one against
//! `sdk/abi.h`.
//!
//! This layer wraps nothing: no `Drop`, no `From`, no lifetimes. Its whole job is to make
//! the Rust side see the same shape byte for byte as the C side. The safe wrapper is the
//! `pier-rs` layer, whose crate name is `levilamina`.
//!
//! The reason for two layers is not tidiness: every declaration here has to match
//! `abi.h` word for word, and a machine does the matching, in `sys-mirrors-abi`. Once
//! something like a convenient `impl Display` slips in here, the comparison first has to
//! tell contract from convenience, and there is no reliable machine criterion for that.

use core::ffi::c_void;

/// A UTF-8 string view. NUL termination is not guaranteed and the length is the only
/// bound.
///
/// It matches `typedef struct PierStr { const char* ptr; size_t len; }` in `abi.h`. An
/// alias for `std::string_view` here would make a C ABI depend on MSVC standard library
/// layout details and would need a runtime self-check to atone for it. The layout is
/// defined by this declaration itself, which is why no self-check is needed: a layout
/// defined by a declaration does not need the declaration verified.
///
/// Ownership follows contract §3: whoever produces frees, a receiver reads only during
/// the callback, and anything kept must be copied.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct PierStr {
    pub ptr: *const core::ffi::c_char,
    pub len: usize,
}

impl PierStr {
    /// The empty string, which is not NULL. The host allows a null `ptr` with
    /// `len == 0`, and going through this one constructor keeps empty to a single
    /// representation.
    pub const EMPTY: Self = Self {
        ptr: core::ptr::null(),
        len: 0,
    };

    /// Borrows a view from a Rust string.
    ///
    /// # Safety
    /// The caller must keep `s` alive until the host has finished reading it. The borrow
    /// checker cannot help across an ABI boundary, which is why this layer is `unsafe`.
    #[inline]
    pub fn borrow(s: &str) -> Self {
        Self {
            ptr: s.as_ptr() as *const core::ffi::c_char,
            len: s.len(),
        }
    }
}

/// A handle to a mod instance managed by the host, opaque to the mod.
pub type PierModHandle = *mut c_void;
/// An event listener handle.
pub type PierListenerHandle = *mut c_void;
/// The runtime id of an actor.
pub type PierActorId = i64;
/// A KvDb handle.
pub type PierKvDbHandle = *mut c_void;
/// A packet hook handle.
pub type PierPacketHookHandle = *mut c_void;
/// A client hotkey handle.
pub type PierKeyHandle = *mut c_void;
/// A hotkey action code, such as press or release.
pub type PierKeyAction = i32;
/// The focus impact of a hotkey.
pub type PierFocusImpact = i32;

/// The return of `get_player_position`. With `found == false` the coordinates mean
/// nothing.
///
/// This struct is itself an example of contract §5.2: not finding the player and the
/// player standing at the origin have to stay apart, so `found` is a separate bit rather
/// than being expressed by all-zero coordinates.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct PierPlayerPos {
    pub x: f64,
    pub y: f64,
    pub z: f64,
    pub dimension: i32,
    pub found: bool,
}

/// A player selector, where `kind` says how to interpret `value`, as a name, an XUID, a
/// UUID and so on.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct PierPlayerSel {
    pub kind: i32,
    pub value: PierStr,
}

/// A container reference, either a container on a player or one at a coordinate in the
/// world.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct PierContainerRef {
    pub which: i32,
    pub player: PierPlayerSel,
    pub dim: i32,
    pub x: i32,
    pub y: i32,
    pub z: i32,
}

/// How a provider describes a same-toolchain fast lane when publishing it.
///
/// `data` and `vtable` are allocated by the provider, which also keeps them alive
/// (contract §3). The host interprets not one byte of them and only compares for equality
/// and marks liveness.
#[repr(C)]
pub struct PierLaneDesc {
    pub struct_size: u32,
    pub protocol: u32,
    pub fingerprint: u64,
    pub data: *mut c_void,
    pub vtable: *const c_void,
    /// The C side allows NULL, as abi.h states. Reading NULL through a non-`Option`
    /// function pointer is undefined behavior in Rust, and `Option<fn>` has the same
    /// layout as a raw pointer.
    pub retain: Option<PierLaneRefFn>,
    pub release: Option<PierLaneRefFn>,
}

/// A lane that was acquired.
///
/// `alive` points at a liveness flag the host owns and never frees, which the host clears
/// the moment the provider disappears. A consumer reads it before every use of `data` or
/// `vtable`, since it is the only criterion that still holds across a `FreeLibrary`.
#[repr(C)]
pub struct PierLaneRef {
    pub struct_size: u32,
    pub lease: u64,
    pub fingerprint: u64,
    pub data: *mut c_void,
    pub vtable: *const c_void,
    pub alive: *const u32,
    pub busy: *mut u32,
}

/// The event a packet hook receives. `body` is valid only during the callback.
#[repr(C)]
pub struct PierPacketEvent {
    pub struct_size: u32,
    pub direction: i32,
    pub conn_id: u64,
    pub address: PierStr,
    pub packet_id: i32,
    pub sender_sub_id: u8,
    pub target_sub_id: u8,
    pub body: *const u8,
    pub body_len: usize,
}

/// Where a callback writes a rewritten packet header. It is read only when
/// `PIER_PKT_REPLACE` is returned.
#[repr(C)]
pub struct PierPacketEdit {
    pub struct_size: u32,
    pub packet_id: i32,
    pub sender_sub_id: u8,
    pub target_sub_id: u8,
}

/// The kind of an economy event.
///
/// The name carries no external product name. A name matching the one in the LegacyMoney
/// headers collides in the global scope, and including both at once redefines it outright.
/// The rename also cleared an external product name off the contract surface
/// (contract §7).
#[repr(i32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum PierMoneyEvent {
    Set = 0,
    Add = 1,
    Reduce = 2,
    Trans = 3,
}

// Callback signatures.
//
// All are `unsafe extern "C" fn` and none is an `Option<...>`, because in the contract
// they are raw function pointer types such as `typedef void (*PierTaskCb)(void*)`, where
// passing NULL is a caller bug and not a valid value. Only the slots of `PierApi` use
// `Option`, since an empty slot is the formal way to say the capability was not compiled
// into the host.

pub type PierTaskCb = unsafe extern "C" fn(user: *mut c_void);
pub type PierStrSink = unsafe extern "C" fn(ctx: *mut c_void, s: PierStr);
pub type PierEventCb = unsafe extern "C" fn(
    user: *mut c_void,
    event_id: PierStr,
    snbt: PierStr,
    write_ctx: *mut c_void,
    write_back: PierStrSink,
);
pub type PierCommandCb = unsafe extern "C" fn(
    user: *mut c_void,
    args: PierStr,
    origin_name: PierStr,
    out_ctx: *mut c_void,
    out_success: PierStrSink,
    out_error: PierStrSink,
);
pub type PierCmdOutputSink = unsafe extern "C" fn(ctx: *mut c_void, success: bool, output: PierStr);
pub type PierBlockSink =
    unsafe extern "C" fn(ctx: *mut c_void, x: i32, y: i32, z: i32, name: PierStr, snbt: PierStr);
pub type PierEntitySink =
    unsafe extern "C" fn(ctx: *mut c_void, x: i32, y: i32, z: i32, type_: PierStr, snbt: PierStr);
pub type PierBytesSink = unsafe extern "C" fn(ctx: *mut c_void, data: *const u8, len: usize);
pub type PierKvSink = unsafe extern "C" fn(ctx: *mut c_void, key: PierStr, value: PierStr);
pub type PierActorSink =
    unsafe extern "C" fn(ctx: *mut c_void, id: PierActorId, type_name: PierStr);
pub type PierFormResultCb = unsafe extern "C" fn(user: *mut c_void, result_snbt: PierStr);
pub type PierBusCb =
    unsafe extern "C" fn(user: *mut c_void, topic: PierStr, payload: PierStr) -> bool;
pub type PierServiceCb = unsafe extern "C" fn(
    user: *mut c_void,
    name: PierStr,
    request: PierStr,
    ctx: *mut c_void,
    reply: PierStrSink,
) -> bool;
pub type PierLaneRefFn = unsafe extern "C" fn(data: *mut c_void);
pub type PierPacketCb = unsafe extern "C" fn(
    user: *mut c_void,
    ev: *const PierPacketEvent,
    edit: *mut PierPacketEdit,
    replace_ctx: *mut c_void,
    replace: PierBytesSink,
) -> i32;
pub type PierConnCb =
    unsafe extern "C" fn(user: *mut c_void, conn_id: u64, address: PierStr, opened: bool);
pub type PierKeyCb =
    unsafe extern "C" fn(user: *mut c_void, action: PierKeyAction, impact: PierFocusImpact);
pub type PierMoneyCb =
    unsafe extern "C" fn(type_: PierMoneyEvent, from: PierStr, to: PierStr, value: i64) -> bool;
