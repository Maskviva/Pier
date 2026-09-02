//! Events: subscribing, reading a payload, editing it, cancelling.
//!
//! # A missing key and a value of 0 must stay apart
//!
//! Contract §5.1 records a land-protection bypass: an event in a custom dimension could
//! not read `dim`, the consumer wrote `unwrap_or(0)` and treated it as the overworld, and
//! it was allowed with nothing logged. [`Event`] therefore offers typed access, where a
//! missing key and a type mismatch are two different errors.
//!
//! It also recognizes `_unresolved`, a marker the host injects when it cannot resolve the
//! source of an event. Methods such as [`Event::dim`] return `Err` on it, so a protection
//! decision fails closed instead of continuing with an invented 0.
//!
//! [`Wiring`] does chained batch subscription and holds the handles together.

pub mod names;

use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{r, s};
use crate::rt::handle::Handle;
use crate::rt::logger::Logger;
use crate::rt::runtime::rt;
use crate::sys;
use std::collections::BTreeMap;
use std::ffi::c_void;

/// Dispatch priority. A lower value runs first, aligned with 0..4 in the ABI.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Default)]
pub enum Priority {
    Highest = 0,
    High = 1,
    #[default]
    Normal = 2,
    Low = 3,
    Lowest = 4,
}

impl Priority {
    fn raw(self) -> i32 {
        self as i32
    }
}

/// The player identity inside an event.
///
/// The three fields each have their own use and must not be mixed:
/// * `xuid` is unique and cannot be changed, and is the only key for permissions or
///   economy. It may be empty in offline mode.
/// * `uuid` is equally stable and suits a save key.
/// * `name` is for display. A player can change their display name, and host name
///   resolution falls back to the display name when the account name misses (see
///   `bridge::resolvePlayer`), so it must not be used as an identity.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct PlayerIdentity {
    pub name: String,
    pub xuid: String,
    pub uuid: String,
}

impl PlayerIdentity {
    /// Returns a selector usable for calling the API.
    ///
    /// The xuid comes first, since it cannot be forged. An empty xuid, in offline mode, falls
    /// back to the uuid and then to the name. On a fall back to `Name`,
    /// [`PlayerSel::is_stable`] is false, which a permission or economy decision should treat
    /// with care, since a name goes through the display-name fallback; see the `sel` module.
    pub fn selector(&self) -> crate::sel::PlayerSel {
        use crate::sel::PlayerSel;
        if !self.xuid.is_empty() {
            PlayerSel::Xuid(self.xuid.clone())
        } else if !self.uuid.is_empty() {
            PlayerSel::Uuid(self.uuid.clone())
        } else {
            PlayerSel::Name(self.name.clone())
        }
    }

    /// Whether there is a reliable identity, an xuid or a uuid. Ask this before using one as
    /// a permission key.
    pub fn is_identified(&self) -> bool {
        !self.xuid.is_empty() || !self.uuid.is_empty()
    }
}

/// One event dispatch. This is what a callback receives.
///
/// It lives only for the duration of the callback and must not be stored, which the
/// lifetime parameter also prevents.
pub struct Event<'a> {
    id: &'a str,
    snbt: &'a str,
    /// Parsed lazily: the many listeners that only watch the id should not pay for
    /// parsing SNBT.
    parsed: Option<NbtValue>,
    edited: Option<NbtValue>,
    write_ctx: *mut c_void,
    write_back: sys::PierStrSink,
}

impl<'a> Event<'a> {
    /// The event id, such as `"ll::event::player::PlayerChatEvent"` or the synthetic
    /// `"BlockDestroyEvent"`.
    pub fn id(&self) -> &str {
        self.id
    }

    /// The raw payload SNBT. The most direct thing while debugging.
    pub fn snbt(&self) -> &str {
        self.snbt
    }

    /// The parsed payload. Parsed on the first call and reused afterwards.
    pub fn value(&mut self) -> Result<&NbtValue> {
        if self.parsed.is_none() {
            self.parsed = Some(NbtValue::parse(self.snbt).map_err(Error::from)?);
        }
        Ok(self.parsed.as_ref().expect("just filled in"))
    }

