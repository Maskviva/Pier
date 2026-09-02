//! Players: addressed by selector and resolved again on every call.
//!
//! # Only an xuid may be used as a key
//!
//! When `PlayerSel::Name` matches no account name on the host side it falls back to the display
//! name, which another mod can change. A player setting their display name to the account name of
//! an offline player redirects every by-name call onto themselves. Permission, economy and
//! ownership decisions all use [`Player::by_xuid`]; see the module documentation of [`crate::sel`].
//!
//! # A player is an actor too
//!
//! [`Player::as_entity`] goes through `player_resolve` for an `ActorUniqueID`, after which the
//! whole of [`crate::entity::Entity`] applies. The two APIs are complementary: player-specific
//! things such as inventory, ability bits, titles and kicking are here, and actor-general ones such
//! as health, teleporting and tags are there.

mod admin;
mod io;
mod items;
mod props;

use crate::entity::Entity;
use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, collect_strs, s};
use crate::sel::PlayerSel;
use crate::sys;
use crate::types::{GameMode, PlayerPermission, PositionF64};

/// One entry `list_players` reports.
///
/// `dimension` and `pos` are `Option` and not bare values. The host omits those keys when
/// the level is not ready, or while that player is midway through a dimension change, and
/// filling them with 0 would say they are at the origin of the overworld. The land
/// protection bypass contract §5.1 records has exactly that shape: an event in a custom
/// dimension could not read `dim`, the consumer wrote `unwrap_or(0)`, and everything was
/// allowed as the overworld.
///
/// A caller that wants a fallback writes `.unwrap_or(0)` itself, and is then the one
/// answerable for that default.
#[derive(Debug, Clone, Default, PartialEq)]
pub struct PlayerInfo {
    pub name: String,
    pub xuid: String,
    pub uuid: String,
    pub dimension: Option<i32>,
    pub pos: Option<PositionF64>,
}

impl PlayerInfo {
    /// Builds a stable selector from an xuid. An empty xuid, on an offline-mode server, falls
    /// back to the name and says so, so a caller knows the key is unreliable.
    pub fn selector(&self) -> PlayerSel {
        if self.xuid.is_empty() {
            PlayerSel::Name(self.name.clone())
        } else {
            PlayerSel::Xuid(self.xuid.clone())
        }
    }
}

/// The network status of a player, from `PIER_PSTR_NETWORK_STATUS`.
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct NetworkStatus {
    pub ping: i32,
    pub avg_ping: i32,
    pub max_ping: i32,
    /// Per mille, exactly as the host gives it.
    pub packet_loss: i32,
}

/// One player. It holds a selector and no pointer.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Player {
    sel: PlayerSel,
}

impl Player {
    /// By name. This goes through the display-name fallback, so identity uses
    /// [`Player::by_xuid`].
    pub fn by_name(name: impl Into<String>) -> Player {
        Player {
            sel: PlayerSel::Name(name.into()),
        }
    }

    /// By xuid: unique, unforgeable and unchangeable by the player.
    pub fn by_xuid(xuid: impl Into<String>) -> Player {
        Player {
            sel: PlayerSel::Xuid(xuid.into()),
        }
    }

    pub fn by_uuid(uuid: impl Into<String>) -> Player {
        Player {
            sel: PlayerSel::Uuid(uuid.into()),
        }
    }

    pub fn from_sel(sel: PlayerSel) -> Player {
        Player { sel }
    }

    pub fn sel(&self) -> &PlayerSel {
        &self.sel
    }

