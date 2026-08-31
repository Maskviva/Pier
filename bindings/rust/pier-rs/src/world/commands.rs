//! 命令拼装层 —— `/fill` 与 `/tickingarea`。
//!
//! 这些**不是 ABI 槽位**:它们把一条命令拼出来交给 `Host::execute_command`。
//! 放在这里而不是让调用方自己拼，是因为拼错的地方（维度选择器、体积上限、
//! 名字字符集）都有明确的失败模式，而症状离根因很远。

use crate::rt::error::{Error, Result};
use crate::types::PositionI32;
use crate::world::World;

/// 一个整数格的长方体,`(min, max)`。
pub type Box3D = (PositionI32, PositionI32);

/// 一条 `/fill` 命令的体积上限。
///
/// 引擎自己的上限是 32768 格,超了整条命令直接失败 —— 不是少填几格,
/// 是一格都不填。
pub const MAX_FILL_VOLUME: i64 = 32_768;

/// 把一个长方体按 y 切成若干条,每条都在 [`MAX_FILL_VOLUME`] 以内。
///
/// 按 y 切而不是按最长边:一条 `/fill` 的代价主要在它跨了多少个区块,
/// 而同一根 y 柱上的格子必然在同一个区块里。
pub fn split_box(from: PositionI32, to: PositionI32) -> Vec<Box3D> {
    let (x0, x1) = (from.0.min(to.0), from.0.max(to.0));
    let (y0, y1) = (from.1.min(to.1), from.1.max(to.1));
    let (z0, z1) = (from.2.min(to.2), from.2.max(to.2));
    let area = (x1 - x0 + 1) as i64 * (z1 - z0 + 1) as i64;
    if area <= 0 {
        return Vec::new();
    }
    // 一层的面积就已经超上限时,per 会被钳到 1:那时每条命令只有一层,
    // 仍然可能超,由引擎自己拒绝。这里不假装能替它切开。
    let per = (MAX_FILL_VOLUME / area).max(1);
    let mut out = Vec::new();
    let mut y = y0 as i64;
    while y <= y1 as i64 {
        let top = (y + per - 1).min(y1 as i64);
        out.push(((x0, y as i32, z0), (x1, top as i32, z1)));
        y = top + 1;
    }
    out
}

/// 常加载区块的名字是不是合法的。
///
/// 引擎只收 `A-Z a-z 0-9 _`。带空格或中文的名字会让 `/tickingarea add`
/// 把名字当成下一个参数来解析,报出来的错和名字毫无关系。
pub fn is_valid_ticking_area_name(name: &str) -> bool {
    !name.is_empty() && name.chars().all(|c| c.is_ascii_alphanumeric() || c == '_')
}

impl World {
    /// 用 `/fill` 填一块区域,自动切成符合体积上限的若干条。
    ///
    /// 返回执行了多少条命令。中途失败就**停在那里**并返回 `Err`,不继续 ——
    /// 继续下去会得到一片填了一半的区域,而调用方从返回值里看不出填到哪了。
    pub fn fill_blocks(
        &self,
        dim: i32,
        from: PositionI32,
        to: PositionI32,
        block: &str,
    ) -> Result<usize> {
        let sel = dim_sel(dim)?;
        let host = crate::Host::get();
        let mut n = 0;
        for (a, b) in split_box(from, to) {
            let out = host.execute_command(&format!(
                "execute in {sel} run fill {} {} {} {} {} {} {block}",
                a.0, a.1, a.2, b.0, b.1, b.2
            ))?;
            n += 1;
            // execute_command 的 Ok 只表示命令跑完了,不表示它成功。
            if out.contains("Syntax error") || out.contains("Unknown command") {
                return Err(Error(format!(
                    "第 {n} 条 fill 被引擎拒绝（方块名 {block} 不认识?）:{out}"
                )));
            }
        }
        Ok(n)
    }

    /// 建一个常加载区块。
    ///
    /// 常加载区块是**存档级**的,活过重启,也不属于任何模组 —— 所以模组卸载
    /// 时不会自动撤掉,要自己 [`World::remove_ticking_area`]。
    pub fn add_ticking_area(
        &self,
        dim: i32,
        from: (i32, i32),
        to: (i32, i32),
        name: &str,
    ) -> Result<()> {
        if !is_valid_ticking_area_name(name) {
            return Err(Error(format!(
                "常加载区块名 `{name}` 不合法:只能用 A-Z a-z 0-9 和下划线"
            )));
        }
        let sel = dim_sel(dim)?;
        let out = crate::Host::get().execute_command(&format!(
            "execute in {sel} run tickingarea add {} 0 {} {} 0 {} {name}",
            from.0, from.1, to.0, to.1
        ))?;
        if out.contains("error") || out.contains("Unknown") {
            return Err(Error(format!("常加载区块 `{name}` 没能建立:{out}")));
        }
        Ok(())
    }

    pub fn remove_ticking_area(&self, dim: i32, name: &str) -> Result<()> {
        let sel = dim_sel(dim)?;
        let out = crate::Host::get()
            .execute_command(&format!("execute in {sel} run tickingarea remove {name}"))?;
        if out.contains("error") || out.contains("Unknown") {
            return Err(Error(format!("常加载区块 `{name}` 没能撤掉:{out}")));
        }
        Ok(())
    }

    /// 列出一个维度里的常加载区块名。
    ///
    /// 引擎的输出是一段给人看的文本,这里按逗号和空白拆。**格式跟着版本走**,
    /// 拆不出来时返回空表而不是报错 —— 但那时日志里有原文可查。
    pub fn list_ticking_areas(&self, dim: i32) -> Result<Vec<String>> {
        let sel = dim_sel(dim)?;
        let out = crate::Host::get()
            .execute_command(&format!("execute in {sel} run tickingarea list"))?;
        let names: Vec<String> = out
            .split([',', '\n', ' ', '\t'])
            .map(|s| s.trim())
            .filter(|s| is_valid_ticking_area_name(s))
            .map(|s| s.to_owned())
            .collect();
        if names.is_empty() && !out.trim().is_empty() {
            crate::Logger::get().warn(&format!(
                "tickingarea list 的输出里一个名字都没拆出来,引擎输出格式可能变了:{out}"
            ));
        }
        Ok(names)
    }
}

fn dim_sel(dim: i32) -> Result<String> {
    crate::sel::dimension_selector(dim).ok_or_else(|| {
        Error(format!(
            "维度 {dim} 没有命令选择器:id 为负,或者它没有登记过（见 dimensions::list）"
        ))
    })
}
