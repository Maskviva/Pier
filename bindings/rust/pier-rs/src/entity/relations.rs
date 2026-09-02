//! The relations between actors, the equipment and effects on one, and the ray cast from
//! its eyes.
//!
//! They belong together because each answers what relation this actor has to something
//! else rather than what it is like itself, which is in `props.rs`.

use super::parse_ray_hit;
use crate::entity::{Aabb, Effect, Entity};
use crate::item::ItemStack;
use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, s};
use crate::sys;
use crate::types::{EquipSlot, RayHit};

impl Entity {
    // Relations

    /// The vehicle being ridden. Riding nothing gives `Ok(None)` and only a missing slot is
    /// an `Err`.
    pub fn vehicle(&self) -> Result<Option<Entity>> {
        let f = crate::require_slot!(actor_get_vehicle, "querying the vehicle of an actor");
        Ok(self.related(f))
    }
    pub fn first_passenger(&self) -> Result<Option<Entity>> {
        let f = crate::require_slot!(
            actor_get_first_passenger,
            "querying the first passenger of an actor"
        );
        Ok(self.related(f))
    }
    pub fn owner(&self) -> Result<Option<Entity>> {
        let f = crate::require_slot!(actor_get_owner, "querying the owner of an actor");
        Ok(self.related(f))
    }
    pub fn target(&self) -> Result<Option<Entity>> {
        let f = crate::require_slot!(actor_get_target, "querying the attack target of an actor");
        Ok(self.related(f))
    }

    /// The shared read shape of the four relation slots: a `false` means there is no such
    /// relation and is not an error.
    ///
    /// Both gates stay at their own call sites, expanded by `require_slot!`. Taking the
    /// function pointer out and handing it to a shared helper is fine, but taking the pointer
    /// itself has to pass the length gate first: with the host table too short to reach the
    /// field, reading it is an out-of-bounds read, and what comes back often looks like a
    /// valid function pointer.
    fn related(
        self,
        f: unsafe extern "C" fn(sys::PierActorId, *mut sys::PierActorId) -> bool,
    ) -> Option<Entity> {
        let mut out: sys::PierActorId = 0;
        if unsafe { f(self.0, &mut out) } {
            Some(Entity(out))
        } else {
            None
        }
    }

    /// The distance between two actors. The host returns a failure across dimensions.
    pub fn distance_to(&self, other: Entity) -> Result<f64> {
        let f = crate::require_slot!(actor_distance_to, "computing the distance between actors");
        let mut out = 0.0f64;
        let ok = unsafe { f(self.0, other.0, &mut out) };
        if ok {
            Ok(out)
        } else {
            Err(Error(format!(
                "the distance from actor {} to {} could not be computed: one of them is gone, or they are not in the same dimension",
                self.0, other.0
            )))
        }
    }

    /// The bounding box.
    pub fn aabb(&self) -> Result<Aabb> {
        let f = crate::require_slot!(actor_get_aabb, "reading the bounding box of an actor");
        let text = call_out_str(|ctx, sink| unsafe { f(self.0, ctx, sink) }).ok_or_else(|| {
            Error(format!(
                "the bounding box of actor {} could not be read",
                self.0
            ))
        })?;
        let v = NbtValue::parse(&text)
            .map_err(|e| Error(format!("parsing the bounding box SNBT failed: {e}")))?;
        Ok(Aabb {
            min: v.get_vec3("min")?,
            max: v.get_vec3("max")?,
        })
    }

    /// Clones one to a given position.
    pub fn clone_at(&self, dim: i32, x: f64, y: f64, z: f64) -> Result<Entity> {
        let f = crate::require_slot!(actor_clone, "cloning an actor");
        let mut out: sys::PierActorId = 0;
        let ok = unsafe { f(self.0, dim, x, y, z, &mut out) };
        if ok {
            Ok(Entity(out))
        } else {
            Err(Error(format!(
                "actor {} could not be cloned: it is gone, or the target dimension is unavailable",
                self.0
            )))
        }
    }

    // Equipment and effects

    pub fn equipped_item(&self, slot: EquipSlot) -> Result<ItemStack> {
        let f = crate::require_slot!(actor_get_equipped_item, "reading the equipment of an actor");
        let snbt = call_out_str(|ctx, sink| unsafe { f(self.0, slot.as_i32(), ctx, sink) })
            .ok_or_else(|| {
                Error(format!(
                    "the {slot:?} equipment of actor {} could not be read",
                    self.0
                ))
            })?;
        Ok(ItemStack::from_snbt(snbt))
    }

