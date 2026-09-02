//! 写方块 —— 含液体层。
//!
//! 两条写入路径:`set` 走 `set_block`，`set_nbt` / `set_states` 走 `edit_*`
//! 并让调用方自己决定更新标志。液体层是同一格里的第二个方块，不是状态。

use crate::block::Block;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, s};
use crate::types::BlockUpdate;

impl Block {
    // ── 写 ────────────────────────────────────────────────────

    /// 放一个方块。`spec` 是名字（`"minecraft:stone"`）或完整 SNBT。
    ///
    /// 名字不认识时**失败**，不会放一个占位方块下去 —— 后者的症状是世界里
    /// 多出一片看不出是哪来的紫黑块。
    pub fn set(&self, spec: &str) -> Result<()> {
        let f = crate::require_slot!(set_block, "放置方块");
        let ok = unsafe { f(self.dim, self.x, self.y, self.z, s(spec)) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "放不了 {spec} 到 {self}（名字不认识，或关卡没就绪）"
            )))
        }
    }

    /// 从完整 NBT 放一个方块，自己决定更新标志。
    pub fn set_nbt(&self, snbt: &str, update: BlockUpdate) -> Result<()> {
        let f = crate::require_slot!(edit_set_block_nbt, "按 NBT 放置方块");
        let ok = unsafe { f(self.dim, self.x, self.y, self.z, s(snbt), update.bits()) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "按 NBT 放不了方块到 {self}（NBT 形状不对，或名字不认识）"
            )))
        }
    }

    /// 按名字加**部分**状态放一个方块。
    ///
    /// `states` 传 `None` 表示全默认状态。版本号由宿主从默认状态里取，
    /// 调用方不要自己填 —— 填错的版本号会让方块以另一套状态语义落地。
    pub fn set_states(self, name: &str, states: Option<&str>, update: BlockUpdate) -> Result<()> {
        let f = crate::require_slot!(edit_set_block_states, "按状态放置方块");
        let ok = unsafe {
            f(
                self.dim,
                self.x,
                self.y,
                self.z,
                s(name),
                s(states.unwrap_or("")),
                update.bits(),
            )
        };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "按状态放不了 {name} 到 {self}（名字不认识，或状态不在取值域里）"
            )))
        }
    }

    // ── 液体层 ────────────────────────────────────────────────

    /// 读液体层。空的液体层读出来是 `"minecraft:air"`，不是错误。
    pub fn extra(&self) -> Result<String> {
        let f = crate::require_slot!(get_extra_block, "读取液体层");
        call_out_str(|ctx, sink| unsafe { f(self.dim, self.x, self.y, self.z, ctx, sink) })
            .ok_or_else(|| Error(format!("读不出 {self} 的液体层")))
    }

    /// 写液体层。写 `"minecraft:air"` 清空它。
    pub fn set_extra(&self, spec: &str, update: BlockUpdate) -> Result<()> {
        let f = crate::require_slot!(set_extra_block, "写入液体层");
        let ok = unsafe { f(self.dim, self.x, self.y, self.z, s(spec), update.bits()) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "写不了 {self} 的液体层（{spec} 这个名字不认识）"
            )))
        }
    }

    /// 这一格上的容器（箱子、漏斗…）。不检查那里到底有没有容器 ——
    /// 检查要过一次 ABI，而返回的 [`crate::container::Container`] 第一次
    /// 被使用时自然会报出来。
    pub fn container(&self) -> crate::container::Container {
        crate::container::Container::block(self.dim, self.x, self.y, self.z)
    }
}
