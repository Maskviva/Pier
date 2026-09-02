//! 同工具链快车道 —— 直接的函数表调用，绕开 JSON。
//!
//! 和 [`crate::service`] 的分工：service 是跨语言的 `(名字, JSON) -> JSON`
//! 通道，任何时候都成立；lane 递的是**裸的 data 与 vtable 指针**，只在两侧
//! 由同一条工具链编出来时成立。指纹对不上就拿不到指针，降级走 service。
//!
//! 指纹必须由调用方算，`0` 非法：这个槽把 vtable 原样交出去，消费方按
//! **自己**的类型布局解释它，跳过校验就是类型混淆；`0` 会被当成「谁都能连」。
//! 只想看有哪些车道用 [`list`]，它一个指针都不递。
//!
//! 取到之后每次调用的纪律见 [`Lane::with`]。

use core::ffi::c_void;
use core::sync::atomic::{AtomicU32, Ordering};

use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, s};
use crate::rt::logger::Logger;
use crate::rt::runtime::rt;
use crate::sys;

/// 一份车道契约：两侧共同商定的函数表形状。
///
/// `FINGERPRINT` 必须在**表的形状改变时一起变**，包括字段顺序、参数类型、
/// 调用约定。两侧引用同一个契约定义（同一个 crate 的同一个版本）时它自然
/// 一致；靠手抄一份相同的常量是错的 —— 那正是它要防的情况。
pub trait LaneContract {
    /// C 布局的函数表类型。
    type Table;
    /// 车道名。
    const NAME: &'static str;
    /// 构建指纹。**不许是 0**。
    const FINGERPRINT: u64;
}

/// 取车道失败的原因。
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum LaneError {
    /// 没人发布这个名字（那个模组没装）。
    NotFound { name: String },
    /// 发布了，但指纹不同。**降级走 service，别递指针。**
    Fingerprint {
        name: String,
        theirs: u64,
        ours: u64,
    },
    /// 名字非法、取自己的、提供方被禁用，或协议版本不符。
    Refused { name: String },
    /// 宿主没有提供快车道能力。
    Unavailable,
}

impl LaneError {
    /// 一句「该怎么办」。日志里打这个，而不是打枚举名（契约 §5.3）。
    pub fn advice(&self) -> String {
        match self {
            LaneError::NotFound { name } => {
                format!("没有模组发布车道 `{name}`：那个模组没装，或者还没起来。走 service 或跳过这个集成。")
            }
            LaneError::Fingerprint { name, theirs, ours } => format!(
                "车道 `{name}` 的指纹对不上（对方 {theirs:#x}，本模组 {ours:#x}）：\
                 两个模组不是同一条工具链编出来的。这不是错误，降级走 service::call。"
            ),
            LaneError::Refused { name } => {
                format!(
                    "宿主拒绝取车道 `{name}`：名字非法、取的是自己、提供方被禁用，或协议版本不符。"
                )
            }
            LaneError::Unavailable => "这个宿主没有提供快车道能力。".to_owned(),
        }
    }
}

impl std::fmt::Display for LaneError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(&self.advice())
    }
}

impl std::error::Error for LaneError {}

impl From<LaneError> for Error {
    fn from(e: LaneError) -> Error {
        Error(e.advice())
    }
}

/// 一条取到的车道。**Drop 即归还租约。**
pub struct Lane<C: LaneContract> {
    lease: u64,
    fingerprint: u64,
    data: *mut c_void,
    vtable: *const C::Table,
    alive: *const u32,
    busy: *mut u32,
}

impl<C: LaneContract> Lane<C> {
    /// 提供方现在还在吗。
    ///
    /// 用 `Acquire` 读：写方用的是 release store，relaxed 读和它建立不了
    /// happens-before 关系。
    pub fn is_alive(&self) -> bool {
        if self.alive.is_null() {
            return false;
        }
        // SAFETY：这块存活标志由宿主持有且**永不释放**，提供方消失之后读它
        // 仍然合法 —— 这正是它存在的全部理由。
        let cell = unsafe { &*(self.alive as *const AtomicU32) };
        cell.load(Ordering::Acquire) != 0
    }

    pub fn fingerprint(&self) -> u64 {
        self.fingerprint
    }

