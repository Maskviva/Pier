//! 事件 id 常量。
//!
//! 事件 id 是**字符串**，拼错的代价是「订阅静默成功、回调一次都不触发」。
//! 宿主会在解析不到时报错并列出相近 id（§5.3），常量把它提前到编译期。
//!
//! 两类：**注册表事件**（`ll::event::…`，LeviLamina 自己发的）一律给全名 ——
//! 宿主也接受唯一后缀，但后缀在上游新增同名事件时会变歧义；**合成事件**
//! （裸名字）是 Pier 用原生 detour 造的，补的是 LL 覆盖不到的点。
//!
//! 每条都注明能不能取消：不可取消的事件上调 `Event::cancel()` 是无害的
//! no-op，但你会以为拦住了。

/* ═══════════════ LeviLamina 注册表事件 ═══════════════ */

/// **可取消**（`Cancellable<ServerPlayerEvent>`）—— 取消 = 拒绝这次进服。
pub const PLAYER_JOIN: &str = "ll::event::player::PlayerJoinEvent";
/// **可取消。**
pub const PLAYER_CONNECT: &str = "ll::event::player::PlayerConnectEvent";
/// 只观察（玩家已经在走了，拦不住）。
pub const PLAYER_DISCONNECT: &str = "ll::event::player::PlayerDisconnectEvent";
/// 只观察。
pub const PLAYER_DIE: &str = "ll::event::player::PlayerDieEvent";
/// 只观察。
pub const PLAYER_RESPAWN: &str = "ll::event::player::PlayerRespawnEvent";

/// 可取消。载荷含 `message`，改它即可改写发言。
pub const PLAYER_CHAT: &str = "ll::event::player::PlayerChatEvent";

/// 可取消。玩家挖掉一个方块。
pub const PLAYER_DESTROY_BLOCK: &str = "ll::event::player::PlayerDestroyBlockEvent";
/// 可取消（`PlayerPlacingBlockEvent` 是「正要放」，`Placed` 是既成事实）。
pub const PLAYER_PLACING_BLOCK: &str = "ll::event::player::PlayerPlacingBlockEvent";
/// 只观察（既成事实）。要拦请用 [`PLAYER_PLACING_BLOCK`]。
pub const PLAYER_PLACED_BLOCK: &str = "ll::event::player::PlayerPlacedBlockEvent";
/// 可取消。右键方块（开箱子、按按钮、用工具）。
pub const PLAYER_INTERACT_BLOCK: &str = "ll::event::player::PlayerInteractBlockEvent";
/// **可取消。** 想禁某类食物/药水就拦这里（而不是 [`PLAYER_USE_ITEM_COMPLETE`]）。
pub const PLAYER_USE_ITEM: &str = "ll::event::player::PlayerUseItemEvent";
/// **可取消。**
pub const PLAYER_PICK_UP_ITEM: &str = "ll::event::player::PlayerPickUpItemEvent";
/// **可取消。** 注意它分不开「打玩家」和「打生物」——那个要用
/// [`PLAYER_ATTACK_TARGET`]（合成事件，载荷带 `targetIsPlayer`）。
pub const PLAYER_ATTACK: &str = "ll::event::player::PlayerAttackEvent";
/// 只观察。
pub const PLAYER_SWING: &str = "ll::event::player::PlayerSwingEvent";
/// 只观察。
pub const PLAYER_JUMP: &str = "ll::event::player::PlayerJumpEvent";
/// **可取消**（基类 `PlayerSneakEvent` 就是 `Cancellable<>`）。
pub const PLAYER_SNEAKING: &str = "ll::event::player::PlayerSneakingEvent";
/// **可取消**（同上）。
pub const PLAYER_SNEAKED: &str = "ll::event::player::PlayerSneakedEvent";
/// 只观察（`PlayerSprintEvent` 不是 Cancellable —— 和 Sneak 不一样，别想当然）。
pub const PLAYER_SPRINTING: &str = "ll::event::player::PlayerSprintingEvent";
/// 只观察（同上）。
pub const PLAYER_SPRINTED: &str = "ll::event::player::PlayerSprintedEvent";
/// **可取消。**
pub const PLAYER_ADD_EXPERIENCE: &str = "ll::event::player::PlayerAddExperienceEvent";
/// **可取消。**
pub const PLAYER_CHANGE_PERM: &str = "ll::event::player::PlayerChangePermEvent";
/// 只观察（它是 `PlayerAttackEvent`/`PlayerDestroyBlockEvent` 的**基类**，
/// 想拦具体动作请订那两个）。
pub const PLAYER_LEFT_CLICK: &str = "ll::event::player::PlayerLeftClickEvent";
/// 只观察（基类，同上）。
pub const PLAYER_RIGHT_CLICK: &str = "ll::event::player::PlayerRightClickEvent";

