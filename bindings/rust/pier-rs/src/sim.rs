//! Simulated players: a real `ServerPlayer` the server builds.
//!
//! Once built, every by-name player API applies to it: teleporting, health, inventory and
//! kicking. Only the family of make-it-do-something actions unique to it lives here.
//!
//! # Actions go through one multiplexed slot
//!
//! `sim_do` takes a verb plus argument SNBT, and the verb table grows on the host side
//! without taking a new table slot. The cost is that a misspelled verb is reported only at
//! runtime, so each verb is wrapped here as a named method.
//!
//! # A real player is never driven
//!
//! The host gates on `isSimulatedPlayer()` and a call against a real player fails outright.

use crate::nbt::NbtValue;
use crate::player::Player;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{collect_strs, s};
use crate::sel::PlayerSel;

/// One simulated player.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SimPlayer {
    name: String,
}

/// Builds a simulated player. It fails when the name is taken by a real player.
pub fn spawn(name: &str, dim: i32, x: f64, y: f64, z: f64) -> Result<SimPlayer> {
    let f = crate::require_slot!(sim_spawn, "spawning a simulated player");
    if unsafe { f(s(name), dim, x, y, z) } {
        Ok(SimPlayer {
            name: name.to_owned(),
        })
    } else {
        Err(Error(format!(
            "the simulated player {name} could not be spawned: the name is taken, the level is not ready, or dimension {dim} is unavailable"
        )))
    }
}

/// The simulated players currently alive.
///
/// A simulated player survives a restart with the save while an in-memory handle does not,
/// so this is how they are found again after a restart.
pub fn list() -> Vec<SimPlayer> {
    if !crate::has_slot!(sim_list) {
        return Vec::new();
    }
    let Some(f) = crate::__rt::api().sim_list else {
        return Vec::new();
    };
    collect_strs(|ctx, sink| unsafe { f(ctx, sink) })
        .into_iter()
        .map(|name| SimPlayer { name })
        .collect()
}

impl SimPlayer {
    /// Attaches by name to a simulated player that already exists. It does not check whether
    /// it really exists; [`SimPlayer::is_simulated`] does that.
    pub fn by_name(name: impl Into<String>) -> SimPlayer {
        SimPlayer { name: name.into() }
    }

    pub fn name(&self) -> &str {
        &self.name
    }

    /// Uses it as an ordinary player, giving the full player API.
    pub fn player(&self) -> Player {
        Player::by_name(self.name.clone())
    }

    fn sel(&self) -> PlayerSel {
        PlayerSel::Name(self.name.clone())
    }

    /// Whether this name currently points at a live simulated player.
    pub fn is_simulated(&self) -> bool {
        if !crate::has_slot!(sim_is) {
            return false;
        }
        match crate::__rt::api().sim_is {
            Some(f) => unsafe { f(self.sel().raw()) },
            None => false,
        }
    }

    /// Runs one verb. The argument is SNBT, and `"{}"` is passed when there is none.
    ///
    /// The verb table is on the host side and the named methods here are a facade over it. A
    /// verb the host does not recognize, an argument of the wrong shape, and a target that is
    /// not a simulated player are all an `Err`.
    pub fn act(&self, verb: &str, args_snbt: &str) -> Result<()> {
        let f = crate::require_slot!(sim_do, "driving a simulated player");
        let ok = unsafe { f(self.sel().raw(), s(verb), s(args_snbt)) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "the action `{verb}` on simulated player {} failed: the verb is unrecognized, the argument shape is wrong, or it is not a simulated player",
                self.name
            )))
        }
    }

    fn act0(&self, verb: &str) -> Result<()> {
        self.act(verb, "{}")
    }

    pub fn despawn(self) -> Result<()> {
        self.act0("despawn")
    }
    pub fn stop(&self) -> Result<()> {
        self.act0("stop")
    }
    pub fn jump(&self) -> Result<()> {
        self.act0("jump")
    }
    pub fn attack(&self) -> Result<()> {
        self.act0("attack")
    }
    pub fn interact(&self) -> Result<()> {
        self.act0("interact")
    }
    pub fn use_item(&self) -> Result<()> {
        self.act0("use_item")
    }
    pub fn drop_selected(&self) -> Result<()> {
        self.act0("drop")
    }
    pub fn respawn(&self) -> Result<()> {
        self.act0("respawn")
    }
    pub fn stop_destroying(&self) -> Result<()> {
        self.act0("stop_destroy")
    }

    /// Walks straight there. `face_target` decides whether it faces the target while walking.
    pub fn move_to(&self, x: f64, y: f64, z: f64, speed: f64, face_target: bool) -> Result<()> {
        self.act(
            "move_to",
            &NbtValue::obj([
                ("x", NbtValue::Double(x)),
                ("y", NbtValue::Double(y)),
                ("z", NbtValue::Double(z)),
                ("speed", NbtValue::Double(speed)),
                ("face_target", NbtValue::from(face_target)),
            ])
            .to_snbt(),
        )
    }

    /// Paths there. Unlike [`SimPlayer::move_to`] it goes around obstacles.
    pub fn navigate_to(&self, x: f64, y: f64, z: f64, speed: f64) -> Result<()> {
        self.act(
            "navigate_to",
            &NbtValue::obj([
                ("x", NbtValue::Double(x)),
                ("y", NbtValue::Double(y)),
                ("z", NbtValue::Double(z)),
                ("speed", NbtValue::Double(speed)),
            ])
            .to_snbt(),
        )
    }

    pub fn look_at(&self, x: f64, y: f64, z: f64) -> Result<()> {
        self.act("look_at", &NbtValue::vec3(x, y, z).to_snbt())
    }

    /// Mines one block. `face` is the face, defaulting to 1, meaning up.
    pub fn destroy_block(&self, x: i32, y: i32, z: i32, face: i32) -> Result<()> {
        self.act("destroy_block", &block_args(x, y, z, face))
    }

    /// Mines the one in the line of sight. `hand` is the reach, in blocks.
    pub fn destroy_look_at(&self, hand: f64) -> Result<()> {
        self.act(
            "destroy_look",
            &NbtValue::obj([("hand", NbtValue::Double(hand))]).to_snbt(),
        )
    }

    pub fn interact_block(&self, x: i32, y: i32, z: i32, face: i32) -> Result<()> {
        self.act("interact_block", &block_args(x, y, z, face))
    }

    pub fn set_sneaking(&self, on: bool) -> Result<()> {
        self.act("sneak", &on_args(on))
    }

    pub fn set_flying(&self, on: bool) -> Result<()> {
        self.act("fly", &on_args(on))
    }

    pub fn chat(&self, msg: &str) -> Result<()> {
        self.act(
            "chat",
            &NbtValue::obj([("msg", NbtValue::from(msg))]).to_snbt(),
        )
    }
}

fn block_args(x: i32, y: i32, z: i32, face: i32) -> String {
    NbtValue::obj([
        ("x", NbtValue::Int(x)),
        ("y", NbtValue::Int(y)),
        ("z", NbtValue::Int(z)),
        ("face", NbtValue::Int(face)),
    ])
    .to_snbt()
}

fn on_args(on: bool) -> String {
    NbtValue::obj([("on", NbtValue::from(on))]).to_snbt()
}

impl std::fmt::Display for SimPlayer {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "sim:{}", self.name)
    }
}
