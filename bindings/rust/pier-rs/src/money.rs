//! 经济。
//!
//! # 后端是延迟加载的，缺席时整族**降级**而不是崩
//!
//! 没装经济后端、或者它被停用时，每个槽返回自己的失败值。这一层把那些失败值
//! 翻译成 `Err`，唯独 [`balance`] 例外 —— 见它自己的文档。
//!
//! # 金额永远非负，转账要抽税
//!
//! 后端自己拒绝负数。[`transfer`] 会按后端配置的 `pay_tax` 抽成，收款方拿到的
//! 是 `val - val * pay_tax` 而不是 `val`；要原额过账就分别 [`add`] 和 [`reduce`]。
//! [`set`] 的参数是**目标余额**，不是增量。

use crate::rt::error::{Error, Result};
use crate::rt::ffi::{collect_strs, s};
use crate::rt::logger::Logger;
use crate::sys;

/// 一笔经济事件。
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MoneyEvent {
    pub kind: sys::PierMoneyEvent,
    /// 付款方 xuid。空串表示凭空产生。
    pub from: String,
    /// 收款方 xuid。空串表示凭空销毁。
    pub to: String,
    pub value: i64,
}

/// 余额。
///
/// 返回 `Err` 而不是 -1：真实余额永远非负，所以负数只可能是「答不上来」
/// （xuid 为空、数据库出错、后端缺席）。
///
/// 注意这**不是**无副作用的读：后端对没见过的 xuid 会按配置的默认值开户。
pub fn balance(xuid: &str) -> Result<i64> {
    let f = crate::require_slot!(get_money, "读取余额");
    let v = unsafe { f(s(xuid)) };
    if v < 0 {
        Err(Error(format!(
            "读不出 {xuid} 的余额（xuid 为空、数据库出错，或没装经济后端）"
        )))
    } else {
        Ok(v)
    }
}

/// 设成一个**目标余额**。
pub fn set(xuid: &str, amount: i64) -> Result<()> {
    let f = crate::require_slot!(set_money, "设置余额");
    ok(unsafe { f(s(xuid), amount) }, "设置余额", xuid)
}

pub fn add(xuid: &str, delta: i64) -> Result<()> {
    let f = crate::require_slot!(add_money, "增加余额");
    ok(unsafe { f(s(xuid), delta) }, "增加余额", xuid)
}

pub fn reduce(xuid: &str, delta: i64) -> Result<()> {
    let f = crate::require_slot!(reduce_money, "扣减余额");
    ok(unsafe { f(s(xuid), delta) }, "扣减余额", xuid)
}

/// 转账。`from == to` 会被后端拒绝；收款方到手的金额已扣税（见模块文档）。
pub fn transfer(from: &str, to: &str, value: i64, note: &str) -> Result<()> {
    let f = crate::require_slot!(trans_money, "转账");
    let done = unsafe { f(s(from), s(to), value, s(note)) };
    if done {
        Ok(())
    } else {
        Err(Error(format!(
            "{from} → {to} 转账 {value} 失败（余额不足、金额为负、两端相同，或没装经济后端）"
        )))
    }
}

fn ok(done: bool, what: &str, xuid: &str) -> Result<()> {
    if done {
        Ok(())
    } else {
        Err(Error(format!(
            "{what}失败：{xuid}（金额为负、余额不足，或没装经济后端）"
        )))
    }
}

/// 最近 `seconds` 秒内的流水，每条一行。
pub fn history(xuid: &str, seconds: i32) -> Vec<String> {
    if !crate::has_slot!(money_get_hist) {
        return Vec::new();
    }
    let Some(f) = crate::__rt::api().money_get_hist else {
        return Vec::new();
    };
    collect_strs(|ctx, sink| unsafe { f(s(xuid), seconds, ctx, sink) })
}

/// 清掉比 `seconds` 秒更早的流水。
pub fn clear_history(seconds: i32) {
    if !crate::has_slot!(money_clear_hist) {
        return;
    }
    if let Some(f) = crate::__rt::api().money_clear_hist {
        unsafe { f(seconds) };
    }
}

