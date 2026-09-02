//! Forms: the three screens sent to a player, whose callback arrives asynchronously.
//!
//! # The callback runs at most once and may never run
//!
//! When the player answers, or closes the form, the callback runs once on the server
//! thread. But if the mod is disabled before the player answers, the host mutes that
//! callback and it never arrives. Therefore:
//!
//! * the callback is an `FnOnce` and is freed once it has run;
//! * on the muted path the memory holding the closure is leaked on purpose. The code able
//!   to free it lives in a dynamic library that may already be unloaded, and freeing it is
//!   the real use-after-free.
//!
//! It follows that cleanup that has to happen does not belong in a form callback: a player
//! may never click.

use core::ffi::c_void;

use crate::nbt::NbtValue;
use crate::player::Player;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{r, s};
use crate::rt::logger::Logger;
use crate::rt::runtime::rt;
use crate::sys;

/// The value one control of a custom form returns.
#[derive(Debug, Clone, PartialEq)]
pub enum FormValue {
    /// The selected index of a dropdown or a step slider.
    Index(usize),
    Number(f64),
    Text(String),
    Bool(bool),
}

impl FormValue {
    pub fn as_index(&self) -> Option<usize> {
        match self {
            FormValue::Index(i) => Some(*i),
            FormValue::Number(n) if *n >= 0.0 => Some(*n as usize),
            _ => None,
        }
    }
    pub fn as_f64(&self) -> Option<f64> {
        match self {
            FormValue::Number(n) => Some(*n),
            FormValue::Index(i) => Some(*i as f64),
            FormValue::Bool(b) => Some(if *b { 1.0 } else { 0.0 }),
            FormValue::Text(_) => None,
        }
    }
    pub fn as_i64(&self) -> Option<i64> {
        self.as_f64().map(|v| v as i64)
    }
    pub fn as_bool(&self) -> Option<bool> {
        match self {
            FormValue::Bool(b) => Some(*b),
            FormValue::Number(n) => Some(*n != 0.0),
            FormValue::Index(i) => Some(*i != 0),
            FormValue::Text(_) => None,
        }
    }
    pub fn as_str(&self) -> Option<&str> {
        match self {
            FormValue::Text(t) => Some(t),
            _ => None,
        }
    }
}

/// A player's answer to a form.
#[derive(Debug, Clone, PartialEq)]
pub enum FormResponse {
    /// The player closed the form. `reason` is the cancellation reason from the engine, or
    /// -1 when it cannot be read.
    Cancelled { reason: i32 },
    /// A simple form: which button was pressed.
    Button(usize),
    /// A modal form: `true` is the upper button.
    Modal { upper: bool },
    /// A custom form, indexed by control name.
    ///
    /// `values` holds the raw values and `texts` the text of the selected item of a dropdown
    /// or step slider. Both are given because the option list may have changed after the form
    /// was sent, and an index alone cannot be matched back.
    Custom {
        values: std::collections::BTreeMap<String, FormValue>,
        texts: std::collections::BTreeMap<String, String>,
    },
    /// The host answered with a shape this side cannot read. It is not swallowed: silently
    /// treating it as a cancellation would make the player pressing confirm and nothing
    /// happening an untraceable problem.
    Unknown { raw: String },
}

impl FormResponse {
    pub fn is_cancelled(&self) -> bool {
        matches!(self, FormResponse::Cancelled { .. })
    }

    /// Reads the value of one control of a custom form.
    pub fn value(&self, name: &str) -> Option<&FormValue> {
        match self {
            FormResponse::Custom { values, .. } => values.get(name),
            _ => None,
        }
    }

    /// Reads the text of the selected item of one control of a custom form.
    pub fn text(&self, name: &str) -> Option<&str> {
        match self {
            FormResponse::Custom { texts, .. } => texts.get(name).map(|s| s.as_str()),
            _ => None,
        }
    }
}

