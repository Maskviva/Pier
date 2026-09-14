# 从不是 Pier mod 的 mod 里调 Pier mod

每个 Pier mod 都可以注册服务：一个名字，一段 UTF-8 请求，一段 UTF-8 回复。管着服务器
经济的那个 mod 答余额查询，管着领地的那个答「这个方块能不能挖」。Pier 内部的 mod 用
`service_call` 去够它们。

原生 LeviLamina mod 够不着。`service_call` 收的是 Pier 装载器发的 `PierModHandle`，
而 Pier 只导出一个 `pier_main`，方向是反的。于是一个「整个价值就在于当大家共用的后端」的
mod，只有已经用了 Pier 的那一半生态能调它。

`bindings/bridge/pier-bridge.h` 就是那扇门。**纯头文件，零依赖，不需要链任何东西。**

```cpp
#include "pier-bridge.h"

auto pier = pier::bridge::Client::open();
if (!pier) {
    logger.warn("Pier 没装；退回只认 OP");
    return;
}

auto reply = pier->call("example:economy.balance",
    R"({"player":"2535470000000000"})");

switch (reply.status) {
    case pier::bridge::Status::Ok:        useTheAnswer(reply.body); break;
    case pier::bridge::Status::NotFound:  /* 没人提供它，这是正常部署 */ break;
    default:                              logger.warn(reply.body); break;
}
```

`pier->services()` 列出所有已注册的服务，形如 `[{"name":…,"mod":…}]`。在装载时问一次，
日志上比「某个人正需要答案的那一刻冒出一个 NotFound」好读。

完整的可运行例子在 `examples/hello-bridge/`：一个原生 LeviLamina mod，一条命令，
把 `services()` 的结果打给你看。

## 它不是什么

不是 Pier 的 mod ABI。没有事件、没有钩子、没有表单、没有快车道、没有维度。要这些的 mod
就是 Pier mod，该 include 的是 `sdk/abi.h`。这扇门是给「已经有自己的装载器、只想问一个
问题」的那种 mod 的。

**方向也是单向的。** 你能调进 Pier mod 注册的服务；反过来，Pier mod 调不到你——
`pier_bridge_call` 只有请求进、回复出这一条路，没有让外面的 mod 注册服务的那一半。

## 调用方是匿名的

Pier mod 的调用带着句柄，所以提供方能知道是谁在问。bridge 调用方没有，而且永远不会有：
它是另一个装载器上的另一个 dll，手里没有任何 Pier 发出来的东西。

这是一条真的限制，不是形式问题。**一个仅凭调用方身份就授予什么的提供方，不该从这扇门
够得着。** 提供方需要知道的东西一律写进请求里：一个有「会改动什么」的端点的提供方，
应该在请求体里收下发起人的身份，而不是从连接去推断权力。

别把它读成「bridge 是后门」。它够到的是同一个注册表，同一把锁，跨进提供方 dylib 之前
做的是同一次重新校验。它缺的只是一个能写进日志行的名字。

## 版本

`pier_bridge_abi()` 返回那三个符号的 ABI。`Client::open()` 先调它，对不上就返回空，
而不是继续试——一个在调用方脚下变过的签名，是唯一一种会崩而不是返回错误的失败。

**加服务永远不会抬这个号。** 服务是跑在这套 ABI 之上的数据，不是它的一部分，所以按
bridge ABI 1 编出来的 mod，在管理器加端点之后照常能用。

## 线程

在你自己的线程上同步调用，没有超时，和 Pier mod 的调用一模一样。提供方卡住，你就跟着卡。
大多数服务预期在服务器线程上被调；某个服务能不能从工作线程调，是那个服务自己的契约。

## 装载顺序

`Client::open()` 要求 Pier **已经在进程里**。它用 `GetModuleHandleEx` 只绑已经装进来的
那一份，绝不会把第二份 Pier 拉进来：第二份会是一个空的注册表，而不是正在跑的那个。

所以在自己的 manifest 里把 Pier 写成依赖：

```json
{
  "name": "your-mod",
  "entry": "your_mod.dll",
  "type": "native",
  "dependencies": [ { "name": "pier" } ]
}
```

然后在 `enable()` 里 `open()`，不要在 `load()` 里。取到的 `Client` 会**持有一份对
Pier.dll 的引用**，在它析构之前那些函数指针不会失效。这不会让 Pier 那个 mod 活着：
Pier 在 disable 时注销自己的服务，之后的调用答 NotFound——那是调用方处理得了的降级。
