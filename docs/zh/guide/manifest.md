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
| `reload_safe` | 否 | 为 `true` 才允许 `/pier reload`，默认 false |
| `load_level` | 否 | 整数，默认 0，越小越早加载 |

## 最容易错的两个

**`type` 必须正好是 `pier`。** 宿主拿它和一个字符串做字面比较。写成别的，模组根本不会
被扫到：不报错、不打日志，就是不出现在 `/pier list` 里。Pier 自己的 CI 有一条检查守这个，
因为它失败的时候什么都不报。

**`entry` 必须和构建产出对得上。** cargo 会把 crate 名里的连字符变成下划线，
所以 crate `my-mod` 产出的是 `my_mod.dll`。这一条最容易失手。

## 加载顺序

模组按 `load_level` 从小到大加载，同级之间按名字。不写这个字段就和大家一起待在 0，
这也是「没有偏好」的模组该待的地方。

```json
{ "load_level": -10 }
```

**等级只排序，不保证时序。** 如果你的模组必须等另一个先加载好才能工作，那是**依赖**，
宿主会检查它、缺了就拒绝加载你。等级只用来表达「在有偏好但没有依赖可挂」时往哪边靠——
比如一个权限管理器希望在任何人来查询之前先把门禁装好。

值不是整数会在 `/pier list` 里报成 problem，而不是当作 0。写 `"load_level": "early"`
说明作者本来就想要一个顺序，默默给他默认值正是本项目在别处一直拒绝的那种静默兜底。

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
