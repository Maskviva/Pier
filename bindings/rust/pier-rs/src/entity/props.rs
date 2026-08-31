//! 实体属性访问器 —— 由 `PIER_APROP_*` / `PIER_ASTR_*` 常量表生成。
//!
//! 「一个槽 + 一个常量」的那些在这里。有失败语义、要组合多次调用、或者
//! 写入方向的，留在 `entity/mod.rs` 里手写。

use crate::entity::Entity;
use crate::rt::accessors::accessors;

accessors! { Entity;
    str  type_name                = PIER_ASTR_TYPE_NAME;
    /// 头顶显示的名字。
    str  name_tag                 = PIER_ASTR_NAME_TAG;
    /// 名字下面那一行。
    str  score_tag                = PIER_ASTR_SCORE_TAG;
    /// 过滤掉敏感词之后的名字。
    str  filtered_name            = PIER_ASTR_FILTERED_NAME;
    i32  dimension                = PIER_APROP_DIMENSION;
    f64  health                   = PIER_APROP_HEALTH;
    f64  max_health               = PIER_APROP_MAX_HEALTH;
    /// 当前移动速度，格/tick。
    f64  speed                    = PIER_APROP_SPEED;
    /// 当前这一次下落已经掉了多少格，落地时用来算摔伤。
    f64  fall_distance            = PIER_APROP_FALL_DISTANCE;
    /// 体型缩放，1.0 是原始大小。
    f64  scale                    = PIER_APROP_SCALE;
    /// `variant` 数据值，含义随实体种类而不同。
    i32  variant                  = PIER_APROP_VARIANT;
    /// `mark_variant` 数据值，和 [`Entity::variant`] 是两套编号。
    i32  mark_variant             = PIER_APROP_MARK_VARIANT;
    /// 死亡动画已经播了多少 tick。
    i32  death_time               = PIER_APROP_DEATH_TIME;
    bool is_alive                 = PIER_APROP_IS_ALIVE;
    bool is_on_ground             = PIER_APROP_IS_ON_GROUND;
    bool is_in_water              = PIER_APROP_IS_IN_WATER;
    bool is_in_lava               = PIER_APROP_IS_IN_LAVA;
    bool is_on_fire               = PIER_APROP_IS_ON_FIRE;
    bool is_invisible             = PIER_APROP_IS_INVISIBLE;
    bool is_sneaking              = PIER_APROP_IS_SNEAKING;
    bool is_baby                  = PIER_APROP_IS_BABY;
    bool is_riding                = PIER_APROP_IS_RIDING;
    bool is_tame                  = PIER_APROP_IS_TAME;
    /// 不会因为离玩家太远而被自然清除。
    bool is_persistent            = PIER_APROP_IS_PERSISTENT;
    bool is_leashed               = PIER_APROP_IS_LEASHED;
    bool is_invulnerable          = PIER_APROP_IS_INVULNERABLE;
    bool is_frozen                = PIER_APROP_IS_FROZEN;
    /// 处在繁殖状态。
    bool is_in_love               = PIER_APROP_IS_IN_LOVE;
    bool is_in_rain               = PIER_APROP_IS_IN_RAIN;
    bool is_in_snow               = PIER_APROP_IS_IN_SNOW;
    bool is_in_thunderstorm       = PIER_APROP_IS_IN_THUNDERSTORM;
    /// 手上或副手里有不死图腾。
    bool has_totem                = PIER_APROP_HAS_TOTEM;
    bool has_passenger            = PIER_APROP_HAS_PASSENGER;
}
