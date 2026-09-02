//! SNBT 解析。
//!
//! 宿主发过来的每一段结构化数据都是 `CompoundTag::toSnbt(Minimize)` 的产物，
//! 这里负责把它读回来。语法比 JSON 多两样东西，也正是这两样让「拿 JSON 解析器
//! 凑合」行不通：
//!
//! * **类型后缀**：`1b`（byte）、`2s`（short）、`3L`（long）、`4.5f`（float）、
//!   `6.7d`（double）。没后缀的整数是 int、没后缀的小数是 double。
//! * **裸键与裸字符串**：`{name:stone}` 里的 `name` 和 `stone` 都不带引号。
//!
//! 另外还有类型化数组 `[B; 1b,2b]` / `[I; 1,2]` / `[L; 1L,2L]`。
//!
//! 错误里带**字节偏移**：SNBT 一行动辄几百字符，只说「解析失败」等于没说。

use std::collections::BTreeMap;
use std::fmt;

use super::NbtValue;

/// 解析失败。`at` 是出错处的字节偏移。
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ParseError {
    pub at: usize,
    pub what: String,
}

impl fmt::Display for ParseError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "SNBT 第 {} 字节处：{}", self.at, self.what)
    }
}

impl std::error::Error for ParseError {}

impl From<ParseError> for crate::Error {
    fn from(e: ParseError) -> Self {
        crate::Error(e.to_string())
    }
}

pub(super) fn parse(text: &str) -> Result<NbtValue, ParseError> {
    let mut p = Parser {
        s: text.as_bytes(),
        i: 0,
    };
    p.ws();
    let v = p.value()?;
    p.ws();
    if p.i != p.s.len() {
        return Err(p.err("解析完之后还有多余的字符"));
    }
    Ok(v)
}

struct Parser<'a> {
    s: &'a [u8],
    i: usize,
}

impl<'a> Parser<'a> {
    fn err(&self, what: impl Into<String>) -> ParseError {
        ParseError {
            at: self.i,
            what: what.into(),
        }
    }

    fn peek(&self) -> Option<u8> {
        self.s.get(self.i).copied()
    }

    fn ws(&mut self) {
        while matches!(self.peek(), Some(b' ' | b'\t' | b'\n' | b'\r')) {
            self.i += 1;
        }
    }

    fn eat(&mut self, c: u8) -> bool {
        if self.peek() == Some(c) {
            self.i += 1;
            true
        } else {
            false
        }
    }

    fn expect(&mut self, c: u8) -> Result<(), ParseError> {
        if self.eat(c) {
            Ok(())
        } else {
            Err(self.err(format!("这里应该是 `{}`", c as char)))
        }
    }

    fn value(&mut self) -> Result<NbtValue, ParseError> {
        self.ws();
        match self.peek() {
            None => Err(self.err("值没写完就到头了")),
            Some(b'{') => self.compound(),
            Some(b'[') => self.array_or_list(),
            Some(b'"') | Some(b'\'') => Ok(NbtValue::String(self.quoted()?)),
            _ => self.scalar(),
        }
    }

    fn compound(&mut self) -> Result<NbtValue, ParseError> {
        self.expect(b'{')?;
        let mut map = BTreeMap::new();
        loop {
            self.ws();
            if self.eat(b'}') {
                return Ok(NbtValue::Compound(map));
            }
            let key = self.key()?;
            self.ws();
            self.expect(b':')?;
            let v = self.value()?;
            map.insert(key, v);
            self.ws();
            if self.eat(b',') {
                continue;
            }
            self.ws();
            if self.eat(b'}') {
                return Ok(NbtValue::Compound(map));
            }
            return Err(self.err("复合标签里应该是 `,` 或 `}`"));
        }
    }

