//! 表单 —— 发给玩家的三种界面，回调异步到达。
//!
//! # 回调**至多**跑一次，也可能一次都不跑
//!
//! 玩家回应（或关掉表单）时回调在服务器线程上跑一次。但如果模组在玩家回应
//! 之前被停用，宿主会把这次回调**静音** —— 它永远不来。所以：
//!
//! * 回调是 `FnOnce`，跑完就释放；
//! * 静音的那一路里，装着闭包的那块内存**故意泄漏**。能释放它的代码住在
//!   可能已经卸载的动态库里，释放才是真正的 use-after-free。
//!
//! 推论：别把「一定要做的收尾」放进表单回调。玩家可以永远不点。

use core::ffi::c_void;

use crate::nbt::NbtValue;
use crate::player::Player;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{r, s};
use crate::rt::logger::Logger;
use crate::rt::runtime::rt;
use crate::sys;

/// 自定义表单里一个控件回传的值。
#[derive(Debug, Clone, PartialEq)]
pub enum FormValue {
    /// 下拉框 / 步进滑块选中的下标。
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

/// 玩家对一个表单的回应。
#[derive(Debug, Clone, PartialEq)]
pub enum FormResponse {
    /// 玩家关掉了表单。`reason` 是引擎给的取消原因，拿不到时是 -1。
    Cancelled { reason: i32 },
    /// 简单表单：点了第几个按钮。
    Button(usize),
    /// 模态表单：`true` 是上面那个按钮。
    Modal { upper: bool },
    /// 自定义表单：按控件名索引。
    ///
    /// `values` 是原始值，`texts` 是下拉框/步进滑块选中项的**文本** ——
    /// 两份都给是因为选项列表可能在发出之后变了，只有下标的话对不回去。
    Custom {
        values: std::collections::BTreeMap<String, FormValue>,
        texts: std::collections::BTreeMap<String, String>,
    },
    /// 宿主回了一个这一侧读不懂的形状。**不吞掉** —— 静默当成取消会让
    /// 「玩家点了确定但什么都没发生」变成一个查不下去的问题。
    Unknown { raw: String },
}

impl FormResponse {
    pub fn is_cancelled(&self) -> bool {
        matches!(self, FormResponse::Cancelled { .. })
    }

    /// 自定义表单里取一个控件的值。
    pub fn value(&self, name: &str) -> Option<&FormValue> {
        match self {
            FormResponse::Custom { values, .. } => values.get(name),
            _ => None,
        }
    }

    /// 自定义表单里取一个控件选中项的文本。
    pub fn text(&self, name: &str) -> Option<&str> {
        match self {
            FormResponse::Custom { texts, .. } => texts.get(name).map(|s| s.as_str()),
            _ => None,
        }
    }
}

/// 简单表单：一列按钮。
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

    /// 带图标的按钮。`image_type` 取 `"path"` 或 `"url"`。
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

    /// 发出去。
    ///
    /// 一个按钮都没有的表单玩家点不动，只能关掉 —— 这里提前挡下，因为它
    /// 几乎总是「列表拼空了」而不是有意为之。
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
                "简单表单 {:?} 一个按钮都没有，玩家点不动它",
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

/// 模态表单：一段文字加两个按钮。
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

/// 自定义表单：输入框、开关、下拉框、滑块。
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

    /// 滑块。
    ///
    /// 越界的默认值、或者 `(max-min)` 不是 `step` 的整数倍，会让基岩客户端
    /// **整个表单不渲染**（玩家看到的是「打开就没了」）。宿主侧会钳制并告警，
    /// 但传进去之前自己算对更好。
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

/// 三种表单共用的发送路径。
fn send(
    player: &Player,
    kind: i32,
    form_snbt: &str,
    cb: impl FnOnce(FormResponse) + Send + 'static,
) -> Result<()> {
    let f = crate::require_slot!(form_send, "发送表单");
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
        // 宿主没收下，闭包还在这一侧，收回所有权。
        drop(unsafe { Box::from_raw(user) });
        Err(Error(format!(
            "给玩家 {player} 发表单失败（不在线，或表单 SNBT 不合法）"
        )))
    }
}

/// # Safety
/// `user` 必须是 `send` 里 `Box<Box<Callback>>::into_raw` 的产物，且只被调一次。
unsafe extern "C" fn trampoline(user: *mut c_void, result_snbt: sys::PierStr) {
    if user.is_null() {
        return;
    }
    let f: Box<Box<Callback>> = Box::from_raw(user.cast());
    let raw = r(result_snbt);
    let response = parse_response(raw);
    let outcome = std::panic::catch_unwind(std::panic::AssertUnwindSafe(move || (*f)(response)));
    if outcome.is_err() {
        Logger::get().error("表单回调 panic 了。已就地拦下 —— 这次回应被丢弃。");
    }
}

/// 解析宿主写回来的结果 SNBT。
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
        // 模态表单回的是 "upper"/"lower"，简单表单回的是下标。
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

/// SNBT 的标签类型带着控件语义：整型来自下拉框/步进滑块的下标，浮点来自
/// 滑块，字符串来自输入框。开关在宿主侧也走整型，所以 0/1 归到 `Index`，
/// 需要布尔的调用方用 [`FormValue::as_bool`]。
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
