//! 模组交给宿主的那张表，以及入口符号的签名。
//!
//! 这是握手的**模组侧**。宿主侧那半在 `pier-host/src/ModHost.cpp`，
//! 顺序有讲究：先读长度、再读版本、最后比对目标标志。

use core::ffi::c_void;

use crate::api::PierApi;
use crate::types::PierModHandle;

/// 模组在 `pier_main` 里填好、交给宿主的表。
///
/// # 为什么 v1 起自带 `struct_size`
///
/// v0 的 vtable 只有 `abi_version` 加三个回调，长度是隐含的。后果是它**永远
/// 不能追加字段** —— 加一个 `on_tick` 就得升 ABI 版本，而升版本会把所有已
/// 编译的模组一起判死。现在宿主只读模组声明长度以内的字段，vtable 从此也能
/// 走「只追加」那条路。
///
/// # 目标匹配走 `mod_flags`，不再往版本号高位藏标记
///
/// v0 用 `PIER_ABI_TARGET_MASK = 0x8000_0000` 把「这是客户端模组」塞进版本号
/// 的最高位。两个问题：版本号从此不是一个数而是两个字段挤在一起；而且这个
/// 标记护不住**没有重编**的模组 —— 它编译时那一位就已经定死了。
///
/// 现在是一个独立的 `mod_flags`，宿主拿它和 `PierApi::host_flags` 的 bit 0
/// 比对，不匹配就明确拒绝装载并说明原因（服务端宿主装不了按客户端编的模组，
/// 反之亦然）。**明确拒绝**是关键：v0 那条路会让不匹配的模组装上去，然后在
/// 第一个只有一侧才有的槽位上崩掉。
#[repr(C)]
pub struct PierModVTable {
    /// `size_of::<PierModVTable>()`，由模组按自己编译出的表填写。
    pub struct_size: u32,
    /// 模组编译时的 `PIER_ABI_VERSION`。
    pub abi_version: u32,
    /// `PIER_FLAG_*` 的按位或。bit 0 = 这是客户端模组。
    pub mod_flags: u32,
    /// 保留，恒为 0。凑齐 16 字节头，也给未来的头部标量留位。
    pub _reserved0: u32,
    /// 模组自己的状态指针。宿主原样回传给三个回调，不解释。
    pub instance: *mut c_void,
    pub on_enable: Option<unsafe extern "C" fn(instance: *mut c_void) -> bool>,
    pub on_disable: Option<unsafe extern "C" fn(instance: *mut c_void) -> bool>,
    pub on_unload: Option<unsafe extern "C" fn(instance: *mut c_void) -> bool>,
}

/// 模组必须导出的唯一入口。宿主只找 `pier_main` 这一个名字，找不到就明确
/// 拒绝装载、不做任何回退（契约 §2.4）。
pub type PierMainFn = unsafe extern "C" fn(
    api: *const PierApi,
    self_: PierModHandle,
    out_vtable: *mut PierModVTable,
) -> bool;
