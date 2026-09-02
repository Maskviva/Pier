//! 各个域共用的值类型。
//!
//! 这里只放**没有行为**的东西：坐标、枚举、位标志。它们不碰 `PierApi`，
//! 所以域模块之间可以共享它们而不互相依赖。
//!
//! # 枚举带 `from_i32`，且返回 `Option`
//!
//! 宿主可能比模组新，报回一个这一侧还不认识的值（契约 §2.2）。`from_i32`
//! 返回 `None` 表示「宿主报了我不认识的值」，和「值是 0」分得开（§5.2）。
//! 反过来往宿主传的时候用 `as_i32`，那一侧不会有未知值。

/// 方块坐标。
pub type PositionI32 = (i32, i32, i32);
/// 实体/精确坐标。
pub type PositionF64 = (f64, f64, f64);

/// 一个闭区间盒子，两角都含。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Bounds {
    pub min: PositionI32,
    pub max: PositionI32,
}

impl Bounds {
    /// 从任意两角建盒子，逐轴取小/大 —— 调用方不必先排序。
    pub fn new(a: PositionI32, b: PositionI32) -> Bounds {
        Bounds {
            min: (a.0.min(b.0), a.1.min(b.1), a.2.min(b.2)),
            max: (a.0.max(b.0), a.1.max(b.1), a.2.max(b.2)),
        }
    }

    /// 各轴的格数（闭区间，所以是 max - min + 1）。
    pub fn size(&self) -> (u64, u64, u64) {
        let d = |lo: i32, hi: i32| (hi as i64 - lo as i64 + 1).max(0) as u64;
        (
            d(self.min.0, self.max.0),
            d(self.min.1, self.max.1),
            d(self.min.2, self.max.2),
        )
    }

    /// 总格数。用 `u64` 是因为一个跨维度的选区能轻松溢出 `u32`。
    pub fn volume(&self) -> u64 {
        let (x, y, z) = self.size();
        x.saturating_mul(y).saturating_mul(z)
    }

    pub fn contains(&self, p: PositionI32) -> bool {
        p.0 >= self.min.0
            && p.0 <= self.max.0
            && p.1 >= self.min.1
            && p.1 <= self.max.1
            && p.2 >= self.min.2
            && p.2 <= self.max.2
    }
}

/// 游戏模式。数值是 ABI 的一部分（`player_set_gamemode`）。
///
/// 注意 `Spectator` 是 6 不是 3 —— 中间那几个值在引擎里另有含义，
/// 顺序猜一个会把玩家设成别的模式而不报错。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum GameMode {
    Survival = 0,
    Creative = 1,
    Adventure = 2,
    Spectator = 6,
}

impl GameMode {
    pub fn from_i32(v: i32) -> Option<GameMode> {
        Some(match v {
            0 => GameMode::Survival,
            1 => GameMode::Creative,
            2 => GameMode::Adventure,
            6 => GameMode::Spectator,
            _ => return None,
        })
    }

    pub fn as_i32(self) -> i32 {
        self as i32
    }
}

/// 玩家权限等级（`PIER_PACT_SET_PERMISSION_LEVEL` 与 `PIER_PPROP_PERMISSION_LEVEL`）。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PlayerPermission {
    Visitor = 0,
    Member = 1,
    Operator = 2,
    /// 引擎里表示「逐项自定义」，不是比 Operator 更高的一级。
    Custom = 3,
}

impl PlayerPermission {
    pub fn from_i32(v: i32) -> Option<PlayerPermission> {
        Some(match v {
            0 => PlayerPermission::Visitor,
            1 => PlayerPermission::Member,
            2 => PlayerPermission::Operator,
            3 => PlayerPermission::Custom,
            _ => return None,
        })
    }

    pub fn as_i32(self) -> i32 {
        self as i32
    }
}

/// 天气。`set_weather` 的三态。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Weather {
    Clear = 0,
    Rain = 1,
    Thunder = 2,
}

impl Weather {
    pub fn from_i32(v: i32) -> Option<Weather> {
        Some(match v {
            0 => Weather::Clear,
            1 => Weather::Rain,
            2 => Weather::Thunder,
            _ => return None,
        })
    }

