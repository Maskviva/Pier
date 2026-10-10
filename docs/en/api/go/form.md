# Go: Forms

## Functions {#functions}

### `SendForm` {#SendForm}

```go
func SendForm(player PlayerSel, kind int32, formSNBT string, fn func(result string)) error
```

SendForm shows a form to a player. fn runs once, on the server thread, with the result SNBT, including when the player closes the form; abi.h documents the result's shape.

- Parameters:
    - player : `PlayerSel`
    - kind : `int32`
    - formSNBT : `string`
    - fn : `func(result string)`
- Return type: `error`
- Slots: [`form_send`](../cpp/form.md#form_send)
