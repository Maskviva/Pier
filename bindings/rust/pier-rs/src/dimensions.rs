//! 自定义维度 —— `pier-dimensions` 这个**可选**能力包的门面。
//!
//! 没编进宿主时全族槽位是 NULL，[`is_available`] 返回 false，其余调用返回
//! 「宿主不提供」的 `Err`。这是契约 §一 规则三的运行期表现：可选包不在，
//! 布局不变，槽位为空。
//!
//! # 注册是幂等的，所以启动时无条件注册
//!
//! [`add_simple`] 和 [`add_plot`] 对同一个名字在下次启动时返回**同一个**
//! 持久化 id。所以正确的用法是启动时直接注册，而不是先用
//! [`dimension_id`] 探一次 —— 后者在首次启动时必然落空。

use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{collect_strs, s};

/// 地形生成器。数值就是引擎的 `GeneratorType`。
///
/// 注意 1 起步而不是 0：从 0 开始编号会让「超平坦」生成出下界。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum GeneratorType {
    Overworld = 1,
    Flat = 2,
    Nether = 3,
    TheEnd = 4,
    Void = 5,
}

impl GeneratorType {
    pub fn from_i32(v: i32) -> Option<GeneratorType> {
        Some(match v {
            1 => GeneratorType::Overworld,
            2 => GeneratorType::Flat,
            3 => GeneratorType::Nether,
            4 => GeneratorType::TheEnd,
            5 => GeneratorType::Void,
            _ => return None,
        })
    }

    pub fn as_i32(self) -> i32 {
        self as i32
    }

    /// 引擎自己对这个生成器的叫法。
    ///
    /// 和枚举名不是同一个东西:枚举名是这一层的,这个是**引擎认的字符串**,
    /// 出现在生成参数和存档里。要拼给引擎看的东西时用它,不要用 `{:?}`。
    pub fn engine_name(self) -> &'static str {
        match self {
            GeneratorType::Overworld => "Overworld",
            GeneratorType::Flat => "Flat",
            GeneratorType::Nether => "Nether",
            GeneratorType::TheEnd => "TheEnd",
            GeneratorType::Void => "Void",
        }
    }
}

/// 逐维度的规则。数值对应 `PIER_DIMRULE_*`。
///
/// 为什么不用游戏规则：基岩版的游戏规则是**全服**的，把创造地皮世界的
/// `doMobSpawning` 关掉会连生存世界一起关。这些标志在真正的调用点上
/// （`Spawner::spawnMob`、`Level::explode`…）被检查，所以真的是逐维度的。
///
/// 没有登记过的维度**完全不受影响** —— 钩子直接落到原版实现，
/// 调用方不需要为原版维度显式放行。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DimensionRule {
    SpawnMonster = 0,
    SpawnAnimal = 1,
    SpawnSpawner = 2,
    ExplodeBlocks = 3,
    FireSpread = 4,
    MobGriefing = 5,
    Projectile = 6,
    PistonPush = 7,
    LiquidFlow = 8,
    FarmlandDecay = 9,
    Ride = 10,
    /// 只拦**跨地皮边界**的活塞推动，地皮内部照常。和
    /// [`DimensionRule::PistonPush`] 同时生效，任一否决就拦下。
    PistonCrossPlot = 11,
    /// 只拦跨地皮边界的实体移动。玩家和被骑的载具永不受限。
    EntityCrossPlot = 12,
}

impl DimensionRule {
    pub fn as_i32(self) -> i32 {
        self as i32
    }
}

/// 地皮世界的网格布局。
///
/// 网格约定（SDK 必须和宿主一致）：令 `cell = plot_size + road_width`，
/// 世界坐标 `(x, z)` 处，当 `mod(x,cell) >= plot_size || mod(z,cell) >= plot_size`
/// 时是路；否则离地皮边缘 `border_width` 以内是边框；再否则是地皮。
#[derive(Debug, Clone, PartialEq)]
pub struct PlotLayout {
    pub plot_size: i32,
    pub road_width: i32,
    pub border_width: i32,
    pub floor_y: i32,
    pub floor_block: String,
    pub fill_block: String,
    pub road_block: String,
    pub border_block: String,
    pub biome: String,
}

impl Default for PlotLayout {
    fn default() -> PlotLayout {
        PlotLayout {
            plot_size: 64,
            road_width: 7,
            border_width: 1,
            floor_y: 64,
            floor_block: "minecraft:grass_block".to_owned(),
            fill_block: "minecraft:dirt".to_owned(),
            road_block: "minecraft:birch_planks".to_owned(),
            border_block: "minecraft:stone_block_slab".to_owned(),
            biome: "minecraft:plains".to_owned(),
        }
    }
}

