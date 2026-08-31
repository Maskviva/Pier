//! 进程内唯一的运行时状态，以及**两道闸**的实现。
//!
//! # 调用一个非核心槽之前要过两道闸
//!
//! ```text
//!   闸一：表够不够长      struct_size >= offset + size_of::<fn ptr>()
//!   闸二：这个槽非不非空  api.field.is_some()
//! ```
//!
//! 两道缺一不可，而且**顺序不能反**：
//!
//! * 只查非空 → 宿主比模组老、表短到够不着这个字段时，`api.field` 读的是
//!   宿主没有分配的内存。那不是「读到一个 None」，那是**越界读**，
//!   读到什么完全看运气，而运气好的时候它看起来像个合法函数指针。
//! * 只查长度 → 表够长但那个能力包没编进宿主（`pier-dimensions` 没编入时
//!   `md_*` 全是 NULL），调用一个空指针。
//!
//! `require_slot!` 把两道闸包在一起，返回一个说得清「缺哪个函数」的 Err。

use std::sync::atomic::{AtomicPtr, Ordering};

use crate::rt::handle::Handle;
use crate::sys;

pub(crate) struct Runtime {
    pub(crate) api: &'static sys::PierApi,
    handle: Handle,
    /// 宿主自报的表长度。前向兼容的全部依据（契约 §2.2）。
    pub(crate) host_struct_size: usize,
}

/// V-43：以前是 `OnceLock`——只能设一次。`/pier reload` 不卸映像、重跑
/// `pier_main`，第二次 `set` 必然失败，于是 pier-rs 模组根本不能 reload，
/// 而且失败时 SDK 一行日志都打不出来。现在允许覆盖：每次 `pier_main` 都拿
/// 到新的 `PierModHandle`（旧 HostedMod 已析构，旧句柄悬空），必须换掉。
/// 旧的 `Runtime` 刻意泄漏（每次 reload 几十字节）：仍在跑的回调可能还持有
/// 指向它的 `&'static`，释放才是真正的 use-after-free。
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

/// 闸一：宿主的表覆盖到这个偏移了吗。
///
/// `size` 固定按函数指针算 —— `PierApi` 里除了开头四个 `u32` 之外全是函数
/// 指针，而那四个在任何宿主上都存在。
#[inline]
pub fn has_slot(offset: usize) -> bool {
    rt().host_struct_size >= offset + core::mem::size_of::<usize>()
}

impl Runtime {
    pub(crate) fn handle(&self) -> sys::PierModHandle {
        self.handle.get()
    }
}

/// 宿主的函数表。`require_slot!` / `has_slot!` 展开后要用，所以是 pub。
#[inline]
pub fn api() -> &'static sys::PierApi {
    rt().api
}

/// 诊断用：宿主的 ABI 版本与表长度。
#[inline]
pub fn host_abi() -> (u32, usize) {
    let r = rt();
    (r.api.abi_version, r.host_struct_size)
}

pub(crate) fn rt() -> &'static Runtime {
    let p = RUNTIME.load(Ordering::Acquire);
    if p.is_null() {
        panic!("Pier 运行时尚未初始化 —— 是不是漏了 register_mod!？");
    }
    // SAFETY：指针来自 Box::into_raw 且永不释放（见 RUNTIME 的注释）。
    unsafe { &*p }
}

/// 交给模组生命周期回调的上下文。
///
/// 它本身不带状态（真正的状态在 `RUNTIME` 里），存在的意义是给各个门面
/// 一个统一的入口，顺便让 `on_load(ctx)` 这样的签名读起来像回事。
pub struct ModContext(());

impl ModContext {
    pub(crate) fn new() -> ModContext {
        ModContext(())
    }

    pub fn logger(&self) -> crate::Logger {
        crate::Logger::get()
    }

    /// 宿主与系统层面的能力（运行阶段、排期、执行命令、协议版本…）。
    pub fn host(&self) -> crate::Host {
        crate::Host::get()
    }

    /// 数据包门面。等价于 `ctx.host().packets()`，写起来短一截。
    pub fn packets(&self) -> crate::packet::Packets {
        crate::packet::Packets::get()
    }

    /// 宿主是按客户端目标编的吗。
    ///
    /// 一般用不到 —— 装错目标的模组在握手阶段就被宿主拒绝了。留着是为了让
    /// 同一份代码能在两个目标上做细微的行为区分，而不必靠编译期 feature。
    pub fn host_is_client(&self) -> bool {
        rt().api.is_client_host()
    }

    /// 宿主的 ABI 版本与表长度。诊断用：报「这个功能你的 pier 太老」时，
    /// 带上这两个数才能让人知道该升到多少。
    pub fn host_abi(&self) -> (u32, usize) {
        let r = rt();
        (r.api.abi_version, r.host_struct_size)
    }
}

/// 一个已排期任务的票据。
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

/// 两道闸，缺任何一道就返回一个说得清缺什么的 `Err`。
///
/// 用法：函数体开头 `require_slot!(md_add_plot_dimension, "创建地皮维度");`
///
/// 报错文案里**不出现任何历史产品名**（契约 §七）。v0 那条是
/// "…Update levilamina-rust-loader"，而那个名字的模组已经不存在了 ——
/// 照它去更新的人会找不到东西，这正是 §5.3「日志要能回答我该做什么」
/// 反对的形状。
#[macro_export]
macro_rules! require_slot {
    ($field:ident, $what:expr) => {{
        let __off = core::mem::offset_of!($crate::sys::PierApi, $field);
        if !$crate::__rt::has_slot(__off) {
            let (__ver, __len) = $crate::__rt::host_abi();
            return Err($crate::Error(format!(
                "{} 需要宿主提供 `{}`，而这个 pier 宿主的函数表里还没有这个槽\
                 （宿主 ABI v{}，表长 {} 字节）。请升级 pier。",
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
                    "{} 需要宿主提供 `{}`，槽位存在但是空的 —— 说明对应的能力包\
                     没有编进这个 pier 宿主。",
                    $what,
                    stringify!($field)
                )))
            }
        }
    }};
}

/// 只问「有没有」，不返回 Err。用在「有就用、没有就降级」的地方。
///
/// 同样是两道闸：够长**且**非空才算有。
#[macro_export]
macro_rules! has_slot {
    ($field:ident) => {
        $crate::__rt::has_slot(core::mem::offset_of!($crate::sys::PierApi, $field))
            && $crate::__rt::api().$field.is_some()
    };
}
