//! Actors: everything addressed by an `ActorUniqueID`, players included.
//!
//! # An id is an identity and not a pointer
//!
//! An [`Entity`] holds one `i64`. The host looks the live table up again on every call, so
//! an `Entity` value can be kept across ticks: once the actor dies a call returns `Err`
//! rather than jumping into freed memory. The cost is one lookup per call, so a hot path
//! caches the result itself.
//!
//! # A player passes through here to use actor capabilities
//!
//! `Player::as_entity()` goes through `player_resolve` for the id. The reverse does not
//! hold: an actor id is not necessarily a player, and there is no slot resolving an id back
//! into a selector.

mod actions;
mod props;
mod relations;

use core::ffi::c_void;

use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, r_owned};
use crate::sys;
use crate::types::{PositionF64, RayHit};

/// One status effect.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Effect {
    pub id: String,
    pub ticks: i32,
    pub amplifier: i32,
    pub visible: bool,
}

/// The axis-aligned bounding box of an actor.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Aabb {
    pub min: PositionF64,
    pub max: PositionF64,
}

/// One entry `list_actors` reports.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ActorEntry {
    pub id: i64,
    pub type_name: String,
}

/// One actor. A zero-cost wrapper around an `ActorUniqueID`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub struct Entity(i64);

impl Entity {
    pub fn from_id(id: i64) -> Entity {
        Entity(id)
    }

    pub fn id(&self) -> i64 {
        self.0
    }

    /// Enumerates live actors. A `dim` of `None` spans every dimension.
    ///
    /// This slot has no failure bit and reports nothing while the level is not ready, so an
    /// empty table means either that the dimension holds no actor or that the level has not
    /// come up, and a caller tells them apart with `Host::gaming_status()`.
    pub fn list(dim: Option<i32>) -> Vec<ActorEntry> {
        if !crate::has_slot!(list_actors) {
            return Vec::new();
        }
        let Some(f) = crate::__rt::api().list_actors else {
            return Vec::new();
        };
        let mut out: Vec<ActorEntry> = Vec::new();
        unsafe {
            f(
                dim.unwrap_or(-1),
                (&mut out as *mut Vec<ActorEntry>).cast(),
                push_actor,
            )
        };
        out
    }

    /// Whether this id still points at a live actor.
    ///
    /// The criterion is whether the type name can be read: every actor that resolves has one,
    /// and one that does not makes `actor_get_str` return false.
    pub fn exists(&self) -> bool {
        self.text(sys::PIER_ASTR_TYPE_NAME).is_ok()
    }

    pub fn snapshot(&self) -> Result<NbtValue> {
        let f = crate::require_slot!(actor_snapshot, "reading an actor snapshot");
        let text = call_out_str(|ctx, sink| unsafe { f(self.0, ctx, sink) })
            .ok_or_else(|| Error(format!("actor {} does not resolve, so no snapshot can be taken", self.0)))?;
        NbtValue::parse(&text).map_err(|e| Error(format!("parsing the actor snapshot SNBT failed: {e}")))
    }

    // Properties

    /// Reads a `PIER_APROP_*` numeric property.
    pub fn num(&self, prop: i32) -> Result<f64> {
        let f = crate::require_slot!(actor_get_num, "reading a numeric actor property");
        let mut out = 0.0f64;
        let ok = unsafe { f(self.0, prop, &mut out) };
        if ok {
            Ok(out)
        } else {
            Err(Error(format!(
                "property {prop} of actor {} could not be read: it is gone, or the host does not recognize the property number",
                self.0
            )))
        }
    }

    /// Reads a `PIER_ASTR_*` string property.
    pub fn text(&self, prop: i32) -> Result<String> {
        let f = crate::require_slot!(actor_get_str, "reading a string actor property");
        call_out_str(|ctx, sink| unsafe { f(self.0, prop, ctx, sink) })
            .ok_or_else(|| Error(format!("string property {prop} of actor {} could not be read", self.0)))
    }

