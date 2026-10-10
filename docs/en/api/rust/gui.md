# levilamina::gui

Forms: the three screens sent to a player, whose callback arrives asynchronously.

**The callback runs at most once and may never run**

When the player answers, or closes the form, the callback runs once on the server
thread. But if the mod is disabled before the player answers, the host mutes that
callback and it never arrives. Therefore:

* the callback is an `FnOnce` and is freed once it has run;
* on the muted path the memory holding the closure is leaked on purpose. The code able
  to free it lives in a dynamic library that may already be unloaded, and freeing it is
  the real use-after-free.

It follows that cleanup that has to happen does not belong in a form callback: a player
may never click.

## `CustomForm` {#CustomForm}

```rust
pub struct CustomForm {
    // private fields
}
```

A custom form: input fields, toggles, dropdowns and sliders.

- Implements: `Debug`, `Clone`, `Default`

### `CustomForm::new` {#CustomForm.new}

```rust
pub fn new(title: impl Into<String>) -> CustomForm
```

- Parameters:
    - title : `impl Into<String>`
- Return type: `CustomForm`

### `CustomForm::submit` {#CustomForm.submit}

```rust
pub fn submit(mut self, text: impl Into<String>) -> CustomForm
```

- Parameters:
    - text : `impl Into<String>`
- Return type: `CustomForm`

### `CustomForm::header` {#CustomForm.header}

```rust
pub fn header(mut self, text: &str) -> CustomForm
```

- Parameters:
    - text : `&str`
- Return type: `CustomForm`

### `CustomForm::label` {#CustomForm.label}

```rust
pub fn label(mut self, text: &str) -> CustomForm
```

- Parameters:
    - text : `&str`
- Return type: `CustomForm`

### `CustomForm::divider` {#CustomForm.divider}

```rust
pub fn divider(mut self) -> CustomForm
```

- Return type: `CustomForm`

### `CustomForm::input` {#CustomForm.input}

```rust
pub fn input(mut self, name: &str, text: &str, placeholder: &str, default: &str) -> CustomForm
```

- Parameters:
    - name : `&str`
    - text : `&str`
    - placeholder : `&str`
    - default : `&str`
- Return type: `CustomForm`

### `CustomForm::toggle` {#CustomForm.toggle}

```rust
pub fn toggle(mut self, name: &str, text: &str, default: bool) -> CustomForm
```

- Parameters:
    - name : `&str`
    - text : `&str`
    - default : `bool`
- Return type: `CustomForm`

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

- Parameters:
    - name : `&str`
    - text : `&str`
    - options : `&[&str]`
    - default : `usize`
- Return type: `CustomForm`

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

- Parameters:
    - name : `&str`
    - text : `&str`
    - steps : `&[&str]`
    - default : `usize`
- Return type: `CustomForm`

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

A slider.

An out-of-range default, or a `(max-min)` that is not a whole multiple of `step`,
makes the Bedrock client render nothing of the form at all, which the player sees as
it opening and disappearing. The host clamps and warns, and getting it right before
passing it in is better.

- Parameters:
    - name : `&str`
    - text : `&str`
    - min : `f64`
    - max : `f64`
    - step : `f64`
    - default : `f64`
- Return type: `CustomForm`

### `CustomForm::send` {#CustomForm.send}

```rust
pub fn send(
        self,
        player: &Player,
        cb: impl FnOnce(FormResponse) + Send + 'static,
    ) -> Result<()>
```

- Parameters:
    - player : `&Player`
    - cb : `impl FnOnce(FormResponse) + Send + 'static`
