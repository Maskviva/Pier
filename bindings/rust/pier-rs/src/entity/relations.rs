//! 实体之间的关系、身上的装备与效果，以及从它眼睛打出去的射线。
//!
//! 放一起是因为它们回答的都是「这个实体和别的东西是什么关系」，
//! 而不是「它自己是什么样」——后者在 `props.rs`。

use super::parse_ray_hit;
use crate::entity::{Aabb, Effect, Entity};
use crate::item::ItemStack;
use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, s};
use crate::sys;
use crate::types::{EquipSlot, RayHit};

impl Entity {
    // ── 关系 ──────────────────────────────────────────────────

    /// 骑着的载具。没有骑任何东西时是 `Ok(None)`，槽位缺席才是 `Err`。
    pub fn vehicle(&self) -> Result<Option<Entity>> {
        let f = crate::require_slot!(actor_get_vehicle, "查询实体的载具");
        Ok(self.related(f))
    }
    pub fn first_passenger(&self) -> Result<Option<Entity>> {
        let f = crate::require_slot!(actor_get_first_passenger, "查询实体的第一位乘客");
        Ok(self.related(f))
    }
    pub fn owner(&self) -> Result<Option<Entity>> {
        let f = crate::require_slot!(actor_get_owner, "查询实体的主人");
        Ok(self.related(f))
    }
    pub fn target(&self) -> Result<Option<Entity>> {
        let f = crate::require_slot!(actor_get_target, "查询实体的攻击目标");
        Ok(self.related(f))
    }

    /// 四个关系槽共用的取值形状：`false` 是「没有这个关系」，不是错误。
    ///
    /// 两道闸留在各自的调用点上，由 `require_slot!` 展开 —— 把函数指针取出来
    /// 交给一个公共 helper 是可以的，但**取指针这一步本身**必须先过长度闸：
    /// 宿主的表短到够不着这个字段时，读它是越界读，而越界读回来的东西
    /// 看起来常常像一个合法的函数指针。
    fn related(
        self,
        f: unsafe extern "C" fn(sys::PierActorId, *mut sys::PierActorId) -> bool,
    ) -> Option<Entity> {
        let mut out: sys::PierActorId = 0;
        if unsafe { f(self.0, &mut out) } {
            Some(Entity(out))
        } else {
            None
        }
    }

    /// 两个实体之间的距离。跨维度时宿主返回失败。
    pub fn distance_to(&self, other: Entity) -> Result<f64> {
        let f = crate::require_slot!(actor_distance_to, "计算实体间距离");
        let mut out = 0.0f64;
        let ok = unsafe { f(self.0, other.0, &mut out) };
        if ok {
            Ok(out)
        } else {
            Err(Error(format!(
                "算不出实体 {} 到 {} 的距离（有一方不在了，或两者不在同一维度）",
                self.0, other.0
            )))
        }
    }

    /// 包围盒。
    pub fn aabb(&self) -> Result<Aabb> {
        let f = crate::require_slot!(actor_get_aabb, "读取实体包围盒");
        let text = call_out_str(|ctx, sink| unsafe { f(self.0, ctx, sink) })
            .ok_or_else(|| Error(format!("读不出实体 {} 的包围盒", self.0)))?;
        let v = NbtValue::parse(&text).map_err(|e| Error(format!("包围盒 SNBT 解析失败：{e}")))?;
        Ok(Aabb {
            min: v.get_vec3("min")?,
            max: v.get_vec3("max")?,
        })
    }

    /// 复制一份到指定位置。
    pub fn clone_at(&self, dim: i32, x: f64, y: f64, z: f64) -> Result<Entity> {
        let f = crate::require_slot!(actor_clone, "复制实体");
        let mut out: sys::PierActorId = 0;
        let ok = unsafe { f(self.0, dim, x, y, z, &mut out) };
        if ok {
            Ok(Entity(out))
        } else {
            Err(Error(format!("复制不了实体 {}（实体不在了，或目标维度不可用）", self.0)))
        }
    }

    // ── 装备与效果 ────────────────────────────────────────────

