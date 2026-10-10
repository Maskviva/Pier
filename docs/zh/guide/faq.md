# 常见问题

## 能用哪些语言写模组？

任何能从 DLL 导出 C 函数的语言。Rust 和 C++ 是官方绑定；新绑定要做的事情，[加一门语言](adding-a-language.md)里列全了。

## Pier 更新后要重新编译模组吗？

只有 ABI 版本变化时才要。保持 `PIER_ABI_VERSION` 不变的版本，能原样加载同一 ABI 版本下编译的所有模组；
这样的版本里，任何绑定都不会从公开接口删东西，要淘汰的先标为弃用。
每个版本的 ABI 版本都写在 [CHANGELOG](https://github.com/Maskviva/pier/blob/main/CHANGELOG.md) 里。

## 死亡事件里怎么拿到凶手？

从 `MobDieEvent` 或 `PlayerDieEvent` 的伤害来源里拿：`attackerUid` 是负责的实体（投射物伤害时是射手），
`attacker` 在它还存在时描述它。详见[事件载荷](event-payloads.md#谁杀了谁)。

## Pier 能停掉卡死服务器的模组吗？

能点名并重启服务器，但不能只停掉那一个模组。模组跑在服务器自己的线程上，没有办法把线程从持有锁的代码里拽出来而不让那些锁一直被占着。
[看门狗](configuration.md#watchdog)会记下是哪个模组，然后结束进程，由守护程序重启。

## Pier 能在客户端用吗？

Pier 有客户端构建，能加载客户端目标的模组。只在服务端有的能力在那里是 NULL 槽位，
每个绑定都会报告为"宿主不提供"，而不是静默失败。[bridge](bridge.md) 只在服务端。

## 槽位明明存在，调用却返回"不提供"？

槽位可以在表里但为空：对应的能力包没编进宿主，或者宿主在这个引擎版本上没有实现它。
绑定把这两种都报告为"不提供"，这和"调用执行了但失败"是两回事。

## LeviLamina 的 C++ 插件能调用 Pier 模组吗？

通过 [bridge](bridge.md)：一个头文件，让原生插件不用链接 Pier 就能按名字调用 Pier 模组注册的服务。
