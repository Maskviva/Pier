//! 这一层唯一的错误类型。
//!
//! 只有一个 `String` 而不是一棵错误枚举树，是刻意的：跨 ABI 边界回来的失败
//! 信息**本来就是一句人话**（宿主 sink 出来的一行字），把它拆成枚举需要在
//! 两侧维护一份错误码表，而那张表的每一次漂移都是静默的。
//!
//! 代价是调用方不能按类型分支处理。目前没有哪个下游需要 —— 真需要了再加
//! 枚举，但那时候错误码必须先进 `abi.h`（契约 §〇：ABI 是产品）。

/// 一次 ABI 调用失败的原因。消息面向**人**，可以直接打进日志。
#[derive(Debug)]
pub struct Error(pub String);

impl core::fmt::Display for Error {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        f.write_str(&self.0)
    }
}

impl std::error::Error for Error {}

impl Error {
    pub fn new(msg: impl core::fmt::Display) -> Self {
        Error(msg.to_string())
    }
}

impl From<String> for Error {
    fn from(s: String) -> Self {
        Error(s)
    }
}

impl From<&str> for Error {
    fn from(s: &str) -> Self {
        Error(s.to_owned())
    }
}

pub type Result<T> = core::result::Result<T, Error>;
