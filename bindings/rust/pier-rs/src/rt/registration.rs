//! The mod-side handshake and the lifecycle fence.
//!
//! # The handshake: three checks, in a deliberate order
//!
//! It has to match the host side in `pier-host/src/ModHost.cpp`:
//!
//! 1. Length. With a table too short to cover even the core slots, the other two cannot be
//!    spoken of, since reading the version number is itself reading that memory.
//! 2. The version range, `MIN_SUPPORTED <= ours <= the host`. Compatibility is a range and
//!    not an equality (§2.2).
//! 3. The target flags: bit 0 of `host_flags` and of `mod_flags` must be equal.
//!
//! All three lifecycle callbacks sit inside a `catch_unwind`: a panic crossing
//! `extern "C"` is undefined behavior, so it is caught into a `false` plus a log line and
//! the host takes its normal rollback path.

use std::panic::{catch_unwind, AssertUnwindSafe};
use std::sync::Mutex;

use crate::context::ModContext;
use crate::rt::error::{Error, Result};
use crate::rt::logger::Logger;
use crate::sys;

/// One Pier mod.
///
/// `Send` is what makes `ModSlot<T>`, a `Mutex<Option<T>>`, genuinely `Sync`: a lifecycle
/// callback may be entered on a different thread, since the host allows `unload` and a
/// cross-mod service call to come from another thread.
pub trait LeviMod: Sized + Send + 'static {
    /// Load. An `Err` makes the host treat loading as failed and roll back, with the teardown
    /// steps running as usual.
    fn on_load(ctx: &ModContext) -> Result<Self>;

    fn on_enable(&mut self, _ctx: &ModContext) -> Result<()> {
        Ok(())
    }
    fn on_disable(&mut self, _ctx: &ModContext) -> Result<()> {
        Ok(())
    }
    fn on_unload(&mut self, _ctx: &ModContext) -> Result<()> {
        Ok(())
    }
}

#[doc(hidden)]
pub struct ModSlot<T: LeviMod>(pub Mutex<Option<T>>);

/// The handshake. It returns true on success and only false on failure, never panicking.
///
/// A failure logs as clearly as it can, but note that logging itself goes through
/// `api.log` while this is precisely where whether that api can be trusted is being
/// decided. It is therefore used only after the length check has passed, and an earlier
/// failure can only be reported by the host, which prints that the vtable of the mod
/// declares an insufficient length.
///
/// # Safety
/// The `pier_main` call `register_mod!` expands to, where `api` comes from the host.
#[doc(hidden)]
pub unsafe fn __init_runtime(api: *const sys::PierApi, handle: sys::PierModHandle) -> bool {
    if api.is_null() {
        return false;
    }
    let api: &'static sys::PierApi = &*api;

    // Gate one: length
    //
    // The criterion is the four header scalars plus the core slots really used
    // unconditionally. It takes the offset of `log` plus one pointer width: once log is
    // covered, every later failure can speak.
    let core_min = core::mem::offset_of!(sys::PierApi, log) + core::mem::size_of::<usize>();
    if (api.struct_size as usize) < core_min {
        return false;
    }

    // Gate two: the version range
    //
    // A host newer than the mod is allowed, with a longer table whose unreachable part is
    // left alone. A host older than the version this crate was compiled against means the
    // table layout may no longer be a prefix of ours.
    if api.abi_version < sys::PIER_ABI_MIN_SUPPORTED {
        return false;
    }
    if sys::PIER_ABI_VERSION > api.abi_version {
        return false;
    }

    // Gate three: the target flags
    //
    // The host checks this too, against the mod_flags in the vtable. Checking on both
    // sides is deliberate: checking here lets the mod say so in its own log, and checking
    // there is the authoritative decision, since a malicious or mistaken mod may skip its
    // check while the host may not.
    if (api.host_flags & sys::PIER_FLAG_CLIENT) != mod_flags() & sys::PIER_FLAG_CLIENT {
        return false;
    }

    crate::rt::runtime::set_runtime(api, handle)
}

/// The `mod_flags` of this build.
///
/// This is the only effect the `client` feature has in v1: it adds and removes no API
/// (contract §2.1, the layout is identical on every target).
#[doc(hidden)]
pub const fn mod_flags() -> u32 {
    #[cfg(feature = "client")]
    {
        sys::PIER_FLAG_CLIENT
    }
    #[cfg(not(feature = "client"))]
    {
        0
    }
}

