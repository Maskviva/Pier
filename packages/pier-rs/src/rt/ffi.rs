//! `PierStr` 与 Rust 字符串之间的收口，以及几个通用的 sink。
//!
//! 契约 §三 的落地点：跨边界的缓冲区由产出方分配、产出方释放，接收方只在
//! 回调期间读。这里的每一个 sink 都在回调内把数据**拷走**，返回之后原指针
//! 就当作失效。

use core::ffi::c_void;

use crate::sys;

/// 借出一个 Rust 串给宿主读。
///
/// # 生命周期
/// 返回的 `PierStr` 借着 `text` 的内存。调用点必须保证 `text` 活到宿主读完，
/// 也就是**活过那一次 ABI 调用**。所有调用点都是
/// `unsafe { (api.f)(.., s(&owned), ..) }` 的形状，`owned` 是同一个语句里
/// 的局部变量 —— 这个形状本身就是保证。
///
/// 不返回 `unsafe fn`：构造视图本身没有不安全操作，不安全的是**用它**，
/// 而用它的地方已经在 `unsafe` 块里了。把 `unsafe` 标在这里只会让每个调用点
/// 多一层噪音，反而淹掉真正需要看的那一行。
pub(crate) fn s(text: &str) -> sys::PierStr {
    sys::PierStr {
        ptr: text.as_ptr() as *const core::ffi::c_char,
        len: text.len(),
    }
}

/// 把宿主交来的 `PierStr` 借成 `&str`。
///
/// # 为什么要校验 UTF-8
///
/// 宿主交过来的字节**最终源自客户端**（玩家名、聊天内容、命令输出）。
/// v0 这里是 `from_utf8_unchecked` —— 一个非法序列就是未定义行为，而
/// 触发它只需要有人把名字改成一段坏字节。
///
/// 现在校验。非法时截到最后一个合法字节（借用不变、不分配），记一条
/// **一次性**告警，debug 构建下直接断言。
///
/// 截断而不是返回 `Result`，是权衡后的取舍：这个函数在事件回调的热路径上
/// 每 tick 跑几十次，让每个调用点处理一个几乎不会发生的错误分支，实际结果
/// 是大家写 `.unwrap()`。截断 + 告警让坏数据**可见**且不致命 —— 契约 §5.1
/// 允许的三种做法里的第二种「打日志再回退并说清回退成了什么」。
///
/// # Safety
/// `raw` 必须指向宿主在当前回调期间保证有效的内存。
pub(crate) unsafe fn r<'a>(raw: sys::PierStr) -> &'a str {
    if raw.ptr.is_null() {
        return "";
    }
    let bytes = core::slice::from_raw_parts(raw.ptr as *const u8, raw.len);
    match core::str::from_utf8(bytes) {
        Ok(s) => s,
        Err(e) => {
            debug_assert!(false, "宿主交来的字节不是 UTF-8：{e}");
            static WARNED: core::sync::atomic::AtomicBool =
                core::sync::atomic::AtomicBool::new(false);
            if !WARNED.swap(true, core::sync::atomic::Ordering::Relaxed) {
                crate::Logger::get().warn(&format!(
                    "宿主交来的字符串不是合法 UTF-8（第 {} 字节起），已截断；这条只报一次。",
                    e.valid_up_to()
                ));
            }
            core::str::from_utf8_unchecked(&bytes[..e.valid_up_to()])
        }
    }
}

// ── 通用 sink ─────────────────────────────────────────────────────
//
// 每个 sink 都在回调内把数据拷走。`ctx` 是调用方栈上那个容器的地址，
// 由紧挨着的 `collect_*` / `call_out_*` 传进去 —— 这两对函数必须成对读，
// 拆开看的话 `ctx` 的类型就是凭空断言的。
//
// **这里只放有调用方的 sink。** 上一版还有一对收字节的（`collect_bytes` /
// `collect_byte_chunks`），一个调用方都没有，`cargo clippy -D warnings` 全
// 报 `never used`。它们已被删除，等第一个字节 sink 的域（NBT 二进制、
// 数据包体）落地时再回来 —— 一个没有调用方的 helper，它对 `ctx` 类型的
// 那些 `# Safety` 断言从来没有被任何真实调用点检验过。

/// # Safety
/// `ctx` 必须是一个有效的 `*mut Vec<String>`。
pub(crate) unsafe extern "C" fn push_string(ctx: *mut c_void, item: sys::PierStr) {
    (*ctx.cast::<Vec<String>>()).push(r(item).to_owned());
}

/// # Safety
/// `ctx` 必须是一个有效的 `*mut Option<String>`。
pub(crate) unsafe extern "C" fn set_string(ctx: *mut c_void, item: sys::PierStr) {
    *ctx.cast::<Option<String>>() = Some(r(item).to_owned());
}

/// 调一个「成功就往 sink 里写一次」的槽，把那一次写取回来。
///
/// 注意 `Some("")` 和 `None` 是**两件事**（契约 §5.2）：前者是「宿主回答了，
/// 答案是空串」，后者是「宿主说这次调用失败了」。所以成功但没写的情况归成
/// `Some(String::new())` 而不是 `None`。
pub(crate) fn call_out_str(f: impl FnOnce(*mut c_void, sys::PierStrSink) -> bool) -> Option<String> {
    let mut out: Option<String> = None;
    let ok = f((&mut out as *mut Option<String>).cast(), set_string);
    if ok {
        Some(out.unwrap_or_default())
    } else {
        None
    }
}

/// 调一个「往 sink 里写零到多次」的槽，把全部写入收成一个 Vec。
pub(crate) fn collect_strs(f: impl FnOnce(*mut c_void, sys::PierStrSink)) -> Vec<String> {
    let mut out: Vec<String> = Vec::new();
    f((&mut out as *mut Vec<String>).cast(), push_string);
    out
}
