//! Cross-mod services: one question, one answer.
//!
//! Complementary to the bus, which broadcasts and returns nothing. A name is exclusive:
//! one name has one provider, and taking one is refused by the host, whose log names the
//! holder, which is far easier to diagnose than the later registration winning.
//!
//! [`call`] gives a bare `String` while [`call_json`] deserializes straight into the type
//! you want, where a parse failure is a definite error and not the fallback of an
//! `unwrap_or`.
//!
//! The errors are categorized in [`CallError`]: no such service and the service refusing
//! are two different things. The first usually means a missing dependency or a misspelled
//! name, and the second is a refusal by business logic.

use std::ffi::c_void;

use serde::de::DeserializeOwned;

use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, r, s};
use crate::rt::logger::Logger;
use crate::rt::runtime::rt;
use crate::sys;

/// Why a call failed.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum CallError {
    /// Nobody provides this name, because it is not installed, not enabled, or misspelled.
    NotFound { name: String },
    /// The provider ran and said no this time. `message` is what it wrote back.
    Provider { name: String, message: String },
    /// The host refused the call: an invalid name, a call to itself, or a call depth exceeded
    /// by a cycle.
    Refused { name: String },
    /// The reply arrived and does not parse into the requested type.
    Decode { name: String, detail: String },
    /// The host offers no service capability, being older or built without that capability
    /// package.
    Unavailable,
}

impl std::fmt::Display for CallError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            CallError::NotFound { name } => {
                write!(f, "no mod provides the service `{name}`: it is not installed, not enabled, or the name is misspelled")
            }
            CallError::Provider { name, message } => {
                write!(f, "the service `{name}` refused this call: {message}")
            }
            CallError::Refused { name } => write!(
                f,
                "the host refused the call to `{name}`: an invalid name, a call to itself, or a cycle in the call chain that exceeded the depth"
            ),
            CallError::Decode { name, detail } => {
                write!(f, "parsing the reply of service `{name}` failed: {detail}")
            }
            CallError::Unavailable => write!(f, "this host offers no cross-mod service capability"),
        }
    }
}

impl std::error::Error for CallError {}

impl From<CallError> for Error {
    fn from(e: CallError) -> Self {
        Error(e.to_string())
    }
}

/// The result of a call.
pub type CallResult<T> = std::result::Result<T, CallError>;

/* The caller side */

/// Calls a service and returns the raw reply text.
pub fn call(name: &str, request: &str) -> CallResult<String> {
    // Neither gate may be skipped (contract §2.2): with a table too short to reach this
    // field, reading it is an out-of-bounds read, and what comes back often looks like a
    // valid function pointer.
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

/// Calls a service and deserializes the reply from JSON into `T`.
///
/// This is the recommended form. It replaces the `from_str`, `as_array`, `get`,
/// `unwrap_or(0)` boilerplate and turns a malformed reply into a real error rather than a
/// perfectly ordinary looking 0.
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
        detail: format!("{e} (first 200 characters of the reply: {})", truncate(&body, 200)),
    })
}

/// As above, with the request side serialized through `serde` as well.
pub fn call_with<Q: serde::Serialize, T: DeserializeOwned>(
    name: &str,
    request: &Q,
) -> CallResult<T> {
    let body = serde_json::to_string(request).map_err(|e| CallError::Decode {
        name: name.to_owned(),
        detail: format!("serializing the request failed: {e}"),
    })?;
    call_json(name, &body)
}

/// Calls, returning `None` on `NotFound` and raising every other error as usual.
///
/// For an optional integration that uses a dependency when installed and degrades
/// otherwise. Such code used to be written as
/// `let Ok(x) = call(..) else { return default }`, which swallowed a service error too.
pub fn call_optional(name: &str, request: &str) -> CallResult<Option<String>> {
    match call(name, request) {
        Ok(v) => Ok(Some(v)),
        Err(CallError::NotFound { .. }) => Ok(None),
        Err(e) => Err(e),
    }
}

/// Whether any mod provides this name.
///
/// An earlier generation substring-matched inside the JSON text of `service_list`, so a
/// name that is a prefix of another matched wrongly, with `plot` hitting `plot:worlds`.
/// This really parses it.
pub fn exists(name: &str) -> bool {
    list().iter().any(|s| s.name == name)
}

/// The registration record of one service.
#[derive(Debug, Clone, PartialEq, Eq, serde::Deserialize)]
pub struct ServiceInfo {
    pub name: String,
    /// The name of the providing mod.
    #[serde(default)]
    #[serde(rename = "mod")]
    pub owner: String,
}