    /// Returns `None` rather than an error when the payload cannot be parsed, for an
    /// observing listener that treats an unparsable payload as no event at all.
    pub fn value_opt(&mut self) -> Option<&NbtValue> {
        self.value().ok()
    }

    /* Typed access, absorbing what payload.rs used to do */

    /// Reads a string by path. A missing key and a type mismatch both name the key.
    pub fn str_at(&mut self, path: &str) -> Result<&str> {
        let v = self.value()?;
        v.get_str(path).map_err(Error::from)
    }

    pub fn i64_at(&mut self, path: &str) -> Result<i64> {
        let v = self.value()?;
        v.get_i64(path).map_err(Error::from)
    }

    pub fn i32_at(&mut self, path: &str) -> Result<i32> {
        let v = self.value()?;
        v.get_i32(path).map_err(Error::from)
    }

    pub fn f64_at(&mut self, path: &str) -> Result<f64> {
        let v = self.value()?;
        v.get_f64(path).map_err(Error::from)
    }

    pub fn bool_at(&mut self, path: &str) -> Result<bool> {
        let v = self.value()?;
        v.get_bool(path).map_err(Error::from)
    }

    /// The lenient form: `None` when it cannot be read, with no explanation. Only for cases
    /// where not reading it does not matter.
    pub fn opt_str(&mut self, path: &str) -> Option<String> {
        self.value_opt()?.opt_str(path).map(str::to_owned)
    }

    pub fn opt_i64(&mut self, path: &str) -> Option<i64> {
        self.value_opt()?.opt_i64(path)
    }

    /* Common fields, where the shape differences are absorbed */

    /// The list of fields the host could not resolve, `_unresolved`.
    ///
    /// When an event carries an Actor stub that is neither an online player nor present in
    /// the runtime actor table, the host records the field name here. A non-empty list means
    /// the payload is incomplete, and a protection decision should refuse rather than guess.
    pub fn unresolved(&mut self) -> Vec<String> {
        let Some(v) = self.value_opt() else {
            return Vec::new();
        };
        v.path("_unresolved")
            .and_then(NbtValue::as_list)
            .map(|l| {
                l.iter()
                    .filter_map(|x| x.as_str().map(str::to_owned))
                    .collect()
            })
            .unwrap_or_default()
    }

    /// Checks whether the payload is complete, meaning `_unresolved` is empty.
    pub fn check_complete(&mut self) -> bool {
        self.unresolved().is_empty()
    }

    /// Which dimension the event happened in.
    ///
    /// An unreadable value is an `Err` and never a 0. That rule is why this module exists: an
    /// earlier design had callers write `payload.i32_at("dim").unwrap_or(0)`, so every event
    /// in a custom dimension, whose id is 3 or above, was judged to be in the overworld, and
    /// land protection refusing in the overworld and allowing elsewhere was bypassed with
    /// nothing logged.
    pub fn dim(&mut self) -> Result<i32> {
        if !self.check_complete() {
            let miss = self.unresolved().join(", ");
            return Err(Error(format!(
                "the payload of event `{}` is incomplete, the host could not resolve {miss}, so the \
                             dimension is unknown and treating it as the overworld is refused",
                self.id
            )));
        }
        self.i32_at("dim")
    }

    /// The player identity inside an event.
    ///
    /// Handles three shapes: the `_player:{name,xuid,uuid}` of a synthetic event, the
    /// `_player` the host enriched, and an older event carrying only a name. Callers used to
    /// have to know the differences themselves.
    pub fn player(&mut self) -> Option<PlayerIdentity> {
        let v = self.value_opt()?;
        if let Some(p) = v.path("_player") {
            return Some(PlayerIdentity {
                name: p.opt_str("name").unwrap_or_default().to_owned(),
                xuid: p.opt_str("xuid").unwrap_or_default().to_owned(),
                uuid: p.opt_str("uuid").unwrap_or_default().to_owned(),
            });
        }
        // The fallback: an event carrying only a name.
        let name = v.first_str(&["playerName", "player", "name"])?;
        Some(PlayerIdentity {
            name: name.to_owned(),
            ..Default::default()
        })
    }

