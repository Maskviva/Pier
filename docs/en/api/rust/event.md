# levilamina::event

Events: subscribing, reading a payload, editing it, cancelling.

**A missing key and a value of 0 must stay apart**

Contract §5.1 records a land-protection bypass: an event in a custom dimension could
not read `dim`, the consumer wrote `unwrap_or(0)` and treated it as the overworld, and
it was allowed with nothing logged. [`Event`](event.md#Event) therefore offers typed access, where a
missing key and a type mismatch are two different errors.

It also recognizes `_unresolved`, a marker the host injects when it cannot resolve the
source of an event. Methods such as [`Event::dim`](event.md#Event.dim) return `Err` on it, so a protection
decision fails closed instead of continuing with an invented 0.

[`Wiring`](event.md#Wiring) does chained batch subscription and holds the handles together.

## Functions {#functions}

### `event::subscribe` {#fn.subscribe}

```rust
pub fn subscribe(
    id: &str,
    handler: impl FnMut(&mut Event<'_>) + Send + 'static,
) -> Result<Listener>
```

Subscribes to an event.

```rust
let l = event::subscribe(names::PLAYER_CHAT, |ev| {
    let Ok(msg) = ev.str_at("message") else { return };
    if !msg.contains("badword") { return; }
    // cancel() returns a Result: an event that cannot be blocked says why and where.
    if let Err(e) = ev.cancel() {
        Logger::get().error(&format!("the chat filter did not take effect: {e}"));
    }
})?;
```

- Parameters:
    - id : `&str`
    - handler : `impl FnMut(&mut Event<'_>) + Send + 'static`
- Return type: `Result<Listener>`
- Slots: [`subscribe_event`](../cpp/events.md#subscribe_event)

### `event::subscribe_with` {#fn.subscribe_with}

```rust
pub fn subscribe_with(
    id: &str,
    priority: Priority,
    handler: impl FnMut(&mut Event<'_>) + Send + 'static,
) -> Result<Listener>
```

Subscribes with a priority.

- Parameters:
    - id : `&str`
    - priority : `Priority`
    - handler : `impl FnMut(&mut Event<'_>) + Send + 'static`
- Return type: `Result<Listener>`
- Slots: [`subscribe_event`](../cpp/events.md#subscribe_event)

### `event::list` {#fn.list}

```rust
pub fn list() -> Vec<String>
```

Every event id the host knows, from the registry plus every synthetic event.

Reading it beats guessing when an event name is misspelled.

- Return type: `Vec<String>`
- Slots: [`list_events`](../cpp/events.md#list_events)

### `event::exists` {#fn.exists}

```rust
pub fn exists(id: &str) -> bool
```

Whether this host knows a given event id.

- Parameters:
    - id : `&str`
- Return type: `bool`
- Slots: [`list_events`](../cpp/events.md#list_events)

## `Event` {#Event}

```rust
pub struct Event<'a> {
    // private fields
}
```

One event dispatch. This is what a callback receives.

It lives only for the duration of the callback and must not be stored, which the
lifetime parameter also prevents.

- Implements: `Debug`

### `Event::id` {#Event.id}

```rust
pub fn id(&self) -> &str
```

The event id, such as `"ll::event::player::PlayerChatEvent"` or the synthetic
`"BlockDestroyEvent"`.

- Return type: `&str`

### `Event::snbt` {#Event.snbt}

```rust
pub fn snbt(&self) -> &str
```

The raw payload SNBT. The most direct thing while debugging.

- Return type: `&str`

### `Event::value` {#Event.value}

```rust
pub fn value(&mut self) -> Result<&NbtValue>
```

The parsed payload. Parsed on the first call and reused afterwards.

- Return type: `Result<&NbtValue>`

### `Event::value_opt` {#Event.value_opt}

```rust
pub fn value_opt(&mut self) -> Option<&NbtValue>
```

Returns `None` rather than an error when the payload cannot be parsed, for an
observing listener that treats an unparsable payload as no event at all.

- Return type: `Option<&NbtValue>`

### `Event::str_at` {#Event.str_at}

```rust
pub fn str_at(&mut self, path: &str) -> Result<&str>
```

Reads a string by path. A missing key and a type mismatch both name the key.

- Parameters:
    - path : `&str`
- Return type: `Result<&str>`

### `Event::i64_at` {#Event.i64_at}

```rust
pub fn i64_at(&mut self, path: &str) -> Result<i64>
```

- Parameters:
    - path : `&str`
- Return type: `Result<i64>`

### `Event::i32_at` {#Event.i32_at}

```rust
pub fn i32_at(&mut self, path: &str) -> Result<i32>
```

- Parameters:
    - path : `&str`
- Return type: `Result<i32>`

### `Event::f64_at` {#Event.f64_at}

```rust
pub fn f64_at(&mut self, path: &str) -> Result<f64>
```

- Parameters:
    - path : `&str`
- Return type: `Result<f64>`

### `Event::bool_at` {#Event.bool_at}

```rust
pub fn bool_at(&mut self, path: &str) -> Result<bool>
```

- Parameters:
    - path : `&str`
- Return type: `Result<bool>`

### `Event::opt_str` {#Event.opt_str}

```rust
pub fn opt_str(&mut self, path: &str) -> Option<String>
```

The lenient form: `None` when it cannot be read, with no explanation. Only for cases
where not reading it does not matter.

- Parameters:
    - path : `&str`
- Return type: `Option<String>`

### `Event::opt_i64` {#Event.opt_i64}

```rust
pub fn opt_i64(&mut self, path: &str) -> Option<i64>
```

- Parameters:
    - path : `&str`
- Return type: `Option<i64>`

### `Event::unresolved` {#Event.unresolved}

```rust
pub fn unresolved(&mut self) -> Vec<String>
```

The list of fields the host could not resolve, `_unresolved`.

When an event carries an Actor stub that is neither an online player nor present in
the runtime actor table, the host records the field name here. A non-empty list means
the payload is incomplete, and a protection decision should refuse rather than guess.
The list is empty too when the payload could not be parsed at all, which
`Self::check_complete` does not count as complete.

- Return type: `Vec<String>`

### `Event::check_complete` {#Event.check_complete}

```rust
pub fn check_complete(&mut self) -> bool
```

Checks whether the payload is complete: it parsed, and `_unresolved` is empty. A
payload that did not parse is not complete, since nothing in it could be resolved.

- Return type: `bool`

### `Event::dim` {#Event.dim}

```rust
pub fn dim(&mut self) -> Result<i32>
```

Which dimension the event happened in.

An unreadable value is an `Err`. An
earlier design had callers write `payload.i32_at("dim").unwrap_or(0)`, so every event
in a custom dimension, whose id is 3 or above, was judged to be in the overworld, and
land protection refusing in the overworld and allowing elsewhere was bypassed with
nothing logged.

- Return type: `Result<i32>`

### `Event::player` {#Event.player}

```rust
pub fn player(&mut self) -> Option<PlayerIdentity>
```

The player identity inside an event.

Handles three shapes: the `_player:{name,xuid,uuid}` of a synthetic event, the
`_player` the host enriched, and an older event carrying only a name. Callers used to
have to know the differences themselves.

- Return type: `Option<PlayerIdentity>`

### `Event::pos` {#Event.pos}

```rust
pub fn pos(&mut self) -> Result<(i32, i32, i32)>
```

The block or position coordinates in an event, as the three flat fields `x`, `y` and
`z`, which is the shape of a synthetic event.

- Return type: `Result<(i32, i32, i32)>`

### `Event::pos_f64` {#Event.pos_f64}

```rust
pub fn pos_f64(&mut self) -> Result<(f64, f64, f64)>
```

As above but as floating point, for a player position and the like.

- Return type: `Result<(f64, f64, f64)>`

### `Event::can_cancel` {#Event.can_cancel}

```rust
pub fn can_cancel(&self) -> Option<bool>
```

Whether this event can be cancelled.

* `Some(true)`: it can;
* `Some(false)`: it cannot, and [`Event::cancel`](event.md#Event.cancel) returns `Err`;
* `None`: it is not in the tables, being an event a third-party mod emits itself or a
  new upstream event the tables have not caught up with. `cancel()` then writes back as
  usual and nobody can confirm for you that it took effect.

- Return type: `Option<bool>`

### `Event::cancel` {#Event.cancel}

```rust
pub fn cancel(&mut self) -> Result<()>
```

Cancels this event.

An event that cannot be cancelled returns `Err` and says which event to block instead,
such as `PlayerStartDestroyBlockEvent` pointing at `PlayerDestroyBlockEvent`. Returning
`()` would let a protection mod believe it had blocked something, and that belief is
more dangerous than a crash, since a crash is at least visible.

An event that is not in the tables, where `can_cancel()` is `None`, does not stand in
the way: it writes back as usual and returns `Ok`. The SDK does not pretend to know
what it does not know.

An `Ok` means only that the cancel bit was written back to the host and not that the
engine stopped: some hook points sit half updated and the host does not accept a cancel
there at all. That boundary rests on the event documentation alone.

- Return type: `Result<()>`

### `Event::cancel_lenient` {#Event.cancel_lenient}

```rust
pub fn cancel_lenient(&mut self) -> bool
```

Cancels without caring whether cancelling is possible.

There is one legitimate use: a generic forwarding or proxy component where the event id
arrives at runtime and failing to block simply has to be accepted. Business code uses
[`Event::cancel`](event.md#Event.cancel) and handles the `Err`.

- Return type: `bool`

### `Event::uncancel` {#Event.uncancel}

```rust
pub fn uncancel(&mut self)
```

Undoes an earlier cancel by writing `cancelled` back to 0.

For a two-stage decision that blocks first, decides, and then finds it may allow. Note
that it undoes only a cancel written by this callback itself: a cancel another mod made
at an earlier priority cannot be undone, and the host bus does not allow a veto to be
turned back into an approval either.

### `Event::set` {#Event.set}

```rust
pub fn set(&mut self, path: &str, value: NbtValue)
```

Edits one field of the payload.

The write-back is a difference: only the keys really touched go back to the host and
untouched ones stay as they are, so two mods on the same event do not erase each
other's edits.

- Parameters:
    - path : `&str`
    - value : `NbtValue`

### `Event::edit` {#Event.edit}

```rust
pub fn edit(&mut self, f: impl FnOnce(&mut NbtValue))
```

An arbitrary rewrite. The closure receives a mutable copy of the payload.

This is the one path that copies the whole payload; [`Event::set`](event.md#Event.set) and
[`Event::cancel`](event.md#Event.cancel) write only the keys they touch.

- Parameters:
    - f : `impl FnOnce(&mut NbtValue)`

## `Wiring` {#Wiring}

```rust
pub struct Wiring {
    // private fields
}
```

Batch subscription, holding the handles together.

Two things over subscribing by hand: a failed subscription is not silent, since
failures are recorded in [`Wiring::failures`](event.md#Wiring.failures) and `arm()` can fail as a whole, and the
tag goes into the log so the failing entry can be located.

```rust
let wiring = Wiring::new("plots")
    .on(names::PLAYER_DESTROY_BLOCK, "protect-break", |ev| { ... })
    .at(names::PLAYER_DISCONNECT, Priority::Low, "forget", |ev| { ... })
    .arm()?;                       // any failure fails the whole thing

// wiring going out of scope unsubscribes everything
```

### `Wiring::new` {#Wiring.new}

```rust
pub fn new(owner: impl Into<String>) -> Wiring
```

- Parameters:
    - owner : `impl Into<String>`
- Return type: `Wiring`

### `Wiring::on` {#Wiring.on}

```rust
pub fn on(
        self,
        id: &str,
        tag: &str,
        handler: impl FnMut(&mut Event<'_>) + Send + 'static,
    ) -> Wiring
```

Adds a subscription at Normal priority. `tag` is used only for logging.

- Parameters:
    - id : `&str`
    - tag : `&str`
    - handler : `impl FnMut(&mut Event<'_>) + Send + 'static`
- Return type: `Wiring`

### `Wiring::at` {#Wiring.at}

```rust
pub fn at(
        mut self,
        id: &str,
        priority: Priority,
        tag: &str,
        handler: impl FnMut(&mut Event<'_>) + Send + 'static,
    ) -> Wiring
```

Adds a subscription at a given priority.

- Parameters:
    - id : `&str`
    - priority : `Priority`
    - tag : `&str`
    - handler : `impl FnMut(&mut Event<'_>) + Send + 'static`
- Return type: `Wiring`

### `Wiring::arm` {#Wiring.arm}

```rust
pub fn arm(mut self) -> Result<Wiring>
```

Performs the subscriptions. Any failure fails the whole thing and the ones that
succeeded are unsubscribed before returning, because half-attached protection is more
dangerous than none: some points block and some do not, and nobody knows which.

- Return type: `Result<Wiring>`
- Slots: [`subscribe_event`](../cpp/events.md#subscribe_event)

### `Wiring::arm_lenient` {#Wiring.arm_lenient}

```rust
pub fn arm_lenient(mut self) -> Wiring
```

The lenient form: failures are recorded and successes attach as usual. Suited to an
optional feature that is nice to have.

- Return type: `Wiring`
- Slots: [`subscribe_event`](../cpp/events.md#subscribe_event)

### `Wiring::failures` {#Wiring.failures}

```rust
pub fn failures(&self) -> &[(String, String)]
```

The ones that did not attach, as (tag, reason).

- Return type: `&[(String, String)]`

### `Wiring::armed` {#Wiring.armed}

```rust
pub fn armed(&self) -> usize
```

How many attached.

- Return type: `usize`

### `Wiring::forget` {#Wiring.forget}

```rust
pub fn forget(mut self)
```

Switches everything to no automatic unsubscription, living until the mod unloads.

## `PlayerIdentity` {#PlayerIdentity}

```rust
pub struct PlayerIdentity {
    pub name: String,
    pub xuid: String,
    pub uuid: String,
}
```

The player identity inside an event.

The three fields each have their own use and must not be mixed:
* `xuid` is unique and cannot be changed, and is the only key for permissions or
  economy. It may be empty in offline mode.
* `uuid` is equally stable and suits a save key.
* `name` is for display. A player can change their display name, and host name
  resolution falls back to the display name when the account name misses (see
  `bridge::resolvePlayer`), so it must not be used as an identity.

- Implements: `Debug`, `Clone`, `Default`, `PartialEq`, `Eq`, `From`

### `PlayerIdentity::selector` {#PlayerIdentity.selector}

```rust
pub fn selector(&self) -> crate::sel::PlayerSel
```

Returns a selector usable for calling the API.

The xuid comes first, since it cannot be forged. An empty xuid, in offline mode, falls
back to the uuid and then to the name. On a fall back to `Name`,
[`PlayerSel::is_stable`](player.md#PlayerSel.is_stable) is false, which a permission or economy decision should treat
with care, since a name goes through the display-name fallback; see the `sel` module.

- Return type: `crate::sel::PlayerSel`

### `PlayerIdentity::is_identified` {#PlayerIdentity.is_identified}

```rust
pub fn is_identified(&self) -> bool
```

Whether there is a reliable identity, an xuid or a uuid. Ask this before using one as
a permission key.

- Return type: `bool`

## `Listener` {#Listener}

```rust
pub struct Listener {
    // private fields
}
```

A subscription handle. Dropping it unsubscribes.

[`Listener::forget`](event.md#Listener.forget) keeps it alive until the mod unloads. The host removes whatever
remains at unload, really calling `removeListener` and reporting a failure rather than
staying silent.

- Implements: `Drop`

### `Listener::event_id` {#Listener.event_id}

```rust
pub fn event_id(&self) -> &str
```

The id of the subscribed event.

- Return type: `&str`

### `Listener::forget` {#Listener.forget}

```rust
pub fn forget(mut self)
```

Gives up automatic unsubscription. The closure leaks with it and lives until the
process ends.

## `Priority` {#Priority}

```rust
pub enum Priority {
        Highest = 0,
        High = 1,
        #[default]
        Normal = 2,
        Low = 3,
        Lowest = 4,
}
```

Dispatch priority. A lower value runs first, aligned with 0..4 in the ABI.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`, `PartialOrd`, `Ord`, `Default`

## `EventRef` {#EventRef}

```rust
pub type EventRef<'a> = Event<'a>;
```

An earlier generation called this `EventRef`. The name is kept and points at the same
type.

## `EventPriority` {#EventPriority}

```rust
pub type EventPriority = Priority;
```

An earlier generation called the priority `EventPriority`.
