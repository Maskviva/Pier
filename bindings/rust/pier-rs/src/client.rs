//! Client-only capabilities.
//!
//! # On a server host this whole family is empty slots
//!
//! Not a compile error but a runtime `Err`. Contract §2.1: the layout is identical on every
//! target and an absent capability is a NULL slot, so the same mod source compiles for both
//! targets and loading onto the wrong one is refused explicitly by the host during the
//! handshake, from `mod_flags`.
//!
//! Decide with [`is_available`] and not with a `cfg`.
//!
//! # A callback runs on the client thread
//!
//! Not the server thread. A hotkey callback must not touch server state.

use core::ffi::c_void;

use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, s};
use crate::rt::handle::Handle;
use crate::rt::logger::Logger;
use crate::rt::runtime::rt;
use crate::sys;

/// A key action.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum KeyAction {
    Up,
    Down,
    /// The host reported an action code this side does not recognize.
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

/// Whether this host was built for the client target, meaning whether the `client_*` slots
/// are filled.
pub fn is_available() -> bool {
    crate::has_slot!(client_is_in_level)
}

/// Whether the client is currently in a world.
pub fn is_in_level() -> Result<bool> {
    let f = crate::require_slot!(client_is_in_level, "querying whether the client is in a world");
    Ok(unsafe { f() })
}

/// The name of the local player. Not being in a world is an `Err`.
pub fn local_player_name() -> Result<String> {
    let f = crate::require_slot!(client_get_local_player, "reading the local player");
    call_out_str(|ctx, sink| unsafe { f(ctx, sink) })
        .ok_or_else(|| Error("the client is not in any world yet".to_owned()))
}

/// The current screen name, such as `"hud_screen"` or `"pause_screen"`.
pub fn screen_name() -> Result<String> {
    let f = crate::require_slot!(client_get_screen_name, "reading the current screen name");
    call_out_str(|ctx, sink| unsafe { f(ctx, sink) })
        .ok_or_else(|| Error("the current screen name could not be read".to_owned()))
}

/// One registered hotkey. Dropping it deregisters.
pub struct KeyBinding {
    handle: Handle,
    forgotten: bool,
    name: String,
    /// Only held and dropped. It holds the `Box` of the two callbacks.
    owned: Option<Box<dyn std::any::Any + Send>>,
}

impl KeyBinding {
    pub fn name(&self) -> &str {
        &self.name
    }

    /// The key codes currently bound. They differ from the defaults once the player has
    /// rebound them.
    pub fn key_codes(&self) -> Result<Vec<i32>> {
        let f = crate::require_slot!(client_get_key_codes, "reading the key codes of a hotkey");
        let text = call_out_str(|ctx, sink| unsafe { f(self.handle.get(), ctx, sink) })
            .ok_or_else(|| Error(format!("the key codes of hotkey {} could not be read", self.name)))?;
        // The host gives an array literal such as "[1,2,3]".
        Ok(text
            .trim()
            .trim_start_matches('[')
            .trim_end_matches(']')
            .split(',')
            .filter_map(|p| p.trim().parse::<i32>().ok())
            .collect())
    }

    /// Keeps it alive until the mod unloads, when the host clears it.
    pub fn forget(mut self) {
        if let Some(o) = self.owned.take() {
            std::mem::forget(o);
        }
        self.forgotten = true;
    }
}

impl Drop for KeyBinding {
    fn drop(&mut self) {
        if self.forgotten || self.handle.is_null() {
            return;
        }
        if !crate::has_slot!(client_unregister_key) {
            return;
        }
        let Some(f) = crate::__rt::api().client_unregister_key else {
            return;
        };
        if !unsafe { f(self.handle.get()) } {
            Logger::get().error(&format!(
                "deregistering hotkey {} failed; it may still be attached to the host, so the callbacks are leaked rather than freed.",
                self.name
            ));
            if let Some(o) = self.owned.take() {
                std::mem::forget(o);
            }
        }
    }
}

type KeyHandler = dyn FnMut(KeyAction, i32) + Send + 'static;

/// Registers a hotkey.
///
/// `key_codes` is the default binding and `allow_remap` decides whether the player may
/// change it in the settings.
/// The callback receives (action, focus impact) and both run on the client thread.
pub fn register_key(
    name: &str,
    key_codes: &[i32],
    allow_remap: bool,
    handler: impl FnMut(KeyAction, i32) + Send + 'static,
) -> Result<KeyBinding> {
    let f = crate::require_slot!(client_register_key, "registering a hotkey");
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
            "registering hotkey {name} failed: the name is taken, or the key code list is empty"
        )));
    }
    Ok(KeyBinding {
        handle: Handle::new(handle),
        forgotten: false,
        name: name.to_owned(),
        owned: Some(unsafe { Box::from_raw(user) }),
    })
}

/// # Safety
/// `user` must come from the `Box<Box<KeyHandler>>::into_raw` inside `register_key`.
unsafe extern "C" fn on_down(
    user: *mut c_void,
    action: sys::PierKeyAction,
    impact: sys::PierFocusImpact,
) {
    dispatch(user, action, impact);
}

/// # Safety
/// As `on_down`.
unsafe extern "C" fn on_up(
    user: *mut c_void,
    action: sys::PierKeyAction,
    impact: sys::PierFocusImpact,
) {
    dispatch(user, action, impact);
}

/// Both entry points share one callback: the ABI wants a non-null function pointer for
/// down and for up, while the action code already tells press from release, and splitting
/// into two closures would only make a caller write it twice.
///
/// # Safety
/// `user` must come from that `Box<Box<KeyHandler>>::into_raw` inside `register_key`.
unsafe fn dispatch(user: *mut c_void, action: sys::PierKeyAction, impact: sys::PierFocusImpact) {
    if user.is_null() {
        return;
    }
    let f = &mut *(user as *mut Box<KeyHandler>);
    let outcome = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        f(KeyAction::from_i32(action), impact)
    }));
    if outcome.is_err() {
        Logger::get().error("a hotkey callback panicked. It was caught here and this key press was discarded.");
    }
}
