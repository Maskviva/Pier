//! 方块属性访问器 —— 由 `PIER_BPROP_*` / `PIER_BSTR_*` 常量表生成。
//!
//! 「一个槽 + 一个常量」的那些在这里。有失败语义、要组合多次调用、或者
//! 写入方向的，留在 `block/mod.rs` 里手写。

use crate::block::Block;
use crate::rt::accessors::accessors;

accessors! { Block;
    str  name                     = PIER_BSTR_TYPE_NAME;
    str  snbt                     = PIER_BSTR_SNBT;
    /// 本地化键名，不是显示出来的文本。
    str  description_id           = PIER_BSTR_DESCRIPTION_ID;
    /// 已本地化的显示名。
    str  display_name             = PIER_BSTR_DISPLAY_NAME;
    /// 引擎自己的调试串，格式跟着版本走，别拿它做判断。
    str  debug_string             = PIER_BSTR_DEBUG_STRING;
    bool is_air                   = PIER_BPROP_IS_AIR;
    bool is_solid                 = PIER_BPROP_IS_SOLID;
    bool is_container             = PIER_BPROP_IS_CONTAINER;
    bool is_door                  = PIER_BPROP_IS_DOOR;
    bool is_fence                 = PIER_BPROP_IS_FENCE;
    bool is_rail                  = PIER_BPROP_IS_RAIL;
    bool is_slab                  = PIER_BPROP_IS_SLAB;
    bool is_stair                 = PIER_BPROP_IS_STAIR;
    bool is_wall                  = PIER_BPROP_IS_WALL;
    bool is_crop                  = PIER_BPROP_IS_CROP;
    bool is_unbreakable           = PIER_BPROP_IS_UNBREAKABLE;
    bool is_crafting_block        = PIER_BPROP_IS_CRAFTING_BLOCK;
    bool is_interactive_block     = PIER_BPROP_IS_INTERACTIVE_BLOCK;
    /// 能主动产生红石信号（拉杆、按钮…）。
    bool is_signal_source         = PIER_BPROP_IS_SIGNAL_SOURCE;
    /// 不用对的工具挖就不掉东西。
    bool requires_tool            = PIER_BPROP_REQUIRES_TOOL;
    bool has_block_entity         = PIER_BPROP_HAS_BLOCK_ENTITY;
    /// 旧的 data value。新方块用 [`Block::states`]。
    i32  data                     = PIER_BPROP_DATA;
    /// `variant` 数据值，含义随实体种类而不同。
    i32  variant                  = PIER_BPROP_VARIANT;
    /// 对应物品的数字 id。
    i32  block_item_id            = PIER_BPROP_BLOCK_ITEM_ID;
    /// 这一格实际的亮度，含天光。
    i32  light                    = PIER_BPROP_LIGHT;
    /// 这个方块自己发多少光。
    i32  light_emission           = PIER_BPROP_LIGHT_EMISSION;
    /// 挖掘硬度，越大越慢。
    f64  destroy_speed            = PIER_BPROP_DESTROY_SPEED;
    /// 抗爆值。
    f64  explosion_resistance     = PIER_BPROP_EXPLOSION_RESISTANCE;
    /// 摩擦系数，冰是低的那种。
    f64  friction                 = PIER_BPROP_FRICTION;
    /// 弹性，史莱姆块非零。
    f64  bounciness               = PIER_BPROP_BOUNCINESS;
    /// 被点燃的概率权重。
    i32  burn_odds                = PIER_BPROP_BURN_ODDS;
    /// 把火传给邻居的概率权重。
    i32  flame_odds               = PIER_BPROP_FLAME_ODDS;
    /// 这一格输出的红石信号强度，0..15。
    i32  redstone_signal          = PIER_BPROP_REDSTONE_SIGNAL;
    /// 比较器从这一格读到的强度，容器按填充度算。
    i32  comparator_signal        = PIER_BPROP_COMPARATOR_SIGNAL;
}
