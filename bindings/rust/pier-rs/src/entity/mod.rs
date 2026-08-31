//! 实体 —— 按 `ActorUniqueID` 寻址的一切，玩家也在内。
//!
//! # id 是身份，不是指针
//!
//! [`Entity`] 里只有一个 `i64`。每次调用宿主都重新查一遍活体表，所以一个
//! `Entity` 值可以跨 tick 留着：实体死了之后调用返回 `Err`，而不是跳进
//! 一块已经释放的内存。代价是每次调用都有一次查表，热路径上要自己缓存结果。
//!
//! # 玩家从这里过一遍才能用实体能力
//!
//! `Player::as_entity()` 走 `player_resolve` 拿到 id。反过来不成立 ——
//! 一个实体 id 未必是玩家，也没有从 id 反查选择器的槽。

mod actions;
mod props;
mod relations;

use core::ffi::c_void;

use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, r_owned};
use crate::sys;
use crate::types::{PositionF64, RayHit};

/// 一条状态效果。
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Effect {
    pub id: String,
    pub ticks: i32,
    pub amplifier: i32,
    pub visible: bool,
}

/// 实体的轴对齐包围盒。
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Aabb {
    pub min: PositionF64,
    pub max: PositionF64,
}

/// `list_actors` 报出来的一条。
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ActorEntry {
    pub id: i64,
    pub type_name: String,
}

/// 一个实体。零成本包一个 `ActorUniqueID`。
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub struct Entity(i64);

impl Entity {
    pub fn from_id(id: i64) -> Entity {
        Entity(id)
    }

    pub fn id(&self) -> i64 {
        self.0
    }

    /// 枚举活着的实体。`dim` 为 `None` 时跨全部维度。
    ///
    /// 这个槽没有失败位 —— 关卡没就绪时它一条都不报。所以空表既可能是
    /// 「这个维度里没有实体」，也可能是「关卡还没起来」，调用方要自己
    /// 用 `Host::gaming_status()` 分辨。
    pub fn list(dim: Option<i32>) -> Vec<ActorEntry> {
        if !crate::has_slot!(list_actors) {
            return Vec::new();
        }
        let Some(f) = crate::__rt::api().list_actors else {
            return Vec::new();
        };
        let mut out: Vec<ActorEntry> = Vec::new();
        unsafe {
            f(
                dim.unwrap_or(-1),
                (&mut out as *mut Vec<ActorEntry>).cast(),
                push_actor,
            )
        };
        out
    }

    /// 这个 id 现在还指向一个活着的实体吗。
    ///
    /// 判据是「类型名读不读得出来」：任何一个能被解析到的实体都有类型名，
    /// 解析不到的会让 `actor_get_str` 返回 false。
    pub fn exists(&self) -> bool {
        self.text(sys::PIER_ASTR_TYPE_NAME).is_ok()
    }

    pub fn snapshot(&self) -> Result<NbtValue> {
        let f = crate::require_slot!(actor_snapshot, "读取实体快照");
        let text = call_out_str(|ctx, sink| unsafe { f(self.0, ctx, sink) })
            .ok_or_else(|| Error(format!("实体 {} 解析不到，取不了快照", self.0)))?;
        NbtValue::parse(&text).map_err(|e| Error(format!("实体快照 SNBT 解析失败：{e}")))
    }

    // ── 属性 ──────────────────────────────────────────────────

    /// 读一个 `PIER_APROP_*` 数值属性。
    pub fn num(&self, prop: i32) -> Result<f64> {
        let f = crate::require_slot!(actor_get_num, "读取实体数值属性");
        let mut out = 0.0f64;
        let ok = unsafe { f(self.0, prop, &mut out) };
        if ok {
            Ok(out)
        } else {
            Err(Error(format!(
                "读不出实体 {} 的属性 {prop}（实体不在了，或宿主不认识这个属性号）",
                self.0
            )))
        }
    }

    /// 读一个 `PIER_ASTR_*` 字符串属性。
    pub fn text(&self, prop: i32) -> Result<String> {
        let f = crate::require_slot!(actor_get_str, "读取实体字符串属性");
        call_out_str(|ctx, sink| unsafe { f(self.0, prop, ctx, sink) })
            .ok_or_else(|| Error(format!("读不出实体 {} 的字符串属性 {prop}", self.0)))
    }


