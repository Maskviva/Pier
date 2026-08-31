//! 方块 —— 按「维度 + 坐标」寻址的格子。
//!
//! # 写方块有两条路，默认走原生那条
//!
//! [`Block::set`] 走 `set_block`（`BlockSource::setBlock`），吃名字或完整 SNBT。
//! [`Block::set_states`] / [`Block::set_nbt`] 走 `edit_*`，多给一个
//! [`BlockUpdate`] 让调用方决定要不要通知邻居和同步客户端 —— 批量填充时
//! 关掉这两样能快一个数量级，代价是填完必须自己重同步。
//!
//! # 含水方块要看液体层
//!
//! 基岩版的含水不是一个方块状态，而是同一格里的**第二个方块**：主层是楼梯，
//! 液体层是水。[`Block::name`] 只看主层，所以复制粘贴含水楼梯会把水丢干净 ——
//! 主层一格不差，水全没了。要连水一起搬就得读写 [`Block::extra`]。

mod edit;
mod props;
mod state;

use core::ffi::c_void;

use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, r_owned, s};
use crate::sys;
use crate::types::{Bounds, PositionI32};

/// 一格方块读出来的样子。
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct BlockInfo {
    pub pos: PositionI32,
    /// 类型名，`"minecraft:redstone_wire"` 这样。
    pub name: String,
    /// 完整序列化（`{name, states, version}`）。
    pub snbt: String,
}

impl BlockInfo {
    pub fn is_air(&self) -> bool {
        self.name == "minecraft:air"
    }
}

/// 世界里的一格。
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Block {
    dim: i32,
    x: i32,
    y: i32,
    z: i32,
}

impl Block {
    pub fn at(dim: i32, x: i32, y: i32, z: i32) -> Block {
        Block { dim, x, y, z }
    }

    pub fn at_pos(dim: i32, pos: PositionI32) -> Block {
        Block {
            dim,
            x: pos.0,
            y: pos.1,
            z: pos.2,
        }
    }

    pub fn dimension(&self) -> i32 {
        self.dim
    }

    pub fn position(&self) -> PositionI32 {
        (self.x, self.y, self.z)
    }

    // ── 读 ────────────────────────────────────────────────────

    /// 类型名加完整 SNBT，一次调用拿齐。
    pub fn read(&self) -> Result<BlockInfo> {
        let f = crate::require_slot!(get_block, "读取方块");
        let mut out: Option<BlockInfo> = None;
        let ok = unsafe {
            f(
                self.dim,
                self.x,
                self.y,
                self.z,
                (&mut out as *mut Option<BlockInfo>).cast(),
                set_block_info,
            )
        };
        if !ok {
            return Err(Error(format!("读不出 {self}（关卡没就绪，或维度不可用）")));
        }
        out.ok_or_else(|| Error(format!("宿主说读 {self} 成功，却一个字都没写回来")))
    }

    /// 完整序列化解析成 NBT 树。要原样写回用 [`Block::set_nbt`]，
    /// 那条路不经过这一层的解析器。
    pub fn to_nbt(&self) -> Result<NbtValue> {
        let text = self.snbt()?;
        NbtValue::parse(&text).map_err(|e| Error(format!("方块 SNBT 解析失败：{e}")))
    }

    /// 读一个 `PIER_BPROP_*` 数值属性。
    pub fn num(&self, prop: i32) -> Result<f64> {
        let f = crate::require_slot!(block_get_num, "读取方块数值属性");
        let mut out = 0.0f64;
        let ok = unsafe { f(self.dim, self.x, self.y, self.z, prop, &mut out) };
        if ok {
            Ok(out)
        } else {
            Err(Error(format!(
                "读不出 {self} 的属性 {prop}（关卡没就绪，或宿主不认识这个属性号）"
            )))
        }
    }

    /// 读一个 `PIER_BSTR_*` 字符串属性。
    pub fn text(&self, prop: i32) -> Result<String> {
        let f = crate::require_slot!(block_get_str, "读取方块字符串属性");
        call_out_str(|ctx, sink| unsafe { f(self.dim, self.x, self.y, self.z, prop, ctx, sink) })
            .ok_or_else(|| Error(format!("读不出 {self} 的字符串属性 {prop}")))
    }


    /// 方块标签。
    pub fn tags(&self) -> Result<Vec<String>> {
        crate::item::parse_str_list(&self.text(sys::PIER_BSTR_TAGS)?, "方块标签")
    }


    /// 跑一个 `PIER_BACT_*` 动作。
    pub fn act(&self, action: i32, sarg: &str) -> Result<String> {
        let f = crate::require_slot!(block_action, "执行方块动作");
        call_out_str(|ctx, sink| unsafe {
            f(self.dim, self.x, self.y, self.z, action, s(sarg), ctx, sink)
        })
        .ok_or_else(|| Error(format!("{self} 的动作 {action} 失败")))
    }

    pub fn has_tag(&self, tag: &str) -> Result<bool> {
        let out = self.act(sys::PIER_BACT_HAS_TAG, tag)?;
        Ok(out.trim() == "1")
    }

    /// 把这一格当成物品（`Block::asItemInstance`）。
    pub fn as_item(&self) -> Result<crate::item::ItemStack> {
        Ok(crate::item::ItemStack::from_snbt(
            self.act(sys::PIER_BACT_AS_ITEM, "")?,
        ))
    }

    /// 在这一格掉落一件物品。
    pub fn pop_resource(&self, item: &crate::item::ItemStack) -> Result<()> {
        self.act(sys::PIER_BACT_POP_RESOURCE, item.snbt()).map(|_| ())
    }

}

impl std::fmt::Display for Block {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "方块[{},{},{},{}]", self.dim, self.x, self.y, self.z)
    }
}

/// # Safety
/// `ctx` 必须是一个有效的 `*mut Option<BlockInfo>`。
unsafe extern "C" fn set_block_info(
    ctx: *mut c_void,
    x: i32,
    y: i32,
    z: i32,
    name: sys::PierStr,
    snbt: sys::PierStr,
) {
    *ctx.cast::<Option<BlockInfo>>() = Some(BlockInfo {
        pos: (x, y, z),
        name: r_owned(name),
        snbt: r_owned(snbt),
    });
}

/// 解析 `[{min:[x,y,z],max:[x,y,z]}, …]` 形状的盒子列表。
///
/// 坐标在 SNBT 里是浮点（碰撞盒是格内偏移），而 [`Bounds`] 是整数格 ——
/// 向下取整到所在格。需要亚格精度的调用方应当直接读
/// `PIER_BSTR_COLLISION_SHAPE` 自己解析。
fn parse_boxes(text: &str) -> Result<Vec<Bounds>> {
    if text.trim().is_empty() {
        return Ok(Vec::new());
    }
    let v = NbtValue::parse(text).map_err(|e| Error(format!("碰撞盒 SNBT 解析失败：{e}")))?;
    let Some(items) = v.as_list() else {
        return Err(Error(format!("碰撞盒不是列表，而是 {}", v.type_name())));
    };
    let floor = |t: (f64, f64, f64)| (t.0.floor() as i32, t.1.floor() as i32, t.2.floor() as i32);
    Ok(items
        .iter()
        .filter_map(|b| {
            Some(Bounds {
                min: floor(b.get_vec3("min").ok()?),
                max: floor(b.get_vec3("max").ok()?),
            })
        })
        .collect())
}