    /// 键可以带引号，也可以是裸的。
    fn key(&mut self) -> Result<String, ParseError> {
        self.ws();
        match self.peek() {
            Some(b'"') | Some(b'\'') => self.quoted(),
            Some(_) => {
                let start = self.i;
                while let Some(c) = self.peek() {
                    // 不含 `:` —— 冒号是键值分隔符。放进来的话
                    // `{layout:{…}}` 的键会被读成 `layout:`，报错落在下一个
                    // 字符上（「第 8 字节应该是 `:`」，指着那个 `{`）；
                    // `{generatorType:Void,…}` 更隐蔽，键读成
                    // `generatorType:Void`，错误落在逗号上。两条报错的位置都
                    // 对，指的都不是出问题的地方。
                    //
                    // 带冒号的键（`minecraft:stone`）在 SNBT 里必须加引号。
                    if c.is_ascii_alphanumeric() || matches!(c, b'_' | b'-' | b'.' | b'+') {
                        self.i += 1;
                    } else {
                        break;
                    }
                }
                if self.i == start {
                    return Err(self.err("这里应该是一个键名"));
                }
                Ok(String::from_utf8_lossy(&self.s[start..self.i]).into_owned())
            }
            None => Err(self.err("键名没写完就到头了")),
        }
    }

    fn quoted(&mut self) -> Result<String, ParseError> {
        let quote = self.peek().ok_or_else(|| self.err("字符串没开始"))?;
        self.i += 1;
        let mut out = String::new();
        loop {
            let Some(c) = self.peek() else {
                return Err(self.err("字符串没有收尾的引号"));
            };
            self.i += 1;
            match c {
                c if c == quote => return Ok(out),
                b'\\' => {
                    let Some(e) = self.peek() else {
                        return Err(self.err("反斜杠后面没东西了"));
                    };
                    self.i += 1;
                    match e {
                        b'n' => out.push('\n'),
                        b'r' => out.push('\r'),
                        b't' => out.push('\t'),
                        b'b' => out.push('\u{8}'),
                        b'f' => out.push('\u{c}'),
                        b'0' => out.push('\0'),
                        b'u' => {
                            // \uXXXX —— 宿主的 snbtEscape 会用它写控制字符。
                            if self.i + 4 > self.s.len() {
                                return Err(self.err("\\u 后面不足四位"));
                            }
                            let hex = std::str::from_utf8(&self.s[self.i..self.i + 4])
                                .map_err(|_| self.err("\\u 后面不是合法十六进制"))?;
                            let cp = u32::from_str_radix(hex, 16)
                                .map_err(|_| self.err("\\u 后面不是合法十六进制"))?;
                            self.i += 4;
                            out.push(char::from_u32(cp).unwrap_or('\u{fffd}'));
                        }
                        other => out.push(other as char),
                    }
                }
                // 多字节 UTF-8 原样搬运：这里按字节走，不能用 `as char`。
                c if c >= 0x80 => {
                    let start = self.i - 1;
                    let len = utf8_len(c);
                    let end = (start + len).min(self.s.len());
                    out.push_str(&String::from_utf8_lossy(&self.s[start..end]));
                    self.i = end;
                }
                c => out.push(c as char),
            }
        }
    }

    /// `[B;…]` / `[I;…]` / `[L;…]` 是类型化数组，其余是普通列表。
    fn array_or_list(&mut self) -> Result<NbtValue, ParseError> {
        self.expect(b'[')?;
        // 前瞻两个字符判断是不是类型化数组
        if self.s.len() >= self.i + 2 && self.s[self.i + 1] == b';' {
            let kind = self.s[self.i];
            if matches!(kind, b'B' | b'I' | b'L') {
                self.i += 2;
                return self.typed_array(kind);
            }
        }
        let mut items = Vec::new();
        loop {
            self.ws();
            if self.eat(b']') {
                return Ok(NbtValue::List(items));
            }
            items.push(self.value()?);
            self.ws();
            if self.eat(b',') {
                continue;
            }
            if self.eat(b']') {
                return Ok(NbtValue::List(items));
            }
            return Err(self.err("列表里应该是 `,` 或 `]`"));
        }
    }

