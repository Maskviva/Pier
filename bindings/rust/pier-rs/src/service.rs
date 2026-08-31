//! 跨模组服务：一问一答。
//!
//! 和总线（广播、无返回）互补。名字**独占**：一个名字只能有一个提供方，
//! 抢占会被宿主拒绝并在日志里点名占用者 —— 这比「后注册者覆盖」好查得多。
//!
//! [`call`] 给裸 `String`，[`call_json`] 直接反序列化成你要的类型，
//! 解析失败是**一种明确的错误**而不是 `unwrap_or` 的兜底值。
//!
//! 错误分了类（[`CallError`]）：「没有这个服务」和「服务说不行」是两回事 ——
//! 前者多半是依赖没装或名字拼错，后者是业务拒绝。

use std::ffi::c_void;

use serde::de::DeserializeOwned;

use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, r, s};
use crate::rt::logger::Logger;
use crate::rt::runtime::rt;
use crate::sys;

/// 调用失败的原因。
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum CallError {
    /// 没人提供这个名字（没装、没启用、或名字拼错）。
    NotFound { name: String },
    /// 提供方跑了，但它说这次不行。`message` 是它写回的内容。
    Provider { name: String, message: String },
    /// 宿主拒绝了这次调用：名字非法、自己调自己、或调用深度超限（环）。
    Refused { name: String },
    /// 应答拿到了，但解析不成要的类型。
    Decode { name: String, detail: String },
    /// 宿主没有提供 service 能力（老宿主，或没编入这个能力包）。
    Unavailable,
}

impl std::fmt::Display for CallError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            CallError::NotFound { name } => {
                write!(f, "没有模组提供服务 `{name}`（没装、没启用，或名字拼错了）")
            }
            CallError::Provider { name, message } => {
                write!(f, "服务 `{name}` 拒绝了这次调用：{message}")
            }
            CallError::Refused { name } => write!(
                f,
                "宿主拒绝调用 `{name}`：名字非法、调了自己，或调用链成环（深度超限）"
            ),
            CallError::Decode { name, detail } => {
                write!(f, "服务 `{name}` 的应答解析失败：{detail}")
            }
            CallError::Unavailable => write!(f, "这个宿主没有提供跨模组服务能力"),
        }
    }
}

impl std::error::Error for CallError {}

impl From<CallError> for Error {
    fn from(e: CallError) -> Self {
        Error(e.to_string())
    }
}

/// 调用结果。
pub type CallResult<T> = std::result::Result<T, CallError>;

/* ═══════════════════════ 调用方 ═══════════════════════ */

/// 调一个服务，拿回原始应答文本。
pub fn call(name: &str, request: &str) -> CallResult<String> {
    // 两道闸缺一不可（契约 §2.2）：表短到够不着这个字段时，读它是越界读，
    // 而越界读回来的东西常常看起来像一个合法的函数指针。
    if !crate::has_slot!(service_call) {
        return Err(CallError::Unavailable);
    }
    let Some(f) = rt().api.service_call else {
        return Err(CallError::Unavailable);
    };
    let mut reply: Option<String> = None;
    let code = unsafe {
        f(
            rt().handle(),
            s(name),
            s(request),
            (&mut reply as *mut Option<String>).cast(),
            crate::rt::ffi::set_string,
        )
    };
    let body = reply.unwrap_or_default();
    match code {
        sys::PIER_SERVICE_OK => Ok(body),
        sys::PIER_SERVICE_NOT_FOUND => Err(CallError::NotFound {
            name: name.to_owned(),
        }),
        sys::PIER_SERVICE_ERROR => Err(CallError::Provider {
            name: name.to_owned(),
            message: body,
        }),
        _ => Err(CallError::Refused {
            name: name.to_owned(),
        }),
    }
}

/// 调一个服务，把应答按 JSON 反序列化成 `T`。
///
/// 这是**推荐用法**。它替掉的是「`from_str` → `as_array` → `get` →
/// `unwrap_or(0)`」那一串样板，顺带把「应答格式不对」变成一个真正的错误 ——
/// 而不是一个看起来很正常的 0。
///
/// ```ignore
/// #[derive(serde::Deserialize)]
/// struct World { dim: i32, #[serde(rename = "plotSize")] plot_size: i32 }
///
/// let worlds: Vec<World> = service::call_json("plot:worlds", "{}")?;
/// ```
pub fn call_json<T: DeserializeOwned>(name: &str, request: &str) -> CallResult<T> {
    let body = call(name, request)?;
    serde_json::from_str(&body).map_err(|e| CallError::Decode {
        name: name.to_owned(),
        detail: format!("{e}（应答前 200 字：{}）", truncate(&body, 200)),
    })
}

