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
/// `underwater` `sourceIsPlayer` `sourceId` `source`, and `partial` when a field could
/// not be read.
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
/// Payload: `x` `y` `z` `dim` `fallDistance` `byPlayer` `actor`, plus `_player` for a player,
/// and `partial` when a field could not be read.
pub const FARMLAND_DECAY: &str = "FarmlandDecayEvent";

/// Cancellable. A piston is about to push or pull a set of blocks. It blocks a cross-plot
/// piston machine.
/// Payload: the piston `x` `y` `z` `dim`, `facing:[x,y,z]` and `attached:[[x,y,z],...]`,
/// and `partial` when a field could not be read.
pub const PISTON_PUSH: &str = "PistonPushEvent";

/// Cancellable. Two chests are about to pair into a double chest.
/// A chest placed against the boundary pairs with the neighbor's, and opening the near half
/// shows everything in theirs. Container protection decides on the cell you clicked, and
/// that cell really belongs to the placer.
/// Payload: `x` `y` `z` `dim` `otherX` `otherY` `otherZ`, and `partial` when a field
/// could not be read.
pub const CHEST_PAIR: &str = "ChestPairEvent";

/// Cancellable. Cancelling means the drop is not spawned and the item disappears rather
/// than lying on the ground.
/// For anti-duplication and drop ownership. Payload: `x` `y` `z` `dim` `item` `count`
/// `throwTime`
/// `sourceIsPlayer` `source`, and `partial` when a field could not be read.
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

/// Cancellable. Rotating the item in a frame, the other half of the frame pair: the
/// attack half is [`PLAYER_ATTACK_ITEM_FRAME`]. Neither is a block place nor a hit on
/// an actor, so nothing else sees it.
pub const PLAYER_OPERATED_ITEM_FRAME: &str = "PlayerOperatedItemFrameEvent";

/// Cancellable. Editing the text of a sign already placed. Placing a sign is a block
/// place and was already covered; changing its text was neither a place nor an interact.
pub const PLAYER_EDIT_SIGN: &str = "PlayerEditSignEvent";

/// Cancellable. A player's client asks the server to move a stack between slots, to remove
/// one, or to consume the ingredients of a craft, as item stack requests applied one action
/// at a time. Cancelling returns the engine's own error result, which makes the client put
/// its screen back the way the server has it.
/// Payload: `action` `amount` `srcContainer` `srcSlot` `dstContainer` `dstSlot` `x` `y` `z`
/// `dim` `_player`. `action` is the engine's action type number and the containers are its
/// container enum numbers. A removal or a consume has no destination worth reading, and its
/// `dst` fields are only what the client sent.
pub const PLAYER_REQUEST_ITEM_ACTION: &str = "PlayerRequestItemActionEvent";

/// Cancellable. A player attacks a target. The payload carries `targetIsPlayer`, which is
/// how the pvp flag tells attacking a player from attacking a mob, and `targetKnown`: when
/// the target could not be read it is 0 and `targetIsPlayer` is 1, so a pvp rule refuses.
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

/// Cancellable, and the target can be rewritten. A player changes dimension, from any
/// cause: a portal, a teleport, `/execute in`, a respawn.
///
/// Payload: `from` `to` `to_x` `to_y` `to_z` `use_portal` `respawn` `_player`.
/// `use_portal` is what separates walking into a portal from being teleported, and a rule
/// that treats the two alike will surprise whoever wrote it.
///
/// Answering with `to`, or with all three of `to_x` `to_y` `to_z`, reroutes the transfer;
/// the engine then performs it itself. Teleporting from the callback instead re-enters
/// this event while the player still stands in the portal, and repeats every tick.
pub const PLAYER_CHANGE_DIMENSION: &str = "PlayerChangeDimensionEvent";

/// Cancellable. A frame is about to become a nether portal.
///
/// It fires after the engine has measured the frame and found the fire, so a consumer
/// needs no geometry of its own. Payload: `dim` `x` `y` `z`.
///
/// There is no player: fire reaches a frame from a dispenser, from lightning and from
/// spreading, and a rule keyed on a player would miss those.
pub const PORTAL_CREATE: &str = "PortalCreateEvent";

