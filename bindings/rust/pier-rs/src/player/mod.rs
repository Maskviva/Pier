//! 玩家 —— 按选择器寻址，每次调用重新解析。
//!
//! # 做键只能用 xuid
//!
//! `PlayerSel::Name` 在宿主侧匹配不上账号名时会**退到显示名**，而显示名可以
//! 被别的模组改。一个玩家把显示名改成某个离线玩家的账号名，就能让所有按名字
//! 寻址的调用落到自己身上。权限、经济、归属判定一律用 [`Player::by_xuid`]。
//! 详见 [`crate::sel`] 的模块文档。
//!
//! # 玩家也是实体
//!
//! [`Player::as_entity`] 走 `player_resolve` 拿 `ActorUniqueID`，之后整套
//! [`crate::entity::Entity`] 的能力都能用。两套 API 是互补的：玩家专属的东西
//! （背包、能力位、标题、踢人）在这里，实体通用的（血量、传送、标签）在那里。

mod admin;
mod io;
mod items;
mod props;

use crate::entity::Entity;
use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, collect_strs, s};
use crate::sel::PlayerSel;
use crate::sys;
use crate::types::{GameMode, PlayerPermission, PositionF64};

/// `list_players` 报出来的一条。
///
/// `dimension` 与 `pos` 是 `Option`，不是裸值。宿主在关卡还没就绪、或者那个
/// 玩家正处在维度切换中途时会漏掉这几个键，而把它们补成 `0` 就等于说
/// 「他在主世界的原点」—— 契约 §5.1 记的那次土地保护绕过正是这个形状:
/// 自定义维度的事件读不到 `dim`，消费方 `unwrap_or(0)`，全被当成主世界放行。
///
/// 需要兜底的调用方自己 `.unwrap_or(0)`，那时是**它**在为这个默认值负责。
#[derive(Debug, Clone, Default, PartialEq)]
pub struct PlayerInfo {
    pub name: String,
    pub xuid: String,
    pub uuid: String,
    pub dimension: Option<i32>,
    pub pos: Option<PositionF64>,
}

impl PlayerInfo {
    /// 用 xuid 建一个稳定的选择器。xuid 为空（离线模式服务器）时退回名字，
    /// 并且**说清楚**退回了 —— 调用方据此知道这个键不可靠。
    pub fn selector(&self) -> PlayerSel {
        if self.xuid.is_empty() {
            PlayerSel::Name(self.name.clone())
        } else {
            PlayerSel::Xuid(self.xuid.clone())
        }
    }
}

/// 玩家的网络状况（`PIER_PSTR_NETWORK_STATUS`）。
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct NetworkStatus {
    pub ping: i32,
    pub avg_ping: i32,
    pub max_ping: i32,
    /// 千分比，宿主原样给什么就是什么。
    pub packet_loss: i32,
}

/// 一个玩家。里面只有选择器，没有指针。
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Player {
    sel: PlayerSel,
}

impl Player {
    /// 按名字。**会走显示名回退**，做身份请用 [`Player::by_xuid`]。
    pub fn by_name(name: impl Into<String>) -> Player {
        Player {
            sel: PlayerSel::Name(name.into()),
        }
    }

    /// 按 xuid。唯一、不可伪造、玩家改不了。
    pub fn by_xuid(xuid: impl Into<String>) -> Player {
        Player {
            sel: PlayerSel::Xuid(xuid.into()),
        }
    }

    pub fn by_uuid(uuid: impl Into<String>) -> Player {
        Player {
            sel: PlayerSel::Uuid(uuid.into()),
        }
    }

    pub fn from_sel(sel: PlayerSel) -> Player {
        Player { sel }
    }

    pub fn sel(&self) -> &PlayerSel {
        &self.sel
    }

