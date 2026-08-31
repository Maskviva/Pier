//! 宿主与系统本身 —— 不涉及任何游戏概念的那部分能力。
//!
//! 放在这一层而不是某个域模块里，判据是「它说的是**宿主**还是**世界**」：
//! 服务器运行状态、把任务丢回服务器线程、执行一条命令、问操作系统的名字 ——
//! 这些换成别的游戏也成立；玩家、方块、物品不成立。
//!
//! # 这个模块也是 `rt::ffi` 那几个 sink 的**第一个调用方**
//!
//! 这一点值得写出来，因为它是 `cargo clippy -D warnings` 逼出来的：
//! 上一版 `rt/ffi.rs` 里七个 helper 一个调用方都没有，clippy 全报
//! `never used`。当时有三条路 —— 加 `#[allow(dead_code)]`、删掉、
//! 或者给它们真正的调用方。
//!
//! 选了第三条，另加一条纪律：**只发布有调用方的东西**。没有调用方的
//! helper 从来没被真正走过一遍，它对 `ctx` 类型的那些 `# Safety` 断言
//! 也就从来没有被任何真实调用点检验过 —— 那不是「准备好了」，那是
//! 「看起来准备好了」。`collect_bytes` / `collect_byte_chunks` 因此被
//! 删掉，等第一个字节 sink 的域（NBT 二进制、数据包体）落地时再回来。

use core::ffi::c_void;

use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, collect_strs, r_owned, s};
use crate::rt::logger::Logger;
use crate::rt::runtime::{api, rt, TaskId};
use crate::sys;

/// 服务器的运行阶段。镜像 `ll::GamingStatus`。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum GamingStatus {
    Default,
    Starting,
    Running,
    Stopping,
    /// 宿主报了一个这一侧还不认识的值。**不是错误** —— 宿主可能比模组新
    /// （契约 §2.2）。用 `Unknown(i32)` 而不是 panic 或静默归成 Default，
    /// 是因为「不认识」和「Default」是两件事（契约 §5.2）。
    Unknown(i32),
}

impl From<i32> for GamingStatus {
    fn from(v: i32) -> Self {
        match v {
            0 => GamingStatus::Default,
            1 => GamingStatus::Starting,
            2 => GamingStatus::Running,
            3 => GamingStatus::Stopping,
            other => GamingStatus::Unknown(other),
        }
    }
}

/// 宿主门面。零大小，随便 `Copy`。
#[derive(Clone, Copy)]
pub struct Host(());

impl Host {
    pub fn get() -> Host {
        Host(())
    }

    /// 服务器现在处于哪个阶段。ABI 上标了线程安全，任何线程都能问。
    pub fn gaming_status(&self) -> GamingStatus {
        match api().gaming_status {
            Some(f) => GamingStatus::from(unsafe { f() }),
            // 核心槽缺席只可能是宿主填表填漏了。不 panic：一个状态查询不该
            // 把服务器带下去；返回 Unknown(-1) 让调用方自己决定怎么办。
            None => GamingStatus::Unknown(-1),
        }
    }

    /// 把一段活儿丢回**服务器线程**尽快执行。线程安全。
    ///
    /// 闭包被装箱后交给宿主，回调里取回并执行，执行完立刻释放 ——
    /// 所有权全程在模组这一侧，符合契约 §三（跨边界不移交所有权：
    /// 我们递过去的是一个不透明指针，宿主只负责原样回传）。
    ///
    /// V-03：走**带模组句柄**的 `schedule_for`，不再走无主的 `schedule`。
    /// 无主槽的任务在模组卸载后照样触发 —— 跳进已经 unmap 的代码段。带句柄
    /// 的任务由宿主按模组记账，卸载时整批丢弃（并打一行 warn 提醒你自己
    /// 取消）。返回的 [`TaskId`] 可用于 [`Host::cancel`]。
    pub fn schedule(&self, f: impl FnOnce() + Send + 'static) -> Result<TaskId> {
        let cb = crate::require_slot!(schedule_for, "排期任务");
        let boxed: Box<Box<dyn FnOnce() + Send>> = Box::new(Box::new(f));
        let user = Box::into_raw(boxed);
        let id = unsafe { cb(rt().handle(), task_trampoline, user.cast()) };
        if id == 0 {
            // 宿主拒收：闭包还没交出去，收回所有权。
            drop(unsafe { Box::from_raw(user) });
            return Err(Error("宿主拒绝了排期任务（模组尚未被接管，或句柄无效）".to_owned()));
        }
        Ok(TaskId(id))
    }

    /// 同上，但延迟 `delay_ms` 毫秒。线程安全。
    pub fn schedule_after(&self, delay_ms: u64, f: impl FnOnce() + Send + 'static) -> Result<TaskId> {
        let cb = crate::require_slot!(schedule_after_for, "延迟排期任务");
        let boxed: Box<Box<dyn FnOnce() + Send>> = Box::new(Box::new(f));
        let user = Box::into_raw(boxed);
        let id = unsafe { cb(rt().handle(), task_trampoline, user.cast(), delay_ms) };
        if id == 0 {
            drop(unsafe { Box::from_raw(user) });
            return Err(Error("宿主拒绝了延迟排期任务（模组尚未被接管，或句柄无效）".to_owned()));
        }
        Ok(TaskId(id))
    }

