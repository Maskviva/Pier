//! 自定义命令。
//!
//! # 命令注册是**单向**的
//!
//! 基岩版没有反注册命令的路子，所以一条命令注册之后活到服务器结束。模组被
//! 停用期间，宿主把回调**静音**（不是移除），重新启用就继续响应。因此这里
//! 没有 `unregister`，也没有返回一个 Drop 就注销的句柄 —— 那种句柄会让人
//! 以为它能撤销，而它不能。
//!
//! 推论：`on_enable` 里注册命令要能承受被调用多次（热重载）。宿主对同名
//! 重注册只换回调，不重建命令。
//!
//! # 两种形状
//!
//! [`register`] 收整行原始文本，适合自己解析。[`CommandBuilder`] 声明带类型的
//! overload，由引擎解析并做补全 —— 玩家在客户端能看到参数提示。

use core::ffi::c_void;

use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{r, s};
use crate::rt::logger::Logger;
use crate::rt::runtime::rt;
use crate::sys;

/// 执行一条命令需要的权限。数值镜像 `CommandPermissionLevel`。
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

/// 一条 overload 里一个参数的类型。
///
/// `Enum` 和 `SoftEnum` 还要一个枚举名，用 [`OverloadBuilder::required_enum`]
/// 那一对方法声明。
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
    /// ABI 上的拼法。宿主按这个字符串分发，拼错了整条 overload 被丢掉。
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

/// 命令发起方。
#[derive(Debug, Clone, PartialEq)]
pub struct CommandOrigin {
    /// 玩家名，或者控制台的名字。
    pub name: String,
    /// `CommandOriginType`。0 是玩家，7 是专用服务器控制台。
    pub kind: i32,
    /// 发起方所在的位置。控制台没有位置，那时是 `None`。
    pub at: Option<(i32, f64, f64, f64)>,
}

impl CommandOrigin {
    pub const PLAYER: i32 = 0;
    pub const DEDICATED_SERVER: i32 = 7;

    pub fn is_console(&self) -> bool {
        self.kind == Self::DEDICATED_SERVER
    }

    /// 发起方是玩家时给出他的名字。
    ///
    /// 注意这是**名字**，不是身份：做权限判断请用 `kind` 加上自己那一套
    /// 玩家表，理由见 [`crate::sel`]。
    pub fn player_name(&self) -> Option<&str> {
        if self.kind == Self::PLAYER {
            Some(&self.name)
        } else {
            None
        }
    }
}

/// 一次命令调用。
pub struct Invocation<'a> {
    /// 原始文本（`register`）或参数 SNBT（`CommandBuilder`）。
    raw: &'a str,
    origin_raw: &'a str,
    parsed: Option<NbtValue>,
    ctx: *mut c_void,
    out_success: sys::PierStrSink,
    out_error: sys::PierStrSink,
}

impl Invocation<'_> {
    /// 原始参数文本。`CommandBuilder` 注册的命令这里是参数 SNBT。
    pub fn raw(&self) -> &str {
        self.raw
    }

    /// 回一条成功输出。
    pub fn success(&self, msg: &str) {
        unsafe { (self.out_success)(self.ctx, s(msg)) };
    }

    /// 回一条错误输出。**和成功是两条通道**，客户端显示颜色不同，
    /// 而且失败的命令不该走成功通道 —— 那会让命令方块判断错。
    pub fn error(&self, msg: &str) {
        unsafe { (self.out_error)(self.ctx, s(msg)) };
    }

    /// 发起方。解析失败时退回一个只有名字的 origin 并告警：命令处理器多半
    /// 只想知道是谁发的，为一个位置字段让整条命令失败不值当。
    pub fn origin(&self) -> CommandOrigin {
        match NbtValue::parse(self.origin_raw) {
            Ok(v) => CommandOrigin {
                name: v.opt_str("name").unwrap_or_default().to_owned(),
                kind: v.opt_i32("type").unwrap_or(-1),
                at: match (v.opt_i32("dim"), v.opt_f64("x"), v.opt_f64("y"), v.opt_f64("z")) {
                    (Some(d), Some(x), Some(y), Some(z)) => Some((d, x, y, z)),
                    _ => None,
                },
            },
            // 纯文本的那一路（`register` 走的是名字而不是 SNBT）走这里，
            // 属于正常形状，不告警。
            Err(_) => CommandOrigin {
                name: self.origin_raw.to_owned(),
                kind: -1,
                at: None,
            },
        }
    }

    /// 命中的是第几条 overload。只有 `CommandBuilder` 注册的命令有。
    pub fn overload(&mut self) -> Option<i32> {
        self.parse().and_then(|v| v.opt_i32("overload"))
    }

    /// 取一个具名参数。没给（可选参数被省略）时是 `None`。
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

    /// 惰性解析参数 SNBT，一次调用只解一次。
    fn parse(&mut self) -> Option<&NbtValue> {
        if self.parsed.is_none() {
            match NbtValue::parse(self.raw) {
                Ok(v) => self.parsed = Some(v),
                Err(e) => {
                    Logger::get().warn(&format!("命令参数 SNBT 解析失败：{e}（原文：{}）", self.raw));
                    return None;
                }
            }
        }
        self.parsed.as_ref()
    }
}