    pub fn set_equipped_item(&self, slot: EquipSlot, item: &ItemStack) -> Result<()> {
        let f = crate::require_slot!(actor_set_equipped_item, "writing the equipment of an actor");
        let ok = unsafe { f(self.0, slot.as_i32(), s(item.snbt())) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "the {slot:?} equipment of actor {} could not be written: it is gone, or the item SNBT is invalid",
                self.0
            )))
        }
    }

    /// Every status effect on it.
    pub fn effects(&self) -> Result<Vec<Effect>> {
        let f = crate::require_slot!(actor_get_effects, "reading the status effects of an actor");
        let text = call_out_str(|ctx, sink| unsafe { f(self.0, ctx, sink) }).ok_or_else(|| {
            Error(format!(
                "the status effects of actor {} could not be read",
                self.0
            ))
        })?;
        let v = NbtValue::parse(&text)
            .map_err(|e| Error(format!("parsing the status effect SNBT failed: {e}")))?;
        let Some(items) = v.as_list() else {
            return Err(Error(format!(
                "the status effects are not a list but {}",
                v.type_name()
            )));
        };
        Ok(items
            .iter()
            .map(|e| Effect {
                // The id may be a number or a name depending on the BDS version, and both are
                // taken.
                id: match e.get("id") {
                    Some(NbtValue::String(s)) => s.clone(),
                    Some(other) => other.as_i64().map(|n| n.to_string()).unwrap_or_default(),
                    None => String::new(),
                },
                ticks: e.opt_i32("ticks").unwrap_or(0),
                amplifier: e.opt_i32("amplifier").unwrap_or(0),
                visible: e.opt_bool("visible").unwrap_or(true),
            })
            .collect())
    }

    /// Reads one bit of `ActorFlags`.
    ///
    /// On the ABI this slot collapses the actor being gone and the bit being false into the
    /// same `false`, the shape contract §5.2 opposes. That signature is already released, so
    /// this only states it truthfully.
    /// Call [`Entity::exists`] first when the two must be told apart.
    pub fn status_flag(&self, flag_index: i32) -> Result<bool> {
        let f = crate::require_slot!(actor_get_status_flag, "reading a status bit of an actor");
        Ok(unsafe { f(self.0, flag_index) })
    }

    pub fn set_status_flag(&self, flag_index: i32, value: bool) -> Result<()> {
        let f = crate::require_slot!(actor_set_status_flag, "writing a status bit of an actor");
        let ok = unsafe { f(self.0, flag_index, value) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "status bit {flag_index} of actor {} could not be written: it is gone, or the bit is read-only",
                self.0
            )))
        }
    }

    // Rays

    /// Casts a ray along the line of sight of this actor, reporting the hit as an exact
    /// coordinate.
    pub fn trace_ray(
        self,
        max_dist: f32,
        include_actors: bool,
        include_blocks: bool,
    ) -> Result<RayHit> {
        let f = crate::require_slot!(actor_trace_ray, "tracing a ray");
        let text = call_out_str(|ctx, sink| unsafe {
            f(self.0, max_dist, include_actors, include_blocks, ctx, sink)
        })
        .ok_or_else(|| Error(format!("the ray trace of actor {} failed", self.0)))?;
        parse_ray_hit(&text)
    }

    /// As above, reporting the hit as a block cell and carrying the face hit.
    ///
    /// Both slots are kept because they answer different questions: placing a block needs a
    /// cell coordinate and a face while drawing a particle needs an exact coordinate, and
    /// flooring the latter into the former is off by one cell at a block boundary.
    pub fn trace_ray_blocks(
        self,
        max_dist: f32,
        include_actors: bool,
        include_blocks: bool,
    ) -> Result<RayHit> {
        let f = crate::require_slot!(edit_trace_ray, "tracing a ray in block cells");
        let text = call_out_str(|ctx, sink| unsafe {
            f(self.0, max_dist, include_actors, include_blocks, ctx, sink)
        })
        .ok_or_else(|| Error(format!("the ray trace of actor {} failed", self.0)))?;
        parse_ray_hit(&text)
    }
}