    fn typed_array(&mut self, kind: u8) -> Result<NbtValue, ParseError> {
        let mut raw: Vec<i64> = Vec::new();
        loop {
            self.ws();
            if self.eat(b']') {
                break;
            }
            let v = self.value()?;
            let n = v
                .as_i64()
                .ok_or_else(|| self.err("类型化数组里只能放整数"))?;
            raw.push(n);
            self.ws();
            if self.eat(b',') {
                continue;
            }
            if self.eat(b']') {
                break;
            }
            return Err(self.err("类型化数组里应该是 `,` 或 `]`"));
        }
        Ok(match kind {
            b'B' => NbtValue::ByteArray(raw.into_iter().map(|v| v as i8).collect()),
            b'I' => NbtValue::IntArray(raw.into_iter().map(|v| v as i32).collect()),
            _ => NbtValue::LongArray(raw),
        })
    }

    /// 裸标量：数字（可带类型后缀）、`true`/`false`、或裸字符串。
    fn scalar(&mut self) -> Result<NbtValue, ParseError> {
        let start = self.i;
        while let Some(c) = self.peek() {
            if c.is_ascii_alphanumeric() || matches!(c, b'_' | b'-' | b'+' | b'.') {
                self.i += 1;
            } else {
                break;
            }
        }
        if self.i == start {
            return Err(self.err("这里应该是一个值"));
        }
        let raw = std::str::from_utf8(&self.s[start..self.i])
            .map_err(|_| ParseError {
                at: start,
                what: "标量不是合法 UTF-8".to_owned(),
            })?
            .to_owned();

        match raw.as_str() {
            "true" => return Ok(NbtValue::Byte(1)),
            "false" => return Ok(NbtValue::Byte(0)),
            _ => {}
        }

        // 类型后缀。注意 `L` 只在整数上有意义，`f`/`d` 两种都行。
        let (body, suffix) = match raw.as_bytes().last() {
            Some(&s)
                if matches!(
                    s,
                    b'b' | b'B' | b's' | b'S' | b'l' | b'L' | b'f' | b'F' | b'd' | b'D'
                ) =>
            {
                (&raw[..raw.len() - 1], Some(s.to_ascii_lowercase()))
            }
            _ => (raw.as_str(), None),
        };

        let numeric = !body.is_empty()
            && body
                .bytes()
                .all(|c| c.is_ascii_digit() || matches!(c, b'-' | b'+' | b'.' | b'e' | b'E'));

        if numeric {
            match suffix {
                Some(b'b') => {
                    if let Ok(v) = body.parse::<i64>() {
                        return Ok(NbtValue::Byte(v as i8));
                    }
                }
                Some(b's') => {
                    if let Ok(v) = body.parse::<i64>() {
                        return Ok(NbtValue::Short(v as i16));
                    }
                }
                Some(b'l') => {
                    if let Ok(v) = body.parse::<i64>() {
                        return Ok(NbtValue::Long(v));
                    }
                }
                Some(b'f') => {
                    if let Ok(v) = body.parse::<f32>() {
                        return Ok(NbtValue::Float(v));
                    }
                }
                Some(b'd') => {
                    if let Ok(v) = body.parse::<f64>() {
                        return Ok(NbtValue::Double(v));
                    }
                }
                _ => {
                    // 无后缀：整数是 int（放不下就升 long），小数是 double。
                    if let Ok(v) = raw.parse::<i32>() {
                        return Ok(NbtValue::Int(v));
                    }
                    if let Ok(v) = raw.parse::<i64>() {
                        return Ok(NbtValue::Long(v));
                    }
                    if let Ok(v) = raw.parse::<f64>() {
                        return Ok(NbtValue::Double(v));
                    }
                }
            }
        }

        // 都不是 → 裸字符串（`{name:stone}` 这种）。
        Ok(NbtValue::String(raw))
    }
}

fn utf8_len(b: u8) -> usize {
    if b >= 0xF0 {
        4
    } else if b >= 0xE0 {
        3
    } else if b >= 0xC0 {
        2
    } else {
        1
    }
}
