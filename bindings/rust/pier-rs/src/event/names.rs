//! Event id constants.
//!
//! An event id is a string, and the cost of a typo is a subscription that succeeds
//! silently while the callback never fires once. The host reports a failed resolution
//! and lists nearby ids (§5.3), and these constants move that to compile time.
//!
//! Two kinds. A registry event always gets the full name `ll::event::<ClassName>`, with
//! no category segment in between. The host accepts a unique suffix too, but a suffix
//! becomes ambiguous once upstream adds an event of the same name. A synthetic event, a
//! bare name, is one Pier builds with a native detour to fill a point LL does not cover.
//!
//! Each entry states whether it can be cancelled. Calling `Event::cancel()` on an event
//! that cannot be is a harmless no-op, and it leaves the impression of having blocked it.

/* LeviLamina registry events */

/// Cancellable, as a `Cancellable<ServerPlayerEvent>`. Cancelling refuses the join.
pub const PLAYER_JOIN: &str = "ll::event::PlayerJoinEvent";
/// Cancellable.
pub const PLAYER_CONNECT: &str = "ll::event::PlayerConnectEvent";
/// Observation only; the player is already leaving and cannot be stopped.
pub const PLAYER_DISCONNECT: &str = "ll::event::PlayerDisconnectEvent";
/// Observation only.
pub const PLAYER_DIE: &str = "ll::event::PlayerDieEvent";
/// Observation only.
pub const PLAYER_RESPAWN: &str = "ll::event::PlayerRespawnEvent";

/// Cancellable. The payload carries `message`, and editing it rewrites what was said.
pub const PLAYER_CHAT: &str = "ll::event::PlayerChatEvent";

/// Cancellable. A player mined a block.
pub const PLAYER_DESTROY_BLOCK: &str = "ll::event::PlayerDestroyBlockEvent";
/// Cancellable. `PlayerPlacingBlockEvent` is about to place while `Placed` is already done.
pub const PLAYER_PLACING_BLOCK: &str = "ll::event::PlayerPlacingBlockEvent";
/// Observation only; it is already done. Use [`PLAYER_PLACING_BLOCK`] to block it.
pub const PLAYER_PLACED_BLOCK: &str = "ll::event::PlayerPlacedBlockEvent";
/// Cancellable. Right-clicking a block: opening a chest, pressing a button, using a tool.
pub const PLAYER_INTERACT_BLOCK: &str = "ll::event::PlayerInteractBlockEvent";
/// Cancellable. Banning a food or a potion is blocked here and not at
/// [`PLAYER_USE_ITEM_COMPLETE`].
pub const PLAYER_USE_ITEM: &str = "ll::event::PlayerUseItemEvent";
/// Cancellable.
pub const PLAYER_PICK_UP_ITEM: &str = "ll::event::PlayerPickUpItemEvent";
/// Cancellable. Note that it cannot tell attacking a player from attacking a mob; that
/// needs [`PLAYER_ATTACK_TARGET`], a synthetic event whose payload carries `targetIsPlayer`.
pub const PLAYER_ATTACK: &str = "ll::event::PlayerAttackEvent";
/// Observation only.
pub const PLAYER_SWING: &str = "ll::event::PlayerSwingEvent";
/// Observation only.
pub const PLAYER_JUMP: &str = "ll::event::PlayerJumpEvent";
/// Cancellable, since the base `PlayerSneakEvent` is a `Cancellable<>`.
pub const PLAYER_SNEAKING: &str = "ll::event::PlayerSneakingEvent";
/// Cancellable, as above.
pub const PLAYER_SNEAKED: &str = "ll::event::PlayerSneakedEvent";
/// Observation only: `PlayerSprintEvent` is not Cancellable, unlike Sneak.
pub const PLAYER_SPRINTING: &str = "ll::event::PlayerSprintingEvent";
/// Observation only, as above.
pub const PLAYER_SPRINTED: &str = "ll::event::PlayerSprintedEvent";
/// Cancellable.
pub const PLAYER_ADD_EXPERIENCE: &str = "ll::event::PlayerAddExperienceEvent";
/// Cancellable.
pub const PLAYER_CHANGE_PERM: &str = "ll::event::PlayerChangePermEvent";
/// Observation only. It is the base of `PlayerAttackEvent` and
/// `PlayerDestroyBlockEvent`; subscribe to those two to block a specific action.
pub const PLAYER_LEFT_CLICK: &str = "ll::event::PlayerLeftClickEvent";
/// Observation only; a base class, as above.
pub const PLAYER_RIGHT_CLICK: &str = "ll::event::PlayerRightClickEvent";

