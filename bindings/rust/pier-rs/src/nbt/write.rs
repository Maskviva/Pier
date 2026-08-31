//! SNBT 写出。
//!
//! 对面（`CompoundTag::fromSnbt`）比我们严格，所以这里的规矩是「宁可啰嗦」：
//!
//! * **类型后缀一个都不省**。`100.0` 不写 `d` 会被读成 int，那正是宿主侧
//!   V-21 修的同一个坑（`snbtNum` 对浮点不带后缀），只不过方向反过来。
//! * **键一律加引号**。裸键在遇到数字开头、含点号或中文时会解析失败，而
//!   业务侧的键名（玩家名、地皮 id）什么都可能是。
//! * **控制字符写成 `\uXXXX`**，非法 UTF-8 在进来的时候就被宿主换成了 U+FFFD
//!   （V-19），这里不会再见到。
//! * 浮点用 `{:?}` 而不是 `{}`：后者对 `1.0` 会输出 `1`，后缀救不回来类型
//!   （`1d` 合法但和 `1.0d` 不是一个书写习惯，容易在 diff 里被误读）。
//!   非有限值（NaN/±Inf）SNBT 表达不了，落成 `0`。

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
    let s = format!("{v:?}"); // `{:?}` 保证小数点在
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
