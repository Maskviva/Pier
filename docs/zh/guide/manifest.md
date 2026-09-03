# manifest

每个 Pier 模组都是 `mods/` 下的一个目录，里面放着动态库和 `manifest.json`：

```
mods/
  my-mod/
    my_mod.dll
    manifest.json
```

```json
{
  "name": "my-mod",
  "entry": "my_mod.dll",
  "type": "pier",
  "version": "0.1.0",
  "description": "这个模组做什么。",
  "dependencies": [{ "name": "pier" }]
}
```

## 字段

| 字段 | 必填 | 说明 |
|---|---|---|
| `name` | 是 | 必须和目录名一致 |
| `entry` | 是 | 构建产出的那个库文件 |
| `type` | 是 | 必须正好是 `pier` |
| `version` | 是 | 语义化版本 |
| `description` | 否 | 显示在 `/pier list` 里 |
| `dependencies` | 是 | 必须含 `{ "name": "pier" }` |

## 最容易错的两个

**`type` 必须正好是 `pier`。** 宿主拿它和一个字符串做字面比较。写成别的，模组根本不会
被扫到：不报错、不打日志，就是不出现在 `/pier list` 里。Pier 自己的 CI 有一条检查守这个，
因为它失败的时候什么都不报。

**`entry` 必须和构建产出对得上。** cargo 会把 crate 名里的连字符变成下划线，
所以 crate `my-mod` 产出的是 `my_mod.dll`。这一条最容易失手。

## 依赖

写上 `pier` 是为了给装载排序，让宿主先于你的模组存在。写成别的名字，
就等于依赖一个不存在的模组。

依赖另一个 Pier 模组同理：

```json
"dependencies": [
  { "name": "pier" },
  { "name": "plot-manager" }
]
```

这只排装载顺序，不会让对方的 API 可达。要通信请用
[服务或总线](/zh/rust/cross-mod)。