/// The raw JSON of the service listing, unparsed.
///
/// For when [`list`] cannot parse it, or the host added a field this layer does not know.
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

/// Every currently registered service.
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
        // An unparsable listing says so. Silently returning an empty table would make a
        // registered service that cannot be found an untraceable problem.
        Logger::get().warn(&format!("parsing the service_list reply failed: {e}"));
        Vec::new()
    })
}

fn truncate(s: &str, n: usize) -> String {
    if s.chars().count() <= n {
        return s.to_owned();
    }
    s.chars().take(n).collect::<String>() + "…"
}

/* The provider side */

/// A service registration handle. Dropping it deregisters.
///
/// [`Registration::forget`] keeps it alive until the mod unloads, and the host clears what
/// remains at unload.
pub struct Registration {
    id: u64,
    /// As with `event::Listener::owned`: only held and dropped, never taken out. What goes in
    /// is a `Box<Provider>`, which guarantees only `Send`, so `Sync` must not be required
    /// here either.
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
                "deregistering service `{}` failed; it may still be attached to the host while its closure is about to be freed.",
                self.name
            ));
            if let Some(o) = self.owned.take() {
                std::mem::forget(o);
            }
        }
    }
}

/// The provider callback: it receives a request and returns a reply.
///
/// `Ok(reply)` is a success and `Err(message)` is a refusal by business logic, where the
/// caller receives a [`CallError::Provider`] carrying `message` unchanged.
type Provider = dyn FnMut(&str, &str) -> std::result::Result<String, String> + Send + 'static;

/// Registers a service.
/// ```ignore let _reg = service::register("plot:worlds", |_name, _req| {
///     Ok(serde_json::to_string(&worlds()).unwrap_or_default())
///
/// })?; ```
///
/// A few host-side rules, which cannot be changed here and are worth knowing:
/// * the callback runs synchronously on the caller's thread, so nothing slow belongs in it;
/// * a name already taken fails outright and does not displace the holder;
/// * a service stays reachable while its mod is disabled, because LeviLamina enables only
///     after every `on_load` has run, and being unreachable in that window would fail every
///     consumer resolving its dependency inside its own `on_load`. Refusing while disabled is
///     left to the provider to decide.
pub fn register(
    name: &str,
    provider: impl FnMut(&str, &str) -> std::result::Result<String, String> + Send + 'static,
) -> Result<Registration> {
    let f = crate::require_slot!(service_register, "registering a cross-mod service");
    let boxed: Box<Box<Provider>> = Box::new(Box::new(provider));
    let user = Box::into_raw(boxed);

    let id = unsafe { f(rt().handle(), s(name), trampoline, user.cast()) };
    if id == 0 {
        drop(unsafe { Box::from_raw(user) });
        return Err(Error(format!(
            "registering service `{name}` failed: the name is invalid, empty or too long, or it is \
             already taken by another mod, whose name the host log gives"
        )));
    }
    Ok(Registration {
        id,
        owned: Some(unsafe { Box::from_raw(user) }),
        name: name.to_owned(),
    })
}

/// Registers a service whose request and reply are both JSON.
///
/// It removes the `to_string` and `from_str` boilerplate on both sides, and a failed
/// deserialization becomes a definite business error returned to the caller rather than
/// the provider writing its own `unwrap_or_default`.
pub fn register_json<Q, R, F>(name: &str, mut provider: F) -> Result<Registration>
where
    Q: DeserializeOwned,
    R: serde::Serialize,
    F: FnMut(Q) -> std::result::Result<R, String> + Send + 'static,
{
    register(name, move |_name, req| {
        let parsed: Q =
            serde_json::from_str(req).map_err(|e| format!("the request is not the JSON shape this service wants: {e}"))?;
        let out = provider(parsed)?;
        serde_json::to_string(&out).map_err(|e| format!("serializing the reply failed: {e}"))
    })
}

/// # Safety
/// `user` must come from `Box<Box<Provider>>::into_raw`.
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
            // A refusal by business logic writes the reason back and the caller sees
            // CallError::Provider.
            reply(ctx, s(&msg));
            false
        }
        Err(_) => {
            let msg = format!("the provider of service `{n}` panicked");
            Logger::get().error(&format!("{msg}. It was caught here and this call returns a failure."));
            reply(ctx, s(&msg));
            false
        }
    }
}
