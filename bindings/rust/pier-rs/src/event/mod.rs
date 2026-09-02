//! 事件：订阅、读载荷、改载荷、取消。
//!
//! # 「没有这个键」和「它是 0」必须分开
//!
//! 契约 §5.1 记着一次土地保护绕过：自定义维度的事件读不到 `dim`，消费方
//! `unwrap_or(0)` 把它当成主世界，零日志放行。所以 [`Event`] 给的是**类型化
//! 取值**，缺键和类型不符是两种不同的错误。
//!
//! 还要认 `_unresolved`：宿主解不出事件来源时会注入这个标记，[`Event::dim`]
//! 一类方法遇到它返回 `Err`，安全判定据此 fail-closed，而不是拿一个编出来的
//! 0 继续走。
//!
//! [`Wiring`] 做链式批量订阅，句柄由它统一持有。

pub mod names;

use std::collections::BTreeMap;
use std::ffi::c_void;

use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{r, s};
use crate::rt::logger::Logger;
use crate::rt::runtime::rt;
use crate::sys;

/// 派发优先级。数值小的先跑（与 ABI 的 0..4 对齐）。
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Default)]
pub enum Priority {
    Highest = 0,
    High = 1,
    #[default]
    Normal = 2,
    Low = 3,
    Lowest = 4,
}

impl Priority {
    fn raw(self) -> i32 {
        self as i32
    }
}

/// 事件里的玩家身份。
///
/// 三个字段各有各的用处，别混：
/// * `xuid` —— **唯一且不可改**，做权限/经济的键只能用它（离线模式下可能为空）；
/// * `uuid` —— 同样稳定，适合做存档键；
/// * `name` —— 展示用。玩家可以改显示名，宿主的名字解析在账号名落空时会退到
///   显示名（见 `bridge::resolvePlayer`），所以**不要拿它当身份**。
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct PlayerIdentity {
    pub name: String,
    pub xuid: String,
    pub uuid: String,
}

impl PlayerIdentity {
    /// 拿一个能用来调 API 的选择器。
    ///
    /// **优先 xuid**：它不可伪造。xuid 空（离线模式）才退到 uuid，再退到名字。
    /// 退到 `Name` 时 [`PlayerSel::is_stable`] 会是 false —— 权限/经济判定
    /// 看到它应当提高警惕（名字会走显示名回退，见 `sel` 模块文档）。
    pub fn selector(&self) -> crate::sel::PlayerSel {
        use crate::sel::PlayerSel;
        if !self.xuid.is_empty() {
            PlayerSel::Xuid(self.xuid.clone())
        } else if !self.uuid.is_empty() {
            PlayerSel::Uuid(self.uuid.clone())
        } else {
            PlayerSel::Name(self.name.clone())
        }
    }

    /// 有没有可靠身份（xuid 或 uuid）。做权限键之前先问这个。
    pub fn is_identified(&self) -> bool {
        !self.xuid.is_empty() || !self.uuid.is_empty()
    }
}

/// 一次事件派发。回调里拿到的就是它。
///
/// 生命周期只在回调期间有效 —— 不要把它存起来（也存不住，带着生命周期参数）。
pub struct Event<'a> {
    id: &'a str,
    snbt: &'a str,
    /// 懒解析：只观察 id 的监听器（很多）不该为解析 SNBT 付钱。
    parsed: Option<NbtValue>,
    edited: Option<NbtValue>,
    write_ctx: *mut c_void,
    write_back: sys::PierStrSink,
}

impl<'a> Event<'a> {
    /// 事件 id，例如 `"ll::event::player::PlayerChatEvent"` 或合成事件
    /// `"BlockDestroyEvent"`。
    pub fn id(&self) -> &str {
        self.id
    }

    /// 原始载荷 SNBT。调试时最直接。
    pub fn snbt(&self) -> &str {
        self.snbt
    }

    /// 解析后的载荷。第一次调用时解析，之后复用。
    pub fn value(&mut self) -> Result<&NbtValue> {
        if self.parsed.is_none() {
            self.parsed = Some(NbtValue::parse(self.snbt).map_err(Error::from)?);
        }
        Ok(self.parsed.as_ref().expect("刚刚填过"))
    }

