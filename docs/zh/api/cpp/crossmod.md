# 跨模组：总线、服务与快速通道

??? note "abi.h 里的分节说明"

    **跨模组事件总线（`Cross-mod event bus`）**

    加载器为什么自己持有订阅表、不让模组之间交换指针，见上面的 `PierBusCb`。这四个槽位都是线程安全的；回调在发布方的线程上运行。

    模组收不到自己发布的消息。原因有两个：想通知自己的模组可以直接调用自己的函数；而自己投递给自己，是唯一一种任何深度上限都分不清它和正常工作的循环。跨模组的循环（A 发布 → B 的处理函数发布 → A 的处理函数发布 → ……）由深度上限截住：碰到上限时，最内层的那次发布被丢弃，并记一次日志。

    **跨模组服务注册表（查询式调用）（`Cross-mod service registry (query-style calls)`）**

    总线是单向广播；服务是请求和响应。两者在每个方面的形状都不同，所以分成两张表，没有合成一张：

    - 每个名字的提供方：总线任意多个；服务恰好一个
    - 没人注册时：总线一切正常；服务是调用方要处理的错误
    - 返回值：总线没有；服务的全部意义就在返回值
    - 顺序：总线没有定义，也不能依赖；服务不涉及

    注册是**独占**的。两个模组都回答 `plot:can`，得到的是一个有歧义的答案，调用方没办法选，所以第二个注册的会被明确拒绝。如果悄悄地以最后一个为准，答案就取决于模组的加载顺序，而加载顺序没有人控制，装一个不相干的模组都可能改变它。

    归属的做法和总线、表单一样，用 `weak_ptr` 加票据：加载器持有表，调用路径在进入提供方的 dylib 之前，立刻重新确认提供方还在。

    同步执行，在调用方的线程上，没有超时。提供方阻塞时，和其他任何回调一样阻塞服务器线程；如果回调还在运行就返回「超时」，调用方拿到的是一个错误的答案，提供方也照样还在运行。

    **同工具链快速通道，追加的槽位，受 `struct_size` 约束（`Same-toolchain fast lane, appended and struct_size-gated.`）**

    追加了五个槽位，没有改动 `PIER_ABI_VERSION`：单纯的追加不算版本变化，`struct_size` 才是准确的关卡。

    两个方向都成立。新加载器运行旧模组：旧表是新表逐字节相同的前缀，模组够不着这五个槽位，照常工作。新模组配旧加载器：SDK 运行时初始化时比较 `struct_size`，发现加载器的表比它编译时用的短，于是拒绝加载。这是正确的结果，因为在没有 `lane_publish` 的加载器上读那个单元会越界。

    简单地说：版本号记录「语义变了」，`struct_size` 记录「表变长了」。这次改动只属于后者。

    见上面 `PierLaneDesc` 处的长注释。一句话概括：服务是跨语言的 (名字, JSON) -> JSON 通道；快速通道直接调用函数表，只在两边由同一个工具链构建时成立；指纹对不上时一个指针都不交出，使用方退回到服务通道。

    只能在服务器线程调用。

    **追加：批量读写方块（`Appended: bulk block reads and writes`）**

## 槽位 {#slots}

### `bus_subscribe` {#bus_subscribe}

```c
uint64_t (*bus_subscribe)(PierModHandle mod, PierStr topic, PierBusCb cb, void* user);
```

让 `mod` 订阅 `topic`。返回订阅 id（大于 0）；主题为空或过长、回调为空，或者模组不认识时返回 0。模组卸载时，订阅会自动删除。

- 调用形式：`api->bus_subscribe(mod, topic, cb, user)`
- 参数：
    - mod : `PierModHandle`
    - topic : `PierStr`
    - cb : `PierBusCb`
    - user : `void*`