    /// 在线玩家清单。
    ///
    /// 宿主每人 sink 一条 SNBT。解析不了的那一条**跳过并告警**，而不是让
    /// 整张表变空 —— 一个坏条目不该让「服务器上有谁」变成无法回答的问题。
    pub fn list() -> Vec<PlayerInfo> {
        if !crate::has_slot!(list_players) {
            return Vec::new();
        }
        let Some(f) = crate::__rt::api().list_players else {
            return Vec::new();
        };
        let raw = collect_strs(|ctx, sink| unsafe { f(ctx, sink) });
        let mut out = Vec::with_capacity(raw.len());
        for text in raw {
            match NbtValue::parse(&text) {
                Ok(v) => out.push(PlayerInfo {
                    name: v.opt_str("name").unwrap_or_default().to_owned(),
                    xuid: v.opt_str("xuid").unwrap_or_default().to_owned(),
                    uuid: v.opt_str("uuid").unwrap_or_default().to_owned(),
                    dimension: v.opt_i32("dim"),
                    // 三轴必须**同时**在，缺一个就整体作废：一个 (x, 0, z)
                    // 看起来是合法坐标，而它其实是「y 读不到」。
                    pos: match (v.opt_f64("x"), v.opt_f64("y"), v.opt_f64("z")) {
                        (Some(x), Some(y), Some(z)) => Some((x, y, z)),
                        _ => None,
                    },
                }),
                Err(e) => crate::Logger::get()
                    .warn(&format!("list_players 里有一条 SNBT 解析不了，已跳过：{e}")),
            }
        }
        out
    }

    /// 给所有在线玩家发一条消息。
    pub fn broadcast(msg: &str) -> Result<()> {
        let f = crate::require_slot!(broadcast_message, "全服广播");
        unsafe { f(s(msg)) };
        Ok(())
    }

    /// 这个选择器现在解析得到人吗。
    pub fn is_online(&self) -> bool {
        self.resolve().is_ok()
    }

    /// 解析成 `ActorUniqueID`。
    fn resolve(&self) -> Result<sys::PierActorId> {
        let f = crate::require_slot!(player_resolve, "解析玩家");
        let mut out: sys::PierActorId = 0;
        let ok = unsafe { f(self.sel.raw(), &mut out) };
        if ok {
            Ok(out)
        } else {
            Err(Error(format!("玩家 {} 不在线（或选择器解析不到）", self.sel)))
        }
    }

    /// 当成实体用，拿到 [`Entity`] 的全套能力。
    pub fn as_entity(&self) -> Result<Entity> {
        self.resolve().map(Entity::from_id)
    }

    // ── 属性 ──────────────────────────────────────────────────

    /// 读一个 `PIER_PPROP_*` 数值属性。
    pub fn num(&self, prop: i32) -> Result<f64> {
        let f = crate::require_slot!(player_get_num, "读取玩家数值属性");
        let mut out = 0.0f64;
        let ok = unsafe { f(self.sel.raw(), prop, &mut out) };
        if ok {
            Ok(out)
        } else {
            Err(Error(format!(
                "读不出玩家 {} 的属性 {prop}（不在线，或宿主不认识这个属性号）",
                self.sel
            )))
        }
    }