/// Observation only. A hopper transfers an item. Payload: `x` `y` `z` `slot` `item` `count`
/// `old_item` `old_count`.
pub const HOPPER_TRANSFER: &str = "HopperTransferEvent";

/// Cancellable. A player uses an item on a block, placing it or right-clicking with it.
pub const PLAYER_USE_ITEM_ON: &str = "PlayerUseItemOnEvent";

/// Cancellable. A hopper, or a hopper minecart, is about to pull one item, from the container
/// above it or from a dropped item lying above it. Unlike [`HOPPER_TRANSFER`] it fires before
/// the move and carries the dimension, and cancelling it makes the engine answer that nothing
/// moved, so the hopper tries again later. It blocks a hopper on one plot emptying a chest on
/// the next.
/// Payload: `x` `y` `z` `dim` `minecart` `fromEntity` `item` `count` `slot` `face`, and
/// `partial` when a field could not be read. `x` `y` `z` are the block coordinates of the
/// hopper, and `fromEntity` is 1 for a dropped item.
pub const HOPPER_SEARCH_ITEM: &str = "HopperSearchItemEvent";

/// Cancellable. A hopper, or a hopper minecart, is about to push one item into the container
/// it faces. The payload is that of [`HOPPER_SEARCH_ITEM`], where `face` names the side the
/// hopper pushes toward. It blocks a hopper on one plot filling a chest on the next.
pub const HOPPER_PUSH_OUT: &str = "HopperPushOutEvent";

/// Cancellable. A dispenser is about to eject an item into the world. Nobody is acting, so
/// no player event sees it, and a dispenser facing a plot boundary reaches the neighbor's
/// side. Cancelling leaves the item in the dispenser.
/// Payload: `x` `y` `z` `dim` `item` `count` `slot` `face`, and `partial` when a field could
/// not be read.
pub const DISPENSE_ITEM: &str = "DispenseItemEvent";

/// Cancellable. A command block, or a command block minecart, is about to run its command.
/// The origin is not a player, so [`EXECUTING_COMMAND`] never reports it. Cancelling means
/// the command does not run.
/// Payload: `command` `x` `y` `z` `dim` `minecart`, and `partial` when a field could not be
/// read.
pub const COMMAND_BLOCK_EXECUTE: &str = "CommandBlockExecuteEvent";

/// Cancellable. A wither is about to break the blocks around its body, which it does when it
/// spawns, when it is hurt and while it charges. It is neither an explosion nor a player.
/// Payload: `x` `y` `z` `dim` `range` `attackType` `witherId`.
pub const WITHER_DESTROY: &str = "WitherDestroyEvent";

/// Cancellable. A player right-clicks a charged respawn anchor outside the nether, which
/// makes it explode. Charging it with glowstone raises nothing. Cancelling leaves the anchor
/// and the player as they were.
/// Payload: `x` `y` `z` `dim` `charge` `_player`.
pub const RESPAWN_ANCHOR_EXPLODE: &str = "RespawnAnchorExplodeEvent";

/// Cancellable. A player right-clicks a charged respawn anchor in the nether, which makes it
/// the player's spawn point. The payload is that of [`RESPAWN_ANCHOR_EXPLODE`].
pub const PLAYER_USE_RESPAWN_ANCHOR: &str = "PlayerUseRespawnAnchorEvent";

/// Cancellable. A player starts hitting a block. It hooks the handler of the start-destroy
/// request, which runs earlier than [`PLAYER_START_DESTROY_BLOCK`], so cancelling it stops the
/// mining before the game mode hears of it.
/// Payload: `x` `y` `z` `dim` `face` `_player`.
pub const PLAYER_ATTACK_BLOCK: &str = "PlayerAttackBlockEvent";

/// Cancellable. A bucket is about to put its liquid into a cell. It is neither a block place
/// nor a block break, so neither of those events sees it. Cancelling is the engine's answer
/// of a bucket that did not empty.
/// Payload: `x` `y` `z` `dim` `face` `item` `block` `byPlayer`, plus `_player` when a player
/// does it, and `partial` when a field could not be read. `block` is the liquid in the bucket.
pub const PLAYER_USE_BUCKET_PLACE: &str = "PlayerUseBucketPlaceEvent";

