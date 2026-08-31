//! 物品 —— 一个**值对象**，不是一个句柄。
//!
//! ABI 上物品全程是一串 SNBT：读属性是「拿这串 SNBT 问一个属性」，改属性是
//! 「拿这串 SNBT 换一串新的」（`item_transform`）。这一层照搬这个形状，
//! 没有在中间藏一个指针。
//!
//! # 后果：`ItemStack` 和世界里那件物品**没有连接**
//!
//! `container.item(0)` 拿到的是一份快照。改它不会动容器里的那一件，要写回去
//! 得显式 `container.set_item(0, &stack)`。这一点和「句柄是身份不是指针」
//! 是同一条规矩的两面：中间没有隐式同步，也就没有「我改了怎么没生效」。

use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, s};
use crate::sys;

/// 一件物品的 SNBT 快照。
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct ItemStack {
    snbt: String,
}

/// 一条附魔。
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Enchant {
    /// 附魔 id。宿主报的是数字 id 还是名字取决于 BDS 版本，原样带过来。
    pub id: String,
    pub level: i32,
}

impl ItemStack {
    /// 直接用一串 SNBT。**不校验** —— 校验要过一次 ABI，而这个构造在热路径上。
    /// 形状不对会在第一次真正使用它的调用上报错。
    pub fn from_snbt(snbt: impl Into<String>) -> ItemStack {
        ItemStack { snbt: snbt.into() }
    }

    /// 按类型名和数量造一件。
    ///
    /// 拼的是最小形状 `{Name:"…",Count:Nb}`；引擎在 `ItemStack::fromTag`
    /// 里补齐其余字段。名字不存在时失败发生在**使用**这件物品的那一刻，
    /// 而不是这里。
    pub fn create(type_name: &str, count: u8) -> ItemStack {
        // 走 `NbtValue::to_snbt` 而不是 `format!`：转义规则（引号、反斜杠、
        // 控制字符）只该有一份实现，而那一份在 `nbt::write` 里，已经被
        // 对面的 `CompoundTag::fromSnbt` 检验过。
        //
        // Count 在 NBT 里是 byte。`u8` 超过 127 的部分表达不了，钳住而不是
        // 让它绕成负数 —— 一个负的堆叠数会被引擎当成空槽。
        let v = NbtValue::obj([
            ("Name", NbtValue::from(type_name)),
            ("Count", NbtValue::Byte(count.min(127) as i8)),
        ]);
        ItemStack { snbt: v.to_snbt() }
    }

    /// 空气。容器里的空槽读出来就是它。
    pub fn empty() -> ItemStack {
        ItemStack::create("minecraft:air", 0)
    }

    /// 底层 SNBT。
    pub fn snbt(&self) -> &str {
        &self.snbt
    }

    /// 解析成 NBT 树。要读 ABI 没给具名访问器的字段时用它。
    pub fn to_nbt(&self) -> Result<NbtValue> {
        NbtValue::parse(&self.snbt).map_err(|e| Error(format!("物品 SNBT 解析失败：{e}")))
    }

    // ── 数值属性 ──────────────────────────────────────────────

    /// 读一个 `PIER_IPROP_*` 数值属性。
    ///
    /// 宿主不认识的属性号返回 `Err` 而不是 0 —— 「问不出来」和「答案是 0」
    /// 必须分开（契约 §5.2）。
    pub fn num(&self, prop: i32) -> Result<f64> {
        let f = crate::require_slot!(item_get_num, "读取物品数值属性");
        let mut out = 0.0f64;
        let ok = unsafe { f(s(&self.snbt), prop, &mut out) };
        if ok {
            Ok(out)
        } else {
            Err(Error(format!(
                "宿主读不出物品属性 {prop}（属性号不认识，或这串 SNBT 不是合法物品）"
            )))
        }
    }

    fn num_i32(&self, prop: i32) -> Result<i32> {
        self.num(prop).map(|v| v as i32)
    }

    fn num_bool(&self, prop: i32) -> Result<bool> {
        self.num(prop).map(|v| v != 0.0)
    }