    /// 位置（`Actor::getPosition`）。玩家的脚下坐标见 [`Entity::feet_pos`]。
    pub fn pos(&self) -> Result<PositionF64> {
        Ok((
            self.num(sys::PIER_APROP_POS_X)?,
            self.num(sys::PIER_APROP_POS_Y)?,
            self.num(sys::PIER_APROP_POS_Z)?,
        ))
    }

    pub fn feet_pos(&self) -> Result<PositionF64> {
        Ok((
            self.num(sys::PIER_APROP_FEET_X)?,
            self.num(sys::PIER_APROP_FEET_Y)?,
            self.num(sys::PIER_APROP_FEET_Z)?,
        ))
    }

    pub fn head_pos(&self) -> Result<PositionF64> {
        Ok((
            self.num(sys::PIER_APROP_HEAD_X)?,
            self.num(sys::PIER_APROP_HEAD_Y)?,
            self.num(sys::PIER_APROP_HEAD_Z)?,
        ))
    }

    pub fn velocity(&self) -> Result<PositionF64> {
        Ok((
            self.num(sys::PIER_APROP_VEL_X)?,
            self.num(sys::PIER_APROP_VEL_Y)?,
            self.num(sys::PIER_APROP_VEL_Z)?,
        ))
    }

    /// 视线方向的单位向量。
    pub fn view_vector(&self) -> Result<PositionF64> {
        Ok((
            self.num(sys::PIER_APROP_VIEW_X)?,
            self.num(sys::PIER_APROP_VIEW_Y)?,
            self.num(sys::PIER_APROP_VIEW_Z)?,
        ))
    }

    /// `(pitch, yaw)`。
    pub fn rotation(&self) -> Result<(f64, f64)> {
        Ok((
            self.num(sys::PIER_APROP_ROT_PITCH)?,
            self.num(sys::PIER_APROP_ROT_YAW)?,
        ))
    }


}

impl std::fmt::Display for Entity {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "entity#{}", self.0)
    }
}

/// # Safety
/// `ctx` 必须是一个有效的 `*mut Vec<ActorEntry>`。
unsafe extern "C" fn push_actor(ctx: *mut c_void, id: sys::PierActorId, type_name: sys::PierStr) {
    (*ctx.cast::<Vec<ActorEntry>>()).push(ActorEntry {
        id,
        type_name: r_owned(type_name),
    });
}

/// 解析两个射线槽的应答。
///
/// 两种形状的差别只在方块那一支（一个给 `block` + `facing`，一个只给 `pos`），
/// 所以一个解析器吃两种：缺哪个字段就用另一个补，都缺才报错。
pub(crate) fn parse_ray_hit(text: &str) -> Result<RayHit> {
    let v = NbtValue::parse(text).map_err(|e| Error(format!("射线结果 SNBT 解析失败：{e}")))?;
    let kind = v.opt_str("type").unwrap_or("none").to_owned();
    let pos = v.get_vec3("pos").unwrap_or((0.0, 0.0, 0.0));
    match kind.as_str() {
        "entity" => {
            let id = v
                .opt_i64("entity_id")
                .or_else(|| v.opt_i64("entity"))
                .ok_or_else(|| Error("射线报了命中实体，却没给实体 id".to_owned()))?;
            Ok(RayHit::Entity { id, pos })
        }
        "block" => {
            let block = match v.get_block_pos("block") {
                Ok(b) => b,
                // 只给了精确坐标的那一支：向下取整得到所在格。
                Err(_) => (
                    pos.0.floor() as i32,
                    pos.1.floor() as i32,
                    pos.2.floor() as i32,
                ),
            };
            Ok(RayHit::Block {
                block,
                facing: v.opt_i32("facing").unwrap_or(-1),
                name: v.opt_str("block_name").unwrap_or_default().to_owned(),
                pos,
            })
        }
        "none" => Ok(RayHit::None),
        other => Err(Error(format!("射线结果的 type 是不认识的 {other:?}"))),
    }
}
