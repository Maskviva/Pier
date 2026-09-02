//! Generates accessors from the property constant tables.
//!
//! The property family of `PierApi` has the shape of one numeric slot plus one constant
//! table: reading the hunger of a player and reading whether they are flying both go
//! through the same `player_get_num` and differ by one constant. Hand-written that is dozens
//! of identical one-line functions, and the compiler says nothing about a mistyped constant,
//! since every type is the same.
//!
//! A table puts the constant next to the method name so a mistake is visible. What it
//! removes is the repetition in the source and not the API surface: `Player` still has
//! dozens of methods, which is the shape of the ABI.

/// Generates accessors line by line from `kind method_name = constant;`.
///
/// The kind decides the return type and the read path: `f64`, `i32` and `bool` go through
/// `num` and `str` goes through `text`.
/// A target type only has to provide those two methods. The truncation for `i32` and the
/// non-zero test for `bool` happen in the `@get` branch below, which is their single
/// definition. A private `num_i32` and `num_bool` helper in each of the three domains became
/// dead code once the property wall moved into a table.
///
/// Documentation is optional, since `///` is desugared into an attribute before macro
/// matching. Adding a line saying hunger to `hunger()` only repeats the method name, so one
/// is written only where the name is not enough: a unit, a value range, or how it differs
/// from a neighboring property.
macro_rules! accessors {
    ($ty:ty; $( $(#[$m:meta])* $kind:ident $name:ident = $konst:ident; )*) => {
        impl $ty {
            $(
                $(#[$m])*
                pub fn $name(&self) -> $crate::rt::error::Result<accessors!(@ret $kind)> {
                    accessors!(@get self, $kind, $crate::sys::$konst)
                }
            )*
        }
    };

    (@ret f64) => { f64 };
    (@ret i32) => { i32 };
    (@ret bool) => { bool };
    (@ret str) => { ::std::string::String };

    (@get $s:expr, f64, $k:expr) => { $s.num($k) };
    (@get $s:expr, i32, $k:expr) => { $s.num($k).map(|v| v as i32) };
    (@get $s:expr, bool, $k:expr) => { $s.num($k).map(|v| v != 0.0) };
    (@get $s:expr, str, $k:expr) => { $s.text($k) };
}

pub(crate) use accessors;
