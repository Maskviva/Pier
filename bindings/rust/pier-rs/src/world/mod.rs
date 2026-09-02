//! 世界 —— 关卡层面的读写：时间、天气、难度、游戏规则、生物群系、区块。
//!
//! 这一层和 [`crate::host`] 的分界是「说的是宿主还是世界」：服务器阶段、排期、
//! 执行命令属于宿主，换个游戏也成立；时间、天气、区块属于世界。
//!
//! 关卡本身的开关在这里；动世界里的东西在 `edit`，拼命令在 `commands`。

mod commands;
mod edit;

pub use commands::{is_valid_ticking_area_name, split_box, Box3D, MAX_FILL_VOLUME};

use crate::block::BlockInfo;
use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, collect_raw, collect_strs, s, s_raw};
use crate::types::{Bounds, Difficulty, PositionI32, Weather};

/// 扫描时落在区域里的一个实体。
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct EntityInfo {
    /// 实体所在的那一格（位置向下取整）。
    pub cell: PositionI32,
    pub type_name: String,
    /// `Actor::save` 的完整 NBT。
    pub snbt: String,
}

/// 一次扫描的结果。
#[derive(Debug, Clone, Default, PartialEq)]
pub struct Scan {
    pub blocks: Vec<BlockInfo>,
    pub entities: Vec<EntityInfo>,
}

impl Scan {
    /// 按坐标索引方块。
    ///
    /// 每调一次都重建一张表，所以循环里别调 —— 那是 O(n²)。要反复查就
    /// 自己留住返回值。ABI 不保证 sink 的遍历顺序，所以不能靠下标算位置。
    pub fn block_map(&self) -> std::collections::HashMap<PositionI32, &BlockInfo> {
        self.blocks.iter().map(|b| (b.pos, b)).collect()
    }

    pub fn non_air_count(&self) -> usize {
        self.blocks.iter().filter(|b| !b.is_air()).count()
    }

    /// 落在这块区域里的实体数。
    pub fn entity_count(&self) -> usize {
        self.entities.len()
    }
}

/// 一个村庄。
#[derive(Debug, Clone, PartialEq)]
pub struct VillageInfo {
    pub uuid: String,
    pub center: PositionI32,
    pub bounds: Bounds,
    pub poi_count: i32,
}

/// 一处硬编码生成区（要塞、女巫小屋、海底神殿、掠夺者前哨站）。
#[derive(Debug, Clone, PartialEq)]
pub struct StructureInfo {
    pub kind: String,
    pub bounds: Bounds,
}

/// 一条游戏规则的值。
#[derive(Debug, Clone, PartialEq)]
pub enum GameRuleValue {
    Bool(bool),
    Int(i64),
    Float(f64),
}

/// 睡眠状态（`level_get_sleep_status`）。
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct SleepStatus {
    pub sleeping: bool,
    pub total_players: i32,
    pub active_sleeping: i32,
}

/// 关卡门面。零大小。
#[derive(Clone, Copy)]
pub struct World(());

impl World {
    pub fn get() -> World {
        World(())
    }

    // ── 时钟与天气 ────────────────────────────────────────────

    pub fn time(&self) -> Result<i64> {
        let f = crate::require_slot!(get_time, "读取世界时间");
        let mut out = 0i64;
        if unsafe { f(&mut out) } {
            Ok(out)
        } else {
            Err(Error("关卡没就绪，读不出世界时间".to_owned()))
        }
    }

    pub fn set_time(&self, t: i64) -> Result<()> {
        let f = crate::require_slot!(set_time, "设置世界时间");
        if unsafe { f(t) } {
            Ok(())
        } else {
            Err(Error("关卡没就绪，设不了世界时间".to_owned()))
        }
    }

    pub fn set_weather(&self, weather: Weather) -> Result<()> {
        let f = crate::require_slot!(set_weather, "设置天气");
        if unsafe { f(weather.as_i32()) } {
            Ok(())
        } else {
            Err(Error("关卡没就绪，设不了天气".to_owned()))
        }
    }

