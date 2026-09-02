//! The same-toolchain fast lane: a direct function-table call that bypasses JSON.
//!
//! The division with [`crate::service`]: service is a cross-language `(name, JSON) -> JSON`
//! channel that always holds, while a lane passes raw data and vtable pointers and holds
//! only when both sides were built by the same toolchain. A fingerprint mismatch yields no
//! pointer and falls back to service.
//!
//! The fingerprint has to be computed by the caller and `0` is invalid: this slot hands
//! the vtable over as is, a consumer interprets it through its own type layout, and
//! skipping the check is type confusion. A `0` would read as anyone may connect.
//! [`list`] shows which lanes exist and passes no pointer at all.
//!
//! The discipline for each call after acquiring one is in [`Lane::with`].

use core::ffi::c_void;
use core::sync::atomic::{AtomicU32, Ordering};

use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, s};
use crate::rt::logger::Logger;
use crate::rt::runtime::rt;
use crate::sys;

/// One lane contract: the function-table shape both sides agreed on.
///
/// `FINGERPRINT` has to change whenever the shape of the table changes, including field
/// order, parameter types and calling convention. It matches automatically when both sides
/// reference the same contract definition, the same version of the same crate. Copying an
/// identical constant by hand is wrong, and is exactly what it guards against.
pub trait LaneContract {
    /// The function-table type with C layout.
    type Table;
    /// The lane name.
    const NAME: &'static str;
    /// The build fingerprint. It must not be 0.
    const FINGERPRINT: u64;
}

/// Why acquiring a lane failed.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum LaneError {
    /// Nobody publishes this name, because the mod is not installed.
    NotFound { name: String },
    /// It is published with a different fingerprint. Fall back to service and pass no pointer.
    Fingerprint {
        name: String,
        theirs: u64,
        ours: u64,
    },
    /// The name is invalid, the lane is its own, the provider is disabled, or the protocol
    /// version does not match.
    Refused { name: String },
    /// The host offers no fast-lane capability.
    Unavailable,
}

impl LaneError {
    /// One sentence on what to do about it. This goes in the log rather than the enum name
    /// (contract §5.3).
    pub fn advice(&self) -> String {
        match self {
            LaneError::NotFound { name } => {
                format!("no mod publishes the lane `{name}`: it is not installed or has not come up yet. Use service or skip this integration.")
            }
            LaneError::Fingerprint { name, theirs, ours } => format!(
                "the fingerprint of lane `{name}` does not match, theirs {theirs:#x} against ours \
                 {ours:#x}: the two mods were not built by the same toolchain. This is not an error; fall back to service::call."
            ),
            LaneError::Refused { name } => {
                format!(
                    "the host refused to acquire lane `{name}`: the name is invalid, the lane is its own, the provider is disabled, or the protocol version does not match."
                )
            }
            LaneError::Unavailable => "this host offers no fast-lane capability.".to_owned(),
        }
    }
}

impl std::fmt::Display for LaneError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(&self.advice())
    }
}

impl std::error::Error for LaneError {}

impl From<LaneError> for Error {
    fn from(e: LaneError) -> Error {
        Error(e.advice())
    }
}

/// An acquired lane. Dropping it returns the lease.
pub struct Lane<C: LaneContract> {
    lease: u64,
    fingerprint: u64,
    data: *mut c_void,
    vtable: *const C::Table,
    alive: *const u32,
    busy: *mut u32,
}

impl<C: LaneContract> Lane<C> {
    /// Whether the provider is still there.
    ///
    /// Read with `Acquire`: the writer uses a release store and a relaxed read establishes no
    /// happens-before relationship with it.
    pub fn is_alive(&self) -> bool {
        if self.alive.is_null() {
            return false;
        }
        // SAFETY: the host owns this liveness flag and never frees it, so reading it after the
        // provider disappears is still valid, which is its entire reason for existing.
        let cell = unsafe { &*(self.alive as *const AtomicU32) };
        cell.load(Ordering::Acquire) != 0
    }

    pub fn fingerprint(&self) -> u64 {
        self.fingerprint
    }

