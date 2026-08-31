//! 跨线程可搬运的裸句柄。
//!
//! 用 `AtomicPtr` 而不是给 `*mut c_void` 写一对 `unsafe impl Send + Sync`：
//! 后者**断言的东西比需要的多** —— 它声称被指对象本身线程安全，而这里只需要
//! 「指针这个值可以跨线程搬」。
//!
//! 被指对象的线程安全归宿主：ABI 标了线程安全的那几个槽本就允许任意线程
//! 调用，其余的按契约 §四 只能在服务器线程调，而那是**调用方的义务**，
//! 不是这个类型能保证的东西。每一次真正使用仍然写在显式的 `unsafe` 里，
//! 读代码的人看得见那道边界。

use core::ffi::c_void;
use core::sync::atomic::{AtomicPtr, Ordering};

#[repr(transparent)]
pub(crate) struct Handle(AtomicPtr<c_void>);

impl Handle {
    pub(crate) const fn new(p: *mut c_void) -> Handle {
        Handle(AtomicPtr::new(p))
    }

    /// `Relaxed` 就够：这个指针在 `set_runtime` 之后再也不变，
    /// 没有需要和它建立 happens-before 的其它写入。
    pub(crate) fn get(&self) -> *mut c_void {
        self.0.load(Ordering::Relaxed)
    }

    #[allow(dead_code)]
    pub(crate) fn is_null(&self) -> bool {
        self.get().is_null()
    }
}