/// Cancellable. An empty bucket is about to take a liquid or powder snow out of a cell. The
/// payload is that of [`PLAYER_USE_BUCKET_PLACE`], where `block` is the block aimed at.
pub const PLAYER_USE_BUCKET_TAKE: &str = "PlayerUseBucketTakeEvent";

/// Cancellable. A totem of undying is about to save a player. Cancelling spends no totem and
/// the death proceeds, for a game where a hit is final.
/// Payload: `x` `y` `z` `dim` `_player`.
pub const PLAYER_CONSUME_TOTEM: &str = "PlayerConsumeTotemEvent";

/// Observation only, emitted before origin, so a callback still finds the player in bed. The
/// other half of [`PLAYER_SLEEP`].
/// Payload: `x` `y` `z` `dim` `forceful` `updateLevelList` `_player`.
pub const PLAYER_STOP_SLEEP: &str = "PlayerStopSleepEvent";

/// Cancellable. An explosion has worked out which blocks it reaches and is about to destroy
/// them. It is the finer half of [`EXPLOSION`], which can only refuse the whole blast: here a
/// plot keeps its own blocks and lets the rest go.
///
/// Payload: `x` `y` `z` `dim` `radius` `sourceIsPlayer` `sourceId` `source` `count`
/// `truncated` and `blocks:[[x,y,z],...]`, and `partial` when the source could not be read.
/// Above 16384 cells `blocks` is empty and `truncated` is 1, and a subscriber that cannot
/// decide should cancel.
///
/// Reply `keep:[[x,y,z],...]` to take those cells out of the blast, or cancel it. The cells
/// of every subscriber are kept together, so one subscriber protecting a cell is enough. A
/// cell that is not a list of three integers is skipped and logged.
pub const EXPLOSION_BLOCKS: &str = "ExplosionBlocksEvent";

/// Cancellable. A player right-clicks a block that does something: a door, a button, a
/// lever, a container, a crafting table, a bed. It is decided below the held item, which is
/// why [`PLAYER_USE_ITEM_ON`] does not see a bare hand on a lever.
/// Payload: `x` `y` `z` `dim` `face` `block` `_player`.
pub const BLOCK_INTERACTED: &str = "BlockInteractedEvent";

/// Cancellable. A player presses a button of an NPC dialogue whose action runs commands, and
/// this is asked before they run. Which commands is the NPC author's choice and not the
/// player's. Cancelling means the button does nothing.
/// Payload: `x` `y` `z` `dim` `npc` `npcId` `actionIndex` `scene` `command` `_player`, and
/// `partial` when the commands could not be read. `command` is the commands of the action
/// joined by semicolons.
pub const NPC_COMMAND: &str = "NpcCommandEvent";

/// Cancellable. A fishing hook is about to drag what it caught toward the angler: an item, a
/// mob, another player. Cancelling leaves the actor where it is.
/// Payload: `x` `y` `z` `dim` `pulled` `pulledId` `pulledIsPlayer` `item` `_player`, and
/// `partial` when the pulled actor could not be read. `item` is set when the pulled actor is
/// an item on the ground. A hook with no player owner is not reported.
pub const PLAYER_PULL_FISHING_HOOK: &str = "PlayerPullFishingHookEvent";

/// Cancellable. A nether portal is about to spawn a zombified piglin. Cancelling means it
/// does not this tick. The frame is the last one the engine measured on the thread, which it
/// does just before asking.
/// Payload: `x` `y` `z` `dim` `axis`.
pub const PORTAL_SPAWN_PIG_ZOMBIE: &str = "PortalSpawnPigZombieEvent";

/// Cancellable. An actor is about to be given an effect, or to have one renewed. Cancelling
/// leaves it as it was.
/// Payload: `effect` `effectId` `amplifier` `duration` `updated` `x` `y` `z` `dim` `actor`
/// `actorId`, plus `_player` for a player, and `partial` when a field could not be read.
/// `updated` is 1 when the actor already had that effect.
pub const ACTOR_GET_EFFECT: &str = "ActorGetEffectEvent";

