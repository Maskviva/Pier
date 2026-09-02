//! Economy.
//!
//! # The backend is delay-loaded, and the whole family degrades rather than crashing
//!
//! Without an economy backend installed, or with it disabled, each slot returns its own failure
//! value. This layer translates those into `Err`, with [`balance`] the one exception; see its own
//! documentation.
//!
//! # Amounts are never negative and a transfer is taxed
//!
//! The backend refuses a negative amount itself. [`transfer`] takes a cut according to the
//! `pay_tax` the backend is configured with, so the recipient receives `val - val * pay_tax` and
//! not `val`. Moving the full amount means separate [`add`] and [`reduce`] calls. The argument of
//! [`set`] is the target balance and not a delta.

use crate::rt::error::{Error, Result};
use crate::rt::ffi::{collect_strs, s};
use crate::rt::logger::Logger;
use crate::sys;

/// One economy event.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MoneyEvent {
    pub kind: sys::PierMoneyEvent,
    /// The xuid of the payer. An empty string means created out of nothing.
    pub from: String,
    /// The xuid of the recipient. An empty string means destroyed into nothing.
    pub to: String,
    pub value: i64,
}

/// The balance.
///
/// It returns `Err` and not -1: a real balance is never negative, so a negative value can
/// only mean the question cannot be answered, because the xuid is empty, the database
/// failed, or the backend is absent.
///
/// Note this read is not free of side effects: the backend opens an account at the
/// configured default for an xuid it has not seen.
pub fn balance(xuid: &str) -> Result<i64> {
    let f = crate::require_slot!(get_money, "reading a balance");
    let v = unsafe { f(s(xuid)) };
    if v < 0 {
        Err(Error(format!(
            "the balance of {xuid} could not be read: the xuid is empty, the database failed, or no economy backend is installed"
        )))
    } else {
        Ok(v)
    }
}

/// Sets it to a target balance.
pub fn set(xuid: &str, amount: i64) -> Result<()> {
    let f = crate::require_slot!(set_money, "setting a balance");
    ok(unsafe { f(s(xuid), amount) }, "setting a balance", xuid)
}

pub fn add(xuid: &str, delta: i64) -> Result<()> {
    let f = crate::require_slot!(add_money, "increasing a balance");
    ok(unsafe { f(s(xuid), delta) }, "increasing a balance", xuid)
}

pub fn reduce(xuid: &str, delta: i64) -> Result<()> {
    let f = crate::require_slot!(reduce_money, "reducing a balance");
    ok(unsafe { f(s(xuid), delta) }, "reducing a balance", xuid)
}

/// Transfers. A `from == to` is refused by the backend, and the amount the recipient
/// receives is already taxed; see the module documentation.
pub fn transfer(from: &str, to: &str, value: i64, note: &str) -> Result<()> {
    let f = crate::require_slot!(trans_money, "transferring");
    let done = unsafe { f(s(from), s(to), value, s(note)) };
    if done {
        Ok(())
    } else {
        Err(Error(format!(
            "transferring {value} from {from} to {to} failed: insufficient balance, a negative amount, the same account on both ends, or no economy backend installed"
        )))
    }
}

fn ok(done: bool, what: &str, xuid: &str) -> Result<()> {
    if done {
        Ok(())
    } else {
        Err(Error(format!(
            "{what} failed for {xuid}: a negative amount, insufficient balance, or no economy backend installed"
        )))
    }
}

/// The transactions of the last `seconds` seconds, one per line.
pub fn history(xuid: &str, seconds: i32) -> Vec<String> {
    if !crate::has_slot!(money_get_hist) {
        return Vec::new();
    }
    let Some(f) = crate::__rt::api().money_get_hist else {
        return Vec::new();
    };
    collect_strs(|ctx, sink| unsafe { f(s(xuid), seconds, ctx, sink) })
}

/// Clears transactions older than `seconds` seconds.
pub fn clear_history(seconds: i32) {
    if !crate::has_slot!(money_clear_hist) {
        return;
    }
    if let Some(f) = crate::__rt::api().money_clear_hist {
        unsafe { f(seconds) };
    }
}

/// The top `top_n` of the rich list, one per line.
pub fn ranking(top_n: u16) -> Vec<String> {
    if !crate::has_slot!(money_ranking) {
        return Vec::new();
    }
    let Some(f) = crate::__rt::api().money_ranking else {
        return Vec::new();
    };
    collect_strs(|ctx, sink| unsafe { f(top_n, ctx, sink) })
}

/// Registers a callback that runs before the event, where returning `false` vetoes the
/// transaction.
///
/// Several mods may each register their own without overwriting one another, and
/// registering the same function pointer twice is idempotent.
/// The host accounts per module and removes them when the mod unloads.
///
/// # The callback is global and not one per call
///
/// The ABI takes a raw function pointer here with no `user` parameter, so it cannot hold a
/// closure that captured its environment. This layer therefore takes an `fn` and not an
/// `impl FnMut`, which would need the state hidden in a global that is still touched after
/// the mod unloads. Carrying state means a `static` inside the mod itself, cleared in
/// `on_unload`.
pub fn on_before(callback: sys::PierMoneyCb) -> Result<()> {
    let f = crate::require_slot!(
        money_listen_before_event,
        "registering an economy before-callback"
    );
    unsafe { f(callback) };
    Ok(())
}