    /// The list of online players.
    ///
    /// The host sinks one SNBT per player. An unparsable entry is skipped with a warning
    /// rather than emptying the whole table: one bad entry should not make who is on the
    /// server unanswerable.
    pub fn list() -> Vec<PlayerInfo> {
        if !crate::has_slot!(list_players) {
            return Vec::new();
        }
        let Some(f) = crate::__rt::api().list_players else {
            return Vec::new();
        };
        let raw = collect_strs(|ctx, sink| unsafe { f(ctx, sink) });
        let mut out = Vec::with_capacity(raw.len());
        for text in raw {
            match NbtValue::parse(&text) {
                Ok(v) => out.push(PlayerInfo {
                    name: v.opt_str("name").unwrap_or_default().to_owned(),
                    xuid: v.opt_str("xuid").unwrap_or_default().to_owned(),
                    uuid: v.opt_str("uuid").unwrap_or_default().to_owned(),
                    dimension: v.opt_i32("dim"),
                    // All three axes have to be present together and a missing one voids
                    // the whole: an (x, 0, z) looks like a valid coordinate while it really
                    // means y could not be read.
                    pos: match (v.opt_f64("x"), v.opt_f64("y"), v.opt_f64("z")) {
                        (Some(x), Some(y), Some(z)) => Some((x, y, z)),
                        _ => None,
                    },
                }),
                Err(e) => crate::Logger::get()
                    .warn(&format!("one SNBT entry of list_players could not be parsed and was skipped: {e}")),
            }
        }
        out
    }

    /// Sends one message to every online player.
    pub fn broadcast(msg: &str) -> Result<()> {
        let f = crate::require_slot!(broadcast_message, "broadcasting to the server");
        unsafe { f(s(msg)) };
        Ok(())
    }

    /// Whether this selector currently resolves to anyone.
    pub fn is_online(&self) -> bool {
        self.resolve().is_ok()
    }

    /// Resolves into an `ActorUniqueID`.
    fn resolve(&self) -> Result<sys::PierActorId> {
        let f = crate::require_slot!(player_resolve, "resolving a player");
        let mut out: sys::PierActorId = 0;
        let ok = unsafe { f(self.sel.raw(), &mut out) };
        if ok {
            Ok(out)
        } else {
            Err(Error(format!(
                "player {} is offline, or the selector resolves to nobody",
                self.sel
            )))
        }
    }

    /// Uses it as an actor, giving the full set of [`Entity`] capabilities.
    pub fn as_entity(&self) -> Result<Entity> {
        self.resolve().map(Entity::from_id)
    }

    // Properties

    /// Reads a `PIER_PPROP_*` numeric property.
    pub fn num(&self, prop: i32) -> Result<f64> {
        let f = crate::require_slot!(player_get_num, "reading a numeric player property");
        let mut out = 0.0f64;
        let ok = unsafe { f(self.sel.raw(), prop, &mut out) };
        if ok {
            Ok(out)
        } else {
            Err(Error(format!(
                "property {prop} of player {} could not be read: they are offline, or the host does not recognize the property number",
                self.sel
            )))
        }
    }

