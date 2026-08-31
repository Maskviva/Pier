//! NBT 值：跨边界一切结构化数据的载体。
//!
//! Pier 的 ABI 上没有结构体传参 —— 事件载荷、表单、物品、方块状态、实体快照、
//! 命令参数、服务请求与应答，全都是 **SNBT 字符串**。所以「怎么从一段 SNBT
//! 里取出一个字段」是模组代码里频率最高的一件事。
//!
//! 两套取值，语义不同：
//!
//! * `opt_*` 返回 `Option` —— 「有就给我，没有我自己兜底」。
//! * `get_*` 返回 `Result` —— **缺键和类型不符是两种不同的错误**，信息里带
//!   键名和实际类型。安全判定（权限、地皮、经济）用这一套，拿到 `Err` 就
//!   fail-closed。压成一个答案的后果见 [`crate::event`]。

pub mod binary;
mod parse;
mod write;

pub use binary::NbtFormat;
pub use parse::ParseError;

use std::collections::BTreeMap;
use std::fmt;

/// 一个 NBT 值。变体与 Bedrock 的标签类型一一对应。
///
/// 变体集合与上一代保持一致，既有代码原样搬过来即可编译。
#[derive(Debug, Clone, PartialEq)]
pub enum NbtValue {
    Byte(i8),
    Short(i16),
    Int(i32),
    Long(i64),
    Float(f32),
    Double(f64),
    String(String),
    List(Vec<NbtValue>),
    Compound(BTreeMap<String, NbtValue>),
    ByteArray(Vec<i8>),
    IntArray(Vec<i32>),
    LongArray(Vec<i64>),
}

/// 取值失败的原因。**缺键和类型不符是两件事**，不要把它们都压成 `None`。
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum NbtError {
    /// 这个键根本不在。
    Missing { path: String },
    /// 键在，但类型不是要的那个。
    WrongType {
        path: String,
        want: &'static str,
        got: &'static str,
    },
    /// 在一个不是复合标签的东西上取键，或者在不是列表的东西上取下标。
    NotIndexable { path: String, got: &'static str },
}

impl fmt::Display for NbtError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            NbtError::Missing { path } => write!(f, "载荷里没有 `{path}`"),
            NbtError::WrongType { path, want, got } => {
                write!(f, "`{path}` 是 {got}，要的是 {want}")
            }
            NbtError::NotIndexable { path, got } => {
                write!(f, "`{path}` 是 {got}，没法在它上面按键/下标取值")
            }
        }
    }
}

impl std::error::Error for NbtError {}

impl From<NbtError> for crate::Error {
    fn from(e: NbtError) -> Self {
        crate::Error(e.to_string())
    }
}

/// 取值结果。
pub type NbtResult<T> = std::result::Result<T, NbtError>;

impl NbtValue {
    /* ───────────────────────── 构造 ───────────────────────── */

    /// 空复合标签。
    pub fn compound() -> NbtValue {
        NbtValue::Compound(BTreeMap::new())
    }

    /// 从键值对直接建一个复合标签。
    ///
    /// ```ignore
    /// let v = NbtValue::obj([
    ///     ("x", 10.into()),
    ///     ("name", "stone".into()),
    /// ]);
    /// ```
    pub fn obj<K, I>(entries: I) -> NbtValue
    where
        K: Into<String>,
        I: IntoIterator<Item = (K, NbtValue)>,
    {
        NbtValue::Compound(entries.into_iter().map(|(k, v)| (k.into(), v)).collect())
    }

    /// 从一串值建列表。
    pub fn list<I: IntoIterator<Item = NbtValue>>(items: I) -> NbtValue {
        NbtValue::List(items.into_iter().collect())
    }

    /// `[x, y, z]` 形状的双精度列表 —— 坐标在载荷里就是这么表示的。
    pub fn vec3(x: f64, y: f64, z: f64) -> NbtValue {
        NbtValue::List(vec![
            NbtValue::Double(x),
            NbtValue::Double(y),
            NbtValue::Double(z),
        ])
    }

    /// 解析一段 SNBT。
    pub fn parse(text: &str) -> std::result::Result<NbtValue, ParseError> {
        parse::parse(text)
    }

    /// 序列化成 SNBT。浮点一定带 `d`/`f` 后缀、字节带 `b`、长整带 `L` ——
    /// 少了后缀对面会把 `100.0` 读成 Int，那正是「明明写了 double 却读不出来」
    /// 这类问题的来源。
    pub fn to_snbt(&self) -> String {
        let mut out = String::new();
        write::write(self, &mut out);
        out
    }

    /* ───────────────────────── 类型名（给错误信息用） ───────────────────────── */