impl PlotLayout {
    /// 一个地皮加一条路的宽度，也就是网格的模。
    pub fn cell_size(&self) -> i32 {
        self.plot_size + self.road_width
    }

    pub fn to_snbt(&self) -> String {
        NbtValue::obj([
            ("plotSize", NbtValue::Int(self.plot_size)),
            ("roadWidth", NbtValue::Int(self.road_width)),
            ("borderWidth", NbtValue::Int(self.border_width)),
            ("floorY", NbtValue::Int(self.floor_y)),
            ("floorBlock", NbtValue::from(self.floor_block.as_str())),
            ("fillBlock", NbtValue::from(self.fill_block.as_str())),
            ("roadBlock", NbtValue::from(self.road_block.as_str())),
            ("borderBlock", NbtValue::from(self.border_block.as_str())),
            ("biome", NbtValue::from(self.biome.as_str())),
        ])
        .to_snbt()
    }
}

/// 一个已经登记过的自定义维度。
#[derive(Debug, Clone, PartialEq)]
pub struct ExistingDimension {
    pub name: String,
    pub dim: i32,
    /// 生成参数原文，由调用方自己解释。
    pub snbt: String,
}

/// 这个宿主编进自定义维度能力了吗。
pub fn is_available() -> bool {
    if !crate::has_slot!(md_is_available) {
        return false;
    }
    match crate::__rt::api().md_is_available {
        Some(f) => unsafe { f() },
        None => false,
    }
}

/// 登记一个简单自定义维度。返回维度 id（≥3）。
pub fn add_simple(name: &str, seed: u32, generator: GeneratorType) -> Result<i32> {
    let f = crate::require_slot!(md_add_simple_dimension, "登记自定义维度");
    let id = unsafe { f(s(name), seed, generator.as_i32()) };
    if id < 0 {
        Err(Error(format!(
            "登记不了维度 {name}（名字非法，或维度号用尽）"
        )))
    } else {
        Ok(id)
    }
}

/// 登记一个地皮世界。网格由生成器在生成时铺出来，不是事后刷方块。
pub fn add_plot(name: &str, seed: u32, layout: &PlotLayout) -> Result<i32> {
    let f = crate::require_slot!(md_add_plot_dimension, "登记地皮维度");
    let spec = layout.to_snbt();
    let id = unsafe { f(s(name), seed, s(&spec)) };
    if id < 0 {
        Err(Error(format!(
            "登记不了地皮维度 {name}（名字非法，或布局参数越界）"
        )))
    } else {
        Ok(id)
    }
}

/// 按名字查维度 id。
///
/// 只对**真正登记过**的名字给 id，没有的返回 `None` —— 而不是那个数值会在
/// 运行期变、看起来却像合法 id 的「未定义维度」。
///
/// 多数时候用不着它，见模块文档里那条「无条件注册」。
pub fn dimension_id(name: &str) -> Option<i32> {
    if !crate::has_slot!(md_get_dimension_id) {
        return None;
    }
    let f = crate::__rt::api().md_get_dimension_id?;
    match unsafe { f(s(name)) } {
        id if id >= 0 => Some(id),
        _ => None,
    }
}

/// 全部已登记的自定义维度。
///
/// 接管一个已有存档的世界管理器**必须**先问这一句：上一个插件建的维度就在
/// 存档里活着，玩家能传进去，而管理器的表里没有它们那几行。后果不是列表短
/// 了几条，而是那些维度不受任何规则管，并且新建世界可能被分到一个和它们
/// 撞号的维度 id。
pub fn list() -> Vec<ExistingDimension> {
    if !crate::has_slot!(md_list_dimensions) {
        return Vec::new();
    }
    let Some(f) = crate::__rt::api().md_list_dimensions else {
        return Vec::new();
    };
    // 每个维度 sink 一次，不是一次交出整个数组（对比 `lane_list`，那个是数组）。
    // 用 `call_out_str` 只留得住最后一条。
    let raw = collect_strs(|ctx, sink| unsafe { f(ctx, sink) });
    let mut out = Vec::with_capacity(raw.len());
    for text in raw {
        if text.trim().is_empty() {
            continue;
        }
        match serde_json::from_str::<ExistingDimensionJson>(&text) {
            Ok(i) => out.push(ExistingDimension {
                name: i.name,
                dim: i.dim,
                snbt: i.snbt,
            }),
            // 坏的那条跳过，不让整批作废 —— `dimension_selector` 依赖这张表。
            Err(e) => crate::Logger::get().warn(&format!(
                "维度清单里有一条解析不了，已跳过：{e}（原文：{}）",
                text.chars().take(200).collect::<String>()
            )),
        }
    }
    out
}

