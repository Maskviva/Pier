//! NBT values: the carrier of all structured data across the boundary.
//!
//! No struct is passed on the Pier ABI. Event payloads, forms, items, block states, actor
//! snapshots, command arguments, service requests and replies are all SNBT strings, which
//! makes reading a field out of some SNBT the most frequent operation in mod code.
//!
//! Two families of accessors, with different meanings:
//!
//! * `opt_*` returns an `Option`: give it to me if it is there and I will handle the rest.
//! * `get_*` returns a `Result`, where a missing key and a type mismatch are two different
//!   errors and the message carries the key name and the actual type. A protection
//!   decision, for permissions, plots or economy, uses this family and fails closed on an
//!   `Err`. [`crate::event`] gives the consequence of collapsing them into one answer.

pub mod binary;
mod parse;
mod write;

pub use binary::NbtFormat;
pub use parse::ParseError;

use std::collections::BTreeMap;
use std::fmt;

/// One NBT value. The variants correspond one to one with the Bedrock tag types.
///
/// The variant set matches the earlier generation, so existing code compiles unchanged.
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

/// Why a read failed. A missing key and a type mismatch are two different things and must
/// not both collapse into `None`.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum NbtError {
    /// The key is simply not there.
    Missing { path: String },
    /// The key is there with a type other than the one requested.
    WrongType {
        path: String,
        want: &'static str,
        got: &'static str,
    },
    /// Reading a key on something that is not a compound tag, or an index on something that is
    /// not a list.
    NotIndexable { path: String, got: &'static str },
}