/// A simple form: a column of buttons.
#[derive(Debug, Clone, Default)]
pub struct SimpleForm {
    title: String,
    content: String,
    elements: Vec<NbtValue>,
}

impl SimpleForm {
    pub fn new(title: impl Into<String>) -> SimpleForm {
        SimpleForm {
            title: title.into(),
            ..SimpleForm::default()
        }
    }

    pub fn content(mut self, content: impl Into<String>) -> SimpleForm {
        self.content = content.into();
        self
    }

    pub fn button(mut self, text: &str) -> SimpleForm {
        self.elements.push(NbtValue::obj([
            ("kind", "button".into()),
            ("text", text.into()),
        ]));
        self
    }

    /// A button with an icon. `image_type` is `"path"` or `"url"`.
    pub fn button_with_image(mut self, text: &str, image: &str, image_type: &str) -> SimpleForm {
        self.elements.push(NbtValue::obj([
            ("kind", "button".into()),
            ("text", text.into()),
            ("image", image.into()),
            ("image_type", image_type.into()),
        ]));
        self
    }

    pub fn header(mut self, text: &str) -> SimpleForm {
        self.elements.push(NbtValue::obj([
            ("kind", "header".into()),
            ("text", text.into()),
        ]));
        self
    }

    pub fn label(mut self, text: &str) -> SimpleForm {
        self.elements.push(NbtValue::obj([
            ("kind", "label".into()),
            ("text", text.into()),
        ]));
        self
    }

    pub fn divider(mut self) -> SimpleForm {
        self.elements
            .push(NbtValue::obj([("kind", "divider".into())]));
        self
    }

    /// Sends it.
    ///
    /// A form with no button at all cannot be pressed and can only be closed. It is stopped
    /// here in advance, because it is almost always a list that assembled empty rather than
    /// something intended.
    pub fn send(
        self,
        player: &Player,
        cb: impl FnOnce(FormResponse) + Send + 'static,
    ) -> Result<()> {
        let buttons = self
            .elements
            .iter()
            .filter(|e| e.opt_str("kind") == Some("button"))
            .count();
        if buttons == 0 {
            return Err(Error(format!(
                "the simple form {:?} has no button at all and a player cannot press it",
                self.title
            )));
        }
        let spec = NbtValue::obj([
            ("title", NbtValue::from(self.title.as_str())),
            ("content", NbtValue::from(self.content.as_str())),
            ("elements", NbtValue::list(self.elements)),
        ]);
        send(player, 0, &spec.to_snbt(), cb)
    }
}

/// A modal form: some text and two buttons.
#[derive(Debug, Clone)]
pub struct ModalForm {
    title: String,
    content: String,
    upper: String,
    lower: String,
}

impl ModalForm {
    pub fn new(title: impl Into<String>, content: impl Into<String>) -> ModalForm {
        ModalForm {
            title: title.into(),
            content: content.into(),
            upper: "OK".to_owned(),
            lower: "Cancel".to_owned(),
        }
    }

    pub fn upper(mut self, text: impl Into<String>) -> ModalForm {
        self.upper = text.into();
        self
    }

    pub fn lower(mut self, text: impl Into<String>) -> ModalForm {
        self.lower = text.into();
        self
    }

    pub fn send(
        self,
        player: &Player,
        cb: impl FnOnce(FormResponse) + Send + 'static,
    ) -> Result<()> {
        let spec = NbtValue::obj([
            ("title", NbtValue::from(self.title.as_str())),
            ("content", NbtValue::from(self.content.as_str())),
            ("upper", NbtValue::from(self.upper.as_str())),
            ("lower", NbtValue::from(self.lower.as_str())),
        ]);
        send(player, 2, &spec.to_snbt(), cb)
    }
}

/// A custom form: input fields, toggles, dropdowns and sliders.
#[derive(Debug, Clone, Default)]
pub struct CustomForm {
    title: String,
    submit: Option<String>,
    elements: Vec<NbtValue>,
}

