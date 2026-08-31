//! 原始数据包拦截。
//!
//! 这一层管闭包的所有权、panic 围栏，以及把 `{ptr,len}` 收成 `&[u8]`。
//! 它**不解释包体的任何一个字节** —— 版本差异、字段布局、编解码全在调用方
//! 那一侧。一个能跨版本用的 loader 不可能同时懂每个版本的线格式。
//!
//! # 线程（写状态之前先读这一段）
//!
//! 入站回调跑在**连接被抽水的那个线程**上，出站跑在**发起发送的那个线程**
//! 上。通常是服务器线程，但异步 flush 意味着**不保证** —— 所以闭包的约束是
//! `Send + Sync` 而不是 `Send`：同一个闭包可能被多个线程同时进入。
//!
//! 多个订阅者时，每一个看到上一个的输出，按注册顺序；**第一个 Drop 胜出**，
//! 后面整个跳过。订阅表在派发前快照，所以回调里注册/注销是安全的。

use core::ffi::c_void;

use crate::rt::error::{Error, Result};
use crate::rt::ffi::r;
use crate::rt::handle::Handle;
use crate::rt::logger::Logger;
use crate::rt::runtime::rt;
use crate::sys;

/// 包的方向。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Direction {
    /// 客户端 → 服务端。
    Inbound,
    /// 服务端 → 客户端。
    Outbound,
}

impl Direction {
    fn from_abi(v: i32) -> Direction {
        // ABI 上只有这两个值。真出了第三个说明宿主比这个 SDK 新，
        // 而对一个「方向」来说猜错的代价是把出站当入站处理 —— 宁可当出站
        // （更常见、且改写出站包的风险低于改写入站包）。
        if v == sys::PIER_PKT_INBOUND {
            Direction::Inbound
        } else {
            Direction::Outbound
        }
    }
}

/// 注册时选哪些方向。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Directions {
    Inbound,
    Outbound,
    Both,
}

impl Directions {
    fn mask(self) -> i32 {
        match self {
            Directions::Inbound => sys::PIER_PKT_MASK_INBOUND,
            Directions::Outbound => sys::PIER_PKT_MASK_OUTBOUND,
            Directions::Both => sys::PIER_PKT_MASK_INBOUND | sys::PIER_PKT_MASK_OUTBOUND,
        }
    }
}

/// 回调对这个包的处置。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Verdict {
    /// 原样转发。若调用过 `set_body` / `set_packet_id`，改写生效。
    Forward,
    /// 整个吃掉这个包。
    Drop,
}

/// 连接的两种状态。
///
/// **关掉是唯一可靠的「清掉这条连接的状态」信号**：一条没走完登录握手的
/// 连接永远不会变成 Player，任何玩家事件都覆盖不到它。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ConnectionState {
    Opened,
    Closed,
}

/// 拦截器闭包的类型。
///
/// 单独起个名字是 clippy 的 `type_complexity` 要求的，但它也确实更好读：
/// 这个类型在注册、蹦床、`PacketHook` 三处各出现一次，写全的话三处得
/// 一模一样地对上，改一次要改三遍。
type PacketFn = dyn Fn(&mut Packet<'_>) -> Verdict + Send + Sync;

/// 连接开关观察者闭包的类型。理由同上。
type ConnFn = dyn Fn(u64, &str, ConnectionState) + Send + Sync;

/// 回调收到的一个包。**只在回调期间有效。**
pub struct Packet<'a> {
    ev: &'a sys::PierPacketEvent,
    edit: &'a mut sys::PierPacketEdit,
    replace_ctx: *mut c_void,
    replace: sys::PierBytesSink,
    replaced: bool,
}

impl<'a> Packet<'a> {
    pub fn direction(&self) -> Direction {
        Direction::from_abi(self.ev.direction)
    }

    /// 这条连接的 id。跨包稳定，用它做每连接状态的键。
    pub fn conn_id(&self) -> u64 {
        self.ev.conn_id
    }

    /// 对端地址（`ip:port`）。诊断用；做键请用 `conn_id`。
    pub fn address(&self) -> &str {
        unsafe { r(self.ev.address) }
    }

    pub fn packet_id(&self) -> i32 {
        self.ev.packet_id
    }