/// Cancellable.
pub const ACTOR_HURT: &str = "ll::event::ActorHurtEvent";
/// Observation only: `MobEvent` is not Cancellable.
pub const MOB_DIE: &str = "ll::event::MobDieEvent";
/// Cancellable.
pub const SPAWNING_MOB: &str = "ll::event::SpawningMobEvent";
/// Observation only; it is already done.
pub const SPAWNED_MOB: &str = "ll::event::SpawnedMobEvent";

/// Observation only. Block changes are blocked through [`PLAYER_PLACING_BLOCK`],
/// [`PLAYER_DESTROY_BLOCK`] or [`BLOCK_DESTROY`], the last of which covers non-player sources.
pub const BLOCK_CHANGED: &str = "ll::event::BlockChangedEvent";
/// Cancellable.
pub const FIRE_SPREAD: &str = "ll::event::FireSpreadEvent";

/// Observation only.
pub const SERVER_STARTED: &str = "ll::event::ServerStartedEvent";
/// Observation only.
pub const SERVER_STOPPING: &str = "ll::event::ServerStoppingEvent";
/// Observation only, once per tick, so the test has to be cheap or use `Host::schedule`.
pub const SERVER_LEVEL_TICK: &str = "ll::event::ServerLevelTickEvent";

/// Cancellable. A command allowlist or an audit hooks here.
pub const EXECUTING_COMMAND: &str = "ll::event::ExecutingCommandEvent";
/// Observation only; it is already done.
pub const EXECUTED_COMMAND: &str = "ll::event::ExecutedCommandEvent";

/* Pier synthetic events, which LeviLamina does not have */

/// Cancellable. Something removed this cell, without asking who.
///
/// It fills the largest gap: an enderman taking a grass block, a wither smashing a wall,
/// a creeper crater, a silverfish burrowing into stone, `/setblock ... destroy`, another
/// plugin calling destroyBlock. None of these fired any event before, and plot protection
/// could only watch blocks vanish.
///
/// Payload: `x` `y` `z` `dim` `dropResources` `block`.
/// Note there is no who: the engine already dropped the source at this layer, and
/// inventing one would only mislead.
pub const BLOCK_DESTROY: &str = "BlockDestroyEvent";

/// Cancellable. Cancelling means the explosion does not happen at all, damage and blocks
/// alike.
/// Payload: `x` `y` `z` `dim` `radius` `maxResistance` `fire` `breaksBlocks`
/// `underwater` `sourceIsPlayer` `sourceId` `source`.
pub const EXPLOSION: &str = "ExplosionEvent";

/// Cancellable. Water or lava is about to spread into a cell. It blocks a neighbor pouring
/// water on their own ground and having it flow across: the pour is legitimate and the
/// spreading step is the crossing.
/// Payload: the target cell `x` `y` `z` `dim`, the source cell `fromX` `fromY` `fromZ`,
/// plus `direction` and `liquid`.
///
/// A hot path: liquid spreads every tick, so the test has to be cheap.
pub const LIQUID_FLOW: &str = "LiquidFlowEvent";

/// Cancellable. Something fell from a height and trampled farmland into dirt, needing no
/// permission and leaving no log.
/// Payload: `x` `y` `z` `dim` `fallDistance` `byPlayer` `actor`, plus `_player` for a player.
pub const FARMLAND_DECAY: &str = "FarmlandDecayEvent";

/// Cancellable. A piston is about to push or pull a set of blocks. It blocks a cross-plot
/// piston machine.
/// Payload: the piston `x` `y` `z` `dim`, `facing:[x,y,z]` and `attached:[[x,y,z],...]`.
pub const PISTON_PUSH: &str = "PistonPushEvent";

/// Cancellable. Two chests are about to pair into a double chest.
/// A chest placed against the boundary pairs with the neighbor's, and opening the near half
/// shows everything in theirs. Container protection decides on the cell you clicked, and
/// that cell really belongs to the placer.
/// Payload: `x` `y` `z` `dim` `otherX` `otherY` `otherZ`.
pub const CHEST_PAIR: &str = "ChestPairEvent";