#[doc(hidden)]
pub fn __lifecycle<T: LeviMod>(slot: &'static ModSlot<T>, stage: u8) -> bool {
    let ctx = ModContext::new();
    let run = || -> Result<()> {
        let mut guard = slot
            .0
            .lock()
            .map_err(|_| Error("the mod state is poisoned, because a previous callback panicked".into()))?;
        let Some(instance) = guard.as_mut() else {
            return Err(Error("there is no mod instance; is this a repeated unload?".into()));
        };
        match stage {
            1 => instance.on_enable(&ctx),
            2 => instance.on_disable(&ctx),
            3 => {
                instance.on_unload(&ctx)?;
                // on_unload runs before the instance is dropped. The other order would leave the
                // cleanup a user does with &mut self inside on_unload nowhere to happen.
                *guard = None;
                Ok(())
            }
            _ => Ok(()),
        }
    };
    match catch_unwind(AssertUnwindSafe(run)) {
        Ok(Ok(())) => true,
        Ok(Err(e)) => {
            Logger::get().error(&format!("the lifecycle callback at stage {stage} failed: {e}"));
            false
        }
        Err(_) => {
            Logger::get().error(&format!(
                "the lifecycle callback at stage {stage} panicked. It was caught here, since a \
                 panic crossing an extern \"C\" boundary is undefined behavior."
            ));
            false
        }
    }
}

#[doc(hidden)]
pub fn __load<T: LeviMod>(slot: &'static ModSlot<T>) -> bool {
    let ctx = ModContext::new();
    match catch_unwind(AssertUnwindSafe(|| T::on_load(&ctx))) {
        Ok(Ok(instance)) => {
            // `lock()` cannot be poisoned here, since this is the first touch of that lock.
            match slot.0.lock() {
                Ok(mut g) => {
                    *g = Some(instance);
                    true
                }
                Err(_) => {
                    Logger::get().error("the mod state slot was already poisoned at load time, so loading is treated as failed");
                    false
                }
            }
        }
        Ok(Err(e)) => {
            Logger::get().error(&format!("on_load failed: {e}"));
            false
        }
        Err(_) => {
            Logger::get().error("on_load panicked. It was caught here.");
            false
        }
    }
}

/// Generates the `pier_main` entry point. Written once per mod.
///
/// ```ignore
/// struct MyMod;
/// impl LeviMod for MyMod { /* ... */ }
/// levilamina::register_mod!(MyMod);
/// ```
#[macro_export]
macro_rules! register_mod {
    ($ty:ty) => {
        #[doc(hidden)]
        static __PIER_SLOT: $crate::ModSlot<$ty> = $crate::ModSlot(::std::sync::Mutex::new(None));

        /// The host looks for this symbol alone and refuses to load explicitly when it is absent
        /// (contract §2.4).
        #[no_mangle]
        pub unsafe extern "C" fn pier_main(
            api: *const $crate::sys::PierApi,
            handle: $crate::sys::PierModHandle,
            out: *mut $crate::sys::PierModVTable,
        ) -> bool {
            if out.is_null() || !$crate::__init_runtime(api, handle) {
                return false;
            }
            if !$crate::__load::<$ty>(&__PIER_SLOT) {
                return false;
            }
            unsafe extern "C" fn on_enable(_: *mut ::core::ffi::c_void) -> bool {
                $crate::__lifecycle::<$ty>(&__PIER_SLOT, 1)
            }
            unsafe extern "C" fn on_disable(_: *mut ::core::ffi::c_void) -> bool {
                $crate::__lifecycle::<$ty>(&__PIER_SLOT, 2)
            }
            unsafe extern "C" fn on_unload(_: *mut ::core::ffi::c_void) -> bool {
                $crate::__lifecycle::<$ty>(&__PIER_SLOT, 3)
            }
            // `struct_size` is filled by the mod with the length it compiled, and the host reads
            // only the fields within it, so the vtable can also gain callbacks without an ABI
            // version bump.
            (*out) = $crate::sys::PierModVTable {
                struct_size: ::core::mem::size_of::<$crate::sys::PierModVTable>() as u32,
                abi_version: $crate::sys::PIER_ABI_VERSION,
                mod_flags: $crate::__rt::mod_flags(),
                _reserved0: 0,
                instance: ::core::ptr::null_mut(),
                on_enable: Some(on_enable),
                on_disable: Some(on_disable),
                on_unload: Some(on_unload),
            };
            true
        }
    };
}