/// 富豪榜前 `top_n` 名，每条一行。
pub fn ranking(top_n: u16) -> Vec<String> {
    if !crate::has_slot!(money_ranking) {
        return Vec::new();
    }
    let Some(f) = crate::__rt::api().money_ranking else {
        return Vec::new();
    };
    collect_strs(|ctx, sink| unsafe { f(top_n, ctx, sink) })
}

/// 注册一个**发生之前**的回调，返回 `false` 否决这笔交易。
///
/// 几个模组可以各注册各的，互不覆盖；同一个函数指针注册两次是幂等的。
/// 宿主按模块记账，模组卸载时替你摘掉。
///
/// # 回调是全局的，不是每次调用一个
///
/// ABI 上这里收的是**裸函数指针**，没有 `user` 参数，所以装不下一个捕获了
/// 环境的闭包。这一层因此只收 `fn`，而不是 `impl FnMut` —— 后者需要把状态
/// 藏进一个全局，而那个全局在模组卸载后仍然会被摸到。要带状态就自己在
/// 模组里用 `static`，并在 `on_unload` 里清空。
pub fn on_before(callback: sys::PierMoneyCb) -> Result<()> {
    let f = crate::require_slot!(money_listen_before_event, "注册经济前置回调");
    unsafe { f(callback) };
    Ok(())
}

/// 注册一个**发生之后**的回调。返回值被忽略。
pub fn on_after(callback: sys::PierMoneyCb) -> Result<()> {
    let f = crate::require_slot!(money_listen_after_event, "注册经济后置回调");
    unsafe { f(callback) };
    Ok(())
}

// ── 带状态的回调 ──────────────────────────────────────────────────
//
// 上面那两个只收裸 `fn`,因为 ABI 上这个槽没有 `user` 参数。要捕获环境就
// 只能把闭包放进一个全局,再注册一个静态蹦床去读它 —— 下面这两个就是。
//
// 这样做在这里是安全的,理由写在 abi.h 上:**装载器按模块给这两个回调记账,
// 模组卸载时替你摘掉**。所以蹦床(它编译在模组自己的动态库里)不会在库被
// 卸载之后还被调到。没有这条承诺的话,这个模式就是个悬垂函数指针。

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

/// 注册一个能捕获环境的前置回调。返回 `false` 否决这笔交易。
///
/// **每个模组只有一个**:再调一次会换掉上一个,而不是并列两个。要并列就
/// 自己在闭包里分发。这不是偷懒 —— ABI 上没有 `user` 参数,所以「哪个闭包」
/// 这件事只能由这一侧的全局回答,而一个全局只装得下一个。
pub fn on_before_with(f: impl FnMut(&MoneyEvent) -> bool + Send + 'static) -> Result<()> {
    *before_slot().lock().unwrap_or_else(|e| e.into_inner()) = Some(Box::new(f));
    on_before(before_trampoline)
}

/// 同上,后置版。返回值被忽略,所以闭包不返回东西。
pub fn on_after_with(f: impl FnMut(&MoneyEvent) + Send + 'static) -> Result<()> {
    *after_slot().lock().unwrap_or_else(|e| e.into_inner()) = Some(Box::new(f));
    on_after(after_trampoline)
}

/// # Safety
/// 由宿主在经济事件发生前调用,四个参数只在调用期内有效。
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
            // 闭包已经被换走或清掉:不否决。
            None => true,
        }
    })
}

/// # Safety
/// 同 `before_trampoline`，但在事件发生之后调用。
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

/// 把 ABI 递来的四个参数拼成 [`MoneyEvent`]。
///
/// 给自己写 `extern "C"` 回调的人用：回调体第一行调它，剩下的就是安全 Rust。
///
/// # Safety
/// 四个参数必须是宿主在回调期间交来的那一组，`from` / `to` 只在回调期内有效。
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

/// 在经济回调里就地拦下 panic。
///
/// panic 穿过 `extern "C"` 是未定义行为，而经济回调是宿主直接调过来的。
/// 包一层，panic 时按**不否决**处理并打日志 —— 否决是更强的动作，
/// 不该由一个 bug 触发。
pub fn guard(f: impl FnOnce() -> bool) -> bool {
    match std::panic::catch_unwind(std::panic::AssertUnwindSafe(f)) {
        Ok(v) => v,
        Err(_) => {
            Logger::get().error("经济回调 panic 了。已就地拦下，本次按不否决处理。");
            true
        }
    }
}