    /// Runs a piece of code inside the provider. Returns `None` when the provider is gone.
    ///
    /// `busy` is incremented on the way in and decremented on the way out. During that window
    /// the host refuses to unload the provider, so `FreeLibrary` cannot pull that stack frame
    /// out from under you; see the module documentation.
    /// The closure receives a [`LaneData`] rather than a raw `*mut c_void`, because the first
    /// parameter of every function in a lane table is a `LaneData`, which is the other side's
    /// self, and handing over a raw pointer would make the caller wrap it by hand at every
    /// call site.
    pub fn with<R>(&self, f: impl FnOnce(&C::Table, LaneData) -> R) -> Option<R> {
        if !self.is_alive() || self.vtable.is_null() {
            return None;
        }
        let busy = if self.busy.is_null() {
            None
        } else {
            // SAFETY: as with `alive`, the host owns this counter and never frees it.
            Some(unsafe { &*(self.busy as *const AtomicU32) })
        };
        if let Some(b) = busy {
            b.fetch_add(1, Ordering::AcqRel);
        }
        // SAFETY: the vtable is non-null and alive is true, and the provider keeps it alive
        // until the lane is withdrawn.
        let table = unsafe { &*self.vtable };
        let out = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            f(table, LaneData(self.data))
        }));
        if let Some(b) = busy {
            b.fetch_sub(1, Ordering::AcqRel);
        }
        match out {
            Ok(v) => Some(v),
            Err(_) => {
                // A panic is caught here and not propagated: this is the critical section of
                // `busy`, letting one through would skip every cleanup after the fetch_sub below,
                // and more importantly the caller would most likely carry it across an extern "C"
                // boundary.
                // The count was already decremented above, so the provider can still be unloaded.
                Logger::get().error(
                    "a lane call panicked. It was caught here and the busy count was restored.",
                );
                None
            }
        }
    }
}

impl<C: LaneContract> Drop for Lane<C> {
    fn drop(&mut self) {
        if self.lease == 0 {
            return;
        }
        if !crate::has_slot!(lane_release) {
            return;
        }
        let Some(f) = crate::__rt::api().lane_release else {
            return;
        };
        // A false means the provider is already gone, the host released it already by
        // then, and calling again would double release. So there is no retry and no error.
        unsafe { f(rt().handle(), self.lease) };
    }
}

pub fn acquire<C: LaneContract>() -> std::result::Result<Lane<C>, LaneError> {
    if !crate::has_slot!(lane_acquire) {
        return Err(LaneError::Unavailable);
    }
    let Some(f) = crate::__rt::api().lane_acquire else {
        return Err(LaneError::Unavailable);
    };
    let mut out = sys::PierLaneRef {
        struct_size: core::mem::size_of::<sys::PierLaneRef>() as u32,
        lease: 0,
        fingerprint: 0,
        data: core::ptr::null_mut(),
        vtable: core::ptr::null(),
        alive: core::ptr::null(),
        busy: core::ptr::null_mut(),
    };
    let code = unsafe {
        f(
            rt().handle(),
            s(C::NAME),
            C::FINGERPRINT,
            &mut out as *mut sys::PierLaneRef,
        )
    };
    match code {
        sys::PIER_LANE_OK => Ok(Lane {
            lease: out.lease,
            fingerprint: out.fingerprint,
            data: out.data,
            vtable: out.vtable as *const C::Table,
            alive: out.alive,
            busy: out.busy,
        }),
        sys::PIER_LANE_NOT_FOUND => Err(LaneError::NotFound {
            name: C::NAME.to_owned(),
        }),
        sys::PIER_LANE_FINGERPRINT => Err(LaneError::Fingerprint {
            name: C::NAME.to_owned(),
            theirs: out.fingerprint,
            ours: C::FINGERPRINT,
        }),
        _ => Err(LaneError::Refused {
            name: C::NAME.to_owned(),
        }),
    }
}