    pub fn count(&self) -> Result<u8> {
        self.num(sys::PIER_IPROP_COUNT).map(|v| v.clamp(0.0, 255.0) as u8)
    }
    pub fn max_stack_size(&self) -> Result<u8> {
        self.num(sys::PIER_IPROP_MAX_STACK_SIZE)
            .map(|v| v.clamp(0.0, 255.0) as u8)
    }
    pub fn aux_value(&self) -> Result<i32> {
        self.num_i32(sys::PIER_IPROP_AUX_VALUE)
    }
    pub fn id(&self) -> Result<i32> {
        self.num_i32(sys::PIER_IPROP_ID)
    }
    pub fn damage(&self) -> Result<i32> {
        self.num_i32(sys::PIER_IPROP_DAMAGE)
    }
    pub fn max_damage(&self) -> Result<i32> {
        self.num_i32(sys::PIER_IPROP_MAX_DAMAGE)
    }
    pub fn attack_damage(&self) -> Result<i32> {
        self.num_i32(sys::PIER_IPROP_ATTACK_DAMAGE)
    }
    pub fn repair_cost(&self) -> Result<i32> {
        self.num_i32(sys::PIER_IPROP_REPAIR_COST)
    }
    pub fn enchant_value(&self) -> Result<i32> {
        self.num_i32(sys::PIER_IPROP_ENCHANT_VALUE)
    }
    pub fn use_duration(&self) -> Result<i32> {
        self.num_i32(sys::PIER_IPROP_USE_DURATION)
    }
    pub fn is_null(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_NULL)
    }
    pub fn is_block(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_BLOCK)
    }
    pub fn is_enchanted(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_ENCHANTED)
    }
    pub fn is_armor(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_ARMOR)
    }
    pub fn is_damageable(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_DAMAGEABLE)
    }
    pub fn is_damaged(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_DAMAGED)
    }
    pub fn is_unbreakable(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_UNBREAKABLE)
    }
    pub fn has_durability(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_HAS_DURABILITY)
    }
    pub fn is_potion(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_POTION)
    }
    pub fn is_throwable(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_THROWABLE)
    }
    pub fn is_fire_resistant(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_FIRE_RESISTANT)
    }
    pub fn is_stackable(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_STACKABLE)
    }
    pub fn is_music_disc(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_MUSIC_DISC)
    }
    pub fn is_offhand(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_OFFHAND)
    }
    pub fn is_glint(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_GLINT)
    }
    pub fn is_bundle(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_BUNDLE)
    }
    pub fn has_user_data(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_HAS_USER_DATA)
    }
    pub fn has_custom_name(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_HAS_CUSTOM_NAME)
    }

    // ── 字符串属性 ────────────────────────────────────────────

    /// 读一个 `PIER_ISTR_*` 字符串属性。
    pub fn text(&self, prop: i32) -> Result<String> {
        let f = crate::require_slot!(item_get_str, "读取物品字符串属性");
        call_out_str(|ctx, sink| unsafe { f(s(&self.snbt), prop, ctx, sink) })
            .ok_or_else(|| Error(format!("宿主读不出物品字符串属性 {prop}")))
    }

    pub fn type_name(&self) -> Result<String> {
        self.text(sys::PIER_ISTR_TYPE_NAME)
    }
    pub fn name(&self) -> Result<String> {
        self.text(sys::PIER_ISTR_NAME)
    }
    pub fn custom_name(&self) -> Result<String> {
        self.text(sys::PIER_ISTR_CUSTOM_NAME)
    }
    pub fn hover_name(&self) -> Result<String> {
        self.text(sys::PIER_ISTR_HOVER_NAME)
    }
    pub fn raw_name_id(&self) -> Result<String> {
        self.text(sys::PIER_ISTR_RAW_NAME_ID)
    }
    pub fn effect_name(&self) -> Result<String> {
        self.text(sys::PIER_ISTR_EFFECT_NAME)
    }

    /// 自定义 lore。宿主给的是 SNBT 字符串列表，这里解析成 `Vec<String>`。
    pub fn lore(&self) -> Result<Vec<String>> {
        parse_str_list(&self.text(sys::PIER_ISTR_LORE)?, "lore")
    }

    pub fn can_destroy(&self) -> Result<Vec<String>> {
        parse_str_list(&self.text(sys::PIER_ISTR_CAN_DESTROY)?, "can_destroy")
    }

    pub fn can_place_on(&self) -> Result<Vec<String>> {
        parse_str_list(&self.text(sys::PIER_ISTR_CAN_PLACE_ON)?, "can_place_on")
    }

    /// 物品的自定义 NBT（`tag` 段）。走专用槽而不是 `PIER_ISTR_USER_DATA`：
    /// 两者内容相同，专用槽在宿主侧少一次属性号分发。
    pub fn user_data(&self) -> Result<NbtValue> {
        let f = crate::require_slot!(item_get_user_data, "读取物品自定义 NBT");
        let text = call_out_str(|ctx, sink| unsafe { f(s(&self.snbt), ctx, sink) })
            .ok_or_else(|| Error("宿主读不出这件物品的自定义 NBT".to_owned()))?;
        NbtValue::parse(&text).map_err(|e| Error(format!("物品自定义 NBT 解析失败：{e}")))
    }

    /// 颜色（`{r,g,b}`）。只有染色类物品有。
    pub fn color(&self) -> Result<(i32, i32, i32)> {
        let text = self.text(sys::PIER_ISTR_COLOR)?;
        let v = NbtValue::parse(&text).map_err(|e| Error(format!("颜色 SNBT 解析失败：{e}")))?;
        Ok((v.get_i32("r")?, v.get_i32("g")?, v.get_i32("b")?))
    }

    // ── 变换 ──────────────────────────────────────────────────

    /// 跑一次 `PIER_IOP_*` 变换，把自己换成结果。
    ///
    /// 失败时**自己不变**：宿主没产出新 SNBT 就没有可写回的东西，把半个结果
    /// 写进来比什么都不做更难查。
    pub fn transform(&mut self, op: i32, sarg: &str, narg: f64) -> Result<()> {
        let f = crate::require_slot!(item_transform, "变换物品");
        let out = call_out_str(|ctx, sink| unsafe {
            f(s(&self.snbt), op, s(sarg), narg, ctx, sink)
        })
        .ok_or_else(|| Error(format!("物品变换 {op} 失败（操作号不认识，或参数不合法）")))?;
        self.snbt = out;
        Ok(())
    }

    /// 同上，但返回新的一件，自己保持不变。
    pub fn transformed(&self, op: i32, sarg: &str, narg: f64) -> Result<ItemStack> {
        let mut copy = self.clone();
        copy.transform(op, sarg, narg)?;
        Ok(copy)
    }

    pub fn set_custom_name(&mut self, name: &str) -> Result<()> {
        self.transform(sys::PIER_IOP_SET_CUSTOM_NAME, name, 0.0)
    }
    pub fn reset_name(&mut self) -> Result<()> {
        self.transform(sys::PIER_IOP_RESET_NAME, "", 0.0)
    }
    pub fn set_damage(&mut self, damage: i32) -> Result<()> {
        self.transform(sys::PIER_IOP_SET_DAMAGE, "", damage as f64)
    }
    pub fn set_count(&mut self, count: u8) -> Result<()> {
        self.transform(sys::PIER_IOP_SET_COUNT, "", count as f64)
    }
    pub fn set_unbreakable(&mut self, on: bool) -> Result<()> {
        self.transform(sys::PIER_IOP_SET_UNBREAKABLE, "", if on { 1.0 } else { 0.0 })
    }
    pub fn hurt_and_break(&mut self, damage: i32) -> Result<()> {
        self.transform(sys::PIER_IOP_HURT_AND_BREAK, "", damage as f64)
    }
    pub fn set_repair_cost(&mut self, cost: i32) -> Result<()> {
        self.transform(sys::PIER_IOP_SET_REPAIR_COST, "", cost as f64)
    }
    pub fn clear_lore(&mut self) -> Result<()> {
        self.transform(sys::PIER_IOP_CLEAR_LORE, "", 0.0)
    }
    pub fn remove_enchants(&mut self) -> Result<()> {
        self.transform(sys::PIER_IOP_REMOVE_ENCHANTS, "", 0.0)
    }

    pub fn set_lore(&mut self, lines: &[&str]) -> Result<()> {
        self.transform(sys::PIER_IOP_SET_LORE, &str_list_snbt(lines), 0.0)
    }
    pub fn set_can_destroy(&mut self, blocks: &[&str]) -> Result<()> {
        self.transform(sys::PIER_IOP_SET_CAN_DESTROY, &str_list_snbt(blocks), 0.0)
    }
    pub fn set_can_place_on(&mut self, blocks: &[&str]) -> Result<()> {
        self.transform(sys::PIER_IOP_SET_CAN_PLACE_ON, &str_list_snbt(blocks), 0.0)
    }

    /// 加一条附魔。等级写 0 在引擎里等于移除那一条。
    pub fn add_enchant(&mut self, id: &str, level: i32) -> Result<()> {
        self.transform(sys::PIER_IOP_ADD_ENCHANT, &format!("{id}:{level}"), 0.0)
    }

    // ── 附魔与比较 ────────────────────────────────────────────

    pub fn enchants(&self) -> Result<Vec<Enchant>> {
        let f = crate::require_slot!(item_get_enchants, "读取物品附魔");
        let text = call_out_str(|ctx, sink| unsafe { f(s(&self.snbt), ctx, sink) })
            .ok_or_else(|| Error("宿主读不出这件物品的附魔".to_owned()))?;
        let v = NbtValue::parse(&text).map_err(|e| Error(format!("附魔 SNBT 解析失败：{e}")))?;
        let Some(items) = v.as_list() else {
            return Err(Error(format!("附魔 SNBT 不是列表，而是 {}", v.type_name())));
        };
        Ok(items
            .iter()
            .filter_map(|e| {
                let id = match e.get("id") {
                    // id 可能是数字也可能是字符串，两种都收下。
                    Some(NbtValue::String(s)) => s.clone(),
                    Some(other) => other.as_i64()?.to_string(),
                    None => return None,
                };
                Some(Enchant {
                    id,
                    level: e.opt_i32("level").unwrap_or(1),
                })
            })
            .collect())
    }

    /// 整套换掉附魔，返回新的一件。
    pub fn with_enchants(&self, enchants: &[Enchant]) -> Result<ItemStack> {
        let f = crate::require_slot!(item_set_enchants, "写入物品附魔");
        let list = NbtValue::list(enchants.iter().map(|e| {
            NbtValue::obj([
                ("id", NbtValue::from(e.id.as_str())),
                ("level", NbtValue::Int(e.level)),
            ])
        }))
        .to_snbt();
        let out = call_out_str(|ctx, sink| unsafe { f(s(&self.snbt), s(&list), ctx, sink) })
            .ok_or_else(|| Error("宿主拒绝写入附魔（附魔名不认识，或物品不可附魔）".to_owned()))?;
        Ok(ItemStack { snbt: out })
    }

    /// 两件是不是「同一种东西」。
    ///
    /// 判据由引擎给（`ItemStack::matches`），**不是**字符串相等：数量、
    /// 耐久这类字段不参与，而 SNBT 文本比较会把它们算进去。
    /// 槽位缺席时返回 `Err`，不退回文本比较 —— 那会让判据在不同宿主上不一样。
    pub fn matches(&self, other: &ItemStack) -> Result<bool> {
        let f = crate::require_slot!(item_matches, "比较两件物品");
        Ok(unsafe { f(s(&self.snbt), s(&other.snbt)) })
    }
}

