# levilamina::event::names · 事件名

事件 id 常量。

事件 id 是字符串，拼错的代价是订阅悄悄成功，回调却一次都不触发。宿主会报告解析失败，并列出相近的 id（§5.3），这些常量把这一步提前到了编译期。

分两类。注册表里的事件总是用完整的名字 `ll::event::<类名>`，中间没有分类段。宿主也接受唯一的后缀，但上游一旦加了同名的事件，后缀就会有歧义。合成事件的名字只有一个单词，是 Pier 用原生钩子构造的，用来补上 LL 没有覆盖的点。

每一项都写明了能不能取消。对不能取消的事件调用 `Event::cancel()`，不会出错，也不起作用，还会留下「已经拦住了」的印象。

## 函数 {#functions}

### `event::names::is_cancellable` {#fn.is_cancellable}

```rust
pub fn is_cancellable(id: &str) -> Option<bool>
```

这个事件能不能取消。

- `Some(true)`：能，`Event::cancel()` 会生效；
- `Some(false)`：不能，`cancel()` 会返回 `Err`，说明应该去拦哪个事件；
- `None`：不在表里。第三方模组自己发出的事件，或者这些表还没跟上的上游新事件，都会落在这里。`cancel()` 照常写回，什么都拦不住，也无法替调用方确认任何事情。

- 参数：
    - id : `&str`
- 返回值类型：`Option<bool>`

### `event::names::why_not_cancellable` {#fn.why_not_cancellable}

```rust
pub fn why_not_cancellable(id: &str) -> Option<&'static str>
```

只能观察的事件为什么不能取消，以及应该改去拦哪个事件。

- 参数：
    - id : `&str`
- 返回值类型：`Option<&'static str>`