/// Catches a panic inside a lane callback.
///
/// The table functions of a publisher are all `extern "C"`, and a panic crossing an
/// `extern "C"` boundary is undefined behavior.
/// This belongs on the first line of every table function. What runs inside is this mod's
/// own business code, no less likely to panic than anywhere else, while the consequence
/// here is far worse: the caller is another mod and its frames are on the stack.
///
/// On a panic it returns `fallback` and logs. `fallback` must be the value this table
/// function uses for cannot-answer and not for no: treating a panic as a definite negative
/// answer lets a bug make a decision that belongs to business logic.
pub fn guard<R>(fallback: R, f: impl FnOnce() -> R) -> R {
    match std::panic::catch_unwind(std::panic::AssertUnwindSafe(f)) {
        Ok(v) => v,
        Err(_) => {
            Logger::get().error("a lane table function panicked. It was caught here and the fallback value is returned.");
            fallback
        }
    }
}

/// One publication. Dropping it withdraws the lane.
pub struct Publication {
    id: u64,
    name: String,
}

impl Publication {
    pub fn id(&self) -> u64 {
        self.id
    }

    pub fn forget(mut self) {
        self.id = 0;
    }
}

impl Drop for Publication {
    fn drop(&mut self) {
        if self.id == 0 {
            return;
        }
        if !crate::has_slot!(lane_unpublish) {
            return;
        }
        let Some(f) = crate::__rt::api().lane_unpublish else {
            return;
        };
        if !unsafe { f(rt().handle(), self.id) } {
            Logger::get().error(&format!("withdrawing lane `{}` failed.", self.name));
        }
    }
}

/// Publishes a lane.
///
/// # Safety
///
/// The caller must guarantee that:
///
/// * `data` and `vtable` stay valid until this lane is withdrawn. The host interprets not
///   one byte of them and copies nothing, and only compares for equality and marks
///   liveness;
/// * `vtable` really points at the C-layout table `C::Table` and its shape agrees with
///   `C::FINGERPRINT`;
/// * `retain` and `release` call back into no `lane_*` slot, which would self-deadlock.
pub unsafe fn publish<C: LaneContract>(
    data: *mut c_void,
    vtable: *const C::Table,
    retain: Option<sys::PierLaneRefFn>,
    release: Option<sys::PierLaneRefFn>,
) -> Result<Publication> {
    let f = crate::require_slot!(lane_publish, "publishing a fast lane");
    if C::FINGERPRINT == 0 {
        return Err(Error(format!(
            "the fingerprint of lane `{}` is 0, which is reserved to mean anyone may connect, and the ABI refuses it",
            C::NAME
        )));
    }
    let desc = sys::PierLaneDesc {
        struct_size: core::mem::size_of::<sys::PierLaneDesc>() as u32,
        protocol: sys::PIER_LANE_PROTOCOL,
        fingerprint: C::FINGERPRINT,
        data,
        vtable: vtable as *const c_void,
        retain,
        release,
    };
    let id = f(rt().handle(), s(C::NAME), &desc as *const sys::PierLaneDesc);
    if id == 0 {
        return Err(Error(format!(
            "publishing lane `{}` failed because the name is taken; the host log names the holder",
            C::NAME
        )));
    }
    Ok(Publication {
        id,
        name: C::NAME.to_owned(),
    })
}

/// The registration record of a published lane.
#[derive(Debug, Clone, PartialEq, Eq, serde::Deserialize)]
pub struct LaneInfo {
    pub name: String,
    #[serde(rename = "mod")]
    #[serde(default)]
    pub owner: String,
    /// The host gives it as a `"0x..."` string.
    #[serde(default)]
    pub fingerprint: String,
    #[serde(default)]
    pub protocol: u32,
    #[serde(default)]
    pub leases: u32,
    #[serde(default)]
    pub alive: bool,
}

/// Every lane. It passes no pointer at all, so it suits looking at what exists first.
pub fn list() -> Vec<LaneInfo> {
    if !crate::has_slot!(lane_list) {
        return Vec::new();
    }
    let Some(f) = crate::__rt::api().lane_list else {
        return Vec::new();
    };
    let Some(text) = call_out_str(|ctx, sink| {
        unsafe { f(ctx, sink) };
        true
    }) else {
        return Vec::new();
    };
    if text.trim().is_empty() {
        return Vec::new();
    }
    serde_json::from_str(&text).unwrap_or_else(|e| {
        Logger::get().warn(&format!("parsing the lane listing failed: {e}"));
        Vec::new()
    })
}