    /// 逐项设置降雨与雷暴的强度和剩余时长（tick）。
    ///
    /// 比 [`World::set_weather`] 细：那个只有三档，这个能做「小雨下三分钟」。
    pub fn update_weather(
        &self,
        rain_level: f32,
        rain_ticks: i32,
        lightning_level: f32,
        lightning_ticks: i32,
    ) -> Result<()> {
        let f = crate::require_slot!(level_update_weather, "更新天气参数");
        let ok = unsafe { f(rain_level, rain_ticks, lightning_level, lightning_ticks) };
        if ok {
            Ok(())
        } else {
            Err(Error("关卡没就绪，更新不了天气参数".to_owned()))
        }
    }

    // ── 关卡设置 ──────────────────────────────────────────────

    pub fn difficulty(&self) -> Result<Difficulty> {
        let f = crate::require_slot!(get_difficulty, "读取难度");
        let mut out = 0i32;
        if !unsafe { f(&mut out) } {
            return Err(Error("关卡没就绪，读不出难度".to_owned()));
        }
        Difficulty::from_i32(out).ok_or_else(|| Error(format!("宿主报了不认识的难度 {out}")))
    }

    pub fn set_difficulty(&self, d: Difficulty) -> Result<()> {
        let f = crate::require_slot!(set_difficulty, "设置难度");
        if unsafe { f(d.as_i32()) } {
            Ok(())
        } else {
            Err(Error("关卡没就绪，设不了难度".to_owned()))
        }
    }

    pub fn seed(&self) -> Result<i64> {
        let f = crate::require_slot!(get_seed, "读取世界种子");
        let mut out = 0i64;
        if unsafe { f(&mut out) } {
            Ok(out)
        } else {
            Err(Error("关卡没就绪，读不出世界种子".to_owned()))
        }
    }

    /// 读一条游戏规则。规则名不认识时是 `Err`，不是某个默认值。
    pub fn game_rule(&self, name: &str) -> Result<GameRuleValue> {
        let f = crate::require_slot!(game_rule_get, "读取游戏规则");
        let text = call_out_str(|ctx, sink| unsafe { f(s(name), ctx, sink) })
            .ok_or_else(|| Error(format!("没有名为 {name} 的游戏规则")))?;
        let v =
            NbtValue::parse(&text).map_err(|e| Error(format!("游戏规则 SNBT 解析失败：{e}")))?;
        let kind = v.get_str("type")?.to_owned();
        match kind.as_str() {
            "bool" => Ok(GameRuleValue::Bool(v.get_bool("value")?)),
            "int" => Ok(GameRuleValue::Int(v.get_i64("value")?)),
            "float" => Ok(GameRuleValue::Float(v.get_f64("value")?)),
            other => Err(Error(format!("游戏规则 {name} 的类型是不认识的 {other:?}"))),
        }
    }

    pub fn set_game_rule(&self, name: &str, value: &str) -> Result<()> {
        let f = crate::require_slot!(game_rule_set, "设置游戏规则");
        if unsafe { f(s(name), s(value)) } {
            Ok(())
        } else {
            Err(Error(format!(
                "设不了游戏规则 {name}={value}（规则名或值不合法）"
            )))
        }
    }

    pub fn default_spawn(&self) -> Result<PositionI32> {
        let f = crate::require_slot!(level_get_default_spawn, "读取默认出生点");
        let (mut x, mut y, mut z) = (0i32, 0i32, 0i32);
        if unsafe { f(&mut x, &mut y, &mut z) } {
            Ok((x, y, z))
        } else {
            Err(Error("关卡没就绪，读不出默认出生点".to_owned()))
        }
    }

    pub fn set_default_spawn(&self, x: i32, y: i32, z: i32) -> Result<()> {
        let f = crate::require_slot!(level_set_default_spawn, "设置默认出生点");
        if unsafe { f(x, y, z) } {
            Ok(())
        } else {
            Err(Error("关卡没就绪，设不了默认出生点".to_owned()))
        }
    }

    /// 立刻存盘。
    pub fn save(&self) -> Result<()> {
        let f = crate::require_slot!(level_save, "保存关卡");
        if unsafe { f() } {
            Ok(())
        } else {
            Err(Error("关卡没就绪，存不了盘".to_owned()))
        }
    }