impl std::fmt::Display for ItemStack {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(&self.snbt)
    }
}

impl From<&str> for ItemStack {
    fn from(v: &str) -> ItemStack {
        ItemStack::from_snbt(v)
    }
}

impl From<String> for ItemStack {
    fn from(v: String) -> ItemStack {
        ItemStack::from_snbt(v)
    }
}

/// 拼一个 SNBT 字符串列表。
pub(crate) fn str_list_snbt(items: &[&str]) -> String {
    NbtValue::list(items.iter().map(|s| NbtValue::from(*s))).to_snbt()
}

/// 解析一个 SNBT 字符串列表。
///
/// 空串按空列表处理：宿主对「这件物品没有 lore」就是 sink 一个空串，
/// 而 `NbtValue::parse("")` 会失败 —— 把「没有」报成解析错误是假警报。
pub(crate) fn parse_str_list(text: &str, what: &str) -> Result<Vec<String>> {
    if text.trim().is_empty() {
        return Ok(Vec::new());
    }
    let v = NbtValue::parse(text).map_err(|e| Error(format!("{what} 的 SNBT 解析失败：{e}")))?;
    let Some(items) = v.as_list() else {
        return Err(Error(format!("{what} 不是列表，而是 {}", v.type_name())));
    };
    Ok(items
        .iter()
        .map(|e| match e.as_str() {
            Some(s) => s.to_owned(),
            None => e.to_snbt(),
        })
        .collect())
}
