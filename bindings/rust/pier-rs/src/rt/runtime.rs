//! The single runtime state in the process, and the implementation of the two gates.
//!
//! ```text Gate one, is the table long enough: struct_size >= offset + size_of::<fn ptr>() Gate
//! two, is this slot non-null:      api.field.is_some() ```
//!
//! Neither may be skipped and the order cannot be reversed. Checking only for non-null, with a host
//! older than the mod and a table too short to reach the field, makes reading `api.field` an out-
//! of-bounds read whose result is luck, and on a lucky day it looks like a valid function pointer.
//! Checking only the length calls a null pointer when the table is long enough while the capability
//! package was not built into the host, as with `md_*` all NULL without `pier-dimensions`.
//!
//! `require_slot!` wraps both gates together; see contract §2.2.

use std::sync::atomic::{AtomicPtr, Ordering};

use crate::rt::handle::Handle;
use crate::sys;

pub(crate) struct Runtime {
    pub(crate) api: &'static sys::PierApi,
    handle: Handle,
    /// The table length the host reports. The whole basis of forward compatibility
    /// (contract §2.2).
    pub(crate) host_struct_size: usize,
}

/// A `OnceLock` can be set once only. `/pier reload` does not unload the image and runs
/// `pier_main` again, so a second `set` necessarily fails, a pier-rs mod cannot reload at
/// all, and the SDK prints not one log line on failure. Overwriting is therefore allowed:
/// every `pier_main` receives a new `PierModHandle`, since the old HostedMod is destroyed
/// and the old handle dangles, and it has to be replaced.
/// The old `Runtime` is leaked on purpose, a few dozen bytes per reload: a callback still
/// running may hold a `&'static` into it, and freeing it is the real use-after-free.
static RUNTIME: AtomicPtr<Runtime> = AtomicPtr::new(core::ptr::null_mut());

pub(crate) fn set_runtime(api: &'static sys::PierApi, handle: sys::PierModHandle) -> bool {
    let host_struct_size = api.struct_size as usize;
    let fresh = Box::into_raw(Box::new(Runtime {
        api,
        handle: Handle::new(handle),
        host_struct_size,
    }));
    RUNTIME.store(fresh, Ordering::Release);
    true
}

/// Gate one: does the host table reach this offset?
///
/// `size` is always computed as a function pointer: apart from the four leading `u32`,
/// everything in `PierApi` is a function pointer, and those four exist on every host.
#[inline]
pub fn has_slot(offset: usize) -> bool {
    rt().host_struct_size >= offset + core::mem::size_of::<usize>()
}

impl Runtime {
    pub(crate) fn handle(&self) -> sys::PierModHandle {
        self.handle.get()
    }
}

/// The host function table. `require_slot!` and `has_slot!` use it after expansion, so it
/// is public.
#[inline]
pub fn api() -> &'static sys::PierApi {
    rt().api
}

/// For diagnostics: the ABI version and table length of the host.
#[inline]
pub fn host_abi() -> (u32, usize) {
    let r = rt();
    (r.api.abi_version, r.host_struct_size)
}

pub(crate) fn rt() -> &'static Runtime {
    let p = RUNTIME.load(Ordering::Acquire);
    if p.is_null() {
        panic!("the Pier runtime is not initialized yet; is register_mod! missing?");
    }
    // SAFETY: the pointer comes from Box::into_raw and is never freed; see the comment on
    // RUNTIME.
    unsafe { &*p }
}

/// The ticket of a scheduled task.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct TaskId(pub(crate) u64);

impl TaskId {
    pub const NONE: TaskId = TaskId(0);

    pub fn is_valid(self) -> bool {
        self.0 != 0
    }

    pub fn raw(self) -> u64 {
        self.0
    }
}

/// Both gates. Missing either returns an `Err` that says what is missing.
///
/// Usage: at the top of a function body,
/// `require_slot!(md_add_plot_dimension, "creating a plot dimension");`
///
/// The message carries no historical product name (contract §7). An earlier one read
/// "...Update levilamina-rust-loader" while no mod of that name exists any more, so anyone
/// following it finds nothing, which is exactly the shape §5.3 opposes when it says a log
/// line has to answer what to do about it.
#[macro_export]
macro_rules! require_slot {
    ($field:ident, $what:expr) => {{
        let __off = core::mem::offset_of!($crate::sys::PierApi, $field);
        if !$crate::__rt::has_slot(__off) {
            let (__ver, __len) = $crate::__rt::host_abi();
            return Err($crate::Error(format!(
                "{} needs the host to provide `{}`, and the function table of this pier host does not \
                 have that slot yet (host ABI v{}, table length {} bytes). Upgrade pier.",
                $what,
                stringify!($field),
                __ver,
                __len
            )));
        }
        match $crate::__rt::api().$field {
            Some(f) => f,
            None => {
                return Err($crate::Error(format!(
                    "{} needs the host to provide `{}`. The slot exists and is empty, which means the \
                     matching capability package was not built into this pier host.",
                    $what,
                    stringify!($field)
                )))
            }
        }
    }};
}

/// Only asks whether it exists and returns no `Err`. For code that uses it when present
/// and degrades otherwise.
///
/// Both gates again: long enough and non-null counts as present.
#[macro_export]
macro_rules! has_slot {
    ($field:ident) => {
        $crate::__rt::has_slot(core::mem::offset_of!($crate::sys::PierApi, $field))
            && $crate::__rt::api().$field.is_some()
    };
}