/// 同上，但请求侧也用 `serde` 序列化。
pub fn call_with<Q: serde::Serialize, T: DeserializeOwned>(
    name: &str,
    request: &Q,
) -> CallResult<T> {
    let body = serde_json::to_string(request).map_err(|e| CallError::Decode {
        name: name.to_owned(),
        detail: format!("请求序列化失败：{e}"),
    })?;
    call_json(name, &body)
}

/// 调用；`NotFound` 时返回 `None`，其余错误照常抛。
///
/// 给「这个依赖装了就用、没装就走降级路径」的可选集成用 —— 这类代码以前
/// 会写成 `let Ok(x) = call(..) else { return default }`，把「服务报错」也
/// 一起吞掉了。
pub fn call_optional(name: &str, request: &str) -> CallResult<Option<String>> {
    match call(name, request) {
        Ok(v) => Ok(Some(v)),
        Err(CallError::NotFound { .. }) => Ok(None),
        Err(e) => Err(e),
    }
}

/// 有没有模组提供这个名字。
///
/// 上一代是在 `service_list` 的 JSON 文本里做**子串匹配** —— 名字里带别人
/// 名字的前缀就会误判（`plot` 命中 `plot:worlds`）。这里真解析。
pub fn exists(name: &str) -> bool {
    list().iter().any(|s| s.name == name)
}

/// 一条服务的登记信息。
#[derive(Debug, Clone, PartialEq, Eq, serde::Deserialize)]
pub struct ServiceInfo {
    pub name: String,
    /// 提供方模组名。
    #[serde(default)]
    #[serde(rename = "mod")]
    pub owner: String,
}

/// 服务清单的原始 JSON,不解析。
///
/// [`list`] 解析不动、或者宿主加了这一层还不认识的字段时,用它自己看。
pub fn list_json() -> String {
    if !crate::has_slot!(service_list) {
        return String::new();
    }
    let Some(f) = rt().api.service_list else {
        return String::new();
    };
    call_out_str(|ctx, sink| {
        unsafe { f(ctx, sink) };
        true
    })
    .unwrap_or_default()
}

/// 当前注册的全部服务。
pub fn list() -> Vec<ServiceInfo> {
    if !crate::has_slot!(service_list) {
        return Vec::new();
    }
    let Some(f) = rt().api.service_list else {
        return Vec::new();
    };
    let Some(text) = call_out_str(|ctx, sink| {
        unsafe { f(ctx, sink) };
        true
    }) else {
        return Vec::new();
    };
    serde_json::from_str(&text).unwrap_or_else(|e| {
        // 解析不了就说出来。静默返回空表会让「服务明明注册了却查不到」
        // 变成一个查不下去的问题。
        Logger::get().warn(&format!("service_list 的应答解析失败：{e}"));
        Vec::new()
    })
}

fn truncate(s: &str, n: usize) -> String {
    if s.chars().count() <= n {
        return s.to_owned();
    }
    s.chars().take(n).collect::<String>() + "…"
}

/* ═══════════════════════ 提供方 ═══════════════════════ */

/// 服务注册句柄。**Drop 即注销。**
///
/// 想让它活到模组卸载就调 [`Registration::forget`]（宿主在卸载时会统一清）。
pub struct Registration {
    id: u64,
    /// 同 `event::Listener::owned`：只持有、只析构，不取出来用。装进去的是
    /// `Box<Provider>`，只保证 `Send`，所以这里也不能要 `Sync`。
    owned: Option<Box<dyn std::any::Any + Send>>,
    name: String,
}

impl Registration {
    pub fn id(&self) -> u64 {
        self.id
    }

    pub fn name(&self) -> &str {
        &self.name
    }

    pub fn forget(mut self) {
        self.id = 0;
        if let Some(o) = self.owned.take() {
            std::mem::forget(o);
        }
    }
}

