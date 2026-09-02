//! 实体动作 —— `PIER_AACT_*` 那一族。
//!
//! 它们共享同一个多路槽 `actor_action`，所以也共享同一组失败模式:
//! 实体不在了、参数不合法、或者宿主不认识这个动作号。

use crate::entity::Entity;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, s};
use crate::sys;

impl Entity {
    // ── 动作 ──────────────────────────────────────────────────

    /// 跑一个 `PIER_AACT_*` 动作，取回它的输出（多数动作没有输出，是空串）。
    pub fn act(&self, action: i32, sarg: &str, a: f64, b: f64, c: f64) -> Result<String> {
        let f = crate::require_slot!(actor_action, "执行实体动作");
        call_out_str(|ctx, sink| unsafe { f(self.0, action, s(sarg), a, b, c, ctx, sink) })
            .ok_or_else(|| {
                Error(format!(
                    "实体 {} 的动作 {action} 失败（实体不在了、参数不合法，或宿主不认识这个动作号）",
                    self.0
                ))
            })
    }

    fn act0(&self, action: i32) -> Result<()> {
        self.act(action, "", 0.0, 0.0, 0.0).map(|_| ())
    }

    /// 返回 `"0"` / `"1"` 的那几个动作（`ADD_TAG` / `HAS_TAG` …）。
    fn act_bool(&self, action: i32, sarg: &str) -> Result<bool> {
        let out = self.act(action, sarg, 0.0, 0.0, 0.0)?;
        match out.trim() {
            "1" => Ok(true),
            "0" => Ok(false),
            other => Err(Error(format!(
                "动作 {action} 应当回 \"0\" 或 \"1\"，实际回了 {other:?}"
            ))),
        }
    }

    pub fn kill(&self) -> Result<()> {
        self.act0(sys::PIER_AACT_KILL)
    }
    pub fn despawn(&self) -> Result<()> {
        self.act0(sys::PIER_AACT_DESPAWN)
    }
    pub fn clear_effects(&self) -> Result<()> {
        self.act0(sys::PIER_AACT_CLEAR_EFFECTS)
    }
    pub fn stop_fire(&self) -> Result<()> {
        self.act0(sys::PIER_AACT_STOP_FIRE)
    }
    pub fn remove_all_passengers(&self) -> Result<()> {
        self.act0(sys::PIER_AACT_REMOVE_ALL_PASSENGERS)
    }