/// **可取消。**
pub const ACTOR_HURT: &str = "ll::event::entity::ActorHurtEvent";
/// 只观察（`MobEvent` 不是 Cancellable）。
pub const MOB_DIE: &str = "ll::event::entity::MobDieEvent";
/// **可取消。**
pub const SPAWNING_MOB: &str = "ll::event::entity::SpawningMobEvent";
/// 只观察（既成事实）。
pub const SPAWNED_MOB: &str = "ll::event::entity::SpawnedMobEvent";

/// 只观察。要拦方块变化用 [`PLAYER_PLACING_BLOCK`] / [`PLAYER_DESTROY_BLOCK`] /
/// [`BLOCK_DESTROY`]（后者覆盖非玩家来源）。
pub const BLOCK_CHANGED: &str = "ll::event::world::BlockChangedEvent";
/// **可取消。**
pub const FIRE_SPREAD: &str = "ll::event::world::FireSpreadEvent";

/// 只观察。
pub const SERVER_STARTED: &str = "ll::event::server::ServerStartedEvent";
/// 只观察。
pub const SERVER_STOPPING: &str = "ll::event::server::ServerStoppingEvent";
/// 只观察。**每 tick 一次** —— 判据要便宜，或者干脆用 `Host::schedule`。
pub const SERVER_LEVEL_TICK: &str = "ll::event::server::ServerLevelTickEvent";

/// **可取消。** 命令白名单/审计拦这里。
pub const EXECUTING_COMMAND: &str = "ll::event::command::ExecutingCommandEvent";
/// 只观察（既成事实）。
pub const EXECUTED_COMMAND: &str = "ll::event::command::ExecutedCommandEvent";

/* ═══════════════ Pier 合成事件（LeviLamina 没有） ═══════════════ */

/// **可取消。** 「有东西把这一格挖掉了」，不问是谁。
///
/// 补的是最大的一个洞：末影人搬走草方块、凋灵撞碎墙、爬行者炸出的坑、
/// 蠹虫钻进石头、`/setblock … destroy`、别的插件调 destroyBlock —— 在此之前
/// 这些**一个事件都不触发**，地皮保护只能看着方块凭空消失。
///
/// 载荷：`x` `y` `z` `dim` `dropResources` `block`。
/// 注意**没有**「谁干的」：引擎在这一层已经把来源丢了，编一个出来只会误导。
pub const BLOCK_DESTROY: &str = "BlockDestroyEvent";

/// **可取消**（取消 = 这次爆炸完全不发生，连伤害带方块）。
/// 载荷：`x` `y` `z` `dim` `radius` `maxResistance` `fire` `breaksBlocks`
/// `underwater` `sourceIsPlayer` `sourceId` `source`。
pub const EXPLOSION: &str = "ExplosionEvent";

/// **可取消。** 水/岩浆要流进某一格。拦「邻居在自己地里放水、流到我家」——
/// 放水那一下是合法的，流过来的这一步才越界。
/// 载荷：目标格 `x` `y` `z` `dim`，来源格 `fromX` `fromY` `fromZ`，`direction`、`liquid`。
///
/// 热路径：液体每 tick 都在流，判据要便宜。
pub const LIQUID_FLOW: &str = "LiquidFlowEvent";

/// **可取消。** 有东西从高处落下把耕地踩成泥土 —— 不需要任何权限，且无日志。
/// 载荷：`x` `y` `z` `dim` `fallDistance` `byPlayer` `actor`，玩家时另有 `_player`。
pub const FARMLAND_DECAY: &str = "FarmlandDecayEvent";

/// **可取消。** 活塞要推/拉一组方块。拦跨地皮的活塞机器。
/// 载荷：活塞 `x` `y` `z` `dim`、`facing:[x,y,z]`、`attached:[[x,y,z],…]`。
pub const PISTON_PUSH: &str = "PistonPushEvent";

/// **可取消。** 两个箱子要配成大箱子。
/// 贴着边界放箱子和邻居的箱子配对，打开自己这半边就能看见对面全部物品 ——
/// 容器保护判的是「你点的那一格」，而那一格确实是你的。
/// 载荷：`x` `y` `z` `dim` `otherX` `otherY` `otherZ`。
pub const CHEST_PAIR: &str = "ChestPairEvent";

/// **可取消**（取消 = 掉落物不生成，**物品消失**，不是留在原地）。
/// 反刷物、掉落归属。载荷：`x` `y` `z` `dim` `item` `count` `throwTime`
/// `sourceIsPlayer` `source`。
pub const SPAWN_ITEM_ACTOR: &str = "SpawnItemActorEvent";

