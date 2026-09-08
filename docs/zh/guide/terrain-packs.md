# 地形包

自定义维度的地形有三种。

| 种类 | 是什么 | 用哪个入口注册 |
|---|---|---|
| `native` | 原版生成器：带结构的 `overworld`、`nether`、`end`，以及 `flat` 和 `void` | `md_add_dimension` |
| `template` | 范围锁死、带参数、每个格子数据相同的世界，由 PIERTPL 包填充 | `md_add_dimension_pack` |
| `volume` | Java 风格的密度函数图，由 PIERVOL 包填充 | `md_add_dimension_pack` |

一个包是一个目录，里面有一个配置文件和一个二进制。模组自己分发它；宿主只读配置里的四个键，
别的一概不看：

```json
{
  "pier_terrain": 1,
  "type": "template",
  "binary": "terrain.ptpl",
  "sha256": "<terrain.ptpl 的 64 位十六进制哈希>",
  "title": "地皮世界",
  "anything_else": "是模组自己的东西"
}
```

`binary` 相对配置文件；模组传给宿主的配置路径相对服务端根目录，用正斜杠，不能是绝对路径，
不能含 `..`。`sha256` 的值用 `pier-pack hash` 算。

## 三处说法必须一致

在写任何东西之前，宿主会把 spec 的 `terrain.kind`、配置的 `type`、二进制的 magic 三处对上，
并用配置里的 `sha256` 校验二进制。任何一处对不上都以 `PIER_PACK_*` 码拒绝，什么都不落盘。
通过之后，存档里的 spec 记下路径、那个哈希、每个绑定后的参数和角色覆盖，因此只凭存档就能把
地形重新生成出来。

下次启动以存档为准：同名返回同一个 id；这次传的参数如果和存档不同，会被忽略并记一条警告；
配置里的哈希如果不再是存档里那个，以 `PIER_PACK_STORED_MISMATCH` 拒绝，因为换了二进制的地形
接不上这个世界。要改包，就换一个世界或换一个名字。

## 模板包

模板包把范围钉死，把内容留给参数：作者决定一个格子长什么样、有哪些旋钮，模组决定旋钮的值。

- **参数**分四种。`free` 取 `[min, max]` 内按 `step` 的任意值；`fixed` 改不了；`choice` 取
  列表里的一个；`derived` 是前面参数的表达式，调用方不能给。`md_pack_inspect` 会把它们列出来，
  模组照着做表单：滑块、只读值、下拉框，`derived` 不展示。
- **角色**是包里用符号命名的方块，比如 `floor`、`road`、`wall`，各有默认值，模组可以在
  `terrain.roles` 里覆盖。
- 两个轴上的**分区**（长度是表达式）合成每一列的 2D 分区，每个分区一份**堆叠**，按画家顺序涂。
- **约束**用绑定后的值检查，不通过就带着它自己的消息拒绝。
- **形状**是基本体和体素块组成的 CSG 图，按格子带权重和朝向**挑选**；`choose` 按 choice 参数
  或按格子哈希选一个子树。
- **confine** 用同一次挂载得到的几何注册格子网格，供 `PIER_DIMRULE_PISTON_CROSS_CELL` 和
  `PIER_DIMRULE_ENTITY_CROSS_CELL` 使用，所以规则看到的网格就是地形画出来的网格。

源是 JSON，地皮世界见 `tools/pier-pack/fixtures/plot.json`，形状和体素见 `town.json`。构建：

```
python3 -m pierpack.cli build plot.json -o terrain.ptpl
python3 -m pierpack.cli inspect terrain.ptpl
python3 -m pierpack.cli hash terrain.ptpl
```

地形包之前的 `{kind:"layers"}` spec 用 `from-layers` 转换。

## 体积包

体积包是一份 Java 的 `noise_settings`，把它引用到的东西一起收进来：按 id 的密度函数、按 id 的
噪声、多噪声 biome 参数表、surface rule。包里存的是噪声参数而不是噪声表；表由宿主按世界种子
用 Java 的算法生成，Xoroshiro128++，或者在 `legacy_random_source` 下用旧随机源，所以一个包能
服务任意种子。

```
python3 -m pierpack.cli from-datapack <数据包目录> minecraft:overworld -o overworld.json
python3 -m pierpack.cli build overworld.json -o terrain.pvol
```

`biome_source` 如果写的是预设，得先把 biome 列表展开写进维度文件里：预设在游戏里，不在数据包里。
支持 31 个密度函数算子、样条、多噪声 biome 表，以及 surface rule 的这些：`block`、`sequence`、
`condition`、`biome`、`noise_threshold`、`vertical_gradient`、`y_above`、`water`、
`temperature`、`steep`、`not`、`hole`、`above_preliminary_surface`、`stone_depth`。构建时明确
拒绝：`bandlands`。没有建模：含水层和矿脉。一列的 biome 取的是它表面那一格的。

## 模组这边

```rust
let json = pier::dimensions::pack_inspect("packs/plot/config.json")?;
let spec = r#"{seed:12345,terrain:{kind:"template",params:{plot_size:64,road_width:7},roles:{floor:"minecraft:stone"}}}"#;
let id = pier::dimensions::add_dimension_pack("plots", "packs/plot/config.json", spec)?;
```

拒绝是一个带 `PackStatus` 的 `PackError`；原因在宿主日志里，对同一个路径调 `pack_inspect`
也会把原因以 JSON 返回。

## 格式怎么验证的

`tools/pier-pack/tests` 里有：布局测试（Python 镜像对着 `pack_format.h` 核对）、地皮等价测试
（对着 26.20.2 `PlotGenerator` 的逐行移植）、两个等价测试（用 g++ 编译无引擎依赖的
`packages/pier-dimensions/src/pack`，与 Python 参考生成器逐格对比）。在 `tools/pier-pack`
下用 `python3 tests/<名字>.py` 跑。
