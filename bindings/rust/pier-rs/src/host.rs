//! The host and the system themselves: the capabilities that involve no game concept.
//!
//! What belongs here is decided by whether it speaks about the host or the world. Run
//! state, handing a task back to the server thread, executing a command and asking the
//! operating system its name all hold for another game as well; players, blocks and items
//! do not.

use core::ffi::c_void;

use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, collect_strs, r_owned, s};
use crate::rt::logger::Logger;
use crate::rt::runtime::{api, rt, TaskId};
use crate::sys;

/// The run stage of the server, mirroring `ll::GamingStatus`.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum GamingStatus {
    Default,
    Starting,
    Running,
    Stopping,
    /// The host reported a value this side does not recognize. This is not an error, since
    /// the host may be newer than the mod (contract §2.2). `Unknown(i32)` is used rather than
    /// a panic or a silent collapse into Default, because unrecognized and Default are two
    /// different things (contract §5.2).
    Unknown(i32),
}

impl From<i32> for GamingStatus {
    fn from(v: i32) -> Self {
        match v {
            0 => GamingStatus::Default,
            1 => GamingStatus::Starting,
            2 => GamingStatus::Running,
            3 => GamingStatus::Stopping,
            other => GamingStatus::Unknown(other),
        }
    }
}

/// The host facade. Zero sized and freely `Copy`.
#[derive(Clone, Copy)]
pub struct Host(());

impl Host {
    pub fn get() -> Host {
        Host(())
    }

    /// Which stage the server is in. The ABI marks it thread safe, so any thread may ask.
    pub fn gaming_status(&self) -> GamingStatus {
        match api().gaming_status {
            Some(f) => GamingStatus::from(unsafe { f() }),
            // A missing core slot can only mean the host left it out while filling the table. No
            // panic: a status query must not take the server down. Returning Unknown(-1) lets the
            // caller decide what to do.
            None => GamingStatus::Unknown(-1),
        }
    }

    /// Hands a piece of work back to the server thread to run as soon as possible. Thread
    /// safe.
    ///
    /// The closure is boxed and handed to the host, taken back and run inside the callback,
    /// and freed immediately afterwards. Ownership stays on the mod side throughout, which
    /// follows contract §3: no ownership crosses the boundary, what is passed is an opaque
    /// pointer and the host only hands it back unchanged.
    ///
    /// It goes through `schedule_for`, which carries a mod handle, and not the ownerless
    /// `schedule`. A task on the ownerless slot still fires after the mod unloads and jumps
    /// into an already unmapped code segment. A task with a handle is accounted per mod by the
    /// host and the whole batch is discarded at unload, with a warning suggesting you cancel
    /// them yourself. The returned [`TaskId`] works with [`Host::cancel`].
    pub fn schedule(&self, f: impl FnOnce() + Send + 'static) -> Result<TaskId> {
        let cb = crate::require_slot!(schedule_for, "scheduling a task");
        let boxed: Box<Box<dyn FnOnce() + Send>> = Box::new(Box::new(f));
        let user = Box::into_raw(boxed);
        let id = unsafe { cb(rt().handle(), task_trampoline, user.cast()) };
        if id == 0 {
            // The host refused. The closure was not handed over, so ownership comes back.
            drop(unsafe { Box::from_raw(user) });
            return Err(Error(
                "the host refused the scheduled task, because the mod is not yet adopted or the handle is invalid".to_owned(),
            ));
        }
        Ok(TaskId(id))
    }

    /// As above, but runs after `delay`. Thread safe.
    ///
    /// It takes a `Duration` rather than a bare millisecond count: whether the 5 in
    /// `schedule_after(5, ...)` means five milliseconds or five seconds is answered only by the
    /// parameter name, which is invisible at the call site. Putting the unit in the type makes
    /// the call site carry the answer.
    ///
    /// The ABI side is in milliseconds, so this converts once. A duration exceeding `u64`
    /// milliseconds is clamped to the maximum rather than wrapping to a small number, since
    /// wrapping would turn run in a year into run immediately.
    pub fn schedule_after(
        &self,
        delay: std::time::Duration,
        f: impl FnOnce() + Send + 'static,
    ) -> Result<TaskId> {
        let cb = crate::require_slot!(schedule_after_for, "scheduling a delayed task");
        let delay_ms = u64::try_from(delay.as_millis()).unwrap_or(u64::MAX);
        let boxed: Box<Box<dyn FnOnce() + Send>> = Box::new(Box::new(f));
        let user = Box::into_raw(boxed);
        let id = unsafe { cb(rt().handle(), task_trampoline, user.cast(), delay_ms) };
        if id == 0 {
            drop(unsafe { Box::from_raw(user) });
            return Err(Error(
                "the host refused the delayed task, because the mod is not yet adopted or the handle is invalid".to_owned(),
            ));
        }
        Ok(TaskId(id))
    }