// The marshalling types both sides of a lane share. Every parameter type in the table
// must be `#[repr(C)]` with identical layout on both sides, so `&str`, `Vec<T>` and
// `String` are out: their layout is a Rust-internal convention.
//
// Shaped like `PierStr` and deliberately a different type: compatibility for that one is
// governed by `struct_size` while a lane is governed by `LaneContract::FINGERPRINT`.
// Mixing them suggests that editing abi.h could affect a lane.

/// A span of UTF-8 text passed over a lane.
///
/// The producer decides how long the pointer stays valid, usually only for the duration of
/// one call (contract §3). A receiver keeping it has to copy it out.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct LaneStr {
    pub ptr: *const u8,
    pub len: usize,
}

impl LaneStr {
    pub const EMPTY: LaneStr = LaneStr {
        ptr: core::ptr::null(),
        len: 0,
    };

    /// Borrows a `&str`. The lifetime of the result is not tracked by the type system, which
    /// is the cost of crossing an ABI boundary, so it belongs only where it is constructed and
    /// passed in immediately.
    pub fn new(s: &str) -> LaneStr {
        LaneStr {
            ptr: s.as_ptr(),
            len: s.len(),
        }
    }

    /// Reads it as a `&str`.
    ///
    /// Content that is not valid UTF-8 returns `None` rather than going through
    /// `from_utf8_unchecked`: the other side may be written in another language whose strings
    /// do not necessarily pass this test.
    ///
    /// # Safety
    /// `ptr` and `len` must describe memory that is still valid now.
    pub unsafe fn try_as_str<'a>(&self) -> Option<&'a str> {
        if self.ptr.is_null() {
            return Some("");
        }
        core::str::from_utf8(core::slice::from_raw_parts(self.ptr, self.len)).ok()
    }
}

/// A contiguous span of same-typed elements passed over a lane.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct LaneSlice<T> {
    pub ptr: *const T,
    pub len: usize,
}

impl<T> LaneSlice<T> {
    pub fn new(v: &[T]) -> LaneSlice<T> {
        LaneSlice {
            ptr: v.as_ptr(),
            len: v.len(),
        }
    }

    /// # Safety
    /// `ptr` and `len` must describe memory that is still valid now and really holds `T`.
    pub unsafe fn as_slice<'a>(&self) -> &'a [T] {
        if self.ptr.is_null() {
            return &[];
        }
        core::slice::from_raw_parts(self.ptr, self.len)
    }
}

/// The opaque context pointer of a lane function.
///
/// It wraps rather than using `*mut c_void` directly so that a table signature in the
/// contract reads as this parameter being that side's self.
#[repr(transparent)]
#[derive(Clone, Copy)]
pub struct LaneData(pub *mut c_void);

/// The callback shape where a producer sinks one entry and a receiver copies it out.
pub type LaneStrSink = unsafe extern "C" fn(ctx: *mut c_void, item: LaneStr);

/// Collects a lane call going through [`LaneStrSink`] into a `Vec<String>`.
///
/// An entry that is not UTF-8 is skipped with a warning rather than voiding the whole
/// batch: the other side may be written in another language, and one bad entry should not
/// make the whole query unanswerable.
pub fn collect_strings(call: impl FnOnce(*mut c_void, LaneStrSink) -> u32) -> Vec<String> {
    let mut out: Vec<String> = Vec::new();
    /// # Safety
    /// `ctx` must be a valid `*mut Vec<String>`.
    unsafe extern "C" fn sink(ctx: *mut c_void, item: LaneStr) {
        let v = &mut *ctx.cast::<Vec<String>>();
        match item.try_as_str() {
            Some(s) => v.push(s.to_owned()),
            None => Logger::get()
                .warn("a lane returned a string that is not valid UTF-8; it was skipped"),
        }
    }
    call((&mut out as *mut Vec<String>).cast(), sink);
    out
}

/// The raw JSON of the lane listing, unparsed.
///
/// For when [`list`] cannot parse it, or the host added a field this layer does not know.
pub fn list_json() -> String {
    if !crate::has_slot!(lane_list) {
        return String::new();
    }
    let Some(f) = crate::__rt::api().lane_list else {
        return String::new();
    };
    call_out_str(|ctx, sink| {
        unsafe { f(ctx, sink) };
        true
    })
    .unwrap_or_default()
}
