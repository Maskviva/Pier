//! The context handed to a mod's lifecycle callbacks.
//!
//! It lives here and not in `rt` because it is the facade for mod authors, a place that
//! gathers the entry points of every domain. `rt` is the foundation the twenty-odd domains
//! are built on, and a foundation should not know what is built on it.
//!
//! A real incident corrected this placement: to break the cycle between `rt` and `host`,
//! `ctx.host()` and `ctx.packets()` were deleted outright. The cycle was real and cutting
//! accessors mod authors were using was the wrong fix: what had to move was the position of
//! `ModContext` and not its API surface.

use crate::rt::runtime::rt;

/// The context handed to a mod's lifecycle callbacks.
///
/// It carries no state of its own, since the real state lives in `RUNTIME`. It exists to
/// give the facades one common entry point, and to make a signature such as
/// `on_load(ctx)` read sensibly.
pub struct ModContext(());

impl ModContext {
    pub(crate) fn new() -> ModContext {
        ModContext(())
    }
}

impl ModContext {
    pub fn logger(&self) -> crate::Logger {
        crate::Logger::get()
    }

    /// Capabilities at the host and system level: the run stage, scheduling, executing
    /// commands and the protocol version.
    pub fn host(&self) -> crate::Host {
        crate::Host::get()
    }

    /// The packet facade.
    pub fn packets(&self) -> crate::Packets {
        crate::Packets::get()
    }

    /// The world facade.
    pub fn world(&self) -> crate::World {
        crate::World::get()
    }

    /// Server runtime control: freezing and warping ticks, and performance sampling.
    pub fn server(&self) -> crate::Server {
        crate::Server::get()
    }

    /// Whether the host was built for the client target.
    ///
    /// Rarely needed, since a mod loaded onto the wrong target is refused by the host during
    /// the handshake. It exists so that one source can make a small behavioral distinction
    /// between the two targets without a compile-time feature.
    pub fn host_is_client(&self) -> bool {
        rt().api.is_client_host()
    }

    /// The ABI version and table length of the host. For diagnostics: reporting that a pier is
    /// too old for a feature only tells someone how far to upgrade when these two numbers come
    /// with it.
    pub fn host_abi(&self) -> (u32, usize) {
        let r = rt();
        (r.api.abi_version, r.host_struct_size)
    }
}
