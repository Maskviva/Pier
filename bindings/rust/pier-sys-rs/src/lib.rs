//! `levilamina_sys` —— Pier ABI v1 的裸 FFI 声明。
//!
//! 这个 crate 是 `packages/pier-abi/include/sdk/abi.h` 的逐格镜像，
//! 也是契约 §十「加一门语言」那四步的参考实现：
//!
//! 1. 只读 `sdk/abi.h`；
//! 2. 声明 `PierApi` 镜像 —— **无条件声明每一个字段**，没有任何目标分支；
//! 3. 导出 `pier_main`，vtable 填 `struct_size / abi_version / mod_flags`
//!    和三个生命周期回调；
//! 4. 每个非核心槽调用前查 `struct_size` 覆盖且槽非 NULL。
//!
//! 换一门语言照着这四步做，而不是照抄这个 crate —— 契约是那份头文件。
//!
//! 这一层**不提供安全性**：每个调用都是 `unsafe`，字符串生命周期靠调用方
//! 保证。安全封装在 `pier-rs`（crate 名 `levilamina`），分层理由见 `types.rs`。

#![no_std]
#![allow(non_camel_case_types)]

pub mod api;
pub mod consts;
pub mod types;
pub mod vtable;

pub use api::PierApi;
pub use consts::*;
pub use types::*;
pub use vtable::{PierMainFn, PierModVTable};

/// 本 crate 编译时对着的 ABI 版本。填进 `PierModVTable::abi_version`。
pub const PIER_ABI_VERSION: u32 = 1;

/// 宿主可接受的最老模组 ABI。兼容是**区间**不是相等：
/// `MIN_SUPPORTED <= mod_abi <= VERSION`（契约 §2.2）。
///
/// 镜像里也留一份，是为了让模组能在装载失败时自己说清楚「我是按 v{N} 编的，
/// 这个宿主要 v{A}..v{B}」，而不是只报一句「版本不符」。
pub const PIER_ABI_MIN_SUPPORTED: u32 = 1;

/// 模组必须导出的入口符号名。
pub const PIER_MAIN_SYMBOL: &str = "pier_main";

/// `host_flags` / `mod_flags` 的 bit 0：这一侧是客户端构建。
///
/// 两侧的 bit 0 必须相等，否则宿主拒绝装载并说明原因。其余位保留，必须为 0。
pub const PIER_FLAG_CLIENT: u32 = 0x1;

// ── 跨模组服务调用的结果码 ────────────────────────────────────────
//
// 注意 `OK` 是 **0**。这一点对错误处理有实际后果：一个「出错就返回零值」的
// 兜底会把失败报成成功，那正是契约 §5.1 反对的形状。C++ 侧的
// `PIER_API_GUARD_END_VAL` 因此对这一族入口必须显式给失败值。

pub const PIER_SERVICE_OK: i32 = 0;
/// 没人提供这个名字（没装那个模组，或者它被禁用/已卸载）。
pub const PIER_SERVICE_NOT_FOUND: i32 = 1;
/// 提供方跑了但返回失败，`reply` 里是它的错误信息。
pub const PIER_SERVICE_ERROR: i32 = 2;
/// 名字非法、自己调自己、或者调用深度超限。
pub const PIER_SERVICE_REFUSED: i32 = 3;

// ── 同工具链快车道 ────────────────────────────────────────────────

/// 车道协议版本。`PierLaneDesc::protocol` 必须等于它，否则发布被拒。
pub const PIER_LANE_PROTOCOL: u32 = 1;

pub const PIER_LANE_OK: i32 = 0;
/// 没人发布这个名字（没装那个模组）。
pub const PIER_LANE_NOT_FOUND: i32 = 1;
/// 发布了，但指纹不同 —— 降级走 `service_call`，**别递指针**。
pub const PIER_LANE_FINGERPRINT: i32 = 2;
/// 名字非法 / 自取 / 提供方被禁用 / 协议不符。
pub const PIER_LANE_REFUSED: i32 = 3;

// ── 数据包方向与处置 ──────────────────────────────────────────────

pub const PIER_PKT_INBOUND: i32 = 0;
pub const PIER_PKT_OUTBOUND: i32 = 1;
pub const PIER_PKT_MASK_INBOUND: i32 = 1 << PIER_PKT_INBOUND;
pub const PIER_PKT_MASK_OUTBOUND: i32 = 1 << PIER_PKT_OUTBOUND;

/// 原样转发，`replace` 的输出被忽略。
pub const PIER_PKT_PASS: i32 = 0;
/// 转发交给 `replace` 的那份包体。
pub const PIER_PKT_REPLACE: i32 = 1;
/// 整个吃掉这个包。
pub const PIER_PKT_DROP: i32 = 2;