    pub fn as_i32(self) -> i32 {
        self as i32
    }
}

/// 难度。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Difficulty {
    Peaceful = 0,
    Easy = 1,
    Normal = 2,
    Hard = 3,
}

impl Difficulty {
    pub fn from_i32(v: i32) -> Option<Difficulty> {
        Some(match v {
            0 => Difficulty::Peaceful,
            1 => Difficulty::Easy,
            2 => Difficulty::Normal,
            3 => Difficulty::Hard,
            _ => return None,
        })
    }

    pub fn as_i32(self) -> i32 {
        self as i32
    }
}

/// `player_send_message_typed` 的 `TextPacketType`。
///
/// 越界值宿主会退到 `Raw`，所以这里不需要兜底分支。带作者/参数的那几种
/// （Chat / Whisper / Translate）在 ABI 上只收单串正文，作者字段到客户端是空的。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MessageType {
    Raw = 0,
    Chat = 1,
    Translate = 2,
    Popup = 3,
    JukeboxPopup = 4,
    Tip = 5,
    SystemMessage = 6,
    Whisper = 7,
    Announcement = 8,
    TextObjectWhisper = 9,
    TextObject = 10,
    TextObjectAnnouncement = 11,
}

impl MessageType {
    pub fn as_i32(self) -> i32 {
        self as i32
    }
}

/// `player_send_title` 的 `SetTitlePacketPayload::TitleType`。
///
/// 6..8 那三种 TextObject 变体需要 `ResolvedTextObject`，宿主明确拒绝，
/// 所以这里根本不提供。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TitleKind {
    Clear = 0,
    Reset = 1,
    Title = 2,
    Subtitle = 3,
    Actionbar = 4,
    Times = 5,
}

impl TitleKind {
    pub fn as_i32(self) -> i32 {
        self as i32
    }

    /// Clear / Reset / Times 三种的 `text` 参数宿主不读。
    pub fn uses_text(self) -> bool {
        matches!(
            self,
            TitleKind::Title | TitleKind::Subtitle | TitleKind::Actionbar
        )
    }
}

/// 标题的三段时长，单位是 **tick**。
///
/// 三个值必须同时给或同时不给：宿主对「一半指定」的组合直接拒绝，
/// 而不是替调用方猜另一半（半套时长没有合理的默认）。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct TitleTimes {
    pub fade_in: i32,
    pub stay: i32,
    pub fade_out: i32,
}

impl TitleTimes {
    pub const fn new(fade_in: i32, stay: i32, fade_out: i32) -> TitleTimes {
        TitleTimes {
            fade_in,
            stay,
            fade_out,
        }
    }
}

impl Default for TitleTimes {
    /// 与原版 `/title` 的默认一致：0.5 秒淡入、3 秒停留、0.5 秒淡出。
    fn default() -> TitleTimes {
        TitleTimes::new(10, 60, 10)
    }
}

/// 写方块时告诉引擎要做哪些后续工作。
///
/// `NONE` 最快，但客户端不会知道方块变了 —— 批量填充结束后必须自己重同步，
/// 否则玩家看到的还是旧世界，直到那块区块被重发。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct BlockUpdate(pub i32);

impl BlockUpdate {
    /// 通知邻居（红石、水流、方块更新）。
    pub const NEIGHBORS: BlockUpdate = BlockUpdate(1);
    /// 同步给客户端。
    pub const NETWORK: BlockUpdate = BlockUpdate(2);
    /// 两者都做，等价于 `/setblock`。
    pub const DEFAULT: BlockUpdate = BlockUpdate(3);
    /// 都不做。批量填充用，之后自己重同步。
    pub const NONE: BlockUpdate = BlockUpdate(0);

    pub fn bits(self) -> i32 {
        self.0
    }
}

impl Default for BlockUpdate {
    fn default() -> BlockUpdate {
        BlockUpdate::DEFAULT
    }
}

