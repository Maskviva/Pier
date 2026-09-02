//! Player selectors: how the rule that a handle is an identity and not a pointer lands.
//!
//! On the ABI it is `{kind: i32, value: PierStr}` with `kind` at 0, 1 or 2. An earlier
//! generation handed that bare integer to callers, so every call site wrote
//! `PlayerSel { kind: 1, .. }` and what `1` meant could only be looked up. This makes it an
//! enum and writes one safety-relevant fact into the type layer:
//!
//! # `Name` is not an identity
//!
//! Resolving `kind=0`, the host falls back to the display name, `getNameTag`, when no
//! account name matches. A display name can be changed by another mod, through
//! `AACT_SET_NAME_TAG` or a title plugin, so a player setting their display name to the
//! account name of an offline player redirects every by-name call onto themselves. A key
//! for permissions, economy or plot ownership may only be [`PlayerSel::xuid`], and `Name`
//! is left for a case such as a player typing a name in chat.

use crate::rt::ffi::s;
use crate::sys;

/// How to point at a player.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum PlayerSel {
    /// By name. The account name is matched first and the host falls back to the display
    /// name on a miss; see the module documentation and do not use it as an identity.
    Name(String),
    /// By xuid: unique, unforgeable and unchangeable by the player. This is the one to use
    /// as a key.
    /// It may be an empty string on an offline-mode server, where only `Name` remains.
    Xuid(String),
    /// By uuid in its canonical string form. Equally stable and suited to a save key.
    Uuid(String),
}

impl PlayerSel {
    pub fn name(v: impl Into<String>) -> PlayerSel {
        PlayerSel::Name(v.into())
    }
    pub fn xuid(v: impl Into<String>) -> PlayerSel {
        PlayerSel::Xuid(v.into())
    }
    pub fn uuid(v: impl Into<String>) -> PlayerSel {
        PlayerSel::Uuid(v.into())
    }

    /// The underlying `kind` value, 0, 1 or 2, aligned with `abi.h`.
    pub fn kind(&self) -> i32 {
        match self {
            PlayerSel::Name(_) => 0,
            PlayerSel::Xuid(_) => 1,
            PlayerSel::Uuid(_) => 2,
        }
    }

    pub fn value(&self) -> &str {
        match self {
            PlayerSel::Name(v) | PlayerSel::Xuid(v) | PlayerSel::Uuid(v) => v,
        }
    }

    /// Whether this selector is a reliable identity, meaning an xuid or a uuid.
    ///
    /// A permission, economy or ownership decision receiving `false` should take care; see
    /// the module documentation.
    pub fn is_stable(&self) -> bool {
        !matches!(self, PlayerSel::Name(_))
    }

    /// An emptiness check: an empty selector resolves to nobody, and finding that early beats
    /// seeing an inexplicable `false` at the call site.
    pub fn is_empty(&self) -> bool {
        self.value().is_empty()
    }

    /// Converts into the FFI shape.
    ///
    /// Note that the returned `PierStr` borrows the string inside `self` and is valid only
    /// while `self` lives. It is `pub(crate)`, every call site is inside this crate, and each
    /// has the shape of constructing it and passing it into the FFI immediately, so none
    /// survives across a call.
    ///
    pub(crate) fn raw(&self) -> sys::PierPlayerSel {
        sys::PierPlayerSel {
            kind: self.kind(),
            value: s(self.value()),
        }
    }
}

impl From<&str> for PlayerSel {
    /// A convenience conversion by name. It goes through the display-name fallback, and using
    /// it as an identity means writing `PlayerSel::xuid` explicitly.
    fn from(v: &str) -> PlayerSel {
        PlayerSel::Name(v.to_owned())
    }
}

impl From<String> for PlayerSel {
    fn from(v: String) -> PlayerSel {
        PlayerSel::Name(v)
    }
}

impl std::fmt::Display for PlayerSel {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            PlayerSel::Name(v) => write!(f, "name:{v}"),
            PlayerSel::Xuid(v) => write!(f, "xuid:{v}"),
            PlayerSel::Uuid(v) => write!(f, "uuid:{v}"),
        }
    }
}

// Dimension selectors

/// The name of a dimension in command text, for `execute in <sel> run ...`.
///
/// The three vanilla dimensions have fixed names. The name of a custom dimension, with an
/// id of 3 or above, comes from [`crate::dimensions::list`], so that branch crosses the
/// ABI once and is slower than the three vanilla ones.
///
/// A `None` means the dimension has no usable selector: the id is negative, or it was
/// never registered.
/// `dim.to_string()` must not be used as a fallback: `execute in 3` is not a valid command,
/// the assembled command fails on the engine side, and the failure message shows nothing
/// about the cause being here.
pub fn dimension_selector(dim: i32) -> Option<String> {
    match dim {
        0 => return Some("overworld".to_owned()),
        1 => return Some("nether".to_owned()),
        2 => return Some("the_end".to_owned()),
        _ => {}
    }
    if dim < 0 {
        return None;
    }
    crate::dimensions::list()
        .into_iter()
        .find(|d| d.dim == dim)
        .map(|d| d.name)
}
