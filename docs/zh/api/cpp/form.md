# 表单

??? note "abi.h 里的分节说明"

    **§G 表单（异步的结果回调）（`§G forms (async result callback)`）**

## 槽位 {#slots}

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

`kind`：0=SimpleForm，1=CustomForm，2=ModalForm。`form_snbt` 描述表单（见 docs/api/gui）。回调只触发一次，在服务器线程上；如果玩家回应之前模组被禁用了，回调会被屏蔽。

- 调用形式：`api->form_send(mod, sel, kind, form_snbt, cb, user)`
- 参数：
    - mod : `PierModHandle`
    - sel : `PierPlayerSel`
    - kind : `int32_t`
    - form_snbt : `PierStr`
    - cb : `PierFormResultCb`
    - user : `void*`
- 返回值类型：`bool`
- 所在分节：§G 表单（异步的结果回调）（`§G forms (async result callback)`）
- 表内序号：第 53 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`SimpleForm::send`](../rust/gui.md#SimpleForm.send)、[`ModalForm::send`](../rust/gui.md#ModalForm.send)、[`CustomForm::send`](../rust/gui.md#CustomForm.send)
    - Go：[`SendForm`](../go/form.md#SendForm)

## 类型 {#types}

### `PierFormResultCb` {#PierFormResultCb}

```c
typedef void (*PierFormResultCb)(void* user, PierStr result_snbt);
```

表单的结果回调。玩家回应（或者表单被取消）时，在服务器线程上调用**一次**。`result_snbt` 的形状：

```text
cancelled       : {cancelled:1b, reason:N}
SimpleForm      : {button:N}
CustomForm      : {values:{<name>: string|double|int64…}}
ModalForm       : {button:"upper"|"lower"}
```

如果玩家回应之前模组被禁用了，回调会被屏蔽，不会被调用。
