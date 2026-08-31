//! ABI 里的整数常量族，按主题分三个模块。
//!
//! 分模块只是为了方便读，**不是**语义边界 —— 全部经 `pub use` 平铺到
//! `levilamina_sys::` 下，调用方不需要知道 `PIER_PPROP_HEALTH` 住在哪个文件。
//!
//! 三个模块的一致性由 `sys-mirrors-abi` 机检守着：`abi.h` 里每一个 `enum`
//! 成员，这里都要有同名同值的常量。这条检查是被现实逼出来的 ——
//! `PIER_AACT_ADD_EFFECT` 因为在 `abi.h` 里的注释换了行，人工搬运时被整条
//! 漏掉了，而漏一个常量的症状是「调 add_effect 实际执行了 remove_effect」。

pub mod actor;
pub mod player;
pub mod world;

pub use actor::*;
pub use player::*;
pub use world::*;