- Return type: `Result<()>`
- Slots: [`form_send`](../cpp/form.md#form_send)

## `SimpleForm` {#SimpleForm}

```rust
pub struct SimpleForm {
    // private fields
}
```

A simple form: a column of buttons.

- Implements: `Debug`, `Clone`, `Default`

### `SimpleForm::new` {#SimpleForm.new}

```rust
pub fn new(title: impl Into<String>) -> SimpleForm
```

- Parameters:
    - title : `impl Into<String>`
- Return type: `SimpleForm`

### `SimpleForm::content` {#SimpleForm.content}

```rust
pub fn content(mut self, content: impl Into<String>) -> SimpleForm
```

- Parameters:
    - content : `impl Into<String>`
- Return type: `SimpleForm`

### `SimpleForm::button` {#SimpleForm.button}

```rust
pub fn button(mut self, text: &str) -> SimpleForm
```

- Parameters:
    - text : `&str`
- Return type: `SimpleForm`

### `SimpleForm::button_with_image` {#SimpleForm.button_with_image}

```rust
pub fn button_with_image(mut self, text: &str, image: &str, image_type: &str) -> SimpleForm
```

A button with an icon. `image_type` is `"path"` or `"url"`.

- Parameters:
    - text : `&str`
    - image : `&str`
    - image_type : `&str`
- Return type: `SimpleForm`

### `SimpleForm::header` {#SimpleForm.header}

```rust
pub fn header(mut self, text: &str) -> SimpleForm
```

- Parameters:
    - text : `&str`
- Return type: `SimpleForm`

### `SimpleForm::label` {#SimpleForm.label}

```rust
pub fn label(mut self, text: &str) -> SimpleForm
```

- Parameters:
    - text : `&str`
- Return type: `SimpleForm`

### `SimpleForm::divider` {#SimpleForm.divider}

```rust
pub fn divider(mut self) -> SimpleForm
```

- Return type: `SimpleForm`

### `SimpleForm::send` {#SimpleForm.send}

```rust
pub fn send(
        self,
        player: &Player,
        cb: impl FnOnce(FormResponse) + Send + 'static,
    ) -> Result<()>
```

Sends it.

A form with no button at all cannot be pressed and can only be closed. It is stopped
here in advance, because it is almost always a list that assembled empty rather than
something intended.

- Parameters:
    - player : `&Player`
    - cb : `impl FnOnce(FormResponse) + Send + 'static`
- Return type: `Result<()>`
- Slots: [`form_send`](../cpp/form.md#form_send)

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

The value one control of a custom form returns.

- Implements: `Debug`, `Clone`, `PartialEq`

### `FormValue::as_index` {#FormValue.as_index}

```rust
pub fn as_index(&self) -> Option<usize>
```

- Return type: `Option<usize>`

### `FormValue::as_f64` {#FormValue.as_f64}

```rust
pub fn as_f64(&self) -> Option<f64>
```

- Return type: `Option<f64>`

### `FormValue::as_i64` {#FormValue.as_i64}

```rust
pub fn as_i64(&self) -> Option<i64>
```

- Return type: `Option<i64>`

### `FormValue::as_bool` {#FormValue.as_bool}

```rust
pub fn as_bool(&self) -> Option<bool>
```

- Return type: `Option<bool>`

### `FormValue::as_str` {#FormValue.as_str}

```rust
pub fn as_str(&self) -> Option<&str>
```

- Return type: `Option<&str>`

## `ModalForm` {#ModalForm}

```rust
pub struct ModalForm {
    // private fields
}
```

A modal form: some text and two buttons.

- Implements: `Debug`, `Clone`

### `ModalForm::new` {#ModalForm.new}

```rust
pub fn new(title: impl Into<String>, content: impl Into<String>) -> ModalForm
```

- Parameters:
    - title : `impl Into<String>`
    - content : `impl Into<String>`
- Return type: `ModalForm`

### `ModalForm::upper` {#ModalForm.upper}

```rust
pub fn upper(mut self, text: impl Into<String>) -> ModalForm
```

- Parameters:
    - text : `impl Into<String>`
- Return type: `ModalForm`

### `ModalForm::lower` {#ModalForm.lower}

```rust
pub fn lower(mut self, text: impl Into<String>) -> ModalForm
```

- Parameters:
    - text : `impl Into<String>`
- Return type: `ModalForm`

### `ModalForm::send` {#ModalForm.send}

```rust
pub fn send(
        self,
        player: &Player,
        cb: impl FnOnce(FormResponse) + Send + 'static,
    ) -> Result<()>
```

- Parameters:
    - player : `&Player`
    - cb : `impl FnOnce(FormResponse) + Send + 'static`
- Return type: `Result<()>`
- Slots: [`form_send`](../cpp/form.md#form_send)

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

A player's answer to a form.

- Implements: `Debug`, `Clone`, `PartialEq`

### `FormResponse::is_cancelled` {#FormResponse.is_cancelled}

```rust
pub fn is_cancelled(&self) -> bool
```

- Return type: `bool`

### `FormResponse::value` {#FormResponse.value}

```rust
pub fn value(&self, name: &str) -> Option<&FormValue>
```

Reads the value of one control of a custom form.

- Parameters:
    - name : `&str`
- Return type: `Option<&FormValue>`

### `FormResponse::text` {#FormResponse.text}

```rust
pub fn text(&self, name: &str) -> Option<&str>
```

Reads the text of the selected item of one control of a custom form.

- Parameters:
    - name : `&str`
- Return type: `Option<&str>`
