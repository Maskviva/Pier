//! 服务器运行时控制 —— tick 冻结、步进、倍速，以及分项性能采样。
//!
//! 这些不放在 [`crate::host`] 里：那一层说的是宿主本身（排期、执行命令、
//! 操作系统），换个游戏也成立；tick 是**世界模拟**的节奏，是游戏概念。
//!
//! # 钩子装上就不摘
//!
//! 第一次调用时惰性安装 detour，之后一直留着。原因是一次控制调用可能来自
//! 正在 tick 内部执行的命令处理器，在那里摘钩子不安全。空载代价是每帧一次
//! 可预测的分支。
//!
//! # 冻结期间玩家还能动
//!
//! 冻结停的是生物、方块、红石、时间。移动是客户端权威的，网络也跑在关卡
//! tick 之外，所以玩家照样走路聊天。

use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::call_out_str;

/// 服务器运行时门面。零大小。
#[derive(Clone, Copy)]
pub struct Server(());

impl Server {
    pub fn get() -> Server {
        Server(())
    }

    /// 冻结或解冻世界。
    pub fn set_tick_freeze(&self, on: bool) -> Result<()> {
        let f = crate::require_slot!(tick_freeze, "冻结世界 tick");
        if unsafe { f(on) } {
            Ok(())
        } else {
            Err(Error("设不了 tick 冻结（关卡没就绪）".to_owned()))
        }
    }

    /// 只在冻结期间有效：再放 `n` 帧过去。
    pub fn step_ticks(&self, n: u32) -> Result<()> {
        let f = crate::require_slot!(tick_step, "步进 tick");
        if unsafe { f(n) } {
            Ok(())
        } else {
            Err(Error(format!("步进 {n} 帧失败（现在没有冻结，或 n 是 0）")))
        }
    }

    /// 时间倍速。`0 < factor <= 100`，小于 1 是慢放，1.0 恢复正常。
    pub fn set_tick_warp(&self, factor: f64) -> Result<()> {
        let f = crate::require_slot!(tick_warp, "设置 tick 倍速");
        if unsafe { f(factor) } {
            Ok(())
        } else {
            Err(Error(format!(
                "设不了 tick 倍速 {factor}（取值必须在 0 到 100 之间）"
            )))
        }
    }

    /// 开一个 `ticks` 帧的采样窗口（1..12000）。同时只能有一个窗口。
    pub fn begin_profile(&self, ticks: u32) -> Result<()> {
        let f = crate::require_slot!(profile_begin, "开始性能采样");
        if unsafe { f(ticks) } {
            Ok(())
        } else {
            Err(Error(format!(
                "开不了 {ticks} 帧的采样窗口（帧数是 0 或超过 12000，或已经在采样了）"
            )))
        }
    }

    /// 取采样报告。
    ///
    /// 采样还没结束时是 `Ok(None)`，一个窗口只会**成功一次**。
    ///
    /// 各分项的耗时是**含嵌套**的，所以并排看，别相加。
    pub fn take_profile(&self) -> Result<Option<NbtValue>> {
        let f = crate::require_slot!(profile_take, "取性能采样报告");
        let Some(text) = call_out_str(|ctx, sink| unsafe { f(ctx, sink) }) else {
            return Ok(None);
        };
        let v =
            NbtValue::parse(&text).map_err(|e| Error(format!("采样报告 SNBT 解析失败：{e}")))?;
        Ok(Some(v))
    }

    /// 模拟是不是暂停着。
    pub fn is_sim_paused(&self) -> Result<bool> {
        let f = crate::require_slot!(get_sim_paused, "查询模拟暂停状态");
        Ok(unsafe { f() })
    }

    /// 上一帧耗时（秒）。20 TPS 时是 0.05。
    ///
    /// 宿主拿不到时返回 -1.0，这里把它翻成 `Err` —— 一个负的帧耗时会让
    /// 调用方算出负的 TPS 而毫无察觉。
    pub fn tick_delta_time(&self) -> Result<f64> {
        let f = crate::require_slot!(get_tick_delta_time, "读取帧耗时");
        let v = unsafe { f() };
        if v < 0.0 {
            Err(Error("关卡没就绪，读不出帧耗时".to_owned()))
        } else {
            Ok(v)
        }
    }

    pub fn tps(&self) -> Result<f64> {
        let dt = self.tick_delta_time()?;
        if dt <= 0.0 {
            // 0 不是「无穷快」，是「这一帧还没量出来」。
            return Err(Error("帧耗时是 0，这一帧还没量出来，算不出 TPS".to_owned()));
        }
        Ok(1.0 / dt)
    }
}
