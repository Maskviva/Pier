//! 容器 —— 玩家身上的四个，加世界里某个坐标上的那一个。
//!
//! ABI 上容器是「所有者 + 哪一个」的组合（`PierContainerRef`），不是指针。
//! 所以一个 `Container` 值可以一直留着：它每次调用都重新解析，玩家下线再上线
//! 之后仍然指向对的东西。
//!
//! # 写完要 [`Container::refresh`]
//!
//! `set_item` / `add_item` / `clear` 走的是 `Container::setItem`，它只改服务端
//! 那一份，一个包都不发。客户端继续渲染它最后收到的内容，直到玩家点一下某个
//! 槽位才被动重同步。批量改完调一次 `refresh` 把整个容器推过去；**别在循环
//! 里逐槽调** —— 它推的是整个容器，逐槽调就是一场包风暴。

use crate::item::ItemStack;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, s};
use crate::sel::PlayerSel;
use crate::sys;

/// 容器的种类。数值与 `PierContainerRef::which` 对齐。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ContainerKind {
    Inventory = 0,
    EnderChest = 1,
    Armor = 2,
    OffHand = 3,
    /// 世界里某个坐标上的容器（箱子、漏斗、熔炉…）。
    Block = 4,
}

impl ContainerKind {
    pub fn as_i32(self) -> i32 {
        self as i32
    }

    /// 方块容器没有单一的所有者，因此不能重同步（见 [`Container::refresh`]）。
    pub fn is_player_owned(self) -> bool {
        !matches!(self, ContainerKind::Block)
    }
}

/// 一个容器的引用。
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Container {
    kind: ContainerKind,
    /// 方块容器用不到它，但 `PierContainerRef` 里这个字段总要有个值，
    /// 所以那时是一个空的按名选择器（永远解析不到人，宿主也不会去解析它）。
    owner: PlayerSel,
    dim: i32,
    x: i32,
    y: i32,
    z: i32,
}

impl Container {
    /// 玩家身上的某个容器。一般不用直接调，走 `Player::inventory()` 那几个。
    pub fn of_player(owner: PlayerSel, kind: ContainerKind) -> Container {
        Container {
            kind,
            owner,
            dim: 0,
            x: 0,
            y: 0,
            z: 0,
        }
    }

    /// 世界里某个坐标上的方块容器。
    pub fn block(dim: i32, x: i32, y: i32, z: i32) -> Container {
        Container {
            kind: ContainerKind::Block,
            owner: PlayerSel::Name(String::new()),
            dim,
            x,
            y,
            z,
        }
    }

    pub fn kind(&self) -> ContainerKind {
        self.kind
    }

    /// 方块容器的坐标。玩家容器返回 `None` —— 它不在某个格子上。
    pub fn position(&self) -> Option<(i32, i32, i32, i32)> {
        match self.kind {
            ContainerKind::Block => Some((self.dim, self.x, self.y, self.z)),
            _ => None,
        }
    }

    /// 转成 FFI 形状。返回的结构体里的 `PierStr` **借用** `self`，
    /// 所以调用点都是「构造完立刻传进去」的形状。
    fn raw(&self) -> sys::PierContainerRef {
        sys::PierContainerRef {
            which: self.kind.as_i32(),
            player: self.owner.raw(),
            dim: self.dim,
            x: self.x,
            y: self.y,
            z: self.z,
        }
    }

    /// 槽位数。
    ///
    /// 解析不到（玩家下线、那个格子上没有容器）返回 `Err` 而不是 0：
    /// 一个 0 会让调用方的 `for i in 0..size` 静默地一次都不跑（契约 §5.2）。
    pub fn size(&self) -> Result<i32> {
        let f = crate::require_slot!(container_size, "读取容器大小");
        let mut out = 0i32;
        let ok = unsafe { f(self.raw(), &mut out) };
        if ok {
            Ok(out)
        } else {
            Err(Error(format!("解析不到容器 {self}")))
        }
    }

    /// 某个槽位里的东西。空槽给的是空气那件物品的 SNBT，不是错误。
    pub fn item(&self, slot: i32) -> Result<ItemStack> {
        let f = crate::require_slot!(container_get_item, "读取容器槽位");
        let snbt = call_out_str(|ctx, sink| unsafe { f(self.raw(), slot, ctx, sink) })
            .ok_or_else(|| Error(format!("读不出容器 {self} 的第 {slot} 槽（槽位越界，或容器解析不到）")))?;
        Ok(ItemStack::from_snbt(snbt))
    }

    /// 全部槽位。
    ///
    /// 中途某一槽读失败就整体失败，不跳过 —— 一个少了几件东西的清单，
    /// 用它做「这个箱子里有什么」的判断会得出错误的答案。
    pub fn items(&self) -> Result<Vec<ItemStack>> {
        let n = self.size()?;
        let mut out = Vec::with_capacity(n.max(0) as usize);
        for slot in 0..n {
            out.push(self.item(slot)?);
        }
        Ok(out)
    }

    /// 写一个槽位。写完记得 [`Container::refresh`]。
    pub fn set_item(&self, slot: i32, item: &ItemStack) -> Result<()> {
        let f = crate::require_slot!(container_set_item, "写入容器槽位");
        let ok = unsafe { f(self.raw(), slot, s(item.snbt())) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "写不进容器 {self} 的第 {slot} 槽（槽位越界、物品 SNBT 不合法，或容器解析不到）"
            )))
        }
    }

    /// 塞一件进去，由引擎挑槽位。
    ///
    /// 容器满了是 `Err` 而不是静默丢弃 —— 后者的症状是玩家的东西凭空消失。
    pub fn add_item(&self, item: &ItemStack) -> Result<()> {
        let f = crate::require_slot!(container_add_item, "向容器添加物品");
        let ok = unsafe { f(self.raw(), s(item.snbt())) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "放不进容器 {self}（容器满了、物品 SNBT 不合法，或容器解析不到）"
            )))
        }
    }

    /// 从某槽拿走 `count` 个。
    pub fn remove_item(&self, slot: i32, count: i32) -> Result<()> {
        let f = crate::require_slot!(container_remove_item, "从容器移除物品");
        let ok = unsafe { f(self.raw(), slot, count) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!("移除不了容器 {self} 第 {slot} 槽的 {count} 个")))
        }
    }

    pub fn clear(&self) -> Result<()> {
        let f = crate::require_slot!(container_clear, "清空容器");
        let ok = unsafe { f(self.raw()) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!("清空不了容器 {self}")))
        }
    }

    /// 把整个容器重发给它的所有者。
    ///
    /// 方块容器返回 `Err`：一个箱子没有唯一的所有者可发，它的观看者由引擎自己
    /// 的容器事务路径刷新。这是宿主的规矩，不是这一层的选择。
    pub fn refresh(&self) -> Result<()> {
        let f = crate::require_slot!(container_refresh, "重同步容器");
        if !self.kind.is_player_owned() {
            return Err(Error(
                "方块容器不能重同步：它没有唯一的所有者，观看者由引擎的容器事务路径刷新".to_owned(),
            ));
        }
        let ok = unsafe { f(self.raw()) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!("重同步不了容器 {self}（玩家多半已经下线）")))
        }
    }
}

impl std::fmt::Display for Container {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self.kind {
            ContainerKind::Block => write!(f, "block[{},{},{},{}]", self.dim, self.x, self.y, self.z),
            other => write!(f, "{:?}({})", other, self.owner),
        }
    }
}
