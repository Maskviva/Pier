# levilamina::gui · 表单

表单：发给玩家的三种界面，回调是异步到达的。

**回调最多运行一次，也可能永远不运行**

玩家回答或者关掉表单时，回调在服务器线程上运行一次。但如果玩家回答之前模组被禁用了，宿主会屏蔽这个回调，它永远不会到达。因此：

- 回调是 `FnOnce`，运行一次以后就被释放；
- 在被屏蔽的那条路径上，存放闭包的内存是有意泄漏的。能释放它的代码在一个可能已经卸载的动态库里，去释放它才是真正的释放后使用。

所以，必须执行的清理工作不该放在表单回调里：玩家可能永远不点。

## `CustomForm` {#CustomForm}

```rust
pub struct CustomForm {
    // private fields
}
```

自定义表单：输入框、开关、下拉框和滑块。

- 实现的 trait：`Debug`、`Clone`、`Default`

### `CustomForm::new` {#CustomForm.new}

```rust
pub fn new(title: impl Into<String>) -> CustomForm
```

- 参数：
    - title : `impl Into<String>`
- 返回值类型：`CustomForm`

### `CustomForm::submit` {#CustomForm.submit}

```rust
pub fn submit(mut self, text: impl Into<String>) -> CustomForm
```

- 参数：
    - text : `impl Into<String>`
- 返回值类型：`CustomForm`

### `CustomForm::header` {#CustomForm.header}

```rust
pub fn header(mut self, text: &str) -> CustomForm
```

- 参数：
    - text : `&str`
- 返回值类型：`CustomForm`

### `CustomForm::label` {#CustomForm.label}

```rust
pub fn label(mut self, text: &str) -> CustomForm
```

- 参数：
    - text : `&str`
- 返回值类型：`CustomForm`

### `CustomForm::divider` {#CustomForm.divider}

```rust
pub fn divider(mut self) -> CustomForm
```

- 返回值类型：`CustomForm`

### `CustomForm::input` {#CustomForm.input}

```rust
pub fn input(mut self, name: &str, text: &str, placeholder: &str, default: &str) -> CustomForm
```

- 参数：
    - name : `&str`
    - text : `&str`
    - placeholder : `&str`
    - default : `&str`
- 返回值类型：`CustomForm`

### `CustomForm::toggle` {#CustomForm.toggle}

```rust
pub fn toggle(mut self, name: &str, text: &str, default: bool) -> CustomForm
```

- 参数：
    - name : `&str`
    - text : `&str`
    - default : `bool`
- 返回值类型：`CustomForm`

### `CustomForm::dropdown` {#CustomForm.dropdown}

```rust
pub fn dropdown(
        mut self,
        name: &str,
        text: &str,
        options: &[&str],
        default: usize,
    ) -> CustomForm
```

- 参数：
    - name : `&str`
    - text : `&str`
    - options : `&[&str]`
    - default : `usize`
- 返回值类型：`CustomForm`

### `CustomForm::step_slider` {#CustomForm.step_slider}

```rust
pub fn step_slider(
        mut self,
        name: &str,
        text: &str,
        steps: &[&str],
        default: usize,
    ) -> CustomForm
```

- 参数：
    - name : `&str`
    - text : `&str`
    - steps : `&[&str]`
    - default : `usize`
- 返回值类型：`CustomForm`

### `CustomForm::slider` {#CustomForm.slider}

```rust
pub fn slider(
        mut self,
        name: &str,
        text: &str,
        min: f64,
        max: f64,
        step: f64,
        default: f64,
    ) -> CustomForm
```

一个滑块。

默认值超出范围，或者 `(max-min)` 不是 `step` 的整数倍，基岩版客户端会把整个表单都画不出来，玩家看到的是表单一打开就消失。宿主会截断并发出警告，但传进来之前就弄对更好。

- 参数：
    - name : `&str`
    - text : `&str`
    - min : `f64`
    - max : `f64`
    - step : `f64`
    - default : `f64`
- 返回值类型：`CustomForm`

### `CustomForm::send` {#CustomForm.send}

```rust
pub fn send(
        self,
        player: &Player,
        cb: impl FnOnce(FormResponse) + Send + 'static,
    ) -> Result<()>
```

- 参数：
    - player : `&Player`
    - cb : `impl FnOnce(FormResponse) + Send + 'static`
