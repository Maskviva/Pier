//! Where `PierStr` and Rust strings meet, plus a few general sinks.
//!
//! Where contract §3 lands: a buffer crossing the boundary is allocated and freed by the
//! producer and read by the receiver only during the callback. Every sink here copies the
//! data out inside the callback and treats the original pointer as dead afterwards.

use core::ffi::c_void;

use crate::sys;

/// Lends a Rust string to the host to read.
///
/// # Lifetime
/// The returned `PierStr` borrows the memory of `text`. The call site must keep `text`
/// alive until the host has finished reading, meaning across that one ABI call. Every call
/// site has the shape `unsafe { (api.f)(.., s(&owned), ..) }` where `owned` is a local in
/// the same statement, and that shape is itself the guarantee.
///
/// It is not an `unsafe fn`: constructing the view performs no unsafe operation and using
/// it is what is unsafe, and the places that use it are already inside an `unsafe` block.
/// Marking it here would add a layer of noise at every call site and bury the line that
/// really needs looking at.
pub(crate) fn s(text: &str) -> sys::PierStr {
    sys::PierStr {
        ptr: text.as_ptr() as *const core::ffi::c_char,
        len: text.len(),
    }
}

/// Borrows a `PierStr` the host handed over as a `&str`.
///
/// UTF-8 is validated, because these bytes ultimately come from a client, as a player name, chat or
/// command output, and the undefined behavior of `from_utf8_unchecked` is triggered by anyone
/// setting their name to a bad byte sequence.
///
/// Invalid input is truncated at the last valid byte, keeping the borrow and allocating nothing,
/// with a one-time warning and an assertion in a debug build. Truncating rather than returning a
/// `Result`: this function runs dozens of times per tick on the event callback hot path, and making
/// every call site handle a branch that almost never happens ends with everyone writing
/// `.unwrap()`. Truncating with a warning makes bad data visible without being fatal, which is the
/// second approach of contract §5.1.
///
/// # Safety `raw` must point at memory the host guarantees valid for the current callback.
pub(crate) unsafe fn r<'a>(raw: sys::PierStr) -> &'a str {
    if raw.ptr.is_null() {
        return "";
    }
    let bytes = core::slice::from_raw_parts(raw.ptr as *const u8, raw.len);
    match core::str::from_utf8(bytes) {
        Ok(s) => s,
        Err(e) => {
            debug_assert!(false, "the bytes the host handed over are not UTF-8: {e}");
            static WARNED: core::sync::atomic::AtomicBool =
                core::sync::atomic::AtomicBool::new(false);
            if !WARNED.swap(true, core::sync::atomic::Ordering::Relaxed) {
                crate::Logger::get().warn(&format!(
                    "the string the host handed over is not valid UTF-8 from byte {} onward and was truncated; this is reported once.",
                    e.valid_up_to()
                ));
            }
            core::str::from_utf8_unchecked(&bytes[..e.valid_up_to()])
        }
    }
}

// General sinks. Every sink copies the data out inside the callback. `ctx` is the address
// of the caller's container on the stack, passed in by the adjacent `collect_*` or
// `call_out_*`, and the two have to be read as a pair.
//
// Only sinks with a caller belong here. A helper nobody has called has never had its
// `# Safety` assertion about the `ctx` type tested, which is looking ready rather than
// being ready.

/// # Safety
/// `ctx` must be a valid `*mut Vec<String>`.
/// Copies a host string. Invalid UTF-8 is not truncated: the truncation in `r()` would cut
/// an event payload in half at the first bad byte and lose the dim and the cancel bit that
/// follow. This uses `from_utf8_lossy` instead, so a bad byte becomes U+FFFD and the
/// structure survives. Anywhere that takes ownership should use it.
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
                    "the string the host handed over is not valid UTF-8 from byte {} onward; the bad bytes were replaced with U+FFFD. This is reported once.",
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
/// `ctx` must be a valid `*mut Option<String>`.
pub(crate) unsafe extern "C" fn set_string(ctx: *mut c_void, item: sys::PierStr) {
    *ctx.cast::<Option<String>>() = Some(r_owned(item));
}

