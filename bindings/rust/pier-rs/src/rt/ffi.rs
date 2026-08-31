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
/// 校验 UTF-8，因为这些字节**最终源自客户端**（玩家名、聊天、命令输出）：
/// `from_utf8_unchecked` 的未定义行为只要有人把名字改成一段坏字节就触发。
///
/// 非法时截到最后一个合法字节（借用不变、不分配），记一条**一次性**告警，
/// debug 构建下断言。截断而不是返回 `Result`：这个函数在事件回调热路径上
/// 每 tick 跑几十次，让每个调用点处理一个几乎不发生的分支，实际结果是大家
/// 写 `.unwrap()`。截断 + 告警让坏数据**可见**且不致命 —— 契约 §5.1 的
/// 第二种做法。
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
// 每个 sink 都在回调内把数据拷走。`ctx` 是调用方栈上那个容器的地址，由紧挨
// 着的 `collect_*` / `call_out_*` 传进去 —— 两者必须成对读。
//
// **只放有调用方的 sink。** 没人调过的 helper，它对 `ctx` 类型的 `# Safety`
// 断言就从来没被检验过 —— 那不是「准备好了」，是「看起来准备好了」。

/// # Safety
/// `ctx` 必须是一个有效的 `*mut Vec<String>`。
/// 拷贝一份宿主字符串。非法 UTF-8 **不截断**：`r()` 的截断会把一条事件载荷
/// 在第一个坏字节处砍成两半，其后的 dim/取消位全部丢失（V-19）；这里改成
/// `from_utf8_lossy`，坏字节变成 U+FFFD，结构保住。凡是要拿走所有权的地方
/// 都该用它。
pub(crate) unsafe fn r_owned(raw: sys::PierStr) -> String {
    if raw.ptr.is_null() {
        return String::new();
    }
    let bytes = core::slice::from_raw_parts(raw.ptr as *const u8, raw.len);
    match core::str::from_utf8(bytes) {
        Ok(s) => s.to_owned(),
        Err(e) => {
            static WARNED: core::sync::atomic::AtomicBool =
                core::sync::atomic::AtomicBool::new(false);
            if !WARNED.swap(true, core::sync::atomic::Ordering::Relaxed) {
                crate::Logger::get().warn(&format!(
                    "宿主交来的字符串不是合法 UTF-8（第 {} 字节起），坏字节已替换为 U+FFFD；这条只报一次。",
                    e.valid_up_to()
                ));
            }
            String::from_utf8_lossy(bytes).into_owned()
        }
    }
}

pub(crate) unsafe extern "C" fn push_string(ctx: *mut c_void, item: sys::PierStr) {
    (*ctx.cast::<Vec<String>>()).push(r_owned(item));
}

/// # Safety
/// `ctx` 必须是一个有效的 `*mut Option<String>`。
pub(crate) unsafe extern "C" fn set_string(ctx: *mut c_void, item: sys::PierStr) {
    *ctx.cast::<Option<String>>() = Some(r_owned(item));
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

// ── 字节 sink ─────────────────────────────────────────────────────
//
// 上一版这里没有收字节的 helper，理由写在模块头：没有调用方的 helper，它对
// `ctx` 类型的 `# Safety` 断言从来没被任何真实调用点检验过。现在
// `nbt::to_binary` 是它的第一个调用方，条件满足了。

/// # Safety
/// `ctx` 必须是一个有效的 `*mut Option<Vec<u8>>`。
pub(crate) unsafe extern "C" fn set_bytes(ctx: *mut c_void, data: *const u8, len: usize) {
    let slot = &mut *ctx.cast::<Option<Vec<u8>>>();
    // `len == 0` 时 `data` 允许为 NULL，而 `from_raw_parts(null, 0)` 仍然是
    // 未定义行为（它要求指针非空且对齐），所以这一支必须单独走。
    *slot = Some(if data.is_null() || len == 0 {
        Vec::new()
    } else {
        core::slice::from_raw_parts(data, len).to_vec()
    });
}

/// 调一个「成功就往字节 sink 里写一次」的槽。语义同 [`call_out_str`]：
/// `Some(vec![])` 是「答案是空」，`None` 是「这次调用失败了」。
pub(crate) fn call_out_bytes(
    f: impl FnOnce(*mut c_void, sys::PierBytesSink) -> bool,
) -> Option<Vec<u8>> {
    let mut out: Option<Vec<u8>> = None;
    let ok = f((&mut out as *mut Option<Vec<u8>>).cast(), set_bytes);
    if ok {
        Some(out.unwrap_or_default())
    } else {
        None
    }
}

// ── 键值 sink ─────────────────────────────────────────────────────

/// # Safety
/// `ctx` 必须是一个有效的 `*mut Vec<(String, String)>`。
pub(crate) unsafe extern "C" fn push_kv(
    ctx: *mut c_void,
    key: sys::PierStr,
    value: sys::PierStr,
) {
    (*ctx.cast::<Vec<(String, String)>>()).push((r_owned(key), r_owned(value)));
}

/// 调一个「往键值 sink 里写零到多次」的槽（`kvdb_iter`）。
pub(crate) fn collect_kv(f: impl FnOnce(*mut c_void, sys::PierKvSink)) -> Vec<(String, String)> {
    let mut out: Vec<(String, String)> = Vec::new();
    f((&mut out as *mut Vec<(String, String)>).cast(), push_kv);
    out
}

/// 收一个「零到多次写入」的字符串槽，但**每一条都是二进制**（存档键含 0 字节）。
///
/// 和 [`collect_strs`] 的区别只在这里：`level_chunk_keys` 报的是原始存档键，
/// 走 UTF-8 转换会把它损坏成一个删不掉的键。所以这一条按字节收。
pub(crate) fn collect_raw(f: impl FnOnce(*mut c_void, sys::PierStrSink)) -> Vec<Vec<u8>> {
    let mut out: Vec<Vec<u8>> = Vec::new();
    f((&mut out as *mut Vec<Vec<u8>>).cast(), push_raw);
    out
}

/// # Safety
/// `ctx` 必须是一个有效的 `*mut Vec<Vec<u8>>`。
unsafe extern "C" fn push_raw(ctx: *mut c_void, item: sys::PierStr) {
    let bytes = if item.ptr.is_null() || item.len == 0 {
        Vec::new()
    } else {
        core::slice::from_raw_parts(item.ptr as *const u8, item.len).to_vec()
    };
    (*ctx.cast::<Vec<Vec<u8>>>()).push(bytes);
}

/// 借出一段原始字节给宿主读，形状是 `PierStr`。
///
/// `level_delete_key` 吃的是 `level_chunk_keys` 报出来的那串字节，它不是 UTF-8。
/// 走 `s` 需要先有个 `&str`，而那一步本身就是损坏。
pub(crate) fn s_raw(bytes: &[u8]) -> sys::PierStr {
    sys::PierStr {
        ptr: bytes.as_ptr() as *const core::ffi::c_char,
        len: bytes.len(),
    }
}
