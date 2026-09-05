//! Raw packet interception.
//! This layer handles closure ownership, the panic fence, and gathering `{ptr,len}` into a `&[u8]`.
//! It interprets not one byte of the body: version differences, field layout and codecs all live on
//! the caller's side. A loader usable across versions cannot understand the wire format of every
//! version at once.
//!
//! # Threads: read this before writing any state
//!
//! An inbound callback runs on the thread pumping that connection and an outbound one on the thread
//! that started the send. Usually that is the server thread, but an async flush means it is not
//! guaranteed, which is why the closure bound is `Send + Sync` and not `Send`: the same closure may
//! be entered by several threads at once.
//!
//! With several subscribers, each sees the output of the previous one in registration order, and
//! the first Drop wins with everything after it skipped. The subscription table is snapshotted
//! before dispatch, so registering and deregistering inside a callback is safe.

use core::ffi::c_void;

use crate::rt::error::{Error, Result};
use crate::rt::ffi::r;
use crate::rt::handle::Handle;
use crate::rt::logger::Logger;
use crate::rt::runtime::rt;
use crate::sys;

/// The direction of a packet.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Direction {
    /// Client to server.
    Inbound,
    /// Server to client.
    Outbound,
}

impl Direction {
    fn from_abi(v: i32) -> Direction {
        // The ABI has only these two values. A third one really appearing means the host is
        // newer than this SDK, and for a direction the cost of guessing wrong is treating
        // outbound as inbound, so outbound is the safer guess: it is more common and rewriting
        // an outbound packet is less risky than rewriting an inbound one.
        if v == sys::PIER_PKT_INBOUND {
            Direction::Inbound
        } else {
            Direction::Outbound
        }
    }
}

/// Which directions to select at registration.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Directions {
    Inbound,
    Outbound,
    Both,
}

impl Directions {
    fn mask(self) -> i32 {
        match self {
            Directions::Inbound => sys::PIER_PKT_MASK_INBOUND,
            Directions::Outbound => sys::PIER_PKT_MASK_OUTBOUND,
            Directions::Both => sys::PIER_PKT_MASK_INBOUND | sys::PIER_PKT_MASK_OUTBOUND,
        }
    }
}

/// What the callback does with this packet.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Verdict {
    /// Forward unchanged. A prior `set_body` or `set_packet_id` takes effect.
    Forward,
    /// Consume the packet entirely.
    Drop,
}

/// The two states of a connection.
///
/// A close is the only reliable signal to clear the state of a connection: one that never
/// completed the login handshake never becomes a Player and no player event covers it.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ConnectionState {
    Opened,
    Closed,
}

/// The type of an interceptor closure.
///
/// Naming it separately is required by the clippy `type_complexity` lint and does read
/// better: this type appears once each in registration, the trampoline and `PacketHook`,
/// and written out in full all three would have to match exactly, so one change would be
/// three.
type PacketFn = dyn Fn(&mut Packet<'_>) -> Verdict + Send + Sync;

/// The type of a connection open and close observer closure. Same reason as above.
type ConnFn = dyn Fn(u64, &str, ConnectionState) + Send + Sync;

/// The packet a callback receives. Valid only during the callback.
pub struct Packet<'a> {
    ev: &'a sys::PierPacketEvent,
    edit: &'a mut sys::PierPacketEdit,
    replace_ctx: *mut c_void,
    replace: sys::PierBytesSink,
    replaced: bool,
}

impl<'a> Packet<'a> {
    pub fn direction(&self) -> Direction {
        Direction::from_abi(self.ev.direction)
    }

    /// The id of this connection. Stable across packets and usable as the key of per-connection
    /// state.
    pub fn conn_id(&self) -> u64 {
        self.ev.conn_id
    }

    /// The peer address as `ip:port`. For diagnostics; use `conn_id` as a key.
    pub fn address(&self) -> &str {
        unsafe { r(self.ev.address) }
    }

    pub fn packet_id(&self) -> i32 {
        self.ev.packet_id
    }

    pub fn sender_sub_id(&self) -> u8 {
        self.ev.sender_sub_id
    }

    pub fn target_sub_id(&self) -> u8 {
        self.ev.target_sub_id
    }

    /// The body, without the header, since the packet id and sub id are already decoded.
    ///
    /// That is deliberate: a rewriter only supplies a new body and the host re-encodes the
    /// header from `edit`, so changing a packet id is a field assignment rather than varint
    /// surgery.
    pub fn body(&self) -> &[u8] {
        if self.ev.body.is_null() || self.ev.body_len == 0 {
            return &[];
        }
        unsafe { core::slice::from_raw_parts(self.ev.body, self.ev.body_len) }
    }

    /// Replaces the body. It may be called several times in one callback and the last one
    /// counts.
    pub fn set_body(&mut self, bytes: &[u8]) {
        let sink = self.replace;
        unsafe { sink(self.replace_ctx, bytes.as_ptr(), bytes.len()) };
        self.replaced = true;
    }

