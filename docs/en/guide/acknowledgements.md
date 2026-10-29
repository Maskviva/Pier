# Acknowledgements

Pier stands on other projects' work. Most of what it knows about the engine, it knows because
someone found it first and published how.

This page is not a dependency list. It records what each project gave Pier, so the credit can
be checked against the code. Each project keeps its own license; follow the links for the
terms.

## LeviLamina

[LeviLamina](https://github.com/LiteLDev/LeviLamina) is the platform. Pier is a LeviLamina
mod: its detours go through LeviLamina's hook framework, its events are LeviLamina's event bus,
its commands, services and players are LeviLamina's, and the engine headers it compiles against
are LeviLamina's SDK. Every capability in this documentation exists because LeviLamina made it
reachable.

## LegacyScriptEngine and iListenAttentively

[LegacyScriptEngine](https://github.com/LiteLDev/LegacyScriptEngine) (LSE) and
[iListenAttentively](https://github.com/LiteLDev/iListenAttentively) (ILA, by MiracleForest)
are the two references Pier checks its events against. ILA is the event library LSE itself
builds on, so between them they answer, for most events, which engine function to hook and what
to return when a subscriber refuses.

Pier's events are its own, with their own payloads and their own cancel rules. What it takes
from these two projects is knowledge of the engine: where the engine can be intercepted, and
what a refusal at that point must return so the game stays consistent.

### Events that hook the same engine function as a reference

| Pier event | Engine function | Reference |
|---|---|---|
| `ArmorStandSwapItemEvent` | `ArmorStand::_trySwapItem` | ILA `ArmorStandSwapItemEvent` |
| `ChestPairEvent` | `ChestBlockActor::_tryToPairWith` | ILA `ChestPairWithEvent` |
| `ExplosionEvent` | `Explosion::explode` | ILA `ExplosionEvent` |
| `FarmlandDecayEvent` | `FarmBlock::transformOnFall` | ILA `FarmDecayEvent`, LSE `onFarmLandDecay` |
| `LiquidFlowEvent` | `LiquidBlock::_trySpreadTo` | ILA `LiquidFlowEvent`, LSE `onLiquidFlow` |
| `PlayerChangeGameModeEvent` | `ServerPlayer::setPlayerGameType` | ILA `PlayerChangeGameTypeEvent` |
| `PlayerDropItemEvent` | `Player::drop`, `ComplexInventoryTransaction::handle` | ILA `PlayerDropItemEvent`, LSE `onDropItem` |
| `PlayerInteractEntityEvent` | `Player::interact` | ILA `PlayerInteractEntityEvent`, LSE `onPlayerInteractEntity` |
| `PlayerOpenContainerEvent` | `VanillaServerGameplayEventListener::onEvent` | ILA `PlayerOpenContainerEvent`, LSE `onOpenContainer` |
| `PlayerAttackItemFrameEvent`, `PlayerOperatedItemFrameEvent` | `ItemFrameBlock::attack`, `ItemFrameBlock::use` | ILA `PlayerOperatedItemFrameEvent` |
| `PlayerSleepEvent` | `Player::startSleepInBed` | ILA `PlayerStartSleepEvent`, LSE `onBedEnter` |
| `PlayerUseItemCompleteEvent` | `Player::completeUsingItem` | ILA `PlayerAteEvent`, LSE `onAte` |
| `PlayerRideEvent`, `ActorRideEvent` | `Actor::canAddPassenger` | LSE `onRide` |
| `PortalCreateEvent` | `PortalBlock::trySpawnPortal` | LSE `onPortalTrySpawn` |
| `PlayerStepOnPressurePlateEvent`, `ActorStepOnPressurePlateEvent` | `BasePressurePlateBlock::shouldTriggerEntityInside` | ILA `ActorTriggerPressurePlateEvent`, LSE `onStepOnPressurePlate` |
| `PlayerSpawnProjectileEvent` | `Spawner::spawnProjectile` and `TridentItem::releaseUsing`, two of its five hook points | LSE `onSpawnProjectile` |
| `SpawnItemActorEvent` | `BedrockSpawner::spawnItem` | ILA `SpawnItemActorEvent` |
| `WeatherChangeEvent` | `Level::updateWeather` | ILA `WeatherUpdateEvent` |

### Events added after reading a reference

| Pier event | Engine function | Reference |
|---|---|---|
| `HopperSearchItemEvent`, `HopperPushOutEvent` | `Hopper::_tryPullInItemsFromAboveContainer`, `Hopper::_tryPullInItemsFromWorld`, `Hopper::_pushOutItems`, `Hopper::_tryMoveInItem` | LSE `onHopperSearchItem`, `onHopperPushOut` |
| `DispenseItemEvent` | `DispenserBlock::ejectItem` | LSE `onDispenseItem` |
| `CommandBlockExecuteEvent` | `BaseCommandBlock::_performCommand` | LSE `onCmdBlockExecute` |
| `RespawnAnchorExplodeEvent`, `PlayerUseRespawnAnchorEvent` | `RespawnAnchorBlock::use` | LSE `onRespawnAnchorExplode`, `onUseRespawnAnchor` |
| `PlayerUseBucketPlaceEvent`, `PlayerUseBucketTakeEvent` | `BucketItem::_emptyBucket`, `BucketItem::_useOn` | LSE `onUseBucketPlace`, `onUseBucketTake` |
| `PlayerConsumeTotemEvent` | `Player::consumeTotem` | LSE `onConsumeTotem` |
| `PlayerAttackBlockEvent` | `ServerPlayerBlockUseHandler::onStartDestroyBlock` | ILA `PlayerAttackBlockEvent`, LSE `onAttackBlock` |
| `WitherDestroyEvent` | `WitherBoss::_destroyBlocks` | ILA `WitherDestroyEvent` |
| `PlayerStopSleepEvent` | `Player::stopSleepInBed` | ILA `PlayerStopSleepEvent` |
| the `MobGriefing` dimension rule, beyond explosions | `EventCoordinatorPimpl<ActorEventListener>::_processEvent`, `TakeBlockGoal::canUse`, `PlaceBlockGoal::canUse`, `WitherBoss::_destroyBlocks` | ILA `ActorDestroyBlockEvent`, `MobTakeBlockEvent`, `MobPlaceBlockEvent`, `WitherDestroyEvent` |
| `ExplosionBlocksEvent` | `BlockEventCoordinator::sendEvent` for the mutable explosion event | ILA `ExplosionBeforeEvent` |
| `BlockInteractedEvent` | `Block::use` | LSE `onBlockInteracted` |
| `NpcCommandEvent` | `NpcComponent::executeCommandAction` | LSE `onNpcCmd` |
| `PlayerPullFishingHookEvent` | `FishingHook::_pullCloser` | LSE `onPlayerPullFishingHook` |
| `PortalSpawnPigZombieEvent` | `SpawnChecks::canSpawnPigZombieFromPortal`, with `PortalShape::evaluate` noting the frame | LSE `onPortalTrySpawnPigZombie` |
| `ActorGetEffectEvent` | `Actor::addEffect` | ILA `ActorGetEffectEvent`, LSE `onEffectAdded`, `onEffectUpdated` |
| `ActorRemoveEffectEvent` | `Actor::onEffectRemoved` | LSE `onEffectRemoved` |
| `PlayerSetArmorEvent` | `Actor::setArmor` | LSE `onSetArmor` |
| `PlayerOpenInventoryEvent`, `PlayerOpenContainerScreenEvent` | `ServerPlayer::openInventory`, `ItemStackNetManagerServer::onContainerScreenOpen` | LSE `onOpenInventory`, `onOpenContainerScreen` |
| `PlayerRequestItemActionEvent` | `ItemStackRequestActionHandler::_handleTransfer`, `_handleRemove`, `ItemStackRequestActionCraftHandler::handleConsume` | ILA `PlayerRequestItemActionEvent` |
| `PlayerCloseContainerEvent` | `ServerPlayer::doDeleteContainerManager`, `PistonBlockActor::_spawnMovingBlock` | LSE `onCloseContainer` |
| `ContainerChangeEvent` | `LevelContainerModel::_onItemChanged` | LSE `onContainerChange` |
| `PlayerInventoryChangeEvent` | `Player::inventoryChanged` | ILA `PlayerChangeSlotEvent`, LSE `onInventoryChange` |
| `ScoreChangedEvent` | `ServerScoreboard::onScoreChanged` | LSE `onScoreChanged` |
| `ProjectileHitEntityEvent`, `ProjectileHitBlockEvent` | `ProjectileComponent::onHit` | LSE `onProjectileHitEntity`, `onProjectileHitBlock` |
| `ActorTransformEvent` | `ActorFactory::createTransformedActor` with `Level::addEntity` | LSE `onEntityTransformation` |
| `ActorDestroyBlockEvent`, `ActorPickupItemEvent` | `EventCoordinatorPimpl<ActorEventListener>::_processEvent`, once for each of the two event lists | ILA `ActorDestroyBlockEvent`, `ActorPickupItemEvent` |
| `ActorChangeDimensionEvent` | `ActorDimensionTransferManager::canChangeDimension` | ILA `ActorChangeDimensionEvent` |
| `DragonEggTeleportEvent` | `DragonEggBlock::_attemptTeleport` | ILA `DragonEggBlockTeleportEvent` |
| `BlockFallEvent` | `FallingBlock::startFalling` | ILA `BlockFallEvent` |
| `SculkBlockGrowthEvent` | `SculkBlockBehavior::attemptUseCharge` | ILA `SculkBlockGrowthEvent` |
| `DragonRespawnEvent` | `EndDragonFight::_createNewDragon` | ILA `DragonRespawnEvent` |

### What Pier did not take over

Two of iListenAttentively's events work by writing the engine's own logic out a second time
inside the detour: the enderman's pick-up and put-down events choose their target block the
way the goal does, and the redstone update event runs the circuit system's update loop itself.
A copy of engine logic stays right only until the engine changes, and then it is wrong without
any error. Pier refuses the enderman's goals at `canUse` instead, which needs no engine logic of
its own, and has no redstone update event.

## MoreDimensions

[MoreDimensions](https://github.com/LiteLDev/MoreDimensions) is where custom dimensions come
from. Pier registers a dimension with the engine the way MoreDimensions does: an id in the name
store, a definition, and the engine's own factory. It also learned from the places
MoreDimensions had to hook to make the engine accept a dimension it had never heard of.

## LegacyMoney

[LegacyMoney](https://github.com/LiteLDev/LegacyMoney) (LLMoney) is the economy backend behind
the money functions. It is delay-loaded, so a server without it still starts, and the semantics
documented for those functions follow its source.
