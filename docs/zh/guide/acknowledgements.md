# 致谢

Pier 站在别的项目的工作上。它对引擎的了解，大多是因为有人先发现了，并且公开了做法。

这一页不是依赖列表，而是记下每个项目给了 Pier 什么，让这份感谢可以对着代码核对。每个项目保留自己的许可证，
条款请看各自的仓库。

## LeviLamina

[LeviLamina](https://github.com/LiteLDev/LeviLamina) 是整个平台。Pier 本身是一个 LeviLamina 模组：它的绕道
走的是 LeviLamina 的钩子框架，它的事件是 LeviLamina 的事件总线，它的命令、服务和玩家都是 LeviLamina 的，
它编译时用的引擎头文件是 LeviLamina 的 SDK。这份文档里的每一项能力，都是因为 LeviLamina 让它够得着才存在。

## LegacyScriptEngine 和 iListenAttentively

[LegacyScriptEngine](https://github.com/LiteLDev/LegacyScriptEngine)（LSE）和
[iListenAttentively](https://github.com/LiteLDev/iListenAttentively)（ILA，作者 MiracleForest）是 Pier 对照自己的事件时用的两份参照。
ILA 是 LSE 自己所依赖的事件库，所以这两个项目合起来，对大多数事件都回答了：该钩引擎里的哪个函数，订阅者拒绝时该返回什么。

Pier 的事件是它自己的，有自己的载荷和自己的取消规则。它从这两个项目取来的是对引擎的了解：引擎在哪里可以被拦住，
在那里拒绝时必须返回什么，游戏才不会出现不一致。

### 钩的是和参照同一个引擎函数的事件

| Pier 事件 | 引擎函数 | 参照 |
|---|---|---|
| `ArmorStandSwapItemEvent` | `ArmorStand::_trySwapItem` | ILA `ArmorStandSwapItemEvent` |
| `ChestPairEvent` | `ChestBlockActor::_tryToPairWith` | ILA `ChestPairWithEvent` |
| `ExplosionEvent` | `Explosion::explode` | ILA `ExplosionEvent` |
| `FarmlandDecayEvent` | `FarmBlock::transformOnFall` | ILA `FarmDecayEvent`，LSE `onFarmLandDecay` |
| `LiquidFlowEvent` | `LiquidBlock::_trySpreadTo` | ILA `LiquidFlowEvent`，LSE `onLiquidFlow` |
| `PlayerChangeGameModeEvent` | `ServerPlayer::setPlayerGameType` | ILA `PlayerChangeGameTypeEvent` |
| `PlayerDropItemEvent` | `Player::drop`、`ComplexInventoryTransaction::handle` | ILA `PlayerDropItemEvent`，LSE `onDropItem` |
| `PlayerInteractEntityEvent` | `Player::interact` | ILA `PlayerInteractEntityEvent`，LSE `onPlayerInteractEntity` |
| `PlayerOpenContainerEvent` | `VanillaServerGameplayEventListener::onEvent` | ILA `PlayerOpenContainerEvent`，LSE `onOpenContainer` |
| `PlayerAttackItemFrameEvent`、`PlayerOperatedItemFrameEvent` | `ItemFrameBlock::attack`、`ItemFrameBlock::use` | ILA `PlayerOperatedItemFrameEvent` |
| `PlayerSleepEvent` | `Player::startSleepInBed` | ILA `PlayerStartSleepEvent`，LSE `onBedEnter` |
| `PlayerUseItemCompleteEvent` | `Player::completeUsingItem` | ILA `PlayerAteEvent`，LSE `onAte` |
| `PlayerRideEvent`、`ActorRideEvent` | `Actor::canAddPassenger` | LSE `onRide` |
| `PortalCreateEvent` | `PortalBlock::trySpawnPortal` | LSE `onPortalTrySpawn` |
| `PlayerStepOnPressurePlateEvent`、`ActorStepOnPressurePlateEvent` | `BasePressurePlateBlock::shouldTriggerEntityInside` | ILA `ActorTriggerPressurePlateEvent`，LSE `onStepOnPressurePlate` |
| `PlayerSpawnProjectileEvent` | `Spawner::spawnProjectile` 和 `TridentItem::releaseUsing`，它五个挂点里的两个 | LSE `onSpawnProjectile` |
| `SpawnItemActorEvent` | `BedrockSpawner::spawnItem` | ILA `SpawnItemActorEvent` |
| `WeatherChangeEvent` | `Level::updateWeather` | ILA `WeatherUpdateEvent` |

### 读过参照之后新增的事件

| Pier 事件 | 引擎函数 | 参照 |
|---|---|---|
| `HopperSearchItemEvent`、`HopperPushOutEvent` | `Hopper::_tryPullInItemsFromAboveContainer`、`Hopper::_tryPullInItemsFromWorld`、`Hopper::_pushOutItems`、`Hopper::_tryMoveInItem` | LSE `onHopperSearchItem`、`onHopperPushOut` |
| `DispenseItemEvent` | `DispenserBlock::ejectItem` | LSE `onDispenseItem` |
| `CommandBlockExecuteEvent` | `BaseCommandBlock::_performCommand` | LSE `onCmdBlockExecute` |
| `RespawnAnchorExplodeEvent`、`PlayerUseRespawnAnchorEvent` | `RespawnAnchorBlock::use` | LSE `onRespawnAnchorExplode`、`onUseRespawnAnchor` |
| `PlayerUseBucketPlaceEvent`、`PlayerUseBucketTakeEvent` | `BucketItem::_emptyBucket`、`BucketItem::_useOn` | LSE `onUseBucketPlace`、`onUseBucketTake` |
| `PlayerConsumeTotemEvent` | `Player::consumeTotem` | LSE `onConsumeTotem` |
| `PlayerAttackBlockEvent` | `ServerPlayerBlockUseHandler::onStartDestroyBlock` | ILA `PlayerAttackBlockEvent`，LSE `onAttackBlock` |
| `WitherDestroyEvent` | `WitherBoss::_destroyBlocks` | ILA `WitherDestroyEvent` |
| `PlayerStopSleepEvent` | `Player::stopSleepInBed` | ILA `PlayerStopSleepEvent` |
| `MobGriefing` 维度规则里爆炸以外的部分 | `EventCoordinatorPimpl<ActorEventListener>::_processEvent`、`TakeBlockGoal::canUse`、`PlaceBlockGoal::canUse`、`WitherBoss::_destroyBlocks` | ILA `ActorDestroyBlockEvent`、`MobTakeBlockEvent`、`MobPlaceBlockEvent`、`WitherDestroyEvent` |
| `ExplosionBlocksEvent` | `BlockEventCoordinator::sendEvent`（可变的爆炸事件） | ILA `ExplosionBeforeEvent` |
| `BlockInteractedEvent` | `Block::use` | LSE `onBlockInteracted` |
| `NpcCommandEvent` | `NpcComponent::executeCommandAction` | LSE `onNpcCmd` |
| `PlayerPullFishingHookEvent` | `FishingHook::_pullCloser` | LSE `onPlayerPullFishingHook` |
| `PortalSpawnPigZombieEvent` | `SpawnChecks::canSpawnPigZombieFromPortal`，由 `PortalShape::evaluate` 记下门框 | LSE `onPortalTrySpawnPigZombie` |
| `ActorGetEffectEvent` | `Actor::addEffect` | ILA `ActorGetEffectEvent`，LSE `onEffectAdded`、`onEffectUpdated` |
| `ActorRemoveEffectEvent` | `Actor::onEffectRemoved` | LSE `onEffectRemoved` |
| `PlayerSetArmorEvent` | `Actor::setArmor` | LSE `onSetArmor` |
| `PlayerOpenInventoryEvent`、`PlayerOpenContainerScreenEvent` | `ServerPlayer::openInventory`、`ItemStackNetManagerServer::onContainerScreenOpen` | LSE `onOpenInventory`、`onOpenContainerScreen` |
| `PlayerRequestItemActionEvent` | `ItemStackRequestActionHandler::_handleTransfer`、`_handleRemove`、`ItemStackRequestActionCraftHandler::handleConsume` | ILA `PlayerRequestItemActionEvent` |
| `PlayerCloseContainerEvent` | `ServerPlayer::doDeleteContainerManager`、`PistonBlockActor::_spawnMovingBlock` | LSE `onCloseContainer` |
| `ContainerChangeEvent` | `LevelContainerModel::_onItemChanged` | LSE `onContainerChange` |
| `PlayerInventoryChangeEvent` | `Player::inventoryChanged` | ILA `PlayerChangeSlotEvent`，LSE `onInventoryChange` |
| `ScoreChangedEvent` | `ServerScoreboard::onScoreChanged` | LSE `onScoreChanged` |
| `ProjectileHitEntityEvent`、`ProjectileHitBlockEvent` | `ProjectileComponent::onHit` | LSE `onProjectileHitEntity`、`onProjectileHitBlock` |
| `ActorTransformEvent` | `ActorFactory::createTransformedActor` 加 `Level::addEntity` | LSE `onEntityTransformation` |
| `ActorDestroyBlockEvent`、`ActorPickupItemEvent` | `EventCoordinatorPimpl<ActorEventListener>::_processEvent`，两个事件列表各钩一次 | ILA `ActorDestroyBlockEvent`、`ActorPickupItemEvent` |
| `ActorChangeDimensionEvent` | `ActorDimensionTransferManager::canChangeDimension` | ILA `ActorChangeDimensionEvent` |
| `DragonEggTeleportEvent` | `DragonEggBlock::_attemptTeleport` | ILA `DragonEggBlockTeleportEvent` |
| `BlockFallEvent` | `FallingBlock::startFalling` | ILA `BlockFallEvent` |
| `SculkBlockGrowthEvent` | `SculkBlockBehavior::attemptUseCharge` | ILA `SculkBlockGrowthEvent` |
| `DragonRespawnEvent` | `EndDragonFight::_createNewDragon` | ILA `DragonRespawnEvent` |

### Pier 没有照搬的

iListenAttentively 有两个事件，是把引擎自己的逻辑在绕道里又写了一遍：末影人拿起和放下方块的事件，按目标选择的方式自己挑目标方块；
红石更新事件，自己跑了一遍电路系统的更新循环。抄下来的引擎逻辑，只在引擎不变的时候是对的，一变就错，而且不报任何错误。
Pier 改成在 `canUse` 上拒绝末影人的目标，这样不需要自己写任何引擎逻辑；红石更新事件则没有提供。

## MoreDimensions

[MoreDimensions](https://github.com/LiteLDev/MoreDimensions) 是自定义维度的来源。Pier 向引擎注册维度的方式沿用它：
在名字表里放一个 id，一份维度定义，再加引擎自己的工厂。Pier 也从 MoreDimensions 为了让引擎接受一个它从没听过的维度而不得不钩的那些地方学到了东西。

## LegacyMoney

[LegacyMoney](https://github.com/LiteLDev/LegacyMoney)（LLMoney）是经济函数背后的后端。它是延迟加载的，
所以没装它的服务器照样能启动，这些函数文档里写明的语义以它的源码为准。