    /// Cancels a task that has not run yet. A ticket that already ran, was already cancelled,
    /// or does not belong to this mod returns `false`.
    ///
    /// Note that cancelling only voids the ticket. The closure itself is neither called nor
    /// freed when the host tears down and is reclaimed when the process ends. Avoiding a leak
    /// means not scheduling and cancelling in bulk on a hot path.
    pub fn cancel(&self, task: TaskId) -> bool {
        if !task.is_valid() {
            return false;
        }
        match api().schedule_cancel {
            Some(f) => unsafe { f(rt().handle(), task.0) },
            None => false,
        }
    }

    /// How many tasks under this mod have not run. Suited to asserting 0 in `on_unload`.
    pub fn pending_tasks(&self) -> u32 {
        match api().schedule_pending_count {
            Some(f) => unsafe { f(rt().handle()) },
            None => 0,
        }
    }

    /// Executes a command as the console and returns its output.
    ///
    /// The two `Err` cases stay apart: a missing slot, meaning the host is too old, and the
    /// command itself failing, where `success == false` and the output holds the error.
    pub fn execute_command(&self, cmd: &str) -> Result<String> {
        let f = crate::require_slot!(execute_command, "executing a command");
        let mut out = CmdOut {
            success: false,
            text: String::new(),
        };
        let ok = unsafe { f(s(cmd), (&mut out as *mut CmdOut).cast(), cmd_sink) };
        if !ok {
            return Err(Error(format!("command `{cmd}` could not be executed; the host refused")));
        }
        if !out.success {
            return Err(Error(format!("command `{cmd}` failed: {}", out.text)));
        }
        Ok(out.text)
    }

    /// Lists every event id the host can currently resolve.
    ///
    /// Printing this list when a subscription fails is far more useful than a bare subscribe
    /// failed (contract §5.3: a log line has to answer what to do about it).
    pub fn list_events(&self) -> Vec<String> {
        let Some(f) = api().list_events else {
            return Vec::new();
        };
        collect_strs(|ctx, sink| unsafe { f(ctx, sink) })
    }

    /// Information at the operating-system level. `prop` comes from `sys::PIER_SYS_*`.
    pub fn sys_info(&self, prop: i32) -> Result<String> {
        let f = crate::require_slot!(sys_info_str, "reading system information");
        call_out_str(|ctx, sink| unsafe { f(prop, ctx, sink) })
            .ok_or_else(|| Error(format!("the host could not read system information item {prop}")))
    }

    /// Information at the server level. `prop` comes from `sys::PIER_SRV_*`.
    pub fn server_info(&self, prop: i32) -> Result<String> {
        let f = crate::require_slot!(server_info_str, "reading server information");
        call_out_str(|ctx, sink| unsafe { f(prop, ctx, sink) })
            .ok_or_else(|| Error(format!("the host could not read server information item {prop}")))
    }

    /// The network protocol version of the server.
    ///
    /// A named accessor rather than having the caller pass `PIER_SRV_PROTOCOL_VERSION`: this
    /// number is the first criterion a version-adapting mod uses, and the cost of the wrong
    /// constant is receiving the BDS version string and failing to parse it, a failure far
    /// from its cause.
    ///
    /// On the ABI it is a string, converted from `SharedConstants::NetworkProtocolVersion`.
    /// This parses it into a number and reports a parse failure truthfully rather than
    /// returning 0: cannot-be-determined and a protocol version of 0 must stay apart
    /// (contract §5.2).
    pub fn protocol_version(&self) -> Result<u32> {
        let raw = self.server_info(sys::PIER_SRV_PROTOCOL_VERSION)?;
        raw.trim()
            .parse::<u32>()
            .map_err(|e| Error(format!("the protocol version {raw:?} the host reported does not parse as a number: {e}")))
    }

    /// The BDS version string, from `Common::getGameVersionString`.
    pub fn bds_version(&self) -> Result<String> {
        self.server_info(sys::PIER_SRV_BDS_VERSION)
    }

