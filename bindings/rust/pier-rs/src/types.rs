//! The value types every domain shares.
//!
//! Only things without behavior belong here: coordinates, enums and bit flags. They touch
//! no `PierApi`, so domain modules can share them without depending on one another.
//!
//! # Enums carry `from_i32` and return an `Option`
//!
//! The host may be newer than the mod and report a value this side does not recognize
//! (contract §2.2). A `None` from `from_i32` means the host reported an unrecognized
//! value and stays apart from the value being 0 (§5.2). The other direction uses `as_i32`,
//! where no unknown value can arise.

/// A block coordinate.
pub type PositionI32 = (i32, i32, i32);
/// An actor or exact coordinate.
pub type PositionF64 = (f64, f64, f64);

/// A closed box, with both corners included.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Bounds {
    pub min: PositionI32,
    pub max: PositionI32,
}

impl Bounds {
    /// Builds a box from any two corners, taking the smaller and larger per axis, so a caller
    /// need not sort them first.
    pub fn new(a: PositionI32, b: PositionI32) -> Bounds {
        Bounds {
            min: (a.0.min(b.0), a.1.min(b.1), a.2.min(b.2)),
            max: (a.0.max(b.0), a.1.max(b.1), a.2.max(b.2)),
        }
    }

    /// The cell count per axis. The interval is closed, so it is max - min + 1.
    pub fn size(&self) -> (u64, u64, u64) {
        let d = |lo: i32, hi: i32| (hi as i64 - lo as i64 + 1).max(0) as u64;
        (
            d(self.min.0, self.max.0),
            d(self.min.1, self.max.1),
            d(self.min.2, self.max.2),
        )
    }

    /// The total cell count. A `u64` is used because a selection spanning a dimension easily
    /// overflows a `u32`.
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

/// The game mode. The values are part of the ABI, through `player_set_gamemode`.
///
/// Note that `Spectator` is 6 and not 3: the values in between mean other things in the
/// engine, and guessing one from the order sets the player to a different mode without an
/// error.
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

/// The player permission level, through `PIER_PACT_SET_PERMISSION_LEVEL` and
/// `PIER_PPROP_PERMISSION_LEVEL`.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PlayerPermission {
    Visitor = 0,
    Member = 1,
    Operator = 2,
    /// In the engine this means customized per item and is not a level above Operator.
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

/// The weather. The three states of `set_weather`.
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

/// The difficulty.
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

/// The `TextPacketType` of `player_send_message_typed`.
///
/// The host falls back to `Raw` on an out-of-range value, so no fallback branch is needed
/// here. The variants carrying an author or parameters, Chat, Whisper and Translate, take
/// a single body string on the ABI, and the author field reaches the client empty.
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

/// The `SetTitlePacketPayload::TitleType` of `player_send_title`.
///
/// The three TextObject variants at 6 through 8 need a `ResolvedTextObject`, which the
/// host refuses explicitly, so they are not offered here at all.
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

    /// The host does not read the `text` argument of Clear, Reset and Times.
    pub fn uses_text(self) -> bool {
        matches!(
            self,
            TitleKind::Title | TitleKind::Subtitle | TitleKind::Actionbar
        )
    }
}

/// The three title durations, in ticks.
///
/// All three are given together or not at all: the host refuses a half-specified
/// combination rather than guessing the rest for the caller, since half a set of durations
/// has no sensible default.
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
    /// Matching the vanilla `/title` defaults: half a second in, three seconds held, half a
    /// second out.
    fn default() -> TitleTimes {
        TitleTimes::new(10, 60, 10)
    }
}

/// Tells the engine which follow-up work a block write needs.
///
/// `NONE` is fastest and leaves the client unaware that the block changed, so a bulk fill
/// has to resynchronize afterwards, otherwise the player keeps seeing the old world until
/// that chunk is resent.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct BlockUpdate(pub i32);

impl BlockUpdate {
    /// Notify the neighbors, meaning redstone, liquid flow and block updates.
    pub const NEIGHBORS: BlockUpdate = BlockUpdate(1);
    /// Synchronize to the client.
    pub const NETWORK: BlockUpdate = BlockUpdate(2);
    /// Both, equivalent to `/setblock`.
    pub const DEFAULT: BlockUpdate = BlockUpdate(3);
    /// Neither. For a bulk fill, resynchronizing afterwards.
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

/// An equipment slot. The numbering `actor_get_equipped_item` and `player_get_equipment`
/// use.
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

/// A player ability bit. The index of `PIER_PACT_SET_ABILITY` and
/// `PIER_PACT_CAN_USE_ABILITY`.
///
/// The three carrying `Speed` are floating-point abilities and the rest are boolean.
/// Passing the wrong type raises no error and only has the value interpreted differently,
/// which is why [`Ability::is_float`] exists and
/// [`crate::player::Player::set_ability`] checks against it.
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

/// The value an ability bit can take. There is a boolean family and a floating-point
/// family, and [`Ability::is_float`] decides which applies.
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

/// The result of one ray trace, from `actor_trace_ray` or `edit_trace_ray`.
#[derive(Debug, Clone, PartialEq)]
pub enum RayHit {
    /// It hit an actor.
    Entity { id: i64, pos: PositionF64 },
    /// It hit a block. `facing` is the face hit and `block` is the block cell coordinate.
    Block {
        block: PositionI32,
        facing: i32,
        name: String,
        pos: PositionF64,
    },
    /// Nothing was within range. This is not an error and stays apart from the question being
    /// unanswerable (contract §5.2).
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

/// The local time, from `PIER_SYS_LOCAL_TIME`.
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