/// Calls a slot that writes once into the sink on success and returns that one write.
///
/// Note that `Some("")` and `None` are two different things (contract §5.2): the first
/// means the host answered and the answer is an empty string, the second that the host
/// said the call failed. Succeeding without writing therefore becomes
/// `Some(String::new())` and not `None`.
pub(crate) fn call_out_str(
    f: impl FnOnce(*mut c_void, sys::PierStrSink) -> bool,
) -> Option<String> {
    let mut out: Option<String> = None;
    let ok = f((&mut out as *mut Option<String>).cast(), set_string);
    if ok {
        Some(out.unwrap_or_default())
    } else {
        None
    }
}

/// Calls a slot that writes zero or more times into the sink and collects every write into
/// a Vec.
pub(crate) fn collect_strs(f: impl FnOnce(*mut c_void, sys::PierStrSink)) -> Vec<String> {
    let mut out: Vec<String> = Vec::new();
    f((&mut out as *mut Vec<String>).cast(), push_string);
    out
}

// Byte sinks
//
// There was no byte-collecting helper here before, for the reason the module header
// gives: a helper with no caller has never had its `# Safety` assertion about the `ctx`
// type tested at a real call site. `nbt::to_binary` is now its first caller and the
// condition is met.

/// # Safety
/// `ctx` must be a valid `*mut Option<Vec<u8>>`.
pub(crate) unsafe extern "C" fn set_bytes(ctx: *mut c_void, data: *const u8, len: usize) {
    let slot = &mut *ctx.cast::<Option<Vec<u8>>>();
    // With `len == 0` the `data` may be NULL while `from_raw_parts(null, 0)` is still
    // undefined behavior, since it requires a non-null aligned pointer, so this branch has
    // to be taken separately.
    *slot = Some(if data.is_null() || len == 0 {
        Vec::new()
    } else {
        core::slice::from_raw_parts(data, len).to_vec()
    });
}

/// Calls a slot that writes once into a byte sink on success. The meaning matches
/// [`call_out_str`]: `Some(vec![])` is an empty answer and `None` is a failed call.
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

// Key-value sinks

/// # Safety
/// `ctx` must be a valid `*mut Vec<(String, String)>`.
pub(crate) unsafe extern "C" fn push_kv(ctx: *mut c_void, key: sys::PierStr, value: sys::PierStr) {
    (*ctx.cast::<Vec<(String, String)>>()).push((r_owned(key), r_owned(value)));
}

/// Calls a slot that writes zero or more times into a key-value sink, such as `kvdb_iter`.
pub(crate) fn collect_kv(f: impl FnOnce(*mut c_void, sys::PierKvSink)) -> Vec<(String, String)> {
    let mut out: Vec<(String, String)> = Vec::new();
    f((&mut out as *mut Vec<(String, String)>).cast(), push_kv);
    out
}

/// Collects a string slot that writes zero or more times where every entry is binary,
/// since a save key contains zero bytes.
///
/// That is the only difference from [`collect_strs`]: `level_chunk_keys` reports raw save
/// keys, and a UTF-8 conversion would corrupt one into a key that cannot be deleted, so
/// this one collects bytes.
pub(crate) fn collect_raw(f: impl FnOnce(*mut c_void, sys::PierStrSink)) -> Vec<Vec<u8>> {
    let mut out: Vec<Vec<u8>> = Vec::new();
    f((&mut out as *mut Vec<Vec<u8>>).cast(), push_raw);
    out
}

/// # Safety
/// `ctx` must be a valid `*mut Vec<Vec<u8>>`.
unsafe extern "C" fn push_raw(ctx: *mut c_void, item: sys::PierStr) {
    let bytes = if item.ptr.is_null() || item.len == 0 {
        Vec::new()
    } else {
        core::slice::from_raw_parts(item.ptr as *const u8, item.len).to_vec()
    };
    (*ctx.cast::<Vec<Vec<u8>>>()).push(bytes);
}

/// Lends a span of raw bytes to the host to read, shaped as a `PierStr`.
///
/// `level_delete_key` takes the byte string `level_chunk_keys` reported, which is not
/// UTF-8. Going through `s` would need a `&str` first, and that step is itself the
/// corruption.
pub(crate) fn s_raw(bytes: &[u8]) -> sys::PierStr {
    sys::PierStr {
        ptr: bytes.as_ptr() as *const core::ffi::c_char,
        len: bytes.len(),
    }
}
