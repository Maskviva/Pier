//! Custom commands.
//! # Command registration is one way
//! Bedrock offers no route to deregister a command, so a registered command lives until the server
//! stops. While a mod is disabled the host mutes the callback rather than removing it, and re-
//! enabling resumes it. There is therefore no `unregister` and no handle that deregisters on drop,
//! since such a handle would suggest it can be undone when it cannot.
//!
//! It follows that registering a command inside `on_enable` has to survive being called several
//! times, as a hot reload does. A re-registration under the same name only swaps the callback and
//! does not rebuild the command.
//!
//! # Two shapes
//!
//! [`register`] takes the whole raw line, which suits parsing it yourself. [`CommandBuilder`]
//! declares typed overloads that the engine parses and completes, so a player sees argument hints
//! on the client.

use core::ffi::c_void;

use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{r, s};
use crate::rt::logger::Logger;
use crate::rt::runtime::rt;
use crate::sys;

/// The permission a command needs. The values mirror `CommandPermissionLevel`.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CommandPermission {
    Any = 0,
    GameDirectors = 1,
    Admin = 2,
    Host = 3,
    Owner = 4,
}

impl CommandPermission {
    pub fn as_i32(self) -> i32 {
        self as i32
    }
}

/// The type of one parameter inside one overload.
///
/// `Enum` and `SoftEnum` also need an enum name, declared through the
/// [`OverloadBuilder::required_enum`] pair of methods.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ParamType {
    Int,
    Bool,
    Float,
    String,
    Enum,
    SoftEnum,
    Actor,
    Player,
    BlockPos,
    Vec3,
    RawText,
    Message,
    Json,
    Item,
    BlockName,
    Effect,
    ActorType,
    Command,
    RelativeFloat,
    FilePath,
}

impl ParamType {
    /// The spelling on the ABI. The host dispatches on this string and a typo drops the whole
    /// overload.
    pub fn as_str(self) -> &'static str {
        match self {
            ParamType::Int => "int",
            ParamType::Bool => "bool",
            ParamType::Float => "float",
            ParamType::String => "string",
            ParamType::Enum => "enum",
            ParamType::SoftEnum => "soft_enum",
            ParamType::Actor => "actor",
            ParamType::Player => "player",
            ParamType::BlockPos => "block_pos",
            ParamType::Vec3 => "vec3",
            ParamType::RawText => "raw_text",
            ParamType::Message => "message",
            ParamType::Json => "json",
            ParamType::Item => "item",
            ParamType::BlockName => "block_name",
            ParamType::Effect => "effect",
            ParamType::ActorType => "actor_type",
            ParamType::Command => "command",
            ParamType::RelativeFloat => "relative_float",
            ParamType::FilePath => "file_path",
        }
    }
}

/// Where a command came from.
#[derive(Debug, Clone, PartialEq)]
pub struct CommandOrigin {
    /// The player name, or the name of the console.
    pub name: String,
    /// The `CommandOriginType`, where 0 is a player and 7 is the dedicated server console.
    pub kind: i32,
    /// Where the origin is. A console has no position and gives `None`.
    pub at: Option<(i32, f64, f64, f64)>,
}

impl CommandOrigin {
    pub const PLAYER: i32 = 0;
    pub const DEDICATED_SERVER: i32 = 7;

    pub fn is_console(&self) -> bool {
        self.kind == Self::DEDICATED_SERVER
    }

    /// The player name when the origin is a player.
    ///
    /// Note this is a name and not an identity: a permission decision uses `kind` plus a
    /// own player table, for the reason [`crate::sel`] gives.
    pub fn player_name(&self) -> Option<&str> {
        if self.kind == Self::PLAYER {
            Some(&self.name)
        } else {
            None
        }
    }
}

/// One command invocation.
pub struct Invocation<'a> {
    /// The raw text for `register`, or the argument SNBT for `CommandBuilder`.
    raw: &'a str,
    origin_raw: &'a str,
    parsed: Option<NbtValue>,
    ctx: *mut c_void,
    out_success: sys::PierStrSink,
    out_error: sys::PierStrSink,
}