    /// The block or position coordinates in an event, as the three flat fields `x`, `y` and
    /// `z`, which is the shape of a synthetic event.
    pub fn pos(&mut self) -> Result<(i32, i32, i32)> {
        let x = self.i32_at("x")?;
        let y = self.i32_at("y")?;
        let z = self.i32_at("z")?;
        Ok((x, y, z))
    }

    /// As above but as floating point, for a player position and the like.
    pub fn pos_f64(&mut self) -> Result<(f64, f64, f64)> {
        let x = self.f64_at("x")?;
        let y = self.f64_at("y")?;
        let z = self.f64_at("z")?;
        Ok((x, y, z))
    }

    /* Editing and cancelling */

    /// Whether this event can be cancelled.
    ///
    /// * `Some(true)`: it can;
    /// * `Some(false)`: it cannot, and [`Event::cancel`] returns `Err`;
    /// * `None`: it is not in the tables, being an event a third-party mod emits itself or a
    ///   new upstream event the tables have not caught up with. `cancel()` then writes back as
    ///   usual and nobody can confirm for you that it took effect.
    pub fn can_cancel(&self) -> Option<bool> {
        names::is_cancellable(self.id)
    }

    /// Cancels this event.
    ///
    /// An event that cannot be cancelled returns `Err` and says which event to block instead,
    /// such as `PlayerStartDestroyBlockEvent` pointing at `PlayerDestroyBlockEvent`. Returning
    /// `()` would let a protection mod believe it had blocked something, and that belief is
    /// more dangerous than a crash, since a crash is at least visible.
    ///
    /// An event that is not in the tables, where `can_cancel()` is `None`, does not stand in
    /// the way: it writes back as usual and returns `Ok`. The SDK does not pretend to know
    /// what it does not know.
    ///
    /// An `Ok` means only that the cancel bit was written back to the host and not that the
    /// engine stopped: some hook points sit half updated and the host does not accept a cancel
    /// there at all. That boundary rests on the event documentation alone.
    pub fn cancel(&mut self) -> Result<()> {
        if self.can_cancel() == Some(false) {
            let why = names::why_not_cancellable(self.id)
                .unwrap_or("this event does not support cancelling");
            return Err(Error(format!(
                "event `{}` cannot be cancelled: {why}",
                self.id
            )));
        }
        self.edit(|v| {
            v.insert("cancelled", NbtValue::Byte(1));
        });
        Ok(())
    }

    /// Cancels without caring whether cancelling is possible.
    ///
    /// There is one legitimate use: a generic forwarding or proxy component where the event id
    /// arrives at runtime and failing to block simply has to be accepted. Business code uses
    /// [`Event::cancel`] and handles the `Err`.
    pub fn cancel_lenient(&mut self) -> bool {
        if self.can_cancel() == Some(false) {
            return false;
        }
        self.edit(|v| {
            v.insert("cancelled", NbtValue::Byte(1));
        });
        true
    }

    /// Undoes an earlier cancel by writing `cancelled` back to 0.
    ///
    /// For a two-stage decision that blocks first, decides, and then finds it may allow. Note
    /// that it undoes only a cancel written by this callback itself: a cancel another mod made
    /// at an earlier priority cannot be undone, and the host bus does not allow a veto to be
    /// turned back into an approval either.
    pub fn uncancel(&mut self) {
        self.edit(|v| {
            v.insert("cancelled", NbtValue::Byte(0));
        });
    }

    /// Edits one field of the payload.
    ///
    /// The write-back is a difference: only the keys really touched go back to the host and
    /// untouched ones stay as they are, so two mods on the same event do not erase each
    /// other's edits.
    pub fn set(&mut self, path: &str, value: NbtValue) {
        let key = path.to_owned();
        self.edit(move |v| {
            v.insert(key, value);
        });
    }