    /// 载荷解析不了时返回 `None` 而不是报错 —— 给「解不出来就当没这个事件」
    /// 的观察型监听器用。
    pub fn value_opt(&mut self) -> Option<&NbtValue> {
        self.value().ok()
    }

    /* ───────────── 类型化取值：把 rsw 的 payload.rs 收进来 ───────────── */

    /// 按路径取字符串。缺键/类型不符都会说清楚是哪个键。
    pub fn str_at(&mut self, path: &str) -> Result<&str> {
        let v = self.value()?;
        v.get_str(path).map_err(Error::from)
    }

    pub fn i64_at(&mut self, path: &str) -> Result<i64> {
        let v = self.value()?;
        v.get_i64(path).map_err(Error::from)
    }

    pub fn i32_at(&mut self, path: &str) -> Result<i32> {
        let v = self.value()?;
        v.get_i32(path).map_err(Error::from)
    }

    pub fn f64_at(&mut self, path: &str) -> Result<f64> {
        let v = self.value()?;
        v.get_f64(path).map_err(Error::from)
    }

    pub fn bool_at(&mut self, path: &str) -> Result<bool> {
        let v = self.value()?;
        v.get_bool(path).map_err(Error::from)
    }

    /// 宽松版：拿不到就 `None`，不解释原因。只在「拿不到也无所谓」时用。
    pub fn opt_str(&mut self, path: &str) -> Option<String> {
        self.value_opt()?.opt_str(path).map(str::to_owned)
    }

    pub fn opt_i64(&mut self, path: &str) -> Option<i64> {
        self.value_opt()?.opt_i64(path)
    }

    /* ───────────── 常用字段：形状差异在这里收口 ───────────── */

    /// 宿主解不出来的字段名单（`_unresolved`）。
    ///
    /// 宿主在事件里带 Actor 桩、但那个桩既不是在线玩家、也不在运行时实体表里
    /// 时，会把字段名记在这里（宿主侧 V-04）。**它不为空就意味着这条载荷不完整**，
    /// 安全判定应当拒绝而不是猜。
    pub fn unresolved(&mut self) -> Vec<String> {
        let Some(v) = self.value_opt() else {
            return Vec::new();
        };
        v.path("_unresolved")
            .and_then(NbtValue::as_list)
            .map(|l| {
                l.iter()
                    .filter_map(|x| x.as_str().map(str::to_owned))
                    .collect()
            })
            .unwrap_or_default()
    }

    /// 检查载荷是否完整（`_unresolved` 为空）。
    pub fn check_complete(&mut self) -> bool {
        self.unresolved().is_empty()
    }

    /// 事件发生在哪个维度。
    ///
    /// **读不到就是 `Err`，绝不返回 0。** 这一条是整个模块存在的理由：上一代
    /// 让调用方写 `payload.i32_at("dim").unwrap_or(0)`，于是自定义维度（id ≥ 3）
    /// 的事件全被判成主世界，土地保护「主世界拒绝、别处放行」被绕过且零日志。
    pub fn dim(&mut self) -> Result<i32> {
        if !self.check_complete() {
            let miss = self.unresolved().join(", ");
            return Err(Error(format!(
                "事件 `{}` 的载荷不完整（宿主解不出 {miss}），维度未知 —— 拒绝按主世界处理",
                self.id
            )));
        }
        self.i32_at("dim")
    }

    /// 事件里的玩家身份。
    ///
    /// 兼容三种形状：合成事件的 `_player:{name,xuid,uuid}`、宿主富化后的
    /// `_player`、以及只带一个名字的老事件。三者的差异以前要调用方自己知道。
    pub fn player(&mut self) -> Option<PlayerIdentity> {
        let v = self.value_opt()?;
        if let Some(p) = v.path("_player") {
            return Some(PlayerIdentity {
                name: p.opt_str("name").unwrap_or_default().to_owned(),
                xuid: p.opt_str("xuid").unwrap_or_default().to_owned(),
                uuid: p.opt_str("uuid").unwrap_or_default().to_owned(),
            });
        }
        // 退路：只带名字的事件。
        let name = v.first_str(&["playerName", "player", "name"])?;
        Some(PlayerIdentity {
            name: name.to_owned(),
            ..Default::default()
        })
    }

