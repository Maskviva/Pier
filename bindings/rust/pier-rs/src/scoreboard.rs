//! 计分板。
//!
//! 全部走同一个多路槽 `scoreboard_op`，`op` 决定做什么。这一层把每个 op
//! 包成一个具名方法，顺带把「读不到分数」和「分数是 0」分开 ——
//! 裸槽两种情况都是一个空输出（契约 §5.2）。

use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, s};
use crate::sys;

/// 计分板显示位置。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DisplaySlot {
    Sidebar,
    List,
    BelowName,
}

impl DisplaySlot {
    /// 宿主按这个字符串分发。
    pub fn as_str(self) -> &'static str {
        match self {
            DisplaySlot::Sidebar => "sidebar",
            DisplaySlot::List => "list",
            DisplaySlot::BelowName => "belowname",
        }
    }
}

/// 一个记分项。
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Objective {
    pub name: String,
    pub display_name: String,
}

/// 计分板门面。零大小。
#[derive(Clone, Copy)]
pub struct Scoreboard(());

impl Scoreboard {
    pub fn get() -> Scoreboard {
        Scoreboard(())
    }

    /// 跑一个 `PIER_SB_*` 操作。
    fn op(&self, op: i32, a: &str, b: &str, n: i64) -> Result<String> {
        let f = crate::require_slot!(scoreboard_op, "计分板操作");
        call_out_str(|ctx, sink| unsafe { f(op, s(a), s(b), n, ctx, sink) }).ok_or_else(|| {
            Error(format!(
                "计分板操作 {op} 失败（记分项不存在，或参数不合法）"
            ))
        })
    }

    /// 建一个 dummy 记分项。同名已存在时失败。
    pub fn add_objective(&self, name: &str, display_name: &str) -> Result<()> {
        self.op(sys::PIER_SB_ADD_OBJECTIVE, name, display_name, 0)
            .map(|_| ())
    }

    pub fn remove_objective(&self, name: &str) -> Result<()> {
        self.op(sys::PIER_SB_REMOVE_OBJECTIVE, name, "", 0)
            .map(|_| ())
    }

    pub fn objectives(&self) -> Result<Vec<Objective>> {
        let text = self.op(sys::PIER_SB_LIST_OBJECTIVES, "", "", 0)?;
        if text.trim().is_empty() {
            return Ok(Vec::new());
        }
        let v = NbtValue::parse(&text).map_err(|e| Error(format!("记分项列表解析失败：{e}")))?;
        let Some(items) = v.as_list() else {
            return Err(Error(format!("记分项列表不是列表，而是 {}", v.type_name())));
        };
        Ok(items
            .iter()
            .filter_map(|o| {
                let name = o.opt_str("name")?.to_owned();
                let display_name = o.opt_str("display").unwrap_or(&name).to_owned();
                Some(Objective { name, display_name })
            })
            .collect())
    }

    /// 读一个分数。
    ///
    /// 这个人在这个记分项上**没有分数**时返回 `Ok(None)`；记分项根本不存在
    /// 才是 `Err`。裸槽把这两种和「分数是 0」压成同一个空输出，所以这里
    /// 按「输出是不是空」分开，并把不存在的记分项交给 op 自己报。
    pub fn score(&self, objective: &str, who: &str) -> Result<Option<i64>> {
        let text = self.op(sys::PIER_SB_GET_SCORE, objective, who, 0)?;
        if text.trim().is_empty() {
            return Ok(None);
        }
        text.trim()
            .parse::<i64>()
            .map(Some)
            .map_err(|e| Error(format!("分数 {text:?} 解析不成数字：{e}")))
    }

    fn write_score(&self, op: i32, objective: &str, who: &str, value: i64) -> Result<i64> {
        let text = self.op(op, objective, who, value)?;
        text.trim()
            .parse::<i64>()
            .map_err(|e| Error(format!("写分数之后的新值 {text:?} 解析不成数字：{e}")))
    }

    /// 设成 `value`，返回写完之后的值。
    pub fn set_score(&self, objective: &str, who: &str, value: i64) -> Result<i64> {
        self.write_score(sys::PIER_SB_SET_SCORE, objective, who, value)
    }

    pub fn add_score(&self, objective: &str, who: &str, delta: i64) -> Result<i64> {
        self.write_score(sys::PIER_SB_ADD_SCORE, objective, who, delta)
    }

    pub fn reduce_score(&self, objective: &str, who: &str, delta: i64) -> Result<i64> {
        self.write_score(sys::PIER_SB_REDUCE_SCORE, objective, who, delta)
    }

    /// 抹掉这个人在这个记分项上的分数（不是设成 0）。
    pub fn reset_score(&self, objective: &str, who: &str) -> Result<()> {
        self.op(sys::PIER_SB_RESET_SCORE, objective, who, 0)
            .map(|_| ())
    }

    pub fn set_display(&self, slot: DisplaySlot, objective: &str) -> Result<()> {
        self.op(sys::PIER_SB_SET_DISPLAY, slot.as_str(), objective, 0)
            .map(|_| ())
    }

    pub fn clear_display(&self, slot: DisplaySlot) -> Result<()> {
        self.op(sys::PIER_SB_CLEAR_DISPLAY, slot.as_str(), "", 0)
            .map(|_| ())
    }
}