    /// An arbitrary rewrite. The closure receives a mutable copy of the payload.
    pub fn edit(&mut self, f: impl FnOnce(&mut NbtValue)) {
        if self.edited.is_none() {
            // Based on the current payload; an unparsable one starts from an empty compound tag,
            // which is at least enough to write cancelled.
            //
            // Cloned into a local before assigning: `self.value()` borrows `self`, and writing
            // `self.edited` in the same statement is a borrow conflict.
            let base = match self.value() {
                Ok(v) => v.clone(),
                Err(_) => NbtValue::compound(),
            };
            self.edited = Some(base);
        }
        if let Some(e) = self.edited.as_mut() {
            f(e);
        }
    }

    /// Called by the trampoline when the callback ends, sending the edited payload back to
    /// the host.
    fn flush(&mut self) {
        let Some(edited) = self.edited.take() else {
            return;
        };
        let text = edited.to_snbt();
        // The host side, in Events.cpp, diffs this against its own snapshot: only keys that
        // really changed are written back into the event object and an absent key is not
        // deleted. Treating absence as deletion loses the whole edit.
        unsafe { (self.write_back)(self.write_ctx, s(&text)) };
    }
}

/// A subscription handle. Dropping it unsubscribes.
///
/// [`Listener::forget`] keeps it alive until the mod unloads. The host removes whatever
/// remains at unload, really calling `removeListener` and reporting a failure rather than
/// staying silent.
pub struct Listener {
    handle: Handle,
    forgotten: bool,
    /// Ownership of the closure. It may be freed only after unsubscribing, since the host may
    /// otherwise be calling it.
    ///
    /// It is only held and dropped and never taken out, so `Any` suffices. `Sync` must not be
    /// required here: what goes in is a `Box<Handler>` and `Handler` guarantees only `Send`.
    owned: Option<Box<dyn std::any::Any + Send>>,
    id: String,
}

impl Listener {
    /// The id of the subscribed event.
    pub fn event_id(&self) -> &str {
        &self.id
    }

    /// Gives up automatic unsubscription. The closure leaks with it and lives until the
    /// process ends.
    pub fn forget(mut self) {
        if let Some(owned) = self.owned.take() {
            std::mem::forget(owned);
        }
        self.forgotten = true;
    }
}

impl Drop for Listener {
    fn drop(&mut self) {
        // `forget` no longer nulls the handle, since `Handle` has no setter, so the flag
        // is what tells a given-up subscription from a live one.
        if self.forgotten || self.handle.is_null() {
            return;
        }
        if !crate::has_slot!(unsubscribe_event) {
            return;
        }
        let Some(f) = rt().api.unsubscribe_event else {
            return;
        };
        // A failed unsubscribe has to speak, following the same discipline as the host side.
        // The consequence of a silent failure is a callback still attached while its closure is
        // about to be freed.
        let ok = unsafe { f(rt().handle(), self.handle.get()) };
        if !ok {
            Logger::get().error(&format!(
                "unsubscribing from event `{}` failed; the listener may still be attached to the host \
                 while its closure is about to be freed. This has to be investigated and is not noise.",
                self.id
            ));
            // Leaking the closure is preferable to letting the host call into freed memory.
            if let Some(owned) = self.owned.take() {
                std::mem::forget(owned);
            }
        }
    }
}

