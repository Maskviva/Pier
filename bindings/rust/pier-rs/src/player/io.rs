//! 玩家的出站通道：聊天、标题、粒子、原始数据包。
//!
//! 这些**都是发包**，所以它们共享同一组失败模式（人不在线就发不出去），
//! 也共享同一条纪律：文本一律走结构化的包，不拼命令行。

use crate::player::Player;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::s;
use crate::types::{MessageType, TitleKind, TitleTimes};

impl Player {
    // ── 消息与标题 ────────────────────────────────────────────

    pub fn send_message(&self, msg: &str) -> Result<()> {
        let f = crate::require_slot!(player_send_message, "给玩家发消息");
        let ok = unsafe { f(self.sel.raw(), s(msg)) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!("玩家 {} 不在线，消息没发出去", self.sel)))
        }
    }

    /// 指定 `TextPacketType` 发一条。
    pub fn tell(&self, msg: &str, kind: MessageType) -> Result<()> {
        let f = crate::require_slot!(player_send_message_typed, "给玩家发定类型消息");
        let ok = unsafe { f(self.sel.raw(), s(msg), kind.as_i32()) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!("玩家 {} 不在线，消息没发出去", self.sel)))
        }
    }

    /// 发一条标题。
    ///
    /// 走的是真的 `SetTitlePacket`，不是拼一条 `/title` 命令 —— 后者会把
    /// 文本原样贴进命令行，一个名字里带引号或 `@e` 的地皮就成了命令注入。
    ///
    /// `times` 给了就先发一个 Times 包，让时序确定；不给则沿用客户端上一次
    /// 存下来的时长。三段时长不能只给一半，那个组合宿主直接拒绝。
    pub fn send_title(&self, kind: TitleKind, text: &str, times: Option<TitleTimes>) -> Result<()> {
        let f = crate::require_slot!(player_send_title, "给玩家发标题");
        let t = times.unwrap_or(TitleTimes::new(-1, -1, -1));
        let ok = unsafe {
            f(
                self.sel.raw(),
                kind.as_i32(),
                s(if kind.uses_text() { text } else { "" }),
                t.fade_in,
                t.stay,
                t.fade_out,
            )
        };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "给玩家 {} 发标题失败（不在线，或 {kind:?} 这一种宿主拒绝）",
                self.sel
            )))
        }
    }

    pub fn set_title(&self, text: &str) -> Result<()> {
        self.send_title(TitleKind::Title, text, None)
    }
    pub fn set_subtitle(&self, text: &str) -> Result<()> {
        self.send_title(TitleKind::Subtitle, text, None)
    }
    pub fn set_actionbar(&self, text: &str) -> Result<()> {
        self.send_title(TitleKind::Actionbar, text, None)
    }
    pub fn clear_title(&self) -> Result<()> {
        self.send_title(TitleKind::Clear, "", None)
    }

    // ── 网络 ──────────────────────────────────────────────────

    /// 只给这一个玩家生成粒子。
    ///
    /// 和 `World::spawn_particle` 的区别是别人看不见 —— 后者走的是整个维度的
    /// 广播。做选区高亮之类的东西必须用这一个，否则全服都看得到。
    pub fn spawn_particle(&self, dim: i32, effect: &str, x: f64, y: f64, z: f64) -> Result<()> {
        let f = crate::require_slot!(spawn_particle_for, "给单个玩家生成粒子");
        let ok = unsafe { f(self.sel.raw(), dim, s(effect), x, y, z) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!("玩家 {} 不在线，粒子没发出去", self.sel)))
        }
    }

    /// 往这个玩家的连接上塞一个原始数据包。
    ///
    /// **逃生口**：`body` 是当前游戏版本的线格式，版本一变就得跟着改，
    /// 这一点由调用方负责。有具名入口的时候优先用具名的。
    pub fn send_packet(&self, packet_id: i32, body: &[u8]) -> Result<()> {
        let f = crate::require_slot!(send_packet, "发送原始数据包");
        let ok = unsafe { f(self.sel.raw(), packet_id, body.as_ptr(), body.len()) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "给玩家 {} 发 id={packet_id} 的包失败（不在线、id 造不出包，或包体在这个版本上形状不对）",
                self.sel
            )))
        }
    }
}