    pub fn sleep_status(&self) -> Result<SleepStatus> {
        let f = crate::require_slot!(level_get_sleep_status, "读取睡眠状态");
        let text = call_out_str(|ctx, sink| unsafe { f(ctx, sink) })
            .ok_or_else(|| Error("关卡没就绪，读不出睡眠状态".to_owned()))?;
        let v =
            NbtValue::parse(&text).map_err(|e| Error(format!("睡眠状态 SNBT 解析失败：{e}")))?;
        Ok(SleepStatus {
            sleeping: v.opt_bool("sleeping").unwrap_or(false),
            total_players: v.opt_i32("total_players").unwrap_or(0),
            active_sleeping: v.opt_i32("active_sleeping").unwrap_or(0),
        })
    }

    // ── 生物群系 ──────────────────────────────────────────────

    pub fn biome(&self, dim: i32, x: i32, y: i32, z: i32) -> Result<String> {
        let f = crate::require_slot!(level_get_biome, "读取生物群系");
        call_out_str(|ctx, sink| unsafe { f(dim, x, y, z, ctx, sink) })
            .ok_or_else(|| Error(format!("读不出维度 {dim} ({x},{y},{z}) 的生物群系")))
    }

    /// 按列设置一片区域的生物群系。
    ///
    /// 不收 y：`setBiome3d` 是逐 y 的，但基岩版按列存生物群系，给一个 y
    /// 会让调用方以为能分层设。返回**成功设置的列数**；0 意味着一列都没设成
    /// （区块没加载，或群系名不认识），而不是「设了但什么都没变」。
    pub fn set_biome(
        &self,
        dim: i32,
        from: (i32, i32),
        to: (i32, i32),
        biome: &str,
    ) -> Result<i32> {
        let f = crate::require_slot!(level_set_biome, "设置生物群系");
        let (min_x, max_x) = (from.0.min(to.0), from.0.max(to.0));
        let (min_z, max_z) = (from.1.min(to.1), from.1.max(to.1));
        Ok(unsafe { f(dim, min_x, min_z, max_x, max_z, s(biome)) })
    }

    // ── 只读查询 ──────────────────────────────────────────────

    /// 一个维度里的村庄。
    pub fn villages(&self, dim: i32) -> Vec<VillageInfo> {
        // 长度闸和非空闸缺一不可（契约 §2.2）：表短到够不着这个字段时，
        // 读它是越界读，而越界读回来的东西常常看起来像个合法函数指针。
        if !crate::has_slot!(villages) {
            return Vec::new();
        }
        let Some(f) = crate::__rt::api().villages else {
            return Vec::new();
        };
        parse_each(
            collect_strs(|ctx, sink| unsafe { f(dim, ctx, sink) }),
            "村庄",
            |v| {
                Some(VillageInfo {
                    uuid: v.opt_str("uuid").unwrap_or_default().to_owned(),
                    center: v.get_block_pos("center").ok()?,
                    bounds: parse_bounds(v)?,
                    poi_count: v.opt_i32("poi_count").unwrap_or(0),
                })
            },
        )
    }

    /// 半径内**已加载**区块里的硬编码生成区。
    ///
    /// 只看已加载的区块：一个只读查询不该把区块强行加载进来。所以结果为空
    /// 既可能是「附近没有」，也可能是「附近的区块没加载」。
    pub fn structures_near(
        &self,
        dim: i32,
        x: i32,
        y: i32,
        z: i32,
        radius: i32,
    ) -> Vec<StructureInfo> {
        if !crate::has_slot!(structures_near) {
            return Vec::new();
        }
        let Some(f) = crate::__rt::api().structures_near else {
            return Vec::new();
        };
        parse_each(
            collect_strs(|ctx, sink| unsafe { f(dim, x, y, z, radius, ctx, sink) }),
            "结构",
            |v| {
                Some(StructureInfo {
                    kind: v.opt_str("type").unwrap_or_default().to_owned(),
                    bounds: parse_bounds(v)?,
                })
            },
        )
    }

    // ── 区块与存档键 ──────────────────────────────────────────

