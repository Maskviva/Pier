//! Writing SNBT.
//! The other side, `CompoundTag::fromSnbt`, is stricter than this one, so the rule here is to be
//! verbose rather than terse:
//! * no type suffix is omitted. A `100.0` without the `d` is read as an int, the same trap
//!   the host side has where `snbtNum` writes no suffix on a float, only in the other direction.
//! * every key is quoted. A bare key fails to parse when it starts with a digit, contains a
//!   dot or contains a non-ASCII character, and a business key such as a player name or a
//!   plot id can be anything.
//! * a control character is written as `\uXXXX`. Invalid UTF-8 was already replaced with
//!   U+FFFD by the host on the way in and is not seen here.
//! * a float uses `{:?}` and not `{}`: the latter prints `1` for `1.0` and the suffix does not
//!   recover the type, since `1d` is valid while not being the same writing habit as `1.0d`
//!   and is easily misread in a diff. A non-finite value, NaN or an infinity, cannot be
//!   expressed in SNBT and lands as `0`.

use super::NbtValue;

pub(super) fn write(v: &NbtValue, out: &mut String) {
    match v {
        NbtValue::Byte(x) => {
            out.push_str(&x.to_string());
            out.push('b');
        }
        NbtValue::Short(x) => {
            out.push_str(&x.to_string());
            out.push('s');
        }
        NbtValue::Int(x) => out.push_str(&x.to_string()),
        NbtValue::Long(x) => {
            out.push_str(&x.to_string());
            out.push('L');
        }
        NbtValue::Float(x) => {
            out.push_str(&fmt_float(*x as f64));
            out.push('f');
        }
        NbtValue::Double(x) => {
            out.push_str(&fmt_float(*x));
            out.push('d');
        }
        NbtValue::String(s) => {
            out.push('"');
            escape(s, out);
            out.push('"');
        }
        NbtValue::List(items) => {
            out.push('[');
            for (i, it) in items.iter().enumerate() {
                if i > 0 {
                    out.push(',');
                }
                write(it, out);
            }
            out.push(']');
        }
        NbtValue::Compound(map) => {
            out.push('{');
            for (i, (k, val)) in map.iter().enumerate() {
                if i > 0 {
                    out.push(',');
                }
                out.push('"');
                escape(k, out);
                out.push_str("\":");
                write(val, out);
            }
            out.push('}');
        }
        NbtValue::ByteArray(a) => {
            out.push_str("[B;");
            for (i, x) in a.iter().enumerate() {
                if i > 0 {
                    out.push(',');
                }
                out.push_str(&x.to_string());
                out.push('b');
            }
            out.push(']');
        }
        NbtValue::IntArray(a) => {
            out.push_str("[I;");
            for (i, x) in a.iter().enumerate() {
                if i > 0 {
                    out.push(',');
                }
                out.push_str(&x.to_string());
            }
            out.push(']');
        }
        NbtValue::LongArray(a) => {
            out.push_str("[L;");
            for (i, x) in a.iter().enumerate() {
                if i > 0 {
                    out.push(',');
                }
                out.push_str(&x.to_string());
                out.push('L');
            }
            out.push(']');
        }
    }
}

fn fmt_float(v: f64) -> String {
    if !v.is_finite() {
        return "0.0".to_owned();
    }
    let s = format!("{v:?}"); // `{:?}` guarantees the decimal point is there
    if s.contains(['.', 'e', 'E']) {
        s
    } else {
        s + ".0"
    }
}

fn escape(s: &str, out: &mut String) {
    for c in s.chars() {
        match c {
            '"' => out.push_str("\\\""),
            '\\' => out.push_str("\\\\"),
            '\n' => out.push_str("\\n"),
            '\r' => out.push_str("\\r"),
            '\t' => out.push_str("\\t"),
            c if (c as u32) < 0x20 || c as u32 == 0x7f => {
                out.push_str(&format!("\\u{:04x}", c as u32));
            }
            c => out.push(c),
        }
    }
}
