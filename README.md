# Pier（栈桥）

一头钉在 BDS 的地基上，另一头任何语言的船都能靠。

- **契约（产品本体）**：`packages/pier-abi/include/sdk/abi.h` —— C11 可解析，
  单一布局，能力缺席 = 槽位 NULL。
- **规矩**：`CONTRACT.md`。改代码之前先读它；和它冲突的代码是要改的那一方。
- **迁移台账**：`MIGRATION.md` —— 「功能只多不少」的逐文件对账单，交付自查
  按它逐行清点。统计行由 `tools/ledger-count.py` 从表体算出，不是手写的。

## 构建

```bash
xmake f --target_type=server && xmake   # 产物 pier.dll（宿主本体）
cargo build --release                    # SDK 与示例
```

两条构建线之间**没有构建依赖**，只有契约依赖：Rust 侧读的是那份头文件，
读法是手写镜像（`packages/pier-sys-rs`），一致性由机检守着。所以「宿主编不过」
和「绑定编不过」是两件可以分别修的事。

## 检查

```bash
python3 tools/run-checks.py     # 契约 §九 的十三条，任一条红就非零退出
```

每条检查的输出里写着**它覆盖到哪、看不见什么**。交付说明照抄那句话 ——
不许把「静态必要条件通过」写成「这条性质成立」。

```bash
python3 tools/run-surrogates.py  # 六个 surrogate，**不是**契约检查
```

surrogate 是没有工具链时的临时替身，每一个都对应一次真机报错：

| 脚本 | 代偿谁 | 对应的真机报错 |
|---|---|---|
| `link-surrogate` | 链接器 | 未解析的外部符号 |
| `include-surrogate` | C++ 编译器 | `C2065` / `C2039` |
| `rust-surrogate` | `cargo clippy` | `cannot find type` / `never used` |
| `example-surrogate` | `cargo check` | 示例用了 prelude 没导出的符号 |
| `typed-storage` | C++ 编译器 | `C2228` / `C2039`（TypedStorage 坍缩规则）|
| `ledger-count` | 人工清点 | （无对应，纯代偿） |

有 MSVC / cargo 之后，前五个是冗余的 —— 但 `typed-storage` 仍有用：编译器
一次只报第一个失败的 TU，它一次把全仓的调用点都验了。它需要引擎头，
用 `PIER_LL_INCLUDE` 指过去；找不到时报 **SKIP 而不是 PASS**。

## 写一个模组

```rust
use levilamina::prelude::*;

struct MyMod;

impl LeviMod for MyMod {
    fn on_load(ctx: &ModContext) -> Result<Self> {
        ctx.logger().info("你好，栈桥。");
        Ok(MyMod)
    }
}

levilamina::register_mod!(MyMod);
```

`examples/hello-pier` 是契约 §十「加一门语言」四步的可运行参考。

manifest 写法（`mods/<名字>/manifest.json`）：

```json
{
  "type": "pier",
  "name": "<名字，必须与目录一致>",
  "entry": "<dll 文件名>",
  "dependencies": [{ "name": "pier" }],
  "reload_safe": false
}
```

`"type"` 写错一个字，宿主根本不会扫到这个模组，而且**不会有任何报错** ——
`manifest-matches-host` 机检就是守这一条的。

## 换一门语言

只需要 `packages/pier-abi/include/sdk/abi.h` 一个文件。四步见 `CONTRACT.md`
第十节，参考实现是 `packages/pier-sys-rs`。新语言的绑定放
`packages/pier-<lang>`，不进主仓库的必编列表 —— Pier 维护的是契约。

## 当前进度

C++ 侧（宿主本体）**全量完成**；Rust 侧完成契约层与运行时地基，域封装未完。
逐文件状态见 `MIGRATION.md`，别信这一段的印象。
