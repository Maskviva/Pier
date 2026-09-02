//! The key-value store: a mod's own persistent storage.
//!
//! # This family is thread safe
//!
//! The ABI marks `kvdb_*` as internally locked, one of the exceptions of contract §4, so any thread
//! may call it. Almost every other domain works on the server thread alone, and this is one of the
//! few usable straight from a `std::thread::spawn`.
//!
//! # Paths are confined to the mod's own data directory
//!
//! The host refuses `..` and an absolute path. The host owns the handle, force-closes it when the
//! mod unloads and warns, so forgetting to close loses no data while leaving a trace in the log.

use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, collect_kv, s};
use crate::rt::runtime::rt;
use crate::sys;

/// One open key-value store. Dropping it closes it.
pub struct KvDb {
    handle: sys::PierKvDbHandle,
    path: String,
}

// SAFETY: the ABI marks this family thread safe, with a mutex on the host side. What is
// asserted here is only that the handle value may be moved across threads and used,
// which is exactly the property the host promises.
unsafe impl Send for KvDb {}
unsafe impl Sync for KvDb {}

impl KvDb {
    /// Opens it, creating it when it does not exist.
    pub fn open(path: &str) -> Result<KvDb> {
        KvDb::open_inner(path, true)
    }

    /// Opens an existing one only. A store that does not exist is an `Err` and no empty one is
    /// quietly created: reading a store that should hold data and creating a new one are two
    /// different things.
    pub fn open_existing(path: &str) -> Result<KvDb> {
        KvDb::open_inner(path, false)
    }

    fn open_inner(path: &str, create: bool) -> Result<KvDb> {
        let f = crate::require_slot!(kvdb_open, "opening a key-value store");
        let handle = unsafe { f(rt().handle(), s(path), create) };
        if handle.is_null() {
            return Err(Error(format!(
                "the key-value store {path} could not be opened: the path leaves the mod data directory, contains `..`, {}",
                if create {
                    "or the disk is not writable"
                } else {
                    "or it does not exist yet"
                }
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

    /// Reads one key. A key that does not exist gives `None`.
    ///
    /// Both gates apply even after `open` has succeeded: `kvdb_get` sits after `kvdb_open` in
    /// the table at a larger offset, and the first being covered does not imply the second is.
    pub fn get(&self, key: &str) -> Option<String> {
        if !crate::has_slot!(kvdb_get) {
            return None;
        }
        let f = crate::__rt::api().kvdb_get?;
        call_out_str(|ctx, sink| unsafe { f(self.handle, s(key), ctx, sink) })
    }

    pub fn set(&self, key: &str, value: &str) -> Result<()> {
        let f = crate::require_slot!(kvdb_set, "writing to a key-value store");
        if unsafe { f(self.handle, s(key), s(value)) } {
            Ok(())
        } else {
            Err(Error(format!(
                "the key {key} could not be written to the key-value store {}",
                self.path
            )))
        }
    }

    /// Deletes one key. A key that never existed also counts as a success.
    pub fn del(&self, key: &str) -> Result<()> {
        let f = crate::require_slot!(kvdb_del, "deleting a key");
        if unsafe { f(self.handle, s(key)) } {
            Ok(())
        } else {
            Err(Error(format!(
                "the key {key} could not be deleted from the key-value store {}",
                self.path
            )))
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

    /// Every key-value pair. The whole store is read into memory, so a large one needs care.
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
            // It opened and cannot be closed. The host force-closes it when the mod unloads and
            // warns, so this does not report it again, and it must never read a slot it cannot
            // reach.
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