/// 装备槽位。`actor_get_equipped_item` / `player_get_equipment` 用的编号。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum EquipSlot {
    MainHand = 0,
    OffHand = 1,
    Helmet = 2,
    Chestplate = 3,
    Leggings = 4,
    Boots = 5,
}

impl EquipSlot {
    pub fn from_i32(v: i32) -> Option<EquipSlot> {
        Some(match v {
            0 => EquipSlot::MainHand,
            1 => EquipSlot::OffHand,
            2 => EquipSlot::Helmet,
            3 => EquipSlot::Chestplate,
            4 => EquipSlot::Leggings,
            5 => EquipSlot::Boots,
            _ => return None,
        })
    }

    pub fn as_i32(self) -> i32 {
        self as i32
    }
}

/// 玩家能力位。`PIER_PACT_SET_ABILITY` / `PIER_PACT_CAN_USE_ABILITY` 的下标。
///
/// 三个带 `Speed` 的是**浮点**能力，其余是布尔。传错类型不会报错，只会
/// 让能力值被当成另一种解释 —— 所以 [`Ability::is_float`] 存在，
/// [`crate::player::Player::set_ability`] 会据它检查。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Ability {
    Build = 0,
    Mine = 1,
    DoorsAndSwitches = 2,
    OpenContainers = 3,
    AttackPlayers = 4,
    AttackMobs = 5,
    Operator = 6,
    Teleport = 7,
    Invulnerable = 8,
    Flying = 9,
    MayFly = 10,
    Instabuild = 11,
    Lightning = 12,
    FlySpeed = 13,
    WalkSpeed = 14,
    Muted = 15,
    WorldBuilder = 16,
    NoClip = 17,
    PrivilegedBuilder = 18,
    VerticalFlySpeed = 19,
}

impl Ability {
    pub fn is_float(self) -> bool {
        matches!(
            self,
            Ability::FlySpeed | Ability::WalkSpeed | Ability::VerticalFlySpeed
        )
    }

    pub fn as_i32(self) -> i32 {
        self as i32
    }
}

/// 能给一个能力位的值。布尔和浮点各一族，[`Ability::is_float`] 决定该用哪族。
pub trait AbilityValue: Copy {
    const IS_BOOL: bool;
    fn as_f64(self) -> f64;
}

impl AbilityValue for bool {
    const IS_BOOL: bool = true;
    fn as_f64(self) -> f64 {
        if self {
            1.0
        } else {
            0.0
        }
    }
}

macro_rules! impl_numeric_ability_value {
    ($($t:ty),* $(,)?) => {$(
        impl AbilityValue for $t {
            const IS_BOOL: bool = false;
            fn as_f64(self) -> f64 {
                self as f64
            }
        }
    )*};
}

impl_numeric_ability_value!(f32, f64, i8, i16, i32, i64, u8, u16, u32, u64);

/// 一次射线检测的结果（`actor_trace_ray` / `edit_trace_ray`）。
#[derive(Debug, Clone, PartialEq)]
pub enum RayHit {
    /// 打到实体。
    Entity { id: i64, pos: PositionF64 },
    /// 打到方块。`facing` 是命中面，`block` 是方块格坐标。
    Block {
        block: PositionI32,
        facing: i32,
        name: String,
        pos: PositionF64,
    },
    /// 射程内什么都没有。**不是错误** —— 和「问不出来」分开（契约 §5.2）。
    None,
}

impl RayHit {
    pub fn block_pos(&self) -> Option<PositionI32> {
        match self {
            RayHit::Block { block, .. } => Some(*block),
            _ => None,
        }
    }

    pub fn entity_id(&self) -> Option<i64> {
        match self {
            RayHit::Entity { id, .. } => Some(*id),
            _ => None,
        }
    }

    pub fn is_none(&self) -> bool {
        matches!(self, RayHit::None)
    }
}

/// 本地时间（`PIER_SYS_LOCAL_TIME`）。
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct LocalTime {
    pub year: i32,
    pub month: i32,
    pub day: i32,
    pub hour: i32,
    pub minute: i32,
    pub second: i32,
    pub ms: i32,
}
