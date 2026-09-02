//! 宿主与系统本身 —— 不涉及任何游戏概念的那部分能力。
//!
//! 放在这一层的判据是「它说的是**宿主**还是**世界**」：运行状态、把任务丢回
//! 服务器线程、执行命令、问操作系统的名字 —— 换成别的游戏也成立；
//! 玩家、方块、物品不成立。

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
            return Err(Error(
                "宿主拒绝了排期任务（模组尚未被接管，或句柄无效）".to_owned(),
            ));
        }
        Ok(TaskId(id))
    }

    /// 同上，但延迟 `delay` 之后再跑。线程安全。
    ///
    /// 收 `Duration` 而不是一个裸的毫秒数：`schedule_after(5, …)` 里那个 5
    /// 是五毫秒还是五秒，只有参数名说得清，而参数名在调用点是看不见的。
    /// 单位放进类型里，调用点就自带答案。
    ///
    /// ABI 那一侧是毫秒，所以这里做一次转换。超过 `u64` 毫秒的时长会被钳到
    /// 上限而不是绕回一个很小的数 —— 后者会让一个「一年后执行」变成「立刻执行」。
    pub fn schedule_after(
        &self,
        delay: std::time::Duration,
        f: impl FnOnce() + Send + 'static,
    ) -> Result<TaskId> {
        let cb = crate::require_slot!(schedule_after_for, "延迟排期任务");
        let delay_ms = u64::try_from(delay.as_millis()).unwrap_or(u64::MAX);
        let boxed: Box<Box<dyn FnOnce() + Send>> = Box::new(Box::new(f));
        let user = Box::into_raw(boxed);
        let id = unsafe { cb(rt().handle(), task_trampoline, user.cast(), delay_ms) };
        if id == 0 {
            drop(unsafe { Box::from_raw(user) });
            return Err(Error(
                "宿主拒绝了延迟排期任务（模组尚未被接管，或句柄无效）".to_owned(),
            ));
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
        raw.trim()
            .parse::<u32>()
            .map_err(|e| Error(format!("宿主报的协议版本 {raw:?} 解析不成数字：{e}")))
    }

    /// BDS 的版本串（`Common::getGameVersionString`）。
    pub fn bds_version(&self) -> Result<String> {
        self.server_info(sys::PIER_SRV_BDS_VERSION)
    }

    pub fn packets(&self) -> crate::packet::Packets {
        crate::packet::Packets::get()
    }

    pub fn current_tick(&self) -> Option<u64> {
        api().get_current_tick.map(|f| unsafe { f() })
    }

    pub fn player_count(&self) -> Option<i32> {
        api().get_player_count.map(|f| unsafe { f() })
    }

    /// 读一个环境变量。
    ///
    /// 读不到和读到空串在这里是同一个空串：ABI 上这个槽的失败位只表示
    /// 「宿主没能去读」，而进程环境里一个存在的空变量本来就等于不存在。
    pub fn env(&self, name: &str) -> Result<String> {
        let f = crate::require_slot!(sys_get_env, "读取环境变量");
        Ok(call_out_str(|ctx, sink| unsafe { f(s(name), ctx, sink) }).unwrap_or_default())
    }

    /// 设一个环境变量。只影响本进程。
    pub fn set_env(&self, name: &str, value: &str) -> Result<()> {
        let f = crate::require_slot!(sys_set_env, "设置环境变量");
        if unsafe { f(s(name), s(value)) } {
            Ok(())
        } else {
            Err(Error(format!("设不了环境变量 {name}")))
        }
    }

    /// 这个宿主是不是跑在 Wine 上。
    ///
    /// 值得单独问一句：Wine 上一部分 Windows API 的行为和真 Windows 不同，
    /// 而症状通常出现在离根因很远的地方。
    pub fn is_wine(&self) -> bool {
        if !crate::has_slot!(sys_is_wine) {
            return false;
        }
        match api().sys_is_wine {
            Some(f) => unsafe { f() },
            None => false,
        }
    }

    pub fn os_name(&self) -> Result<String> {
        self.sys_info(sys::PIER_SYS_OS_NAME)
    }

    /// 操作系统版本串。
    pub fn os_version(&self) -> Result<String> {
        self.sys_info(sys::PIER_SYS_OS_VERSION)
    }

    pub fn locale(&self) -> Result<String> {
        self.sys_info(sys::PIER_SYS_LOCALE)
    }

    pub fn local_time(&self) -> Result<crate::types::LocalTime> {
        let text = self.sys_info(sys::PIER_SYS_LOCAL_TIME)?;
        let v = crate::nbt::NbtValue::parse(&text)
            .map_err(|e| Error(format!("本地时间 SNBT 解析失败：{e}")))?;
        Ok(crate::types::LocalTime {
            year: v.opt_i32("year").unwrap_or(0),
            month: v.opt_i32("month").unwrap_or(0),
            day: v.opt_i32("day").unwrap_or(0),
            hour: v.opt_i32("hour").unwrap_or(0),
            minute: v.opt_i32("minute").unwrap_or(0),
            second: v.opt_i32("second").unwrap_or(0),
            ms: v.opt_i32("ms").unwrap_or(0),
        })
    }

    /// 世界门面。
    ///
    /// 这个访问器和 `Host` 之间**不构成环**:`world` 确实要用
    /// `Host::execute_command` 拼 `/fill`，但两边的入口都只是零大小门面的
    /// `get()`，不共享状态。分层检查里这条边是显式声明的。
    pub fn world(&self) -> crate::World {
        crate::World::get()
    }

    /// 服务器运行时控制（tick 冻结、倍速、性能采样）。
    pub fn server(&self) -> crate::Server {
        crate::Server::get()
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