/// Registers a callback that runs after the event. The return value is ignored.
pub fn on_after(callback: sys::PierMoneyCb) -> Result<()> {
    let f = crate::require_slot!(
        money_listen_after_event,
        "registering an economy after-callback"
    );
    unsafe { f(callback) };
    Ok(())
}

// Callbacks that carry state. The two above take a raw `fn`, because this slot has no
// `user` parameter on the ABI. Capturing an environment means putting the closure in a
// global and registering a static trampoline that reads it, which is what the two below
// do. That is safe here for the reason abi.h states: the loader accounts for these two
// callbacks per module and removes them at unload, so the trampoline, compiled into the
// mod's own dynamic library, is not called after that library is unloaded. Without that
// promise this pattern is a dangling function pointer.

type BeforeFn = dyn FnMut(&MoneyEvent) -> bool + Send + 'static;
type AfterFn = dyn FnMut(&MoneyEvent) + Send + 'static;

fn before_slot() -> &'static std::sync::Mutex<Option<Box<BeforeFn>>> {
    static S: std::sync::OnceLock<std::sync::Mutex<Option<Box<BeforeFn>>>> =
        std::sync::OnceLock::new();
    S.get_or_init(|| std::sync::Mutex::new(None))
}

fn after_slot() -> &'static std::sync::Mutex<Option<Box<AfterFn>>> {
    static S: std::sync::OnceLock<std::sync::Mutex<Option<Box<AfterFn>>>> =
        std::sync::OnceLock::new();
    S.get_or_init(|| std::sync::Mutex::new(None))
}

/// Registers a before-callback that can capture its environment. Returning `false` vetoes
/// the transaction.
///
/// There is one per mod: calling again replaces the previous one rather than running both.
/// Running several means dispatching inside the closure yourself. This is not laziness:
/// the ABI has no `user` parameter, so which closure can only be answered by a global on
/// this side, and one global holds one.
pub fn on_before_with(f: impl FnMut(&MoneyEvent) -> bool + Send + 'static) -> Result<()> {
    *before_slot().lock().unwrap_or_else(|e| e.into_inner()) = Some(Box::new(f));
    on_before(before_trampoline)
}

/// As above, the after version. The return value is ignored, so the closure returns
/// nothing.
pub fn on_after_with(f: impl FnMut(&MoneyEvent) + Send + 'static) -> Result<()> {
    *after_slot().lock().unwrap_or_else(|e| e.into_inner()) = Some(Box::new(f));
    on_after(after_trampoline)
}

/// # Safety
/// Called by the host before an economy event. The four arguments are valid only during
/// the call.
unsafe extern "C" fn before_trampoline(
    kind: sys::PierMoneyEvent,
    from: sys::PierStr,
    to: sys::PierStr,
    value: i64,
) -> bool {
    let ev = event_from_raw(kind, from, to, value);
    guard(|| {
        let mut slot = before_slot().lock().unwrap_or_else(|e| e.into_inner());
        match slot.as_mut() {
            Some(f) => f(&ev),
            // The closure was replaced or cleared: no veto.
            None => true,
        }
    })
}

/// # Safety
/// As `before_trampoline`, but called after the event.
unsafe extern "C" fn after_trampoline(
    kind: sys::PierMoneyEvent,
    from: sys::PierStr,
    to: sys::PierStr,
    value: i64,
) -> bool {
    let ev = event_from_raw(kind, from, to, value);
    guard(|| {
        let mut slot = after_slot().lock().unwrap_or_else(|e| e.into_inner());
        if let Some(f) = slot.as_mut() {
            f(&ev);
        }
        true
    })
}

/// Assembles the four arguments the ABI passes into a [`MoneyEvent`].
///
/// For anyone writing their own `extern "C"` callback: call it on the first line of the
/// body and the rest is safe Rust.
///
/// # Safety
/// The four arguments must be the set the host passed during the callback, and `from` and
/// `to` are valid only for its duration.
pub unsafe fn event_from_raw(
    kind: sys::PierMoneyEvent,
    from: sys::PierStr,
    to: sys::PierStr,
    value: i64,
) -> MoneyEvent {
    MoneyEvent {
        kind,
        from: crate::rt::ffi::r_owned(from),
        to: crate::rt::ffi::r_owned(to),
        value,
    }
}

/// Catches a panic inside an economy callback.
///
/// A panic crossing `extern "C"` is undefined behavior, and an economy callback is called
/// directly by the host.
/// This wraps it, treating a panic as no veto and logging: a veto is the stronger action
/// and should not be triggered by a bug.
pub fn guard(f: impl FnOnce() -> bool) -> bool {
    match std::panic::catch_unwind(std::panic::AssertUnwindSafe(f)) {
        Ok(v) => v,
        Err(_) => {
            Logger::get().error("an economy callback panicked. It was caught here and this one is treated as no veto.");
            true
        }
    }
}
