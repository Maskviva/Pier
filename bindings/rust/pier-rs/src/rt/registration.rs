//! 模组侧的握手与生命周期围栏。
//!
//! # 握手：三道检查，顺序有讲究
//!
//! 和宿主侧（`pier-host/src/ModHost.cpp`）必须一致：
//!
//! 1. **长度** —— 表短到连核心槽都不覆盖时，后两项无从谈起（读版本号本身
//!    也是读那块内存）。
//! 2. **版本区间** —— `MIN_SUPPORTED <= 我 <= 宿主`。兼容是区间不是相等（§2.2）。
//! 3. **目标标志** —— `host_flags` 与 `mod_flags` 的 bit 0 必须相等。
//!
//! 三个生命周期回调都在 `catch_unwind` 里：panic 穿过 `extern "C"` 是未定义
//! 行为，这里把它拦成一个 `false` 加一条日志，让宿主走正常的回滚路径。

use std::panic::{catch_unwind, AssertUnwindSafe};
use std::sync::Mutex;

use crate::context::ModContext;
use crate::rt::error::{Error, Result};
use crate::rt::logger::Logger;
use crate::sys;

/// 一个 Pier 模组。
///
/// `Send` 是 `ModSlot<T>`（`Mutex<Option<T>>`）真正 `Sync` 的前提：生命周期
/// 回调可能在不同线程上进入 —— 宿主允许 `unload` 与跨模组服务调用来自
/// 别的线程。
pub trait LeviMod: Sized + Send + 'static {
    /// 装载。返回 Err 会让宿主判定装载失败并回滚（拆除步骤照常跑）。
    fn on_load(ctx: &ModContext) -> Result<Self>;

    fn on_enable(&mut self, _ctx: &ModContext) -> Result<()> {
        Ok(())
    }
    fn on_disable(&mut self, _ctx: &ModContext) -> Result<()> {
        Ok(())
    }
    fn on_unload(&mut self, _ctx: &ModContext) -> Result<()> {
        Ok(())
    }
}

#[doc(hidden)]
pub struct ModSlot<T: LeviMod>(pub Mutex<Option<T>>);

/// 握手。成功返回 true，失败**只返回 false**，不 panic。
///
/// 失败时尽量打一条说得清的日志 —— 但注意：日志本身要走 `api.log`，而这里
/// 恰恰是在判断这个 api 能不能信。所以只有在长度检查过了之后才敢用它，
/// 更早的失败只能靠宿主那边报（宿主会打「模组 X 的 vtable 声明长度不足」）。
///
/// # Safety
/// 由 `register_mod!` 展开的 `pier_main` 调用，`api` 来自宿主。
#[doc(hidden)]
pub unsafe fn __init_runtime(api: *const sys::PierApi, handle: sys::PierModHandle) -> bool {
    if api.is_null() {
        return false;
    }
    let api: &'static sys::PierApi = &*api;

    // ── 闸一：长度 ─────────────────────────────────────────────
    //
    // 判据是「表头四个标量 + 我们真正会无条件用到的那几个核心槽」。
    // 取 `log` 的偏移加一个指针宽：只要覆盖得到 log，后面的失败就都能说话。
    let core_min = core::mem::offset_of!(sys::PierApi, log) + core::mem::size_of::<usize>();
    if (api.struct_size as usize) < core_min {
        return false;
    }

    // ── 闸二：版本区间 ─────────────────────────────────────────
    //
    // 宿主比模组新是允许的（表更长，模组够不到的部分不碰）；宿主比模组老
    // 到低于本 crate 编译时的版本，说明表布局可能已经不是我们的前缀了。
    if api.abi_version < sys::PIER_ABI_MIN_SUPPORTED {
        return false;
    }
    if sys::PIER_ABI_VERSION > api.abi_version {
        return false;
    }

    // ── 闸三：目标标志 ─────────────────────────────────────────
    //
    // 宿主那边也会查一遍（拿 vtable 里的 mod_flags）。两边都查是有意的：
    // 这边查能让模组在自己的日志里说清楚，那边查是**权威**判定 ——
    // 一个恶意或者写错的模组可以不查，宿主不能不查。
    if (api.host_flags & sys::PIER_FLAG_CLIENT) != mod_flags() & sys::PIER_FLAG_CLIENT {
        return false;
    }

    crate::rt::runtime::set_runtime(api, handle)
}

/// 本次构建的 `mod_flags`。
///
/// 这是 `client` feature 在 v1 里**唯一**的作用 —— 它不增删任何 API
/// （契约 §2.1：布局在所有目标下相同）。
#[doc(hidden)]
pub const fn mod_flags() -> u32 {
    #[cfg(feature = "client")]
    {
        sys::PIER_FLAG_CLIENT
    }
    #[cfg(not(feature = "client"))]
    {
        0
    }
}

