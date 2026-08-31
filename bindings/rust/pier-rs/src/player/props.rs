//! 玩家属性访问器 —— 由 `PIER_PPROP_*` / `PIER_PSTR_*` 常量表生成。
//!
//! 这些方法全都是「一个槽 + 一个常量」，所以它们是一张表而不是一面墙。
//! 需要更多说明的（写入、组合、有失败语义的）留在 `player/mod.rs` 里手写。

use crate::player::Player;
use crate::rt::accessors::accessors;

accessors! { Player;
    /// 账号名。
    str  real_name                  = PIER_PSTR_REAL_NAME;
    str  uuid                       = PIER_PSTR_UUID;
    str  xuid                       = PIER_PSTR_XUID;
    /// `地址:端口`。IPv6 也是这个形状，所以别按最后一个冒号切。
    str  ip_and_port                = PIER_PSTR_IP_AND_PORT;
    str  locale_code                = PIER_PSTR_LOCALE_CODE;
    /// 头顶显示的名字，可以被改。做身份判断用 [`Player::xuid`]。
    str  name_tag                   = PIER_PSTR_NAME_TAG;
    str  platform_online_id         = PIER_PSTR_PLATFORM_ONLINE_ID;
    i32  dimension                  = PIER_PPROP_DIMENSION;
    /// 经验等级。
    i32  level                      = PIER_PPROP_LEVEL;
    /// 经验条的进度，0..1。**不是**累计经验值。
    f64  experience                 = PIER_PPROP_EXPERIENCE;
    f64  hunger                     = PIER_PPROP_HUNGER;
    /// 饱和度，扣完才开始掉饥饿值。
    f64  saturation                 = PIER_PPROP_SATURATION;
    /// 疲劳度，攒满一格会扣一点饱和度。
    f64  exhaustion                 = PIER_PPROP_EXHAUSTION;
    /// 升到下一级还差多少经验。
    i32  xp_needed_for_next_level   = PIER_PPROP_XP_NEEDED_NEXT_LEVEL;
    f64  luck                       = PIER_PPROP_LUCK;
    i32  selected_slot              = PIER_PPROP_SELECTED_SLOT;
    /// 计分板上的 `score` 伪记分项，不是任何自定义记分项。
    i32  score                      = PIER_PPROP_SCORE;
    /// 客户端申请的视距，单位是区块。
    i32  chunk_radius               = PIER_PPROP_CHUNK_RADIUS;
    i32  enchantment_seed           = PIER_PPROP_ENCHANTMENT_SEED;
    /// `BuildPlatform` 的值，不是操作系统名。
    i32  platform                   = PIER_PPROP_PLATFORM;
    /// 朝向：0=南 1=西 2=北 3=东。
    i32  direction                  = PIER_PPROP_DIRECTION;
    /// 往返延迟，毫秒。明细见 [`Player::network_status`]。
    i32  ping                       = PIER_PPROP_NETWORK_RTT;
    i32  client_sub_id              = PIER_PPROP_CLIENT_SUB_ID;
    f64  fall_distance              = PIER_PPROP_FALL_DISTANCE;
    /// 在 `permissions.json` 里被列为管理员。和 [`Player::permission_level`] 不是一回事：后者可以在运行期改。
    bool is_operator                = PIER_PPROP_IS_OPERATOR;
    bool can_use_operator_blocks    = PIER_PPROP_CAN_USE_OPERATOR_BLOCKS;
    bool is_flying                  = PIER_PPROP_IS_FLYING;
    bool can_jump                   = PIER_PPROP_CAN_JUMP;
    bool is_emoting                 = PIER_PPROP_IS_EMOTING;
    bool is_in_raid                 = PIER_PPROP_IS_IN_RAID;
    /// 正处在受伤的无敌帧里。
    bool is_hurt                    = PIER_PPROP_IS_HURT;
    bool is_scoping                 = PIER_PPROP_IS_SCOPING;
    bool can_sleep                  = PIER_PPROP_CAN_SLEEP;
    bool has_respawn_position       = PIER_PPROP_HAS_RESPAWN_POSITION;
    bool is_using_item              = PIER_PPROP_IS_USING_ITEM;
    bool is_blocking                = PIER_PPROP_IS_BLOCKING;
    bool is_gliding                 = PIER_PPROP_IS_GLIDING;
    bool is_swimming                = PIER_PPROP_IS_SWIMMING;
    bool is_dead                    = PIER_PPROP_IS_DEAD;
    /// 这个存档里死过至少一次。
    bool has_died_before            = PIER_PPROP_HAS_DIED_BEFORE;
}