#[derive(serde::Deserialize)]
struct ExistingDimensionJson {
    name: String,
    dim: i32,
    #[serde(default)]
    snbt: String,
}

/// 设一条逐维度规则。
pub fn set_rule(dimension: i32, rule: DimensionRule, allow: bool) -> Result<()> {
    let f = crate::require_slot!(md_set_dimension_rule, "设置维度规则");
    unsafe { f(dimension, rule.as_i32(), allow) };
    Ok(())
}

/// 读一条规则。这个维度对这条规则**没有显式登记**时是 `Ok(None)`，
/// 意思是它走原版行为 —— 和「登记了且值为 false」是两件事。
pub fn rule(dimension: i32, rule: DimensionRule) -> Result<Option<bool>> {
    let f = crate::require_slot!(md_get_dimension_rule, "读取维度规则");
    let mut out = false;
    if unsafe { f(dimension, rule.as_i32(), &mut out) } {
        Ok(Some(out))
    } else {
        Ok(None)
    }
}

/// 清掉一个维度的全部规则（世界被删了）。
pub fn clear_rules(dimension: i32) -> Result<()> {
    let f = crate::require_slot!(md_clear_dimension_rules, "清除维度规则");
    unsafe { f(dimension) };
    Ok(())
}

/// 一块地皮的合并标记。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct PlotMerge {
    pub x: i32,
    pub z: i32,
    /// 1=北 2=东 4=南 8=西 的位集合。
    pub mask: u32,
}

impl PlotMerge {
    pub const NORTH: u32 = 1;
    pub const EAST: u32 = 2;
    pub const SOUTH: u32 = 4;
    pub const WEST: u32 = 8;

    pub fn is_empty(&self) -> bool {
        self.mask == 0
    }

    /// 按 `[北, 东, 南, 西]` 四个方向拼出掩码。
    ///
    /// 顺序就是位序:`NORTH` 是 bit 0。手写 `1 | 4` 要读的人反查这张表,
    /// 而写反了的症状是地皮朝错误的方向合并。
    pub fn from_dirs(x: i32, z: i32, dirs: [bool; 4]) -> PlotMerge {
        let mut mask = 0u32;
        for (i, on) in dirs.iter().enumerate() {
            if *on {
                mask |= 1u32 << i;
            }
        }
        PlotMerge { x, z, mask }
    }
}

/// 登记（或更新）一个维度的地皮网格。`plot_size <= 0` 清掉它。
///
/// 几何一变，合并表就被清空 —— 旧的合并标记在新网格下指向别的地皮。
pub fn set_plot_grid(dimension: i32, plot_size: i32, road_width: i32) -> Result<()> {
    let f = crate::require_slot!(md_set_plot_grid, "登记地皮网格");
    unsafe { f(dimension, plot_size, road_width) };
    Ok(())
}

pub fn clear_plot_grid(dimension: i32) -> Result<()> {
    let f = crate::require_slot!(md_clear_plot_grid, "清除地皮网格");
    unsafe { f(dimension) };
    Ok(())
}

/// **整体替换**一个维度的合并标记。
///
/// 整体而不是增量：增量要求两边永远对同一份当前状态达成一致，而解绑操作会
/// 先清邻居再存自己 —— 中间失败就让两边的视图分岔且回不来。整体推送每次都
/// 把两边拉回一致。
///
/// 先调 [`set_plot_grid`]：往没登记网格的维度推送会被丢弃并告警。
pub fn set_plot_merges(dimension: i32, merges: &[PlotMerge]) -> Result<()> {
    let f = crate::require_slot!(md_set_plot_merges, "推送地皮合并表");
    // ABI 收的是 count 个 (x, z, mask) 三元组，也就是 count*3 个 i32。
    let mut flat: Vec<i32> = Vec::with_capacity(merges.len() * 3);
    for m in merges {
        flat.push(m.x);
        flat.push(m.z);
        flat.push(m.mask as i32);
    }
    unsafe { f(dimension, flat.as_ptr(), merges.len() as i32) };
    Ok(())
}
