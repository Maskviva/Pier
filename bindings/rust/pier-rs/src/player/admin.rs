//! Kicking, changing the game mode, teleporting, ability bits and permissions.
//!
//! What they share is changing the player's own state, and almost all of them need server
//! permission.

use crate::entity::Entity;
use crate::item::ItemStack;
use crate::player::Player;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, s};
use crate::sys;
use crate::types::{Ability, AbilityValue, GameMode, PlayerPermission};

impl Player {
    // Administration

    pub fn disconnect(&self, reason: &str) -> Result<()> {
        let f = crate::require_slot!(player_disconnect, "kicking a player");
        let ok = unsafe { f(self.sel.raw(), s(reason)) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!("player {} is offline and cannot be kicked", self.sel)))
        }
    }

    pub fn set_gamemode(&self, mode: GameMode) -> Result<()> {
        let f = crate::require_slot!(player_set_gamemode, "setting the game mode of a player");
        let ok = unsafe { f(self.sel.raw(), mode.as_i32()) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "the game mode of player {} could not be set: they are offline",
                self.sel
            )))
        }
    }

    /// Teleports. A custom dimension, with an id of 3 or above, goes through here too, and
    /// when the dimension bridge cannot build a matching instance the host fails rather than
    /// dropping the person into a mismatched dimension.
    pub fn teleport(&self, dim: i32, x: f64, y: f64, z: f64) -> Result<()> {
        let f = crate::require_slot!(player_teleport, "teleporting a player");
        let ok = unsafe { f(self.sel.raw(), dim, x, y, z) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "player {} could not be teleported: they are offline, or dimension {dim} is unavailable",
                self.sel
            )))
        }
    }

    /// Runs a `PIER_PACT_*` action and returns its output.
    pub fn act(&self, action: i32, sarg: &str, a: f64, b: f64, c: f64) -> Result<String> {
        let f = crate::require_slot!(player_action, "running a player action");
        call_out_str(|ctx, sink| unsafe { f(self.sel.raw(), action, s(sarg), a, b, c, ctx, sink) })
            .ok_or_else(|| {
                Error(format!(
                    "action {action} on player {} failed: they are offline, an argument is invalid, or the host does not recognize the action number",
                    self.sel
                ))
            })
    }

    /// Sets one ability bit.
    ///
    /// Passing a boolean ability where a floating-point one belongs raises no error and is
    /// simply written under the other interpretation, so [`Ability::is_float`] stops it first.
    pub fn set_ability<V: AbilityValue>(&self, ability: Ability, value: V) -> Result<()> {
        if ability.is_float() == V::IS_BOOL {
            return Err(Error(format!(
                "the ability bit {ability:?} wants a {} value and was given a {} value",
                if ability.is_float() {
                    "floating-point"
                } else {
                    "boolean"
                },
                if V::IS_BOOL { "boolean" } else { "numeric" }
            )));
        }
        self.set_ability_raw(ability.as_i32(), value.as_f64())
    }

    /// Sets an ability bit by index, for when the host is newer than this layer and has extra
    /// bits.
    pub fn set_ability_raw(&self, index: i32, value: f64) -> Result<()> {
        self.act(sys::PIER_PACT_SET_ABILITY, "", index as f64, value, 0.0)
            .map(|_| ())
    }

    pub fn can_use_ability(&self, ability: Ability) -> Result<bool> {
        let out = self.act(
            sys::PIER_PACT_CAN_USE_ABILITY,
            "",
            ability.as_i32() as f64,
            0.0,
            0.0,
        )?;
        match out.trim() {
            "1" => Ok(true),
            "0" => Ok(false),
            other => Err(Error(format!(
                "an ability bit query should answer \"0\" or \"1\" and answered {other:?}"
            ))),
        }
    }

    pub fn set_permission_level(&self, level: PlayerPermission) -> Result<()> {
        self.act(
            sys::PIER_PACT_SET_PERMISSION_LEVEL,
            "",
            level.as_i32() as f64,
            0.0,
            0.0,
        )
        .map(|_| ())
    }

    pub fn set_selected_slot(&self, slot: i32) -> Result<()> {
        self.act(sys::PIER_PACT_SET_SELECTED_SLOT, "", slot as f64, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn give_item(&self, item: &ItemStack) -> Result<()> {
        self.act(sys::PIER_PACT_GIVE_ITEM, item.snbt(), 0.0, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn set_spawn_point(&self, dim: i32, x: i32, y: i32, z: i32) -> Result<()> {
        self.act(
            sys::PIER_PACT_SET_SPAWN_POINT,
            &dim.to_string(),
            x as f64,
            y as f64,
            z as f64,
        )
        .map(|_| ())
    }
    pub fn add_experience(&self, xp: i32) -> Result<()> {
        self.act(sys::PIER_PACT_ADD_EXPERIENCE, "", xp as f64, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn add_levels(&self, levels: i32) -> Result<()> {
        self.act(sys::PIER_PACT_ADD_LEVELS, "", levels as f64, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn set_chunk_radius(&self, radius: i32) -> Result<()> {
        self.act(sys::PIER_PACT_SET_CHUNK_RADIUS, "", radius as f64, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn set_enchantment_seed(&self, seed: i32) -> Result<()> {
        self.act(
            sys::PIER_PACT_SET_ENCHANTMENT_SEED,
            "",
            seed as f64,
            0.0,
            0.0,
        )
        .map(|_| ())
    }
    pub fn play_emote(&self, piece_id: &str) -> Result<()> {
        self.act(sys::PIER_PACT_PLAY_EMOTE, piece_id, 0.0, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn resend_all_chunks(&self) -> Result<()> {
        self.act(sys::PIER_PACT_RESEND_ALL_CHUNKS, "", 0.0, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn open_inventory(&self) -> Result<()> {
        self.act(sys::PIER_PACT_OPEN_INVENTORY, "", 0.0, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn start_riding(&self, vehicle: Entity) -> Result<()> {
        self.act(
            sys::PIER_PACT_START_RIDING,
            "",
            vehicle.id() as f64,
            0.0,
            0.0,
        )
        .map(|_| ())
    }
    pub fn stop_riding(&self) -> Result<()> {
        self.act(sys::PIER_PACT_STOP_RIDING, "", 0.0, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn attack(&self, target: Entity) -> Result<()> {
        self.act(sys::PIER_PACT_ATTACK, "", target.id() as f64, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn interact(&self, target: Entity) -> Result<()> {
        self.act(sys::PIER_PACT_INTERACT, "", target.id() as f64, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn drop_item(&self, item: &ItemStack, random: bool) -> Result<()> {
        self.act(
            sys::PIER_PACT_DROP,
            item.snbt(),
            if random { 1.0 } else { 0.0 },
            0.0,
            0.0,
        )
        .map(|_| ())
    }

    /// A per-player sidebar. `lines` runs from top to bottom.
    pub fn set_sidebar(&self, objective: &str, title: &str, lines: &[String]) -> Result<()> {
        // The host takes the newline-separated form "objective\ntitle\nline...".
        let mut payload = String::with_capacity(objective.len() + title.len() + 16);
        payload.push_str(objective);
        payload.push('\n');
        payload.push_str(title);
        for l in lines {
            payload.push('\n');
            payload.push_str(l);
        }
        self.act(sys::PIER_PACT_SIDEBAR_SET, &payload, 0.0, 0.0, 0.0)
            .map(|_| ())
    }

    pub fn clear_sidebar(&self, objective: &str) -> Result<()> {
        self.act(sys::PIER_PACT_SIDEBAR_CLEAR, objective, 0.0, 0.0, 0.0)
            .map(|_| ())
    }
}