    pub fn equipped_item(&self, slot: EquipSlot) -> Result<ItemStack> {
        let f = crate::require_slot!(actor_get_equipped_item, "读取实体装备");
        let snbt = call_out_str(|ctx, sink| unsafe { f(self.0, slot.as_i32(), ctx, sink) })
            .ok_or_else(|| Error(format!("读不出实体 {} 的 {slot:?} 装备", self.0)))?;
        Ok(ItemStack::from_snbt(snbt))
    }

    pub fn set_equipped_item(&self, slot: EquipSlot, item: &ItemStack) -> Result<()> {
        let f = crate::require_slot!(actor_set_equipped_item, "写入实体装备");
        let ok = unsafe { f(self.0, slot.as_i32(), s(item.snbt())) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "写不进实体 {} 的 {slot:?} 装备（实体不在了，或物品 SNBT 不合法）",
                self.0
            )))
        }
    }

    /// 身上的全部状态效果。
    pub fn effects(&self) -> Result<Vec<Effect>> {
        let f = crate::require_slot!(actor_get_effects, "读取实体状态效果");
        let text = call_out_str(|ctx, sink| unsafe { f(self.0, ctx, sink) })
            .ok_or_else(|| Error(format!("读不出实体 {} 的状态效果", self.0)))?;
        let v = NbtValue::parse(&text).map_err(|e| Error(format!("状态效果 SNBT 解析失败：{e}")))?;
        let Some(items) = v.as_list() else {
            return Err(Error(format!("状态效果不是列表，而是 {}", v.type_name())));
        };
        Ok(items
            .iter()
            .map(|e| Effect {
                // id 在不同 BDS 版本上可能是数字或名字，两种都收。
                id: match e.get("id") {
                    Some(NbtValue::String(s)) => s.clone(),
                    Some(other) => other.as_i64().map(|n| n.to_string()).unwrap_or_default(),
                    None => String::new(),
                },
                ticks: e.opt_i32("ticks").unwrap_or(0),
                amplifier: e.opt_i32("amplifier").unwrap_or(0),
                visible: e.opt_bool("visible").unwrap_or(true),
            })
            .collect())
    }

    /// 读一位 `ActorFlags`。
    ///
    /// ABI 上这个槽把「实体不在了」和「这一位是 false」压成同一个 `false`
    /// （契约 §5.2 反对的形状，但它已经是发布出去的签名，只能如实说明）。
    /// 需要分辨时先 [`Entity::exists`]。
    pub fn status_flag(&self, flag_index: i32) -> Result<bool> {
        let f = crate::require_slot!(actor_get_status_flag, "读取实体状态位");
        Ok(unsafe { f(self.0, flag_index) })
    }

    pub fn set_status_flag(&self, flag_index: i32, value: bool) -> Result<()> {
        let f = crate::require_slot!(actor_set_status_flag, "写入实体状态位");
        let ok = unsafe { f(self.0, flag_index, value) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "写不了实体 {} 的状态位 {flag_index}（实体不在了，或这一位只读）",
                self.0
            )))
        }
    }

    // ── 射线 ──────────────────────────────────────────────────

    /// 从这个实体的视线打一条射线，命中点按**精确坐标**报。
    pub fn trace_ray(
        self,
        max_dist: f32,
        include_actors: bool,
        include_blocks: bool,
    ) -> Result<RayHit> {
        let f = crate::require_slot!(actor_trace_ray, "射线检测");
        let text = call_out_str(|ctx, sink| unsafe {
            f(self.0, max_dist, include_actors, include_blocks, ctx, sink)
        })
        .ok_or_else(|| Error(format!("实体 {} 的射线检测失败", self.0)))?;
        parse_ray_hit(&text)
    }

    /// 同上，但命中点按**方块格**报，并带上命中面。
    ///
    /// 两个槽都留着是因为它们回答的不是同一个问题：放置方块要格坐标和面，
    /// 画粒子要精确坐标。把后者取整得到前者会在方块边界上差一格。
    pub fn trace_ray_blocks(
        self,
        max_dist: f32,
        include_actors: bool,
        include_blocks: bool,
    ) -> Result<RayHit> {
        let f = crate::require_slot!(edit_trace_ray, "射线检测（方块格）");
        let text = call_out_str(|ctx, sink| unsafe {
            f(self.0, max_dist, include_actors, include_blocks, ctx, sink)
        })
        .ok_or_else(|| Error(format!("实体 {} 的射线检测失败", self.0)))?;
        parse_ray_hit(&text)
    }
}
