//! 踢人、改游戏模式、传送、能力位、权限。
//!
//! 共同点是它们**改变玩家自己的状态**，而且几乎都需要服务端权限。

use crate::entity::Entity;
use crate::item::ItemStack;
use crate::player::Player;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, s};
use crate::sys;
use crate::types::{Ability, AbilityValue, GameMode, PlayerPermission};

impl Player {
    // ── 管理 ──────────────────────────────────────────────────

    pub fn disconnect(&self, reason: &str) -> Result<()> {
        let f = crate::require_slot!(player_disconnect, "踢出玩家");
        let ok = unsafe { f(self.sel.raw(), s(reason)) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!("玩家 {} 不在线，踢不动", self.sel)))
        }
    }

    pub fn set_gamemode(&self, mode: GameMode) -> Result<()> {
        let f = crate::require_slot!(player_set_gamemode, "设置玩家游戏模式");
        let ok = unsafe { f(self.sel.raw(), mode.as_i32()) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "设不了玩家 {} 的游戏模式（不在线）",
                self.sel
            )))
        }
    }

    /// 传送。自定义维度（id ≥ 3）也走这里；目标维度桥造不出匹配的实例时
    /// 宿主**失败**而不是把人丢进一个对不上的维度。
    pub fn teleport(&self, dim: i32, x: f64, y: f64, z: f64) -> Result<()> {
        let f = crate::require_slot!(player_teleport, "传送玩家");
        let ok = unsafe { f(self.sel.raw(), dim, x, y, z) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "传送不了玩家 {}（不在线，或维度 {dim} 不可用）",
                self.sel
            )))
        }
    }

    /// 跑一个 `PIER_PACT_*` 动作，取回它的输出。
    pub fn act(&self, action: i32, sarg: &str, a: f64, b: f64, c: f64) -> Result<String> {
        let f = crate::require_slot!(player_action, "执行玩家动作");
        call_out_str(|ctx, sink| unsafe { f(self.sel.raw(), action, s(sarg), a, b, c, ctx, sink) })
            .ok_or_else(|| {
                Error(format!(
                    "玩家 {} 的动作 {action} 失败（不在线、参数不合法，或宿主不认识这个动作号）",
                    self.sel
                ))
            })
    }

    /// 设一个能力位。
    ///
    /// 布尔能力和浮点能力传错了不会报错，只会被按另一种解释写下去，所以
    /// 这里按 [`Ability::is_float`] 先挡一道。
    pub fn set_ability<V: AbilityValue>(&self, ability: Ability, value: V) -> Result<()> {
        if ability.is_float() == V::IS_BOOL {
            return Err(Error(format!(
                "能力位 {ability:?} 要的是{}值，给的是{}值",
                if ability.is_float() {
                    "浮点"
                } else {
                    "布尔"
                },
                if V::IS_BOOL { "布尔" } else { "数" }
            )));
        }
        self.set_ability_raw(ability.as_i32(), value.as_f64())
    }

    /// 按下标设能力位。宿主比这一层新、多出几位能力时用它。
    pub fn set_ability_raw(&self, index: i32, value: f64) -> Result<()> {
        self.act(sys::PIER_PACT_SET_ABILITY, "", index as f64, value, 0.0)
            .map(|_| ())
    }

    pub fn can_use_ability(&self, ability: Ability) -> Result<bool> {
        let out = self.act(
            sys::PIER_PACT_CAN_USE_ABILITY,
            "",
            ability.as_i32() as f64,
            0.0,
            0.0,
        )?;
        match out.trim() {
            "1" => Ok(true),
            "0" => Ok(false),
            other => Err(Error(format!(
                "能力位查询应当回 \"0\"/\"1\"，实际回了 {other:?}"
            ))),
        }
    }

    pub fn set_permission_level(&self, level: PlayerPermission) -> Result<()> {
        self.act(
            sys::PIER_PACT_SET_PERMISSION_LEVEL,
            "",
            level.as_i32() as f64,
            0.0,
            0.0,
        )
        .map(|_| ())
    }

    pub fn set_selected_slot(&self, slot: i32) -> Result<()> {
        self.act(sys::PIER_PACT_SET_SELECTED_SLOT, "", slot as f64, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn give_item(&self, item: &ItemStack) -> Result<()> {
        self.act(sys::PIER_PACT_GIVE_ITEM, item.snbt(), 0.0, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn set_spawn_point(&self, dim: i32, x: i32, y: i32, z: i32) -> Result<()> {
        self.act(
            sys::PIER_PACT_SET_SPAWN_POINT,
            &dim.to_string(),
            x as f64,
            y as f64,
            z as f64,
        )
        .map(|_| ())
    }
    pub fn add_experience(&self, xp: i32) -> Result<()> {
        self.act(sys::PIER_PACT_ADD_EXPERIENCE, "", xp as f64, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn add_levels(&self, levels: i32) -> Result<()> {
        self.act(sys::PIER_PACT_ADD_LEVELS, "", levels as f64, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn set_chunk_radius(&self, radius: i32) -> Result<()> {
        self.act(sys::PIER_PACT_SET_CHUNK_RADIUS, "", radius as f64, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn set_enchantment_seed(&self, seed: i32) -> Result<()> {
        self.act(
            sys::PIER_PACT_SET_ENCHANTMENT_SEED,
            "",
            seed as f64,
            0.0,
            0.0,
        )
        .map(|_| ())
    }
    pub fn play_emote(&self, piece_id: &str) -> Result<()> {
        self.act(sys::PIER_PACT_PLAY_EMOTE, piece_id, 0.0, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn resend_all_chunks(&self) -> Result<()> {
        self.act(sys::PIER_PACT_RESEND_ALL_CHUNKS, "", 0.0, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn open_inventory(&self) -> Result<()> {
        self.act(sys::PIER_PACT_OPEN_INVENTORY, "", 0.0, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn start_riding(&self, vehicle: Entity) -> Result<()> {
        self.act(
            sys::PIER_PACT_START_RIDING,
            "",
            vehicle.id() as f64,
            0.0,
            0.0,
        )
        .map(|_| ())
    }
    pub fn stop_riding(&self) -> Result<()> {
        self.act(sys::PIER_PACT_STOP_RIDING, "", 0.0, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn attack(&self, target: Entity) -> Result<()> {
        self.act(sys::PIER_PACT_ATTACK, "", target.id() as f64, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn interact(&self, target: Entity) -> Result<()> {
        self.act(sys::PIER_PACT_INTERACT, "", target.id() as f64, 0.0, 0.0)
            .map(|_| ())
    }
    pub fn drop_item(&self, item: &ItemStack, random: bool) -> Result<()> {
        self.act(
            sys::PIER_PACT_DROP,
            item.snbt(),
            if random { 1.0 } else { 0.0 },
            0.0,
            0.0,
        )
        .map(|_| ())
    }

    /// 每人一份的侧边栏。`lines` 从上到下。
    pub fn set_sidebar(&self, objective: &str, title: &str, lines: &[String]) -> Result<()> {
        // 宿主吃的是 "objective\ntitle\nline…" 的换行分隔形式。
        let mut payload = String::with_capacity(objective.len() + title.len() + 16);
        payload.push_str(objective);
        payload.push('\n');
        payload.push_str(title);
        for l in lines {
            payload.push('\n');
            payload.push_str(l);
        }
        self.act(sys::PIER_PACT_SIDEBAR_SET, &payload, 0.0, 0.0, 0.0)
            .map(|_| ())
    }

    pub fn clear_sidebar(&self, objective: &str) -> Result<()> {
        self.act(sys::PIER_PACT_SIDEBAR_CLEAR, objective, 0.0, 0.0, 0.0)
            .map(|_| ())
    }
}
