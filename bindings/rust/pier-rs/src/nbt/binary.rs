//! SNBT 与二进制 NBT 的互转。
//!
//! 走宿主的解析器而不是自己实现：二进制 NBT 的格式细节（网络格式的 varint
//! 长度、磁盘格式的小端序）跟着 BDS 版本走，自己写一份就要跟着它的版本升级，
//! 而对不上的症状是存档里多出一段谁都读不了的字节。

use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_bytes, call_out_str, s};

/// 二进制 NBT 的两种编码。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum NbtFormat {
    /// 存档用的小端序。
    LittleEndian = 0,
    /// 网络传输用的变长编码。
    Network = 1,
}

impl NbtFormat {
    pub fn as_i32(self) -> i32 {
        self as i32
    }
}

/// SNBT 文本转二进制。
pub fn to_binary(snbt: &str, fmt: NbtFormat) -> Result<Vec<u8>> {
    let f = crate::require_slot!(nbt_snbt_to_binary, "SNBT 转二进制 NBT");
    call_out_bytes(|ctx, sink| unsafe { f(s(snbt), fmt.as_i32(), ctx, sink) })
        .ok_or_else(|| Error("SNBT 转二进制失败（这段 SNBT 不合法）".to_owned()))
}

/// 二进制转 SNBT 文本。
pub fn from_binary(data: &[u8], fmt: NbtFormat) -> Result<String> {
    let f = crate::require_slot!(nbt_binary_to_snbt, "二进制 NBT 转 SNBT");
    call_out_str(|ctx, sink| unsafe { f(data.as_ptr(), data.len(), fmt.as_i32(), ctx, sink) })
        .ok_or_else(|| {
            Error(format!(
                "二进制转 SNBT 失败（{} 字节按{}解不出来）",
                data.len(),
                match fmt {
                    NbtFormat::LittleEndian => "存档格式",
                    NbtFormat::Network => "网络格式",
                }
            ))
        })
}

impl NbtValue {
    /// 把这棵树编码成二进制 NBT。
    pub fn to_binary(&self, fmt: NbtFormat) -> Result<Vec<u8>> {
        to_binary(&self.to_snbt(), fmt)
    }

    /// 从二进制 NBT 解出一棵树。
    pub fn from_binary(data: &[u8], fmt: NbtFormat) -> Result<NbtValue> {
        let text = from_binary(data, fmt)?;
        NbtValue::parse(&text).map_err(|e| Error(format!("宿主转出的 SNBT 解析失败：{e}")))
    }
}
