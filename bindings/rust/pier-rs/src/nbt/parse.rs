//! SNBT parsing.
//!
//! Every piece of structured data the host sends comes from
//! `CompoundTag::toSnbt(Minimize)` and this reads it back. The grammar has two things
//! JSON does not, and those two are what make a JSON parser unusable here:
//!
//! * type suffixes: `1b` for byte, `2s` for short, `3L` for long, `4.5f` for float and
//!   `6.7d` for double. An integer without a suffix is an int and a decimal without one is
//!   a double.
//! * bare keys and bare strings: neither `name` nor `stone` in `{name:stone}` is quoted.
//!
//! There are also typed arrays: `[B; 1b,2b]`, `[I; 1,2]` and `[L; 1L,2L]`.
//!
//! An error carries a byte offset: one line of SNBT easily runs to hundreds of characters,
//! and saying only that parsing failed says nothing.

use std::collections::BTreeMap;
use std::fmt;

use super::NbtValue;

/// A parse failure. `at` is the byte offset of the fault.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ParseError {
    pub at: usize,
    pub what: String,
}

impl fmt::Display for ParseError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "SNBT at byte {}: {}", self.at, self.what)
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
        return Err(p.err("there are leftover characters after the parse"));
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
            Err(self.err(format!("a `{}` was expected here", c as char)))
        }
    }

    fn value(&mut self) -> Result<NbtValue, ParseError> {
        self.ws();
        match self.peek() {
            None => Err(self.err("the input ended in the middle of a value")),
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
            return Err(self.err("a `,` or a `}` was expected inside the compound tag"));
        }
    }

    /// A key may be quoted or bare.
    fn key(&mut self) -> Result<String, ParseError> {
        self.ws();
        match self.peek() {
            Some(b'"') | Some(b'\'') => self.quoted(),
            Some(_) => {
                let start = self.i;
                while let Some(c) = self.peek() {
                    // It excludes `:`, the key-value separator. Including it reads the key of
                    // `{layout:{...}}` as `layout:` and puts the error on the next character,
                    // saying a `:` was expected at byte 8 while pointing at the `{`. The
                    // shape `{generatorType:Void,...}` is subtler, reading the key as
                    // `generatorType:Void` and putting the error on the comma. Both errors
                    // point at a correct position and neither points at the problem. A key
                    // containing a colon, such as `minecraft:stone`, has to be quoted in SNBT.
                    if c.is_ascii_alphanumeric() || matches!(c, b'_' | b'-' | b'.' | b'+') {
                        self.i += 1;
                    } else {
                        break;
                    }
                }
                if self.i == start {
                    return Err(self.err("a key name was expected here"));
                }
                Ok(String::from_utf8_lossy(&self.s[start..self.i]).into_owned())
            }
            None => Err(self.err("the input ended in the middle of a key name")),
        }
    }

    fn quoted(&mut self) -> Result<String, ParseError> {
        let quote = self.peek().ok_or_else(|| self.err("the string never begins"))?;
        self.i += 1;
        let mut out = String::new();
        loop {
            let Some(c) = self.peek() else {
                return Err(self.err("the string has no closing quote"));
            };
            self.i += 1;
            match c {
                c if c == quote => return Ok(out),
                b'\\' => {
                    let Some(e) = self.peek() else {
                        return Err(self.err("there is nothing after the backslash"));
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
                            // \uXXXX, which the snbtEscape of the host uses to write a control
                            // character.
                            if self.i + 4 > self.s.len() {
                                return Err(self.err("there are fewer than four digits after \\u"));
                            }
                            let hex = std::str::from_utf8(&self.s[self.i..self.i + 4])
                                .map_err(|_| self.err("what follows \\u is not valid hexadecimal"))?;
                            let cp = u32::from_str_radix(hex, 16)
                                .map_err(|_| self.err("what follows \\u is not valid hexadecimal"))?;
                            self.i += 4;
                            out.push(char::from_u32(cp).unwrap_or('\u{fffd}'));
                        }
                        other => out.push(other as char),
                    }
                }
                // Multi-byte UTF-8 is carried through unchanged: this walks bytes and cannot use
                // `as char`.
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

    /// `[B;...]`, `[I;...]` and `[L;...]` are typed arrays and everything else is an ordinary
    /// list.
    fn array_or_list(&mut self) -> Result<NbtValue, ParseError> {
        self.expect(b'[')?;
        // Two characters of lookahead decide whether it is a typed array
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
            return Err(self.err("a `,` or a `]` was expected inside the list"));
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
                .ok_or_else(|| self.err("a typed array holds integers only"))?;
            raw.push(n);
            self.ws();
            if self.eat(b',') {
                continue;
            }
            if self.eat(b']') {
                break;
            }
            return Err(self.err("a `,` or a `]` was expected inside the typed array"));
        }
        Ok(match kind {
            b'B' => NbtValue::ByteArray(raw.into_iter().map(|v| v as i8).collect()),
            b'I' => NbtValue::IntArray(raw.into_iter().map(|v| v as i32).collect()),
            _ => NbtValue::LongArray(raw),
        })
    }

    /// A bare scalar: a number, optionally with a type suffix, `true` or `false`, or a bare
    /// string.
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
            return Err(self.err("a value was expected here"));
        }
        let raw = std::str::from_utf8(&self.s[start..self.i])
            .map_err(|_| ParseError {
                at: start,
                what: "the scalar is not valid UTF-8".to_owned(),
            })?
            .to_owned();

        match raw.as_str() {
            "true" => return Ok(NbtValue::Byte(1)),
            "false" => return Ok(NbtValue::Byte(0)),
            _ => {}
        }

        // The type suffix. Note that `L` means something only on an integer while both `f` and
        // `d` apply.
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
                    // Without a suffix an integer is an int, promoted to long when it does not fit,
                    // and a
                    // decimal is a double.
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

        // Neither one makes it a bare string, as in `{name:stone}`.
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
