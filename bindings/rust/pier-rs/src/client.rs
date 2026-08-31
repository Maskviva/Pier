//! 客户端专属能力。
//!
//! # 服务端宿主上这一族全是空槽
//!
//! 不是编译错误，是运行期的 `Err`。契约 §2.1：布局在所有目标下相同，能力
//! 缺席等于槽位 NULL。所以同一份模组源码在两个目标上都编得过，装错目标由
//! 宿主在握手时按 `mod_flags` 明确拒绝。
//!
//! 用 [`is_available`] 判断，别用 `cfg`。
//!
//! # 回调在**客户端线程**上跑
//!
//! 不是服务器线程。热键回调里别碰服务端状态。

use core::ffi::c_void;

use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, s};
use crate::rt::logger::Logger;
use crate::rt::runtime::rt;
use crate::sys;

/// 按键动作。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum KeyAction {
    Up,
    Down,
    /// 宿主报了一个这一侧还不认识的动作码。
    Unknown(i32),
}

impl KeyAction {
    pub fn from_i32(v: i32) -> KeyAction {
        match v {
            0 => KeyAction::Up,
            1 => KeyAction::Down,
            other => KeyAction::Unknown(other),
        }
    }

    pub fn is_down(self) -> bool {
        matches!(self, KeyAction::Down)
    }
}

/// 这个宿主是按客户端目标编的吗（`client_*` 槽位有没有填）。
pub fn is_available() -> bool {
    crate::has_slot!(client_is_in_level)
}

/// 客户端现在在一个世界里吗。
pub fn is_in_level() -> Result<bool> {
    let f = crate::require_slot!(client_is_in_level, "查询客户端是否在世界中");
    Ok(unsafe { f() })
}

/// 本地玩家的名字。不在世界里时是 `Err`。
pub fn local_player_name() -> Result<String> {
    let f = crate::require_slot!(client_get_local_player, "读取本地玩家");
    call_out_str(|ctx, sink| unsafe { f(ctx, sink) })
        .ok_or_else(|| Error("客户端还不在任何世界里".to_owned()))
}

/// 当前界面名（`"hud_screen"`、`"pause_screen"` …）。
pub fn screen_name() -> Result<String> {
    let f = crate::require_slot!(client_get_screen_name, "读取当前界面名");
    call_out_str(|ctx, sink| unsafe { f(ctx, sink) })
        .ok_or_else(|| Error("读不出当前界面名".to_owned()))
}

/// 一个注册好的热键。**Drop 即注销。**
pub struct KeyBinding {
    handle: sys::PierKeyHandle,
    name: String,
    /// 只持有、只析构。装的是两个回调的 `Box`。
    owned: Option<Box<dyn std::any::Any + Send>>,
}

impl KeyBinding {
    pub fn name(&self) -> &str {
        &self.name
    }

    /// 当前实际绑定的键码。玩家改过键位时和默认值不同。
    pub fn key_codes(&self) -> Result<Vec<i32>> {
        let f = crate::require_slot!(client_get_key_codes, "读取热键键码");
        let text = call_out_str(|ctx, sink| unsafe { f(self.handle, ctx, sink) })
            .ok_or_else(|| Error(format!("读不出热键 {} 的键码", self.name)))?;
        // 宿主给的是 "[1,2,3]" 这样的数组字面量。
        Ok(text
            .trim()
            .trim_start_matches('[')
            .trim_end_matches(']')
            .split(',')
            .filter_map(|p| p.trim().parse::<i32>().ok())
            .collect())
    }

    /// 让它活到模组卸载，宿主届时统一清理。
    pub fn forget(mut self) {
        self.handle = core::ptr::null_mut();
        if let Some(o) = self.owned.take() {
            std::mem::forget(o);
        }
    }
}

impl Drop for KeyBinding {
    fn drop(&mut self) {
        if self.handle.is_null() {
            return;
        }
        if !crate::has_slot!(client_unregister_key) {
            return;
        }
        let Some(f) = crate::__rt::api().client_unregister_key else {
            return;
        };
        if !unsafe { f(self.handle) } {
            Logger::get().error(&format!(
                "注销热键 {} 失败 —— 它可能还挂在宿主上，回调改为泄漏而不是释放。",
                self.name
            ));
            if let Some(o) = self.owned.take() {
                std::mem::forget(o);
            }
        }
    }
}

type KeyHandler = dyn FnMut(KeyAction, i32) + Send + 'static;

/// 注册一个热键。
///
/// `key_codes` 是默认键位；`allow_remap` 决定玩家能不能在设置里改它。
/// 回调收到 `(动作, 焦点影响)`，两个都在客户端线程上跑。
pub fn register_key(
    name: &str,
    key_codes: &[i32],
    allow_remap: bool,
    handler: impl FnMut(KeyAction, i32) + Send + 'static,
) -> Result<KeyBinding> {
    let f = crate::require_slot!(client_register_key, "注册热键");
    let boxed: Box<Box<KeyHandler>> = Box::new(Box::new(handler));
    let user = Box::into_raw(boxed);
    let handle = unsafe {
        f(
            rt().handle(),
            s(name),
            key_codes.as_ptr(),
            key_codes.len() as i32,
            allow_remap,
            on_down,
            on_up,
            user.cast(),
        )
    };
    if handle.is_null() {
        drop(unsafe { Box::from_raw(user) });
        return Err(Error(format!(
            "注册热键 {name} 失败（名字被占用，或键码列表为空）"
        )));
    }
    Ok(KeyBinding {
        handle,
        name: name.to_owned(),
        owned: Some(unsafe { Box::from_raw(user) }),
    })
}

/// # Safety
/// `user` 必须是 `register_key` 里 `Box<Box<KeyHandler>>::into_raw` 的产物。
unsafe extern "C" fn on_down(
    user: *mut c_void,
    action: sys::PierKeyAction,
    impact: sys::PierFocusImpact,
) {
    dispatch(user, action, impact);
}

/// # Safety
/// 同 `on_down`。
unsafe extern "C" fn on_up(
    user: *mut c_void,
    action: sys::PierKeyAction,
    impact: sys::PierFocusImpact,
) {
    dispatch(user, action, impact);
}

/// 两个入口共用一个回调：ABI 要求 down 和 up 各给一个非空函数指针，
/// 而动作码本身已经区分了按下和抬起，分成两个闭包只会让调用方写两遍。
///
/// # Safety
/// `user` 必须是 `register_key` 里那个 `Box<Box<KeyHandler>>::into_raw` 的产物。
unsafe fn dispatch(user: *mut c_void, action: sys::PierKeyAction, impact: sys::PierFocusImpact) {
    if user.is_null() {
        return;
    }
    let f = &mut *(user as *mut Box<KeyHandler>);
    let outcome = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        f(KeyAction::from_i32(action), impact)
    }));
    if outcome.is_err() {
        Logger::get().error("热键回调 panic 了。已就地拦下 —— 这一次按键被丢弃。");
    }
}