## 常量 {#constants}

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PLAYER_JOIN"></span>`PLAYER_JOIN` | `"ll::event::PlayerJoinEvent"` | 可以取消，类型是 `Cancellable<ServerPlayerEvent>`。取消就是拒绝加入。 |
| <span id="PLAYER_CONNECT"></span>`PLAYER_CONNECT` | `"ll::event::PlayerConnectEvent"` | 可以取消。 |
| <span id="PLAYER_DISCONNECT"></span>`PLAYER_DISCONNECT` | `"ll::event::PlayerDisconnectEvent"` | 只能观察；玩家已经在离开，拦不住。 |
| <span id="PLAYER_DIE"></span>`PLAYER_DIE` | `"ll::event::PlayerDieEvent"` | 只能观察。 |
| <span id="PLAYER_RESPAWN"></span>`PLAYER_RESPAWN` | `"ll::event::PlayerRespawnEvent"` | 只能观察。 |
| <span id="PLAYER_CHAT"></span>`PLAYER_CHAT` | `"ll::event::PlayerChatEvent"` | 可以取消。载荷里有 `message`，修改它就改写了说的话。 |
| <span id="PLAYER_DESTROY_BLOCK"></span>`PLAYER_DESTROY_BLOCK` | `"ll::event::PlayerDestroyBlockEvent"` | 可以取消。玩家挖掉了一个方块。 |
| <span id="PLAYER_PLACING_BLOCK"></span>`PLAYER_PLACING_BLOCK` | `"ll::event::PlayerPlacingBlockEvent"` | 可以取消。`PlayerPlacingBlockEvent` 是即将放置，`Placed` 是已经放好。 |
| <span id="PLAYER_PLACED_BLOCK"></span>`PLAYER_PLACED_BLOCK` | `"ll::event::PlayerPlacedBlockEvent"` | 只能观察；已经放好了。要拦请用 [`PLAYER_PLACING_BLOCK`](event-names.md#PLAYER_PLACING_BLOCK)。 |
| <span id="PLAYER_INTERACT_BLOCK"></span>`PLAYER_INTERACT_BLOCK` | `"ll::event::PlayerInteractBlockEvent"` | 可以取消。右键点方块：开箱子、按按钮、使用工具。 |
| <span id="PLAYER_USE_ITEM"></span>`PLAYER_USE_ITEM` | `"ll::event::PlayerUseItemEvent"` | 可以取消。禁止某种食物或药水要在这里拦，不要在 [`PLAYER_USE_ITEM_COMPLETE`](event-names.md#PLAYER_USE_ITEM_COMPLETE) 拦。 |
| <span id="PLAYER_PICK_UP_ITEM"></span>`PLAYER_PICK_UP_ITEM` | `"ll::event::PlayerPickUpItemEvent"` | 可以取消。 |
| <span id="PLAYER_ATTACK"></span>`PLAYER_ATTACK` | `"ll::event::PlayerAttackEvent"` | 可以取消。注意它分不清攻击的是玩家还是生物；要区分得用 [`PLAYER_ATTACK_TARGET`](event-names.md#PLAYER_ATTACK_TARGET)，那是一个合成事件，载荷里带着 `targetIsPlayer`。 |
| <span id="PLAYER_SWING"></span>`PLAYER_SWING` | `"ll::event::PlayerSwingEvent"` | 只能观察。 |
| <span id="PLAYER_JUMP"></span>`PLAYER_JUMP` | `"ll::event::PlayerJumpEvent"` | 只能观察。 |
| <span id="PLAYER_SNEAKING"></span>`PLAYER_SNEAKING` | `"ll::event::PlayerSneakingEvent"` | 可以取消，因为基类 `PlayerSneakEvent` 是 `Cancellable<>`。 |
| <span id="PLAYER_SNEAKED"></span>`PLAYER_SNEAKED` | `"ll::event::PlayerSneakedEvent"` | 可以取消，同上。 |
| <span id="PLAYER_SPRINTING"></span>`PLAYER_SPRINTING` | `"ll::event::PlayerSprintingEvent"` | 只能观察：和潜行不同，`PlayerSprintEvent` 不是 Cancellable。 |
| <span id="PLAYER_SPRINTED"></span>`PLAYER_SPRINTED` | `"ll::event::PlayerSprintedEvent"` | 只能观察，同上。 |
| <span id="PLAYER_ADD_EXPERIENCE"></span>`PLAYER_ADD_EXPERIENCE` | `"ll::event::PlayerAddExperienceEvent"` | 可以取消。 |
| <span id="PLAYER_CHANGE_PERM"></span>`PLAYER_CHANGE_PERM` | `"ll::event::PlayerChangePermEvent"` | 可以取消。 |
| <span id="PLAYER_LEFT_CLICK"></span>`PLAYER_LEFT_CLICK` | `"ll::event::PlayerLeftClickEvent"` | 只能观察。它是 `PlayerAttackEvent` 和 `PlayerDestroyBlockEvent` 的基类；要拦具体的动作，请订阅那两个。 |
| <span id="PLAYER_RIGHT_CLICK"></span>`PLAYER_RIGHT_CLICK` | `"ll::event::PlayerRightClickEvent"` | 只能观察；同样是基类。 |
| <span id="ACTOR_HURT"></span>`ACTOR_HURT` | `"ll::event::ActorHurtEvent"` | 可以取消。 |
| <span id="MOB_DIE"></span>`MOB_DIE` | `"ll::event::MobDieEvent"` | 只能观察：`MobEvent` 不是 Cancellable。 |
| <span id="SPAWNING_MOB"></span>`SPAWNING_MOB` | `"ll::event::SpawningMobEvent"` | 可以取消。 |
| <span id="SPAWNED_MOB"></span>`SPAWNED_MOB` | `"ll::event::SpawnedMobEvent"` | 只能观察；已经发生了。 |
| <span id="BLOCK_CHANGED"></span>`BLOCK_CHANGED` | `"ll::event::BlockChangedEvent"` | 只能观察。方块的变化要通过 [`PLAYER_PLACING_BLOCK`](event-names.md#PLAYER_PLACING_BLOCK)、[`PLAYER_DESTROY_BLOCK`](event-names.md#PLAYER_DESTROY_BLOCK) 或 [`BLOCK_DESTROY`](event-names.md#BLOCK_DESTROY) 来拦，最后一个覆盖非玩家的来源。 |
| <span id="FIRE_SPREAD"></span>`FIRE_SPREAD` | `"ll::event::FireSpreadEvent"` | 可以取消。 |
| <span id="SERVER_STARTED"></span>`SERVER_STARTED` | `"ll::event::ServerStartedEvent"` | 只能观察。 |
| <span id="SERVER_STOPPING"></span>`SERVER_STOPPING` | `"ll::event::ServerStoppingEvent"` | 只能观察。 |
| <span id="SERVER_LEVEL_TICK"></span>`SERVER_LEVEL_TICK` | `"ll::event::ServerLevelTickEvent"` | 只能观察，每刻一次，所以判断必须很快，或者改用 `Host::schedule`。 |
| <span id="EXECUTING_COMMAND"></span>`EXECUTING_COMMAND` | `"ll::event::ExecutingCommandEvent"` | 可以取消。命令白名单或者审计就挂在这里。 |
| <span id="EXECUTED_COMMAND"></span>`EXECUTED_COMMAND` | `"ll::event::ExecutedCommandEvent"` | 只能观察；已经发生了。 |
| <span id="BLOCK_DESTROY"></span>`BLOCK_DESTROY` | `"BlockDestroyEvent"` | 可以取消。有东西移除了这一格，不问是谁。 它补上了最大的一块空缺：末影人搬走草方块、凋灵撞碎墙、苦力怕炸出的坑、蠹虫钻进石头、`/setblock ... destroy`、另一个插件调用 destroyBlock。以前这些都不触发任何事件，地皮保护只能看着方块消失。 载荷：`x` `y` `z` `dim` `dropResources` `block`。注意里面没有「是谁」：引擎在这一层已经丢掉了来源，编一个出来只会误导人。 |
| <span id="EXPLOSION"></span>`EXPLOSION` | `"ExplosionEvent"` | 可以取消。取消就是这次爆炸完全不发生，伤害和方块都不受影响。 载荷：`x` `y` `z` `dim` `radius` `maxResistance` `fire` `breaksBlocks` `underwater` `sourceIsPlayer` `sourceId` `source`。 |
| <span id="LIQUID_FLOW"></span>`LIQUID_FLOW` | `"LiquidFlowEvent"` | 可以取消。水或岩浆即将流进一格。用来拦这种情况：邻居在自己的地上倒水，水却流过了边界；倒水本身是合法的，流过去的那一步才越界。 载荷：目标格 `x` `y` `z` `dim`，来源格 `fromX` `fromY` `fromZ`，以及 `direction` 和 `liquid`。 这是热路径：液体每刻都在扩散，所以判断必须很快。 |
| <span id="FARMLAND_DECAY"></span>`FARMLAND_DECAY` | `"FarmlandDecayEvent"` | 可以取消。有东西从高处落下，把耕地踩回了泥土，不需要任何权限，也不留任何日志。 载荷：`x` `y` `z` `dim` `fallDistance` `byPlayer` `actor`，是玩家时还有 `_player`。 |
| <span id="PISTON_PUSH"></span>`PISTON_PUSH` | `"PistonPushEvent"` | 可以取消。活塞即将推动或拉回一组方块。用来拦跨地皮的活塞机器。 载荷：活塞的 `x` `y` `z` `dim`，`facing:[x,y,z]` 和 `attached:[[x,y,z],...]`。 |
| <span id="CHEST_PAIR"></span>`CHEST_PAIR` | `"ChestPairEvent"` | 可以取消。两个箱子即将合成一个大箱子。 贴着边界放的箱子会和邻居的箱子合并，打开靠自己这边的一半，就能看到对方那边的所有东西。容器保护按你点的那一格做判断，而那一格确实属于放箱子的人。 载荷：`x` `y` `z` `dim` `otherX` `otherY` `otherZ`。 |
| <span id="SPAWN_ITEM_ACTOR"></span>`SPAWN_ITEM_ACTOR` | `"SpawnItemActorEvent"` | 可以取消。取消就是不生成这个掉落物，物品直接消失，不会躺在地上。 用于防刷和掉落物归属。载荷：`x` `y` `z` `dim` `item` `count` `throwTime` `sourceIsPlayer` `source`。 |
| <span id="WEATHER_CHANGE"></span>`WEATHER_CHANGE` | `"WeatherChangeEvent"` | 只能观察。天气变化。载荷：`rainLevel` `rainTime` `lightningLevel` `lightningTime`。 |
| <span id="PLAYER_SLEEP"></span>`PLAYER_SLEEP` | `"PlayerSleepEvent"` | 可以取消，取消时用引擎自己的 `NotPossibleHere`，所以客户端显示的是原版的提示。 用于别人的床、不能跳过夜晚的玩法，以及床会爆炸的维度。 |
| <span id="PLAYER_CHANGE_SLOT"></span>`PLAYER_CHANGE_SLOT` | `"PlayerChangeSlotEvent"` | 只能观察：返回值是新槽位里那个物品的引用，取消就意味着凭空编出一个物品。 载荷：`from` `to` `item` `dim` `_player`。 |
| <span id="PLAYER_USE_ITEM_COMPLETE"></span>`PLAYER_USE_ITEM_COMPLETE` | `"PlayerUseItemCompleteEvent"` | 只能观察：在这里取消，玩家会一直拿着这个物品放不下来。吃完、喝完，或者放下望远镜时触发。 禁止某种食物，要在 [`PLAYER_USE_ITEM`](event-names.md#PLAYER_USE_ITEM) 拦住使用的开始。 |
| <span id="ARMOR_STAND_SWAP_ITEM"></span>`ARMOR_STAND_SWAP_ITEM` | `"ArmorStandSwapItemEvent"` | 可以取消。玩家和盔甲架交换装备；盔甲架既不是容器也不是方块，两种保护都看不到它。 |
| <span id="PLAYER_ATTACK_ITEM_FRAME"></span>`PLAYER_ATTACK_ITEM_FRAME` | `"PlayerAttackItemFrameEvent"` | 可以取消。左键点物品展示框取出物品：展示框还在，所以不算破坏方块；展示框是方块，所以也不算攻击实体。 |
| <span id="PLAYER_OPERATED_ITEM_FRAME"></span>`PLAYER_OPERATED_ITEM_FRAME` | `"PlayerOperatedItemFrameEvent"` | 可以取消。旋转展示框里的物品，是展示框这一对事件的另一半，攻击的那一半是 [`PLAYER_ATTACK_ITEM_FRAME`](event-names.md#PLAYER_ATTACK_ITEM_FRAME)。两者都不是放方块，也不是攻击实体，所以别的事件都看不到。 |
| <span id="PLAYER_EDIT_SIGN"></span>`PLAYER_EDIT_SIGN` | `"PlayerEditSignEvent"` | 可以取消。修改一块已经放好的告示牌上的文字。放告示牌属于放方块，早就覆盖了；改文字既不是放置也不是交互。 |
| <span id="PLAYER_REQUEST_ITEM_ACTION"></span>`PLAYER_REQUEST_ITEM_ACTION` | `"PlayerRequestItemActionEvent"` | 这个宿主不会触发它。保留这个名字，是为了已经订阅它的模组；[`is_cancellable`](event-names.md#fn.is_cancellable) 对它回答 `Some(false)`，免得有模组把它当成关卡。它没有列在 [`ALL_SYNTHETIC`](event-names.md#ALL_SYNTHETIC) 里，那里只列宿主注册的事件。 |
| <span id="PLAYER_ATTACK_TARGET"></span>`PLAYER_ATTACK_TARGET` | `"PlayerAttackTargetEvent"` | 可以取消。玩家攻击一个目标。载荷里有 `targetIsPlayer`，pvp 开关靠它区分攻击玩家和攻击生物；还有 `targetKnown`：目标读不出来时它为 0，同时 `targetIsPlayer` 为 1，于是 pvp 规则会拒绝。 |
| <span id="PLAYER_CHANGE_GAME_MODE"></span>`PLAYER_CHANGE_GAME_MODE` | `"PlayerChangeGameModeEvent"` | 可以取消。玩家切换游戏模式，包括通过 `/gamemode` 和其他插件的调用。 |
| <span id="PLAYER_DROP_ITEM"></span>`PLAYER_DROP_ITEM` | `"PlayerDropItemEvent"` | 可以取消。丢出物品，手动丢弃和从物品栏界面拖出去都算。 |
| <span id="PLAYER_INTERACT_ENTITY"></span>`PLAYER_INTERACT_ENTITY` | `"PlayerInteractEntityEvent"` | 可以取消。右键点实体：和村民交易、喂动物、剪羊毛。 |
| <span id="PLAYER_STEP_ON_PRESSURE_PLATE"></span>`PLAYER_STEP_ON_PRESSURE_PLATE` | `"PlayerStepOnPressurePlateEvent"` | 可以取消。玩家踩上压力板或绊线。内部按（玩家，位置）节流，每 250 毫秒最多触发一次。 |
| <span id="ACTOR_STEP_ON_PRESSURE_PLATE"></span>`ACTOR_STEP_ON_PRESSURE_PLATE` | `"ActorStepOnPressurePlateEvent"` | 可以取消。同上，但针对非玩家的实体，用的是另一张节流表。 |
| <span id="PLAYER_SPAWN_PROJECTILE"></span>`PLAYER_SPAWN_PROJECTILE` | `"PlayerSpawnProjectileEvent"` | 可以取消。玩家发射一个弹射物：雪球、末影珍珠、箭、三叉戟、弩射出的烟花。 |
| <span id="PLAYER_PUSH_ENTITY"></span>`PLAYER_PUSH_ENTITY` | `"PlayerPushEntityEvent"` | 可以取消。玩家推动一个实体。内部有节流。 |
| <span id="PLAYER_RIDE"></span>`PLAYER_RIDE` | `"PlayerRideEvent"` | 可以取消。玩家骑上一个载具。 |
| <span id="ACTOR_RIDE"></span>`ACTOR_RIDE` | `"ActorRideEvent"` | 可以取消。非玩家的实体骑上一个载具，比如村民坐船、猪坐矿车。 载荷用 `passenger` 和 `passengerId`，没有 `_player`。 |
| <span id="PLAYER_TAKE_ENTITY"></span>`PLAYER_TAKE_ENTITY` | `"PlayerTakeEntityEvent"` | 可以取消。玩家捡起一个弹射物实体，比如箭或三叉戟。 |
| <span id="PLAYER_OPEN_CONTAINER"></span>`PLAYER_OPEN_CONTAINER` | `"PlayerOpenContainerEvent"` | 可以取消。玩家打开一个容器。 |
| <span id="PLAYER_START_DESTROY_BLOCK"></span>`PLAYER_START_DESTROY_BLOCK` | `"PlayerStartDestroyBlockEvent"` | 只能观察，在原函数之前发出，用于记录是谁开始挖哪一格。 |
| <span id="PLAYER_CHANGE_DIMENSION"></span>`PLAYER_CHANGE_DIMENSION` | `"PlayerChangeDimensionEvent"` | 可以取消，目标也可以改写。玩家切换维度，任何原因都算：传送门、传送、`/execute in`、重生。 载荷：`from` `to` `to_x` `to_y` `to_z` `use_portal` `respawn` `_player`。`use_portal` 用来区分走进传送门和被传送，把两者一视同仁的规则，会让写它的人吃惊。 回答 `to`，或者同时回答 `to_x` `to_y` `to_z` 三个，就能给这次转移改道，之后由引擎自己完成。如果改为在回调里传送，玩家还站在传送门里时会再次进入这个事件，每刻都重复一遍。 |
| <span id="PORTAL_CREATE"></span>`PORTAL_CREATE` | `"PortalCreateEvent"` | 可以取消。一个框架即将变成下界传送门。 它在引擎量好框架、找到火之后触发，所以使用方不需要自己做任何几何计算。载荷：`dim` `x` `y` `z`。 里面没有玩家：火可以来自发射器、闪电和蔓延，按玩家判断的规则会漏掉这些情况。 |
| <span id="HOPPER_TRANSFER"></span>`HOPPER_TRANSFER` | `"HopperTransferEvent"` | 只能观察。漏斗转移了一个物品。载荷：`x` `y` `z` `slot` `item` `count` `old_item` `old_count`。 |
| <span id="PLAYER_USE_ITEM_ON"></span>`PLAYER_USE_ITEM_ON` | `"PlayerUseItemOnEvent"` | 可以取消。玩家对一个方块使用物品，放置它，或者拿着它右键。 |
| <span id="ALL_SYNTHETIC"></span>`ALL_SYNTHETIC` | `&[ BLOCK_DESTROY, EXPLOSION, LIQUID_FLOW, FARMLAND_DECAY, PISTON_PUSH, CHEST_PAIR, SPAWN_ITEM_ACTOR, WEATHER_CHANGE, PLAYER_SLEEP, PLAYER_CHANGE_SLOT, PLAYER_USE_ITEM_COMPLETE, ARMOR_STAND_SWAP_ITEM, PLAYER_ATTACK_ITEM_FRAME, PLAYER_OPERATED_ITEM_FRAME, PLAYER_EDIT_SIGN, PLAYER_ATTACK_TARGET, PLAYER_CHANGE_GAME_MODE, PLAYER_DROP_ITEM, PLAYER_INTERACT_ENTITY, PLAYER_STEP_ON_PRESSURE_PLATE, ACTOR_STEP_ON_PRESSURE_PLATE, PLAYER_SPAWN_PROJECTILE, PLAYER_PUSH_ENTITY, PLAYER_RIDE, ACTOR_RIDE, PLAYER_TAKE_ENTITY, PLAYER_OPEN_CONTAINER, PLAYER_START_DESTROY_BLOCK, PLAYER_CHANGE_DIMENSION, PORTAL_CREATE, HOPPER_TRANSFER, PLAYER_USE_ITEM_ON, ]` | 所有合成事件的 id。启动时拿它和 [`super::list()`](dimensions.md#fn.list) 对比，就能看出这个宿主编入了哪些能力包。 |
