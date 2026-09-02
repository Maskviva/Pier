//! The cross-mod event bus: broadcast, with no return value.
//!
//! Complementary to [`crate::service`]: the bus is one to many, returns nothing and guarantees no
//! order, while a service is one to one, returns a value and holds its name exclusively.
//!
//! # A mod does not receive its own publish
//!
//! A mod does not receive its own publish. Notifying yourself is a direct function call, and
//! publishing to yourself is the one cycle no depth limit can tell apart. A cross-mod cycle, A to B
//! to A, is caught by the depth cap, and hitting it discards the innermost publish with a log line.
//!
//! # The whole family is thread safe and a callback runs on the publisher's thread
//!
//! A callback therefore must not touch world state; touching it means `Host::schedule` back onto
//! the server thread.

use core::ffi::c_void;

use crate::rt::error::{Error, Result};
use crate::rt::ffi::{r, s};
use crate::rt::logger::Logger;
use crate::rt::runtime::rt;
use crate::sys;

/// One subscription. Dropping it unsubscribes.
///
/// [`Subscription::forget`] keeps it alive until the mod unloads, when the host clears it.
pub struct Subscription {
    id: u64,
    topic: String,
    /// Only held and dropped, never taken out, following the same discipline as
    /// `service::Registration`.
    owned: Option<Box<dyn std::any::Any + Send>>,
}

impl Subscription {
    pub fn id(&self) -> u64 {
        self.id
    }

    pub fn topic(&self) -> &str {
        &self.topic
    }

    pub fn forget(mut self) {
        self.id = 0;
        if let Some(o) = self.owned.take() {
            std::mem::forget(o);
        }
    }
}

impl Drop for Subscription {
    fn drop(&mut self) {
        if self.id == 0 {
            return;
        }
        if !crate::has_slot!(bus_unsubscribe) {
            return;
        }
        let Some(f) = crate::__rt::api().bus_unsubscribe else {
            return;
        };
        if !unsafe { f(rt().handle(), self.id) } {
            // A failed unsubscribe with the closure about to be freed is a dangling pointer.
            // Leaking is preferable.
            Logger::get().error(&format!(
                "unsubscribing from topic `{}` failed; it may still be attached to the host, so the closure is leaked rather than freed.",
                self.topic
            ));
            if let Some(o) = self.owned.take() {
                std::mem::forget(o);
            }
        }
    }
}

type Handler = dyn FnMut(&str, &str) -> bool + Send + 'static;

/// Subscribes to a topic.
///
/// A `true` from the callback is a veto, which only [`publish_vetoable`] reads; an
/// ordinary [`publish`] ignores the return value.
pub fn subscribe(
    topic: &str,
    handler: impl FnMut(&str, &str) -> bool + Send + 'static,
) -> Result<Subscription> {
    let f = crate::require_slot!(bus_subscribe, "subscribing to a bus topic");
    let boxed: Box<Box<Handler>> = Box::new(Box::new(handler));
    let user = Box::into_raw(boxed);
    let id = unsafe { f(rt().handle(), s(topic), trampoline, user.cast()) };
    if id == 0 {
        drop(unsafe { Box::from_raw(user) });
        return Err(Error(format!(
            "subscribing to topic `{topic}` failed: the topic is empty or too long, or the mod is not adopted yet"
        )));
    }
    Ok(Subscription {
        id,
        topic: topic.to_owned(),
        owned: Some(unsafe { Box::from_raw(user) }),
    })
}

/// Broadcasts. It returns how many subscribers really ran, and 0 is a normal result
/// meaning nobody is listening.
pub fn publish(topic: &str, payload: &str) -> Result<u32> {
    let f = crate::require_slot!(bus_publish, "publishing a bus message");
    Ok(unsafe { f(rt().handle(), s(topic), s(payload)) })
}

/// The result of one vetoable broadcast.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Vetoable {
    /// A subscriber cast a veto.
    pub vetoed: bool,
    /// How many subscribers really ran. There is no short circuit: even after a veto the
    /// later observers still receive it, so they see a consistent stream.
    pub delivered: u32,
}

/// Broadcasts and collects the veto bit.
pub fn publish_vetoable(topic: &str, payload: &str) -> Result<Vetoable> {
    let f = crate::require_slot!(bus_publish_vetoable, "publishing a vetoable bus message");
    let mut delivered: u32 = 0;
    let vetoed = unsafe { f(rt().handle(), s(topic), s(payload), &mut delivered) };
    Ok(Vetoable { vetoed, delivered })
}

/// How many subscribers this topic currently has, across every mod.
///
/// For skipping the cost of assembling a payload nobody will read.
pub fn subscriber_count(topic: &str) -> u32 {
    if !crate::has_slot!(bus_subscriber_count) {
        return 0;
    }
    match crate::__rt::api().bus_subscriber_count {
        Some(f) => unsafe { f(s(topic)) },
        None => 0,
    }
}

/// # Safety
/// `user` must come from the `Box<Box<Handler>>::into_raw` inside `subscribe`.
unsafe extern "C" fn trampoline(
    user: *mut c_void,
    topic: sys::PierStr,
    payload: sys::PierStr,
) -> bool {
    if user.is_null() {
        return false;
    }
    let f = &mut *(user as *mut Box<Handler>);
    let t = r(topic);
    let p = r(payload);
    match std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| f(t, p))) {
        Ok(v) => v,
        Err(_) => {
            // A veto is the stronger action and should not be triggered by a bug. Treated as no
            // veto.
            Logger::get().error(&format!(
                "a subscription callback for topic `{t}` panicked. It was caught here and this one is treated as no veto."
            ));
            false
        }
    }
}