    pub fn packets(&self) -> crate::packet::Packets {
        crate::packet::Packets::get()
    }

    pub fn current_tick(&self) -> Option<u64> {
        api().get_current_tick.map(|f| unsafe { f() })
    }

    pub fn player_count(&self) -> Option<i32> {
        api().get_player_count.map(|f| unsafe { f() })
    }

    /// Reads an environment variable.
    ///
    /// Failing to read and reading an empty string are the same empty string here: the failure
    /// bit of this slot on the ABI means only that the host could not perform the read, and an
    /// existing empty variable in a process environment is the same as an absent one.
    pub fn env(&self, name: &str) -> Result<String> {
        let f = crate::require_slot!(sys_get_env, "reading an environment variable");
        Ok(call_out_str(|ctx, sink| unsafe { f(s(name), ctx, sink) }).unwrap_or_default())
    }

    /// Sets an environment variable. It affects this process only.
    pub fn set_env(&self, name: &str, value: &str) -> Result<()> {
        let f = crate::require_slot!(sys_set_env, "setting an environment variable");
        if unsafe { f(s(name), s(value)) } {
            Ok(())
        } else {
            Err(Error(format!("the environment variable {name} could not be set")))
        }
    }

    /// Whether this host runs under Wine.
    ///
    /// Worth asking on its own: some Windows APIs behave differently under Wine than on real
    /// Windows, and the symptom usually appears far from the cause.
    pub fn is_wine(&self) -> bool {
        if !crate::has_slot!(sys_is_wine) {
            return false;
        }
        match api().sys_is_wine {
            Some(f) => unsafe { f() },
            None => false,
        }
    }

    pub fn os_name(&self) -> Result<String> {
        self.sys_info(sys::PIER_SYS_OS_NAME)
    }

    /// The operating system version string.
    pub fn os_version(&self) -> Result<String> {
        self.sys_info(sys::PIER_SYS_OS_VERSION)
    }

    pub fn locale(&self) -> Result<String> {
        self.sys_info(sys::PIER_SYS_LOCALE)
    }

    pub fn local_time(&self) -> Result<crate::types::LocalTime> {
        let text = self.sys_info(sys::PIER_SYS_LOCAL_TIME)?;
        let v = crate::nbt::NbtValue::parse(&text)
            .map_err(|e| Error(format!("parsing the local time SNBT failed: {e}")))?;
        Ok(crate::types::LocalTime {
            year: v.opt_i32("year").unwrap_or(0),
            month: v.opt_i32("month").unwrap_or(0),
            day: v.opt_i32("day").unwrap_or(0),
            hour: v.opt_i32("hour").unwrap_or(0),
            minute: v.opt_i32("minute").unwrap_or(0),
            second: v.opt_i32("second").unwrap_or(0),
            ms: v.opt_i32("ms").unwrap_or(0),
        })
    }

    /// The world facade.
    ///
    /// This accessor forms no cycle with `Host`: `world` really does need
    /// `Host::execute_command` to assemble a `/fill`, but the entry point on each side is only
    /// the `get()` of a zero-sized facade and no state is shared. The layering check declares
    /// this edge explicitly.
    pub fn world(&self) -> crate::World {
        crate::World::get()
    }

    /// Server runtime control: freezing ticks, warping them, and performance sampling.
    pub fn server(&self) -> crate::Server {
        crate::Server::get()
    }
}

// The two trampolines crossing extern "C".

/// # Safety
/// `user` must come from `Box<Box<dyn FnOnce() + Send>>::into_raw` and be called once.
unsafe extern "C" fn task_trampoline(user: *mut c_void) {
    if user.is_null() {
        return;
    }
    let f: Box<Box<dyn FnOnce() + Send>> = Box::from_raw(user.cast());
    // A panic crossing extern "C" is undefined behavior. Catching it here costs this one
    // task rather than aborting the whole process without a diagnostic.
    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(move || (*f)()));
    if result.is_err() {
        Logger::get().error("a scheduled task panicked. It was caught here and the task was discarded.");
    }
}

struct CmdOut {
    success: bool,
    text: String,
}

/// # Safety
/// `ctx` must be a valid `*mut CmdOut`.
unsafe extern "C" fn cmd_sink(ctx: *mut c_void, success: bool, output: sys::PierStr) {
    let out = &mut *ctx.cast::<CmdOut>();
    out.success = success;
    out.text = r_owned(output);
}