    /// Rewrites the packet id, for remapping onto the numbering of another version.
    pub fn set_packet_id(&mut self, id: i32) {
        self.edit.packet_id = id;
        self.replaced = true;
    }

    pub fn set_sender_sub_id(&mut self, id: u8) {
        self.edit.sender_sub_id = id;
        self.replaced = true;
    }

    pub fn set_target_sub_id(&mut self, id: u8) {
        self.edit.target_sub_id = id;
        self.replaced = true;
    }
}

/// A registered packet interceptor.
///
/// Dropping it deregisters. `forget()` keeps it alive until the mod unloads, and it is
/// explicit because registering and discarding looks identical in code to registering and
/// forgetting to keep the returned value, and the latter is a bug. The host clears what
/// remains when the mod unloads, at teardown stage 90.
pub struct PacketHook {
    handle: Handle,
    unregister: Option<unsafe extern "C" fn(sys::PierModHandle, sys::PierPacketHookHandle) -> bool>,
    /// Ownership of the closure, freed only after deregistering. The other order leaves the
    /// host calling back through an already freed pointer, and that moment is usually the
    /// arrival of the next packet, far from here.
    ///
    /// Rust guarantees the order: `Drop::drop` runs first and deregisters, then the fields drop
    /// in declaration order.
    owned: Option<Box<dyn core::any::Any + Send + Sync>>,
}

impl PacketHook {
    /// Gives up deregistering on drop and keeps it alive until the mod unloads.
    ///
    /// This step is explicit because registering and discarding looks identical in code to
    /// registering and forgetting to keep the returned value, and the latter is a bug where the
    /// interceptor disappears the moment it is installed.
    pub fn forget(mut self) {
        self.unregister = None;
        // The closure has to stay alive, since the host will keep calling it. Leaked on
        // purpose; the host clears the remaining registrations when the mod unloads, at
        // teardown stage 90.
        if let Some(owned) = self.owned.take() {
            core::mem::forget(owned);
        }
    }
}

impl Drop for PacketHook {
    fn drop(&mut self) {
        if let Some(f) = self.unregister {
            unsafe { f(rt().handle(), self.handle.get()) };
        }
    }
}

/// The packet facade.
#[derive(Clone, Copy)]
pub struct Packets(());

impl Packets {
    pub fn get() -> Packets {
        Packets(())
    }