    pub fn heal(&self, amount: f64) -> Result<()> {
        self.act(sys::PIER_AACT_HEAL, "", amount, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn hurt(&self, amount: f64) -> Result<()> {
        self.act(sys::PIER_AACT_HURT, "", amount, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn burn(&self, damage: f64) -> Result<()> {
        self.act(sys::PIER_AACT_BURN, "", damage, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn set_on_fire(&self, seconds: i32) -> Result<()> {
        self.act(sys::PIER_AACT_SET_ON_FIRE, "", seconds as f64, 0.0, 0.0)
            .map(|_| ())
    }

    /// 传送到同一维度的另一处。
    pub fn teleport(&self, x: f64, y: f64, z: f64) -> Result<()> {
        let dim = self.dimension()?;
        self.teleport_to(dim, x, y, z)
    }

    /// 传送到指定维度。自定义维度（id ≥ 3）也走这里。
    pub fn teleport_to(&self, dim: i32, x: f64, y: f64, z: f64) -> Result<()> {
        self.act(sys::PIER_AACT_TELEPORT, &dim.to_string(), x, y, z)
            .map(|_| ())
    }

    pub fn set_rotation(&self, pitch: f64, yaw: f64) -> Result<()> {
        self.act(sys::PIER_AACT_SET_ROTATION, "", pitch, yaw, 0.0)
            .map(|_| ())
    }
    pub fn set_velocity(&self, x: f64, y: f64, z: f64) -> Result<()> {
        self.act(sys::PIER_AACT_SET_VELOCITY, "", x, y, z)
            .map(|_| ())
    }
    pub fn apply_impulse(&self, x: f64, y: f64, z: f64) -> Result<()> {
        self.act(sys::PIER_AACT_APPLY_IMPULSE, "", x, y, z)
            .map(|_| ())
    }

    pub fn set_name_tag(&self, name: &str) -> Result<()> {
        self.act(sys::PIER_AACT_SET_NAME_TAG, name, 0.0, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn set_name_tag_visible(&self, on: bool) -> Result<()> {
        self.act(
            sys::PIER_AACT_SET_NAME_TAG_VISIBLE,
            "",
            if on { 1.0 } else { 0.0 },
            0.0,
            0.0,
        )
        .map(|_| ())
    }
    pub fn set_score_tag(&self, text: &str) -> Result<()> {
        self.act(sys::PIER_AACT_SET_SCORE_TAG, text, 0.0, 0.0, 0.0)
            .map(|_| ())
    }

    pub fn add_tag(&self, tag: &str) -> Result<bool> {
        self.act_bool(sys::PIER_AACT_ADD_TAG, tag)
    }
    pub fn remove_tag(&self, tag: &str) -> Result<bool> {
        self.act_bool(sys::PIER_AACT_REMOVE_TAG, tag)
    }
    pub fn has_tag(&self, tag: &str) -> Result<bool> {
        self.act_bool(sys::PIER_AACT_HAS_TAG, tag)
    }

    /// 加一条状态效果。
    pub fn add_effect(
        &self,
        effect: &str,
        ticks: i32,
        amplifier: i32,
        visible: bool,
    ) -> Result<()> {
        self.act(
            sys::PIER_AACT_ADD_EFFECT,
            effect,
            ticks as f64,
            amplifier as f64,
            if visible { 1.0 } else { 0.0 },
        )
        .map(|_| ())
    }

    pub fn remove_effect(&self, effect: &str) -> Result<()> {
        self.act(sys::PIER_AACT_REMOVE_EFFECT, effect, 0.0, 0.0, 0.0)
            .map(|_| ())
    }

    /// 读一个属性（`minecraft:health` 这样的名字）的当前值。
    pub fn attribute(&self, name: &str) -> Result<f64> {
        let out = self.act(sys::PIER_AACT_ATTRIBUTE_GET, name, 0.0, 0.0, 0.0)?;
        out.trim()
            .parse::<f64>()
            .map_err(|e| Error(format!("属性 {name} 的值 {out:?} 解析不成数字：{e}")))
    }

    pub fn set_variant(&self, v: i32) -> Result<()> {
        self.act(sys::PIER_AACT_SET_VARIANT, "", v as f64, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn set_mark_variant(&self, v: i32) -> Result<()> {
        self.act(sys::PIER_AACT_SET_MARK_VARIANT, "", v as f64, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn set_persistent(&self) -> Result<()> {
        self.act0(sys::PIER_AACT_SET_PERSISTENT)
    }
    pub fn set_invisible(&self, on: bool) -> Result<()> {
        self.act(
            sys::PIER_AACT_SET_INVISIBLE,
            "",
            if on { 1.0 } else { 0.0 },
            0.0,
            0.0,
        )
        .map(|_| ())
    }
    pub fn set_sneaking(&self, on: bool) -> Result<()> {
        self.act(
            sys::PIER_AACT_SET_SNEAKING,
            "",
            if on { 1.0 } else { 0.0 },
            0.0,
            0.0,
        )
        .map(|_| ())
    }
    pub fn set_skin_id(&self, id: i32) -> Result<()> {
        self.act(sys::PIER_AACT_SET_SKIN_ID, "", id as f64, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn set_strength(&self, v: i32) -> Result<()> {
        self.act(sys::PIER_AACT_SET_STRENGTH, "", v as f64, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn set_target(&self, target: Entity) -> Result<()> {
        self.act(sys::PIER_AACT_SET_TARGET, "", target.0 as f64, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn set_owner(&self, owner: Entity) -> Result<()> {
        self.act(sys::PIER_AACT_SET_OWNER, "", owner.0 as f64, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn set_leash_holder(&self, holder: Entity) -> Result<()> {
        self.act(
            sys::PIER_AACT_SET_LEASH_HOLDER,
            "",
            holder.0 as f64,
            0.0,
            0.0,
        )
        .map(|_| ())
    }
    pub fn execute_event(&self, event: &str) -> Result<()> {
        self.act(sys::PIER_AACT_EXECUTE_EVENT, event, 0.0, 0.0, 0.0)
            .map(|_| ())
    }
}
