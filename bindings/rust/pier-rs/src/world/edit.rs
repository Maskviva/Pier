//! 改动世界内容：区域扫描、生成实体、爆炸、粒子、寻路。
//!
//! 和 `mod.rs` 里那些关卡设置的区别是它们**动的是世界里的东西**，
//! 而不是世界本身的开关。大区域一律走 [`World::scan_with`]。

use core::ffi::c_void;

use crate::block::BlockInfo;
use crate::entity::Entity;
use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, r_owned, s};
use crate::sys;
use crate::types::Bounds;
use crate::world::{EntityInfo, Scan, World};

impl World {
    // ── 扫描 ──────────────────────────────────────────────────

    /// 扫一块区域，把方块和实体全收进内存。
    ///
    /// 大区域请改用 [`World::scan_with`] —— 这个函数的内存占用和格数成正比。
    pub fn scan(&self, dim: i32, bounds: Bounds) -> Result<Scan> {
        // 两个累加器必须是**两个变量**：一个 `Scan` 会被两个闭包各借走一次
        // 可变引用，借用检查器不接受。
        let mut blocks: Vec<BlockInfo> = Vec::new();
        let mut entities: Vec<EntityInfo> = Vec::new();
        {
            let mut on_block = |b: BlockInfo| blocks.push(b);
            let mut on_entity = |e: EntityInfo| entities.push(e);
            self.scan_with(dim, bounds, &mut on_block, &mut on_entity)?;
        }
        Ok(Scan { blocks, entities })
    }

    /// 流式扫描：宿主 sink 一条，回调就跑一次，什么都不攒。
    ///
    /// 两个回调都在**这次调用期间**同步执行，返回之后宿主交来的指针立刻失效，
    /// 所以回调收到的是已经拷贝过的 `String`（契约 §三）。
    pub fn scan_with(
        &self,
        dim: i32,
        bounds: Bounds,
        on_block: &mut dyn FnMut(BlockInfo),
        on_entity: &mut dyn FnMut(EntityInfo),
    ) -> Result<()> {
        let f = crate::require_slot!(scan_region, "扫描区域");
        let mut ctx = ScanCtx {
            on_block,
            on_entity,
            panicked: false,
        };
        let ok = unsafe {
            f(
                dim,
                bounds.min.0,
                bounds.min.1,
                bounds.min.2,
                bounds.max.0,
                bounds.max.1,
                bounds.max.2,
                (&mut ctx as *mut ScanCtx).cast(),
                scan_block_sink,
                scan_entity_sink,
            )
        };
        if ctx.panicked {
            return Err(Error(
                "扫描回调 panic 了。已就地拦下 —— 这次扫描的结果不完整".to_owned(),
            ));
        }
        if ok {
            Ok(())
        } else {
            Err(Error(format!("扫描维度 {dim} 失败（关卡没就绪，或维度不可用）")))
        }
    }

    // ── 实体与效果 ────────────────────────────────────────────

    pub fn spawn_mob(
        &self,
        dim: i32,
        type_name: &str,
        x: f64,
        y: f64,
        z: f64,
    ) -> Result<Entity> {
        let f = crate::require_slot!(spawn_mob, "生成生物");
        let mut out: sys::PierActorId = 0;
        let ok = unsafe { f(dim, s(type_name), x, y, z, &mut out) };
        if ok {
            Ok(Entity::from_id(out))
        } else {
            Err(Error(format!(
                "生成不了 {type_name}（类型名不认识，或维度 {dim} 不可用）"
            )))
        }
    }

    /// 从完整 NBT 生成一个实体，[`Entity::snapshot`] 的逆操作。
    ///
    /// `pos` 给了就覆盖 NBT 里的 `Pos`。UniqueID 由引擎重新分配，
    /// 所以存档里的那个 id 不会被复用。
    pub fn spawn_entity_nbt(
        &self,
        dim: i32,
        snbt: &str,
        pos: Option<(f64, f64, f64)>,
    ) -> Result<Entity> {
        let f = crate::require_slot!(edit_spawn_entity_nbt, "按 NBT 生成实体");
        let (use_pos, (x, y, z)) = match pos {
            Some(p) => (true, p),
            None => (false, (0.0, 0.0, 0.0)),
        };
        let mut out: sys::PierActorId = 0;
        let ok = unsafe { f(dim, s(snbt), use_pos, x, y, z, &mut out) };
        if ok {
            Ok(Entity::from_id(out))
        } else {
            Err(Error("按 NBT 生成不了实体（NBT 形状不对，或维度不可用）".to_owned()))
        }
    }

