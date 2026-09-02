//! 背包、装备、手持、冷却。
//!
//! 容器那几个只是造一个 [`crate::Container`] 值，不过 ABI；
//! 真正的读写在被使用时才发生。

use crate::container::{Container, ContainerKind};
use crate::item::ItemStack;
use crate::nbt::NbtValue;
use crate::player::Player;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, s};

impl Player {
    // ── 背包 ──────────────────────────────────────────────────

    pub fn inventory(&self) -> Container {
        Container::of_player(self.sel.clone(), ContainerKind::Inventory)
    }
    pub fn ender_chest(&self) -> Container {
        Container::of_player(self.sel.clone(), ContainerKind::EnderChest)
    }
    pub fn armor(&self) -> Container {
        Container::of_player(self.sel.clone(), ContainerKind::Armor)
    }
    pub fn offhand_container(&self) -> Container {
        Container::of_player(self.sel.clone(), ContainerKind::OffHand)
    }

    /// 副手里那一件。空手时是空气那件物品,不是错误。
    pub fn offhand(&self) -> Result<ItemStack> {
        self.offhand_container().item(0)
    }

    /// 写副手。写完记得 [`Container::refresh`],否则客户端仍显示旧的那件。
    pub fn set_offhand(&self, item: &ItemStack) -> Result<()> {
        self.offhand_container().set_item(0, item)
    }

    /// 手上那件。
    pub fn carried_item(&self) -> Result<ItemStack> {
        let f = crate::require_slot!(player_get_carried_item, "读取玩家手持物品");
        let snbt = call_out_str(|ctx, sink| unsafe { f(self.sel.raw(), ctx, sink) })
            .ok_or_else(|| Error(format!("读不出玩家 {} 手上的东西", self.sel)))?;
        Ok(ItemStack::from_snbt(snbt))
    }

    /// 背包某一槽。
    pub fn item(&self, slot: i32) -> Result<ItemStack> {
        let f = crate::require_slot!(player_get_item, "读取玩家背包槽位");
        let snbt = call_out_str(|ctx, sink| unsafe { f(self.sel.raw(), slot, ctx, sink) })
            .ok_or_else(|| Error(format!("读不出玩家 {} 背包第 {slot} 槽", self.sel)))?;
        Ok(ItemStack::from_snbt(snbt))
    }

    pub fn set_item(&self, slot: i32, item: &ItemStack) -> Result<()> {
        let f = crate::require_slot!(player_set_item, "写入玩家背包槽位");
        let ok = unsafe { f(self.sel.raw(), slot, s(item.snbt())) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "写不进玩家 {} 背包第 {slot} 槽（不在线、槽位越界，或物品 SNBT 不合法）",
                self.sel
            )))
        }
    }

    /// 全套装备。`slot` 编号见 [`crate::types::EquipSlot`]。
    pub fn equipment(&self) -> Result<Vec<(i32, ItemStack)>> {
        let f = crate::require_slot!(player_get_equipment, "读取玩家装备");
        let text = call_out_str(|ctx, sink| unsafe { f(self.sel.raw(), ctx, sink) })
            .ok_or_else(|| Error(format!("读不出玩家 {} 的装备", self.sel)))?;
        let v = NbtValue::parse(&text).map_err(|e| Error(format!("装备 SNBT 解析失败：{e}")))?;
        let Some(items) = v.as_list() else {
            return Err(Error(format!("装备不是列表，而是 {}", v.type_name())));
        };
        Ok(items
            .iter()
            .filter_map(|e| {
                let slot = e.opt_i32("slot")?;
                let snbt = e.opt_str("item_snbt")?;
                Some((slot, ItemStack::from_snbt(snbt)))
            })
            .collect())
    }

    /// 某件物品的冷却还剩多少 tick。
    ///
    /// ABI 上 -1 同时表示「不在冷却」和「玩家不在线」。这里把它原样交出去
    /// 并说明，而不是替调用方猜是哪一种（契约 §5.2）。要分辨就先
    /// [`Player::is_online`]。
    pub fn cooldown(&self, item_name: &str) -> Result<i32> {
        let f = crate::require_slot!(player_get_cooldown, "读取物品冷却");
        Ok(unsafe { f(self.sel.raw(), s(item_name)) })
    }

    pub fn start_cooldown(&self, item_name: &str, ticks: i32) -> Result<()> {
        let f = crate::require_slot!(player_start_cooldown, "开始物品冷却");
        let ok = unsafe { f(self.sel.raw(), s(item_name), ticks) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "起不了玩家 {} 的 {item_name} 冷却（不在线，或物品名不认识）",
                self.sel
            )))
        }
    }
}