/// Observation only, emitted before origin. An effect is being removed from an actor. The
/// engine runs it after the effect is gone, and cancelling there would leave the modifiers of
/// an effect that no longer exists, so to keep an effect, block what removes it. The payload
/// is that of [`ACTOR_GET_EFFECT`] without `updated`.
pub const ACTOR_REMOVE_EFFECT: &str = "ActorRemoveEffectEvent";

/// Cancellable. A player's armor slot is about to be set, which covers putting armor on,
/// taking it off and swapping it. Cancelling leaves the slot as it was.
/// Payload: `slot` `item` `count` `x` `y` `z` `dim` `_player`, and `partial` when the item
/// could not be read. `item` is empty when the slot is being emptied.
pub const PLAYER_SET_ARMOR: &str = "PlayerSetArmorEvent";

/// Cancellable. A player opens their own inventory screen. [`PLAYER_OPEN_CONTAINER`] covers
/// a container block and never sees this.
/// Payload: `x` `y` `z` `dim` `_player`.
pub const PLAYER_OPEN_INVENTORY: &str = "PlayerOpenInventoryEvent";

/// Cancellable. A container screen is about to open for a player, whoever owns it: a
/// container block, a minecart chest, a horse. Cancelling means the screen does not open.
/// Payload: `x` `y` `z` `dim` `owner` `_player`, with `owner` one of `block`, `actor` and
/// `none`, and `ownerX` `ownerY` `ownerZ` for a block or `ownerId` for an actor.
pub const PLAYER_OPEN_CONTAINER_SCREEN: &str = "PlayerOpenContainerScreenEvent";

/// Observation only, emitted before origin, so a callback still finds the screen open. A
/// player stops looking at a container block, by closing the screen, by disconnecting, or
/// because a piston moved the block. Only a screen owned by a container block is reported.
/// Payload: `x` `y` `z` `dim` `block` `_player`.
pub const PLAYER_CLOSE_CONTAINER: &str = "PlayerCloseContainerEvent";

/// Observation only. A slot of a container block a player has open changed. It runs after
/// the slot holds the new stack.
/// Payload: `x` `y` `z` `dim` `slot` `oldItem` `oldCount` `newItem` `newCount` `_player`, and
/// `partial` when an item could not be read. `slot` is counted the way the player's screen
/// counts it.
pub const CONTAINER_CHANGE: &str = "ContainerChangeEvent";

/// Observation only. A stack in one of a player's containers changed, whatever the cause: a
/// pick-up, a craft, a command, another mod. It is not [`PLAYER_CHANGE_SLOT`], which is the
/// held slot moving. It runs after the stack changed, so block the cause instead.
/// Payload: `slot` `oldItem` `oldCount` `newItem` `newCount` `forceBalanced` `x` `y` `z`
/// `dim` `_player`, and `partial` when an item could not be read.
pub const PLAYER_INVENTORY_CHANGE: &str = "PlayerInventoryChangeEvent";

/// Observation only. A score has a new value, whoever changed it: a command, a command
/// block, another mod.
/// Payload: `objective` `displayName` `score` `kind` `name` `entityId`, plus `_player` for a
/// player who is online, and `partial` when a field could not be read. `kind` is `player`,
/// `entity` or `fake`, `entityId` is the player's or the entity's id, and `name` is a fake
/// player's name.
pub const SCORE_CHANGED: &str = "ScoreChangedEvent";

/// Observation only, emitted before origin. A projectile hit an actor.
/// Payload: `x` `y` `z` `dim` `projectile` `projectileId` `target` `targetId`
/// `targetIsPlayer`, with the position of the projectile. To stop a projectile, refuse its
/// launch at [`PLAYER_SPAWN_PROJECTILE`].
pub const PROJECTILE_HIT_ENTITY: &str = "ProjectileHitEntityEvent";

/// Observation only, emitted before origin. A projectile hit a block.
/// Payload: `x` `y` `z` `dim` `block` `projectile` `projectileId`, with the block that was
/// hit.
pub const PROJECTILE_HIT_BLOCK: &str = "ProjectileHitBlockEvent";