- 返回值类型：`uint64_t`
- 所在分节：跨模组事件总线（`Cross-mod event bus`）
- 表内序号：第 157 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`bus::subscribe`](../rust/bus.md#fn.subscribe)
    - Go：[`BusSubscribe`](../go/crossmod.md#BusSubscribe)
    - Zig：[`busSubscribe`](../zig/crossmod.md#busSubscribe)

### `bus_unsubscribe` {#bus_unsubscribe}

```c
bool (*bus_unsubscribe)(PierModHandle mod, uint64_t sub_id);
```

删除这个模组的一个订阅。只作用于调用方自己，一个模组不能取消另一个模组的订阅。真的删除了一个订阅时返回 true。在回调内部（包括自己的回调里）调用也是安全的。

- 调用形式：`api->bus_unsubscribe(mod, sub_id)`
- 参数：
    - mod : `PierModHandle`
    - sub_id : `uint64_t`
- 返回值类型：`bool`
- 所在分节：跨模组事件总线（`Cross-mod event bus`）
- 表内序号：第 158 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Go：[`Subscription.Unsubscribe`](../go/crossmod.md#Subscription.Unsubscribe)、[`Raw.BusUnsubscribe`](../go/raw.md#Raw.BusUnsubscribe)
    - Zig：[`Subscription.unsubscribe`](../zig/crossmod.md#Subscription.unsubscribe)

### `bus_publish` {#bus_publish}

```c
uint32_t (*bus_publish)(PierModHandle mod, PierStr topic, PierStr payload);
```

把 `payload` 投递给订阅了 `topic` 的每一个**其他**模组。返回实际运行了多少个订阅者（0 是正常的，表示没有人在听）。订阅者的返回值被忽略。

- 调用形式：`api->bus_publish(mod, topic, payload)`
- 参数：
    - mod : `PierModHandle`
    - topic : `PierStr`
    - payload : `PierStr`
- 返回值类型：`uint32_t`
- 所在分节：跨模组事件总线（`Cross-mod event bus`）
- 表内序号：第 159 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`bus::publish`](../rust/bus.md#fn.publish)
    - Go：[`BusPublish`](../go/crossmod.md#BusPublish)、[`Raw.BusPublish`](../go/raw.md#Raw.BusPublish)
    - Zig：[`busPublish`](../zig/crossmod.md#busPublish)

### `bus_publish_vetoable` {#bus_publish_vetoable}

```c
bool (*bus_publish_vetoable)(
    PierModHandle mod, PierStr topic, PierStr payload, uint32_t* out_delivered);
```

同上，但会收集否决位：任何一个订阅者返回 true，就返回 true。每个订阅者照样都会运行，不会提前结束，所以无论前面有没有订阅者拒绝，观察者看到的都是一样完整的消息流。`out_delivered` 可以为 NULL。

- 调用形式：`api->bus_publish_vetoable(mod, topic, payload, out_delivered)`
- 参数：
    - mod : `PierModHandle`
    - topic : `PierStr`
    - payload : `PierStr`
    - out_delivered : `uint32_t*`
- 返回值类型：`bool`
- 所在分节：跨模组事件总线（`Cross-mod event bus`）
- 表内序号：第 160 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`bus::publish_vetoable`](../rust/bus.md#fn.publish_vetoable)
    - Go：[`BusPublishVetoable`](../go/crossmod.md#BusPublishVetoable)、[`Raw.BusPublishVetoable`](../go/raw.md#Raw.BusPublishVetoable)
    - Zig：[`busPublishVetoable`](../zig/crossmod.md#busPublishVetoable)

### `bus_subscriber_count` {#bus_subscriber_count}

```c
uint32_t (*bus_subscriber_count)(PierStr topic);
```

一个主题当前有多少订阅者，所有模组加在一起。用来在没人会读的时候，省掉构造载荷的开销。

- 调用形式：`api->bus_subscriber_count(topic)`
- 参数：
    - topic : `PierStr`
- 返回值类型：`uint32_t`
- 所在分节：跨模组事件总线（`Cross-mod event bus`）
- 表内序号：第 161 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`bus::subscriber_count`](../rust/bus.md#fn.subscriber_count)、[`bus::subscriber_count`](../rust/bus.md#fn.subscriber_count)
    - Go：[`BusSubscriberCount`](../go/crossmod.md#BusSubscriberCount)、[`Raw.BusSubscriberCount`](../go/raw.md#Raw.BusSubscriberCount)
    - Zig：[`busSubscriberCount`](../zig/crossmod.md#busSubscriberCount)

### `service_register` {#service_register}

```c
uint64_t (*service_register)(
    PierModHandle mod, PierStr name, PierServiceCb cb, void* user);
```

把 `mod` 注册为 `name` 的提供方。返回注册 id（大于 0）；名字为空、过长或者已被占用，回调为空，或者模组不认识时返回 0。卸载时自动删除。

- 调用形式：`api->service_register(mod, name, cb, user)`
- 参数：
    - mod : `PierModHandle`
    - name : `PierStr`
    - cb : `PierServiceCb`
    - user : `void*`
- 返回值类型：`uint64_t`
- 所在分节：跨模组服务注册表（查询式调用）（`Cross-mod service registry (query-style calls)`）
- 表内序号：第 163 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`service::register`](../rust/service.md#fn.register)、[`service::register_json`](../rust/service.md#fn.register_json)
    - Go：[`RegisterService`](../go/crossmod.md#RegisterService)
    - Zig：[`registerService`](../zig/crossmod.md#registerService)

### `service_unregister` {#service_unregister}

```c
bool (*service_unregister)(PierModHandle mod, uint64_t reg_id);
```

删除这个模组的一项注册。只作用于调用方自己，一个模组不能注销另一个模组的服务。

- 调用形式：`api->service_unregister(mod, reg_id)`
- 参数：
    - mod : `PierModHandle`
    - reg_id : `uint64_t`
- 返回值类型：`bool`
- 所在分节：跨模组服务注册表（查询式调用）（`Cross-mod service registry (query-style calls)`）
- 表内序号：第 164 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Go：[`ServiceRegistration.Unregister`](../go/crossmod.md#ServiceRegistration.Unregister)、[`Raw.ServiceUnregister`](../go/raw.md#Raw.ServiceUnregister)
    - Zig：[`ServiceRegistration.unregister`](../zig/crossmod.md#ServiceRegistration.unregister)

### `service_call` {#service_call}

```c
int32_t (*service_call)(
    PierModHandle mod, PierStr name, PierStr request, void* ctx, PierStrSink reply);
```

用 `request` 调用名为 `name` 的服务，提供方的回答通过 `reply` 送回。返回某个 `PIER_SERVICE_*`。模组不能调用自己的服务：它可以直接调用自己的函数，而自己调用自己是最难看清的一种循环。

- 调用形式：`api->service_call(mod, name, request, ctx, reply)`
- 参数：
    - mod : `PierModHandle`
    - name : `PierStr`
    - request : `PierStr`
    - ctx : `void*`
    - reply : `PierStrSink`
- 返回值类型：`int32_t`
- 所在分节：跨模组服务注册表（查询式调用）（`Cross-mod service registry (query-style calls)`）
- 表内序号：第 165 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`service::call`](../rust/service.md#fn.call)、[`service::call_json`](../rust/service.md#fn.call_json)、[`service::call_with`](../rust/service.md#fn.call_with)、[`service::call_optional`](../rust/service.md#fn.call_optional)
    - Go：[`CallService`](../go/crossmod.md#CallService)、[`CallServiceOptional`](../go/crossmod.md#CallServiceOptional)、[`Raw.ServiceCall`](../go/raw.md#Raw.ServiceCall)
    - Zig：[`callService`](../zig/crossmod.md#callService)

### `service_list` {#service_list}

```c
void (*service_list)(void* ctx, PierStrSink sink);
```

所有已注册的服务，以 JSON 数组给出，每一项是 `{"name":…,"mod":…}`。用于诊断，也方便调用方在构造请求之前先确认有没有人能回答。

- 调用形式：`api->service_list(ctx, sink)`
- 参数：
    - ctx : `void*`
    - sink : `PierStrSink`
- 所在分节：跨模组服务注册表（查询式调用）（`Cross-mod service registry (query-style calls)`）
- 表内序号：第 166 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`service::exists`](../rust/service.md#fn.exists)、[`service::list_json`](../rust/service.md#fn.list_json)、[`service::list`](../rust/service.md#fn.list)
    - Go：[`ListServices`](../go/crossmod.md#ListServices)、[`ServiceExists`](../go/crossmod.md#ServiceExists)、[`Raw.ServiceList`](../go/raw.md#Raw.ServiceList)
    - Zig：[`listServicesJson`](../zig/crossmod.md#listServicesJson)

### `lane_publish` {#lane_publish}

```c
uint64_t (*lane_publish)(PierModHandle mod, PierStr name, PierLaneDesc const* desc);
```

发布一个快速通道。和 `service_register` 一样是独占的：名字已被占用时返回 0，并在日志里写出现在占着这个名字的模组。返回发布 id（大于 0），卸载时自动撤回。

- 调用形式：`api->lane_publish(mod, name, desc)`
- 参数：
    - mod : `PierModHandle`
    - name : `PierStr`
    - desc : `PierLaneDesc const*`
- 返回值类型：`uint64_t`
- 所在分节：同工具链快速通道，追加的槽位，受 `struct_size` 约束（`Same-toolchain fast lane, appended and struct_size-gated.`）
- 表内序号：第 172 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`lane::publish`](../rust/lane.md#fn.publish)

### `lane_unpublish` {#lane_unpublish}

```c
bool (*lane_unpublish)(PierModHandle mod, uint64_t pub_id);
```

撤回这个模组拥有的一个快速通道。对每一个还没归还的租约调用 `release`，并清除存活标记，这样使用方下一次检查就会发现通道已经没了，不会再通过一个失效的指针跳转。

- 调用形式：`api->lane_unpublish(mod, pub_id)`
- 参数：
    - mod : `PierModHandle`
    - pub_id : `uint64_t`
- 返回值类型：`bool`
- 所在分节：同工具链快速通道，追加的槽位，受 `struct_size` 约束（`Same-toolchain fast lane, appended and struct_size-gated.`）
- 表内序号：第 173 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Go：[`Raw.LaneUnpublish`](../go/raw.md#Raw.LaneUnpublish)

### `lane_acquire` {#lane_acquire}

```c
int32_t (*lane_acquire)(
    PierModHandle mod, PierStr name, uint64_t want_fingerprint, PierLaneRef* out);
```

获取一个快速通道。`want_fingerprint` 必须是使用方自己算出来的值。

0 不是有效的指纹，总是得到 `PIER_LANE_FINGERPRINT`。它不能表示「跳过检查」：这个调用交出的是原始的函数表指针和数据指针，使用方随后按自己的表偏移去调用，跳过检查就是类型混淆。要查看有哪些通道、它们的指纹是什么，请用 `lane_list`，它不交出任何指针。

返回一个 `PIER_LANE_*` 值。获取成功以后必须调用 `lane_release`，否则提供方的状态会一直被保留。

- 调用形式：`api->lane_acquire(mod, name, want_fingerprint, out)`
- 参数：
    - mod : `PierModHandle`
    - name : `PierStr`
    - want_fingerprint : `uint64_t`
    - out : `PierLaneRef*`
- 返回值类型：`int32_t`
- 所在分节：同工具链快速通道，追加的槽位，受 `struct_size` 约束（`Same-toolchain fast lane, appended and struct_size-gated.`）
- 表内序号：第 174 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`lane::acquire`](../rust/lane.md#fn.acquire)、[`lane::acquire`](../rust/lane.md#fn.acquire)

### `lane_release` {#lane_release}

```c
bool (*lane_release)(PierModHandle mod, uint64_t lease);
```

归还一个租约，只能是这个模组持有的租约。提供方已经不在时返回 false：那时加载器已经替它调用过 `release`，再调用一次就是重复释放。

- 调用形式：`api->lane_release(mod, lease)`
- 参数：
    - mod : `PierModHandle`
    - lease : `uint64_t`
- 返回值类型：`bool`
- 所在分节：同工具链快速通道，追加的槽位，受 `struct_size` 约束（`Same-toolchain fast lane, appended and struct_size-gated.`）
- 表内序号：第 175 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Go：[`Raw.LaneRelease`](../go/raw.md#Raw.LaneRelease)

### `lane_list` {#lane_list}

```c
void (*lane_list)(void* ctx, PierStrSink sink);
```

所有快速通道，以 JSON 数组给出：`[{"name":…,"mod":…,"fingerprint":"0x…","protocol":1,"leases":N,"alive":true}]`

- 调用形式：`api->lane_list(ctx, sink)`
- 参数：
    - ctx : `void*`
    - sink : `PierStrSink`
- 所在分节：同工具链快速通道，追加的槽位，受 `struct_size` 约束（`Same-toolchain fast lane, appended and struct_size-gated.`）
- 表内序号：第 176 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`lane::list`](../rust/lane.md#fn.list)、[`lane::list`](../rust/lane.md#fn.list)、[`lane::list_json`](../rust/lane.md#fn.list_json)、[`lane::list_json`](../rust/lane.md#fn.list_json)
    - Go：[`Raw.LaneList`](../go/raw.md#Raw.LaneList)

### `service_caller` {#service_caller}

```c
void (*service_caller)(void* ctx, PierStrSink sink);
```

正在运行的这个服务回调，是谁调用的。

服务回调只收到请求，不知道发送方是谁，所以按请求里的某个名字（所有者、执行操作的玩家）做判断的提供方，等于相信请求说的是实话。这个槽位让提供方可以改为去问宿主：在 `PierServiceCb` 里，输出回调收到调用栈上那次 `service_call` 所属模组的清单名。调用会嵌套，报告的是最内层的那一次。

在回调之外，或者调用没有带模组句柄时，输出回调一次都不会被调用；提供方由此知道它无法确定请求来自谁，不会把请求算到一个空名字头上。它读的是线程局部变量，所以可以在任何线程调用；回调换了线程就会丢掉归属，在那种情况下这正是正确的回答。

- 调用形式：`api->service_caller(ctx, sink)`
- 参数：
    - ctx : `void*`
    - sink : `PierStrSink`
- 所在分节：追加：批量读写方块（`Appended: bulk block reads and writes`）
- 表内序号：第 197 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`service::caller`](../rust/service.md#fn.caller)
    - Go：[`ServiceCaller`](../go/crossmod.md#ServiceCaller)、[`Raw.ServiceCaller`](../go/raw.md#Raw.ServiceCaller)
    - Zig：[`serviceCaller`](../zig/crossmod.md#serviceCaller)

## 宏 {#macros}

### `PIER_LANE_PROTOCOL` {#PIER_LANE_PROTOCOL}

```c
#define PIER_LANE_PROTOCOL 1u
```

快速通道的协议版本。和 `PIER_ABI_VERSION` 分开：通道的形状独立演进，不匹配时的处理也不一样（只拒绝这一条通道，不拒绝整个模组）。

## 类型 {#types}

### `PierBusCb` {#PierBusCb}

```c
typedef bool (*PierBusCb)(void* user, PierStr topic, PierStr payload);
```

订阅者回调。`topic` 和 `payload` 在调用期间是借来的，调用之后还要用的东西必须复制一份。

返回值是否决，只对 `bus_publish_vetoable` 有效：

- true = 「拒绝这件事」
- false = 「没有意见」

`bus_publish` 完全忽略返回值。有意没有提供把拒绝变回同意的办法：订阅者只能收紧，永远不能放松。如果允许一个模组推翻另一个模组的拒绝，那就是**最后**运行的订阅者说了算，而订阅者的顺序，两个模组都控制不了。

在发布消息的那个线程上调用。所属模组卸载以后，或者被禁用期间，永远不会被调用。

### `PierServiceCb` {#PierServiceCb}

```c
typedef bool (*PierServiceCb)(
    void* user, PierStr name, PierStr request, void* ctx, PierStrSink reply);
```

跨模组服务注册表的提供方回调（查询式调用，和总线的单向广播不同）。

通过 `reply(ctx, ...)` 写入回答，正好一次，然后返回 true。返回 false 表示失败；在此之前写入的内容会作为错误文本交给调用方，所以调用处能分清「没有这块地皮」和「数据库挂了」。

`request` 和 `reply` 是两个模组在别处约定好的不透明 UTF-8。加载器从不查看里面的内容。

在**调用方**的线程上、在 `service_call` 内部同步运行。提供方模组卸载以后永远不会被调用。提供方只是被禁用时**仍然**会被调用：`service_call` 有意不检查 `isEnabled()`（见 Services.cpp），因为 LeviLamina 要等所有的 `on_load` 都运行完才启用模组，服务如果在这段时间里够不着，每一个在自己的 `on_load` 里解析它的使用方都会出问题。

### `PierLaneRefFn` {#PierLaneRefFn}

```c
typedef void (*PierLaneRefFn)(void* data);
```

引用计数的钩子，在提供方自己的 dylib 里执行。

加载器在 `lane_acquire` 和 `lane_release` 里调用它们；提供方卸载或者调用 `lane_unpublish` 时，加载器对每一个还没归还的租约调用 `release`。发布本身不占一个计数：加载器从不为交给 `lane_publish` 的数据调用 `release`，提供方在撤回之后自己回收它。

它们不能回头调用加载器（任何 `lane_*` 槽位），那样会自己锁死自己。典型的实现是对提供方自己的引用计数做一次原子加或减，不碰任何锁。

### `PierLaneDesc` {#PierLaneDesc}

```c
typedef struct PierLaneDesc
{
    /** sizeof(PierLaneDesc); same discipline as PierApi::struct_size. */
    uint32_t struct_size;
    /** Must equal PIER_LANE_PROTOCOL or publish is refused. */
    uint32_t protocol;
    /** Build fingerprint. 0 is reserved (it would mean "anyone may connect") and
     *  must not be used. */
    uint64_t fingerprint;
    /** The provider's state pointer, typically a raw pointer handed out by a
     *  reference-counted object. The loader does not interpret it. */
    void* data;
    /** C-layout function table. The loader neither interprets nor copies it; the
     *  provider must keep it alive until the lane is withdrawn (static storage,
     *  or an allocation deliberately never reclaimed). */
    void const* vtable;
    /** May be NULL, in which case leases are not counted and the alive flag is the
     *  only guard. */
    PierLaneRefFn retain;
    PierLaneRefFn release;
} PierLaneDesc;
```

提供方发布快速通道时用来描述它的结构。每个字段都由提供方填写，加载器只负责传递。

### `PierLaneRef` {#PierLaneRef}

```c
typedef struct PierLaneRef
{
    /** sizeof(PierLaneRef), filled in by the caller before the call; the loader
     *  uses it to decide which trailing fields to write. The direction is the
     *  reverse of elsewhere because here the loader writes the caller's
     *  struct. */
    uint32_t struct_size;
    /** Used when returning the lease. 0 means nothing was acquired. */
    uint64_t lease;
    /** The provider's fingerprint. Filled in even on mismatch, for diagnostics: an
     *  operator needs to read "these two mods were built by different
     *  compilers", not the word "mismatch". */
    uint64_t fingerprint;
    void* data;
    void const* vtable;
    /**
     * Liveness flag owned by the loader; non-zero means the provider is still
     * there. The cell is never freed, so reading it after the provider unloads
     * is still legal, which is the entire reason it exists. Read it with acquire
     * before each call (the writer uses a release store; a relaxed load paired
     * with a release store does not synchronize-with).
     *
     * NULL when the fingerprint did not match.
     */
    uint32_t const* alive;
    /**
     * In-call counter, also owned by the loader and never freed. The consumer
     * increments it before entering a provider entry point and decrements after
     * returning.
     *
     * alive only rules out "the provider was already gone before the call"; it
     * does not close the window between the check and the call. Server-thread-
     * only calling rules out concurrent unload but not reentrant unload: a
     * provider entry point dispatches a command, that command unloads the
     * provider, and FreeLibrary happens underneath a stack frame still sitting
     * in provider code.
     *
     * The loader reads this counter first thing in unload and refuses to unload
     * with a reason when it is non-zero, rather than unloading and crashing.
     *
     * Appended field, guarded by struct_size: an older consumer's struct_size
     * does not reach here, the loader does not write it, and its behavior is
     * unchanged.
     */
    uint32_t* busy;
} PierLaneRef;
```

`lane_acquire` 产出的结果。

## `PIER_SERVICE_*` {#PIER_SERVICE_OK-group}

`service_call` 的返回码。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PIER_SERVICE_OK"></span>`PIER_SERVICE_OK` | `0` | 提供方运行了，并写了回答 |
| <span id="PIER_SERVICE_NOT_FOUND"></span>`PIER_SERVICE_NOT_FOUND` | `1` | 没有模组提供这个名字（或者提供方已被禁用、已卸载） |
| <span id="PIER_SERVICE_ERROR"></span>`PIER_SERVICE_ERROR` | `2` | 提供方返回了 false；回答里是它的错误信息 |
| <span id="PIER_SERVICE_REFUSED"></span>`PIER_SERVICE_REFUSED` | `3` | 名字不合法、调用了自己，或者超过了调用深度上限 |

## `PIER_LANE_*` {#PIER_LANE_OK-group}

`lane_acquire` 的返回值。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PIER_LANE_OK"></span>`PIER_LANE_OK` | `0` | 获取成功；`out` 已经填好 |
| <span id="PIER_LANE_NOT_FOUND"></span>`PIER_LANE_NOT_FOUND` | `1` | 没有模组发布这个名字（那个模组没有安装） |
| <span id="PIER_LANE_FINGERPRINT"></span>`PIER_LANE_FINGERPRINT` | `2` | 已经发布，但指纹不同；降级处理，什么都不交出 |
| <span id="PIER_LANE_REFUSED"></span>`PIER_LANE_REFUSED` | `3` | 名字不合法、获取了自己的通道、提供方被禁用，或者协议不匹配 |