/// Cancellable. Cancelling means the drop is not spawned and the item disappears rather
/// than lying on the ground.
/// For anti-duplication and drop ownership. Payload: `x` `y` `z` `dim` `item` `count`
/// `throwTime`
/// `sourceIsPlayer` `source`.
pub const SPAWN_ITEM_ACTOR: &str = "SpawnItemActorEvent";

/// Observation only. A weather change. Payload: `rainLevel` `rainTime` `lightningLevel`
/// `lightningTime`.
pub const WEATHER_CHANGE: &str = "WeatherChangeEvent";

/// Cancellable through the engine's own `NotPossibleHere`, so the client shows the vanilla
/// message.
/// For someone else's bed, a game mode where the night must not be skipped, and a dimension
/// where a bed is a bomb.
pub const PLAYER_SLEEP: &str = "PlayerSleepEvent";

/// Observation only: the return is a reference to the item in the new slot, and cancelling
/// would mean inventing one out of nothing.
/// Payload: `from` `to` `item` `dim` `_player`.
pub const PLAYER_CHANGE_SLOT: &str = "PlayerChangeSlotEvent";

/// Observation only: cancelling here leaves the player holding the item forever. Finishing
/// eating, drinking or lowering a spyglass.
/// Banning a food blocks the start of the use at [`PLAYER_USE_ITEM`].
pub const PLAYER_USE_ITEM_COMPLETE: &str = "PlayerUseItemCompleteEvent";

/// Cancellable. A player swaps equipment with an armor stand, which is neither a container
/// nor a block, so neither protection sees it.
pub const ARMOR_STAND_SWAP_ITEM: &str = "ArmorStandSwapItemEvent";

/// Cancellable. Left-clicking an item frame to take the item: not breaking a block, since
/// the frame remains, and not hitting an actor, since the frame is a block.
pub const PLAYER_ATTACK_ITEM_FRAME: &str = "PlayerAttackItemFrameEvent";

/// Cancellable. A player attacks a target. The payload carries `targetIsPlayer`, which is
/// how the pvp flag tells attacking a player from attacking a mob.
pub const PLAYER_ATTACK_TARGET: &str = "PlayerAttackTargetEvent";

/// Cancellable. A player changes game mode, including through `/gamemode` and calls from
/// other plugins.
pub const PLAYER_CHANGE_GAME_MODE: &str = "PlayerChangeGameModeEvent";

/// Cancellable. Dropping an item, covering both dropping by hand and dragging out of the
/// inventory UI.
pub const PLAYER_DROP_ITEM: &str = "PlayerDropItemEvent";

/// Cancellable. Right-clicking an actor: villager trading, feeding an animal, shearing.
pub const PLAYER_INTERACT_ENTITY: &str = "PlayerInteractEntityEvent";

/// Cancellable. A player steps on a pressure plate or a tripwire. Throttled internally at
/// 250 ms per (player, position).
pub const PLAYER_STEP_ON_PRESSURE_PLATE: &str = "PlayerStepOnPressurePlateEvent";

/// Cancellable. As above, but for a non-player actor, using a separate throttle table.
pub const ACTOR_STEP_ON_PRESSURE_PLATE: &str = "ActorStepOnPressurePlateEvent";

/// Cancellable. A player launches a projectile: a snowball, an ender pearl, an arrow, a
/// trident, a crossbow firework.
pub const PLAYER_SPAWN_PROJECTILE: &str = "PlayerSpawnProjectileEvent";

/// Cancellable. A player pushes an actor. Throttled internally.
pub const PLAYER_PUSH_ENTITY: &str = "PlayerPushEntityEvent";

/// Cancellable. A player mounts a vehicle.
pub const PLAYER_RIDE: &str = "PlayerRideEvent";

/// Cancellable. A non-player actor mounts a vehicle, such as a villager in a boat or a pig
/// in a minecart.
/// The payload uses `passenger` and `passengerId` instead of `_player`.
pub const ACTOR_RIDE: &str = "ActorRideEvent";

/// Cancellable. A player picks up a projectile actor such as an arrow or a trident.
pub const PLAYER_TAKE_ENTITY: &str = "PlayerTakeEntityEvent";

/// Cancellable. A player opens a container.
pub const PLAYER_OPEN_CONTAINER: &str = "PlayerOpenContainerEvent";

