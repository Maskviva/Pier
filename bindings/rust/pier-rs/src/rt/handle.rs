//! A raw handle that can be moved across threads.
//!
//! It uses an `AtomicPtr` rather than an `unsafe impl Send + Sync` pair on a
//! `*mut c_void`, because the latter asserts more than is needed: it claims the pointee
//! itself is thread safe while all that is needed is that the pointer value can be moved
//! across threads.
//!
//! Thread safety of the pointee belongs to the host: the slots the ABI marks thread safe
//! already allow a call from any thread while the rest may be called on the server thread
//! only under contract §4, and that is the caller's obligation and not something this type
//! can guarantee. Every real use is still written inside an explicit `unsafe`, so a reader
//! sees that boundary.

use core::ffi::c_void;
use core::sync::atomic::{AtomicPtr, Ordering};

#[repr(transparent)]
pub(crate) struct Handle(AtomicPtr<c_void>);

impl Handle {
    pub(crate) const fn new(p: *mut c_void) -> Handle {
        Handle(AtomicPtr::new(p))
    }

    /// `Relaxed` suffices: this pointer never changes after `set_runtime` and there is no
    /// other write that needs a happens-before relationship with it.
    pub(crate) fn get(&self) -> *mut c_void {
        self.0.load(Ordering::Relaxed)
    }

    #[allow(dead_code)]
    pub(crate) fn is_null(&self) -> bool {
        self.get().is_null()
    }
}