    /// The position, from `Actor::getPosition`. For the feet coordinate of a player see
    /// [`Entity::feet_pos`].
    pub fn pos(&self) -> Result<PositionF64> {
        Ok((
            self.num(sys::PIER_APROP_POS_X)?,
            self.num(sys::PIER_APROP_POS_Y)?,
            self.num(sys::PIER_APROP_POS_Z)?,
        ))
    }

    pub fn feet_pos(&self) -> Result<PositionF64> {
        Ok((
            self.num(sys::PIER_APROP_FEET_X)?,
            self.num(sys::PIER_APROP_FEET_Y)?,
            self.num(sys::PIER_APROP_FEET_Z)?,
        ))
    }

    pub fn head_pos(&self) -> Result<PositionF64> {
        Ok((
            self.num(sys::PIER_APROP_HEAD_X)?,
            self.num(sys::PIER_APROP_HEAD_Y)?,
            self.num(sys::PIER_APROP_HEAD_Z)?,
        ))
    }

    pub fn velocity(&self) -> Result<PositionF64> {
        Ok((
            self.num(sys::PIER_APROP_VEL_X)?,
            self.num(sys::PIER_APROP_VEL_Y)?,
            self.num(sys::PIER_APROP_VEL_Z)?,
        ))
    }

    /// The unit vector of the line of sight.
    pub fn view_vector(&self) -> Result<PositionF64> {
        Ok((
            self.num(sys::PIER_APROP_VIEW_X)?,
            self.num(sys::PIER_APROP_VIEW_Y)?,
            self.num(sys::PIER_APROP_VIEW_Z)?,
        ))
    }

    /// `(pitch, yaw)`.
    pub fn rotation(&self) -> Result<(f64, f64)> {
        Ok((
            self.num(sys::PIER_APROP_ROT_PITCH)?,
            self.num(sys::PIER_APROP_ROT_YAW)?,
        ))
    }
}

impl std::fmt::Display for Entity {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "entity#{}", self.0)
    }
}

/// # Safety
/// `ctx` must be a valid `*mut Vec<ActorEntry>`.
unsafe extern "C" fn push_actor(ctx: *mut c_void, id: sys::PierActorId, type_name: sys::PierStr) {
    (*ctx.cast::<Vec<ActorEntry>>()).push(ActorEntry {
        id,
        type_name: r_owned(type_name),
    });
}

/// Parses the reply of both ray slots.
///
/// The two shapes differ only in the block branch, where one gives `block` plus `facing`
/// and the other only `pos`, so one parser takes both: a missing field is filled from the
/// other and only both missing is an error.
pub(crate) fn parse_ray_hit(text: &str) -> Result<RayHit> {
    let v = NbtValue::parse(text).map_err(|e| Error(format!("parsing the ray result SNBT failed: {e}")))?;
    let kind = v.opt_str("type").unwrap_or("none").to_owned();
    let pos = v.get_vec3("pos").unwrap_or((0.0, 0.0, 0.0));
    match kind.as_str() {
        "entity" => {
            let id = v
                .opt_i64("entity_id")
                .or_else(|| v.opt_i64("entity"))
                .ok_or_else(|| Error("the ray reported hitting an actor and gave no actor id".to_owned()))?;
            Ok(RayHit::Entity { id, pos })
        }
        "block" => {
            let block = match v.get_block_pos("block") {
                Ok(b) => b,
                // The branch giving only an exact coordinate: floored into the cell it is in.
                Err(_) => (
                    pos.0.floor() as i32,
                    pos.1.floor() as i32,
                    pos.2.floor() as i32,
                ),
            };
            Ok(RayHit::Block {
                block,
                facing: v.opt_i32("facing").unwrap_or(-1),
                name: v.opt_str("block_name").unwrap_or_default().to_owned(),
                pos,
            })
        }
        "none" => Ok(RayHit::None),
        other => Err(Error(format!("the type of the ray result is an unrecognized {other:?}"))),
    }
}