impl CustomForm {
    pub fn new(title: impl Into<String>) -> CustomForm {
        CustomForm {
            title: title.into(),
            ..CustomForm::default()
        }
    }

    pub fn submit(mut self, text: impl Into<String>) -> CustomForm {
        self.submit = Some(text.into());
        self
    }

    pub fn header(mut self, text: &str) -> CustomForm {
        self.elements.push(NbtValue::obj([
            ("kind", "header".into()),
            ("text", text.into()),
        ]));
        self
    }

    pub fn label(mut self, text: &str) -> CustomForm {
        self.elements.push(NbtValue::obj([
            ("kind", "label".into()),
            ("text", text.into()),
        ]));
        self
    }

    pub fn divider(mut self) -> CustomForm {
        self.elements
            .push(NbtValue::obj([("kind", "divider".into())]));
        self
    }

    pub fn input(mut self, name: &str, text: &str, placeholder: &str, default: &str) -> CustomForm {
        self.elements.push(NbtValue::obj([
            ("kind", "input".into()),
            ("name", name.into()),
            ("text", text.into()),
            ("placeholder", placeholder.into()),
            ("default", default.into()),
        ]));
        self
    }

    pub fn toggle(mut self, name: &str, text: &str, default: bool) -> CustomForm {
        self.elements.push(NbtValue::obj([
            ("kind", "toggle".into()),
            ("name", name.into()),
            ("text", text.into()),
            ("default", NbtValue::Double(if default { 1.0 } else { 0.0 })),
        ]));
        self
    }

    pub fn dropdown(
        mut self,
        name: &str,
        text: &str,
        options: &[&str],
        default: usize,
    ) -> CustomForm {
        self.elements
            .push(choice("dropdown", name, text, options, default));
        self
    }

    pub fn step_slider(
        mut self,
        name: &str,
        text: &str,
        steps: &[&str],
        default: usize,
    ) -> CustomForm {
        self.elements
            .push(choice("step_slider", name, text, steps, default));
        self
    }

    /// A slider.
    ///
    /// An out-of-range default, or a `(max-min)` that is not a whole multiple of `step`,
    /// makes the Bedrock client render nothing of the form at all, which the player sees as
    /// it opening and disappearing. The host clamps and warns, and getting it right before
    /// passing it in is better.
    pub fn slider(
        mut self,
        name: &str,
        text: &str,
        min: f64,
        max: f64,
        step: f64,
        default: f64,
    ) -> CustomForm {
        self.elements.push(NbtValue::obj([
            ("kind", "slider".into()),
            ("name", name.into()),
            ("text", text.into()),
            ("min", NbtValue::Double(min)),
            ("max", NbtValue::Double(max)),
            ("step", NbtValue::Double(step)),
            ("default", NbtValue::Double(default)),
        ]));
        self
    }

    pub fn send(
        self,
        player: &Player,
        cb: impl FnOnce(FormResponse) + Send + 'static,
    ) -> Result<()> {
        let mut entries = vec![
            ("title".to_owned(), NbtValue::from(self.title.as_str())),
            ("elements".to_owned(), NbtValue::list(self.elements)),
        ];
        if let Some(sub) = &self.submit {
            entries.push(("submit".to_owned(), NbtValue::from(sub.as_str())));
        }
        send(player, 1, &NbtValue::obj(entries).to_snbt(), cb)
    }
}

fn choice(kind: &str, name: &str, text: &str, options: &[&str], default: usize) -> NbtValue {
    NbtValue::obj([
        ("kind", NbtValue::from(kind)),
        ("name", NbtValue::from(name)),
        ("text", NbtValue::from(text)),
        (
            "options",
            NbtValue::list(options.iter().map(|o| NbtValue::from(*o))),
        ),
        ("default", NbtValue::Double(default as f64)),
    ])
}

