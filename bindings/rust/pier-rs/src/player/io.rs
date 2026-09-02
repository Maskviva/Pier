//! The outbound channels of a player: chat, titles, particles and raw packets.
//!
//! All of these send packets, so they share the same failure mode, that nothing goes out
//! when the person is offline, and the same discipline: text always goes through a
//! structured packet and never an assembled command line.

use crate::player::Player;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::s;
use crate::types::{MessageType, TitleKind, TitleTimes};

impl Player {
    // Messages and titles

    pub fn send_message(&self, msg: &str) -> Result<()> {
        let f = crate::require_slot!(player_send_message, "sending a message to a player");
        let ok = unsafe { f(self.sel.raw(), s(msg)) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!("player {} is offline and the message did not go out", self.sel)))
        }
    }

    /// Sends one with a given `TextPacketType`.
    pub fn tell(&self, msg: &str, kind: MessageType) -> Result<()> {
        let f = crate::require_slot!(player_send_message_typed, "sending a typed message to a player");
        let ok = unsafe { f(self.sel.raw(), s(msg), kind.as_i32()) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!("player {} is offline and the message did not go out", self.sel)))
        }
    }

    /// Sends one title.
    ///
    /// It goes through a real `SetTitlePacket` and not an assembled `/title` command, which
    /// would paste the text into a command line verbatim and turn a plot whose name contains a
    /// quote or an `@e` into a command injection.
    ///
    /// A given `times` sends a Times packet first so the timing is definite, and omitting it
    /// reuses the durations the client stored last. The three durations cannot be given in
    /// part, a combination the host refuses outright.
    pub fn send_title(&self, kind: TitleKind, text: &str, times: Option<TitleTimes>) -> Result<()> {
        let f = crate::require_slot!(player_send_title, "sending a title to a player");
        let t = times.unwrap_or(TitleTimes::new(-1, -1, -1));
        let ok = unsafe {
            f(
                self.sel.raw(),
                kind.as_i32(),
                s(if kind.uses_text() { text } else { "" }),
                t.fade_in,
                t.stay,
                t.fade_out,
            )
        };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "sending a title to player {} failed: they are offline, or the host refuses the {kind:?} kind",
                self.sel
            )))
        }
    }

    pub fn set_title(&self, text: &str) -> Result<()> {
        self.send_title(TitleKind::Title, text, None)
    }
    pub fn set_subtitle(&self, text: &str) -> Result<()> {
        self.send_title(TitleKind::Subtitle, text, None)
    }
    pub fn set_actionbar(&self, text: &str) -> Result<()> {
        self.send_title(TitleKind::Actionbar, text, None)
    }
    pub fn clear_title(&self) -> Result<()> {
        self.send_title(TitleKind::Clear, "", None)
    }

    // Networking

    /// Spawns a particle for this one player only.
    ///
    /// Unlike `World::spawn_particle`, nobody else sees it, since that one broadcasts across
    /// the whole dimension. Something like a selection highlight has to use this one, otherwise
    /// the whole server sees it.
    pub fn spawn_particle(&self, dim: i32, effect: &str, x: f64, y: f64, z: f64) -> Result<()> {
        let f = crate::require_slot!(spawn_particle_for, "spawning a particle for one player");
        let ok = unsafe { f(self.sel.raw(), dim, s(effect), x, y, z) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!("player {} is offline and the particle did not go out", self.sel)))
        }
    }

    /// Pushes a raw packet onto the connection of this player.
    ///
    /// An escape hatch: `body` is the wire format of the current game version and has to
    /// follow every version change, which is the caller's responsibility. A named entry point
    /// is preferred wherever one exists.
    pub fn send_packet(&self, packet_id: i32, body: &[u8]) -> Result<()> {
        let f = crate::require_slot!(send_packet, "sending a raw packet");
        let ok = unsafe { f(self.sel.raw(), packet_id, body.as_ptr(), body.len()) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "sending the packet with id={packet_id} to player {} failed: they are offline, the id builds no packet, or the body has the wrong shape on this version",
                self.sel
            )))
        }
    }
}