    /// 进提供方的代码跑一段。提供方已经不在时返回 `None`。
    ///
    /// 进去之前把 `busy` 加一，出来减一 —— 这段期间宿主会拒绝卸载提供方，
    /// 所以栈上那一帧不会被 `FreeLibrary` 抽走（见模块文档）。
    /// 交给闭包的是 [`LaneData`] 而不是裸 `*mut c_void`：车道表里每个函数的
    /// 第一个参数都是 `LaneData`（那是对面的 self），交裸指针等于让调用方在
    /// 每个调用点手动包一次。
    pub fn with<R>(&self, f: impl FnOnce(&C::Table, LaneData) -> R) -> Option<R> {
        if !self.is_alive() || self.vtable.is_null() {
            return None;
        }
        let busy = if self.busy.is_null() {
            None
        } else {
            // SAFETY：同 `alive`，这块计数器由宿主持有且永不释放。
            Some(unsafe { &*(self.busy as *const AtomicU32) })
        };
        if let Some(b) = busy {
            b.fetch_add(1, Ordering::AcqRel);
        }
        // SAFETY：vtable 非空且 alive 为真，提供方保证它活到车道被撤下。
        let table = unsafe { &*self.vtable };
        let out = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            f(table, LaneData(self.data))
        }));
        if let Some(b) = busy {
            b.fetch_sub(1, Ordering::AcqRel);
        }
        match out {
            Ok(v) => Some(v),
            Err(_) => {
                // panic 就地拦下，不再向上传播：这里是 `busy` 的临界区，
                // 让 panic 穿过去会跳过下面已经执行完的那次 fetch_sub 之后的
                // 所有清理，而更重要的是调用方多半会把它带过 extern "C" 边界。
                // 计数在上面已经减回去了，提供方仍然卸得掉。
                Logger::get().error("车道调用 panic 了。已就地拦下，busy 计数已复位。");
                None
            }
        }
    }
}

impl<C: LaneContract> Drop for Lane<C> {
    fn drop(&mut self) {
        if self.lease == 0 {
            return;
        }
        if !crate::has_slot!(lane_release) {
            return;
        }
        let Some(f) = crate::__rt::api().lane_release else {
            return;
        };
        // 返回 false 表示提供方已经不在了 —— 宿主那时已经替你 release 过，
        // 再调一次就是双重释放。所以这里**不**重试，也不报错。
        unsafe { f(rt().handle(), self.lease) };
    }
}

pub fn acquire<C: LaneContract>() -> std::result::Result<Lane<C>, LaneError> {
    if !crate::has_slot!(lane_acquire) {
        return Err(LaneError::Unavailable);
    }
    let Some(f) = crate::__rt::api().lane_acquire else {
        return Err(LaneError::Unavailable);
    };
    let mut out = sys::PierLaneRef {
        struct_size: core::mem::size_of::<sys::PierLaneRef>() as u32,
        lease: 0,
        fingerprint: 0,
        data: core::ptr::null_mut(),
        vtable: core::ptr::null(),
        alive: core::ptr::null(),
        busy: core::ptr::null_mut(),
    };
    let code = unsafe {
        f(
            rt().handle(),
            s(C::NAME),
            C::FINGERPRINT,
            &mut out as *mut sys::PierLaneRef,
        )
    };
    match code {
        sys::PIER_LANE_OK => Ok(Lane {
            lease: out.lease,
            fingerprint: out.fingerprint,
            data: out.data,
            vtable: out.vtable as *const C::Table,
            alive: out.alive,
            busy: out.busy,
        }),
        sys::PIER_LANE_NOT_FOUND => Err(LaneError::NotFound {
            name: C::NAME.to_owned(),
        }),
        sys::PIER_LANE_FINGERPRINT => Err(LaneError::Fingerprint {
            name: C::NAME.to_owned(),
            theirs: out.fingerprint,
            ours: C::FINGERPRINT,
        }),
        _ => Err(LaneError::Refused {
            name: C::NAME.to_owned(),
        }),
    }
}

/// 在车道回调里就地拦下 panic。
///
/// 发布方的表函数都是 `extern "C"`，而 panic 穿过 `extern "C"` 是未定义行为。
/// 每个表函数的第一行都该是这个 —— 里面跑的是本模组的业务代码，它 panic
/// 的概率不比别处低，而这里 panic 的后果比别处严重得多:调用方是**另一个
/// 模组**，栈上还压着它的帧。
///
/// panic 时返回 `fallback` 并打日志。`fallback` 必须是这个表函数语义上
/// 「答不上来」的那个值，不是「否」—— 把 panic 当成一次明确的否定回答，
/// 就是让一个 bug 去做本该由业务逻辑做的判断。
pub fn guard<R>(fallback: R, f: impl FnOnce() -> R) -> R {
    match std::panic::catch_unwind(std::panic::AssertUnwindSafe(f)) {
        Ok(v) => v,
        Err(_) => {
            Logger::get().error("车道表函数 panic 了。已就地拦下，本次返回兜底值。");
            fallback
        }
    }
}