type Handler = dyn FnMut(&mut Invocation<'_>) + Send + 'static;

/// 注册一条收整行原始文本的命令。
///
/// 回调里的 [`Invocation::raw`] 是 `/name` 后面那一整串，自己解析。
pub fn register(
    name: &str,
    description: &str,
    permission: CommandPermission,
    handler: impl FnMut(&mut Invocation<'_>) + Send + 'static,
) -> Result<()> {
    let f = crate::require_slot!(register_command, "注册命令");
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
        // 宿主没接管这个闭包，收回所有权 —— 泄漏一个失败的注册没有意义。
        drop(unsafe { Box::from_raw(user.cast::<Box<Handler>>()) });
        Err(Error(format!(
            "注册命令 /{name} 失败（名字非法，或已被占用且形状不同）"
        )))
    }
}

/// 一条 overload 的声明。
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

/// 带类型 overload 的命令。
///
/// ```ignore
/// command::builder("plot", "地皮管理", CommandPermission::Any)
///     .overload(|o| o.required("action", ParamType::String))
///     .overload(|o| o.required("action", ParamType::String).optional("who", ParamType::Player))
///     .register(|inv| {
///         match inv.arg_str("action") {
///             Some("info") => inv.success("……"),
///             _ => inv.error("不认识的子命令"),
///         }
///     })?;
/// ```
pub struct CommandBuilder {
    name: String,
    description: String,
    permission: CommandPermission,
    overloads: Vec<NbtValue>,
}

/// 开始声明一条带 overload 的命令。
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

    /// 注册。至少要有一条 overload —— 一条都没有的话宿主直接拒绝，
    /// 所以这里提前挡下并说明，省得对着「注册失败」猜原因。
    pub fn register(
        self,
        handler: impl FnMut(&mut Invocation<'_>) + Send + 'static,
    ) -> Result<()> {
        let f = crate::require_slot!(register_command_ex, "注册带参数的命令");
        if self.overloads.is_empty() {
            return Err(Error(format!(
                "命令 /{} 一条 overload 都没声明；不需要参数的话请用 command::register",
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
                "注册命令 /{} 失败（名字非法、已被占用且 overload 形状不同，或某个参数类型拼错了）",
                self.name
            )))
        }
    }
}

/// 注册一个静态枚举，供 [`ParamType::Enum`] 参数引用。
pub fn register_enum(name: &str, values: &[(&str, u64)]) -> Result<()> {
    let f = crate::require_slot!(register_command_enum, "注册命令枚举");
    let list = NbtValue::list(values.iter().map(|(k, v)| {
        NbtValue::list([NbtValue::from(*k), NbtValue::Long(*v as i64)])
    }));
    let spec = NbtValue::obj([("values", list)]).to_snbt();
    if unsafe { f(s(name), s(&spec)) } {
        Ok(())
    } else {
        Err(Error(format!("注册命令枚举 {name} 失败（名字非法或已存在）")))
    }
}

/// 注册一个可以在运行期改的软枚举，供 [`ParamType::SoftEnum`] 参数引用。
pub fn register_soft_enum(name: &str, values: &[&str]) -> Result<()> {
    let f = crate::require_slot!(register_command_soft_enum, "注册命令软枚举");
    let spec = soft_enum_snbt(values);
    if unsafe { f(s(name), s(&spec)) } {
        Ok(())
    } else {
        Err(Error(format!("注册命令软枚举 {name} 失败（名字非法或已存在）")))
    }
}

/// 改一个软枚举的取值。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SoftEnumOp {
    Set = 0,
    Add = 1,
    Remove = 2,
}

pub fn update_soft_enum(name: &str, op: SoftEnumOp, values: &[&str]) -> Result<()> {
    let f = crate::require_slot!(update_command_soft_enum, "更新命令软枚举");
    let spec = soft_enum_snbt(values);
    if unsafe { f(s(name), op as i32, s(&spec)) } {
        Ok(())
    } else {
        Err(Error(format!("更新命令软枚举 {name} 失败（这个名字没注册过）")))
    }
}

fn soft_enum_snbt(values: &[&str]) -> String {
    NbtValue::obj([(
        "values",
        NbtValue::list(values.iter().map(|v| NbtValue::from(*v))),
    )])
    .to_snbt()
}

/// 把回调装箱交给宿主。
///
/// **刻意泄漏**：命令不能反注册，所以这个闭包必须活到进程结束。没有一个
/// 「什么时候可以释放它」的时刻 —— 模组卸载之后宿主仍然持有这条命令，
/// 只是把回调静音。释放它就是给静音期的一次误触留一个悬垂指针。
fn leak_handler(handler: impl FnMut(&mut Invocation<'_>) + Send + 'static) -> *mut c_void {
    let boxed: Box<Box<Handler>> = Box::new(Box::new(handler));
    Box::into_raw(boxed).cast()
}

/// # Safety
/// `user` 必须是 `leak_handler` 的产物。
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
        // panic 穿过 extern "C" 是未定义行为。拦在这里，并且**告诉发命令的人**
        // ——否则他看到的是一条什么都没回的命令。
        Logger::get().error("命令处理器 panic 了。已就地拦下。");
        out_error(out_ctx, s("命令处理器内部出错，详见服务器日志"));
    }
}