type Handler = dyn FnMut(&mut Event<'_>) + Send + 'static;

/// Subscribes to an event.
///
/// ```ignore
/// let l = event::subscribe(names::PLAYER_CHAT, |ev| {
///     let Ok(msg) = ev.str_at("message") else { return };
///     if !msg.contains("badword") { return; }
///     // cancel() returns a Result: an event that cannot be blocked says why and where.
///     if let Err(e) = ev.cancel() {
///         Logger::get().error(&format!("the chat filter did not take effect: {e}"));
///     }
/// })?;
/// ```
pub fn subscribe(
    id: &str,
    handler: impl FnMut(&mut Event<'_>) + Send + 'static,
) -> Result<Listener> {
    subscribe_with(id, Priority::Normal, handler)
}

/// Subscribes with a priority.
pub fn subscribe_with(
    id: &str,
    priority: Priority,
    handler: impl FnMut(&mut Event<'_>) + Send + 'static,
) -> Result<Listener> {
    let f = crate::require_slot!(subscribe_event, "subscribing to an event");
    let boxed: Box<Box<Handler>> = Box::new(Box::new(handler));
    let user = Box::into_raw(boxed);

    let handle = unsafe {
        f(
            rt().handle(),
            s(id),
            priority.raw(),
            trampoline,
            user.cast(),
        )
    };
    if handle.is_null() {
        // The host refused. The closure was not handed over, so ownership comes back.
        drop(unsafe { Box::from_raw(user) });
        return Err(Error(format!(
            "subscribing to `{id}` failed. The host log lists the nearby ids it knows, and the \
             cause is usually a misspelled event name or an event this BDS version does not have."
        )));
    }
    Ok(Listener {
        handle: Handle::new(handle),
        forgotten: false,
        owned: Some(unsafe { Box::from_raw(user) }),
        id: id.to_owned(),
    })
}

/// Every event id the host knows, from the registry plus every synthetic event.
///
/// Reading it beats guessing when an event name is misspelled.
pub fn list() -> Vec<String> {
    if !crate::has_slot!(list_events) {
        return Vec::new();
    }
    let Some(f) = rt().api.list_events else {
        return Vec::new();
    };
    crate::rt::ffi::collect_strs(|ctx, sink| unsafe { f(ctx, sink) })
}

/// Whether this host knows a given event id.
pub fn exists(id: &str) -> bool {
    list().iter().any(|e| e == id)
}

/// # Safety
/// `user` must come from `Box<Box<Handler>>::into_raw` and stay valid while the listener
/// lives.
unsafe extern "C" fn trampoline(
    user: *mut c_void,
    event_id: sys::PierStr,
    snbt: sys::PierStr,
    write_ctx: *mut c_void,
    write_back: sys::PierStrSink,
) {
    if user.is_null() {
        return;
    }
    let f = &mut *(user as *mut Box<Handler>);
    let id = r(event_id);
    let text = r(snbt);

    let mut ev = Event {
        id,
        snbt: text,
        parsed: None,
        edited: None,
        write_ctx,
        write_back,
    };

    // A panic must not cross an extern "C" boundary, which is undefined behavior. It is
    // caught here and logged, and the edit is not written back, since what a half-panicked
    // decision would write back means nothing.
    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        f(&mut ev);
        ev.flush();
    }));
    if result.is_err() {
        Logger::get().error(&format!(
            "a listener for event `{id}` panicked. It was caught here and this edit was discarded."
        ));
    }
}

/* ═══════════════════════════ Wiring ═══════════════════════════ */

/// Batch subscription, holding the handles together.
///
/// Business code was already writing this itself, as `Wiring::new("worldedit").on(...).at(...)`, so
/// it moved into the SDK. It adds two things over a hand-written version: a failed subscription is
/// not silent, since failures are recorded in [`Wiring::failures`] and `arm()` can fail as a whole,
/// and the tag goes into the log so the failing entry can be located.
///
/// ```ignore let wiring = Wiring::new("plots")
///     .on(names::PLAYER_DESTROY_BLOCK, "protect-break", |ev| { ... })
///     .at(names::PLAYER_DISCONNECT, Priority::Low, "forget", |ev| { ... })
///     .arm()?;                       // any failure fails the whole thing
///
/// // wiring going out of scope unsubscribes everything ```
pub struct Wiring {
    owner: String,
    pending: Vec<(String, Priority, String, Box<Handler>)>,
    listeners: Vec<Listener>,
    failures: Vec<(String, String)>,
}

impl Wiring {
    pub fn new(owner: impl Into<String>) -> Wiring {
        Wiring {
            owner: owner.into(),
            pending: Vec::new(),
            listeners: Vec::new(),
            failures: Vec::new(),
        }
    }