    /// `[min..max]` 覆盖的区块是不是**全部**在内存里。
    ///
    /// 删存档键之前必须问这一句：一个加载着的区块在内存里有副本，卸载时会把
    /// 刚删掉的键原样写回去，而删除本身「成功」了并报了一个正数。
    pub fn chunks_loaded(
        &self,
        dim: i32,
        min_x: i32,
        min_z: i32,
        max_x: i32,
        max_z: i32,
    ) -> Result<bool> {
        let f = crate::require_slot!(level_chunks_loaded, "查询区块加载状态");
        match unsafe { f(dim, min_x, min_z, max_x, max_z) } {
            1 => Ok(true),
            0 => Ok(false),
            _ => Err(Error(format!("维度 {dim} 不可用，问不出区块加载状态"))),
        }
    }

    /// 删掉一个区块的全部存档键，让引擎下次加载时按生成器重新生成。
    ///
    /// **区块必须先卸载**，见 [`World::chunks_loaded`]。让区块卸载是调用方的事：
    /// 谁在附近、什么时候可以卸载，要的是这一层不该有的领域知识。
    ///
    /// 返回删掉的键数。0 是正常结果，意思是那个区块从来没生成过。
    pub fn delete_chunk_keys(&self, dim: i32, chunk_x: i32, chunk_z: i32) -> Result<i32> {
        let f = crate::require_slot!(level_delete_chunk_keys, "删除区块存档键");
        let n = unsafe { f(dim, chunk_x, chunk_z) };
        if n < 0 {
            Err(Error("存档层不可用，删不了区块键".to_owned()))
        } else {
            Ok(n)
        }
    }

    /// 列出一个区块的全部存档键。
    ///
    /// 键是**二进制**，含 0 字节，所以是 `Vec<u8>` 而不是 `String` ——
    /// 走一次 UTF-8 转换会把它损坏成一个删不掉的键。
    pub fn chunk_keys(&self, dim: i32, chunk_x: i32, chunk_z: i32) -> Result<Vec<Vec<u8>>> {
        let f = crate::require_slot!(level_chunk_keys, "列出区块存档键");
        let mut n = 0i32;
        let keys = collect_raw(|ctx, sink| {
            n = unsafe { f(dim, chunk_x, chunk_z, ctx, sink) };
        });
        if n < 0 {
            Err(Error("存档层不可用，列不出区块键".to_owned()))
        } else {
            Ok(keys)
        }
    }

    /// 逐字删掉一个存档键。内容不解释，传什么删什么。
    pub fn delete_key(&self, key: &[u8]) -> Result<()> {
        let f = crate::require_slot!(level_delete_key, "删除存档键");
        if unsafe { f(s_raw(key)) } {
            Ok(())
        } else {
            Err(Error("存档层不可用，或这个键不存在".to_owned()))
        }
    }
}

// ── 解析助手 ──────────────────────────────────────────────────────

/// 逐条解析一批 SNBT，坏的那条跳过并告警。
///
/// 整批作废是不对的：一个村庄的条目坏了不该让「这个世界有哪些村庄」变成
/// 无法回答的问题。跳过时打日志，否则就成了契约 §5.1 禁的静默回退。
fn parse_each<T>(
    raw: Vec<String>,
    what: &str,
    mut build: impl FnMut(&NbtValue) -> Option<T>,
) -> Vec<T> {
    let mut out = Vec::with_capacity(raw.len());
    for text in raw {
        match NbtValue::parse(&text) {
            Ok(v) => match build(&v) {
                Some(item) => out.push(item),
                None => {
                    crate::Logger::get().warn(&format!("{what}条目缺了必需的字段，已跳过：{text}"))
                }
            },
            Err(e) => crate::Logger::get().warn(&format!("{what}条目 SNBT 解析失败，已跳过：{e}")),
        }
    }
    out
}

/// 取 `{bounds:{min:[…],max:[…]}}`。
fn parse_bounds(v: &NbtValue) -> Option<Bounds> {
    let b = v.get("bounds")?;
    Some(Bounds {
        min: b.get_block_pos("min").ok()?,
        max: b.get_block_pos("max").ok()?,
    })
}