impl Invocation<'_> {
    /// The raw argument text. For a command registered through `CommandBuilder` this is the
    /// argument SNBT.
    pub fn raw(&self) -> &str {
        self.raw
    }

    /// Returns one success output.
    pub fn success(&self, msg: &str) {
        unsafe { (self.out_success)(self.ctx, s(msg)) };
    }

    /// Returns one error output. It is a separate channel from success, the client colors it
    /// differently, and a failed command must not use the success channel, which would make a
    /// command block decide wrongly.
    pub fn error(&self, msg: &str) {
        unsafe { (self.out_error)(self.ctx, s(msg)) };
    }

    /// The origin. A failed parse falls back to an origin carrying only a name, with a
    /// warning: a command handler usually only wants to know who sent it, and failing the
    /// whole command over one position field is not worth it.
    pub fn origin(&self) -> CommandOrigin {
        match NbtValue::parse(self.origin_raw) {
            Ok(v) => CommandOrigin {
                name: v.opt_str("name").unwrap_or_default().to_owned(),
                kind: v.opt_i32("type").unwrap_or(-1),
                at: match (
                    v.opt_i32("dim"),
                    v.opt_f64("x"),
                    v.opt_f64("y"),
                    v.opt_f64("z"),
                ) {
                    (Some(d), Some(x), Some(y), Some(z)) => Some((d, x, y, z)),
                    _ => None,
                },
            },
            // The plain-text route, where `register` carries a name rather than SNBT, comes
            // through here as a normal shape and is not warned about.
            Err(_) => CommandOrigin {
                name: self.origin_raw.to_owned(),
                kind: -1,
                at: None,
            },
        }
    }

    /// Which overload matched. Only a command registered through `CommandBuilder` has one.
    pub fn overload(&mut self) -> Option<i32> {
        self.parse().and_then(|v| v.opt_i32("overload"))
    }

    /// Reads a named argument. An omitted optional argument gives `None`.
    pub fn arg(&mut self, name: &str) -> Option<&NbtValue> {
        self.parse()?;
        self.parsed.as_ref()?.path(&format!("args.{name}"))
    }

    pub fn arg_str(&mut self, name: &str) -> Option<&str> {
        self.arg(name).and_then(|v| v.as_str())
    }

    pub fn arg_i64(&mut self, name: &str) -> Option<i64> {
        self.arg(name).and_then(|v| v.as_i64())
    }

    pub fn arg_f64(&mut self, name: &str) -> Option<f64> {
        self.arg(name).and_then(|v| v.as_f64())
    }

    pub fn arg_bool(&mut self, name: &str) -> Option<bool> {
        self.arg(name).and_then(|v| v.as_bool())
    }

    /// Parses the argument SNBT lazily, once per invocation.
    fn parse(&mut self) -> Option<&NbtValue> {
        if self.parsed.is_none() {
            match NbtValue::parse(self.raw) {
                Ok(v) => self.parsed = Some(v),
                Err(e) => {
                    Logger::get().warn(&format!(
                        "parsing the command argument SNBT failed: {e} (raw: {})",
                        self.raw
                    ));
                    return None;
                }
            }
        }
        self.parsed.as_ref()
    }
}

