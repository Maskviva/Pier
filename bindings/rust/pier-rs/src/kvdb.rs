//! 键值库 —— 模组自己的持久化存储。
//!
//! # 这一族是**线程安全**的
//!
//! ABI 上 `kvdb_*` 明确标了内部有锁（契约 §四的例外之一），任何线程都能调。
//! 其余的域几乎都只能在服务器线程上用，这一族是少数几个能在
//! `std::thread::spawn` 里直接用的。
//!
//! # 路径被圈在模组自己的数据目录里
//!
//! `..` 和绝对路径由宿主拒绝。句柄由宿主持有，模组卸载时强制关闭并打一条
//! 告警 —— 所以忘了关不会丢数据，但会在日志里留下痕迹。

use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, collect_kv, s};
use crate::rt::runtime::rt;
use crate::sys;

/// 一个打开着的键值库。**Drop 即关闭。**
pub struct KvDb {
    handle: sys::PierKvDbHandle,
    path: String,
}

// SAFETY：ABI 上这一族标了线程安全（宿主侧有互斥锁）。这里断言的只是
// 「句柄这个值可以跨线程搬并使用」，而那正是宿主承诺的那条性质。
unsafe impl Send for KvDb {}
unsafe impl Sync for KvDb {}

impl KvDb {
    /// 打开，不存在就建。
    pub fn open(path: &str) -> Result<KvDb> {
        KvDb::open_inner(path, true)
    }

    /// 只打开已存在的。库不存在时是 `Err`，不会悄悄建一个空的 ——
    /// 「读一个本该有数据的库」和「建一个新库」是两件事。
    pub fn open_existing(path: &str) -> Result<KvDb> {
        KvDb::open_inner(path, false)
    }

    fn open_inner(path: &str, create: bool) -> Result<KvDb> {
        let f = crate::require_slot!(kvdb_open, "打开键值库");
        let handle = unsafe { f(rt().handle(), s(path), create) };
        if handle.is_null() {
            return Err(Error(format!(
                "打开不了键值库 {path}（路径越出了模组数据目录、含 `..`，{}）",
                if create { "或磁盘不可写" } else { "或它还不存在" }
            )));
        }
        Ok(KvDb {
            handle,
            path: path.to_owned(),
        })
    }

    pub fn path(&self) -> &str {
        &self.path
    }

    /// 读一个键。键不存在时是 `None`。
    ///
    /// 两道闸都要走，即使 `open` 已经成功过：`kvdb_get` 在表里排在
    /// `kvdb_open` **后面**，偏移更大，前者覆盖得到不蕴含后者覆盖得到。
    pub fn get(&self, key: &str) -> Option<String> {
        if !crate::has_slot!(kvdb_get) {
            return None;
        }
        let f = crate::__rt::api().kvdb_get?;
        call_out_str(|ctx, sink| unsafe { f(self.handle, s(key), ctx, sink) })
    }

    pub fn set(&self, key: &str, value: &str) -> Result<()> {
        let f = crate::require_slot!(kvdb_set, "写入键值库");
        if unsafe { f(self.handle, s(key), s(value)) } {
            Ok(())
        } else {
            Err(Error(format!("写不进键值库 {} 的键 {key}", self.path)))
        }
    }

    /// 删一个键。键本来就不存在时也算成功。
    pub fn del(&self, key: &str) -> Result<()> {
        let f = crate::require_slot!(kvdb_del, "删除键");
        if unsafe { f(self.handle, s(key)) } {
            Ok(())
        } else {
            Err(Error(format!("删不掉键值库 {} 的键 {key}", self.path)))
        }
    }

    pub fn has(&self, key: &str) -> bool {
        if !crate::has_slot!(kvdb_has) {
            return false;
        }
        match crate::__rt::api().kvdb_has {
            Some(f) => unsafe { f(self.handle, s(key)) },
            None => false,
        }
    }

    pub fn is_empty(&self) -> bool {
        if !crate::has_slot!(kvdb_is_empty) {
            return true;
        }
        match crate::__rt::api().kvdb_is_empty {
            Some(f) => unsafe { f(self.handle) },
            None => true,
        }
    }

    /// 全部键值对。整库都读进内存，大库慎用。
    pub fn iter(&self) -> Vec<(String, String)> {
        if !crate::has_slot!(kvdb_iter) {
            return Vec::new();
        }
        let Some(f) = crate::__rt::api().kvdb_iter else {
            return Vec::new();
        };
        collect_kv(|ctx, sink| unsafe { f(self.handle, ctx, sink) })
    }
}

impl Drop for KvDb {
    fn drop(&mut self) {
        if !crate::has_slot!(kvdb_close) {
            // 打得开却关不上：宿主在模组卸载时会强制关闭并打一条告警，
            // 所以这里不再重复报，但也绝不能去读一个够不着的槽。
            return;
        }
        if let Some(f) = crate::__rt::api().kvdb_close {
            unsafe { f(self.handle) };
        }
    }
}

impl std::fmt::Debug for KvDb {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "KvDb({})", self.path)
    }
}