impl fmt::Display for NbtError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            NbtError::Missing { path } => write!(f, "the payload has no `{path}`"),
            NbtError::WrongType { path, want, got } => {
                write!(f, "`{path}` is {got} while {want} was requested")
            }
            NbtError::NotIndexable { path, got } => {
                write!(f, "`{path}` is {got}, which cannot be read by key or index")
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

/// The result of a read.
pub type NbtResult<T> = std::result::Result<T, NbtError>;

impl NbtValue {
    /* Construction */

    /// An empty compound tag.
    pub fn compound() -> NbtValue {
        NbtValue::Compound(BTreeMap::new())
    }

    /// Builds a compound tag straight from key-value pairs.
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

    /// Builds a list from a sequence of values.
    pub fn list<I: IntoIterator<Item = NbtValue>>(items: I) -> NbtValue {
        NbtValue::List(items.into_iter().collect())
    }

    /// A double list shaped `[x, y, z]`, which is how a coordinate appears in a payload.
    pub fn vec3(x: f64, y: f64, z: f64) -> NbtValue {
        NbtValue::List(vec![
            NbtValue::Double(x),
            NbtValue::Double(y),
            NbtValue::Double(z),
        ])
    }

    /// Parses a piece of SNBT.
    pub fn parse(text: &str) -> std::result::Result<NbtValue, ParseError> {
        parse::parse(text)
    }

    /// Serializes to SNBT. A float always carries a `d` or `f` suffix, a byte a `b` and a long
    /// an `L`. Without the suffix the other side reads `100.0` as an Int, which is the source
    /// of a double being written and then failing to read back.
    pub fn to_snbt(&self) -> String {
        let mut out = String::new();
        write::write(self, &mut out);
        out
    }

    /* Type names, used in error messages */

    /// The type name of this value, used only in error messages.
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

    /* Lenient reads returning an Option */

    /// Any integer type into an i64. A float does not convert, since `3.7` quietly becoming
    /// `3` breeds bugs.
    pub fn as_i64(&self) -> Option<i64> {
        match self {
            NbtValue::Byte(v) => Some(*v as i64),
            NbtValue::Short(v) => Some(*v as i64),
            NbtValue::Int(v) => Some(*v as i64),
            NbtValue::Long(v) => Some(*v),
            _ => None,
        }
    }

    /// As above, narrowed to an i32. Out of range returns `None` rather than truncating.
    pub fn as_i32(&self) -> Option<i32> {
        self.as_i64().and_then(|v| i32::try_from(v).ok())
    }

    /// Any numeric type into an f64, integers included.
    pub fn as_f64(&self) -> Option<f64> {
        match self {
            NbtValue::Float(v) => Some(*v as f64),
            NbtValue::Double(v) => Some(*v),
            _ => self.as_i64().map(|v| v as f64),
        }
    }

    /// A boolean. In SNBT a boolean is `1b` or `0b`, so any integer type is accepted and
    /// non-zero is true.
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

    /// An `[x, y, z]` list into a triple. This is the shape the host uses for coordinates, as
    /// in `pos` and `block` of `edit_trace_ray` and `min` and `max` of `actor_get_aabb`.
    pub fn as_vec3(&self) -> Option<(f64, f64, f64)> {
        let l = self.as_list()?;
        if l.len() != 3 {
            return None;
        }
        Some((l[0].as_f64()?, l[1].as_f64()?, l[2].as_f64()?))
    }

    /// As above but as integers, for a block coordinate.
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

    /* Keys, indices and paths */

    /// Reads a direct child key.
    pub fn get(&self, key: &str) -> Option<&NbtValue> {
        self.as_compound()?.get(key)
    }

    pub fn get_mut(&mut self, key: &str) -> Option<&mut NbtValue> {
        self.as_compound_mut()?.get_mut(key)
    }

    /// Reads item i of a list.
    pub fn index(&self, i: usize) -> Option<&NbtValue> {
        self.as_list()?.get(i)
    }

    /// Writes a key. It returns false when this is not a compound tag and never quietly turns
    /// it into one.
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

    /// A dotted path with array indices, such as `"a.b[2].c"`.
    ///
    /// An earlier generation supported plain dots only, so reading the y of the min of an aabb
    /// took three lines.
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

    /* Strict reads returning a Result */

    /// Reads by path. A missing value is a `Missing` carrying the full path name.
    pub fn require(&self, path: &str) -> NbtResult<&NbtValue> {
        match self.path(path) {
            Some(v) => Ok(v),
            None => {
                // Tells hitting something unindexable partway apart from simply having no such key.
                // The former usually means the shape changed and is worth saying separately.
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

    /// `'a` has to be written out: omitted, `f` desugars to `for<'x>`, `T` then has to be
    /// independent of `'x`, and an accessor whose result borrows from its argument, such as
    /// `as_str` or `as_list`, no longer matches, giving E0308 about one type being more
    /// general than the other.
    /// Bound to `&'a self`, `T` may be a `&'a str`.
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

    /// Reads an integer. A missing key is a `Missing` and a mismatched type a `WrongType`. It
    /// never gives back a 0.
    pub fn get_i64(&self, path: &str) -> NbtResult<i64> {
        self.typed(path, "an integer", NbtValue::as_i64)
    }

    pub fn get_i32(&self, path: &str) -> NbtResult<i32> {
        self.typed(path, "int", NbtValue::as_i32)
    }

    pub fn get_f64(&self, path: &str) -> NbtResult<f64> {
        self.typed(path, "a number", NbtValue::as_f64)
    }

    pub fn get_bool(&self, path: &str) -> NbtResult<bool> {
        self.typed(path, "a boolean", NbtValue::as_bool)
    }

    pub fn get_str(&self, path: &str) -> NbtResult<&str> {
        self.typed(path, "a string", NbtValue::as_str)
    }

    pub fn get_list(&self, path: &str) -> NbtResult<&[NbtValue]> {
        self.typed(path, "a list", NbtValue::as_list)
    }

    pub fn get_vec3(&self, path: &str) -> NbtResult<(f64, f64, f64)> {
        self.typed(path, "[x,y,z]", NbtValue::as_vec3)
    }

    pub fn get_block_pos(&self, path: &str) -> NbtResult<(i32, i32, i32)> {
        self.typed(path, "an [x,y,z] integer list", NbtValue::as_block_pos)
    }

    /* Lenient reads taking a path */

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

    /// Tries several paths in order and returns the first string that exists.
    ///
    /// The reason for this method is concrete: the same concept has different key names across
    /// events, `player`, `_player.name` or `name`, and business code was already writing this
    /// loop.
    pub fn first_str(&self, paths: &[&str]) -> Option<&str> {
        paths.iter().find_map(|p| self.opt_str(p))
    }

    /* The serde_json bridge */

    /// Converts into a `serde_json::Value`. The type suffix information is lost, since JSON
    /// does not distinguish byte from long, so this route suits saving, logging and sending to
    /// the web, and not converting back to feed the host.
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

    /// Converts from a `serde_json::Value`. An integer becomes a `Long` and a float a
    /// `Double`, which is the lossless choice. A narrower type has to be constructed by hand.
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

/* Convenience From implementations */

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
    /// `{}` gives the SNBT directly, which is what a log line most often wants.
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(&self.to_snbt())
    }
}

/* Path parsing helpers */

/// Splits `name[1][2]` into `("name", [1, 2])`.
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

/// `"a.b.c"` gives `Some("a.b")`; a single segment gives `None`.
fn parent_of(path: &str) -> Option<&str> {
    path.rfind('.').map(|i| &path[..i])
}
