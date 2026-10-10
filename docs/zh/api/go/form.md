# Go：表单

## 函数 {#functions}

### `SendForm` {#SendForm}

```go
func SendForm(player PlayerSel, kind int32, formSNBT string, fn func(result string)) error
```

向一名玩家显示一个表单。`fn` 在服务器线程上运行一次，收到结果的 SNBT，玩家关掉表单时也会运行；结果的形状写在 abi.h 里。

- 参数：
    - player : `PlayerSel`
    - kind : `int32`
    - formSNBT : `string`
    - fn : `func(result string)`
- 返回值类型：`error`
- 对应槽位：[`form_send`](../cpp/form.md#form_send)