    pub fn sender_sub_id(&self) -> u8 {
        self.ev.sender_sub_id
    }

    pub fn target_sub_id(&self) -> u8 {
        self.ev.target_sub_id
    }

    /// 包体，**不含头**（包 id 和 sub id 已经被解出来了）。
    ///
    /// 这一点是刻意的：改写方只需要给出新的**包体**，头由宿主按 `edit`
    /// 重新编码 —— 于是「改一个包 id」是一次字段赋值，而不是一场
    /// varint 拆装手术。
    pub fn body(&self) -> &[u8] {
        if self.ev.body.is_null() || self.ev.body_len == 0 {
            return &[];
        }
        unsafe { core::slice::from_raw_parts(self.ev.body, self.ev.body_len) }
    }

    /// 换掉包体。可以在一次回调里调多次，最后一次算数。
    pub fn set_body(&mut self, bytes: &[u8]) {
        let sink = self.replace;
        unsafe { sink(self.replace_ctx, bytes.as_ptr(), bytes.len()) };
        self.replaced = true;
    }

    /// 改写包 id（重映射到另一个版本的编号时用）。
    pub fn set_packet_id(&mut self, id: i32) {
        self.edit.packet_id = id;
        self.replaced = true;
    }

    pub fn set_sender_sub_id(&mut self, id: u8) {
        self.edit.sender_sub_id = id;
        self.replaced = true;
    }

    pub fn set_target_sub_id(&mut self, id: u8) {
        self.edit.target_sub_id = id;
        self.replaced = true;
    }
}

/// 一个已注册的包拦截器。
///
/// **Drop 即注销。** 想让它活到模组卸载就调 `forget()` —— 那是显式的，
/// 因为「注册完就扔」和「注册完忘了保存返回值」在代码里长得一模一样，
/// 而后者是个 bug。宿主在模组卸载时会统一清掉剩下的（拆除步骤 stage 90）。
pub struct PacketHook {
    handle: Handle,
    unregister: Option<unsafe extern "C" fn(sys::PierModHandle, sys::PierPacketHookHandle) -> bool>,
    /// 闭包的所有权。**注销之后才释放** —— 反过来会让宿主拿着一个已经
    /// 释放的指针继续回调，而那一刻通常是下一个包到达时，离这里很远。
    ///
    /// 顺序由 Rust 保证：`Drop::drop` 先跑（注销），字段再按声明顺序析构。
    owned: Option<Box<dyn core::any::Any + Send + Sync>>,
}

impl PacketHook {
    /// 放弃 Drop 时注销，让它活到模组卸载。
    ///
    /// 这一步是**显式**的，因为「注册完就扔」和「注册完忘了保存返回值」
    /// 在代码里长得一模一样，而后者是个 bug —— 拦截器刚装上就没了。
    pub fn forget(mut self) {
        self.unregister = None;
        // 闭包必须继续活着，宿主还会调它。有意泄漏；宿主在模组卸载时
        // 统一清掉剩下的注册（拆除步骤 stage 90）。
        if let Some(owned) = self.owned.take() {
            core::mem::forget(owned);
        }
    }
}

impl Drop for PacketHook {
    fn drop(&mut self) {
        if let Some(f) = self.unregister {
            unsafe { f(rt().handle(), self.handle.get()) };
        }
    }
}

/// 数据包门面。
#[derive(Clone, Copy)]
pub struct Packets(());

impl Packets {
    pub fn get() -> Packets {
        Packets(())
    }

