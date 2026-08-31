//! 模组自己的日志入口。
//!
//! 走的是 `PierApi::log`，宿主那边会挂到这个模组名下的 LeviLamina logger，
//! 所以玩家在控制台看到的是 `[你的模组名] ...` 而不是 `[pier] ...`。
//!
//! `log` 是 ABI 上明确标了**线程安全**的少数几个槽之一（契约 §四），
//! 所以 `Logger` 可以随便 `Copy` 到任何线程上用。

use crate::rt::ffi::s;
use crate::rt::runtime::rt;

/// 镜像 `ll::io::LogLevel`。数值是 ABI 的一部分。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LogLevel {
    Fatal = 0,
    Error = 1,
    Warn = 2,
    Info = 3,
    Debug = 4,
    Trace = 5,
}

#[derive(Clone, Copy)]
pub struct Logger(());

impl Logger {
    pub fn get() -> Logger {
        Logger(())
    }

    pub fn log(&self, level: LogLevel, msg: &str) {
        let rt = rt();
        // `log` 是核心槽，宿主一定有它 —— 它在表头之后的第一个位置，
        // 任何能通过版本闸的宿主都覆盖得到，所以这里不走 require_slot!。
        // 但仍然查一次非空：一个填表填漏了的宿主不该让日志变成崩溃，
        // 而日志恰恰是唯一能告诉人「出事了」的通道。
        if let Some(f) = rt.api.log {
            unsafe { f(rt.handle(), level as i32, s(msg)) }
        }
    }

    pub fn fatal(&self, msg: &str) {
        self.log(LogLevel::Fatal, msg);
    }
    pub fn error(&self, msg: &str) {
        self.log(LogLevel::Error, msg);
    }
    pub fn warn(&self, msg: &str) {
        self.log(LogLevel::Warn, msg);
    }
    pub fn info(&self, msg: &str) {
        self.log(LogLevel::Info, msg);
    }
    pub fn debug(&self, msg: &str) {
        self.log(LogLevel::Debug, msg);
    }
    pub fn trace(&self, msg: &str) {
        self.log(LogLevel::Trace, msg);
    }
}