    /// 事件里的方块/位置坐标。`x`/`y`/`z` 三个平铺字段（合成事件的形状）。
    pub fn pos(&mut self) -> Result<(i32, i32, i32)> {
        let x = self.i32_at("x")?;
        let y = self.i32_at("y")?;
        let z = self.i32_at("z")?;
        Ok((x, y, z))
    }

    /// 同上但要浮点（玩家位置一类）。
    pub fn pos_f64(&mut self) -> Result<(f64, f64, f64)> {
        let x = self.f64_at("x")?;
        let y = self.f64_at("y")?;
        let z = self.f64_at("z")?;
        Ok((x, y, z))
    }

    /* ───────────── 改写与取消 ───────────── */

    /// 这个事件能不能取消。
    ///
    /// * `Some(true)` —— 能；
    /// * `Some(false)` —— 不能，[`Event::cancel`] 会返回 `Err`；
    /// * `None` —— 查不到（第三方模组自己发的事件，或本表还没跟上的上游新事件）。
    ///   这时 `cancel()` 会照常写回，但没人能替你确认它生效了。
    pub fn can_cancel(&self) -> Option<bool> {
        names::is_cancellable(self.id)
    }

    /// 取消这个事件。
    ///
    /// 不可取消的事件返回 `Err`，并**告诉你该去拦哪个**（`PlayerStartDestroy
    /// BlockEvent` → `PlayerDestroyBlockEvent`）。返回 `()` 会让保护类模组
    /// 以为自己拦住了，而「以为拦住了」比崩溃危险 —— 崩溃至少看得见。
    ///
    /// 查不到的事件（`can_cancel()` 为 `None`）**不拦着你**：照常写回并返回
    /// `Ok`。SDK 不知道的事，不假装知道。
    ///
    /// `Ok` 只代表取消位已经写回宿主，不代表引擎停下了 —— 有些钩点在半更新
    /// 的位置，宿主对这类点本就不接受取消。这条边界只能靠事件文档。
    pub fn cancel(&mut self) -> Result<()> {
        if self.can_cancel() == Some(false) {
            let why = names::why_not_cancellable(self.id).unwrap_or("这个事件不支持取消");
            return Err(Error(format!("事件 `{}` 不可取消：{why}", self.id)));
        }
        self.edit(|v| {
            v.insert("cancelled", NbtValue::Byte(1));
        });
        Ok(())
    }

    /// 取消，但不在乎能不能取消。
    ///
    /// 只有一种正当用法：你在写一个**转发/代理**类的通用组件，事件 id 是运行期
    /// 传进来的，拦不住也只能算了。业务代码请用 [`Event::cancel`] 并处理 `Err`。
    pub fn cancel_lenient(&mut self) -> bool {
        if self.can_cancel() == Some(false) {
            return false;
        }
        self.edit(|v| {
            v.insert("cancelled", NbtValue::Byte(1));
        });
        true
    }

    /// 撤销之前的取消（把 `cancelled` 写回 0）。
    ///
    /// 用于「我先拦下、再判、发现可以放行」这种两段式判定。注意它只能撤销
    /// **本次回调里自己写的**取消 —— 别的模组在更早的优先级上取消了的，
    /// 你撤不回来（宿主侧的总线也不允许把否决翻回批准）。
    pub fn uncancel(&mut self) {
        self.edit(|v| {
            v.insert("cancelled", NbtValue::Byte(0));
        });
    }

    /// 改载荷里的一个字段。
    ///
    /// 写回是**差量**的：只有你真的动过的键会被送回宿主，没碰的保持原样。
    /// 这让两个模组挂同一个事件时不会互相把对方的编辑抹掉。
    pub fn set(&mut self, path: &str, value: NbtValue) {
        let key = path.to_owned();
        self.edit(move |v| {
            v.insert(key, value);
        });
    }

