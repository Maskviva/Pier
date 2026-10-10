# Forms

??? note "Section notes in abi.h"

    **§G forms (async result callback)**

## Slots {#slots}

### `form_send` {#form_send}

```c
bool (*form_send)(
    PierModHandle mod,
    PierPlayerSel sel,
    int32_t kind,
    PierStr form_snbt,
    PierFormResultCb cb,
    void* user
);
```

kind: 0=SimpleForm 1=CustomForm 2=ModalForm. `form_snbt` describes the form (see docs/api/gui). The callback fires once, on the server thread, and is muted if the mod is disabled before the player responds.

- Call: `api->form_send(mod, sel, kind, form_snbt, cb, user)`
- Parameters:
    - mod : `PierModHandle`
    - sel : `PierPlayerSel`
    - kind : `int32_t`
    - form_snbt : `PierStr`
    - cb : `PierFormResultCb`
    - user : `void*`
- Return type: `bool`
- Section of abi.h: §G forms (async result callback)
- Position in the table: slot 53, counting from 0
- Callers in each binding:
    - Rust: [`SimpleForm::send`](../rust/gui.md#SimpleForm.send), [`ModalForm::send`](../rust/gui.md#ModalForm.send), [`CustomForm::send`](../rust/gui.md#CustomForm.send)
    - Go: [`SendForm`](../go/form.md#SendForm)

## Types {#types}

### `PierFormResultCb` {#PierFormResultCb}

```c
typedef void (*PierFormResultCb)(void* user, PierStr result_snbt);
```

Form result callback. Invoked ONCE on the server thread when the player responds (or the form is cancelled). `result_snbt`:

```text
cancelled       : {cancelled:1b, reason:N}
SimpleForm      : {button:N}
CustomForm      : {values:{<name>: string|double|int64…}}
ModalForm       : {button:"upper"|"lower"}
```

Muted (never called) if the mod is disabled before the player responds.
