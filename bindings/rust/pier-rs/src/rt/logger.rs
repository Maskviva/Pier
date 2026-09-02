//! A mod's own logging entry point.
//!
//! It goes through `PierApi::log` and the host attaches it to the LeviLamina logger under
//! this mod's name, so what appears in the console is `[the mod name] ...` and not
//! `[pier] ...`.
//!
//! `log` is one of the few slots the ABI marks thread safe (contract §4), so a `Logger` can
//! be `Copy`ed onto any thread freely.

use crate::rt::ffi::s;
use crate::rt::runtime::rt;

/// Mirrors `ll::io::LogLevel`. The values are part of the ABI.
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
        // `log` is a core slot every host has: it sits first after the header, any host that
        // passes the version gate covers it, and require_slot! is therefore not used here.
        // It is still checked for non-null: a host that left the table incomplete should not
        // turn logging into a crash, and logging is the one channel that can say something went
        // wrong.
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
