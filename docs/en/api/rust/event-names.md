# levilamina::event::names

Event id constants.

An event id is a string, and the cost of a typo is a subscription that succeeds
silently while the callback never fires once. The host reports a failed resolution
and lists nearby ids (§5.3), and these constants move that to compile time.

Two kinds. A registry event always gets the full name `ll::event::<ClassName>`, with
no category segment in between. The host accepts a unique suffix too, but a suffix
becomes ambiguous once upstream adds an event of the same name. A synthetic event, a
bare name, is one Pier builds with a native detour to fill a point LL does not cover.

Each entry states whether it can be cancelled. Calling `Event::cancel()` on an event
that cannot be is a harmless no-op, and it leaves the impression of having blocked it.

## Functions {#functions}

### `event::names::is_cancellable` {#fn.is_cancellable}

```rust
pub fn is_cancellable(id: &str) -> Option<bool>
```

Whether this event can be cancelled.

* `Some(true)`: it can, and `Event::cancel()` takes effect;
* `Some(false)`: it cannot, and `cancel()` returns `Err` saying which event to block;
* `None`: it is not in the tables. An event a third-party mod emits itself, or a new
  upstream event these tables have not caught up with, both land here. `cancel()` writes
  back as usual and blocks nothing, and it can confirm nothing on the caller's behalf.

- Parameters:
    - id : `&str`
- Return type: `Option<bool>`

### `event::names::why_not_cancellable` {#fn.why_not_cancellable}

```rust
pub fn why_not_cancellable(id: &str) -> Option<&'static str>
```

Why an observation-only event cannot be cancelled, and which event to block instead.

- Parameters:
    - id : `&str`
- Return type: `Option<&'static str>`

## Constants {#constants}

