//! 运行时地基：握手、槽位闸、字符串收口、日志、panic 围栏。
//!
//! 这一层不认识任何游戏概念。域模块（`player` / `block` / …）全部建在它
//! 上面，而它只认识 `levilamina_sys`。

pub(crate) mod error;
pub(crate) mod ffi;
pub(crate) mod handle;
pub(crate) mod logger;
pub(crate) mod registration;
pub(crate) mod runtime;