    /// 任意改写。闭包拿到的是载荷的可变副本。
    pub fn edit(&mut self, f: impl FnOnce(&mut NbtValue)) {
        if self.edited.is_none() {
            // 以当前载荷为基准；解析不了就从空复合标签起步（至少能写 cancelled）。
            //
            // 先克隆到局部再赋值：`self.value()` 借着 `self`，同一条语句里再写
            // `self.edited` 就是借用冲突。
            let base = match self.value() {
                Ok(v) => v.clone(),
                Err(_) => NbtValue::compound(),
            };
            self.edited = Some(base);
        }
        if let Some(e) = self.edited.as_mut() {
            f(e);
        }
    }

    /// 回调结束时由蹦床调用：把编辑过的载荷送回宿主。
    fn flush(&mut self) {
        let Some(edited) = self.edited.take() else {
            return;
        };
        let text = edited.to_snbt();
        // 宿主侧（Events.cpp）会把这份和它自己那份快照做差量：只有真的变了的
        // 键才写回事件对象，缺席的键**不会**被删除（宿主侧 V-02 修的正是
        // 「缺席即删除」导致整次编辑丢失）。
        unsafe { (self.write_back)(self.write_ctx, s(&text)) };
    }
}

/// 订阅句柄。**Drop 即退订。**
///
/// 想让它活到模组卸载就调 [`Listener::forget`]。宿主在卸载时会统一摘掉剩下的
/// （它现在会真的调 `removeListener` 并在失败时报错，不再静默）。
pub struct Listener {
    handle: sys::PierListenerHandle,
    /// 闭包的所有权。退订之后才能释放，否则宿主可能正在调它。
    ///
    /// 只用来「持有 + 析构」，从不取出来用，所以 `Any` 够了。这里**不能**要
    /// `Sync`：装进去的是 `Box<Handler>`，而 `Handler` 只保证 `Send`。
    owned: Option<Box<dyn std::any::Any + Send>>,
    id: String,
}

impl Listener {
    /// 订阅的事件 id。
    pub fn event_id(&self) -> &str {
        &self.id
    }

    /// 放弃自动退订。闭包随之泄漏（活到进程结束）。
    pub fn forget(mut self) {
        self.handle = std::ptr::null_mut();
        if let Some(owned) = self.owned.take() {
            std::mem::forget(owned);
        }
    }
}

impl Drop for Listener {
    fn drop(&mut self) {
        if self.handle.is_null() {
            return;
        }
        if !crate::has_slot!(unsubscribe_event) {
            return;
        }
        let Some(f) = rt().api.unsubscribe_event else {
            return;
        };
        // 退订失败要说话：宿主侧同样的纪律（W-EV2）。静默失败的后果是回调
        // 还挂着、而闭包马上要被释放。
        let ok = unsafe { f(rt().handle(), self.handle) };
        if !ok {
            Logger::get().error(&format!(
                "退订事件 `{}` 失败 —— 监听器可能还挂在宿主上，而它的闭包即将释放。\
                 这条一定要查，不要当噪音。",
                self.id
            ));
            // 宁可泄漏闭包也不能让宿主调进已释放的内存。
            if let Some(owned) = self.owned.take() {
                std::mem::forget(owned);
            }
        }
    }
}