/// Observation only, emitted before origin, for recording who started mining which cell.
pub const PLAYER_START_DESTROY_BLOCK: &str = "PlayerStartDestroyBlockEvent";

/// Observation only. A player changes dimension. Payload: `from` `to` `_player`.
pub const PLAYER_CHANGE_DIMENSION: &str = "PlayerChangeDimensionEvent";

/// Observation only. A hopper transfers an item. Payload: `x` `y` `z` `slot` `item` `count`
/// `old_item` `old_count`.
pub const HOPPER_TRANSFER: &str = "HopperTransferEvent";

/// Cancellable. A player uses an item on a block, placing it or right-clicking with it.
pub const PLAYER_USE_ITEM_ON: &str = "PlayerUseItemOnEvent";

/* The cancellability lookup tables */

/// The events known to be cancellable. On the LL side they derive from `Cancellable<>` and
/// on the synthetic side they go through `dispatchHookEventCancellable`. Both lists were
/// taken from the source and not from intuition.
const CANCELLABLE: &[&str] = &[
    // The LeviLamina registry
    "PlayerJoinEvent",
    "PlayerConnectEvent",
    "PlayerChatEvent",
    "PlayerDestroyBlockEvent",
    "PlayerPlacingBlockEvent",
    "PlayerInteractBlockEvent",
    "PlayerUseItemEvent",
    "PlayerPickUpItemEvent",
    "PlayerAttackEvent",
    "PlayerAddExperienceEvent",
    "PlayerChangePermEvent",
    "PlayerSneakEvent",
    "PlayerSneakingEvent",
    "PlayerSneakedEvent",
    "ActorHurtEvent",
    "SpawningMobEvent",
    "FireSpreadEvent",
    "ExecutingCommandEvent",
    "ConsoleOutputtingEvent",
    // Pier synthetic
    "BlockDestroyEvent",
    "ExplosionEvent",
    "LiquidFlowEvent",
    "FarmlandDecayEvent",
    "PistonPushEvent",
    "ChestPairEvent",
    "SpawnItemActorEvent",
    "PlayerSleepEvent",
    "ArmorStandSwapItemEvent",
    "PlayerAttackItemFrameEvent",
    "PlayerAttackTargetEvent",
    "PlayerChangeGameModeEvent",
    "PlayerDropItemEvent",
    "PlayerInteractEntityEvent",
    "PlayerStepOnPressurePlateEvent",
    "ActorStepOnPressurePlateEvent",
    "PlayerSpawnProjectileEvent",
    "PlayerPushEntityEvent",
    "PlayerRideEvent",
    "ActorRideEvent",
    "PlayerTakeEntityEvent",
    "PlayerOpenContainerEvent",
    "PlayerUseItemOnEvent",
];

/// The events known to be observation only, each with where to block instead.
///
/// Why this table exists: calling `cancel()` on an observation-only event is a harmless
/// no-op, harmless meaning it does not crash while leaving the impression of having
/// blocked it. In a protection mod that impression is far more dangerous than a crash.
const OBSERVE_ONLY: &[(&str, &str)] = &[
    // The LeviLamina registry
    ("PlayerDisconnectEvent", "the player is already leaving and cannot be stopped"),
    ("PlayerDieEvent", "the death is already done; block ActorHurtEvent to prevent it"),
    ("PlayerRespawnEvent", "the respawn is already done"),
    ("PlayerPlacedBlockEvent", "already done; use PlayerPlacingBlockEvent to block it"),
    ("PlayerSwingEvent", "a swing is purely presentational"),
    ("PlayerJumpEvent", "a jump cannot be cancelled"),
    ("PlayerSprintingEvent", "PlayerSprintEvent is not Cancellable, unlike Sneak"),
    ("PlayerSprintedEvent", "as PlayerSprintingEvent"),
    ("PlayerLeftClickEvent", "a base class; subscribe to PlayerAttackEvent or PlayerDestroyBlockEvent"),
    ("PlayerRightClickEvent", "a base class; subscribe to PlayerInteractBlockEvent or PlayerUseItemEvent"),
    ("PlayerClickEvent", "a base class"),
    ("MobDieEvent", "MobEvent is not Cancellable; block ActorHurtEvent to prevent death"),
    ("SpawnedMobEvent", "already done; use SpawningMobEvent to block it"),
    ("BlockChangedEvent", "block a block change through PlayerPlacingBlockEvent, PlayerDestroyBlockEvent or BlockDestroyEvent"),
    ("ServerLevelTickEvent", "a tick cannot be cancelled"),
    ("ServerStartedEvent", "cannot be cancelled"),
    ("ServerStoppingEvent", "cannot be cancelled"),
    ("ExecutedCommandEvent", "already done; use ExecutingCommandEvent to block it"),
    ("ConsoleOutputtedEvent", "already done; use ConsoleOutputtingEvent to block it"),
    // Pier synthetic, where the host uses dispatchHookEvent and the write-back sink is a no-op
    ("PlayerStartDestroyBlockEvent", "emitted before origin only to record who started mining which cell; use PlayerDestroyBlockEvent or BlockDestroyEvent to block it"),
    ("PlayerChangeDimensionEvent", "stopping a dimension change midway strands the player between two dimensions"),
    ("HopperTransferEvent", "the transfer already happened, since the hook point is after origin"),
    ("WeatherChangeEvent", "stopping it midway leaves the timer disagreeing with the actual weather; control weather through the Server weather interface or a gamerule"),
    ("PlayerChangeSlotEvent", "the return is a reference to the item in the new slot and cancelling would mean inventing one; pin the held item by setting the slot back inside the event"),
    ("PlayerUseItemCompleteEvent", "cancelling here leaves the player holding the item forever; block PlayerUseItemEvent to forbid it"),
];