/// 只观察。天气变化。载荷：`rainLevel` `rainTime` `lightningLevel` `lightningTime`。
pub const WEATHER_CHANGE: &str = "WeatherChangeEvent";

/// **可取消**（取消走引擎的 `NotPossibleHere`，客户端弹原版提示）。
/// 别人家的床、不该跳夜的玩法、床即炸弹的维度。
pub const PLAYER_SLEEP: &str = "PlayerSleepEvent";

/// 只观察（返回值是新槽位物品的引用，取消就得凭空造一个）。
/// 载荷：`from` `to` `item` `dim` `_player`。
pub const PLAYER_CHANGE_SLOT: &str = "PlayerChangeSlotEvent";

/// 只观察（这里取消会把玩家卡在「一直举着」）。吃完/喝完/望远镜放下。
/// 想禁食物在 [`PLAYER_USE_ITEM`] 拦「开始使用」。
pub const PLAYER_USE_ITEM_COMPLETE: &str = "PlayerUseItemCompleteEvent";

/// **可取消。** 玩家和盔甲架换装备 —— 盔甲架既不是容器也不是方块，
/// 两套保护都看不见它。
pub const ARMOR_STAND_SWAP_ITEM: &str = "ArmorStandSwapItemEvent";

/// **可取消。** 左键打物品展示框取物：不是破坏方块（框还在）、
/// 也不是打实体（框是方块）。
pub const PLAYER_ATTACK_ITEM_FRAME: &str = "PlayerAttackItemFrameEvent";

/// **可取消。** 玩家攻击目标。载荷含 `targetIsPlayer`，pvp 旗标靠它区分
/// 「打玩家」和「打生物」。
pub const PLAYER_ATTACK_TARGET: &str = "PlayerAttackTargetEvent";

/// **可取消。** 玩家切换游戏模式（含 `/gamemode` 与其它插件的调用）。
pub const PLAYER_CHANGE_GAME_MODE: &str = "PlayerChangeGameModeEvent";

/// **可取消。** 丢弃物品，覆盖手动丢和背包 UI 拖出两条路。
pub const PLAYER_DROP_ITEM: &str = "PlayerDropItemEvent";

/// **可取消。** 右键实体（村民交易、给动物喂食、剪羊毛…）。
pub const PLAYER_INTERACT_ENTITY: &str = "PlayerInteractEntityEvent";

/// **可取消。** 玩家踩上压力板/绊线。内置 250ms 按（玩家,位置）节流。
pub const PLAYER_STEP_ON_PRESSURE_PLATE: &str = "PlayerStepOnPressurePlateEvent";

/// **可取消。** 同上，但踩的是非玩家实体（用独立节流表）。
pub const ACTOR_STEP_ON_PRESSURE_PLATE: &str = "ActorStepOnPressurePlateEvent";

/// **可取消。** 玩家发射投射物（雪球、末影珍珠、弓箭、三叉戟、弩烟花）。
pub const PLAYER_SPAWN_PROJECTILE: &str = "PlayerSpawnProjectileEvent";

/// **可取消。** 玩家推挤实体。内置节流。
pub const PLAYER_PUSH_ENTITY: &str = "PlayerPushEntityEvent";

/// **可取消。** 玩家上载具。
pub const PLAYER_RIDE: &str = "PlayerRideEvent";

/// **可取消。** 非玩家实体上载具（船里的村民、矿车里的猪）。
/// 载荷用 `passenger`/`passengerId` 而不是 `_player`。
pub const ACTOR_RIDE: &str = "ActorRideEvent";

/// **可取消。** 玩家拾取箭矢/三叉戟一类投射物实体。
pub const PLAYER_TAKE_ENTITY: &str = "PlayerTakeEntityEvent";

/// **可取消。** 玩家打开容器。
pub const PLAYER_OPEN_CONTAINER: &str = "PlayerOpenContainerEvent";

/// 只观察（在 origin 之前发，用于记录「谁开始挖哪一格」）。
pub const PLAYER_START_DESTROY_BLOCK: &str = "PlayerStartDestroyBlockEvent";

/// 只观察。玩家换维度。载荷：`from` `to` `_player`。
pub const PLAYER_CHANGE_DIMENSION: &str = "PlayerChangeDimensionEvent";

/// 只观察。漏斗转移物品。载荷：`x` `y` `z` `slot` `item` `count`
/// `old_item` `old_count`。
pub const HOPPER_TRANSFER: &str = "HopperTransferEvent";

/// **可取消。** 玩家对着方块用物品（放置、右键使用）。
pub const PLAYER_USE_ITEM_ON: &str = "PlayerUseItemOnEvent";


/* ═══════════════ 可取消性查表 ═══════════════ */

