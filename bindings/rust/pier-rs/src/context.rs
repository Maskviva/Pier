//! 交给模组生命周期回调的上下文。
//!
//! 它住在这里而不是 `rt` 里，是因为它是**给模组作者的门面** —— 一个把各个
//! 域的入口聚到一处的地方。`rt` 是二十几个域建在其上的地基，地基不该认识
//! 建在它上面的东西。
//!
//! 这个位置是被一次真实事故纠正过来的：为了拆 `rt ↔ host` 的环，
//! `ctx.host()` / `ctx.packets()` 曾被直接删掉。环是真的，但砍掉模组作者在用
//! 的访问器是错的解法 —— 该动的是 `ModContext` 的位置，不是它的 API 面。

use crate::rt::runtime::rt;

/// 交给模组生命周期回调的上下文。
///
/// 它本身不带状态（真正的状态在 `RUNTIME` 里），存在的意义是给各个门面
/// 一个统一的入口，顺便让 `on_load(ctx)` 这样的签名读起来像回事。
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

    /// 宿主与系统层面的能力（运行阶段、排期、执行命令、协议版本…）。
    pub fn host(&self) -> crate::Host {
        crate::Host::get()
    }

    /// 数据包门面。
    pub fn packets(&self) -> crate::Packets {
        crate::Packets::get()
    }

    /// 世界门面。
    pub fn world(&self) -> crate::World {
        crate::World::get()
    }

    /// 服务器运行时控制（tick 冻结、倍速、性能采样）。
    pub fn server(&self) -> crate::Server {
        crate::Server::get()
    }

    /// 宿主是按客户端目标编的吗。
    ///
    /// 一般用不到 —— 装错目标的模组在握手阶段就被宿主拒绝了。留着是为了让
    /// 同一份代码能在两个目标上做细微的行为区分，而不必靠编译期 feature。
    pub fn host_is_client(&self) -> bool {
        rt().api.is_client_host()
    }

    /// 宿主的 ABI 版本与表长度。诊断用：报「这个功能你的 pier 太老」时，
    /// 带上这两个数才能让人知道该升到多少。
    pub fn host_abi(&self) -> (u32, usize) {
        let r = rt();
        (r.api.abi_version, r.host_struct_size)
    }
}