    /// Registers a packet interceptor.
    ///
    /// The closure needs `Send + Sync`; see the thread section of the module header. A callback
    /// is not guaranteed to be on the server thread and may be entered by several threads at
    /// once.
    pub fn intercept<F>(&self, dirs: Directions, f: F) -> Result<PacketHook>
    where
        F: Fn(&mut Packet<'_>) -> Verdict + Send + Sync + 'static,
    {
        let reg = crate::require_slot!(packet_hook_register, "intercepting packets");
        // Neither gate may be skipped (contract §2.2): first whether the table is long
        // enough, then whether the slot is non-null. The deregistration slot sits at a
        // larger offset than the registration slot, so a successful registration does not
        // imply this slot can be read.
        let unreg = if crate::has_slot!(packet_hook_unregister) {
            rt().api.packet_hook_unregister
        } else {
            None
        };

        let boxed: Box<Box<PacketFn>> = Box::new(Box::new(f));
        let user = Box::into_raw(boxed);

        let handle = unsafe { reg(rt().handle(), dirs.mask(), packet_trampoline, user.cast()) };
        Self::finish_register(handle, user, unreg)
    }

    /// As [`Packets::intercept`], but the interceptor runs only for the listed packet ids.
    ///
    /// This is the form to use whenever the ids are known, which is nearly always. The
    /// unfiltered form costs one callback per packet in each direction it asked for, chunk
    /// data included, while here the host passes a packet no subscriber listed through before
    /// taking any lock. `ids` are `MinecraftPacketIds` values in `0..1024`; an id outside that
    /// range is ignored by the host, and a list with nothing left is refused.
    ///
    /// On a host without this slot the call falls back to the unfiltered form and filters the
    /// id in the trampoline, so the interceptor still sees only the ids it asked for, at the
    /// cost of one callback per packet.
    pub fn intercept_ids<F>(&self, dirs: Directions, ids: &[i32], f: F) -> Result<PacketHook>
    where
        F: Fn(&mut Packet<'_>) -> Verdict + Send + Sync + 'static,
    {
        if ids.is_empty() {
            return Err(Error(
                "intercept_ids needs at least one packet id; an interceptor that can never fire is a bug at the call site".to_owned(),
            ));
        }
        let unreg = if crate::has_slot!(packet_hook_unregister) {
            rt().api.packet_hook_unregister
        } else {
            None
        };
        if crate::has_slot!(packet_hook_register_ids) {
            if let Some(reg) = rt().api.packet_hook_register_ids {
                let boxed: Box<Box<PacketFn>> = Box::new(Box::new(f));
                let user = Box::into_raw(boxed);
                let handle = unsafe {
                    reg(
                        rt().handle(),
                        dirs.mask(),
                        ids.as_ptr(),
                        ids.len(),
                        packet_trampoline,
                        user.cast(),
                    )
                };
                return Self::finish_register(handle, user, unreg);
            }
        }
        // Older host: filter here. The set is small, so a linear scan beats a hash.
        let wanted: Vec<i32> = ids.to_vec();
        self.intercept(dirs, move |p| {
            if wanted.contains(&p.packet_id()) {
                f(p)
            } else {
                Verdict::Forward
            }
        })
    }

    fn finish_register(
        handle: sys::PierPacketHookHandle,
        user: *mut Box<PacketFn>,
        unreg: Option<unsafe extern "C" fn(sys::PierModHandle, sys::PierPacketHookHandle) -> bool>,
    ) -> Result<PacketHook> {
        if handle.is_null() {
            // Registration failed: the closure is taken back and freed rather than leaked.
            drop(unsafe { Box::from_raw(user) });
            return Err(Error(
                "the host refused to register the packet interceptor, because the direction mask is empty or the host failed internally".to_owned(),
            ));
        }
        Ok(PacketHook {
            handle: Handle::new(handle),
            unregister: unreg,
            owned: Some(unsafe { Box::from_raw(user) }),
        })
    }

    /// Registers an observer of connection opens and closes.
    pub fn on_connection<F>(&self, f: F) -> Result<PacketHook>
    where
        F: Fn(u64, &str, ConnectionState) + Send + Sync + 'static,
    {
        let reg = crate::require_slot!(
            packet_conn_hook_register,
            "observing connection opens and closes"
        );
        // Neither gate may be skipped (contract §2.2): first whether the table is long
        let unreg = if crate::has_slot!(packet_conn_hook_unregister) {
            rt().api.packet_conn_hook_unregister
        } else {
            None
        };

        let boxed: Box<Box<ConnFn>> = Box::new(Box::new(f));
        let user = Box::into_raw(boxed);

        let handle = unsafe { reg(rt().handle(), conn_trampoline, user.cast()) };
        if handle.is_null() {
            drop(unsafe { Box::from_raw(user) });
            return Err(Error(
                "the host refused to register the connection observer".to_owned(),
            ));
        }
        Ok(PacketHook {
            handle: Handle::new(handle),
            unregister: unreg,
            owned: Some(unsafe { Box::from_raw(user) }),
        })
    }
}

// The two trampolines
//
// A panic crossing extern "C" is undefined behavior. Both are wrapped in catch_unwind, at
// the cost of this one packet being forwarded unchanged rather than the whole process
// aborting without a diagnostic.
// Drop must not be chosen here: a panicking interceptor that consumed every packet would
// give the symptom of players being unable to join while the server looks fine, which is
// far harder to diagnose than one unmodified packet.

/// # Safety
/// Called by the host, where `user` is the closure `intercept` boxed.
unsafe extern "C" fn packet_trampoline(
    user: *mut c_void,
    ev: *const sys::PierPacketEvent,
    edit: *mut sys::PierPacketEdit,
    replace_ctx: *mut c_void,
    replace: sys::PierBytesSink,
) -> i32 {
    if user.is_null() || ev.is_null() || edit.is_null() {
        return sys::PIER_PKT_PASS;
    }
    let f = &*(user as *const Box<PacketFn>);
    let mut packet = Packet {
        ev: &*ev,
        edit: &mut *edit,
        replace_ctx,
        replace,
        replaced: false,
    };
    let verdict = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| f(&mut packet)));
    match verdict {
        Ok(Verdict::Drop) => sys::PIER_PKT_DROP,
        Ok(Verdict::Forward) => {
            if packet.replaced {
                sys::PIER_PKT_REPLACE
            } else {
                sys::PIER_PKT_PASS
            }
        }
        Err(_) => {
            Logger::get().error(
                "a packet interceptor panicked. It was caught here and this packet is forwarded \
                 unchanged; choosing Drop would give the symptom of players being unable to join while the server looks fine, which is harder to diagnose.",
            );
            sys::PIER_PKT_PASS
        }
    }
}

/// # Safety
/// Called by the host, where `user` is the closure `on_connection` boxed.
unsafe extern "C" fn conn_trampoline(
    user: *mut c_void,
    conn_id: u64,
    address: sys::PierStr,
    opened: bool,
) {
    if user.is_null() {
        return;
    }
    let f = &*(user as *const Box<ConnFn>);
    let addr = r(address);
    let state = if opened {
        ConnectionState::Opened
    } else {
        ConnectionState::Closed
    };
    if std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| f(conn_id, addr, state))).is_err() {
        Logger::get().error("a connection observer panicked. It was caught here.");
    }
}