type Callback = dyn FnOnce(FormResponse) + Send + 'static;

/// The send path all three form kinds share.
fn send(
    player: &Player,
    kind: i32,
    form_snbt: &str,
    cb: impl FnOnce(FormResponse) + Send + 'static,
) -> Result<()> {
    let f = crate::require_slot!(form_send, "sending a form");
    let boxed: Box<Box<Callback>> = Box::new(Box::new(cb));
    let user = Box::into_raw(boxed);
    let ok = unsafe {
        f(
            rt().handle(),
            player.sel().raw(),
            kind,
            s(form_snbt),
            trampoline,
            user.cast(),
        )
    };
    if ok {
        Ok(())
    } else {
        // The host did not take it, the closure is still on this side, and ownership comes
        // back.
        drop(unsafe { Box::from_raw(user) });
        Err(Error(format!(
            "sending a form to player {player} failed: they are offline, or the form SNBT is invalid"
        )))
    }
}

/// # Safety
/// `user` must come from the `Box<Box<Callback>>::into_raw` inside `send` and be called
/// once.
unsafe extern "C" fn trampoline(user: *mut c_void, result_snbt: sys::PierStr) {
    if user.is_null() {
        return;
    }
    let f: Box<Box<Callback>> = Box::from_raw(user.cast());
    let raw = r(result_snbt);
    let response = parse_response(raw);
    let outcome = std::panic::catch_unwind(std::panic::AssertUnwindSafe(move || (*f)(response)));
    if outcome.is_err() {
        Logger::get()
            .error("a form callback panicked. It was caught here and this answer was discarded.");
    }
}

/// Parses the result SNBT the host wrote back.
fn parse_response(raw: &str) -> FormResponse {
    let Ok(v) = NbtValue::parse(raw) else {
        return FormResponse::Unknown {
            raw: raw.to_owned(),
        };
    };
    if v.opt_bool("cancelled").unwrap_or(false) {
        return FormResponse::Cancelled {
            reason: v.opt_i32("reason").unwrap_or(-1),
        };
    }
    match v.get("button") {
        // A modal form answers "upper" or "lower" while a simple form answers an index.
        Some(NbtValue::String(which)) => {
            return FormResponse::Modal {
                upper: which == "upper",
            }
        }
        Some(other) => {
            if let Some(n) = other.as_i64() {
                if n >= 0 {
                    return FormResponse::Button(n as usize);
                }
            }
        }
        None => {}
    }
    if let Some(values) = v.get("values").and_then(|x| x.as_compound()) {
        let mut out = std::collections::BTreeMap::new();
        for (k, val) in values {
            out.insert(k.clone(), to_form_value(val));
        }
        let mut texts = std::collections::BTreeMap::new();
        if let Some(t) = v.get("texts").and_then(|x| x.as_compound()) {
            for (k, val) in t {
                if let Some(sv) = val.as_str() {
                    texts.insert(k.clone(), sv.to_owned());
                }
            }
        }
        return FormResponse::Custom { values: out, texts };
    }
    FormResponse::Unknown {
        raw: raw.to_owned(),
    }
}

/// The SNBT tag type carries the control meaning: an integer comes from the index of a
/// dropdown or step slider, a float from a slider and a string from an input field. A
/// toggle also goes through an integer on the host side, so 0 and 1 land in `Index` and a
/// caller needing a boolean uses [`FormValue::as_bool`].
fn to_form_value(v: &NbtValue) -> FormValue {
    match v {
        NbtValue::String(s) => FormValue::Text(s.clone()),
        NbtValue::Double(d) => FormValue::Number(*d),
        NbtValue::Float(x) => FormValue::Number(*x as f64),
        other => match other.as_i64() {
            Some(n) if n >= 0 => FormValue::Index(n as usize),
            Some(n) => FormValue::Number(n as f64),
            None => FormValue::Text(other.to_snbt()),
        },
    }
}