    /// 这个值的类型名，只用于错误信息。
    pub fn type_name(&self) -> &'static str {
        match self {
            NbtValue::Byte(_) => "byte",
            NbtValue::Short(_) => "short",
            NbtValue::Int(_) => "int",
            NbtValue::Long(_) => "long",
            NbtValue::Float(_) => "float",
            NbtValue::Double(_) => "double",
            NbtValue::String(_) => "string",
            NbtValue::List(_) => "list",
            NbtValue::Compound(_) => "compound",
            NbtValue::ByteArray(_) => "byte[]",
            NbtValue::IntArray(_) => "int[]",
            NbtValue::LongArray(_) => "long[]",
        }
    }

    /* ───────────────────────── 宽松取值（Option） ───────────────────────── */

    /// 任意整数型 → i64。浮点**不**参与转换：`3.7` 静默变成 `3` 是 bug 的温床。
    pub fn as_i64(&self) -> Option<i64> {
        match self {
            NbtValue::Byte(v) => Some(*v as i64),
            NbtValue::Short(v) => Some(*v as i64),
            NbtValue::Int(v) => Some(*v as i64),
            NbtValue::Long(v) => Some(*v),
            _ => None,
        }
    }

    /// 同上，但收窄到 i32（超出范围返回 `None`，不截断）。
    pub fn as_i32(&self) -> Option<i32> {
        self.as_i64().and_then(|v| i32::try_from(v).ok())
    }

    /// 任意数值型 → f64（整数也算）。
    pub fn as_f64(&self) -> Option<f64> {
        match self {
            NbtValue::Float(v) => Some(*v as f64),
            NbtValue::Double(v) => Some(*v),
            _ => self.as_i64().map(|v| v as f64),
        }
    }

    /// 布尔。SNBT 里布尔就是 `1b`/`0b`，所以任意整数型都认；非零为真。
    pub fn as_bool(&self) -> Option<bool> {
        self.as_i64().map(|v| v != 0)
    }

    pub fn as_str(&self) -> Option<&str> {
        match self {
            NbtValue::String(s) => Some(s.as_str()),
            _ => None,
        }
    }

    pub fn as_list(&self) -> Option<&[NbtValue]> {
        match self {
            NbtValue::List(v) => Some(v.as_slice()),
            _ => None,
        }
    }

    pub fn as_compound(&self) -> Option<&BTreeMap<String, NbtValue>> {
        match self {
            NbtValue::Compound(m) => Some(m),
            _ => None,
        }
    }

    pub fn as_compound_mut(&mut self) -> Option<&mut BTreeMap<String, NbtValue>> {
        match self {
            NbtValue::Compound(m) => Some(m),
            _ => None,
        }
    }

    /// `[x, y, z]` 列表 → 三元组。宿主发坐标用的就是这个形状
    /// （`edit_trace_ray` 的 `pos`/`block`、`actor_get_aabb` 的 `min`/`max`…）。
    pub fn as_vec3(&self) -> Option<(f64, f64, f64)> {
        let l = self.as_list()?;
        if l.len() != 3 {
            return None;
        }
        Some((l[0].as_f64()?, l[1].as_f64()?, l[2].as_f64()?))
    }

    /// 同上但取整（用于方块坐标）。
    pub fn as_block_pos(&self) -> Option<(i32, i32, i32)> {
        let l = self.as_list()?;
        if l.len() != 3 {
            return None;
        }
        Some((l[0].as_i32()?, l[1].as_i32()?, l[2].as_i32()?))
    }

    pub fn is_compound(&self) -> bool {
        matches!(self, NbtValue::Compound(_))
    }

    pub fn is_list(&self) -> bool {
        matches!(self, NbtValue::List(_))
    }

    /* ───────────────────────── 键 / 下标 / 路径 ───────────────────────── */

    /// 取一个直接子键。
    pub fn get(&self, key: &str) -> Option<&NbtValue> {
        self.as_compound()?.get(key)
    }

    pub fn get_mut(&mut self, key: &str) -> Option<&mut NbtValue> {
        self.as_compound_mut()?.get_mut(key)
    }

    /// 取列表的第 i 项。
    pub fn index(&self, i: usize) -> Option<&NbtValue> {
        self.as_list()?.get(i)
    }

    /// 写一个键；自身不是复合标签时返回 false（不会把它悄悄变成复合标签）。
    pub fn insert(&mut self, key: impl Into<String>, value: NbtValue) -> bool {
        match self.as_compound_mut() {
            Some(m) => {
                m.insert(key.into(), value);
                true
            }
            None => false,
        }
    }

    pub fn remove(&mut self, key: &str) -> Option<NbtValue> {
        self.as_compound_mut()?.remove(key)
    }

    pub fn contains(&self, key: &str) -> bool {
        self.get(key).is_some()
    }

    /// 点分路径，支持数组下标：`"a.b[2].c"`。
    ///
    /// 上一代只支持纯点分，于是「取 aabb 的 min 的 y」得写三行。
    pub fn path(&self, dotted: &str) -> Option<&NbtValue> {
        let mut cur = self;
        for seg in dotted.split('.') {
            if seg.is_empty() {
                continue;
            }
            let (name, idxs) = split_indices(seg);
            if !name.is_empty() {
                cur = cur.get(name)?;
            }
            for i in idxs {
                cur = cur.index(i)?;
            }
        }
        Some(cur)
    }

    /* ───────────────────────── 严格取值（Result） ───────────────────────── */

    /// 按路径取值；缺了就是 `Missing`，带着完整路径名。
    pub fn require(&self, path: &str) -> NbtResult<&NbtValue> {
        match self.path(path) {
            Some(v) => Ok(v),
            None => {
                // 分辨「中途撞上一个不可索引的东西」和「单纯没这个键」，
                // 前者多半是形状变了，值得单独说。
                if let Some(parent) = parent_of(path) {
                    if let Some(p) = self.path(parent) {
                        if !p.is_compound() && !p.is_list() {
                            return Err(NbtError::NotIndexable {
                                path: parent.to_owned(),
                                got: p.type_name(),
                            });
                        }
                    }
                }
                Err(NbtError::Missing {
                    path: path.to_owned(),
                })
            }
        }
    }

    /// `'a` 必须显式写出来：省略掉的话 `f` 会被脱糖成 `for<'x>`，于是
    /// `T` 得独立于 `'x`，`as_str` / `as_list` 这种「返回值借自入参」的
    /// 访问器就对不上（E0308「one type is more general than the other」）。
    /// 绑到 `&'a self` 上之后，`T` 可以是 `&'a str`。
    fn typed<'a, T>(
        &'a self,
        path: &str,
        want: &'static str,
        f: impl FnOnce(&'a NbtValue) -> Option<T>,
    ) -> NbtResult<T> {
        let v = self.require(path)?;
        f(v).ok_or_else(|| NbtError::WrongType {
            path: path.to_owned(),
            want,
            got: v.type_name(),
        })
    }

    /// 取整数。缺键 → `Missing`；类型不符 → `WrongType`。**不会给你 0。**
    pub fn get_i64(&self, path: &str) -> NbtResult<i64> {
        self.typed(path, "整数", NbtValue::as_i64)
    }

    pub fn get_i32(&self, path: &str) -> NbtResult<i32> {
        self.typed(path, "int", NbtValue::as_i32)
    }

    pub fn get_f64(&self, path: &str) -> NbtResult<f64> {
        self.typed(path, "数值", NbtValue::as_f64)
    }

    pub fn get_bool(&self, path: &str) -> NbtResult<bool> {
        self.typed(path, "布尔", NbtValue::as_bool)
    }

    pub fn get_str(&self, path: &str) -> NbtResult<&str> {
        self.typed(path, "字符串", NbtValue::as_str)
    }

    pub fn get_list(&self, path: &str) -> NbtResult<&[NbtValue]> {
        self.typed(path, "列表", NbtValue::as_list)
    }

    pub fn get_vec3(&self, path: &str) -> NbtResult<(f64, f64, f64)> {
        self.typed(path, "[x,y,z]", NbtValue::as_vec3)
    }

    pub fn get_block_pos(&self, path: &str) -> NbtResult<(i32, i32, i32)> {
        self.typed(path, "[x,y,z] 整数", NbtValue::as_block_pos)
    }

    /* ───────────────────────── 宽松取值（带路径） ───────────────────────── */

    pub fn opt_i64(&self, path: &str) -> Option<i64> {
        self.path(path)?.as_i64()
    }
    pub fn opt_i32(&self, path: &str) -> Option<i32> {
        self.path(path)?.as_i32()
    }
    pub fn opt_f64(&self, path: &str) -> Option<f64> {
        self.path(path)?.as_f64()
    }
    pub fn opt_bool(&self, path: &str) -> Option<bool> {
        self.path(path)?.as_bool()
    }
    pub fn opt_str(&self, path: &str) -> Option<&str> {
        self.path(path)?.as_str()
    }

    /// 按顺序试几个路径，返回第一个存在的字符串。
    ///
    /// 这个方法存在的理由很具体：同一个概念在不同事件里键名不统一
    /// （`player` / `_player.name` / `name`），业务侧本来就要写这个循环。
    pub fn first_str(&self, paths: &[&str]) -> Option<&str> {
        paths.iter().find_map(|p| self.opt_str(p))
    }

    /* ───────────────────────── serde_json 桥 ───────────────────────── */

    /// 转成 `serde_json::Value`。类型后缀信息会丢（JSON 没有 byte/long 之分），
    /// 所以这条路适合「存档、日志、发给 Web」，不适合再转回来喂给宿主。
    pub fn to_json(&self) -> serde_json::Value {
        use serde_json::Value as J;
        match self {
            NbtValue::Byte(v) => J::from(*v),
            NbtValue::Short(v) => J::from(*v),
            NbtValue::Int(v) => J::from(*v),
            NbtValue::Long(v) => J::from(*v),
            NbtValue::Float(v) => serde_json::Number::from_f64(*v as f64)
                .map(J::Number)
                .unwrap_or(J::Null),
            NbtValue::Double(v) => serde_json::Number::from_f64(*v)
                .map(J::Number)
                .unwrap_or(J::Null),
            NbtValue::String(s) => J::String(s.clone()),
            NbtValue::List(l) => J::Array(l.iter().map(NbtValue::to_json).collect()),
            NbtValue::Compound(m) => {
                J::Object(m.iter().map(|(k, v)| (k.clone(), v.to_json())).collect())
            }
            NbtValue::ByteArray(a) => J::Array(a.iter().map(|v| J::from(*v)).collect()),
            NbtValue::IntArray(a) => J::Array(a.iter().map(|v| J::from(*v)).collect()),
            NbtValue::LongArray(a) => J::Array(a.iter().map(|v| J::from(*v)).collect()),
        }
    }

    /// 从 `serde_json::Value` 转过来。整数落成 `Long`、浮点落成 `Double` ——
    /// 这是无损的选择：想要更窄的类型请自己构造。
    pub fn from_json(v: &serde_json::Value) -> NbtValue {
        use serde_json::Value as J;
        match v {
            J::Null => NbtValue::String(String::new()),
            J::Bool(b) => NbtValue::Byte(i8::from(*b)),
            J::Number(n) => {
                if let Some(i) = n.as_i64() {
                    NbtValue::Long(i)
                } else {
                    NbtValue::Double(n.as_f64().unwrap_or(0.0))
                }
            }
            J::String(s) => NbtValue::String(s.clone()),
            J::Array(a) => NbtValue::List(a.iter().map(NbtValue::from_json).collect()),
            J::Object(o) => NbtValue::Compound(
                o.iter()
                    .map(|(k, v)| (k.clone(), NbtValue::from_json(v)))
                    .collect(),
            ),
        }
    }
}