| Name | Value | Description |
|---|---|---|
| <span id="PLAYER_JOIN"></span>`PLAYER_JOIN` | `"ll::event::PlayerJoinEvent"` | Cancellable, as a `Cancellable<ServerPlayerEvent>`. Cancelling refuses the join. |
| <span id="PLAYER_CONNECT"></span>`PLAYER_CONNECT` | `"ll::event::PlayerConnectEvent"` | Cancellable. |
| <span id="PLAYER_DISCONNECT"></span>`PLAYER_DISCONNECT` | `"ll::event::PlayerDisconnectEvent"` | Observation only; the player is already leaving and cannot be stopped. |
| <span id="PLAYER_DIE"></span>`PLAYER_DIE` | `"ll::event::PlayerDieEvent"` | Observation only. |
| <span id="PLAYER_RESPAWN"></span>`PLAYER_RESPAWN` | `"ll::event::PlayerRespawnEvent"` | Observation only. |
| <span id="PLAYER_CHAT"></span>`PLAYER_CHAT` | `"ll::event::PlayerChatEvent"` | Cancellable. The payload carries `message`, and editing it rewrites what was said. |
| <span id="PLAYER_DESTROY_BLOCK"></span>`PLAYER_DESTROY_BLOCK` | `"ll::event::PlayerDestroyBlockEvent"` | Cancellable. A player mined a block. |
| <span id="PLAYER_PLACING_BLOCK"></span>`PLAYER_PLACING_BLOCK` | `"ll::event::PlayerPlacingBlockEvent"` | Cancellable. `PlayerPlacingBlockEvent` is about to place while `Placed` is already done. |
| <span id="PLAYER_PLACED_BLOCK"></span>`PLAYER_PLACED_BLOCK` | `"ll::event::PlayerPlacedBlockEvent"` | Observation only; it is already done. Use [`PLAYER_PLACING_BLOCK`](event-names.md#PLAYER_PLACING_BLOCK) to block it. |
| <span id="PLAYER_INTERACT_BLOCK"></span>`PLAYER_INTERACT_BLOCK` | `"ll::event::PlayerInteractBlockEvent"` | Cancellable. Right-clicking a block: opening a chest, pressing a button, using a tool. |
| <span id="PLAYER_USE_ITEM"></span>`PLAYER_USE_ITEM` | `"ll::event::PlayerUseItemEvent"` | Cancellable. Banning a food or a potion is blocked here and not at [`PLAYER_USE_ITEM_COMPLETE`](event-names.md#PLAYER_USE_ITEM_COMPLETE). |
| <span id="PLAYER_PICK_UP_ITEM"></span>`PLAYER_PICK_UP_ITEM` | `"ll::event::PlayerPickUpItemEvent"` | Cancellable. |
| <span id="PLAYER_ATTACK"></span>`PLAYER_ATTACK` | `"ll::event::PlayerAttackEvent"` | Cancellable. Note that it cannot tell attacking a player from attacking a mob; that needs [`PLAYER_ATTACK_TARGET`](event-names.md#PLAYER_ATTACK_TARGET), a synthetic event whose payload carries `targetIsPlayer`. |
| <span id="PLAYER_SWING"></span>`PLAYER_SWING` | `"ll::event::PlayerSwingEvent"` | Observation only. |
| <span id="PLAYER_JUMP"></span>`PLAYER_JUMP` | `"ll::event::PlayerJumpEvent"` | Observation only. |
| <span id="PLAYER_SNEAKING"></span>`PLAYER_SNEAKING` | `"ll::event::PlayerSneakingEvent"` | Cancellable, since the base `PlayerSneakEvent` is a `Cancellable<>`. |
| <span id="PLAYER_SNEAKED"></span>`PLAYER_SNEAKED` | `"ll::event::PlayerSneakedEvent"` | Cancellable, as above. |
| <span id="PLAYER_SPRINTING"></span>`PLAYER_SPRINTING` | `"ll::event::PlayerSprintingEvent"` | Observation only: `PlayerSprintEvent` is not Cancellable, unlike Sneak. |
| <span id="PLAYER_SPRINTED"></span>`PLAYER_SPRINTED` | `"ll::event::PlayerSprintedEvent"` | Observation only, as above. |
| <span id="PLAYER_ADD_EXPERIENCE"></span>`PLAYER_ADD_EXPERIENCE` | `"ll::event::PlayerAddExperienceEvent"` | Cancellable. |
| <span id="PLAYER_CHANGE_PERM"></span>`PLAYER_CHANGE_PERM` | `"ll::event::PlayerChangePermEvent"` | Cancellable. |
| <span id="PLAYER_LEFT_CLICK"></span>`PLAYER_LEFT_CLICK` | `"ll::event::PlayerLeftClickEvent"` | Observation only. It is the base of `PlayerAttackEvent` and `PlayerDestroyBlockEvent`; subscribe to those two to block a specific action. |
| <span id="PLAYER_RIGHT_CLICK"></span>`PLAYER_RIGHT_CLICK` | `"ll::event::PlayerRightClickEvent"` | Observation only; a base class, as above. |
| <span id="ACTOR_HURT"></span>`ACTOR_HURT` | `"ll::event::ActorHurtEvent"` | Cancellable. |
| <span id="MOB_DIE"></span>`MOB_DIE` | `"ll::event::MobDieEvent"` | Observation only: `MobEvent` is not Cancellable. |
| <span id="SPAWNING_MOB"></span>`SPAWNING_MOB` | `"ll::event::SpawningMobEvent"` | Cancellable. |
| <span id="SPAWNED_MOB"></span>`SPAWNED_MOB` | `"ll::event::SpawnedMobEvent"` | Observation only; it is already done. |
| <span id="BLOCK_CHANGED"></span>`BLOCK_CHANGED` | `"ll::event::BlockChangedEvent"` | Observation only. Block changes are blocked through [`PLAYER_PLACING_BLOCK`](event-names.md#PLAYER_PLACING_BLOCK), [`PLAYER_DESTROY_BLOCK`](event-names.md#PLAYER_DESTROY_BLOCK) or [`BLOCK_DESTROY`](event-names.md#BLOCK_DESTROY), the last of which covers non-player sources. |
| <span id="FIRE_SPREAD"></span>`FIRE_SPREAD` | `"ll::event::FireSpreadEvent"` | Cancellable. |
| <span id="SERVER_STARTED"></span>`SERVER_STARTED` | `"ll::event::ServerStartedEvent"` | Observation only. |
| <span id="SERVER_STOPPING"></span>`SERVER_STOPPING` | `"ll::event::ServerStoppingEvent"` | Observation only. |
| <span id="SERVER_LEVEL_TICK"></span>`SERVER_LEVEL_TICK` | `"ll::event::ServerLevelTickEvent"` | Observation only, once per tick, so the test has to be cheap or use `Host::schedule`. |
| <span id="EXECUTING_COMMAND"></span>`EXECUTING_COMMAND` | `"ll::event::ExecutingCommandEvent"` | Cancellable. A command allowlist or an audit hooks here. |
| <span id="EXECUTED_COMMAND"></span>`EXECUTED_COMMAND` | `"ll::event::ExecutedCommandEvent"` | Observation only; it is already done. |
| <span id="BLOCK_DESTROY"></span>`BLOCK_DESTROY` | `"BlockDestroyEvent"` | Cancellable. Something removed this cell, without asking who. It fills the largest gap: an enderman taking a grass block, a wither smashing a wall, a creeper crater, a silverfish burrowing into stone, `/setblock ... destroy`, another plugin calling destroyBlock. None of these fired any event before, and plot protection could only watch blocks vanish. Payload: `x` `y` `z` `dim` `dropResources` `block`. Note there is no who: the engine already dropped the source at this layer, and inventing one would only mislead. |
| <span id="EXPLOSION"></span>`EXPLOSION` | `"ExplosionEvent"` | Cancellable. Cancelling means the explosion does not happen at all, damage and blocks alike. Payload: `x` `y` `z` `dim` `radius` `maxResistance` `fire` `breaksBlocks` `underwater` `sourceIsPlayer` `sourceId` `source`. |
| <span id="LIQUID_FLOW"></span>`LIQUID_FLOW` | `"LiquidFlowEvent"` | Cancellable. Water or lava is about to spread into a cell. It blocks a neighbor pouring water on their own ground and having it flow across: the pour is legitimate and the spreading step is the crossing. Payload: the target cell `x` `y` `z` `dim`, the source cell `fromX` `fromY` `fromZ`, plus `direction` and `liquid`. A hot path: liquid spreads every tick, so the test has to be cheap. |
| <span id="FARMLAND_DECAY"></span>`FARMLAND_DECAY` | `"FarmlandDecayEvent"` | Cancellable. Something fell from a height and trampled farmland into dirt, needing no permission and leaving no log. Payload: `x` `y` `z` `dim` `fallDistance` `byPlayer` `actor`, plus `_player` for a player. |
| <span id="PISTON_PUSH"></span>`PISTON_PUSH` | `"PistonPushEvent"` | Cancellable. A piston is about to push or pull a set of blocks. It blocks a cross-plot piston machine. Payload: the piston `x` `y` `z` `dim`, `facing:[x,y,z]` and `attached:[[x,y,z],...]`. |
| <span id="CHEST_PAIR"></span>`CHEST_PAIR` | `"ChestPairEvent"` | Cancellable. Two chests are about to pair into a double chest. A chest placed against the boundary pairs with the neighbor's, and opening the near half shows everything in theirs. Container protection decides on the cell you clicked, and that cell really belongs to the placer. Payload: `x` `y` `z` `dim` `otherX` `otherY` `otherZ`. |
| <span id="SPAWN_ITEM_ACTOR"></span>`SPAWN_ITEM_ACTOR` | `"SpawnItemActorEvent"` | Cancellable. Cancelling means the drop is not spawned and the item disappears rather than lying on the ground. For anti-duplication and drop ownership. Payload: `x` `y` `z` `dim` `item` `count` `throwTime` `sourceIsPlayer` `source`. |
| <span id="WEATHER_CHANGE"></span>`WEATHER_CHANGE` | `"WeatherChangeEvent"` | Observation only. A weather change. Payload: `rainLevel` `rainTime` `lightningLevel` `lightningTime`. |
| <span id="PLAYER_SLEEP"></span>`PLAYER_SLEEP` | `"PlayerSleepEvent"` | Cancellable through the engine's own `NotPossibleHere`, so the client shows the vanilla message. For someone else's bed, a game mode where the night must not be skipped, and a dimension where a bed is a bomb. |
| <span id="PLAYER_CHANGE_SLOT"></span>`PLAYER_CHANGE_SLOT` | `"PlayerChangeSlotEvent"` | Observation only: the return is a reference to the item in the new slot, and cancelling would mean inventing one out of nothing. Payload: `from` `to` `item` `dim` `_player`. |
| <span id="PLAYER_USE_ITEM_COMPLETE"></span>`PLAYER_USE_ITEM_COMPLETE` | `"PlayerUseItemCompleteEvent"` | Observation only: cancelling here leaves the player holding the item forever. Finishing eating, drinking or lowering a spyglass. Banning a food blocks the start of the use at [`PLAYER_USE_ITEM`](event-names.md#PLAYER_USE_ITEM). |
| <span id="ARMOR_STAND_SWAP_ITEM"></span>`ARMOR_STAND_SWAP_ITEM` | `"ArmorStandSwapItemEvent"` | Cancellable. A player swaps equipment with an armor stand, which is neither a container nor a block, so neither protection sees it. |
| <span id="PLAYER_ATTACK_ITEM_FRAME"></span>`PLAYER_ATTACK_ITEM_FRAME` | `"PlayerAttackItemFrameEvent"` | Cancellable. Left-clicking an item frame to take the item: not breaking a block, since the frame remains, and not hitting an actor, since the frame is a block. |
| <span id="PLAYER_OPERATED_ITEM_FRAME"></span>`PLAYER_OPERATED_ITEM_FRAME` | `"PlayerOperatedItemFrameEvent"` | Cancellable. Rotating the item in a frame, the other half of the frame pair: the attack half is [`PLAYER_ATTACK_ITEM_FRAME`](event-names.md#PLAYER_ATTACK_ITEM_FRAME). Neither is a block place nor a hit on an actor, so nothing else sees it. |
| <span id="PLAYER_EDIT_SIGN"></span>`PLAYER_EDIT_SIGN` | `"PlayerEditSignEvent"` | Cancellable. Editing the text of a sign already placed. Placing a sign is a block place and was already covered; changing its text was neither a place nor an interact. |
| <span id="PLAYER_REQUEST_ITEM_ACTION"></span>`PLAYER_REQUEST_ITEM_ACTION` | `"PlayerRequestItemActionEvent"` | Not raised by this host. The name is kept for mods that already subscribe to it, and [`is_cancellable`](event-names.md#fn.is_cancellable) answers `Some(false)` for it so no mod mistakes it for a gate. It is left out of [`ALL_SYNTHETIC`](event-names.md#ALL_SYNTHETIC), which lists only what the host registers. |
| <span id="PLAYER_ATTACK_TARGET"></span>`PLAYER_ATTACK_TARGET` | `"PlayerAttackTargetEvent"` | Cancellable. A player attacks a target. The payload carries `targetIsPlayer`, which is how the pvp flag tells attacking a player from attacking a mob, and `targetKnown`: when the target could not be read it is 0 and `targetIsPlayer` is 1, so a pvp rule refuses. |
| <span id="PLAYER_CHANGE_GAME_MODE"></span>`PLAYER_CHANGE_GAME_MODE` | `"PlayerChangeGameModeEvent"` | Cancellable. A player changes game mode, including through `/gamemode` and calls from other plugins. |
| <span id="PLAYER_DROP_ITEM"></span>`PLAYER_DROP_ITEM` | `"PlayerDropItemEvent"` | Cancellable. Dropping an item, covering both dropping by hand and dragging out of the inventory UI. |
| <span id="PLAYER_INTERACT_ENTITY"></span>`PLAYER_INTERACT_ENTITY` | `"PlayerInteractEntityEvent"` | Cancellable. Right-clicking an actor: villager trading, feeding an animal, shearing. |
| <span id="PLAYER_STEP_ON_PRESSURE_PLATE"></span>`PLAYER_STEP_ON_PRESSURE_PLATE` | `"PlayerStepOnPressurePlateEvent"` | Cancellable. A player steps on a pressure plate or a tripwire. Throttled internally at 250 ms per (player, position). |
| <span id="ACTOR_STEP_ON_PRESSURE_PLATE"></span>`ACTOR_STEP_ON_PRESSURE_PLATE` | `"ActorStepOnPressurePlateEvent"` | Cancellable. As above, but for a non-player actor, using a separate throttle table. |
| <span id="PLAYER_SPAWN_PROJECTILE"></span>`PLAYER_SPAWN_PROJECTILE` | `"PlayerSpawnProjectileEvent"` | Cancellable. A player launches a projectile: a snowball, an ender pearl, an arrow, a trident, a crossbow firework. |
| <span id="PLAYER_PUSH_ENTITY"></span>`PLAYER_PUSH_ENTITY` | `"PlayerPushEntityEvent"` | Cancellable. A player pushes an actor. Throttled internally. |
| <span id="PLAYER_RIDE"></span>`PLAYER_RIDE` | `"PlayerRideEvent"` | Cancellable. A player mounts a vehicle. |
| <span id="ACTOR_RIDE"></span>`ACTOR_RIDE` | `"ActorRideEvent"` | Cancellable. A non-player actor mounts a vehicle, such as a villager in a boat or a pig in a minecart. The payload uses `passenger` and `passengerId` instead of `_player`. |
| <span id="PLAYER_TAKE_ENTITY"></span>`PLAYER_TAKE_ENTITY` | `"PlayerTakeEntityEvent"` | Cancellable. A player picks up a projectile actor such as an arrow or a trident. |
| <span id="PLAYER_OPEN_CONTAINER"></span>`PLAYER_OPEN_CONTAINER` | `"PlayerOpenContainerEvent"` | Cancellable. A player opens a container. |
| <span id="PLAYER_START_DESTROY_BLOCK"></span>`PLAYER_START_DESTROY_BLOCK` | `"PlayerStartDestroyBlockEvent"` | Observation only, emitted before origin, for recording who started mining which cell. |
| <span id="PLAYER_CHANGE_DIMENSION"></span>`PLAYER_CHANGE_DIMENSION` | `"PlayerChangeDimensionEvent"` | Cancellable, and the target can be rewritten. A player changes dimension, from any cause: a portal, a teleport, `/execute in`, a respawn. Payload: `from` `to` `to_x` `to_y` `to_z` `use_portal` `respawn` `_player`. `use_portal` is what separates walking into a portal from being teleported, and a rule that treats the two alike will surprise whoever wrote it. Answering with `to`, or with all three of `to_x` `to_y` `to_z`, reroutes the transfer; the engine then performs it itself. Teleporting from the callback instead re-enters this event while the player still stands in the portal, and repeats every tick. |
| <span id="PORTAL_CREATE"></span>`PORTAL_CREATE` | `"PortalCreateEvent"` | Cancellable. A frame is about to become a nether portal. It fires after the engine has measured the frame and found the fire, so a consumer needs no geometry of its own. Payload: `dim` `x` `y` `z`. There is no player: fire reaches a frame from a dispenser, from lightning and from spreading, and a rule keyed on a player would miss those. |
| <span id="HOPPER_TRANSFER"></span>`HOPPER_TRANSFER` | `"HopperTransferEvent"` | Observation only. A hopper transfers an item. Payload: `x` `y` `z` `slot` `item` `count` `old_item` `old_count`. |
| <span id="PLAYER_USE_ITEM_ON"></span>`PLAYER_USE_ITEM_ON` | `"PlayerUseItemOnEvent"` | Cancellable. A player uses an item on a block, placing it or right-clicking with it. |
| <span id="ALL_SYNTHETIC"></span>`ALL_SYNTHETIC` | `&[ BLOCK_DESTROY, EXPLOSION, LIQUID_FLOW, FARMLAND_DECAY, PISTON_PUSH, CHEST_PAIR, SPAWN_ITEM_ACTOR, WEATHER_CHANGE, PLAYER_SLEEP, PLAYER_CHANGE_SLOT, PLAYER_USE_ITEM_COMPLETE, ARMOR_STAND_SWAP_ITEM, PLAYER_ATTACK_ITEM_FRAME, PLAYER_OPERATED_ITEM_FRAME, PLAYER_EDIT_SIGN, PLAYER_ATTACK_TARGET, PLAYER_CHANGE_GAME_MODE, PLAYER_DROP_ITEM, PLAYER_INTERACT_ENTITY, PLAYER_STEP_ON_PRESSURE_PLATE, ACTOR_STEP_ON_PRESSURE_PLATE, PLAYER_SPAWN_PROJECTILE, PLAYER_PUSH_ENTITY, PLAYER_RIDE, ACTOR_RIDE, PLAYER_TAKE_ENTITY, PLAYER_OPEN_CONTAINER, PLAYER_START_DESTROY_BLOCK, PLAYER_CHANGE_DIMENSION, PORTAL_CREATE, HOPPER_TRANSFER, PLAYER_USE_ITEM_ON, ]` | The ids of every synthetic event. Comparing it against [`super::list()`](dimensions.md#fn.list) at startup shows which capability packages this host was built with. |