/// 一次发布。**Drop 即撤下。**
pub struct Publication {
    id: u64,
    name: String,
}

impl Publication {
    pub fn id(&self) -> u64 {
        self.id
    }

    pub fn forget(mut self) {
        self.id = 0;
    }
}

impl Drop for Publication {
    fn drop(&mut self) {
        if self.id == 0 {
            return;
        }
        if !crate::has_slot!(lane_unpublish) {
            return;
        }
        let Some(f) = crate::__rt::api().lane_unpublish else {
            return;
        };
        if !unsafe { f(rt().handle(), self.id) } {
            Logger::get().error(&format!("撤下车道 `{}` 失败。", self.name));
        }
    }
}

/// 发布一条车道。
///
/// # Safety
///
/// 调用方必须保证：
///
/// * `data` 与 `vtable` 在这条车道被撤下之前一直有效。宿主一个字节都不解释，
///   也不拷贝，只做相等比较和存活标记；
/// * `vtable` 指向的确实是 `C::Table` 那个 C 布局的表，且它的形状和
///   `C::FINGERPRINT` 对得上；
/// * `retain` / `release` 不回调任何 `lane_*` 槽 —— 那会自死锁。
pub unsafe fn publish<C: LaneContract>(
    data: *mut c_void,
    vtable: *const C::Table,
    retain: Option<sys::PierLaneRefFn>,
    release: Option<sys::PierLaneRefFn>,
) -> Result<Publication> {
    let f = crate::require_slot!(lane_publish, "发布快车道");
    if C::FINGERPRINT == 0 {
        return Err(Error(format!(
            "车道 `{}` 的指纹是 0，而 0 被保留为「谁都能连」，ABI 拒绝它",
            C::NAME
        )));
    }
    let desc = sys::PierLaneDesc {
        struct_size: core::mem::size_of::<sys::PierLaneDesc>() as u32,
        protocol: sys::PIER_LANE_PROTOCOL,
        fingerprint: C::FINGERPRINT,
        data,
        vtable: vtable as *const c_void,
        retain,
        release,
    };
    let id = f(rt().handle(), s(C::NAME), &desc as *const sys::PierLaneDesc);
    if id == 0 {
        return Err(Error(format!(
            "发布车道 `{}` 失败（名字被占用，宿主日志里点了占用者的名字）",
            C::NAME
        )));
    }
    Ok(Publication {
        id,
        name: C::NAME.to_owned(),
    })
}

/// 一条已发布车道的登记信息。
#[derive(Debug, Clone, PartialEq, Eq, serde::Deserialize)]
pub struct LaneInfo {
    pub name: String,
    #[serde(rename = "mod")]
    #[serde(default)]
    pub owner: String,
    /// 宿主给的是 `"0x…"` 字符串形式。
    #[serde(default)]
    pub fingerprint: String,
    #[serde(default)]
    pub protocol: u32,
    #[serde(default)]
    pub leases: u32,
    #[serde(default)]
    pub alive: bool,
}

/// 全部车道。**不递任何指针**，所以想先看看有什么就用它。
pub fn list() -> Vec<LaneInfo> {
    if !crate::has_slot!(lane_list) {
        return Vec::new();
    }
    let Some(f) = crate::__rt::api().lane_list else {
        return Vec::new();
    };
    let Some(text) = call_out_str(|ctx, sink| {
        unsafe { f(ctx, sink) };
        true
    }) else {
        return Vec::new();
    };
    if text.trim().is_empty() {
        return Vec::new();
    }
    serde_json::from_str(&text).unwrap_or_else(|e| {
        Logger::get().warn(&format!("车道清单解析失败：{e}"));
        Vec::new()
    })
}