    /// 引爆。`source` 给 `None` 表示没有来源实体。
    #[allow(clippy::too_many_arguments)]
    pub fn explode(
        &self,
        dim: i32,
        x: f64,
        y: f64,
        z: f64,
        radius: f32,
        max_resistance: f32,
        source: Option<Entity>,
        fire: bool,
        breaks_blocks: bool,
        allow_underwater: bool,
    ) -> Result<()> {
        let f = crate::require_slot!(explode, "引爆");
        let ok = unsafe {
            f(
                dim,
                x,
                y,
                z,
                radius,
                max_resistance,
                source.map_or(0, |e| e.id()),
                fire,
                breaks_blocks,
                allow_underwater,
            )
        };
        if ok {
            Ok(())
        } else {
            Err(Error(format!("在维度 {dim} 引爆失败（关卡没就绪，或维度不可用）")))
        }
    }

    /// 整个维度都能看到的粒子。只想给一个人看用
    /// [`crate::player::Player::spawn_particle`]。
    pub fn spawn_particle(&self, dim: i32, effect: &str, x: f64, y: f64, z: f64) -> Result<()> {
        let f = crate::require_slot!(spawn_particle, "生成粒子");
        if unsafe { f(dim, s(effect), x, y, z) } {
            Ok(())
        } else {
            Err(Error(format!("在维度 {dim} 生成粒子失败（关卡没就绪）")))
        }
    }

    /// 给一个实体算一条到目标格的路径。
    pub fn find_path(&self, who: Entity, x: i32, y: i32, z: i32) -> Result<NbtValue> {
        let f = crate::require_slot!(level_find_path, "寻路");
        let text = call_out_str(|ctx, sink| unsafe { f(who.id(), x, y, z, ctx, sink) })
            .ok_or_else(|| Error(format!("给 {who} 寻路失败（实体不在了，或它不会走路）")))?;
        NbtValue::parse(&text).map_err(|e| Error(format!("寻路结果 SNBT 解析失败：{e}")))
    }

}

// ── 扫描的两个 sink ───────────────────────────────────────────────

struct ScanCtx<'a> {
    on_block: &'a mut dyn FnMut(BlockInfo),
    on_entity: &'a mut dyn FnMut(EntityInfo),
    /// 回调 panic 过。panic 穿过 `extern "C"` 是未定义行为，所以在 sink 里
    /// 就地拦下，记一位，等这次调用返回之后报成 `Err`。
    panicked: bool,
}

/// # Safety
/// `ctx` 必须是一个有效的 `*mut ScanCtx`。
unsafe extern "C" fn scan_block_sink(
    ctx: *mut c_void,
    x: i32,
    y: i32,
    z: i32,
    name: sys::PierStr,
    snbt: sys::PierStr,
) {
    let c = &mut *ctx.cast::<ScanCtx>();
    if c.panicked {
        return;
    }
    let info = BlockInfo {
        pos: (x, y, z),
        name: r_owned(name),
        snbt: r_owned(snbt),
    };
    if std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| (c.on_block)(info))).is_err() {
        c.panicked = true;
    }
}

/// # Safety
/// `ctx` 必须是一个有效的 `*mut ScanCtx`。
unsafe extern "C" fn scan_entity_sink(
    ctx: *mut c_void,
    x: i32,
    y: i32,
    z: i32,
    type_name: sys::PierStr,
    snbt: sys::PierStr,
) {
    let c = &mut *ctx.cast::<ScanCtx>();
    if c.panicked {
        return;
    }
    let info = EntityInfo {
        cell: (x, y, z),
        type_name: r_owned(type_name),
        snbt: r_owned(snbt),
    };
    if std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| (c.on_entity)(info))).is_err() {
        c.panicked = true;
    }
}

