# Event payloads

Every event reaches a mod as one SNBT text, whatever language the mod is written in. This
page is the reference for what that text carries. The language guides show how to
subscribe; the fields are the same everywhere.

## Two kinds of event

**LeviLamina events** are the ones in LeviLamina's own event registry, subscribed by their
full id, such as `ll::event::MobDieEvent`. Their payload is LeviLamina's own serialization
of the event, and Pier adds fields to it, described below, because that serialization
names engine objects without describing them.

**Synthetic events** are the ones Pier raises from its own hooks, such as
`PlayerAttackTargetEvent`. Their payload is written by Pier and documented per event in
each binding's list of event names.

## What Pier adds to a LeviLamina event

An engine object in the payload is described next to it, under the same key with a leading
underscore. A field that is there was read by the host. A field it could not read is left
out or listed in `_unresolved`, and Pier puts no default in its place.

| The payload names | Pier adds | Shape |
|---|---|---|
| the player the event is about (`self`) | `_player` | `{name, xuid, uuid, pos:{x,y,z}}` |
| any other actor, or a non-player `self` | `_<key>` | `{uid, type, name, isPlayer, dim}`, and for a player also `xuid` and `realName` |
| a damage source | `_<key>` | `{cause, attackerUid?, attacker?, projectileUid?, projectile?}` |
| an entity type | `_identifier` | `{full, namespace, name}` |
| a block or an item | `_<key>` | `{name}` |
| a block source, or a `self` actor | `dim` | the dimension id, when the event did not carry one |

`uid` is the actor's unique id, the same number every actor slot takes, so a mod can act on
the actor a payload names. `name` is the name tag, which a plugin may have changed;
`realName` is the player's account name.

### Who killed whom

`MobDieEvent`, `PlayerDieEvent` and `ActorHurtEvent` carry the damage source. When the
damage came from an actor, the source has:

- `attackerUid`: the actor responsible. For a projectile that is the one that fired it,
  not the arrow.
- `attacker`: that actor described, when it still exists.
- `projectileUid` and `projectile`: the arrow, trident or other projectile, for projectile
  damage only.

The uid is written even when the actor is already gone, so a kill by an arrow whose shooter
logged out still says who. An absent `attacker` therefore means "no longer here", and an
absent `attackerUid` means the damage did not come from an actor at all: fire, a fall, the
void. `cause` is the engine's damage cause number either way.

### When something could not be resolved

An actor that is neither an online player nor in the runtime actor table cannot be
described. Its key is listed in `_unresolved` instead, and the event still arrives. A mod
making a protection decision on such a payload refuses rather than guesses; the Rust SDK's
`check_complete()` and `dim()` do that for you.

## Conventions of synthetic events

Synthetic payloads follow three rules that let a mod tell an answer from a failure:

- **`partial`**: `"partial":1` means at least one field could not be read and holds a
  default only. It appears in `ExplosionEvent`, `FarmlandDecayEvent`,
  `SpawnItemActorEvent`, `ChestPairEvent` and `PistonPushEvent`, and is absent when every
  read succeeded.
- **`targetKnown`**: in `PlayerAttackTargetEvent` and `PlayerInteractEntityEvent`, 0 means
  the target could not be read. `targetIsPlayer` is 1 then, so a PvP rule refuses.
- **Cancelling**: the callback receives a write-back, and a cancellable event is cancelled
  by writing it back with the cancel set; the bindings wrap this, as `ev.cancel()` in Rust.
  An event marked observation-only cannot be cancelled, and its documentation names the
  event to block instead.

## When an event does not arrive

- The event may be turned off: a synthetic event named in `hooks.disabled` in
  `config.json` refuses the subscription with a line saying so. See
  [Configuring](configuration.md).
- A LeviLamina event whose engine hook point changed may stop arriving on a new engine
  version; [Troubleshooting](troubleshooting.md) lists how to tell.
