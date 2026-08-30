//! 跨线程可搬运的裸句柄。
//!
//! # 为什么不是 `unsafe impl Send + Sync`
//!
//! v0 的 `Runtime` / `KvDb` / `PacketHook` 各自写了一对 `unsafe impl` 来让
//! `*mut c_void` 过编译。那两行**断言的东西比需要的多**：它们声称被指对象
//! 本身线程安全，而我们其实只需要「指针这个值可以跨线程搬」。
//!
//! `AtomicPtr` 恰好只提供后者。被指对象的线程安全由宿主那一侧负责 ——
//! ABI 上标了线程安全的那几个槽（`log` / `gaming_status` / `schedule` …）
//! 本就允许任意线程调用，其余的按契约 §四 只能在服务器线程调，而那一条
//! 是**调用方的义务**，不是这个类型能保证的东西。
//!
//! 差别在什么时候显形：有人把一个只能在服务器线程用的句柄丢进
//! `std::thread::spawn`。`unsafe impl Sync` 会让它编过；`AtomicPtr` 也会 ——
//! 但每一次真正使用仍然写在显式的 `unsafe { (api.xxx)(handle.get(), ..) }`
//! 里，读代码的人看得见那道边界。这是「少断言一点」能换到的全部，不多，
//! 但方向是对的。

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