// ── 车道两侧共用的编组类型 ────────────────────────────────────────
//
// 表里每个参数类型都必须 `#[repr(C)]` 且两侧布局一致，所以 `&str` / `Vec<T>`
// / `String` 都不行 —— 它们的布局是 Rust 内部约定。
//
// 和 `PierStr` 同形但**刻意是另一个类型**：那个由 `struct_size` 管兼容，
// 车道由 `LaneContract::FINGERPRINT` 管。混用会让人以为改 abi.h 能影响车道。

/// 车道上传一段 UTF-8 文本。
///
/// 指针的有效期由**产出方**决定,通常只在一次调用期间(契约 §三)。
/// 接收方要留着就得拷走。
#[repr(C)]
#[derive(Clone, Copy)]
pub struct LaneStr {
    pub ptr: *const u8,
    pub len: usize,
}

impl LaneStr {
    pub const EMPTY: LaneStr = LaneStr {
        ptr: core::ptr::null(),
        len: 0,
    };

    /// 借一个 `&str`。返回值的生命周期没有被类型系统跟踪 —— 这正是
    /// 跨 ABI 边界的代价,所以它只该出现在「构造完立刻传进去」的位置。
    pub fn new(s: &str) -> LaneStr {
        LaneStr {
            ptr: s.as_ptr(),
            len: s.len(),
        }
    }

    /// 读成 `&str`。
    ///
    /// 内容不是合法 UTF-8 时返回 `None` 而不是 `from_utf8_unchecked` ——
    /// 对面可能是另一种语言写的,它的「字符串」未必过得了这一关。
    ///
    /// # Safety
    /// `ptr` / `len` 必须描述一段现在仍然有效的内存。
    pub unsafe fn try_as_str<'a>(&self) -> Option<&'a str> {
        if self.ptr.is_null() {
            return Some("");
        }
        core::str::from_utf8(core::slice::from_raw_parts(self.ptr, self.len)).ok()
    }
}

/// 车道上传一段同类型元素的连续内存。
#[repr(C)]
#[derive(Clone, Copy)]
pub struct LaneSlice<T> {
    pub ptr: *const T,
    pub len: usize,
}

impl<T> LaneSlice<T> {
    pub fn new(v: &[T]) -> LaneSlice<T> {
        LaneSlice {
            ptr: v.as_ptr(),
            len: v.len(),
        }
    }

    /// # Safety
    /// `ptr` / `len` 必须描述一段现在仍然有效、且确实是 `T` 的内存。
    pub unsafe fn as_slice<'a>(&self) -> &'a [T] {
        if self.ptr.is_null() {
            return &[];
        }
        core::slice::from_raw_parts(self.ptr, self.len)
    }
}

/// 车道函数的不透明上下文指针。
///
/// 包一层而不是直接用 `*mut c_void`,是为了让契约里的表签名读得出
/// 「这个参数是那一侧的 self」。
#[repr(transparent)]
#[derive(Clone, Copy)]
pub struct LaneData(pub *mut c_void);

/// 车道上「产出方 sink 一条,接收方拷走」的回调形状。
pub type LaneStrSink = unsafe extern "C" fn(ctx: *mut c_void, item: LaneStr);

/// 把一个走 [`LaneStrSink`] 的车道调用收成 `Vec<String>`。
///
/// 非 UTF-8 的那一条**跳过并告警**,不让整批作废 —— 对面可能是另一种语言
/// 写的,一条坏数据不该让整次查询无法回答。
pub fn collect_strings(call: impl FnOnce(*mut c_void, LaneStrSink) -> u32) -> Vec<String> {
    let mut out: Vec<String> = Vec::new();
    /// # Safety
    /// `ctx` 必须是一个有效的 `*mut Vec<String>`。
    unsafe extern "C" fn sink(ctx: *mut c_void, item: LaneStr) {
        let v = &mut *ctx.cast::<Vec<String>>();
        match item.try_as_str() {
            Some(s) => v.push(s.to_owned()),
            None => Logger::get().warn("车道回传了一条不是合法 UTF-8 的字符串,已跳过"),
        }
    }
    call((&mut out as *mut Vec<String>).cast(), sink);
    out
}

/// 车道清单的原始 JSON,不解析。
///
/// [`list`] 解析不动、或者宿主加了这一层还不认识的字段时,用它自己看。
pub fn list_json() -> String {
    if !crate::has_slot!(lane_list) {
        return String::new();
    }
    let Some(f) = crate::__rt::api().lane_list else {
        return String::new();
    };
    call_out_str(|ctx, sink| {
        unsafe { f(ctx, sink) };
        true
    })
    .unwrap_or_default()
}
