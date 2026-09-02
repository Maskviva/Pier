//! Converting between SNBT and binary NBT.
//!
//! It goes through the host parser rather than an implementation of its own: the format
//! details of binary NBT, the varint lengths of the network form and the little-endian
//! order of the disk form, follow the BDS version, so a copy here would have to follow
//! every version bump, and the symptom of a mismatch is a stretch of bytes in the save
//! nobody can read.

use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_bytes, call_out_str, s};

/// The two encodings of binary NBT.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum NbtFormat {
    /// The little-endian form used in a save.
    LittleEndian = 0,
    /// The variable-length encoding used on the network.
    Network = 1,
}

impl NbtFormat {
    pub fn as_i32(self) -> i32 {
        self as i32
    }
}

/// Converts SNBT text into binary.
pub fn to_binary(snbt: &str, fmt: NbtFormat) -> Result<Vec<u8>> {
    let f = crate::require_slot!(nbt_snbt_to_binary, "converting SNBT into binary NBT");
    call_out_bytes(|ctx, sink| unsafe { f(s(snbt), fmt.as_i32(), ctx, sink) })
        .ok_or_else(|| Error("converting SNBT into binary failed: this SNBT is invalid".to_owned()))
}

/// Converts binary into SNBT text.
pub fn from_binary(data: &[u8], fmt: NbtFormat) -> Result<String> {
    let f = crate::require_slot!(nbt_binary_to_snbt, "converting binary NBT into SNBT");
    call_out_str(|ctx, sink| unsafe { f(data.as_ptr(), data.len(), fmt.as_i32(), ctx, sink) })
        .ok_or_else(|| {
            Error(format!(
                "converting binary into SNBT failed: {} bytes could not be decoded as the {}",
                data.len(),
                match fmt {
                    NbtFormat::LittleEndian => "save form",
                    NbtFormat::Network => "network form",
                }
            ))
        })
}

impl NbtValue {
    /// Encodes this tree into binary NBT.
    pub fn to_binary(&self, fmt: NbtFormat) -> Result<Vec<u8>> {
        to_binary(&self.to_snbt(), fmt)
    }

    /// Decodes a tree from binary NBT.
    pub fn from_binary(data: &[u8], fmt: NbtFormat) -> Result<NbtValue> {
        let text = from_binary(data, fmt)?;
        NbtValue::parse(&text)
            .map_err(|e| Error(format!("parsing the SNBT the host produced failed: {e}")))
    }
}