    /// Writes a `PIER_PPROP_*` numeric property. Only the ones marked (S) are writable.
    pub fn set_num(&self, prop: i32, v: f64) -> Result<()> {
        let f = crate::require_slot!(player_set_num, "writing a numeric player property");
        let ok = unsafe { f(self.sel.raw(), prop, v) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "property {prop} of player {} could not be written: they are offline, or the property is read-only",
                self.sel
            )))
        }
    }

    /// Reads a `PIER_PSTR_*` string property.
    pub fn text(&self, prop: i32) -> Result<String> {
        let f = crate::require_slot!(player_get_str, "reading a string player property");
        call_out_str(|ctx, sink| unsafe { f(self.sel.raw(), prop, ctx, sink) })
            .ok_or_else(|| Error(format!("string property {prop} of player {} could not be read", self.sel)))
    }

    fn num_i32(&self, prop: i32) -> Result<i32> {
        self.num(prop).map(|v| v as i32)
    }

    /// The position of the last death. Never having died gives `Ok(None)`, since the host
    /// sends an empty string.
    pub fn last_death_pos(&self) -> Result<Option<(PositionF64, i32)>> {
        let text = self.text(sys::PIER_PSTR_LAST_DEATH_POS)?;
        if text.trim().is_empty() {
            return Ok(None);
        }
        let v = NbtValue::parse(&text).map_err(|e| Error(format!("parsing the death coordinate failed: {e}")))?;
        let dim = self
            .text(sys::PIER_PSTR_LAST_DEATH_DIMENSION)?
            .trim()
            .parse::<i32>()
            .unwrap_or(0);
        Ok(Some((
            (v.get_f64("x")?, v.get_f64("y")?, v.get_f64("z")?),
            dim,
        )))
    }

    pub fn game_type(&self) -> Result<GameMode> {
        let v = self.num_i32(sys::PIER_PPROP_GAME_TYPE)?;
        GameMode::from_i32(v).ok_or_else(|| Error(format!("the host reported an unrecognized game mode {v}")))
    }
    pub fn permission_level(&self) -> Result<PlayerPermission> {
        let v = self.num_i32(sys::PIER_PPROP_PERMISSION_LEVEL)?;
        PlayerPermission::from_i32(v).ok_or_else(|| Error(format!("the host reported an unrecognized permission level {v}")))
    }
    pub fn set_level(&self, level: i32) -> Result<()> {
        self.set_num(sys::PIER_PPROP_LEVEL, level as f64)
    }
    /// The experience bar progress, from 0 to 1.
    pub fn set_experience(&self, progress: f64) -> Result<()> {
        self.set_num(sys::PIER_PPROP_EXPERIENCE, progress)
    }
    pub fn set_hunger(&self, v: f64) -> Result<()> {
        self.set_num(sys::PIER_PPROP_HUNGER, v)
    }
    pub fn set_saturation(&self, v: f64) -> Result<()> {
        self.set_num(sys::PIER_PPROP_SATURATION, v)
    }
    pub fn set_exhaustion(&self, v: f64) -> Result<()> {
        self.set_num(sys::PIER_PPROP_EXHAUSTION, v)
    }

    /// The position. It goes through a dedicated slot rather than a property number, because
    /// one call gives all three axes plus the dimension, while a player may have moved between
    /// three separate property calls.
    pub fn position(&self) -> Result<(PositionF64, i32)> {
        let f = crate::require_slot!(get_player_position, "reading a player position");
        let name = self
            .real_name()
            .unwrap_or_else(|_| self.sel.value().to_owned());
        let p = unsafe { f(s(&name)) };
        if p.found {
            Ok(((p.x, p.y, p.z), p.dimension))
        } else {
            Err(Error(format!("player {} is offline, so no position can be read", self.sel)))
        }
    }

    /// The network status in detail.
    pub fn network_status(&self) -> Result<NetworkStatus> {
        let f = crate::require_slot!(player_get_network_status, "reading the network status of a player");
        let text = call_out_str(|ctx, sink| unsafe { f(self.sel.raw(), ctx, sink) })
            .ok_or_else(|| Error(format!("the network status of player {} could not be read", self.sel)))?;
        let v =
            NbtValue::parse(&text).map_err(|e| Error(format!("parsing the network status SNBT failed: {e}")))?;
        Ok(NetworkStatus {
            ping: v.opt_i32("ping").unwrap_or(-1),
            avg_ping: v.opt_i32("avg_ping").unwrap_or(-1),
            max_ping: v.opt_i32("max_ping").unwrap_or(-1),
            packet_loss: v.opt_i32("packet_loss").unwrap_or(-1),
        })
    }

    /// The connection id of this player, the same number a packet interceptor sees.
    ///
    /// A 0 means offline or no network identity available. On the ABI 0 is not a valid
    /// connection id, so this reports it truthfully as an `Err` rather than handing over a 0.
    pub fn conn_id(&self) -> Result<u64> {
        let f = crate::require_slot!(player_conn_id, "reading the connection id of a player");
        let id = unsafe { f(self.sel.raw()) };
        if id == 0 {
            Err(Error(format!("player {} is offline, or the network identity is unavailable", self.sel)))
        } else {
            Ok(id)
        }
    }
}

impl std::fmt::Display for Player {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.sel)
    }
}

impl From<PlayerSel> for Player {
    fn from(sel: PlayerSel) -> Player {
        Player { sel }
    }
}