    /// Adds a subscription at Normal priority. `tag` is used only for logging.
    #[must_use]
    pub fn on(
        self,
        id: &str,
        tag: &str,
        handler: impl FnMut(&mut Event<'_>) + Send + 'static,
    ) -> Wiring {
        self.at(id, Priority::Normal, tag, handler)
    }

    /// Adds a subscription at a given priority.
    #[must_use]
    pub fn at(
        mut self,
        id: &str,
        priority: Priority,
        tag: &str,
        handler: impl FnMut(&mut Event<'_>) + Send + 'static,
    ) -> Wiring {
        self.pending
            .push((id.to_owned(), priority, tag.to_owned(), Box::new(handler)));
        self
    }

    /// Performs the subscriptions. Any failure fails the whole thing and the ones that
    /// succeeded are unsubscribed before returning, because half-attached protection is more
    /// dangerous than none: some points block and some do not, and nobody knows which.
    pub fn arm(mut self) -> Result<Wiring> {
        let pending = std::mem::take(&mut self.pending);
        for (id, prio, tag, handler) in pending {
            // `Box<Handler>` implements FnMut itself, so it is passed straight through with no
            // extra
            // closure around it.
            match subscribe_with(&id, prio, handler) {
                Ok(l) => self.listeners.push(l),
                Err(e) => {
                    self.failures
                        .push((format!("{}/{}", self.owner, tag), e.to_string()));
                }
            }
        }
        if !self.failures.is_empty() {
            let detail = self
                .failures
                .iter()
                .map(|(t, e)| format!("  {t}: {e}"))
                .collect::<Vec<_>>()
                .join("\n");
            // listeners unsubscribe automatically when self drops, so no manual cleanup.
            return Err(Error(format!(
                "the event wiring of `{}` had {} entries fail to attach, so the whole set was withdrawn:\n{detail}",
                self.owner,
                self.failures.len()
            )));
        }
        Ok(self)
    }

    /// The lenient form: failures are recorded and successes attach as usual. Suited to an
    /// optional feature that is nice to have.
    pub fn arm_lenient(mut self) -> Wiring {
        let pending = std::mem::take(&mut self.pending);
        for (id, prio, tag, handler) in pending {
            match subscribe_with(&id, prio, handler) {
                Ok(l) => self.listeners.push(l),
                Err(e) => {
                    Logger::get().warn(&format!(
                        "`{}/{}` failed to subscribe to `{id}`, continuing with the rest: {e}",
                        self.owner, tag
                    ));
                    self.failures
                        .push((format!("{}/{}", self.owner, tag), e.to_string()));
                }
            }
        }
        self
    }

    /// The ones that did not attach, as (tag, reason).
    pub fn failures(&self) -> &[(String, String)] {
        &self.failures
    }

    /// How many attached.
    pub fn armed(&self) -> usize {
        self.listeners.len()
    }

    /// Switches everything to no automatic unsubscription, living until the mod unloads.
    pub fn forget(mut self) {
        for l in self.listeners.drain(..) {
            l.forget();
        }
    }
}

/* Compatibility aliases */

/// An earlier generation called this `EventRef`. The name is kept and points at the same
/// type.
pub type EventRef<'a> = Event<'a>;

/// An earlier generation called the priority `EventPriority`.
pub type EventPriority = Priority;

/// The raw error type of a failed payload read, exposed so business code can branch
/// finely on it.
pub use crate::nbt::NbtError as PayloadError;


/// Builds a `PlayerIdentity` straight from a compound tag, for a custom payload.
impl From<&BTreeMap<String, NbtValue>> for PlayerIdentity {
    fn from(m: &BTreeMap<String, NbtValue>) -> Self {
        let get = |k: &str| {
            m.get(k)
                .and_then(NbtValue::as_str)
                .unwrap_or_default()
                .to_owned()
        };
        PlayerIdentity {
            name: get("name"),
            xuid: get("xuid"),
            uuid: get("uuid"),
        }
    }
}

impl std::fmt::Debug for Event<'_> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("Event")
            .field("id", &self.id)
            .field("snbt", &self.snbt)
            .finish()
    }
}
