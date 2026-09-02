//! 方块状态与方块实体。
//!
//! 方块状态是方块身份的一部分（改了就是另一个方块）；方块实体是挂在
//! 这一格上的额外数据（箱子里的东西、告示牌上的字）。两者不是一回事。

use super::parse_boxes;
use crate::block::Block;
use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, s};
use crate::sys;
use crate::types::Bounds;

impl Block {
    // ── 方块状态 ──────────────────────────────────────────────

    /// 读一个方块状态的值。
    pub fn state(&self, name: &str) -> Result<String> {
        let f = crate::require_slot!(block_get_state, "读取方块状态");
        call_out_str(|ctx, sink| unsafe { f(self.dim, self.x, self.y, self.z, s(name), ctx, sink) })
            .ok_or_else(|| Error(format!("{self} 没有名为 {name} 的方块状态")))
    }

    /// 全部方块状态。
    pub fn states(&self) -> Result<NbtValue> {
        let text = self.text(sys::PIER_BSTR_STATE)?;
        NbtValue::parse(&text).map_err(|e| Error(format!("方块状态 SNBT 解析失败：{e}")))
    }

    pub fn set_state(&self, name: &str, value: &str) -> Result<()> {
        let f = crate::require_slot!(block_set_state, "写入方块状态");
        let ok = unsafe { f(self.dim, self.x, self.y, self.z, s(name), s(value)) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "写不了 {self} 的状态 {name}={value}（这个方块没有这个状态，或值不在取值域里）"
            )))
        }
    }

    /// 碰撞盒。
    pub fn collision_shape(&self) -> Result<Vec<Bounds>> {
        let f = crate::require_slot!(block_get_collision_shape, "读取方块碰撞盒");
        let text =
            call_out_str(|ctx, sink| unsafe { f(self.dim, self.x, self.y, self.z, ctx, sink) })
                .ok_or_else(|| Error(format!("读不出 {self} 的碰撞盒")))?;
        parse_boxes(&text)
    }

    // ── 方块实体 ──────────────────────────────────────────────

    /// 方块实体的 NBT。这一格上没有方块实体时是 `Ok(None)`。
    pub fn block_entity(&self) -> Result<Option<NbtValue>> {
        let f = crate::require_slot!(block_entity_snbt, "读取方块实体");
        let Some(text) =
            call_out_str(|ctx, sink| unsafe { f(self.dim, self.x, self.y, self.z, ctx, sink) })
        else {
            return Ok(None);
        };
        let v =
            NbtValue::parse(&text).map_err(|e| Error(format!("方块实体 SNBT 解析失败：{e}")))?;
        Ok(Some(v))
    }

    /// 把方块实体的 NBT 写回去（`BlockActor::load`）。
    /// 这一格上必须已经是对应的那种方块。
    pub fn set_block_entity(&self, snbt: &str) -> Result<()> {
        let f = crate::require_slot!(edit_set_block_entity, "写入方块实体");
        let ok = unsafe { f(self.dim, self.x, self.y, self.z, s(snbt)) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "写不了 {self} 的方块实体（这一格上不是对应的方块，或 NBT 形状不对）"
            )))
        }
    }
}
