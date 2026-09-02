//! The only error type of this layer.
//!
//! One `String` rather than a tree of error enums, deliberately: the failure information
//! coming back across the ABI boundary is already a sentence in prose, a line the host
//! sank, and splitting it into an enum would need an error code table maintained on both
//! sides, whose every drift is silent.
//!
//! The cost is that a caller cannot branch on the type. No downstream needs that today,
//! and when one really does an enum can be added, at which point the error codes go into
//! `abi.h` first (contract §0: the ABI is the product).

/// Why one ABI call failed. The message is meant for a human and can go straight into a
/// log.
#[derive(Debug)]
pub struct Error(pub String);

impl core::fmt::Display for Error {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        f.write_str(&self.0)
    }
}

impl std::error::Error for Error {}

impl Error {
    pub fn new(msg: impl core::fmt::Display) -> Self {
        Error(msg.to_string())
    }
}

impl From<String> for Error {
    fn from(s: String) -> Self {
        Error(s)
    }
}

impl From<&str> for Error {
    fn from(s: &str) -> Self {
        Error(s.to_owned())
    }
}

pub type Result<T> = core::result::Result<T, Error>;
