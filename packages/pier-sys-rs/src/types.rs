//! 契约里的基础类型与回调签名，逐个对着 `sdk/abi.h`。
//!
//! 这一层**不做任何封装**：没有 `Drop`、没有 `From`、没有生命周期。
//! 它的全部职责是让 Rust 侧看到和 C 侧逐字节相同的形状。安全封装在
//! `pier-rs`（crate 名 `levilamina`）那一层。
//!
//! 分成两层的理由不是洁癖：这一层的每一个声明都必须能和 `abi.h` 逐字对上，
//! 而对照是**机器做**的（`sys-mirrors-abi`）。一旦这里混进「顺手加个
//! `impl Display`」之类的东西，对照就得先分辨哪些是契约、哪些是便利，
//! 而那个分辨没有可靠的机器判据。

use core::ffi::c_void;

/// UTF-8 字符串视图。**不保证 NUL 结尾**，长度是唯一的边界。
///
/// 对着 `abi.h` 的 `typedef struct PierStr { const char* ptr; size_t len; }`。
/// v0 这里曾经是 `std::string_view` 的别名 —— 那让一份「C ABI」依赖了 MSVC
/// 标准库的布局细节，还得配一段运行期自检来赎罪。现在布局由这份声明本身
/// 定义，自检因此可以删掉：**布局由声明定义，就不需要验证声明**。
///
/// 所有权见契约 §三：谁产出谁释放，接收方只在回调期间读，要留就拷贝。
#[repr(C)]
#[derive(Clone, Copy)]
pub struct PierStr {
    pub ptr: *const core::ffi::c_char,
    pub len: usize,
}

impl PierStr {
    /// 空串。**不是** NULL —— 宿主允许 `ptr` 为空且 `len == 0`，
    /// 但统一走这个构造能让「空」只有一种表示。
    pub const EMPTY: Self = Self {
        ptr: core::ptr::null(),
        len: 0,
    };

    /// 从一个 Rust 串借出视图。
    ///
    /// # Safety
    /// 调用方必须保证 `s` 在宿主读完之前一直活着。跨过 ABI 边界之后
    /// 借用检查器帮不上忙 —— 这正是这一层被标成 `unsafe` 的原因。
    #[inline]
    pub fn borrow(s: &str) -> Self {
        Self {
            ptr: s.as_ptr() as *const core::ffi::c_char,
            len: s.len(),
        }
    }
}

/// 宿主管理的模组实例句柄，对模组不透明。
pub type PierModHandle = *mut c_void;
/// 事件监听器句柄。
pub type PierListenerHandle = *mut c_void;
/// 实体的运行期 id。
pub type PierActorId = i64;
/// KvDb 句柄。
pub type PierKvDbHandle = *mut c_void;
/// 数据包 hook 句柄。
pub type PierPacketHookHandle = *mut c_void;
/// 客户端热键句柄。
pub type PierKeyHandle = *mut c_void;
/// 热键动作码（按下 / 抬起 …）。
pub type PierKeyAction = i32;
/// 热键的焦点影响。
pub type PierFocusImpact = i32;

/// `get_player_position` 的返回。`found == false` 时坐标无意义。
///
/// 这个结构体本身就是契约 §5.2 的一个例子：「查不到这个玩家」和「他站在
/// 原点」必须分得开，所以 `found` 是独立的一位，而不是靠坐标全零来表示。
#[repr(C)]
#[derive(Clone, Copy)]
pub struct PierPlayerPos {
    pub x: f64,
    pub y: f64,
    pub z: f64,
    pub dimension: i32,
    pub found: bool,
}

/// 玩家选择器：`kind` 说明 `value` 怎么解释（名字 / XUID / UUID …）。
#[repr(C)]
#[derive(Clone, Copy)]
pub struct PierPlayerSel {
    pub kind: i32,
    pub value: PierStr,
}

/// 容器引用：要么是某个玩家身上的容器，要么是世界里某个坐标上的。
#[repr(C)]
#[derive(Clone, Copy)]
pub struct PierContainerRef {
    pub which: i32,
    pub player: PierPlayerSel,
    pub dim: i32,
    pub x: i32,
    pub y: i32,
    pub z: i32,
}

/// 提供方发布一条同工具链快车道时对它自己的描述。
///
/// `data` / `vtable` 由**提供方**分配并保证存活（契约 §三）。宿主一个字节
/// 都不解释，只做相等比较和存活标记。
#[repr(C)]
pub struct PierLaneDesc {
    pub struct_size: u32,
    pub protocol: u32,
    pub fingerprint: u64,
    pub data: *mut c_void,
    pub vtable: *const c_void,
    /// C 侧允许为 NULL（abi.h：「可为 NULL」）；非 `Option` 的函数指针在 Rust
    /// 里读到 NULL 是未定义行为（V-41）。`Option<fn>` 与裸指针同布局。
    pub retain: Option<PierLaneRefFn>,
    pub release: Option<PierLaneRefFn>,
}

