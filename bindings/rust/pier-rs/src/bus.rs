//! 跨模组事件总线 —— 广播，无返回值。
//!
//! 和 [`crate::service`] 互补：总线一对多、无返回、顺序不保证；服务一对一、
//! 有返回、名字独占。
//!
//! # 收不到自己发的
//!
//! 一个模组不会收到自己的 publish。想通知自己有直接函数调用；而自发自收是
//! 唯一一种任何深度限制都分辨不出来的环。跨模组的环（A→B→A）由深度上限
//! 接住，撞上限时最内层那次 publish 被丢弃并打一条日志。
//!
//! # 全族线程安全，回调在**发布方**的线程上跑
//!
//! 所以回调里别碰世界状态 —— 要碰就 `Host::schedule` 丢回服务器线程。

use core::ffi::c_void;

use crate::rt::error::{Error, Result};
use crate::rt::ffi::{r, s};
use crate::rt::logger::Logger;
use crate::rt::runtime::rt;
use crate::sys;

/// 一次订阅。**Drop 即退订。**
///
/// 想让它活到模组卸载就调 [`Subscription::forget`]，宿主卸载时会统一清。
pub struct Subscription {
    id: u64,
    topic: String,
    /// 只持有、只析构，不取出来用 —— 和 `service::Registration` 同一套纪律。
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
            // 退订失败而闭包马上要被释放 —— 那就是个悬垂指针。宁可泄漏。
            Logger::get().error(&format!(
                "退订主题 `{}` 失败 —— 它可能还挂在宿主上，闭包改为泄漏而不是释放。",
                self.topic
            ));
            if let Some(o) = self.owned.take() {
                std::mem::forget(o);
            }
        }
    }
}

type Handler = dyn FnMut(&str, &str) -> bool + Send + 'static;

/// 订阅一个主题。
///
/// 回调返回 `true` 表示**否决**，只有 [`publish_vetoable`] 会去看它；
/// 普通 [`publish`] 忽略返回值。
pub fn subscribe(
    topic: &str,
    handler: impl FnMut(&str, &str) -> bool + Send + 'static,
) -> Result<Subscription> {
    let f = crate::require_slot!(bus_subscribe, "订阅总线主题");
    let boxed: Box<Box<Handler>> = Box::new(Box::new(handler));
    let user = Box::into_raw(boxed);
    let id = unsafe { f(rt().handle(), s(topic), trampoline, user.cast()) };
    if id == 0 {
        drop(unsafe { Box::from_raw(user) });
        return Err(Error(format!(
            "订阅主题 `{topic}` 失败（主题为空或过长，或模组还没被接管）"
        )));
    }
    Ok(Subscription {
        id,
        topic: topic.to_owned(),
        owned: Some(unsafe { Box::from_raw(user) }),
    })
}

/// 广播。返回**真正跑了**的订阅者数；0 是正常结果，表示没人在听。
pub fn publish(topic: &str, payload: &str) -> Result<u32> {
    let f = crate::require_slot!(bus_publish, "发布总线消息");
    Ok(unsafe { f(rt().handle(), s(topic), s(payload)) })
}

/// 一次可否决广播的结果。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Vetoable {
    /// 有订阅者投了否决。
    pub vetoed: bool,
    /// 实际跑了几个订阅者。**没有短路** —— 即使前面有人否决，
    /// 后面的观察者照样收到，这样它们看到的是一条一致的流。
    pub delivered: u32,
}

/// 广播并收集否决位。
pub fn publish_vetoable(topic: &str, payload: &str) -> Result<Vetoable> {
    let f = crate::require_slot!(bus_publish_vetoable, "发布可否决的总线消息");
    let mut delivered: u32 = 0;
    let vetoed = unsafe { f(rt().handle(), s(topic), s(payload), &mut delivered) };
    Ok(Vetoable { vetoed, delivered })
}

/// 这个主题现在有几个订阅者（跨全部模组）。
///
/// 用来跳过「拼一份没人看的载荷」的开销。
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
/// `user` 必须是 `subscribe` 里 `Box<Box<Handler>>::into_raw` 的产物。
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
            // 否决是更强的动作，不该由一个 bug 触发。按不否决处理。
            Logger::get().error(&format!(
                "主题 `{t}` 的订阅回调 panic 了。已就地拦下，本次按不否决处理。"
            ));
            false
        }
    }
}