/// 已知**可取消**的事件（LL 侧继承自 `Cancellable<>`；合成侧走
/// `dispatchHookEventCancellable`）。两边都是照着源码列的，不是照着直觉。
const CANCELLABLE: &[&str] = &[
    // —— LeviLamina 注册表 ——
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
    // —— Pier 合成 ——
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

/// 已知**只观察**的事件，附「想拦的话该去哪」。
///
/// 这张表存在的理由：在只观察的事件上调 `cancel()` 以前是**无害的 no-op** ——
/// 无害是指不会崩，但你会以为拦住了。保护类模组里这种「以为拦住了」比崩溃
/// 危险得多。
const OBSERVE_ONLY: &[(&str, &str)] = &[
    // —— LeviLamina 注册表 ——
    ("PlayerDisconnectEvent", "玩家已经在走了，拦不住"),
    ("PlayerDieEvent", "死亡已成事实；要防死请拦 ActorHurtEvent"),
    ("PlayerRespawnEvent", "重生已成事实"),
    ("PlayerPlacedBlockEvent", "已成事实；要拦请用 PlayerPlacingBlockEvent"),
    ("PlayerSwingEvent", "挥手是纯表现层"),
    ("PlayerJumpEvent", "跳跃不可取消"),
    ("PlayerSprintingEvent", "PlayerSprintEvent 不是 Cancellable（和 Sneak 不一样）"),
    ("PlayerSprintedEvent", "同 PlayerSprintingEvent"),
    ("PlayerLeftClickEvent", "它是基类；请订 PlayerAttackEvent 或 PlayerDestroyBlockEvent"),
    ("PlayerRightClickEvent", "它是基类；请订 PlayerInteractBlockEvent 或 PlayerUseItemEvent"),
    ("PlayerClickEvent", "它是基类"),
    ("MobDieEvent", "MobEvent 不是 Cancellable；要防死请拦 ActorHurtEvent"),
    ("SpawnedMobEvent", "已成事实；要拦请用 SpawningMobEvent"),
    ("BlockChangedEvent", "要拦方块变化请用 PlayerPlacingBlockEvent / PlayerDestroyBlockEvent / BlockDestroyEvent"),
    ("ServerLevelTickEvent", "tick 不可取消"),
    ("ServerStartedEvent", "不可取消"),
    ("ServerStoppingEvent", "不可取消"),
    ("ExecutedCommandEvent", "已成事实；要拦请用 ExecutingCommandEvent"),
    ("ConsoleOutputtedEvent", "已成事实；要拦请用 ConsoleOutputtingEvent"),
    // —— Pier 合成（宿主用 dispatchHookEvent，写回 sink 是 no-op）——
    ("PlayerStartDestroyBlockEvent", "它在 origin 之前发、只为记录「谁开始挖哪一格」；要拦请用 PlayerDestroyBlockEvent 或 BlockDestroyEvent"),
    ("PlayerChangeDimensionEvent", "换维度的中途拦下会让玩家卡在两个维度之间"),
    ("HopperTransferEvent", "转移已经发生（钩点在 origin 之后）"),
    ("WeatherChangeEvent", "中途拦下会让计时器和实际天气不一致；要控制天气用 Server 的天气接口或 gamerule"),
    ("PlayerChangeSlotEvent", "返回值是新槽位物品的引用，取消就得凭空造一个；要锁定手持请在事件里把槽位设回去"),
    ("PlayerUseItemCompleteEvent", "这里取消会把玩家卡在「一直举着」；要禁用请拦 PlayerUseItemEvent"),
];

/// 去掉 `ll::event::xxx::` 前缀，只留类名 —— 宿主也接受唯一后缀订阅，
/// 所以查表按后缀查。
fn short(id: &str) -> &str {
    match id.rfind("::") {
        Some(i) => &id[i + 2..],
        None => id,
    }
}

/// 这个事件能不能取消。
///
/// * `Some(true)` —— 能，`Event::cancel()` 会生效；
/// * `Some(false)` —— 不能，`cancel()` 会返回 `Err` 并说明该去拦哪个；
/// * `None` —— 表里没有。第三方模组自己发的事件、或本表还没跟上的上游新事件
///   都会落到这里。`cancel()` 会照常写回（**不拦着你**），但也无从替你确认。
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

/// 只观察的事件为什么不能取消、以及该去拦哪个。
pub fn why_not_cancellable(id: &str) -> Option<&'static str> {
    let s = short(id);
    OBSERVE_ONLY.iter().find(|(n, _)| *n == s).map(|(_, why)| *why)
}

/// 全部合成事件的 id —— 启动时拿它和 [`super::list()`] 对一遍，
/// 就知道这个宿主编入了哪些能力包。
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
