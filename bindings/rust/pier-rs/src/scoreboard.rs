//! Scoreboards.
//!
//! Everything goes through one multiplexed slot, `scoreboard_op`, where `op` decides what
//! happens. This layer wraps each op as a named method and separates a score that cannot be
//! read from a score of 0, which the bare slot reports as the same empty output
//! (contract §5.2).

use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, s};
use crate::sys;

/// The display slot of a scoreboard.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DisplaySlot {
    Sidebar,
    List,
    BelowName,
}

impl DisplaySlot {
    /// The host dispatches on this string.
    pub fn as_str(self) -> &'static str {
        match self {
            DisplaySlot::Sidebar => "sidebar",
            DisplaySlot::List => "list",
            DisplaySlot::BelowName => "belowname",
        }
    }
}

/// One objective.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Objective {
    pub name: String,
    pub display_name: String,
}

/// The scoreboard facade. Zero sized.
#[derive(Clone, Copy)]
pub struct Scoreboard(());

impl Scoreboard {
    pub fn get() -> Scoreboard {
        Scoreboard(())
    }

    /// Runs one `PIER_SB_*` operation.
    fn op(&self, op: i32, a: &str, b: &str, n: i64) -> Result<String> {
        let f = crate::require_slot!(scoreboard_op, "a scoreboard operation");
        call_out_str(|ctx, sink| unsafe { f(op, s(a), s(b), n, ctx, sink) }).ok_or_else(|| {
            Error(format!(
                "scoreboard operation {op} failed: the objective does not exist, or an argument is invalid"
            ))
        })
    }

    /// Creates a dummy objective. It fails when one of that name already exists.
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
        let v = NbtValue::parse(&text)
            .map_err(|e| Error(format!("parsing the objective list failed: {e}")))?;
        let Some(items) = v.as_list() else {
            return Err(Error(format!(
                "the objective list is not a list but {}",
                v.type_name()
            )));
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

    /// Reads one score.
    ///
    /// A person with no score on that objective gives `Ok(None)`, and only an objective that
    /// does not exist is an `Err`. The bare slot collapses both of those and a score of 0 into
    /// the same empty output, so this separates them by whether the output is empty and leaves
    /// a nonexistent objective for the op itself to report.
    pub fn score(&self, objective: &str, who: &str) -> Result<Option<i64>> {
        let text = self.op(sys::PIER_SB_GET_SCORE, objective, who, 0)?;
        if text.trim().is_empty() {
            return Ok(None);
        }
        text.trim().parse::<i64>().map(Some).map_err(|e| {
            Error(format!(
                "the score {text:?} does not parse as a number: {e}"
            ))
        })
    }

    fn write_score(&self, op: i32, objective: &str, who: &str, value: i64) -> Result<i64> {
        let text = self.op(op, objective, who, value)?;
        text.trim().parse::<i64>().map_err(|e| {
            Error(format!(
                "the new value {text:?} after the write does not parse as a number: {e}"
            ))
        })
    }

    /// Sets it to `value` and returns the value after the write.
    pub fn set_score(&self, objective: &str, who: &str, value: i64) -> Result<i64> {
        self.write_score(sys::PIER_SB_SET_SCORE, objective, who, value)
    }

    pub fn add_score(&self, objective: &str, who: &str, delta: i64) -> Result<i64> {
        self.write_score(sys::PIER_SB_ADD_SCORE, objective, who, delta)
    }

    pub fn reduce_score(&self, objective: &str, who: &str, delta: i64) -> Result<i64> {
        self.write_score(sys::PIER_SB_REDUCE_SCORE, objective, who, delta)
    }

    /// Erases the score of this person on this objective, which is not setting it to 0.
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