    /// 取消一个尚未执行的任务。已执行、已取消或不属于本模组的票据返回 `false`。
    ///
    /// 注意：取消只是把票据作废；闭包本身在宿主拆除时不会被调用，也不会被
    /// 释放 —— 它随进程结束回收。要避免泄漏就别在热路径上大量排期再取消。
    pub fn cancel(&self, task: TaskId) -> bool {
        if !task.is_valid() {
            return false;
        }
        match api().schedule_cancel {
            Some(f) => unsafe { f(rt().handle(), task.0) },
            None => false,
        }
    }

    /// 本模组名下还有多少任务没跑。适合在 `on_unload` 里断言为 0。
    pub fn pending_tasks(&self) -> u32 {
        match api().schedule_pending_count {
            Some(f) => unsafe { f(rt().handle()) },
            None => 0,
        }
    }

    /// 以控制台身份执行一条命令，取回它的输出。
    ///
    /// 返回 `Err` 的两种情况分得开：槽位缺席（宿主太老）和命令本身失败
    /// （`success == false`，输出里是错误信息）。
    pub fn execute_command(&self, cmd: &str) -> Result<String> {
        let f = crate::require_slot!(execute_command, "执行命令");
        let mut out = CmdOut {
            success: false,
            text: String::new(),
        };
        let ok = unsafe { f(s(cmd), (&mut out as *mut CmdOut).cast(), cmd_sink) };
        if !ok {
            return Err(Error(format!("命令 `{cmd}` 没能执行（宿主拒绝）")));
        }
        if !out.success {
            return Err(Error(format!("命令 `{cmd}` 执行失败：{}", out.text)));
        }
        Ok(out.text)
    }

    /// 列出宿主当前能解析的全部事件 id。
    ///
    /// 订阅失败时把这个列表打出来，比一句「subscribe failed」有用得多
    /// （契约 §5.3：日志要能回答「我该做什么」）。
    pub fn list_events(&self) -> Vec<String> {
        let Some(f) = api().list_events else {
            return Vec::new();
        };
        collect_strs(|ctx, sink| unsafe { f(ctx, sink) })
    }

    /// 操作系统层面的信息。`prop` 取 `sys::PIER_SYS_*`。
    pub fn sys_info(&self, prop: i32) -> Result<String> {
        let f = crate::require_slot!(sys_info_str, "读取系统信息");
        call_out_str(|ctx, sink| unsafe { f(prop, ctx, sink) })
            .ok_or_else(|| Error(format!("宿主读不出系统信息项 {prop}")))
    }

    /// 服务器层面的信息。`prop` 取 `sys::PIER_SRV_*`。
    pub fn server_info(&self, prop: i32) -> Result<String> {
        let f = crate::require_slot!(server_info_str, "读取服务器信息");
        call_out_str(|ctx, sink| unsafe { f(prop, ctx, sink) })
            .ok_or_else(|| Error(format!("宿主读不出服务器信息项 {prop}")))
    }

    /// 服务端的网络协议版本号。
    ///
    /// 具名访问器而不是让调用方自己传 `PIER_SRV_PROTOCOL_VERSION`：这个数
    /// 是**跨版本适配类模组的第一个判据**，写错常量号的代价是拿到 BDS
    /// 版本串然后解析失败，而那个失败离根因很远。
    ///
    /// ABI 上它是字符串（`SharedConstants::NetworkProtocolVersion` 转过来的），
    /// 这里解析成数字并把解析失败**如实报错**，不返回 0 —— 「问不出来」
    /// 和「协议版本是 0」必须分开（契约 §5.2）。
    pub fn protocol_version(&self) -> Result<u32> {
        let raw = self.server_info(sys::PIER_SRV_PROTOCOL_VERSION)?;
        raw.trim().parse::<u32>().map_err(|e| {
            Error(format!("宿主报的协议版本 {raw:?} 解析不成数字：{e}"))
        })
    }

    /// BDS 的版本串（`Common::getGameVersionString`）。
    pub fn bds_version(&self) -> Result<String> {
        self.server_info(sys::PIER_SRV_BDS_VERSION)
    }

    /// 数据包门面。
    pub fn packets(&self) -> crate::packet::Packets {
        crate::packet::Packets::get()
    }

    /// 当前 tick 计数。
    pub fn current_tick(&self) -> Option<u64> {
        api().get_current_tick.map(|f| unsafe { f() })
    }

    /// 在线玩家数。
    pub fn player_count(&self) -> Option<i32> {
        api().get_player_count.map(|f| unsafe { f() })
    }

}

// ── 跨 extern "C" 的两个蹦床 ───────────────────────────────────────

/// # Safety
/// `user` 必须是 `Box<Box<dyn FnOnce() + Send>>::into_raw` 的产物，且只被调一次。
unsafe extern "C" fn task_trampoline(user: *mut c_void) {
    if user.is_null() {
        return;
    }
    let f: Box<Box<dyn FnOnce() + Send>> = Box::from_raw(user.cast());
    // panic 穿过 extern "C" 是未定义行为。拦在这里，代价是这一个任务被丢掉，
    // 而不是整个进程无诊断 abort。
    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(move || (*f)()));
    if result.is_err() {
        Logger::get().error("排期任务 panic 了。已就地拦下 —— 这个任务被丢弃。");
    }
}

struct CmdOut {
    success: bool,
    text: String,
}

/// # Safety
/// `ctx` 必须是一个有效的 `*mut CmdOut`。
unsafe extern "C" fn cmd_sink(ctx: *mut c_void, success: bool, output: sys::PierStr) {
    let out = &mut *ctx.cast::<CmdOut>();
    out.success = success;
    out.text = r_owned(output);
}