    /// 注册一个包拦截器。
    ///
    /// 闭包要 `Send + Sync`：见模块头的线程那一段 —— 回调不保证在服务器
    /// 线程，而且可能被多个线程同时进入。
    pub fn intercept<F>(&self, dirs: Directions, f: F) -> Result<PacketHook>
    where
        F: Fn(&mut Packet<'_>) -> Verdict + Send + Sync + 'static,
    {
        let reg = crate::require_slot!(packet_hook_register, "拦截数据包");
        // 两道闸缺一不可（契约 §2.2）：先查表够不够长，再查槽非不非空。
        // 反注册槽的偏移比注册槽更大，所以注册成功**不蕴含**这个槽读得到。
        let unreg = if crate::has_slot!(packet_hook_unregister) {
            rt().api.packet_hook_unregister
        } else {
            None
        };

        let boxed: Box<Box<PacketFn>> = Box::new(Box::new(f));
        let user = Box::into_raw(boxed);

        let handle = unsafe { reg(rt().handle(), dirs.mask(), packet_trampoline, user.cast()) };
        if handle.is_null() {
            // 注册失败：把闭包收回来释放，别泄漏。
            drop(unsafe { Box::from_raw(user) });
            return Err(Error(
                "宿主拒绝注册数据包拦截器（方向掩码为空，或宿主内部失败）".to_owned(),
            ));
        }
        Ok(PacketHook {
            handle: Handle::new(handle),
            unregister: unreg,
            owned: Some(unsafe { Box::from_raw(user) }),
        })
    }

    /// 注册一个连接开/关的观察者。
    pub fn on_connection<F>(&self, f: F) -> Result<PacketHook>
    where
        F: Fn(u64, &str, ConnectionState) + Send + Sync + 'static,
    {
        let reg = crate::require_slot!(packet_conn_hook_register, "观察连接开关");
        // 两道闸缺一不可（契约 §2.2）：先查表够不够长，再查槽非不非空。
        let unreg = if crate::has_slot!(packet_conn_hook_unregister) {
            rt().api.packet_conn_hook_unregister
        } else {
            None
        };

        let boxed: Box<Box<ConnFn>> = Box::new(Box::new(f));
        let user = Box::into_raw(boxed);

        let handle = unsafe { reg(rt().handle(), conn_trampoline, user.cast()) };
        if handle.is_null() {
            drop(unsafe { Box::from_raw(user) });
            return Err(Error("宿主拒绝注册连接观察者".to_owned()));
        }
        Ok(PacketHook {
            handle: Handle::new(handle),
            unregister: unreg,
            owned: Some(unsafe { Box::from_raw(user) }),
        })
    }
}

// ── 两个蹦床 ──────────────────────────────────────────────────────
//
// panic 穿过 extern "C" 是未定义行为。两个都包在 catch_unwind 里：
// 代价是这一个包按「原样转发」处理，而不是整个进程无诊断 abort。
// **不能**在这里选 Drop —— 一个 panic 的拦截器把包全吃掉，症状是
// 「玩家进不来且服务端一切正常」，比漏改一个包难查得多。

/// # Safety
/// 由宿主调用，`user` 是 `intercept` 装箱的闭包。
unsafe extern "C" fn packet_trampoline(
    user: *mut c_void,
    ev: *const sys::PierPacketEvent,
    edit: *mut sys::PierPacketEdit,
    replace_ctx: *mut c_void,
    replace: sys::PierBytesSink,
) -> i32 {
    if user.is_null() || ev.is_null() || edit.is_null() {
        return sys::PIER_PKT_PASS;
    }
    let f = &*(user as *const Box<PacketFn>);
    let mut packet = Packet {
        ev: &*ev,
        edit: &mut *edit,
        replace_ctx,
        replace,
        replaced: false,
    };
    let verdict = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| f(&mut packet)));
    match verdict {
        Ok(Verdict::Drop) => sys::PIER_PKT_DROP,
        Ok(Verdict::Forward) => {
            if packet.replaced {
                sys::PIER_PKT_REPLACE
            } else {
                sys::PIER_PKT_PASS
            }
        }
        Err(_) => {
            Logger::get().error(
                "数据包拦截器 panic 了。已就地拦下，这个包按原样转发 —— \
                 选 Drop 的话症状会是「玩家进不来而服务端一切正常」，更难查。",
            );
            sys::PIER_PKT_PASS
        }
    }
}

/// # Safety
/// 由宿主调用，`user` 是 `on_connection` 装箱的闭包。
unsafe extern "C" fn conn_trampoline(
    user: *mut c_void,
    conn_id: u64,
    address: sys::PierStr,
    opened: bool,
) {
    if user.is_null() {
        return;
    }
    let f = &*(user as *const Box<ConnFn>);
    let addr = r(address);
    let state = if opened {
        ConnectionState::Opened
    } else {
        ConnectionState::Closed
    };
    if std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| f(conn_id, addr, state))).is_err() {
        Logger::get().error("连接观察者 panic 了。已就地拦下。");
    }
}