#[doc(hidden)]
pub fn __lifecycle<T: LeviMod>(slot: &'static ModSlot<T>, stage: u8) -> bool {
    let ctx = ModContext::new();
    let run = || -> Result<()> {
        let mut guard = slot
            .0
            .lock()
            .map_err(|_| Error("模组状态已被毒化（上一次回调 panic 过）".into()))?;
        let Some(instance) = guard.as_mut() else {
            return Err(Error("模组实例不存在（重复卸载？）".into()));
        };
        match stage {
            1 => instance.on_enable(&ctx),
            2 => instance.on_disable(&ctx),
            3 => {
                instance.on_unload(&ctx)?;
                // 先跑完 on_unload 再丢实例。反过来的话，用户在 on_unload
                // 里拿 &mut self 做的收尾就没地方做了。
                *guard = None;
                Ok(())
            }
            _ => Ok(()),
        }
    };
    match catch_unwind(AssertUnwindSafe(run)) {
        Ok(Ok(())) => true,
        Ok(Err(e)) => {
            Logger::get().error(&format!("生命周期回调（阶段 {stage}）失败：{e}"));
            false
        }
        Err(_) => {
            Logger::get().error(&format!(
                "生命周期回调（阶段 {stage}）panic 了。已就地拦下 —— \
                 panic 穿过 extern \"C\" 边界是未定义行为。"
            ));
            false
        }
    }
}

#[doc(hidden)]
pub fn __load<T: LeviMod>(slot: &'static ModSlot<T>) -> bool {
    let ctx = ModContext::new();
    match catch_unwind(AssertUnwindSafe(|| T::on_load(&ctx))) {
        Ok(Ok(instance)) => {
            // `lock()` 在这里不可能被毒化：这是第一次碰这把锁。
            match slot.0.lock() {
                Ok(mut g) => {
                    *g = Some(instance);
                    true
                }
                Err(_) => {
                    Logger::get().error("模组状态槽在装载时已被毒化 —— 判定装载失败");
                    false
                }
            }
        }
        Ok(Err(e)) => {
            Logger::get().error(&format!("on_load 失败：{e}"));
            false
        }
        Err(_) => {
            Logger::get().error("on_load panic 了。已就地拦下。");
            false
        }
    }
}

/// 生成 `pier_main` 入口。一个模组写一次。
///
/// ```ignore
/// struct MyMod;
/// impl LeviMod for MyMod { /* ... */ }
/// levilamina::register_mod!(MyMod);
/// ```
#[macro_export]
macro_rules! register_mod {
    ($ty:ty) => {
        #[doc(hidden)]
        static __PIER_SLOT: $crate::ModSlot<$ty> = $crate::ModSlot(::std::sync::Mutex::new(None));

        /// 宿主只找这一个符号，找不到就明确拒绝装载（契约 §2.4）。
        #[no_mangle]
        pub unsafe extern "C" fn pier_main(
            api: *const $crate::sys::PierApi,
            handle: $crate::sys::PierModHandle,
            out: *mut $crate::sys::PierModVTable,
        ) -> bool {
            if out.is_null() || !$crate::__init_runtime(api, handle) {
                return false;
            }
            if !$crate::__load::<$ty>(&__PIER_SLOT) {
                return false;
            }
            unsafe extern "C" fn on_enable(_: *mut ::core::ffi::c_void) -> bool {
                $crate::__lifecycle::<$ty>(&__PIER_SLOT, 1)
            }
            unsafe extern "C" fn on_disable(_: *mut ::core::ffi::c_void) -> bool {
                $crate::__lifecycle::<$ty>(&__PIER_SLOT, 2)
            }
            unsafe extern "C" fn on_unload(_: *mut ::core::ffi::c_void) -> bool {
                $crate::__lifecycle::<$ty>(&__PIER_SLOT, 3)
            }
            // `struct_size` 由**模组**填自己编出来的长度，宿主只读这个长度
            // 以内的字段 —— vtable 从此也能追加回调而不升 ABI 版本。
            (*out) = $crate::sys::PierModVTable {
                struct_size: ::core::mem::size_of::<$crate::sys::PierModVTable>() as u32,
                abi_version: $crate::sys::PIER_ABI_VERSION,
                mod_flags: $crate::__rt::mod_flags(),
                _reserved0: 0,
                instance: ::core::ptr::null_mut(),
                on_enable: Some(on_enable),
                on_disable: Some(on_disable),
                on_unload: Some(on_unload),
            };
            true
        }
    };
}
