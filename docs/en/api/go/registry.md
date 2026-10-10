# Go: Registries

## Functions {#functions}

### `RegistryList` {#RegistryList}

```go
func RegistryList(kind int32) ([]string, error)
```

RegistryList lists one of the engine's registries, one of the PIER\_REGISTRY\_\* kinds.

- Parameters:
    - kind : `int32`
- Return type: `([]string, error)`
- Slots: [`registry_list`](../cpp/registry.md#registry_list)
