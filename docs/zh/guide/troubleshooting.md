# 排错

Pier 的每一次拒绝都会把原因写进日志，所以第一步永远是看服务器控制台或 `logs/latest.log`。
这一页列出会遇到的日志行，以及各自该怎么办。

## 模组加载不了

| 日志写着 | 含义 | 怎么办 |
|---|---|---|
| `'x' does not export pier_main` | 这个 DLL 不是 Pier 模组，或者入口符号没导出 | 用绑定的入口宏构建，C++ 用 `PIER_MAIN_EXPORT` |
| `'x': pier_main returned false` | 模组自己拒绝启动，原因在它自己的日志里 | 看紧挨着的上面几行 |
| `'x': an exception left pier_main` | 模组让异常从入口点抛了出来 | 修模组：任何栈展开都不能进入宿主 |
| `'x' was built against Pier ABI vN, below the minimum vM` | 模组早于当前 ABI | 用当前 SDK 重新编译模组 |
| 一行提到客户端或服务端目标 | 客户端构建装到了服务端，或者反过来 | 装对应这一端的构建 |
| `'x' filled in a vtable of N bytes` | 模组用的 SDK 没填 `struct_size` | 更新模组构建时用的 SDK |
| 模组出现在 `mods.disabled` 里 | 服主把它关掉了 | 从 `config.json` 的 `mods.disabled` 里删掉 |

## 服务器起不来，报错 0x7E

`0x7E, the specified module could not be found` 表示 Windows 找不到 Pier 或某个模组依赖的 DLL。
Pier 本身在缺少可选依赖（比如 LegacyMoney）时也能加载；用 MinGW 编译的模组需要旁边放上 MinGW 的运行时 DLL，
Debug 构建的模组需要调试版 C++ 运行时。用 MSVC 或 clang-cl 以 Release 重新编译，或者把它依赖的 DLL 一起带上。

## 服务器卡住了，或以退出码 70 退出

退出码 70 来自 Pier 的看门狗。某个模组占住服务器线程超过了 `watchdog.hang_ms`，
或者关服时超过了 `watchdog.shutdown_ms`，进程被结束，好让它被重启。前一行会点名模组、卡在哪个入口、卡了多久：

```
[watchdog] 'land-claims' has held thread 4812 inside event for 30000 ms, past the runtime
limit of 30000 ms; ending the process with exit code 70 so it can be restarted
```

把它报给那个模组的作者。如果模组确实要在启动时干这么久，调大 `watchdog.hang_ms`，不要关掉看门狗；
见[配置](configuration.md#watchdog)。

## 事件一直不触发

- 订阅被拒绝了：`hooks.disabled` 里列出的合成事件，在模组订阅时会在日志里说明。
- 载荷会说明它为什么不完整：`_unresolved`、`partial` 和 `targetKnown` 见[事件载荷](event-payloads.md)。
- 在一个引擎版本上触发、另一个上不触发，通常是它 hook 的引擎函数变了。带上启动行里的 Pier 和 BDS 版本报告。

## 设置没生效

Pier 不读的键会被静默丢掉；类型不对的值会被忽略，并记一行写明键名和它要的类型，比如
`'watchdog.hang_ms' wants number, and the value written there is ignored`。`config.json` 只写一次、
之后不再回写，所以后续版本新增的键不会出现在旧文件里，用的是默认值。对照[配置](configuration.md)里的文件检查。

## 求助

带上 Pier 和 BDS 版本（日志开头几行就有）、从启动到出问题的完整日志，以及模组列表。拒绝加载、看门狗动手、配置项被忽略，日志里都有一行写着原因和名字；没有日志，这一行就只能请你再去找一次。