    /// 写一个 `PIER_PPROP_*` 数值属性。只有标了 (S) 的那几个可写。
    pub fn set_num(&self, prop: i32, v: f64) -> Result<()> {
        let f = crate::require_slot!(player_set_num, "写入玩家数值属性");
        let ok = unsafe { f(self.sel.raw(), prop, v) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "写不了玩家 {} 的属性 {prop}（不在线，或这个属性只读）",
                self.sel
            )))
        }
    }

    /// 读一个 `PIER_PSTR_*` 字符串属性。
    pub fn text(&self, prop: i32) -> Result<String> {
        let f = crate::require_slot!(player_get_str, "读取玩家字符串属性");
        call_out_str(|ctx, sink| unsafe { f(self.sel.raw(), prop, ctx, sink) })
            .ok_or_else(|| Error(format!("读不出玩家 {} 的字符串属性 {prop}", self.sel)))
    }

    fn num_i32(&self, prop: i32) -> Result<i32> {
        self.num(prop).map(|v| v as i32)
    }


    /// 上一次死亡的位置。没死过时是 `Ok(None)`（宿主给空串）。
    pub fn last_death_pos(&self) -> Result<Option<(PositionF64, i32)>> {
        let text = self.text(sys::PIER_PSTR_LAST_DEATH_POS)?;
        if text.trim().is_empty() {
            return Ok(None);
        }
        let v = NbtValue::parse(&text).map_err(|e| Error(format!("死亡坐标解析失败：{e}")))?;
        let dim = self
            .text(sys::PIER_PSTR_LAST_DEATH_DIMENSION)?
            .trim()
            .parse::<i32>()
            .unwrap_or(0);
        Ok(Some((
            (v.get_f64("x")?, v.get_f64("y")?, v.get_f64("z")?),
            dim,
        )))
    }

    pub fn game_type(&self) -> Result<GameMode> {
        let v = self.num_i32(sys::PIER_PPROP_GAME_TYPE)?;
        GameMode::from_i32(v).ok_or_else(|| Error(format!("宿主报了不认识的游戏模式 {v}")))
    }
    pub fn permission_level(&self) -> Result<PlayerPermission> {
        let v = self.num_i32(sys::PIER_PPROP_PERMISSION_LEVEL)?;
        PlayerPermission::from_i32(v).ok_or_else(|| Error(format!("宿主报了不认识的权限等级 {v}")))
    }
    pub fn set_level(&self, level: i32) -> Result<()> {
        self.set_num(sys::PIER_PPROP_LEVEL, level as f64)
    }
    /// 经验条进度，0..1。
    pub fn set_experience(&self, progress: f64) -> Result<()> {
        self.set_num(sys::PIER_PPROP_EXPERIENCE, progress)
    }
    pub fn set_hunger(&self, v: f64) -> Result<()> {
        self.set_num(sys::PIER_PPROP_HUNGER, v)
    }
    pub fn set_saturation(&self, v: f64) -> Result<()> {
        self.set_num(sys::PIER_PPROP_SATURATION, v)
    }
    pub fn set_exhaustion(&self, v: f64) -> Result<()> {
        self.set_num(sys::PIER_PPROP_EXHAUSTION, v)
    }

    /// 位置。走的是**专用槽**而不是属性号：它一次调用给齐三轴加维度，
    /// 而三次属性调用之间玩家可能已经动了。
    pub fn position(&self) -> Result<(PositionF64, i32)> {
        let f = crate::require_slot!(get_player_position, "读取玩家位置");
        let name = self.real_name().unwrap_or_else(|_| self.sel.value().to_owned());
        let p = unsafe { f(s(&name)) };
        if p.found {
            Ok(((p.x, p.y, p.z), p.dimension))
        } else {
            Err(Error(format!("玩家 {} 不在线，取不到位置", self.sel)))
        }
    }

    /// 网络状况明细。
    pub fn network_status(&self) -> Result<NetworkStatus> {
        let f = crate::require_slot!(player_get_network_status, "读取玩家网络状况");
        let text = call_out_str(|ctx, sink| unsafe { f(self.sel.raw(), ctx, sink) })
            .ok_or_else(|| Error(format!("读不出玩家 {} 的网络状况", self.sel)))?;
        let v = NbtValue::parse(&text).map_err(|e| Error(format!("网络状况 SNBT 解析失败：{e}")))?;
        Ok(NetworkStatus {
            ping: v.opt_i32("ping").unwrap_or(-1),
            avg_ping: v.opt_i32("avg_ping").unwrap_or(-1),
            max_ping: v.opt_i32("max_ping").unwrap_or(-1),
            packet_loss: v.opt_i32("packet_loss").unwrap_or(-1),
        })
    }

    /// 这个玩家的连接 id，和数据包拦截器看到的是同一个数。
    ///
    /// 返回 0 表示不在线或拿不到网络标识 —— ABI 上 0 不是合法连接 id，
    /// 所以这里如实报成 `Err` 而不是把 0 交出去。
    pub fn conn_id(&self) -> Result<u64> {
        let f = crate::require_slot!(player_conn_id, "读取玩家连接 id");
        let id = unsafe { f(self.sel.raw()) };
        if id == 0 {
            Err(Error(format!("玩家 {} 不在线，或网络标识不可用", self.sel)))
        } else {
            Ok(id)
        }
    }

}

impl std::fmt::Display for Player {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.sel)
    }
}

impl From<PlayerSel> for Player {
    fn from(sel: PlayerSel) -> Player {
        Player { sel }
    }
}