type Handler = dyn FnMut(&mut Event<'_>) + Send + 'static;

/// 订阅一个事件。
///
/// ```ignore
/// let l = event::subscribe(names::PLAYER_CHAT, |ev| {
///     let Ok(msg) = ev.str_at("message") else { return };
///     if !msg.contains("badword") { return; }
///     // cancel() 返回 Result：拦不住的事件会告诉你原因和该去拦哪个。
///     if let Err(e) = ev.cancel() {
///         Logger::get().error(&format!("聊天过滤没生效：{e}"));
///     }
/// })?;
/// ```
pub fn subscribe(
    id: &str,
    handler: impl FnMut(&mut Event<'_>) + Send + 'static,
) -> Result<Listener> {
    subscribe_with(id, Priority::Normal, handler)
}

/// 带优先级的订阅。
pub fn subscribe_with(
    id: &str,
    priority: Priority,
    handler: impl FnMut(&mut Event<'_>) + Send + 'static,
) -> Result<Listener> {
    let f = crate::require_slot!(subscribe_event, "订阅事件");
    let boxed: Box<Box<Handler>> = Box::new(Box::new(handler));
    let user = Box::into_raw(boxed);

    let handle = unsafe {
        f(
            rt().handle(),
            s(id),
            priority.raw(),
            trampoline,
            user.cast(),
        )
    };
    if handle.is_null() {
        // 宿主拒绝了：闭包还没交出去，收回所有权。
        drop(unsafe { Box::from_raw(user) });
        return Err(Error(format!(
            "订阅 `{id}` 失败。宿主日志里会列出它认得的相近 id —— \
             多半是事件名拼错，或者这个 BDS 版本上没有这个事件。"
        )));
    }
    Ok(Listener {
        handle,
        owned: Some(unsafe { Box::from_raw(user) }),
        id: id.to_owned(),
    })
}

/// 宿主认得的全部事件 id（注册表里的 + 全部合成事件）。
///
/// 拼错事件名时先看这个，比猜快。
pub fn list() -> Vec<String> {
    if !crate::has_slot!(list_events) {
        return Vec::new();
    }
    let Some(f) = rt().api.list_events else {
        return Vec::new();
    };
    crate::rt::ffi::collect_strs(|ctx, sink| unsafe { f(ctx, sink) })
}

/// 这个宿主认不认得某个事件 id。
pub fn exists(id: &str) -> bool {
    list().iter().any(|e| e == id)
}

/// # Safety
/// `user` 必须是 `Box<Box<Handler>>::into_raw` 的产物，且在监听器存活期间有效。
unsafe extern "C" fn trampoline(
    user: *mut c_void,
    event_id: sys::PierStr,
    snbt: sys::PierStr,
    write_ctx: *mut c_void,
    write_back: sys::PierStrSink,
) {
    if user.is_null() {
        return;
    }
    let f = &mut *(user as *mut Box<Handler>);
    let id = r(event_id);
    let text = r(snbt);

    let mut ev = Event {
        id,
        snbt: text,
        parsed: None,
        edited: None,
        write_ctx,
        write_back,
    };

    // panic 不许穿过 extern "C"（那是 UB）。就地拦下并打日志；**编辑不写回** ——
    // 一个 panic 到一半的判定写回去的东西没有意义。
    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        f(&mut ev);
        ev.flush();
    }));
    if result.is_err() {
        Logger::get().error(&format!(
            "事件 `{id}` 的监听器 panic 了。已就地拦下，这次的编辑被丢弃。"
        ));
    }
}

/* ═══════════════════════════ Wiring ═══════════════════════════ */

/// 批量订阅，句柄统一持有。
///
/// 业务侧本来就在自己写这个东西（`Wiring::new("worldedit").on(...).at(...)`），
/// 所以把它收进 SDK。相对手写版多两样：**订阅失败不会静默**（失败的条目会
/// 记在 [`Wiring::failures`] 里，`arm()` 时可以选择整体失败），以及标签会进
/// 日志，便于定位是哪一条挂的。
///
/// ```ignore
/// let wiring = Wiring::new("plots")
///     .on(names::PLAYER_DESTROY_BLOCK, "protect-break", |ev| { ... })
///     .at(names::PLAYER_DISCONNECT, Priority::Low, "forget", |ev| { ... })
///     .arm()?;                       // 任一失败即整体失败
/// // wiring 掉出作用域 = 全部退订
/// ```
pub struct Wiring {
    owner: String,
    pending: Vec<(String, Priority, String, Box<Handler>)>,
    listeners: Vec<Listener>,
    failures: Vec<(String, String)>,
}

impl Wiring {
    pub fn new(owner: impl Into<String>) -> Wiring {
        Wiring {
            owner: owner.into(),
            pending: Vec::new(),
            listeners: Vec::new(),
            failures: Vec::new(),
        }
    }

