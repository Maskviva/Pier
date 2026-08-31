//! 玩家选择器 —— 「句柄是身份，不是指针」这条规矩的落地形式。
//!
//! ABI 上它是 `{kind: i32, value: PierStr}`，`kind` 取 0/1/2。上一代直接把
//! 这个裸整数递给调用方，于是每个调用点都在写 `PlayerSel { kind: 1, .. }`，
//! 而 `1` 是什么只能翻文档。这里把它变成枚举，顺带把一件**安全相关**的事
//! 写进类型层：
//!
//! # `Name` 不是身份
//!
//! 宿主解析 `kind=0` 时，账号名匹配不上会**退到显示名**（`getNameTag`）。
//! 显示名是可以被别的模组改的（`AACT_SET_NAME_TAG`、各种称号插件），所以
//! 一个玩家把自己的显示名改成某个**离线**玩家的账号名，就能让所有按名字
//! 寻址的调用落到自己身上。做权限、经济、地皮归属的键**只能用
//! [`PlayerSel::xuid`]**；`Name` 留给「玩家在聊天里打了个名字」这类场景。

use crate::rt::ffi::s;
use crate::sys;

/// 怎么指一个玩家。
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum PlayerSel {
    /// 按名字。**先匹配账号名，落空后宿主会退到显示名** —— 见模块文档，
    /// 不要拿它当身份。
    Name(String),
    /// 按 xuid。唯一、不可伪造、玩家改不了。做键就用它。
    /// 离线模式服务器上可能为空串，那时只能退回 `Name`。
    Xuid(String),
    /// 按 uuid（规范字符串形式）。同样稳定，适合做存档键。
    Uuid(String),
}

impl PlayerSel {
    pub fn name(v: impl Into<String>) -> PlayerSel {
        PlayerSel::Name(v.into())
    }
    pub fn xuid(v: impl Into<String>) -> PlayerSel {
        PlayerSel::Xuid(v.into())
    }
    pub fn uuid(v: impl Into<String>) -> PlayerSel {
        PlayerSel::Uuid(v.into())
    }

    /// 底层 `kind` 值（0/1/2，与 `abi.h` 对齐）。
    pub fn kind(&self) -> i32 {
        match self {
            PlayerSel::Name(_) => 0,
            PlayerSel::Xuid(_) => 1,
            PlayerSel::Uuid(_) => 2,
        }
    }

    pub fn value(&self) -> &str {
        match self {
            PlayerSel::Name(v) | PlayerSel::Xuid(v) | PlayerSel::Uuid(v) => v,
        }
    }

    /// 这个选择器是不是可靠身份（xuid / uuid）。
    ///
    /// 权限、经济、归属判定在拿到 `false` 时应当格外小心 —— 见模块文档。
    pub fn is_stable(&self) -> bool {
        !matches!(self, PlayerSel::Name(_))
    }

    /// 空值检查：空选择器永远解析不到任何人，早点发现比在调用点看到
    /// 一个莫名其妙的 `false` 强。
    pub fn is_empty(&self) -> bool {
        self.value().is_empty()
    }

    /// 转成 FFI 形状。
    ///
    /// 注意：返回的 `PierStr` **借用** `self` 里的字符串，只在 `self` 存活期间
    /// 有效。它是 `pub(crate)` 的，调用点都在本 crate 内、且都是「构造完立刻
    /// 传进 FFI」的形状，不会跨调用留存。
    ///
    pub(crate) fn raw(&self) -> sys::PierPlayerSel {
        sys::PierPlayerSel {
            kind: self.kind(),
            value: s(self.value()),
        }
    }
}

impl From<&str> for PlayerSel {
    /// 便利转换：**按名字**。会走显示名回退，做身份用请显式写 `PlayerSel::xuid`。
    fn from(v: &str) -> PlayerSel {
        PlayerSel::Name(v.to_owned())
    }
}

impl From<String> for PlayerSel {
    fn from(v: String) -> PlayerSel {
        PlayerSel::Name(v)
    }
}

impl std::fmt::Display for PlayerSel {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            PlayerSel::Name(v) => write!(f, "name:{v}"),
            PlayerSel::Xuid(v) => write!(f, "xuid:{v}"),
            PlayerSel::Uuid(v) => write!(f, "uuid:{v}"),
        }
    }
}

// ── 维度选择器 ────────────────────────────────────────────────────

/// 一个维度在**命令文本**里的名字，用于 `execute in <sel> run …`。
///
/// 三个原版维度是固定名。自定义维度（id ≥ 3）的名字来自
/// [`crate::dimensions::list`]，所以这一支要过一次 ABI，比原版那三个慢。
///
/// 返回 `None` 表示这个维度**没有可用的选择器**：id 为负、或者它没有登记过。
/// 不要拿 `dim.to_string()` 兜底 —— `execute in 3` 不是合法命令，拼出来的
/// 命令会在引擎那侧失败，而失败信息里看不出根因在这里。
pub fn dimension_selector(dim: i32) -> Option<String> {
    match dim {
        0 => return Some("overworld".to_owned()),
        1 => return Some("nether".to_owned()),
        2 => return Some("the_end".to_owned()),
        _ => {}
    }
    if dim < 0 {
        return None;
    }
    crate::dimensions::list()
        .into_iter()
        .find(|d| d.dim == dim)
        .map(|d| d.name)
}