/// 取到的一条车道。
///
/// `alive` 指向宿主持有的、**永不释放**的存活标志：提供方消失的那一刻宿主
/// 把它清零。消费方每次用 `data` / `vtable` 之前都要读它 —— 这是跨
/// `FreeLibrary` 之后唯一还成立的判据。
#[repr(C)]
pub struct PierLaneRef {
    pub struct_size: u32,
    pub lease: u64,
    pub fingerprint: u64,
    pub data: *mut c_void,
    pub vtable: *const c_void,
    pub alive: *const u32,
    pub busy: *mut u32,
}

/// 一个数据包 hook 收到的事件。`body` 只在回调期间有效。
#[repr(C)]
pub struct PierPacketEvent {
    pub struct_size: u32,
    pub direction: i32,
    pub conn_id: u64,
    pub address: PierStr,
    pub packet_id: i32,
    pub sender_sub_id: u8,
    pub target_sub_id: u8,
    pub body: *const u8,
    pub body_len: usize,
}

/// 回调想改写包头时填这里。只有返回 `PIER_PKT_REPLACE` 时才被读。
#[repr(C)]
pub struct PierPacketEdit {
    pub struct_size: u32,
    pub packet_id: i32,
    pub sender_sub_id: u8,
    pub target_sub_id: u8,
}

/// 经济事件类型。
///
/// 名字里没有外部产品名（v0 叫 `LLMoneyEvent`）—— 那个名字和
/// LegacyMoney 自己的头文件在全局作用域**撞名**，两头一起 include 直接
/// 重定义。改名同时把外部产品名从契约面清掉了（契约 §七）。
#[repr(i32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum PierMoneyEvent {
    Set = 0,
    Add = 1,
    Reduce = 2,
    Trans = 3,
}

// ── 回调签名 ───────────────────────────────────────────────────────
//
// 全部是 `unsafe extern "C" fn`，没有一个是 `Option<...>` —— 契约里它们是
// 裸函数指针类型（`typedef void (*PierTaskCb)(void*)`），传 NULL 是调用方的
// bug 而不是一种合法取值。`PierApi` 的**槽位**才用 `Option`，因为空槽是
// 「这个能力没编进宿主」的正式表示。

pub type PierTaskCb = unsafe extern "C" fn(user: *mut c_void);
pub type PierStrSink = unsafe extern "C" fn(ctx: *mut c_void, s: PierStr);
pub type PierEventCb = unsafe extern "C" fn(
    user: *mut c_void,
    event_id: PierStr,
    snbt: PierStr,
    write_ctx: *mut c_void,
    write_back: PierStrSink,
);
pub type PierCommandCb = unsafe extern "C" fn(
    user: *mut c_void,
    args: PierStr,
    origin_name: PierStr,
    out_ctx: *mut c_void,
    out_success: PierStrSink,
    out_error: PierStrSink,
);
pub type PierCmdOutputSink =
    unsafe extern "C" fn(ctx: *mut c_void, success: bool, output: PierStr);
pub type PierBlockSink =
    unsafe extern "C" fn(ctx: *mut c_void, x: i32, y: i32, z: i32, name: PierStr, snbt: PierStr);
pub type PierEntitySink =
    unsafe extern "C" fn(ctx: *mut c_void, x: i32, y: i32, z: i32, type_: PierStr, snbt: PierStr);
pub type PierBytesSink = unsafe extern "C" fn(ctx: *mut c_void, data: *const u8, len: usize);
pub type PierKvSink = unsafe extern "C" fn(ctx: *mut c_void, key: PierStr, value: PierStr);
pub type PierActorSink =
    unsafe extern "C" fn(ctx: *mut c_void, id: PierActorId, type_name: PierStr);
pub type PierFormResultCb = unsafe extern "C" fn(user: *mut c_void, result_snbt: PierStr);
pub type PierBusCb =
    unsafe extern "C" fn(user: *mut c_void, topic: PierStr, payload: PierStr) -> bool;
pub type PierServiceCb = unsafe extern "C" fn(
    user: *mut c_void,
    name: PierStr,
    request: PierStr,
    ctx: *mut c_void,
    reply: PierStrSink,
) -> bool;
pub type PierLaneRefFn = unsafe extern "C" fn(data: *mut c_void);
pub type PierPacketCb = unsafe extern "C" fn(
    user: *mut c_void,
    ev: *const PierPacketEvent,
    edit: *mut PierPacketEdit,
    replace_ctx: *mut c_void,
    replace: PierBytesSink,
) -> i32;
pub type PierConnCb =
    unsafe extern "C" fn(user: *mut c_void, conn_id: u64, address: PierStr, opened: bool);
pub type PierKeyCb =
    unsafe extern "C" fn(user: *mut c_void, action: PierKeyAction, impact: PierFocusImpact);
pub type PierMoneyCb = unsafe extern "C" fn(
    type_: PierMoneyEvent,
    from: PierStr,
    to: PierStr,
    value: i64,
) -> bool;
