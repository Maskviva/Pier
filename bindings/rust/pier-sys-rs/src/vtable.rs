//! 模组交给宿主的那张表，以及入口符号的签名。
//!
//! 这是握手的**模组侧**。宿主侧那半在 `pier-host/src/ModHost.cpp`，
//! 顺序有讲究：先读长度、再读版本、最后比对目标标志。

use core::ffi::c_void;

use crate::api::PierApi;
use crate::types::PierModHandle;

/// 模组在 `pier_main` 里填好、交给宿主的表。
///
/// 自带 `struct_size`，所以 vtable 也走「只追加」那条路：宿主只读模组声明
/// 长度以内的字段，加一个回调不必升 ABI 版本、不必把已编译的模组判死。
///
/// 目标匹配走独立的 `mod_flags`，不往版本号高位藏标记 —— 挤在一个数里的话
/// 每次判断都要先解包，而且那种标记护不住**没有重编**的模组。
///
/// 宿主拿 `mod_flags` 和 `PierApi::host_flags` 的 bit 0 比对，不匹配就
/// **明确拒绝装载**并说明原因。明确是关键：让它装上去，然后在第一个只有
/// 一侧才有的槽位上崩掉，是查不下去的。
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