    /// 加一条 Normal 优先级的订阅。`tag` 只用于日志。
    #[must_use]
    pub fn on(
        self,
        id: &str,
        tag: &str,
        handler: impl FnMut(&mut Event<'_>) + Send + 'static,
    ) -> Wiring {
        self.at(id, Priority::Normal, tag, handler)
    }

    /// 加一条指定优先级的订阅。
    #[must_use]
    pub fn at(
        mut self,
        id: &str,
        priority: Priority,
        tag: &str,
        handler: impl FnMut(&mut Event<'_>) + Send + 'static,
    ) -> Wiring {
        self.pending
            .push((id.to_owned(), priority, tag.to_owned(), Box::new(handler)));
        self
    }

    /// 真正去订阅。**任何一条失败即整体失败**，已成功的那些会在返回前退订 ——
    /// 半挂着的保护比完全没挂更危险（有些点拦得住、有些拦不住，而且没人知道
    /// 是哪些）。
    pub fn arm(mut self) -> Result<Wiring> {
        let pending = std::mem::take(&mut self.pending);
        for (id, prio, tag, handler) in pending {
            // `Box<Handler>` 自己就实现 FnMut，直接递进去 —— 不用再套一层闭包。
            match subscribe_with(&id, prio, handler) {
                Ok(l) => self.listeners.push(l),
                Err(e) => {
                    self.failures
                        .push((format!("{}/{}", self.owner, tag), e.to_string()));
                }
            }
        }
        if !self.failures.is_empty() {
            let detail = self
                .failures
                .iter()
                .map(|(t, e)| format!("  {t}: {e}"))
                .collect::<Vec<_>>()
                .join("\n");
            // listeners 在 self 掉落时自动退订，不用手动清。
            return Err(Error(format!(
                "`{}` 的事件接线有 {} 条没挂上，已整体撤回：\n{detail}",
                self.owner,
                self.failures.len()
            )));
        }
        Ok(self)
    }

    /// 宽松版：失败的记下来，成功的照常挂上。适合「有就更好」的可选功能。
    pub fn arm_lenient(mut self) -> Wiring {
        let pending = std::mem::take(&mut self.pending);
        for (id, prio, tag, handler) in pending {
            match subscribe_with(&id, prio, handler) {
                Ok(l) => self.listeners.push(l),
                Err(e) => {
                    Logger::get().warn(&format!(
                        "`{}/{}` 订阅 `{id}` 失败（继续挂其余的）：{e}",
                        self.owner, tag
                    ));
                    self.failures
                        .push((format!("{}/{}", self.owner, tag), e.to_string()));
                }
            }
        }
        self
    }

    /// 没挂上的那些（标签, 原因）。
    pub fn failures(&self) -> &[(String, String)] {
        &self.failures
    }

    /// 挂上的条数。
    pub fn armed(&self) -> usize {
        self.listeners.len()
    }

    /// 全部改为「不自动退订」，活到模组卸载。
    pub fn forget(mut self) {
        for l in self.listeners.drain(..) {
            l.forget();
        }
    }
}

/* ───────────── 兼容别名 ───────────── */

/// 上一代把它叫 `EventRef`。名字保留，指向同一个类型。
pub type EventRef<'a> = Event<'a>;

/// 上一代把优先级叫 `EventPriority`。
pub type EventPriority = Priority;

/// 载荷取值失败时的原始错误类型（透出去便于业务侧做精细分支）。
pub use crate::nbt::NbtError as PayloadError;

/// 从一个复合标签直接建 `PlayerIdentity`（给自定义载荷用）。
impl From<&BTreeMap<String, NbtValue>> for PlayerIdentity {
    fn from(m: &BTreeMap<String, NbtValue>) -> Self {
        let get = |k: &str| {
            m.get(k)
                .and_then(NbtValue::as_str)
                .unwrap_or_default()
                .to_owned()
        };
        PlayerIdentity {
            name: get("name"),
            xuid: get("xuid"),
            uuid: get("uuid"),
        }
    }
}

impl std::fmt::Debug for Event<'_> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("Event")
            .field("id", &self.id)
            .field("snbt", &self.snbt)
            .finish()
    }
}