impl Drop for Registration {
    fn drop(&mut self) {
        if self.id == 0 {
            return;
        }
        if !crate::has_slot!(service_unregister) {
            return;
        }
        let Some(f) = rt().api.service_unregister else {
            return;
        };
        let ok = unsafe { f(rt().handle(), self.id) };
        if !ok {
            Logger::get().error(&format!(
                "注销服务 `{}` 失败 —— 它可能还挂在宿主上，而闭包即将释放。",
                self.name
            ));
            if let Some(o) = self.owned.take() {
                std::mem::forget(o);
            }
        }
    }
}

/// 提供方回调：收到请求，回一个应答。
///
/// 返回 `Ok(reply)` = 成功；`Err(message)` = 业务拒绝（调用方收到
/// [`CallError::Provider`]，`message` 原样带过去）。
type Provider = dyn FnMut(&str, &str) -> std::result::Result<String, String> + Send + 'static;

/// 注册一个服务。
///
/// ```ignore
/// let _reg = service::register("plot:worlds", |_name, _req| {
///     Ok(serde_json::to_string(&worlds()).unwrap_or_default())
/// })?;
/// ```
///
/// 注意几条宿主侧的纪律（不是这里能改的，但你需要知道）：
/// * 回调**在调用方的线程上同步执行**，别在里面做慢活儿。
/// * 名字被别人占了会直接失败，不会顶掉对方。
/// * 模组被**禁用**期间服务**仍然可达** —— 因为 LeviLamina 要等所有 `on_load`
///   跑完才 enable，服务在那个窗口不可达会让每个在自己 `on_load` 里解析依赖
///   的消费方全部失败。要在禁用期拒绝就自己判。
pub fn register(
    name: &str,
    provider: impl FnMut(&str, &str) -> std::result::Result<String, String> + Send + 'static,
) -> Result<Registration> {
    let f = crate::require_slot!(service_register, "注册跨模组服务");
    let boxed: Box<Box<Provider>> = Box::new(Box::new(provider));
    let user = Box::into_raw(boxed);

    let id = unsafe { f(rt().handle(), s(name), trampoline, user.cast()) };
    if id == 0 {
        drop(unsafe { Box::from_raw(user) });
        return Err(Error(format!(
            "注册服务 `{name}` 失败：名字非法/为空/过长，或已被另一个模组占用\
             （宿主日志里点了占用者的名字）"
        )));
    }
    Ok(Registration {
        id,
        owned: Some(unsafe { Box::from_raw(user) }),
        name: name.to_owned(),
    })
}

/// 注册一个「请求与应答都是 JSON」的服务。
///
/// 省掉两侧的 `to_string`/`from_str` 样板，并且**反序列化失败会变成一条
/// 明确的业务错误**回给调用方，而不是让提供方自己写一遍 `unwrap_or_default`。
pub fn register_json<Q, R, F>(name: &str, mut provider: F) -> Result<Registration>
where
    Q: DeserializeOwned,
    R: serde::Serialize,
    F: FnMut(Q) -> std::result::Result<R, String> + Send + 'static,
{
    register(name, move |_name, req| {
        let parsed: Q =
            serde_json::from_str(req).map_err(|e| format!("请求不是本服务要的 JSON 形状：{e}"))?;
        let out = provider(parsed)?;
        serde_json::to_string(&out).map_err(|e| format!("应答序列化失败：{e}"))
    })
}

/// # Safety
/// `user` 必须是 `Box<Box<Provider>>::into_raw` 的产物。
unsafe extern "C" fn trampoline(
    user: *mut c_void,
    name: sys::PierStr,
    request: sys::PierStr,
    ctx: *mut c_void,
    reply: sys::PierStrSink,
) -> bool {
    if user.is_null() {
        return false;
    }
    let f = &mut *(user as *mut Box<Provider>);
    let n = r(name);
    let q = r(request);

    let outcome = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| f(n, q)));
    match outcome {
        Ok(Ok(body)) => {
            reply(ctx, s(&body));
            true
        }
        Ok(Err(msg)) => {
            // 业务拒绝：把理由写回去，调用方会看到 CallError::Provider。
            reply(ctx, s(&msg));
            false
        }
        Err(_) => {
            let msg = format!("服务 `{n}` 的提供方 panic 了");
            Logger::get().error(&format!("{msg}。已就地拦下，本次调用按失败返回。"));
            reply(ctx, s(&msg));
            false
        }
    }
}
