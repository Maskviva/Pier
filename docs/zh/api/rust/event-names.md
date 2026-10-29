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
| <span id="EXPLOSION"></span>`EXPLOSION` | `"ExplosionEvent"` | 可以取消。取消就是这次爆炸完全不发生，伤害和方块都不受影响。 载荷：`x` `y` `z` `dim` `radius` `maxResistance` `fire` `breaksBlocks` `underwater` `sourceIsPlayer` `sourceId` `source`，有字段读不出来时还带 `partial`。 |
| <span id="LIQUID_FLOW"></span>`LIQUID_FLOW` | `"LiquidFlowEvent"` | 可以取消。水或岩浆即将流进一格。用来拦这种情况：邻居在自己的地上倒水，水却流过了边界；倒水本身是合法的，流过去的那一步才越界。 载荷：目标格 `x` `y` `z` `dim`，来源格 `fromX` `fromY` `fromZ`，以及 `direction` 和 `liquid`。 这是热路径：液体每刻都在扩散，所以判断必须很快。 |
| <span id="FARMLAND_DECAY"></span>`FARMLAND_DECAY` | `"FarmlandDecayEvent"` | 可以取消。有东西从高处落下，把耕地踩回了泥土，不需要任何权限，也不留任何日志。 载荷：`x` `y` `z` `dim` `fallDistance` `byPlayer` `actor`，是玩家时还有 `_player`，有字段读不出来时还带 `partial`。 |
| <span id="PISTON_PUSH"></span>`PISTON_PUSH` | `"PistonPushEvent"` | 可以取消。活塞即将推动或拉回一组方块。用来拦跨地皮的活塞机器。 载荷：活塞的 `x` `y` `z` `dim`，`facing:[x,y,z]` 和 `attached:[[x,y,z],...]`，有字段读不出来时还带 `partial`。 |
| <span id="CHEST_PAIR"></span>`CHEST_PAIR` | `"ChestPairEvent"` | 可以取消。两个箱子即将合成一个大箱子。 贴着边界放的箱子会和邻居的箱子合并，打开靠自己这边的一半，就能看到对方那边的所有东西。容器保护按你点的那一格做判断，而那一格确实属于放箱子的人。 载荷：`x` `y` `z` `dim` `otherX` `otherY` `otherZ`，有字段读不出来时还带 `partial`。 |
| <span id="SPAWN_ITEM_ACTOR"></span>`SPAWN_ITEM_ACTOR` | `"SpawnItemActorEvent"` | 可以取消。取消就是不生成这个掉落物，物品直接消失，不会躺在地上。 用于防刷和掉落物归属。载荷：`x` `y` `z` `dim` `item` `count` `throwTime` `sourceIsPlayer` `source`，有字段读不出来时还带 `partial`。 |
| <span id="WEATHER_CHANGE"></span>`WEATHER_CHANGE` | `"WeatherChangeEvent"` | 只能观察。天气变化。载荷：`rainLevel` `rainTime` `lightningLevel` `lightningTime`。 |
| <span id="PLAYER_SLEEP"></span>`PLAYER_SLEEP` | `"PlayerSleepEvent"` | 可以取消，取消时用引擎自己的 `NotPossibleHere`，所以客户端显示的是原版的提示。 用于别人的床、不能跳过夜晚的玩法，以及床会爆炸的维度。 |
| <span id="PLAYER_CHANGE_SLOT"></span>`PLAYER_CHANGE_SLOT` | `"PlayerChangeSlotEvent"` | 只能观察：返回值是新槽位里那个物品的引用，取消就意味着凭空编出一个物品。 载荷：`from` `to` `item` `dim` `_player`。 |
| <span id="PLAYER_USE_ITEM_COMPLETE"></span>`PLAYER_USE_ITEM_COMPLETE` | `"PlayerUseItemCompleteEvent"` | 只能观察：在这里取消，玩家会一直拿着这个物品放不下来。吃完、喝完，或者放下望远镜时触发。 禁止某种食物，要在 [`PLAYER_USE_ITEM`](event-names.md#PLAYER_USE_ITEM) 拦住使用的开始。 |
| <span id="ARMOR_STAND_SWAP_ITEM"></span>`ARMOR_STAND_SWAP_ITEM` | `"ArmorStandSwapItemEvent"` | 可以取消。玩家和盔甲架交换装备；盔甲架既不是容器也不是方块，两种保护都看不到它。 |
| <span id="PLAYER_ATTACK_ITEM_FRAME"></span>`PLAYER_ATTACK_ITEM_FRAME` | `"PlayerAttackItemFrameEvent"` | 可以取消。左键点物品展示框取出物品：展示框还在，所以不算破坏方块；展示框是方块，所以也不算攻击实体。 |
| <span id="PLAYER_OPERATED_ITEM_FRAME"></span>`PLAYER_OPERATED_ITEM_FRAME` | `"PlayerOperatedItemFrameEvent"` | 可以取消。旋转展示框里的物品，是展示框这一对事件的另一半，攻击的那一半是 [`PLAYER_ATTACK_ITEM_FRAME`](event-names.md#PLAYER_ATTACK_ITEM_FRAME)。两者都不是放方块，也不是攻击实体，所以别的事件都看不到。 |
| <span id="PLAYER_EDIT_SIGN"></span>`PLAYER_EDIT_SIGN` | `"PlayerEditSignEvent"` | 可以取消。修改一块已经放好的告示牌上的文字。放告示牌属于放方块，早就覆盖了；改文字既不是放置也不是交互。 |
| <span id="PLAYER_REQUEST_ITEM_ACTION"></span>`PLAYER_REQUEST_ITEM_ACTION` | `"PlayerRequestItemActionEvent"` | 可取消。玩家的客户端请求服务端在格子之间移动一叠物品、移除一叠物品，或者消耗合成的材料，这些都是逐个动作执行的物品栈请求。取消会返回引擎自己的错误结果，让客户端把界面恢复成服务端手里的样子。 载荷：`action` `amount` `srcContainer` `srcSlot` `dstContainer` `dstSlot` `x` `y` `z` `dim` `_player`。`action` 是引擎的动作类型编号，容器是引擎的容器枚举编号。移除或消耗没有值得读的目标，它的 `dst` 字段只是客户端发来的内容。 |
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
| <span id="HOPPER_SEARCH_ITEM"></span>`HOPPER_SEARCH_ITEM` | `"HopperSearchItemEvent"` | 可取消。漏斗或漏斗矿车即将拉进一个物品，来源是上方的容器，或者落在上方的掉落物。和 [`HOPPER_TRANSFER`](event-names.md#HOPPER_TRANSFER) 不同，它在物品移动之前触发，并带着维度；取消它，引擎就当作什么都没移动，漏斗稍后会再试。它能挡住一块地上的漏斗把下一块地上的箱子抽空。 载荷：`x` `y` `z` `dim` `minecart` `fromEntity` `item` `count` `slot` `face`，有字段读不出来时还带 `partial`。`x` `y` `z` 是漏斗所在的方块坐标，掉落物的 `fromEntity` 为 1。 |
| <span id="HOPPER_PUSH_OUT"></span>`HOPPER_PUSH_OUT` | `"HopperPushOutEvent"` | 可取消。漏斗或漏斗矿车即将把一个物品推进它朝向的容器。载荷和 [`HOPPER_SEARCH_ITEM`](event-names.md#HOPPER_SEARCH_ITEM) 相同，其中 `face` 是漏斗推向的那一侧。它能挡住一块地上的漏斗把东西塞进下一块地上的箱子。 |
| <span id="DISPENSE_ITEM"></span>`DISPENSE_ITEM` | `"DispenseItemEvent"` | 可取消。发射器即将把一个物品弹到世界里。这不是谁的操作，所以没有玩家事件看得到它，而朝着地界边缘的发射器会够到邻居那一侧。取消后物品留在发射器里。 载荷：`x` `y` `z` `dim` `item` `count` `slot` `face`，有字段读不出来时还带 `partial`。 |
| <span id="COMMAND_BLOCK_EXECUTE"></span>`COMMAND_BLOCK_EXECUTE` | `"CommandBlockExecuteEvent"` | 可取消。命令方块或命令方块矿车即将执行它的命令。来源不是玩家，所以 [`EXECUTING_COMMAND`](event-names.md#EXECUTING_COMMAND) 永远不会报告它。取消就是这条命令不执行。 载荷：`command` `x` `y` `z` `dim` `minecart`，有字段读不出来时还带 `partial`。 |
| <span id="WITHER_DESTROY"></span>`WITHER_DESTROY` | `"WitherDestroyEvent"` | 可取消。凋零即将破坏它身体周围的方块，它在生成时、受伤时和冲撞时都会这样做。这既不是爆炸也不是玩家。 载荷：`x` `y` `z` `dim` `range` `attackType` `witherId`。 |
| <span id="RESPAWN_ANCHOR_EXPLODE"></span>`RESPAWN_ANCHOR_EXPLODE` | `"RespawnAnchorExplodeEvent"` | 可取消。玩家在下界以外右键一个已充能的重生锚，它会爆炸。用荧石充能不会触发任何事件。取消后锚和玩家都保持原样。 载荷：`x` `y` `z` `dim` `charge` `_player`。 |
| <span id="PLAYER_USE_RESPAWN_ANCHOR"></span>`PLAYER_USE_RESPAWN_ANCHOR` | `"PlayerUseRespawnAnchorEvent"` | 可取消。玩家在下界右键一个已充能的重生锚，它会成为玩家的重生点。载荷和 [`RESPAWN_ANCHOR_EXPLODE`](event-names.md#RESPAWN_ANCHOR_EXPLODE) 相同。 |
| <span id="PLAYER_ATTACK_BLOCK"></span>`PLAYER_ATTACK_BLOCK` | `"PlayerAttackBlockEvent"` | 可取消。玩家开始击打一个方块。它钩的是“开始破坏”请求的处理函数，比 [`PLAYER_START_DESTROY_BLOCK`](event-names.md#PLAYER_START_DESTROY_BLOCK) 更早，所以取消它能在游戏模式得知之前就停住挖掘。 载荷：`x` `y` `z` `dim` `face` `_player`。 |
| <span id="PLAYER_USE_BUCKET_PLACE"></span>`PLAYER_USE_BUCKET_PLACE` | `"PlayerUseBucketPlaceEvent"` | 可取消。桶即将把它的液体放进一个格子。这既不是放置方块也不是破坏方块，所以那两个事件都看不到它。取消就是引擎对“桶没倒出来”的回答。 载荷：`x` `y` `z` `dim` `face` `item` `block` `byPlayer`，玩家操作时还带 `_player`，有字段读不出来时带 `partial`。`block` 是桶里的液体。 |
| <span id="PLAYER_USE_BUCKET_TAKE"></span>`PLAYER_USE_BUCKET_TAKE` | `"PlayerUseBucketTakeEvent"` | 可取消。空桶即将从一个格子里取走液体或细雪。载荷和 [`PLAYER_USE_BUCKET_PLACE`](event-names.md#PLAYER_USE_BUCKET_PLACE) 相同，其中 `block` 是瞄准的方块。 |
| <span id="PLAYER_CONSUME_TOTEM"></span>`PLAYER_CONSUME_TOTEM` | `"PlayerConsumeTotemEvent"` | 可取消。不死图腾即将救下一名玩家。取消后不消耗图腾，死亡照常发生，适合一击必死的玩法。 载荷：`x` `y` `z` `dim` `_player`。 |
| <span id="PLAYER_STOP_SLEEP"></span>`PLAYER_STOP_SLEEP` | `"PlayerStopSleepEvent"` | 只观察，在 origin 之前发出，所以回调看到的玩家还在床上。它是 [`PLAYER_SLEEP`](event-names.md#PLAYER_SLEEP) 的另一半。 载荷：`x` `y` `z` `dim` `forceful` `updateLevelList` `_player`。 |
| <span id="EXPLOSION_BLOCKS"></span>`EXPLOSION_BLOCKS` | `"ExplosionBlocksEvent"` | 可取消。爆炸已经算出它会波及哪些方块，正要把它们毁掉。这是 [`EXPLOSION`](event-names.md#EXPLOSION) 更细的那一半，后者只能拒绝整场爆炸：在这里，地块可以保住自己的方块，其余的照炸。 载荷：`x` `y` `z` `dim` `radius` `sourceIsPlayer` `sourceId` `source` `count` `truncated` 和 `blocks:[[x,y,z],...]`，来源读不出来时还带 `partial`。格子超过 16384 个时 `blocks` 为空、`truncated` 为 1，决定不了的订阅者应当取消。 回复 `keep:[[x,y,z],...]` 把这些格子从爆炸里拿掉，或者取消爆炸。所有订阅者的格子会合在一起保留，所以一个订阅者保护某个格子就够了。不是三个整数组成的列表的元素会被跳过并写进日志。 |
| <span id="BLOCK_INTERACTED"></span>`BLOCK_INTERACTED` | `"BlockInteractedEvent"` | 可取消。玩家右键一个会有反应的方块：门、按钮、拉杆、容器、工作台、床。它是在手持物品之下判定的，所以 [`PLAYER_USE_ITEM_ON`](event-names.md#PLAYER_USE_ITEM_ON) 看不到空手按拉杆。 载荷：`x` `y` `z` `dim` `face` `block` `_player`。 |
| <span id="NPC_COMMAND"></span>`NPC_COMMAND` | `"NpcCommandEvent"` | 可取消。玩家按下 NPC 对话里一个会执行命令的按钮，在命令执行之前询问。执行哪些命令是 NPC 作者的决定，不是玩家的。取消就是这个按钮什么也不做。 载荷：`x` `y` `z` `dim` `npc` `npcId` `actionIndex` `scene` `command` `_player`，命令读不出来时还带 `partial`。`command` 是这个动作的各条命令用分号连起来的文本。 |
| <span id="PLAYER_PULL_FISHING_HOOK"></span>`PLAYER_PULL_FISHING_HOOK` | `"PlayerPullFishingHookEvent"` | 可取消。鱼钩即将把钩到的东西往钓鱼的人拉过去：物品、生物、另一名玩家。取消后那个实体留在原地。 载荷：`x` `y` `z` `dim` `pulled` `pulledId` `pulledIsPlayer` `item` `_player`，被拉的实体读不出来时还带 `partial`。被拉的是地上的物品时 `item` 有值。没有玩家主人的鱼钩不上报。 |
| <span id="PORTAL_SPAWN_PIG_ZOMBIE"></span>`PORTAL_SPAWN_PIG_ZOMBIE` | `"PortalSpawnPigZombieEvent"` | 可取消。下界传送门即将生成一只僵尸猪灵。取消就是这一刻不生成。门框是引擎在这个线程上最后量过的那一个，它总是在询问之前刚量过。 载荷：`x` `y` `z` `dim` `axis`。 |
| <span id="ACTOR_GET_EFFECT"></span>`ACTOR_GET_EFFECT` | `"ActorGetEffectEvent"` | 可取消。一个实体即将获得一个效果，或者被续上一个。取消后它保持原样。 载荷：`effect` `effectId` `amplifier` `duration` `updated` `x` `y` `z` `dim` `actor` `actorId`，玩家还带 `_player`，有字段读不出来时带 `partial`。实体已经有这个效果时 `updated` 为 1。 |
| <span id="ACTOR_REMOVE_EFFECT"></span>`ACTOR_REMOVE_EFFECT` | `"ActorRemoveEffectEvent"` | 只观察，在 origin 之前发出。一个效果正从实体身上被移除。引擎是在效果已经没了之后才运行它的，在这里取消会留下一个已经不存在的效果的修饰值，所以想留住一个效果，就去拦住移除它的来源。载荷和 [`ACTOR_GET_EFFECT`](event-names.md#ACTOR_GET_EFFECT) 相同，只是没有 `updated`。 |
| <span id="PLAYER_SET_ARMOR"></span>`PLAYER_SET_ARMOR` | `"PlayerSetArmorEvent"` | 可取消。玩家的盔甲槽即将被设置，穿上、脱下和替换都算。取消后这个槽保持原样。 载荷：`slot` `item` `count` `x` `y` `z` `dim` `_player`，物品读不出来时还带 `partial`。槽被清空时 `item` 为空。 |
| <span id="PLAYER_OPEN_INVENTORY"></span>`PLAYER_OPEN_INVENTORY` | `"PlayerOpenInventoryEvent"` | 可取消。玩家打开自己的背包界面。[`PLAYER_OPEN_CONTAINER`](event-names.md#PLAYER_OPEN_CONTAINER) 管的是容器方块，看不到这个。 载荷：`x` `y` `z` `dim` `_player`。 |
| <span id="PLAYER_OPEN_CONTAINER_SCREEN"></span>`PLAYER_OPEN_CONTAINER_SCREEN` | `"PlayerOpenContainerScreenEvent"` | 可取消。一个容器界面即将为玩家打开，不管它属于谁：容器方块、运输矿车里的箱子、马。取消就是界面不打开。 载荷：`x` `y` `z` `dim` `owner` `_player`，`owner` 是 `block`、`actor`、`none` 之一，方块还带 `ownerX` `ownerY` `ownerZ`，实体还带 `ownerId`。 |
| <span id="PLAYER_CLOSE_CONTAINER"></span>`PLAYER_CLOSE_CONTAINER` | `"PlayerCloseContainerEvent"` | 只观察，在 origin 之前发出，所以回调看到的界面还开着。玩家不再看着一个容器方块：关掉界面、断开连接，或者活塞把方块推走了。只上报属于容器方块的界面。 载荷：`x` `y` `z` `dim` `block` `_player`。 |
| <span id="CONTAINER_CHANGE"></span>`CONTAINER_CHANGE` | `"ContainerChangeEvent"` | 只观察。玩家打开着的容器方块的一个格子变了。它在格子已经装上新物品之后才运行。 载荷：`x` `y` `z` `dim` `slot` `oldItem` `oldCount` `newItem` `newCount` `_player`，物品读不出来时还带 `partial`。`slot` 按玩家界面的数法计。 |
| <span id="PLAYER_INVENTORY_CHANGE"></span>`PLAYER_INVENTORY_CHANGE` | `"PlayerInventoryChangeEvent"` | 只观察。玩家某个容器里的一叠物品变了，不管原因：捡起、合成、命令、别的模组。它不是 [`PLAYER_CHANGE_SLOT`](event-names.md#PLAYER_CHANGE_SLOT)，那个是手持格子在动。它在物品变了之后才运行，所以要拦就拦原因。 载荷：`slot` `oldItem` `oldCount` `newItem` `newCount` `forceBalanced` `x` `y` `z` `dim` `_player`，物品读不出来时还带 `partial`。 |
| <span id="SCORE_CHANGED"></span>`SCORE_CHANGED` | `"ScoreChangedEvent"` | 只观察。一个分数有了新值，不管是谁改的：命令、命令方块、别的模组。 载荷：`objective` `displayName` `score` `kind` `name` `entityId`，在线的玩家还带 `_player`，有字段读不出来时带 `partial`。`kind` 是 `player`、`entity` 或 `fake`，`entityId` 是玩家或实体的 id，`name` 是虚拟玩家的名字。 |
| <span id="PROJECTILE_HIT_ENTITY"></span>`PROJECTILE_HIT_ENTITY` | `"ProjectileHitEntityEvent"` | 只观察，在 origin 之前发出。一个投射物击中了一个实体。 载荷：`x` `y` `z` `dim` `projectile` `projectileId` `target` `targetId` `targetIsPlayer`，位置是投射物的。要拦住一个投射物，就在 [`PLAYER_SPAWN_PROJECTILE`](event-names.md#PLAYER_SPAWN_PROJECTILE) 拒绝它的发射。 |
| <span id="PROJECTILE_HIT_BLOCK"></span>`PROJECTILE_HIT_BLOCK` | `"ProjectileHitBlockEvent"` | 只观察，在 origin 之前发出。一个投射物击中了一个方块。 载荷：`x` `y` `z` `dim` `block` `projectile` `projectileId`，位置是被击中的方块的。 |
| <span id="ACTOR_TRANSFORM"></span>`ACTOR_TRANSFORM` | `"ActorTransformEvent"` | 只观察。一个实体被另一个实体取代，新实体换了 id 站在原来的位置上，比如村民变成僵尸村民。 载荷：`x` `y` `z` `dim` `from` `fromId` `to` `toId`，新实体读不出来时还带 `partial`。位置是新实体的。 |
| <span id="ACTOR_DESTROY_BLOCK"></span>`ACTOR_DESTROY_BLOCK` | `"ActorDestroyBlockEvent"` | 可取消。一只生物即将拿走或破坏一个方块，比如末影人捡起一个。这不是谁的操作，所以没有玩家事件看得到它。取消就是引擎自己的 Cancel 回答，方块留在原处。 载荷：`x` `y` `z` `dim` `actor` `actorId` `block`，方块读不出来时还带 `partial`。位置是这个实体想改动的那个格子。 |
| <span id="ACTOR_PICKUP_ITEM"></span>`ACTOR_PICKUP_ITEM` | `"ActorPickupItemEvent"` | 可取消。一个实体即将把地上的一件物品捡起来，玩家也算。取消后物品留在原地。 载荷：`x` `y` `z` `dim` `actor` `actorId` `item` `itemId`，玩家捡的时候还带 `_player`，物品读不出来时带 `partial`。位置是物品的。 |
| <span id="ACTOR_CHANGE_DIMENSION"></span>`ACTOR_CHANGE_DIMENSION` | `"ActorChangeDimensionEvent"` | 可取消。一个不是玩家的实体即将去另一个维度：穿过传送门的掉落物、生物、矿车或船。玩家有 [`PLAYER_CHANGE_DIMENSION`](event-names.md#PLAYER_CHANGE_DIMENSION)。取消就是它留在原地。 载荷：`x` `y` `z` `dim` `from` `to` `actor` `actorId`，实体读不出来时还带 `partial`。`dim` 和 `from` 是它现在所在的维度，`to` 是它要去的那个。 |
| <span id="DRAGON_EGG_TELEPORT"></span>`DRAGON_EGG_TELEPORT` | `"DragonEggTeleportEvent"` | 可取消。一枚龙蛋即将跳到别处。取消后它留在原地。它要跳去的格子此时还不知道。 载荷：`x` `y` `z` `dim`。 |
| <span id="BLOCK_FALL"></span>`BLOCK_FALL` | `"BlockFallEvent"` | 可取消。一个失去支撑的方块即将变成下落的方块：沙子、沙砾、铁砧。取消后它悬在原处。 载荷：`x` `y` `z` `dim` `block` `creative`，方块读不出来时还带 `partial`。创造模式玩家的操作引起它时 `creative` 为 1。 |
| <span id="SCULK_BLOCK_GROWTH"></span>`SCULK_BLOCK_GROWTH` | `"SculkBlockGrowthEvent"` | 可取消。一份幽匿电荷即将被花掉，把一个格子变成幽匿方块。取消就是引擎对“电荷没有用掉”的回答，那个格子保持原样。属于世界生成的调用不会被询问。 载荷：`x` `y` `z` `dim`。 |
| <span id="DRAGON_RESPAWN"></span>`DRAGON_RESPAWN` | `"DragonRespawnEvent"` | 可取消。末地战斗即将在末影龙被杀死之后再造一条。全新末地的第一条龙不会被询问。取消后末地保持没有龙，直到下一次尝试。 载荷：`dim`，总是末地。 |
| <span id="ALL_SYNTHETIC"></span>`ALL_SYNTHETIC` | `&[ BLOCK_DESTROY, EXPLOSION, LIQUID_FLOW, FARMLAND_DECAY, PISTON_PUSH, CHEST_PAIR, SPAWN_ITEM_ACTOR, WEATHER_CHANGE, PLAYER_SLEEP, PLAYER_CHANGE_SLOT, PLAYER_USE_ITEM_COMPLETE, ARMOR_STAND_SWAP_ITEM, PLAYER_ATTACK_ITEM_FRAME, PLAYER_OPERATED_ITEM_FRAME, PLAYER_EDIT_SIGN, PLAYER_ATTACK_TARGET, PLAYER_CHANGE_GAME_MODE, PLAYER_DROP_ITEM, PLAYER_INTERACT_ENTITY, PLAYER_STEP_ON_PRESSURE_PLATE, ACTOR_STEP_ON_PRESSURE_PLATE, PLAYER_SPAWN_PROJECTILE, PLAYER_PUSH_ENTITY, PLAYER_RIDE, ACTOR_RIDE, PLAYER_TAKE_ENTITY, PLAYER_OPEN_CONTAINER, PLAYER_START_DESTROY_BLOCK, PLAYER_CHANGE_DIMENSION, PORTAL_CREATE, HOPPER_TRANSFER, PLAYER_USE_ITEM_ON, HOPPER_SEARCH_ITEM, HOPPER_PUSH_OUT, DISPENSE_ITEM, COMMAND_BLOCK_EXECUTE, WITHER_DESTROY, RESPAWN_ANCHOR_EXPLODE, PLAYER_USE_RESPAWN_ANCHOR, PLAYER_ATTACK_BLOCK, PLAYER_USE_BUCKET_PLACE, PLAYER_USE_BUCKET_TAKE, PLAYER_CONSUME_TOTEM, PLAYER_STOP_SLEEP, EXPLOSION_BLOCKS, BLOCK_INTERACTED, NPC_COMMAND, PLAYER_PULL_FISHING_HOOK, PORTAL_SPAWN_PIG_ZOMBIE, ACTOR_GET_EFFECT, ACTOR_REMOVE_EFFECT, PLAYER_SET_ARMOR, PLAYER_OPEN_INVENTORY, PLAYER_OPEN_CONTAINER_SCREEN, PLAYER_CLOSE_CONTAINER, CONTAINER_CHANGE, PLAYER_INVENTORY_CHANGE, SCORE_CHANGED, PROJECTILE_HIT_ENTITY, PROJECTILE_HIT_BLOCK, ACTOR_TRANSFORM, PLAYER_REQUEST_ITEM_ACTION, ACTOR_DESTROY_BLOCK, ACTOR_PICKUP_ITEM, ACTOR_CHANGE_DIMENSION, DRAGON_EGG_TELEPORT, BLOCK_FALL, SCULK_BLOCK_GROWTH, DRAGON_RESPAWN, ]` | 所有合成事件的 id。启动时拿它和 [`super::list()`](dimensions.md#fn.list) 对比，就能看出这个宿主编入了哪些能力包。 |