/// Drops the `ll::event::xxx::` prefix and keeps the class name. The host accepts a unique
/// suffix for a subscription too, so the tables are looked up by suffix.
fn short(id: &str) -> &str {
    match id.rfind("::") {
        Some(i) => &id[i + 2..],
        None => id,
    }
}

/// Whether this event can be cancelled.
///
/// * `Some(true)`: it can, and `Event::cancel()` takes effect;
/// * `Some(false)`: it cannot, and `cancel()` returns `Err` saying which event to block;
/// * `None`: it is not in the tables. An event a third-party mod emits itself, or a new
///   upstream event these tables have not caught up with, both land here. `cancel()` writes
///   back as usual and blocks nothing, and it can confirm nothing on the caller's behalf.
pub fn is_cancellable(id: &str) -> Option<bool> {
    let s = short(id);
    if CANCELLABLE.contains(&s) {
        return Some(true);
    }
    if OBSERVE_ONLY.iter().any(|(n, _)| *n == s) {
        return Some(false);
    }
    None
}

/// Why an observation-only event cannot be cancelled, and which event to block instead.
pub fn why_not_cancellable(id: &str) -> Option<&'static str> {
    let s = short(id);
    OBSERVE_ONLY
        .iter()
        .find(|(n, _)| *n == s)
        .map(|(_, why)| *why)
}

/// The ids of every synthetic event. Comparing it against [`super::list()`] at startup
/// shows which capability packages this host was built with.
pub const ALL_SYNTHETIC: &[&str] = &[
    BLOCK_DESTROY,
    EXPLOSION,
    LIQUID_FLOW,
    FARMLAND_DECAY,
    PISTON_PUSH,
    CHEST_PAIR,
    SPAWN_ITEM_ACTOR,
    WEATHER_CHANGE,
    PLAYER_SLEEP,
    PLAYER_CHANGE_SLOT,
    PLAYER_USE_ITEM_COMPLETE,
    ARMOR_STAND_SWAP_ITEM,
    PLAYER_ATTACK_ITEM_FRAME,
    PLAYER_ATTACK_TARGET,
    PLAYER_CHANGE_GAME_MODE,
    PLAYER_DROP_ITEM,
    PLAYER_INTERACT_ENTITY,
    PLAYER_STEP_ON_PRESSURE_PLATE,
    ACTOR_STEP_ON_PRESSURE_PLATE,
    PLAYER_SPAWN_PROJECTILE,
    PLAYER_PUSH_ENTITY,
    PLAYER_RIDE,
    ACTOR_RIDE,
    PLAYER_TAKE_ENTITY,
    PLAYER_OPEN_CONTAINER,
    PLAYER_START_DESTROY_BLOCK,
    PLAYER_CHANGE_DIMENSION,
    HOPPER_TRANSFER,
    PLAYER_USE_ITEM_ON,
];