type Handler = dyn FnMut(&mut Invocation<'_>) + Send + 'static;

/// Registers a command that takes the whole raw line.
///
/// [`Invocation::raw`] in the callback is everything after `/name`, parsed by you.
pub fn register(
    name: &str,
    description: &str,
    permission: CommandPermission,
    handler: impl FnMut(&mut Invocation<'_>) + Send + 'static,
) -> Result<()> {
    let f = crate::require_slot!(register_command, "registering a command");
    let user = leak_handler(handler);
    let ok = unsafe {
        f(
            rt().handle(),
            s(name),
            s(description),
            permission.as_i32(),
            trampoline,
            user,
        )
    };
    if ok {
        Ok(())
    } else {
        // The host did not take the closure, so ownership comes back: leaking a failed
        // registration serves no purpose.
        drop(unsafe { Box::from_raw(user.cast::<Box<Handler>>()) });
        Err(Error(format!(
            "registering command /{name} failed: the name is invalid, or it is taken with a different shape"
        )))
    }
}

/// The declaration of one overload.
#[derive(Debug, Clone, Default)]
pub struct OverloadBuilder {
    params: Vec<NbtValue>,
}

impl OverloadBuilder {
    pub fn required(self, name: &str, kind: ParamType) -> OverloadBuilder {
        self.push(name, kind, None, false)
    }

    pub fn optional(self, name: &str, kind: ParamType) -> OverloadBuilder {
        self.push(name, kind, None, true)
    }

    pub fn required_enum(self, name: &str, kind: ParamType, enum_name: &str) -> OverloadBuilder {
        self.push(name, kind, Some(enum_name), false)
    }

    pub fn optional_enum(self, name: &str, kind: ParamType, enum_name: &str) -> OverloadBuilder {
        self.push(name, kind, Some(enum_name), true)
    }

    fn push(
        mut self,
        name: &str,
        kind: ParamType,
        enum_name: Option<&str>,
        optional: bool,
    ) -> OverloadBuilder {
        let mut entries = vec![
            ("name".to_owned(), NbtValue::from(name)),
            ("kind".to_owned(), NbtValue::from(kind.as_str())),
            ("optional".to_owned(), NbtValue::from(optional)),
        ];
        if let Some(e) = enum_name {
            entries.push(("enum".to_owned(), NbtValue::from(e)));
        }
        self.params.push(NbtValue::obj(entries));
        self
    }
}

/// A command with typed overloads.
///
/// ```ignore
/// command::builder("plot", "plot management", CommandPermission::Any)
///     .overload(|o| o.required("action", ParamType::String))
///     .overload(|o| o.required("action", ParamType::String).optional("who", ParamType::Player))
///     .register(|inv| {
///         match inv.arg_str("action") {
///             Some("info") => inv.success("……"),
///             _ => inv.error("unrecognized subcommand"),
///         }
///     })?;
/// ```
pub struct CommandBuilder {
    name: String,
    description: String,
    permission: CommandPermission,
    overloads: Vec<NbtValue>,
}

/// Begins declaring a command with overloads.
pub fn builder(
    name: impl Into<String>,
    description: impl Into<String>,
    permission: CommandPermission,
) -> CommandBuilder {
    CommandBuilder {
        name: name.into(),
        description: description.into(),
        permission,
        overloads: Vec::new(),
    }
}

impl CommandBuilder {
    pub fn overload(
        mut self,
        build: impl FnOnce(OverloadBuilder) -> OverloadBuilder,
    ) -> CommandBuilder {
        let o = build(OverloadBuilder::default());
        self.overloads.push(NbtValue::list(o.params));
        self
    }

    /// Registers it. At least one overload is required, since the host refuses outright with
    /// none, so this stops it earlier and says why rather than leaving a bare registration
    /// failure to be guessed at.
    pub fn register(self, handler: impl FnMut(&mut Invocation<'_>) + Send + 'static) -> Result<()> {
        let f = crate::require_slot!(register_command_ex, "registering a command with arguments");
        if self.overloads.is_empty() {
            return Err(Error(format!(
                "command /{} declares no overload at all; use command::register when no argument is needed",
                self.name
            )));
        }
        let spec = NbtValue::obj([("overloads", NbtValue::list(self.overloads))]).to_snbt();
        let user = leak_handler(handler);
        let ok = unsafe {
            f(
                rt().handle(),
                s(&self.name),
                s(&self.description),
                self.permission.as_i32(),
                s(&spec),
                trampoline,
                user,
            )
        };
        if ok {
            Ok(())
        } else {
            drop(unsafe { Box::from_raw(user.cast::<Box<Handler>>()) });
            Err(Error(format!(
                "registering command /{} failed: the name is invalid, it is taken with a different overload shape, or a parameter type is misspelled",
                self.name
            )))
        }
    }
}

/// Registers a static enum for a [`ParamType::Enum`] parameter to reference.
pub fn register_enum(name: &str, values: &[(&str, u64)]) -> Result<()> {
    let f = crate::require_slot!(register_command_enum, "registering a command enum");
    let list = NbtValue::list(
        values
            .iter()
            .map(|(k, v)| NbtValue::list([NbtValue::from(*k), NbtValue::Long(*v as i64)])),
    );
    let spec = NbtValue::obj([("values", list)]).to_snbt();
    if unsafe { f(s(name), s(&spec)) } {
        Ok(())
    } else {
        Err(Error(format!(
            "registering command enum {name} failed: the name is invalid or already exists"
        )))
    }
}

/// Registers a soft enum that can change at runtime, for a [`ParamType::SoftEnum`]
/// parameter to reference.
pub fn register_soft_enum(name: &str, values: &[&str]) -> Result<()> {
    let f = crate::require_slot!(
        register_command_soft_enum,
        "registering a command soft enum"
    );
    let spec = soft_enum_snbt(values);
    if unsafe { f(s(name), s(&spec)) } {
        Ok(())
    } else {
        Err(Error(format!(
            "registering command soft enum {name} failed: the name is invalid or already exists"
        )))
    }
}

/// Changes the values of a soft enum.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SoftEnumOp {
    Set = 0,
    Add = 1,
    Remove = 2,
}

pub fn update_soft_enum(name: &str, op: SoftEnumOp, values: &[&str]) -> Result<()> {
    let f = crate::require_slot!(update_command_soft_enum, "updating a command soft enum");
    let spec = soft_enum_snbt(values);
    if unsafe { f(s(name), op as i32, s(&spec)) } {
        Ok(())
    } else {
        Err(Error(format!(
            "updating command soft enum {name} failed: that name was never registered"
        )))
    }
}

fn soft_enum_snbt(values: &[&str]) -> String {
    NbtValue::obj([(
        "values",
        NbtValue::list(values.iter().map(|v| NbtValue::from(*v))),
    )])
    .to_snbt()
}

/// Boxes the callback and hands it to the host.
///
/// Leaked on purpose: a command cannot be deregistered, so this closure has to live until
/// the process ends. There is no moment at which it may be freed, since the host still
/// holds the command after the mod unloads and only mutes the callback. Freeing it would
/// leave a dangling pointer for a stray invocation during the muted period.
fn leak_handler(handler: impl FnMut(&mut Invocation<'_>) + Send + 'static) -> *mut c_void {
    let boxed: Box<Box<Handler>> = Box::new(Box::new(handler));
    Box::into_raw(boxed).cast()
}

/// # Safety
/// `user` must come from `leak_handler`.
unsafe extern "C" fn trampoline(
    user: *mut c_void,
    args: sys::PierStr,
    origin_name: sys::PierStr,
    out_ctx: *mut c_void,
    out_success: sys::PierStrSink,
    out_error: sys::PierStrSink,
) {
    if user.is_null() {
        return;
    }
    let f = &mut *(user as *mut Box<Handler>);
    let mut inv = Invocation {
        raw: r(args),
        origin_raw: r(origin_name),
        parsed: None,
        ctx: out_ctx,
        out_success,
        out_error,
    };
    let outcome = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| f(&mut inv)));
    if outcome.is_err() {
        // A panic crossing extern "C" is undefined behavior. It is caught here and the sender
        // is told, otherwise they see a command that answered nothing.
        Logger::get().error("a command handler panicked. It was caught here.");
        out_error(
            out_ctx,
            s("the command handler failed internally; see the server log"),
        );
    }
}