- 返回值类型：`Result<()>`
- 对应槽位：[`form_send`](../cpp/form.md#form_send)

## `SimpleForm` {#SimpleForm}

```rust
pub struct SimpleForm {
    // private fields
}
```

简单表单：一列按钮。

- 实现的 trait：`Debug`、`Clone`、`Default`

### `SimpleForm::new` {#SimpleForm.new}

```rust
pub fn new(title: impl Into<String>) -> SimpleForm
```

- 参数：
    - title : `impl Into<String>`
- 返回值类型：`SimpleForm`

### `SimpleForm::content` {#SimpleForm.content}

```rust
pub fn content(mut self, content: impl Into<String>) -> SimpleForm
```

- 参数：
    - content : `impl Into<String>`
- 返回值类型：`SimpleForm`

### `SimpleForm::button` {#SimpleForm.button}

```rust
pub fn button(mut self, text: &str) -> SimpleForm
```

- 参数：
    - text : `&str`
- 返回值类型：`SimpleForm`

### `SimpleForm::button_with_image` {#SimpleForm.button_with_image}

```rust
pub fn button_with_image(mut self, text: &str, image: &str, image_type: &str) -> SimpleForm
```

一个带图标的按钮。`image_type` 是 `"path"` 或 `"url"`。

- 参数：
    - text : `&str`
    - image : `&str`
    - image_type : `&str`
- 返回值类型：`SimpleForm`

### `SimpleForm::header` {#SimpleForm.header}

```rust
pub fn header(mut self, text: &str) -> SimpleForm
```

- 参数：
    - text : `&str`
- 返回值类型：`SimpleForm`

### `SimpleForm::label` {#SimpleForm.label}

```rust
pub fn label(mut self, text: &str) -> SimpleForm
```

- 参数：
    - text : `&str`
- 返回值类型：`SimpleForm`

### `SimpleForm::divider` {#SimpleForm.divider}

```rust
pub fn divider(mut self) -> SimpleForm
```

- 返回值类型：`SimpleForm`

### `SimpleForm::send` {#SimpleForm.send}

```rust
pub fn send(
        self,
        player: &Player,
        cb: impl FnOnce(FormResponse) + Send + 'static,
    ) -> Result<()>
```

发送这个表单。

一个按钮都没有的表单没法点，只能关掉。这里提前拦下，因为这种情况几乎总是一个拼装出来是空的列表，很少是有意为之。

- 参数：
    - player : `&Player`
    - cb : `impl FnOnce(FormResponse) + Send + 'static`
- 返回值类型：`Result<()>`
- 对应槽位：[`form_send`](../cpp/form.md#form_send)

## `FormValue` {#FormValue}

```rust
pub enum FormValue {
        /// The selected index of a dropdown or a step slider.
        Index(usize),
        Number(f64),
        Text(String),
        Bool(bool),
}
```

自定义表单里一个控件返回的值。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`

### `FormValue::as_index` {#FormValue.as_index}

```rust
pub fn as_index(&self) -> Option<usize>
```

- 返回值类型：`Option<usize>`

### `FormValue::as_f64` {#FormValue.as_f64}

```rust
pub fn as_f64(&self) -> Option<f64>
```

- 返回值类型：`Option<f64>`

### `FormValue::as_i64` {#FormValue.as_i64}

```rust
pub fn as_i64(&self) -> Option<i64>
```

- 返回值类型：`Option<i64>`

### `FormValue::as_bool` {#FormValue.as_bool}

```rust
pub fn as_bool(&self) -> Option<bool>
```

- 返回值类型：`Option<bool>`

### `FormValue::as_str` {#FormValue.as_str}

```rust
pub fn as_str(&self) -> Option<&str>
```

- 返回值类型：`Option<&str>`

## `ModalForm` {#ModalForm}

```rust
pub struct ModalForm {
    // private fields
}
```

模态表单：一段文字和两个按钮。

- 实现的 trait：`Debug`、`Clone`

### `ModalForm::new` {#ModalForm.new}

```rust
pub fn new(title: impl Into<String>, content: impl Into<String>) -> ModalForm
```

- 参数：
    - title : `impl Into<String>`
    - content : `impl Into<String>`
- 返回值类型：`ModalForm`

### `ModalForm::upper` {#ModalForm.upper}

```rust
pub fn upper(mut self, text: impl Into<String>) -> ModalForm
```

- 参数：
    - text : `impl Into<String>`
- 返回值类型：`ModalForm`

### `ModalForm::lower` {#ModalForm.lower}

```rust
pub fn lower(mut self, text: impl Into<String>) -> ModalForm
```

- 参数：
    - text : `impl Into<String>`
- 返回值类型：`ModalForm`

### `ModalForm::send` {#ModalForm.send}

```rust
pub fn send(
        self,
        player: &Player,
        cb: impl FnOnce(FormResponse) + Send + 'static,
    ) -> Result<()>
```

- 参数：
    - player : `&Player`
    - cb : `impl FnOnce(FormResponse) + Send + 'static`
- 返回值类型：`Result<()>`
- 对应槽位：[`form_send`](../cpp/form.md#form_send)

## `FormResponse` {#FormResponse}

```rust
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
```

玩家对一个表单的回答。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`

### `FormResponse::is_cancelled` {#FormResponse.is_cancelled}

```rust
pub fn is_cancelled(&self) -> bool
```

- 返回值类型：`bool`

### `FormResponse::value` {#FormResponse.value}

```rust
pub fn value(&self, name: &str) -> Option<&FormValue>
```

读取自定义表单里一个控件的值。

- 参数：
    - name : `&str`
- 返回值类型：`Option<&FormValue>`

### `FormResponse::text` {#FormResponse.text}

```rust
pub fn text(&self, name: &str) -> Option<&str>
```

读取自定义表单里一个控件所选那一项的文字。

- 参数：
    - name : `&str`
- 返回值类型：`Option<&str>`
