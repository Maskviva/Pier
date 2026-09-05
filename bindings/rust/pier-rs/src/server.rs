//! Server runtime control: freezing, stepping and warping ticks, plus performance sampling broken
//! down by subsystem.
//! These do not live in [`crate::host`]: that layer speaks about the host itself, meaning
//! scheduling, executing commands and the operating system, and holds for another game, while a
//! tick is the rhythm of the world simulation and is a game concept.
//!
//! # A hook is installed and never removed
//!
//! The detour installs lazily on the first call and stays. The reason is that a control call may
//! come from a command handler executing inside the tick, where removing a hook is unsafe. The idle
//! cost is one predictable branch per frame.
//!
//! # Players still move while frozen
//!
//! Freezing stops mobs, blocks, redstone and time. Movement is client authoritative and the network
//! runs outside the level tick, so players keep walking and chatting.

use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::call_out_str;

/// The server runtime facade. Zero sized.
#[derive(Clone, Copy)]
pub struct Server(());

impl Server {
    pub fn get() -> Server {
        Server(())
    }

    /// Freezes or unfreezes the world.
    pub fn set_tick_freeze(&self, on: bool) -> Result<()> {
        let f = crate::require_slot!(tick_freeze, "freezing the world tick");
        if unsafe { f(on) } {
            Ok(())
        } else {
            Err(Error(
                "the tick freeze could not be set: the level is not ready".to_owned(),
            ))
        }
    }

    /// Valid only while frozen: lets `n` more frames through.
    pub fn step_ticks(&self, n: u32) -> Result<()> {
        let f = crate::require_slot!(tick_step, "stepping the tick");
        if unsafe { f(n) } {
            Ok(())
        } else {
            Err(Error(format!(
                "stepping {n} frames failed: nothing is frozen right now, or n is 0"
            )))
        }
    }

    /// The time warp. `0 < factor <= 100`, below 1 is slow motion and 1.0 is normal.
    pub fn set_tick_warp(&self, factor: f64) -> Result<()> {
        let f = crate::require_slot!(tick_warp, "setting the tick warp");
        if unsafe { f(factor) } {
            Ok(())
        } else {
            Err(Error(format!(
                "the tick warp {factor} could not be set: the value has to lie between 0 and 100"
            )))
        }
    }

    /// Opens a sampling window of `ticks` frames, from 1 to 12000. Only one window at a time.
    pub fn begin_profile(&self, ticks: u32) -> Result<()> {
        let f = crate::require_slot!(profile_begin, "beginning performance sampling");
        if unsafe { f(ticks) } {
            Ok(())
        } else {
            Err(Error(format!(
                "a sampling window of {ticks} frames could not be opened: the count is 0 or above 12000, or sampling is already running"
            )))
        }
    }

    /// Takes the sampling report.
    ///
    /// Sampling still running gives `Ok(None)`, and one window succeeds exactly once.
    ///
    /// The per-item times are inclusive of nesting, so they are read side by side and not
    /// summed.
    pub fn take_profile(&self) -> Result<Option<NbtValue>> {
        let f = crate::require_slot!(profile_take, "taking the performance sampling report");
        let Some(text) = call_out_str(|ctx, sink| unsafe { f(ctx, sink) }) else {
            return Ok(None);
        };
        let v = NbtValue::parse(&text)
            .map_err(|e| Error(format!("parsing the sampling report SNBT failed: {e}")))?;
        Ok(Some(v))
    }

    /// Whether the simulation is paused.
    pub fn is_sim_paused(&self) -> Result<bool> {
        let f = crate::require_slot!(get_sim_paused, "querying the simulation pause state");
        Ok(unsafe { f() })
    }

    /// The wall-clock period of the last frame in seconds. At 20 TPS it is 0.05.
    ///
    /// This is the frame period, sleep included, and not the time the server spent computing
    /// the tick: on an idle server it reads 0.05 all the same. It is kept for callers that want
    /// the raw engine value; [`Server::tps`] and [`Server::mspt`] are the numbers a monitor
    /// wants.
    ///
    /// The host returns -1.0 when it cannot read it, translated here into an `Err`: a negative
    /// frame duration would have a caller compute a negative TPS without noticing.
    pub fn tick_delta_time(&self) -> Result<f64> {
        let f = crate::require_slot!(get_tick_delta_time, "reading the frame duration");
        let v = unsafe { f() };
        if v < 0.0 {
            Err(Error(
                "the level is not ready, so the frame duration cannot be read".to_owned(),
            ))
        } else {
            Ok(v)
        }
    }

    /// Ticks per second over the last 5 seconds of wall clock.
    ///
    /// Measured by counting the `Level::tick` calls that really run and dividing by elapsed
    /// time, so it stays right under the tick warp (reads above 20), the tick freeze (reads
    /// 0) and lag (reads below 20). The reciprocal of one frame period is not this number:
    /// it reads above 20 on about half the frames of an idle server and reads 20 while the
    /// world is frozen.
    ///
    /// `Err` until the host has sampled its first frame, which takes two ticks after start.
    pub fn tps(&self) -> Result<f64> {
        self.tps_over(5)
    }

    /// As [`Server::tps`], over a window of `window_seconds` (1..=60, clipped by the host).
    pub fn tps_over(&self, window_seconds: u32) -> Result<f64> {
        let f = crate::require_slot!(get_tps, "reading the tick rate");
        let v = unsafe { f(window_seconds.clamp(1, 60) as i32) };
        if v < 0.0 {
            return Err(Error(
                "no frame has been sampled yet, so no TPS can be computed; retry a tick later".to_owned(),
            ));
        }
        Ok(v)
    }

    /// Milliseconds spent computing each tick, averaged over the last 5 seconds.
    ///
    /// This excludes the idle sleep between frames: a healthy server reads a few
    /// milliseconds and only approaches 50 when it is saturated. `tick_delta_time() * 1000`
    /// is not this number, it is the frame period and reads about 50 on an idle server.
    pub fn mspt(&self) -> Result<f64> {
        self.mspt_over(5)
    }

    /// As [`Server::mspt`], over a window of `window_seconds` (1..=60, clipped by the host).
    pub fn mspt_over(&self, window_seconds: u32) -> Result<f64> {
        let f = crate::require_slot!(get_mspt, "reading the time per tick");
        let v = unsafe { f(window_seconds.clamp(1, 60) as i32) };
        if v < 0.0 {
            return Err(Error(
                "no tick ran in the window, so no MSPT can be computed; the world is frozen or the host just started".to_owned(),
            ));
        }
        Ok(v)
    }
}