/* ───────────────────────── From 便利实现 ───────────────────────── */

macro_rules! nbt_from {
    ($($t:ty => $variant:ident),* $(,)?) => {
        $(impl From<$t> for NbtValue {
            fn from(v: $t) -> NbtValue { NbtValue::$variant(v) }
        })*
    };
}
nbt_from! {
    i8 => Byte, i16 => Short, i32 => Int, i64 => Long,
    f32 => Float, f64 => Double, String => String,
}

impl From<bool> for NbtValue {
    fn from(v: bool) -> NbtValue {
        NbtValue::Byte(i8::from(v))
    }
}

impl From<&str> for NbtValue {
    fn from(v: &str) -> NbtValue {
        NbtValue::String(v.to_owned())
    }
}

impl<T: Into<NbtValue>> From<Vec<T>> for NbtValue {
    fn from(v: Vec<T>) -> NbtValue {
        NbtValue::List(v.into_iter().map(Into::into).collect())
    }
}

impl fmt::Display for NbtValue {
    /// `{}` 直接给 SNBT —— 日志里最常要的就是它。
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(&self.to_snbt())
    }
}

/* ───────────────────────── 路径解析助手 ───────────────────────── */

/// 把 `name[1][2]` 拆成 `("name", [1, 2])`。
fn split_indices(seg: &str) -> (&str, Vec<usize>) {
    let Some(open) = seg.find('[') else {
        return (seg, Vec::new());
    };
    let name = &seg[..open];
    let mut idxs = Vec::new();
    let mut rest = &seg[open..];
    while let Some(close) = rest.find(']') {
        if let Ok(i) = rest[1..close].parse::<usize>() {
            idxs.push(i);
        }
        rest = &rest[close + 1..];
        if !rest.starts_with('[') {
            break;
        }
    }
    (name, idxs)
}

/// `"a.b.c"` → `Some("a.b")`；单段返回 `None`。
fn parent_of(path: &str) -> Option<&str> {
    path.rfind('.').map(|i| &path[..i])
}
