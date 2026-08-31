//! 模拟玩家 —— 服务端造出来的真 `ServerPlayer`。
//!
//! 造出来之后，**每一个按名字寻址的玩家 API 都对它成立**：传送、血量、背包、
//! 踢出。这里只放它独有的那一族「让它做点什么」的动作。
//!
//! # 动作走一个多路槽
//!
//! `sim_do` 收「动词 + 参数 SNBT」，动词表在宿主侧长，不占新的表槽位。
//! 代价是拼错的动词要到运行期才报错，所以这里把每个动词包成一个具名方法。
//!
//! # 真玩家永远不会被操控
//!
//! 宿主在 `isSimulatedPlayer()` 上把关，对真玩家的调用直接失败。

use crate::nbt::NbtValue;
use crate::player::Player;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{collect_strs, s};
use crate::sel::PlayerSel;

/// 一个模拟玩家。
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SimPlayer {
    name: String,
}

/// 造一个模拟玩家。名字已被真玩家占用时失败。
pub fn spawn(name: &str, dim: i32, x: f64, y: f64, z: f64) -> Result<SimPlayer> {
    let f = crate::require_slot!(sim_spawn, "生成模拟玩家");
    if unsafe { f(s(name), dim, x, y, z) } {
        Ok(SimPlayer {
            name: name.to_owned(),
        })
    } else {
        Err(Error(format!(
            "生成不了模拟玩家 {name}（名字被占用、关卡没就绪，或维度 {dim} 不可用）"
        )))
    }
}

/// 当前活着的模拟玩家。
///
/// 模拟玩家会跟着存档活过重启，而内存里的句柄不会 —— 重启之后用这个把它们
/// 找回来。
pub fn list() -> Vec<SimPlayer> {
    if !crate::has_slot!(sim_list) {
        return Vec::new();
    }
    let Some(f) = crate::__rt::api().sim_list else {
        return Vec::new();
    };
    collect_strs(|ctx, sink| unsafe { f(ctx, sink) })
        .into_iter()
        .map(|name| SimPlayer { name })
        .collect()
}

impl SimPlayer {
    /// 按名字接上一个已经存在的模拟玩家。**不检查**它是不是真的存在，
    /// 检查用 [`SimPlayer::is_simulated`]。
    pub fn by_name(name: impl Into<String>) -> SimPlayer {
        SimPlayer { name: name.into() }
    }

    pub fn name(&self) -> &str {
        &self.name
    }

    /// 当成普通玩家用，拿到全套玩家 API。
    pub fn player(&self) -> Player {
        Player::by_name(self.name.clone())
    }

    fn sel(&self) -> PlayerSel {
        PlayerSel::Name(self.name.clone())
    }

    /// 这个名字现在确实指向一个活着的模拟玩家吗。
    pub fn is_simulated(&self) -> bool {
        if !crate::has_slot!(sim_is) {
            return false;
        }
        match crate::__rt::api().sim_is {
            Some(f) => unsafe { f(self.sel().raw()) },
            None => false,
        }
    }

    /// 跑一个动词。参数是 SNBT；无参时传 `"{}"`。
    ///
    /// 动词表在宿主侧，这里的具名方法只是它的一层门面。宿主不认识的动词、
    /// 形状不对的参数、或者目标不是模拟玩家，都是 `Err`。
    pub fn act(&self, verb: &str, args_snbt: &str) -> Result<()> {
        let f = crate::require_slot!(sim_do, "驱动模拟玩家");
        let ok = unsafe { f(self.sel().raw(), s(verb), s(args_snbt)) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "模拟玩家 {} 的动作 `{verb}` 失败（动词不认识、参数形状不对，或它不是模拟玩家）",
                self.name
            )))
        }
    }

    fn act0(&self, verb: &str) -> Result<()> {
        self.act(verb, "{}")
    }

    pub fn despawn(self) -> Result<()> {
        self.act0("despawn")
    }
    pub fn stop(&self) -> Result<()> {
        self.act0("stop")
    }
    pub fn jump(&self) -> Result<()> {
        self.act0("jump")
    }
    pub fn attack(&self) -> Result<()> {
        self.act0("attack")
    }
    pub fn interact(&self) -> Result<()> {
        self.act0("interact")
    }
    pub fn use_item(&self) -> Result<()> {
        self.act0("use_item")
    }
    pub fn drop_selected(&self) -> Result<()> {
        self.act0("drop")
    }
    pub fn respawn(&self) -> Result<()> {
        self.act0("respawn")
    }
    pub fn stop_destroying(&self) -> Result<()> {
        self.act0("stop_destroy")
    }

    /// 直接走过去。`face_target` 决定走的时候是不是面朝目标。
    pub fn move_to(&self, x: f64, y: f64, z: f64, speed: f64, face_target: bool) -> Result<()> {
        self.act(
            "move_to",
            &NbtValue::obj([
                ("x", NbtValue::Double(x)),
                ("y", NbtValue::Double(y)),
                ("z", NbtValue::Double(z)),
                ("speed", NbtValue::Double(speed)),
                ("face_target", NbtValue::from(face_target)),
            ])
            .to_snbt(),
        )
    }

    /// 寻路过去。和 [`SimPlayer::move_to`] 的区别是会绕开障碍。
    pub fn navigate_to(&self, x: f64, y: f64, z: f64, speed: f64) -> Result<()> {
        self.act(
            "navigate_to",
            &NbtValue::obj([
                ("x", NbtValue::Double(x)),
                ("y", NbtValue::Double(y)),
                ("z", NbtValue::Double(z)),
                ("speed", NbtValue::Double(speed)),
            ])
            .to_snbt(),
        )
    }

    pub fn look_at(&self, x: f64, y: f64, z: f64) -> Result<()> {
        self.act("look_at", &NbtValue::vec3(x, y, z).to_snbt())
    }

    /// 挖一个方块。`face` 是朝向面，默认 1（上）。
    pub fn destroy_block(&self, x: i32, y: i32, z: i32, face: i32) -> Result<()> {
        self.act("destroy_block", &block_args(x, y, z, face))
    }

    /// 挖视线里那一个。`hand` 是手长（方块）。
    pub fn destroy_look_at(&self, hand: f64) -> Result<()> {
        self.act(
            "destroy_look",
            &NbtValue::obj([("hand", NbtValue::Double(hand))]).to_snbt(),
        )
    }

    pub fn interact_block(&self, x: i32, y: i32, z: i32, face: i32) -> Result<()> {
        self.act("interact_block", &block_args(x, y, z, face))
    }

    pub fn set_sneaking(&self, on: bool) -> Result<()> {
        self.act("sneak", &on_args(on))
    }

    pub fn set_flying(&self, on: bool) -> Result<()> {
        self.act("fly", &on_args(on))
    }

    pub fn chat(&self, msg: &str) -> Result<()> {
        self.act(
            "chat",
            &NbtValue::obj([("msg", NbtValue::from(msg))]).to_snbt(),
        )
    }
}

fn block_args(x: i32, y: i32, z: i32, face: i32) -> String {
    NbtValue::obj([
        ("x", NbtValue::Int(x)),
        ("y", NbtValue::Int(y)),
        ("z", NbtValue::Int(z)),
        ("face", NbtValue::Int(face)),
    ])
    .to_snbt()
}

fn on_args(on: bool) -> String {
    NbtValue::obj([("on", NbtValue::from(on))]).to_snbt()
}

impl std::fmt::Display for SimPlayer {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "sim:{}", self.name)
    }
}