/// Observation only. An actor was replaced by another that takes its place under a new id, a
/// villager turning into a zombie villager for one.
/// Payload: `x` `y` `z` `dim` `from` `fromId` `to` `toId`, and `partial` when the new actor
/// could not be read. The position is the new actor's.
pub const ACTOR_TRANSFORM: &str = "ActorTransformEvent";

/// Cancellable. A mob is about to take or break a block, an enderman picking one up for
/// instance. Nobody is acting, so no player event sees it. Cancelling is the engine's own
/// answer of Cancel, and the block stays.
/// Payload: `x` `y` `z` `dim` `actor` `actorId` `block`, and `partial` when the block could
/// not be read. The position is the cell the actor wants to change.
pub const ACTOR_DESTROY_BLOCK: &str = "ActorDestroyBlockEvent";

/// Cancellable. An actor is about to pick an item up off the ground, a player included.
/// Cancelling leaves the item where it is.
/// Payload: `x` `y` `z` `dim` `actor` `actorId` `item` `itemId`, plus `_player` when a player
/// picks it up, and `partial` when the item could not be read. The position is the item's.
pub const ACTOR_PICKUP_ITEM: &str = "ActorPickupItemEvent";

/// Cancellable. An actor that is not a player is about to go to another dimension: an item, a
/// mob, a minecart or a boat through a portal. Players have [`PLAYER_CHANGE_DIMENSION`].
/// Cancelling means it stays.
/// Payload: `x` `y` `z` `dim` `from` `to` `actor` `actorId`, and `partial` when the actor
/// could not be read. `dim` and `from` are the dimension it is in and `to` the one it is
/// headed for.
pub const ACTOR_CHANGE_DIMENSION: &str = "ActorChangeDimensionEvent";

/// Cancellable. A dragon egg is about to jump somewhere else. Cancelling leaves it where it
/// is. The cell it would jump to is not known yet.
/// Payload: `x` `y` `z` `dim`.
pub const DRAGON_EGG_TELEPORT: &str = "DragonEggTeleportEvent";

/// Cancellable. A block that lost its support is about to turn into a falling block: sand,
/// gravel, an anvil. Cancelling leaves it hanging.
/// Payload: `x` `y` `z` `dim` `block` `creative`, and `partial` when the block could not be
/// read. `creative` is 1 when a creative player's action started it.
pub const BLOCK_FALL: &str = "BlockFallEvent";

/// Cancellable. A sculk charge is about to be spent turning a cell into sculk. Cancelling is
/// the engine's answer of a charge that was not used, and the cell stays as it is. A call that
/// belongs to world generation is not asked about.
/// Payload: `x` `y` `z` `dim`.
pub const SCULK_BLOCK_GROWTH: &str = "SculkBlockGrowthEvent";

