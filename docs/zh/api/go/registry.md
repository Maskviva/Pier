# Go：注册表

## 函数 {#functions}

### `RegistryList` {#RegistryList}

```go
func RegistryList(kind int32) ([]string, error)
```

列出引擎的某一个注册表，种类是某个 `PIER_REGISTRY_*`。

- 参数：
    - kind : `int32`
- 返回值类型：`([]string, error)`
- 对应槽位：[`registry_list`](../cpp/registry.md#registry_list)
