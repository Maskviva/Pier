//! Actor actions: the `PIER_AACT_*` family.
//!
//! They share one multiplexed slot, `actor_action`, and therefore share the same failure
//! modes: the actor is gone, an argument is invalid, or the host does not recognize the
//! action number.

use crate::entity::Entity;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, s};
use crate::sys;

impl Entity {
    // Actions

    /// Runs one `PIER_AACT_*` action and returns its output, which is an empty string for most
    /// actions.
    pub fn act(&self, action: i32, sarg: &str, a: f64, b: f64, c: f64) -> Result<String> {
        let f = crate::require_slot!(actor_action, "running an actor action");
        call_out_str(|ctx, sink| unsafe { f(self.0, action, s(sarg), a, b, c, ctx, sink) })
            .ok_or_else(|| {
                Error(format!(
                    "action {action} on actor {} failed: it is gone, an argument is invalid, or the host does not recognize the action number",
                    self.0
                ))
            })
    }

    fn act0(&self, action: i32) -> Result<()> {
        self.act(action, "", 0.0, 0.0, 0.0).map(|_| ())
    }

    /// The actions that answer `"0"` or `"1"`, such as `ADD_TAG` and `HAS_TAG`.
    fn act_bool(&self, action: i32, sarg: &str) -> Result<bool> {
        let out = self.act(action, sarg, 0.0, 0.0, 0.0)?;
        match out.trim() {
            "1" => Ok(true),
            "0" => Ok(false),
            other => Err(Error(format!(
                "action {action} should answer \"0\" or \"1\" and answered {other:?}"
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

    /// Teleports elsewhere within the same dimension.
    pub fn teleport(&self, x: f64, y: f64, z: f64) -> Result<()> {
        let dim = self.dimension()?;
        self.teleport_to(dim, x, y, z)
    }

    /// Teleports into a given dimension. A custom dimension, with an id of 3 or above, goes
    /// through here too.
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

    /// Adds one status effect.
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

    /// Reads the current value of one attribute, named as `minecraft:health` is.
    pub fn attribute(&self, name: &str) -> Result<f64> {
        let out = self.act(sys::PIER_AACT_ATTRIBUTE_GET, name, 0.0, 0.0, 0.0)?;
        out.trim().parse::<f64>().map_err(|e| {
            Error(format!(
                "the value {out:?} of attribute {name} does not parse as a number: {e}"
            ))
        })
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