/// Cancellable. The end fight is about to make the ender dragon again, after it has been
/// killed. The first dragon of a fresh end is not asked about. Cancelling leaves the end
/// without a dragon until the next try.
/// Payload: `dim`, always the end.
pub const DRAGON_RESPAWN: &str = "DragonRespawnEvent";

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
    "PlayerChangeDimensionEvent",
    "PortalCreateEvent",
    "PlayerEditSignEvent",
    "PlayerOperatedItemFrameEvent",
    "HopperSearchItemEvent",
    "HopperPushOutEvent",
    "DispenseItemEvent",
    "CommandBlockExecuteEvent",
    "WitherDestroyEvent",
    "RespawnAnchorExplodeEvent",
    "PlayerUseRespawnAnchorEvent",
    "PlayerAttackBlockEvent",
    "PlayerUseBucketPlaceEvent",
    "PlayerUseBucketTakeEvent",
    "PlayerConsumeTotemEvent",
    "ExplosionBlocksEvent",
    "BlockInteractedEvent",
    "NpcCommandEvent",
    "PlayerPullFishingHookEvent",
    "PortalSpawnPigZombieEvent",
    "ActorGetEffectEvent",
    "PlayerSetArmorEvent",
    "PlayerOpenInventoryEvent",
    "PlayerOpenContainerScreenEvent",
    "PlayerRequestItemActionEvent",
    "ActorDestroyBlockEvent",
    "ActorPickupItemEvent",
    "ActorChangeDimensionEvent",
    "DragonEggTeleportEvent",
    "BlockFallEvent",
    "SculkBlockGrowthEvent",
    "DragonRespawnEvent",
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
    ("HopperTransferEvent", "the transfer already happened, since the hook point is after origin"),
    ("WeatherChangeEvent", "stopping it midway leaves the timer disagreeing with the actual weather; control weather through the Server weather interface or a gamerule"),
    ("PlayerChangeSlotEvent", "the return is a reference to the item in the new slot and cancelling would mean inventing one; pin the held item by setting the slot back inside the event"),
    ("PlayerUseItemCompleteEvent", "cancelling here leaves the player holding the item forever; block PlayerUseItemEvent to forbid it"),
    ("PlayerStopSleepEvent", "the engine has already decided to leave the bed, and refusing would leave the player lying in a bed it considers empty"),
    ("PlayerCloseContainerEvent", "the engine has already decided to close the screen; refuse the open at PlayerOpenContainerEvent or PlayerOpenContainerScreenEvent"),
    ("ContainerChangeEvent", "a notification that the slot already holds the new stack; refuse the move at PlayerRequestItemActionEvent"),
    ("PlayerInventoryChangeEvent", "a notification that the stack already changed; block the cause with PlayerPickUpItemEvent, PlayerDropItemEvent or PlayerRequestItemActionEvent"),
    ("ScoreChangedEvent", "the score already holds the new value"),
    ("ActorRemoveEffectEvent", "the engine runs it after the effect is gone, and cancelling would leave the modifiers of an effect that no longer exists"),
    ("ProjectileHitEntityEvent", "the projectile has already hit; refuse its launch with PlayerSpawnProjectileEvent"),
    ("ProjectileHitBlockEvent", "the projectile has already hit; refuse its launch with PlayerSpawnProjectileEvent"),
    ("ActorTransformEvent", "the new actor already exists"),
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
    PLAYER_OPERATED_ITEM_FRAME,
    PLAYER_EDIT_SIGN,
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
    PORTAL_CREATE,
    HOPPER_TRANSFER,
    PLAYER_USE_ITEM_ON,
    HOPPER_SEARCH_ITEM,
    HOPPER_PUSH_OUT,
    DISPENSE_ITEM,
    COMMAND_BLOCK_EXECUTE,
    WITHER_DESTROY,
    RESPAWN_ANCHOR_EXPLODE,
    PLAYER_USE_RESPAWN_ANCHOR,
    PLAYER_ATTACK_BLOCK,
    PLAYER_USE_BUCKET_PLACE,
    PLAYER_USE_BUCKET_TAKE,
    PLAYER_CONSUME_TOTEM,
    PLAYER_STOP_SLEEP,
    EXPLOSION_BLOCKS,
    BLOCK_INTERACTED,
    NPC_COMMAND,
    PLAYER_PULL_FISHING_HOOK,
    PORTAL_SPAWN_PIG_ZOMBIE,
    ACTOR_GET_EFFECT,
    ACTOR_REMOVE_EFFECT,
    PLAYER_SET_ARMOR,
    PLAYER_OPEN_INVENTORY,
    PLAYER_OPEN_CONTAINER_SCREEN,
    PLAYER_CLOSE_CONTAINER,
    CONTAINER_CHANGE,
    PLAYER_INVENTORY_CHANGE,
    SCORE_CHANGED,
    PROJECTILE_HIT_ENTITY,
    PROJECTILE_HIT_BLOCK,
    ACTOR_TRANSFORM,
    PLAYER_REQUEST_ITEM_ACTION,
    ACTOR_DESTROY_BLOCK,
    ACTOR_PICKUP_ITEM,
    ACTOR_CHANGE_DIMENSION,
    DRAGON_EGG_TELEPORT,
    BLOCK_FALL,
    SCULK_BLOCK_GROWTH,
    DRAGON_RESPAWN,
];

// Constants for another mod's events used to sit here. Shipping a name for an event this
// host does not raise reads as supported, and the subscription fails only on a server
// without that mod, where the failure looks like this host's. A mod that wants one can
// pass the string, which is all a constant ever did for it.
