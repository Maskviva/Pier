/**
 * Pier ABI — `sdk/abi.h`（ABI v1）
 *
 * 这份头文件是 Pier 的**产品**：C++ 宿主（pier-host + 各能力包）与任何语言的
 * SDK 之间唯一的契约。参考镜像是 `packages/pier-sys-rs/src/api.rs`（全手写、
 * 无 bindgen，同时充当本文件的可读注解）。
 *
 * # 这份文件必须能被 C 编译器解析
 *
 * 契约的消费方是「任何语言」，所以它只用 C11：没有 std::string_view、没有
 * enum class、没有嵌套类型。C++ 侧的便利封装（PierStr ↔ string_view 等）在
 * `pier-support`，**不在这里** —— 契约里一旦出现某门语言的类型，别的语言就
 * 得去猜那个类型的布局。CI 会分别用 C11 和 C++20 各编译本文件一遍。
 *
 * # 改这个文件的规矩（全文只有这一套版本规则）
 *
 * 1. **只在 `PierApi` 结构体末尾追加**。永远不重排、不删除、不改已有槽位的
 *    签名。追加**不**升 `PIER_ABI_VERSION`。
 * 2. 追加后同步每个 SDK 的镜像（字段顺序逐格对齐；`sys-mirrors-abi` 机检）。
 * 3. 只有**非追加**变更（重排 / 删除 / 改签名）才把 `PIER_ABI_VERSION` 和
 *    `PIER_ABI_MIN_SUPPORTED` **同时**推进到同一个数。
 *
 * 为什么追加不升版本：版本号回答的是「哪些已编译模组还能装」。追加一个槽位
 * 不会让任何旧模组失效 —— 旧表是新表逐字节相同的前缀，旧模组永远够不到新
 * 槽。为它升版本等于宣布一次不存在的不兼容。两个方向的安全各有专门的闸：
 *
 *   - 新宿主 + 旧模组：版本区间检查（见 `PIER_ABI_MIN_SUPPORTED`）
 *   - 旧宿主 + 新模组：模组侧**逐槽**比对 `struct_size`（SDK 的
 *     `require_slot!`），表不够长就把那一个调用报成「宿主没有此功能」
 *
 * # 布局在所有构建目标下都相同（v1 的核心决定）
 *
 * `PierApi` 没有任何条件编译：客户端专属、维度扩展等**能力组**的槽位永远存
 * 在于布局中，只是当那个能力包没有编进宿主时，对应槽位为 **NULL**。
 * 「有没有这个能力」= 「这个槽是不是 NULL」，SDK 据此报「宿主不提供 X」。
 * 这换来三件事：镜像不需要任何条件编译；跨目标错配不可能造成槽位错位调用；
 * 结构体只有一个追加点 —— 末尾。
 *
 * # 约定（全文件适用，逐槽注释只写例外）
 *
 *   - 字符串一律是 UTF-8 的 (ptr, len) 视图，**不保证 NUL 结尾**。
 *   - 传进回调的字符串归调用方所有，只在回调期间有效；要留就拷贝。
 *   - 模组向外传字符串一律走 sink 回调，在当前调用帧内完成 —— 跨边界永远
 *     不移交所有权，ABI 上不存在「返回一个需要对方释放的指针」。
 *   - 线程：除非该槽注释另有说明，一律只能在**服务器线程**调用。
 *     `log` / `gaming_status` / `schedule` / `schedule_after` 线程安全。
 *     所有回调（事件、命令、计划任务）都在服务器线程触发。
 */
#ifndef PIER_SDK_ABI_H
#define PIER_SDK_ABI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 见文件头「改这个文件的规矩」。追加槽位**不**改这里。 */
#define PIER_ABI_VERSION 1u

/** 宿主可接受的最老模组 ABI。只随非追加变更移动，且与 PIER_ABI_VERSION
 *  一起移到同一个数 —— 它是「低于此的表不再是我的前缀」的开关。 */
#define PIER_ABI_MIN_SUPPORTED 1u

/** 模组必须导出的唯一入口符号。宿主只找这一个名字，找不到就明确拒绝装载，
 *  不做任何回退 —— v1 是全新起点，不承载历史别名。 */
#define PIER_MAIN_SYMBOL "pier_main"

/** `PierApi.host_flags` / `PierModVTable.mod_flags` 的位。
 *  两侧的 bit 0 必须相等，否则宿主拒绝装载并说明原因（服务端宿主装不了按
 *  客户端编译的模组，反之亦然）。其余位保留，当前必须为 0。 */
#define PIER_FLAG_CLIENT 0x1u

/**
 * UTF-8 字符串视图。**显式的 {指针, 长度} 结构体**，不是任何语言字符串类型
 * 的别名 —— 布局由这份声明本身定义，不依赖任何一侧标准库的实现细节。
 * C++ 侧与 std::string_view 的零拷贝互转在 pier-support。
 */
typedef struct PierStr
{
    const char* ptr;
    size_t len;
} PierStr;

/** Opaque handle to the HostedMod instance managed by the loader. */
typedef void* PierModHandle;
/** Opaque handle to an event listener. */
typedef void* PierListenerHandle;

/** Generic "run this" callback. */
typedef void (*PierTaskCb)(void* user);

/** Generic string sink: receives a string within the current call frame. */
typedef void (*PierStrSink)(void* ctx, PierStr s);

/**
 * Event callback.
 *   event_id : the full event id this listener fired for.
 *   snbt     : event data serialized as SNBT (CompoundTag). For cancellable
 *              events it contains a `cancelled` byte field.
 *   write_ctx / write_back : to mutate the event (e.g. cancel it, edit the
 *              chat message), call write_back(write_ctx, new_snbt) with the
 *              modified SNBT before returning. The loader deserializes it
 *              back into the event. Calling it zero times leaves the event
 *              untouched; the last call wins.
 */
typedef void (*PierEventCb)(
    void* user,
    PierStr event_id,
    PierStr snbt,
    void* write_ctx,
    PierStrSink write_back
);

/**
 * Custom command callback.
 *   args        : raw text following the command name (may be empty).
 *   origin_name : display name of the command origin (player name / "Server").
 *   out_success / out_error : call any number of times to emit output lines.
 */
typedef void (*PierCommandCb)(
    void* user,
    PierStr args,
    PierStr origin_name,
    void* out_ctx,
    PierStrSink out_success,
    PierStrSink out_error
);

/** Output sink for execute_command: full command output + success flag. */
typedef void (*PierCmdOutputSink)(void* ctx, bool success, PierStr output);

/* ─────────────────── 世界读取（scan） ─────────────────── */

/** A player's feet position + dimension. `found` is false if no such player. */
typedef struct PierPlayerPos
{
    double x;
    double y;
    double z;
    int32_t dimension;
    bool found;
} PierPlayerPos;

/**
 * Block sink: invoked once per cell during scan_region.
 *   x, y, z : the cell's world coordinates.
 *   name    : block type name, e.g. "minecraft:redstone_wire".
 *   snbt    : full block serialization (name + states + version) as SNBT.
 */
typedef void (*PierBlockSink)(void* ctx, int32_t x, int32_t y, int32_t z, PierStr name, PierStr snbt);

/**
 * Entity sink: invoked once per entity whose position falls inside the region.
 *   x, y, z : the block cell that contains the entity (floor of its position).
 *   type    : entity type name, e.g. "minecraft:creeper".
 *   snbt    : the entity's serialized NBT (Actor::save) as SNBT.
 */
typedef void (*PierEntitySink)(void* ctx, int32_t x, int32_t y, int32_t z, PierStr type, PierStr snbt);


/* ═════════════ 各域的载荷类型 ═════════════ */

/**
 * Player selector — the identifier half of the "handles are identifiers,
 * not pointers" rule. Resolved against the live player list on every call.
 *   kind: 0 = name (getRealName, falling back to getNameTag),
 *         1 = xuid, 2 = uuid (canonical string form).
 */
typedef struct PierPlayerSel
{
    int32_t kind;
    PierStr value;
} PierPlayerSel;

/** ActorUniqueID raw value. 0 / negative-invalid never resolves. */
typedef int64_t PierActorId;

/**
 * Container reference — "owner + which container".
 *   which: 0=inventory 1=ender_chest 2=armor 3=offhand 4=block container.
 *   player: valid for which 0..3.   dim/x/y/z: valid for which == 4.
 */
typedef struct PierContainerRef
{
    int32_t which;
    PierPlayerSel player;
    int32_t dim;
    int32_t x;
    int32_t y;
    int32_t z;
} PierContainerRef;

/** Raw byte sink (binary NBT). Bytes valid only within the call frame. */
typedef void (*PierBytesSink)(void* ctx, uint8_t const* data, size_t len);
/** Key/value sink (kvdb_iter). Views valid only within the call frame. */
typedef void (*PierKvSink)(void* ctx, PierStr key, PierStr value);
/** Actor sink (list_actors). */
typedef void (*PierActorSink)(void* ctx, PierActorId id, PierStr type_name);
/**
 * Form result callback. Invoked ONCE on the server thread when the player
 * responds (or the form is cancelled). result_snbt:
 *   cancelled       : {cancelled:1b, reason:N}
 *   SimpleForm      : {button:N}
 *   CustomForm      : {values:{<name>: string|double|int64 …}}
 *   ModalForm       : {button:"upper"|"lower"}
 * Muted (never called) if the mod is disabled before the player responds.
 */
typedef void (*PierFormResultCb)(void* user, PierStr result_snbt);

/** Opaque handle to an open key-value database owned by the loader. */
typedef void* PierKvDbHandle;

/* ═════════════════ Cross-mod event bus FFI types ═════════════════
 * A mod cannot hand another mod a function pointer: `ModHost::unload`
 * calls FreeLibrary, so the publisher would be left holding a pointer into an
 * unmapped dylib. The loader therefore owns the subscription table, with the
 * same weak_ptr + ticket discipline as Forms.cpp and the mod-scoped scheduler.
 *
 * The loader never parses `payload` — it is opaque UTF-8 (JSON, SNBT, or
 * anything else the two mods agree on). Keeping the loader format-agnostic is
 * deliberate: the alternative is a schema that every publisher has to satisfy
 * and that the loader has to version.
 *
 * Topics are plain strings; namespace them (`plot:enter`, not `enter`).
 */

/**
 * Subscriber callback. `topic` and `payload` are borrowed for the duration of
 * the call — copy anything you keep.
 *
 * The return value is a **veto**, and only for `bus_publish_vetoable`:
 *   true  = "refuse this",
 *   false = "no opinion".
 * It is ignored entirely by `bus_publish`. There is deliberately no way to
 * turn a refusal back into an approval: a subscriber can only tighten, never
 * loosen. Letting one mod override another's refusal means the *last*
 * subscriber to run decides, and subscriber order is not something either mod
 * controls.
 *
 * Called on the thread that published. Never called after the owning mod is
 * unloaded or while it is disabled.
 */
typedef bool (*PierBusCb)(void* user, PierStr topic, PierStr payload);

/**
 * Provider callback for the cross-mod **service registry** (query-style calls,
 * as opposed to the bus's one-way broadcast).
 *
 * Write the answer through `reply(ctx, ...)` — exactly once — and return true.
 * Return false to report failure; anything written first is handed to the
 * caller as the error text, which is what makes "no such plot" and "the
 * database is down" distinguishable at the call site.
 *
 * `request` and `reply` are opaque UTF-8 the two mods agree on out of band. The
 * loader never looks inside either.
 *
 * Runs synchronously on the CALLING thread, inside `service_call`. Never called
 * after the providing mod is unloaded or while it is disabled.
 */
typedef bool (*PierServiceCb)(
    void* user, PierStr name, PierStr request, void* ctx, PierStrSink reply);

/** service_call return codes. */
#define PIER_SERVICE_OK 0        /* provider ran and wrote a reply */
#define PIER_SERVICE_NOT_FOUND 1 /* nobody provides this name (or is disabled/unloaded) */
#define PIER_SERVICE_ERROR 2     /* provider returned false; reply holds its message */
#define PIER_SERVICE_REFUSED 3   /* bad name, self-call, or call-depth limit */

/* ═════════════════ 同工具链快车道 (same-toolchain fast lane) ═════════════════
 *
 * bus / service 都是「(名字, UTF-8 载荷) -> UTF-8 载荷」。那个形状是**跨语言**
 * 的最大公约数：任何语言的 mod 都能说这门话。代价是每次调用都要
 * 序列化一次，而且类型信息在字符串里全丢了。
 *
 * 这条车道是给「两边由同一次工具链编出、内存布局逐字节相同」这个特例准备的。
 * 那时候两个动态库里的 C 布局函数表逐字节相同，可以直接递指针。
 *
 * ── loader 在这里做什么、不做什么 ──
 *
 * 做：拥有名字 -> 车道的表（独占，和 service 一样）、校验指纹、发/收租约、
 *     持有一个**永不释放**的存活标志，并在提供方消失的那一刻把它清零。
 * 不做：解释 `data` / `vtable` 里的任何一个字节。那两个指针对 loader 而言是
 *       不透明的，就像 bus 的 payload 一样。
 *
 * ── 为什么非要 loader 掺一脚 ──
 *
 * 因为 `ModHost::unload` 调 `FreeLibrary`。提供方的**内存**可以靠 Arc
 * 活下去，但它的**代码段会被 unmap** —— 消费方手里那个函数指针在卸载之后是
 * use-after-free，而崩溃发生在消费方，日志里没有任何线索指向刚离开的那个 mod。
 *
 * 所以：
 *   1. `alive` 指向 loader 自己堆上的一格，**永远不释放**（车道数量是几十，
 *      泄漏的是几十个 uint32）。提供方走掉时 loader 把它写 0。消费方每次调用
 *      前读一下这一格 —— 一次普通的原子读，不过 FFI、不拿锁。这就是「高速」
 *      的实际含义：热路径上 loader 一行代码都不跑。
 *   2. 提供方消失时，loader 在 `FreeLibrary` **之前**替所有未归还的租约调用
 *      `release`，让提供方在自己的 dylib 里、用自己的分配器释放自己的东西。
 *
 * ── 指纹 ──
 *
 * 多数原生语言没有稳定 ABI。同一份契约类型被两个 cdylib 各编一遍，编译器
 * 元数据不同时，字段顺序**可能**不同 —— 而那是静默的内存错乱，不是崩溃。
 *
 * 所以不比版本号，比**指纹**：编译器版本、target、契约名与版本、函数表类型的
 * `TypeId` 与 `size_of`/`align_of`，全部揉进一个 u64。任何一处不同 -> 指纹不同
 * -> `lane_acquire` 返回 PIER_LANE_FINGERPRINT，一个指针都不递出去。
 *
 * 失败模式是「慢」（消费方降级回 service 的 JSON 通道），不是 UB。这个性质是
 * 这条车道敢存在的全部理由。
 *
 * 宿主只做**相等比较**，不解释指纹的含义 —— 那是 SDK 侧的事，而且必须是，
 * 否则「指纹里加一项」就成了一次 ABI 变更。
 */

/** 车道协议版本。和 PIER_ABI_VERSION 分开：车道的形状可以独立演进，
 *  而且不匹配时的处理方式不一样（拒绝这一条车道，而不是拒绝整个 mod）。 */
#define PIER_LANE_PROTOCOL 1u

/** lane_acquire 返回值。 */
#define PIER_LANE_OK 0          /* 拿到了，out 已填好 */
#define PIER_LANE_NOT_FOUND 1   /* 没人发布这个名字（没装那个 mod）*/
#define PIER_LANE_FINGERPRINT 2 /* 发布了，但指纹不同 —— 降级，别递指针 */
#define PIER_LANE_REFUSED 3     /* 名字非法 / 自取 / 提供方被禁用 / 协议不符 */

/**
 * 引用计数钩子，在**提供方自己的 dylib 里**执行。
 *
 * 由 loader 在 `lane_acquire` / `lane_release` 里调用，以及在提供方卸载时替
 * 所有未归还的租约补调 `release`。
 *
 * 不许回调进 loader（`lane_*` 的任何一个），会自死锁。典型实现是对提供方
 * 自己的引用计数做一次原子加/减 —— 两行都不碰锁。
 */
typedef void (*PierLaneRefFn)(void* data);

/** 提供方发布一条车道时描述它自己。所有字段由提供方填，loader 只搬运。 */
typedef struct PierLaneDesc
{
    /** sizeof(PierLaneDesc)，和 PierApi::struct_size 一个纪律。 */
    uint32_t struct_size;
    /** 必须等于 PIER_LANE_PROTOCOL，否则 publish 被拒。 */
    uint32_t protocol;
    /** 编译指纹。0 是保留值（表示「随便谁都能连」），**不要用**。 */
    uint64_t fingerprint;
    /** 提供方的状态指针（通常是一个引用计数对象交出来的裸指针）。
     *  loader 不解释。 */
    void* data;
    /** C 布局的函数表。loader 不解释，也不复制内容 —— 提供方必须保证它
     *  活到这条车道被撤销为止（静态存储期，或一块有意不回收的分配）。 */
    void const* vtable;
    /** 可为 NULL（那时租约不计数，靠 alive 标志兜底）。 */
    PierLaneRefFn retain;
    PierLaneRefFn release;
} PierLaneDesc;

/** `lane_acquire` 的产出。 */
typedef struct PierLaneRef
{
    /** sizeof(PierLaneRef) —— **由调用方填好再传进来**，loader 据此决定写到
     *  哪一格为止。这个方向和别处相反，因为这里是 loader 在写调用方的结构体。 */
    uint32_t struct_size;
    /** 归还时用。0 表示没拿到。 */
    uint64_t lease;
    /** 提供方的指纹。指纹不匹配时也会填，专门给诊断用 —— 服主要看到的是
     *  「你的两个 mod 是不同编译器编的」，不是「不匹配」四个字。 */
    uint64_t fingerprint;
    void* data;
    void const* vtable;
    /**
     * **loader 拥有的存活标志**，非 0 = 提供方还在。
     *
     * 这一格永不释放，所以提供方卸载之后读它仍然是合法的 —— 那正是它存在的
     * 理由。消费方每次调用前读一次，用 **acquire**（写端是 release store；
     * relaxed load 配 release store 不构成 synchronizes-with）。
     *
     * 指纹不匹配时为 NULL。
     */
    uint32_t const* alive;
    /**
     * **调用中计数**，同样由 loader 拥有、永不释放。消费方在进入提供方的表项
     * 之前 +1，返回之后 -1。
     *
     * 为什么需要它：`alive` 只能挡住「调用之前提供方已经走了」，挡不住检查与
     * 调用之间的那个窗口。全部服务器线程调用挡住了**并发**卸载，但挡不住
     * **重入**卸载 —— 提供方的表项自己触发了一次命令派发，那条命令把提供方
     * 卸了，于是 `FreeLibrary` 发生在一个仍然停在提供方代码里的栈帧下面。
     *
     * loader 在 `unload` 的最前面读这个计数：非 0 就直接拒绝卸载并说明原因，
     * 而不是先卸再崩。
     *
     * 追加字段，受 `struct_size` 保护：老消费方填的 struct_size 到不了这里，
     * loader 就不写，它们的行为和以前一模一样。
     */
    uint32_t* busy;
} PierLaneRef;

/* ═════════════════ Packet interception FFI types ═════════════════
 * Used by packet_hook_register / packet_conn_hook_register. See the block
 * comment on those fields in PierApi for the full contract. */

/** PierPacketEvent::direction, and the bit positions used by dir_mask. */
#define PIER_PKT_INBOUND 0  /* client -> server */
#define PIER_PKT_OUTBOUND 1 /* server -> client */

#define PIER_PKT_MASK_INBOUND (1 << PIER_PKT_INBOUND)
#define PIER_PKT_MASK_OUTBOUND (1 << PIER_PKT_OUTBOUND)

/** PierPacketCb return value. Anything else is treated as PASS. */
#define PIER_PKT_PASS 0    /* forward unchanged; `replace` output ignored */
#define PIER_PKT_REPLACE 1 /* forward the body handed to `replace` */
#define PIER_PKT_DROP 2    /* swallow the packet entirely */

/**
 * One intercepted packet. Every pointer inside is borrowed and valid only for
 * the duration of the callback — copy anything you keep.
 */
typedef struct PierPacketEvent
{
    /** sizeof(PierPacketEvent) as the LOADER knows it. Check before reading
     *  trailing fields, same discipline as PierApi::struct_size. */
    uint32_t struct_size;
    /** PIER_PKT_INBOUND / PIER_PKT_OUTBOUND. */
    int32_t direction;
    /** NetworkIdentifier::getHash() — stable for the connection's lifetime and
     *  available before the player exists, which is exactly when a login-phase
     *  rewrite needs to key its state. */
    uint64_t conn_id;
    /** "host:port" (NetworkIdentifier::getIPAndPort). */
    PierStr address;
    /** MinecraftPacketIds value decoded from the header. */
    int32_t packet_id;
    uint8_t sender_sub_id;
    uint8_t target_sub_id;
    /** Packet body, header excluded. NULL only when body_len is 0. */
    uint8_t const* body;
    size_t body_len;
} PierPacketEvent;

/**
 * Mutable header fields, pre-filled from the event. Assignments here only take
 * effect when the callback returns PIER_PKT_REPLACE.
 */
typedef struct PierPacketEdit
{
    uint32_t struct_size;
    int32_t packet_id;
    uint8_t sender_sub_id;
    uint8_t target_sub_id;
} PierPacketEdit;

/** Drop via packet_hook_unregister / packet_conn_hook_unregister. */
typedef void* PierPacketHookHandle;

/**
 * Packet interceptor. To rewrite, call `replace(replace_ctx, bytes, len)` with
 * the NEW BODY (header excluded) and return PIER_PKT_REPLACE. Calling
 * `replace` more than once keeps the last body; returning REPLACE without ever
 * calling it means "empty body".
 */
typedef int32_t (*PierPacketCb)(
    void* user,
    PierPacketEvent const* ev,
    PierPacketEdit* edit,
    void* replace_ctx,
    PierBytesSink replace
);

/** Connection lifecycle: `opened` is true on accept, false on close. */
typedef void (*PierConnCb)(void* user, uint64_t conn_id, PierStr address, bool opened);

/* ── 客户端能力组的 FFI 类型。类型声明无条件存在（不占布局），对应功能
 * 是否可用由 PierApi 里 client_* 槽位是否为 NULL 决定。 ── */
/** Opaque handle to a registered key binding owned by the loader's
 * ll::input::KeyRegistry. Drop via client_unregister_key. */
typedef void* PierKeyHandle;

/** Key action: 0 = released (up), 1 = pressed (down).
 * Mirrors ll::event::KeyInputEvent::Action. */
typedef int32_t PierKeyAction;

/** Focus impact level: 0=Neutral 1=ActivateFocus 2=DeactivateFocus.
 * Mirrors ::FocusImpact. */
typedef int32_t PierFocusImpact;

/** Callback for key press/release events. Runs on the client thread.
 *  user   — pointer passed to client_register_key
 *  action — 0=released 1=pressed (see PierKeyAction)
 *  impact — current focus impact (see PierFocusImpact) */
typedef void (*PierKeyCb)(void* user, PierKeyAction action, PierFocusImpact impact);

/* ── 属性 / 动作键。  APPEND-ONLY: never renumber or remove. ──
 * Unknown values make the call return false; a safe SDK layer maps that
 * to Err("unsupported"), which is the forward-compat negotiation.        */

/** player_get_num / player_set_num keys. (G)=get-only, (S)=settable. */
enum PierPlayerNumProp
{
    PIER_PPROP_GAME_TYPE = 0, /* (G) Player::getPlayerGameType; write via player_set_gamemode */
    PIER_PPROP_LEVEL = 1, /* (S) attribute Player::LEVEL() */
    PIER_PPROP_EXPERIENCE = 2, /* (S) attribute Player::EXPERIENCE() (progress 0..1) */
    PIER_PPROP_HUNGER = 3, /* (S) attribute Player::HUNGER() */
    PIER_PPROP_SATURATION = 4, /* (S) attribute Player::SATURATION() */
    PIER_PPROP_EXHAUSTION = 5, /* (S) attribute Player::EXHAUSTION() */
    PIER_PPROP_XP_NEEDED_NEXT_LEVEL = 6, /* (G) Player::getXpNeededForNextLevel */
    PIER_PPROP_LUCK = 7, /* (G) Player::getLuck */
    PIER_PPROP_SELECTED_SLOT = 8, /* (G) Player::getSelectedItemSlot; set via PIER_PACT_SET_SELECTED_SLOT */
    PIER_PPROP_IS_OPERATOR = 9, /* (G) Player::isOperator */
    PIER_PPROP_CAN_USE_OPERATOR_BLOCKS = 10, /* (G) Player::canUseOperatorBlocks */
    PIER_PPROP_IS_FLYING = 11, /* (G) Player::isFlying */
    PIER_PPROP_CAN_JUMP = 12, /* (G) Player::canJump */
    PIER_PPROP_IS_EMOTING = 13, /* (G) Player::isEmoting */
    PIER_PPROP_IS_IN_RAID = 14, /* (G) Player::isInRaid */
    PIER_PPROP_IS_HURT = 15, /* (G) Player::isHurt */
    PIER_PPROP_IS_SCOPING = 16, /* (G) Player::isScoping */
    PIER_PPROP_CAN_SLEEP = 17, /* (G) Player::canSleep */
    PIER_PPROP_HAS_RESPAWN_POSITION = 18, /* (G) Player::hasRespawnPosition */
    PIER_PPROP_CLIENT_SUB_ID = 19, /* (G) Player::getClientSubId */
    PIER_PPROP_CAN_USE_ABILITY = 20,
    /* (G) Player::canUseAbility; ability index passed via player_action GET path — see PIER_PACT_CAN_USE_ABILITY */
    /* ── 追加：player gap fill ── */
    PIER_PPROP_DIRECTION = 21, /* (G) Player::getDirection (0=S,1=W,2=N,3=E) */
    PIER_PPROP_CHUNK_RADIUS = 22, /* (G) Player::getChunkRadius */
    PIER_PPROP_NETWORK_RTT = 23, /* (G) getNetworkStatus().mPing (ms) */
    PIER_PPROP_PLATFORM = 24, /* (G) Player::getPlatform */
    PIER_PPROP_ENCHANTMENT_SEED = 25, /* (G) Player::getEnchantmentSeed */
    PIER_PPROP_IS_USING_ITEM = 26, /* (G) Player::isUsingItem */
    PIER_PPROP_IS_BLOCKING = 27, /* (G) Player::isBlocking */
    PIER_PPROP_IS_GLIDING = 28, /* (G) Player::isGliding */
    PIER_PPROP_IS_SWIMMING = 29, /* (G) Player::isSwimming */
    PIER_PPROP_PERMISSION_LEVEL = 30, /* (G) Player::getPlayerPermissionLevel */
    PIER_PPROP_SCORE = 31, /* (G) Player::getScore */
    PIER_PPROP_FALL_DISTANCE = 32, /* (G) Actor::getFallDistance */
    PIER_PPROP_IS_DEAD = 33, /* (G) Actor::isDead */
    PIER_PPROP_HAS_DIED_BEFORE = 34, /* (G) Player::hasDiedBefore */
    PIER_PPROP_DIMENSION = 35, /* (G) Actor::getDimensionId */
};

/** player_get_str keys. */
enum PierPlayerStrProp
{
    PIER_PSTR_REAL_NAME = 0, /* Player::getRealName */
    PIER_PSTR_UUID = 1, /* Player::getUuid().asString() */
    PIER_PSTR_XUID = 2, /* Player::getXuid */
    PIER_PSTR_IP_AND_PORT = 3, /* Player::getIPAndPort */
    PIER_PSTR_LOCALE_CODE = 4, /* Player::getLocaleCode */
    PIER_PSTR_NAME_TAG = 5, /* Actor::getNameTag (display name) */
    /* ── 追加 ── */
    PIER_PSTR_LAST_DEATH_POS = 6, /* SNBT {x,y,z} or "" if none */
    PIER_PSTR_LAST_DEATH_DIMENSION = 7, /* dimension id as string */
    PIER_PSTR_NETWORK_STATUS = 8, /* SNBT {ping,avg_ping,packet_loss,max_ping} */
    PIER_PSTR_PLATFORM_ONLINE_ID = 9, /* Player::getPlatformOnlineId */
};

/**
 * player_action verbs.  Args are (sarg, a, b, c); unused args are ignored.
 * `out` (when non-NULL) receives a result string where noted.
 */
enum PierPlayerAction
{
    PIER_PACT_SET_ABILITY = 0,
    /* a=AbilitiesIndex, b=0/1 (bool slots) or float (FlySpeed etc.) Player::setAbility.
       写完会把 PlayerPermissionLevel 还原成写之前那个值 —— 引擎的
       LayeredAbilities::setAbility 是「切成自定义权限」那条路，会把玩家推到
       Custom 上，而那个等级和能力层一起装在 UpdateAbilitiesPacket 里发给客户端。
       要改等级请显式用 PIER_PACT_SET_PERMISSION_LEVEL。 */
    PIER_PACT_CAN_USE_ABILITY = 1, /* a=AbilitiesIndex → out "0"/"1" Player::canUseAbility */
    PIER_PACT_SET_SELECTED_SLOT = 2, /* a=slot                          Player::setSelectedSlot */
    PIER_PACT_GIVE_ITEM = 3, /* sarg=item SNBT                  ItemStack::fromTag + Player::addAndRefresh */
    PIER_PACT_SET_SPAWN_POINT = 4, /* a,b,c=pos, sarg=dim ("0".."2")  via /spawnpoint */
    PIER_PACT_CLEAR_TITLE = 5, /* via /title clear */
    PIER_PACT_SET_TITLE = 6, /* sarg=text, a=slot(0 title,1 subtitle,2 actionbar) via /title */
    /* ── 追加 ── */
    PIER_PACT_ADD_EXPERIENCE = 7, /* a=xp                  Player::addExperience */
    PIER_PACT_ADD_LEVELS = 8, /* a=levels              Player::addLevels */
    PIER_PACT_START_COOLDOWN = 9, /* sarg=item name, a=ticks Player::startItemCooldown */
    PIER_PACT_START_RIDING = 10, /* a=vehicle ActorUniqueID (lower 64b) Player::startRiding */
    PIER_PACT_STOP_RIDING = 11, /*                       Player::stopRiding */
    PIER_PACT_ATTACK = 12, /* a=target ActorUniqueID (lower 64b) Player::attack */
    PIER_PACT_DROP = 13, /* sarg=item SNBT, a=random(0/1) Player::drop */
    PIER_PACT_INTERACT = 14, /* a=target ActorUniqueID        Player::interact */
    PIER_PACT_START_USING_ITEM = 15, /* sarg=item SNBT, a=duration    Player::startUsingItem */
    PIER_PACT_STOP_USING_ITEM = 16, /*                       Player::stopUsingItem */
    PIER_PACT_SET_CHUNK_RADIUS = 17, /* a=radius              Player::setChunkRadius */
    PIER_PACT_SET_ENCHANTMENT_SEED = 18, /* a=seed                Player::setEnchantmentSeed */
    PIER_PACT_REGISTER_TRACKED_BOSS = 19, /* a=boss ActorUniqueID  Player::registerTrackedBoss */
    PIER_PACT_UNREGISTER_TRACKED_BOSS = 20, /* a=boss ActorUniqueID Player::unRegisterTrackedBoss */
    PIER_PACT_PLAY_EMOTE = 21, /* sarg=piece id         Player::playEmote */
    PIER_PACT_RESEND_ALL_CHUNKS = 22, /*                       Player::resendAllChunks */
    PIER_PACT_OPEN_INVENTORY = 23, /*                       Player::openInventory */
    PIER_PACT_SIDEBAR_SET = 24, /* sarg="obj\ntitle\nline…"  per-player sidebar */
    PIER_PACT_SIDEBAR_CLEAR = 25, /* sarg=objective        RemoveObjectivePacket */
    PIER_PACT_SET_PERMISSION_LEVEL = 26,
    /* a=PlayerPermissionLevel（0 Visitor / 1 Member / 2 Operator / 3 Custom）
       LayeredAbilities::setPlayerPermissions + UpdateAbilitiesPacket。
       读的那一侧是 PIER_PPROP_PERMISSION_LEVEL。 */
};

/** actor_get_num / actor_set_num keys. (S)=settable via actor_set_num. */
enum PierActorNumProp
{
    PIER_APROP_POS_X = 0, /* (G) Actor::getPosition().x (feet: getFeetPos for players; POS_* uses getPosition) */
    PIER_APROP_POS_Y = 1, /* (G) */
    PIER_APROP_POS_Z = 2, /* (G) */
    PIER_APROP_ROT_PITCH = 3, /* (G) Actor::getRotation().x */
    PIER_APROP_ROT_YAW = 4, /* (G) Actor::getRotation().y */
    PIER_APROP_DIMENSION = 5, /* (G) Actor::getDimensionId */
    PIER_APROP_HEALTH = 6, /* (G) Actor::getHealth; heal/hurt via actions */
    PIER_APROP_MAX_HEALTH = 7, /* (G) Actor::getMaxHealth */
    PIER_APROP_IS_ALIVE = 8, /* (G) Actor::isAlive */
    PIER_APROP_IS_ON_GROUND = 9, /* (G) Actor::isOnGround */
    PIER_APROP_IS_IN_WATER = 10, /* (G) Actor::isInWater */
    PIER_APROP_IS_IN_LAVA = 11, /* (G) Actor::isInLava */
    PIER_APROP_IS_ON_FIRE = 12, /* (G) Actor::isOnFire */
    PIER_APROP_IS_INVISIBLE = 13, /* (G) Actor::isInvisible */
    PIER_APROP_IS_SNEAKING = 14, /* (G) Actor::isSneaking */
    PIER_APROP_IS_BABY = 15, /* (G) Actor::isBaby */
    PIER_APROP_IS_RIDING = 16, /* (G) Actor::isRiding */
    PIER_APROP_IS_TAME = 17, /* (G) Actor::isTame */
    PIER_APROP_SPEED = 18, /* (G) Actor::getSpeedInMetersPerSecond */
    /* ── 追加：actor gap fill ── */
    PIER_APROP_VIEW_X = 19, /* (G) Actor::getViewVector().x */
    PIER_APROP_VIEW_Y = 20, /* (G) Actor::getViewVector().y */
    PIER_APROP_VIEW_Z = 21, /* (G) Actor::getViewVector().z */
    PIER_APROP_VEL_X = 22, /* (G) Actor::getVelocity().x */
    PIER_APROP_VEL_Y = 23, /* (G) Actor::getVelocity().y */
    PIER_APROP_VEL_Z = 24, /* (G) Actor::getVelocity().z */
    PIER_APROP_HEAD_X = 25, /* (G) Actor::getHeadPos().x */
    PIER_APROP_HEAD_Y = 26, /* (G) Actor::getHeadPos().y */
    PIER_APROP_HEAD_Z = 27, /* (G) Actor::getHeadPos().z */
    PIER_APROP_FEET_X = 28, /* (G) Actor::getFeetPos().x */
    PIER_APROP_FEET_Y = 29, /* (G) Actor::getFeetPos().y */
    PIER_APROP_FEET_Z = 30, /* (G) Actor::getFeetPos().z */
    PIER_APROP_FALL_DISTANCE = 31, /* (G) Actor::getFallDistance */
    PIER_APROP_IS_PERSISTENT = 32, /* (G) Actor::isPersistent */
    PIER_APROP_IS_LEASHED = 33, /* (G) Actor::isLeashed */
    PIER_APROP_IS_INVULNERABLE = 34, /* (G) Actor::isInvulnerable */
    PIER_APROP_VARIANT = 35, /* (G) Actor::getVariant */
    PIER_APROP_MARK_VARIANT = 36, /* (G) Actor::getMarkVariant */
    PIER_APROP_SCALE = 37, /* (G) Actor::getScaleFactor */
    PIER_APROP_BRIGHTNESS = 38, /* (G) Actor::getBrightness */
    PIER_APROP_RADIUS = 39, /* (G) Actor::getRadius */
    PIER_APROP_HAS_TOTEM = 40, /* (G) Actor::hasTotemEquipped */
    PIER_APROP_IS_IN_RAIN = 41, /* (G) Actor::isInRain */
    PIER_APROP_IS_IN_SNOW = 42, /* (G) Actor::isInSnow */
    PIER_APROP_IS_IN_THUNDERSTORM = 43, /* (G) Actor::isInThunderstorm */
    PIER_APROP_IS_FROZEN = 44, /* (G) Actor::isFrozen */
    PIER_APROP_IS_IN_LOVE = 45, /* (G) Actor::isInLove */
    PIER_APROP_DEATH_TIME = 46, /* (G) Actor::getDeathTime */
    PIER_APROP_HAS_PASSENGER = 47, /* (G) Actor::hasPassenger */
};

/** actor_get_str keys. */
enum PierActorStrProp
{
    PIER_ASTR_TYPE_NAME = 0, /* Actor::getTypeName */
    PIER_ASTR_NAME_TAG = 1, /* Actor::getNameTag */
    /* ── 追加 ── */
    PIER_ASTR_SCORE_TAG = 2, /* Actor::getScoreTag */
    PIER_ASTR_FILTERED_NAME = 3, /* Actor::getFilteredNameTag */
};

/** actor_action verbs. Args (sarg, a, b, c); `out` receives a result where noted. */
enum PierActorAction
{
    PIER_AACT_KILL = 0, /* Actor::kill */
    PIER_AACT_DESPAWN = 1, /* Actor::despawn */
    PIER_AACT_HEAL = 2, /* a=amount                            Actor::heal */
    PIER_AACT_SET_ON_FIRE = 3, /* a=seconds                           Actor::setOnFire */
    PIER_AACT_TELEPORT = 4, /* a,b,c=pos, sarg=dim ("0".."2")      Actor::teleport */
    PIER_AACT_SET_NAME_TAG = 5, /* sarg=name                           Actor::setNameTag */
    PIER_AACT_ADD_TAG = 6, /* sarg=tag → out "0"/"1"              Actor::addTag */
    PIER_AACT_REMOVE_TAG = 7, /* sarg=tag → out "0"/"1"              Actor::removeTag */
    PIER_AACT_HAS_TAG = 8, /* sarg=tag → out "0"/"1"              Actor::hasTag */
    PIER_AACT_ADD_EFFECT = 9, /* sarg=effect name, a=ticks, b=amplifier, c=visible(0/1)
                                         MobEffect::getByName + Actor::addEffect */
    PIER_AACT_REMOVE_EFFECT = 10, /* sarg=effect name                    Actor::removeEffect(id) */
    PIER_AACT_CLEAR_EFFECTS = 11, /* Actor::removeAllEffects */
    PIER_AACT_HURT = 12, /* a=damage (generic damage source)    Actor::hurt */
    PIER_AACT_ATTRIBUTE_GET = 13, /* sarg=attribute name ("minecraft:health" …) → out value */
    /* ── 追加 ── */
    PIER_AACT_SET_VARIANT = 14, /* a=variant             Actor::setVariant */
    PIER_AACT_SET_MARK_VARIANT = 15, /* a=variant             Actor::setMarkVariant */
    PIER_AACT_SET_PERSISTENT = 16, /*                       Actor::setPersistent */
    PIER_AACT_SET_LEASH_HOLDER = 17, /* a=holder ActorUniqueID Actor::setLeashHolder */
    PIER_AACT_SET_INVISIBLE = 18, /* a=0/1                 Actor::setInvisible */
    PIER_AACT_SET_SNEAKING = 19, /* a=0/1                 Actor::setSneaking */
    PIER_AACT_SET_NAME_TAG_VISIBLE = 20, /* a=0/1                 Actor::setNameTagVisible */
    PIER_AACT_SET_TARGET = 21, /* a=target ActorUniqueID Actor::setTarget */
    PIER_AACT_SET_OWNER = 22, /* a=owner ActorUniqueID  Actor::setOwner */
    PIER_AACT_BURN = 23, /* a=damage              Actor::burn */
    PIER_AACT_STOP_FIRE = 24, /*                       Actor::extinguishFire */
    PIER_AACT_SET_VELOCITY = 25, /* a,b,c=vel             Actor::setVelocity */
    PIER_AACT_APPLY_IMPULSE = 26, /* a,b,c=impulse         Actor::applyImpulse */
    PIER_AACT_SET_SCORE_TAG = 27, /* sarg=text             Actor::setScoreTag */
    PIER_AACT_SET_SKIN_ID = 28, /* a=skin id             Actor::setSkinID */
    PIER_AACT_SET_STRENGTH = 29, /* a=strength            Actor::setStrength */
    PIER_AACT_REMOVE_ALL_PASSENGERS = 30, /*                       Actor::removeAllPassengers */
    PIER_AACT_EXECUTE_EVENT = 31, /* sarg=event name       Actor::executeEvent */
    PIER_AACT_SET_ROTATION = 32, /* a=pitch b=yaw         Actor::setRotationWrapped */
};

/** block_get_num keys. */
enum PierBlockNumProp
{
    PIER_BPROP_IS_AIR = 0, /* Block::isAir */
    PIER_BPROP_DATA = 1, /* Block::getData (legacy data value) */
    PIER_BPROP_BLOCK_ITEM_ID = 2, /* Block::getBlockItemId */
    PIER_BPROP_IS_CRAFTING_BLOCK = 3, /* Block::isCraftingBlock */
    PIER_BPROP_IS_INTERACTIVE_BLOCK = 4, /* Block::isInteractiveBlock */
    PIER_BPROP_HAS_BLOCK_ENTITY = 5, /* BlockSource::getBlockEntity(pos) != null */
    /* ── 追加：block gap fill ── */
    PIER_BPROP_LIGHT = 6, /* Block::getLight */
    PIER_BPROP_LIGHT_EMISSION = 7, /* Block::getLightEmission */
    PIER_BPROP_DESTROY_SPEED = 8, /* Block::getDestroySpeed */
    PIER_BPROP_EXPLOSION_RESISTANCE = 9, /* Block::getExplosionResistance */
    PIER_BPROP_FRICTION = 10, /* Block::getFriction */
    PIER_BPROP_IS_CONTAINER = 11, /* Block::isContainerBlock */
    PIER_BPROP_IS_DOOR = 12, /* Block::isDoorBlock */
    PIER_BPROP_IS_FENCE = 13, /* Block::isFenceBlock */
    PIER_BPROP_IS_RAIL = 14, /* Block::isRailBlock */
    PIER_BPROP_IS_SLAB = 15, /* Block::isSlabBlock */
    PIER_BPROP_IS_STAIR = 16, /* Block::isStairBlock */
    PIER_BPROP_IS_WALL = 17, /* Block::isWallBlock */
    PIER_BPROP_IS_CROP = 18, /* Block::isCropBlock */
    PIER_BPROP_IS_UNBREAKABLE = 19, /* Block::isUnbreakable */
    PIER_BPROP_REDSTONE_SIGNAL = 20, /* Block::getDirectSignal */
    PIER_BPROP_COMPARATOR_SIGNAL = 21, /* Block::getComparatorSignal */
    PIER_BPROP_IS_SIGNAL_SOURCE = 22, /* Block::isSignalSource */
    PIER_BPROP_VARIANT = 23, /* Block::getVariant */
    PIER_BPROP_BURN_ODDS = 24, /* Block::getBurnOdds */
    PIER_BPROP_FLAME_ODDS = 25, /* Block::getFlameOdds */
    PIER_BPROP_BOUNCINESS = 26, /* Block::getBounciness */
    PIER_BPROP_IS_SOLID = 27, /* Block::isSolid */
    PIER_BPROP_REQUIRES_TOOL = 28, /* Block::requiresCorrectToolForDrops */
};

/** block_get_str keys. */
enum PierBlockStrProp
{
    PIER_BSTR_TYPE_NAME = 0, /* Block::getTypeName */
    PIER_BSTR_SNBT = 1, /* Block::mSerializationId → SNBT {name,states,version} */
    PIER_BSTR_DESCRIPTION_ID = 2, /* Block::getDescriptionId */
    PIER_BSTR_DEBUG_STRING = 3, /* Block::toDebugString */
    PIER_BSTR_TAGS = 4, /* Block::mTags → SNBT string list ["a","b"] */
    /* ── 追加 ── */
    PIER_BSTR_STATE = 5, /* SNBT {state_name:value, …} all block states */
    PIER_BSTR_COLLISION_SHAPE = 6, /* SNBT [{min:[x,y,z],max:[x,y,z]}, …] */
    PIER_BSTR_OUTLINE_SHAPE = 7, /* SNBT [{min,max}] render outline */
    PIER_BSTR_DISPLAY_NAME = 8, /* Block::getDisplayName */
};

/** block_action verbs. */
enum PierBlockAction
{
    PIER_BACT_HAS_TAG = 0, /* sarg=tag → out "0"/"1"  Block::hasTag */
    /* ── 追加 ── */
    PIER_BACT_GET_STATE = 1, /* sarg=state name → out value string  Block::getState */
    PIER_BACT_POP_RESOURCE = 2, /* sarg=item SNBT → pop resource at pos  Block::popResource */
    PIER_BACT_AS_ITEM = 3, /* → out item SNBT   Block::asItemInstance */
};

/** item_get_num keys (query a transient ItemStack rebuilt from SNBT). */
enum PierItemNumProp
{
    PIER_IPROP_COUNT = 0, /* ItemStackBase::mCount */
    PIER_IPROP_MAX_STACK_SIZE = 1, /* ItemStackBase::getMaxStackSize */
    PIER_IPROP_AUX_VALUE = 2, /* ItemStackBase::getAuxValue */
    PIER_IPROP_ID = 3, /* ItemStackBase::getId */
    PIER_IPROP_DAMAGE = 4, /* ItemStackBase::getDamageValue */
    PIER_IPROP_IS_NULL = 5, /* ItemStackBase::isNull */
    PIER_IPROP_IS_BLOCK = 6, /* ItemStackBase::isBlock */
    PIER_IPROP_IS_ENCHANTED = 7, /* ItemStackBase::isEnchanted */
    PIER_IPROP_IS_ARMOR = 8, /* ItemStackBase::isArmorItem */
    PIER_IPROP_IS_DAMAGEABLE = 9, /* ItemStackBase::isDamageableItem */
    PIER_IPROP_IS_DAMAGED = 10, /* ItemStackBase::isDamaged */
    /* ── 追加：item gap fill ── */
    PIER_IPROP_MAX_DAMAGE = 11, /* ItemStackBase::getMaxDamage */
    PIER_IPROP_IS_UNBREAKABLE = 12, /* ItemStackBase::isUnbreakable */
    PIER_IPROP_HAS_DURABILITY = 13, /* ItemStackBase::hasDurability */
    PIER_IPROP_IS_POTION = 14, /* ItemStackBase::isPotionItem */
    PIER_IPROP_IS_THROWABLE = 15, /* ItemStackBase::isThrowable */
    PIER_IPROP_IS_FIRE_RESISTANT = 16, /* ItemStackBase::isFireResistant */
    PIER_IPROP_ATTACK_DAMAGE = 17, /* ItemStackBase::getAttackDamage */
    PIER_IPROP_REPAIR_COST = 18, /* ItemStackBase::getBaseRepairCost */
    PIER_IPROP_ENCHANT_VALUE = 19, /* ItemStackBase::getEnchantValue */
    PIER_IPROP_IS_STACKABLE = 20, /* ItemStackBase::isStackable */
    PIER_IPROP_IS_MUSIC_DISC = 21, /* ItemStackBase::isMusicDiscItem */
    PIER_IPROP_IS_OFFHAND = 22, /* ItemStackBase::isOffhandItem */
    PIER_IPROP_USE_DURATION = 23, /* ItemStackBase::getMaxUseDuration */
    PIER_IPROP_IS_GLINT = 24, /* ItemStackBase::isGlint */
    PIER_IPROP_IS_BUNDLE = 25, /* ItemStackBase::isBundle */
    PIER_IPROP_HAS_USER_DATA = 26, /* ItemStackBase::hasUserData */
    PIER_IPROP_HAS_CUSTOM_NAME = 27, /* ItemStackBase::hasCustomHoverName */
};

/** item_get_str keys. */
enum PierItemStrProp
{
    PIER_ISTR_TYPE_NAME = 0, /* ItemStackBase::getTypeName ("minecraft:apple") */
    PIER_ISTR_NAME = 1, /* ItemStackBase::getName (display) */
    PIER_ISTR_CUSTOM_NAME = 2, /* ItemStackBase::getCustomName */
    PIER_ISTR_RAW_NAME_ID = 3, /* ItemStackBase::getRawNameId */
    /* ── 追加 ── */
    PIER_ISTR_LORE = 4, /* SNBT list ["l1","l2"]  ItemStackBase::getCustomLore */
    PIER_ISTR_CAN_DESTROY = 5, /* SNBT list ["minecraft:stone", …] */
    PIER_ISTR_CAN_PLACE_ON = 6, /* SNBT list */
    PIER_ISTR_USER_DATA = 7, /* full NBT user data as SNBT */
    PIER_ISTR_HOVER_NAME = 8, /* ItemStackBase::getHoverName */
    PIER_ISTR_EFFECT_NAME = 9, /* ItemStackBase::getEffectName */
    PIER_ISTR_COLOR = 10, /* SNBT {r,g,b}  ItemStackBase::getColor */
};

/** item_transform ops: rebuild → mutate → serialize back (out = new SNBT). */
enum PierItemOp
{
    PIER_IOP_SET_CUSTOM_NAME = 0, /* sarg=name             ItemStackBase::setCustomName */
    PIER_IOP_SET_DAMAGE = 1, /* narg=damage           ItemStackBase::setDamageValue */
    PIER_IOP_SET_COUNT = 2, /* narg=count            ItemStackBase::mCount */
    PIER_IOP_SET_LORE = 3, /* sarg=SNBT list ["l1","l2"]  ItemStackBase::setCustomLore */
    /* ── 追加 ── */
    PIER_IOP_SET_UNBREAKABLE = 4, /* narg=0/1               ItemStackBase::setUnbreakable */
    PIER_IOP_HURT_AND_BREAK = 5, /* narg=damage            ItemStackBase::hurtAndBreak */
    PIER_IOP_SET_REPAIR_COST = 6, /* narg=cost              ItemStackBase::setRepairCost */
    PIER_IOP_ADD_ENCHANT = 7, /* sarg="name:level"      saveEnchantsToUserData */
    PIER_IOP_REMOVE_ENCHANTS = 8, /*                        ItemStackBase::removeEnchants */
    PIER_IOP_CLEAR_LORE = 9, /*                        ItemStackBase::clearCustomLore */
    PIER_IOP_RESET_NAME = 10, /*                        ItemStackBase::resetHoverName */
    PIER_IOP_SET_CAN_DESTROY = 11, /* sarg=SNBT list         ItemStackBase::setCanDestroy */
    PIER_IOP_SET_CAN_PLACE_ON = 12, /* sarg=SNBT list         ItemStackBase::setCanPlaceOn */
};

/** scoreboard_op verbs (args a=objective/slot, b=target, n=value). */
enum PierScoreboardOp
{
    PIER_SB_ADD_OBJECTIVE = 0, /* a=name, b=display name → out "1"      Scoreboard::addObjective("dummy") */
    PIER_SB_REMOVE_OBJECTIVE = 1, /* a=name                                Scoreboard::removeObjective */
    PIER_SB_LIST_OBJECTIVES = 2, /* → out SNBT [{name,display}, …]        Scoreboard::getObjectives */
    PIER_SB_GET_SCORE = 3, /* a=objective, b=fake-player name → out value  Objective::getPlayerScore */
    PIER_SB_SET_SCORE = 4, /* a=objective, b=name, n=value          Scoreboard::modifyPlayerScore(Set) */
    PIER_SB_ADD_SCORE = 5, /* a=objective, b=name, n=value          … (Add) */
    PIER_SB_REDUCE_SCORE = 6, /* a=objective, b=name, n=value          … (Subtract) */
    PIER_SB_RESET_SCORE = 7, /* a=objective, b=name                   Scoreboard::resetPlayerScore */
    PIER_SB_SET_DISPLAY = 8, /* a=slot("sidebar"/"list"/"belowname"), b=objective  setDisplayObjective */
    PIER_SB_CLEAR_DISPLAY = 9, /* a=slot                                clearDisplayObjective */
};

/** sys_info_str keys. */
/** Per-dimension behaviour rules for md_set_dimension_rule.
 *
 *  These are deliberately NOT a mirror of any engine enum: they name things
 *  the loader intercepts itself. Values are ABI — append only, never renumber.
 */
enum PierDimRule
{
    PIER_DIMRULE_SPAWN_MONSTER = 0, /* natural hostile spawns */
    PIER_DIMRULE_SPAWN_ANIMAL = 1, /* natural passive spawns */
    PIER_DIMRULE_SPAWN_SPAWNER = 2, /* spawns from mob spawners */
    PIER_DIMRULE_EXPLODE_BLOCKS = 3, /* explosions damaging terrain */
    PIER_DIMRULE_FIRE_SPREAD = 4, /* fire spreading to neighbours */
    PIER_DIMRULE_MOB_GRIEFING = 5, /* mobs changing blocks */
    PIER_DIMRULE_PROJECTILE = 6, /* projectile spawns */
    /* ── 第二批（挂载点参考 LegacyScriptEngine 的同名事件） ── */
    PIER_DIMRULE_PISTON_PUSH = 7, /* pistons moving blocks */
    PIER_DIMRULE_LIQUID_FLOW = 8, /* water/lava spreading */
    PIER_DIMRULE_FARMLAND_DECAY = 9, /* farmland trampled back to dirt */
    PIER_DIMRULE_RIDE = 10, /* mounting boats/minecarts/animals */
    /* ── Plot-boundary confinement (needs md_set_plot_grid) ── */
    /* Pistons moving blocks ACROSS a plot boundary. Distinct from
     * PIER_DIMRULE_PISTON_PUSH, which disables pistons for the whole
     * dimension: this one leaves them working inside a plot and only refuses
     * the push that would cross the edge. Both apply — either one denying is
     * enough to stop the push. Inert in dimensions with no registered grid. */
    PIER_DIMRULE_PISTON_CROSS_PLOT = 11,
    /* Entities crossing a plot boundary. Players and ridden vehicles are
     * never confined — see PlotConfine.cpp for why. */
    PIER_DIMRULE_ENTITY_CROSS_PLOT = 12,
};

enum PierSysInfoProp
{
    PIER_SYS_OS_NAME = 0, /* sys_utils::getSystemName */
    PIER_SYS_OS_VERSION = 1, /* sys_utils::getSystemVersion → string */
    PIER_SYS_LOCALE = 2, /* sys_utils::getSystemLocaleCode */
    PIER_SYS_LOCAL_TIME = 3, /* sys_utils::getLocalTime → SNBT {year,month,day,hour,minute,second,ms} */
};

/** server_info_str keys. */
enum PierServerInfoProp
{
    PIER_SRV_BDS_VERSION = 0, /* Common::getGameVersionString */
    PIER_SRV_PROTOCOL_VERSION = 1, /* SharedConstants::NetworkProtocolVersion → string */
};

/**
 * Function table handed to the mod at load time.
 * Pointer remains valid for the whole lifetime of the mod.
 */
/*
 * ⚠ 加新槽之前：**先把现有的槽名读一遍。**
 *
 * 这不是客套话。最近两轮各犯了一次同样的错：
 *
 *   加 `level_actors_in_box` —— 而 `list_actors` 早就能按维度列出实体
 *   加 `actor_despawn` / `actor_set_health` —— 而 `actor_action` 的
 *       `AACT_DESPAWN` / `AACT_HEAL` 早就做了同样的事，SDK 侧连
 *       `Entity::despawn()` / `Entity::heal()` 都封装好了
 *
 * 两次都是编译期才发现（重复定义），而如果名字恰好不冲突，
 * 它们会一直并存 —— 直到某天两份实现分岔，而
 * 「为什么这里删得掉那里删不掉」是个没人答得上的问题。
 *
 * 尤其要先查这三个"什么都能干"的槽，它们覆盖面很宽：
 *
 *   actor_action     删除、治疗、点燃、传送、加效果…（见 PIER_AACT_*）
 *   actor_get_num    坐标、血量、朝向、各种数值（见 PIER_APROP_*）
 *   list_actors      按维度列出全部实体，带类型名
 *
 * 试过一次自动检查（按词根找重复），噪音大到没法用 —— `actor_get_*` 会
 * 两两配对报一屏。所以这里靠这段话，而不是靠脚本。
 */
/** legacymoney 事件类型（服务端经济能力组用）。 */
typedef enum PierMoneyEvent
{
    PIER_MONEY_SET = 0,
    PIER_MONEY_ADD = 1,
    PIER_MONEY_REDUCE = 2,
    PIER_MONEY_TRANS = 3
} PierMoneyEvent;

/** legacymoney 事件回调。返回 false 否决这次金额变动。 */
typedef bool (*PierMoneyCb)(PierMoneyEvent type, PierStr from, PierStr to, int64_t value);

typedef struct PierApi
{
    /** sizeof(PierApi)，由宿主按自己编译出的表填写。前向兼容的全部依据：
     *  SDK 在每个非核心槽的调用点比对它（require_slot!）。 */
    uint32_t struct_size;
    /** == 宿主的 PIER_ABI_VERSION。 */
    uint32_t abi_version;
    /** PIER_FLAG_* 的按位或。bit 0 = 客户端构建。 */
    uint32_t host_flags;
    /** 保留，恒为 0。凑齐 16 字节头，也给未来的头部标量留位。 */
    uint32_t _reserved0;

    /**
     * Log a message through the mod's own LeviLamina logger.
     * level: -1=Off, 0=Fatal, 1=Error, 2=Warn, 3=Info, 4=Debug, 5=Trace
     * (mirrors ll::io::LogLevel). Thread-safe.
     */
    void (*log)(PierModHandle mod, int32_t level, PierStr msg);

    /**
     * Current gaming status: 0=Default, 1=Starting, 2=Running, 3=Stopping
     * (mirrors ll::GamingStatus). Thread-safe.
     */
    int32_t (*gaming_status)();

    /** Queue a task onto the server thread ASAP. Thread-safe. */
    void (*schedule)(PierTaskCb cb, void* user);

    /** Queue a task onto the server thread after `delay_ms`. Thread-safe. */
    void (*schedule_after)(PierTaskCb cb, void* user, uint64_t delay_ms);

    /**
     * Subscribe to a LeviLamina event by id (server thread only).
     *   event_id : full id, e.g. "ll::event::PlayerChatEvent". If no exact
     *              match exists, the loader falls back to a unique suffix
     *              match ("PlayerChatEvent" works if unambiguous).
     *   priority : 0..4 (Highest..Lowest), 2 = Normal
     *              (mirrors ll::event::EventPriority).
     * Returns NULL on failure (unknown/ambiguous id).
     */
    PierListenerHandle (*subscribe_event)(
        PierModHandle mod,
        PierStr event_id,
        int32_t priority,
        PierEventCb cb,
        void* user
    );

    /** Remove a listener previously returned by subscribe_event. Server thread only. */
    bool (*unsubscribe_event)(PierModHandle mod, PierListenerHandle listener);

    /** Enumerate all currently registered event ids. Server thread only. */
    void (*list_events)(void* ctx, PierStrSink sink);

    /**
     * Execute a command as the server console (permission: Owner) and collect
     * its output. Server thread only. Returns false if the level is not ready.
     */
    bool (*execute_command)(PierStr cmd, void* ctx, PierCmdOutputSink sink);

    /**
     * Register a custom command `/name [args: raw text]`.
     *   permission: 0=Any,1=GameDirectors,2=Admin,3=Host,4=Owner
     *               (mirrors CommandPermissionLevel).
     * Call during on_enable, on the server thread. The command stays
     * registered for the lifetime of the server (Bedrock cannot unregister
     * commands); callbacks for disabled mods are muted by the loader.
     */
    bool (*register_command)(
        PierModHandle mod,
        PierStr name,
        PierStr description,
        int32_t permission,
        PierCommandCb cb,
        void* user
    );

    /**
     * Current server tick (the tickID from Level::getCurrentTick()).
     * Returns 0 when the level is not ready. Server thread only.
     */
    uint64_t (*get_current_tick)();

    /**
     * Seconds taken by the last tick (mTickDeltaTime; 0.05 at 20 TPS).
     * TPS = 1.0 / tick_delta_time when > 0. Returns -1.0 if unavailable.
     * Server thread only.
     */
    double (*get_tick_delta_time)();

    /**
     * Number of currently connected players
     * (Level::getActivePlayerCount()). Server thread only.
     */
    int32_t (*get_player_count)();

    /**
     * Whether the simulation is currently paused
     * (Level::getSimPaused()). Server thread only.
     */
    bool (*get_sim_paused)();

    /* ── 追加 ── */

    /**
     * Spawn a particle effect at a world coordinate. Used to outline a
     * selection box edge-by-edge. Server thread only. Returns false if the
     * level/dimension is not ready.
     *   dimension   : 0 = overworld, 1 = nether, 2 = the end.
     *   effect_name : e.g. "minecraft:basic_flame_particle" / "minecraft:redstone_wire_dust_particle".
     */
    bool (*spawn_particle)(int32_t dimension, PierStr effect_name, double x, double y, double z);

    /**
     * Look up a connected player's feet position and dimension by name.
     * Used to pick selection corners from where the player is standing.
     * Server thread only.
     */
    PierPlayerPos (*get_player_position)(PierStr name);

    /**
     * Scan a cuboid region, corners inclusive (order-independent). For every
     * cell in the box, blocks_sink is called with the block name + full SNBT.
     * For every entity whose position lies within the box, entities_sink is
     * called with the containing cell and the entity's SNBT. Both sinks run
     * synchronously within this call; nothing is retained afterwards.
     * Server thread only. Returns false if the level/dimension is not ready.
     */
    bool (*scan_region)(
        int32_t dimension,
        int32_t x1,
        int32_t y1,
        int32_t z1,
        int32_t x2,
        int32_t y2,
        int32_t z2,
        void* ctx,
        PierBlockSink blocks_sink,
        PierEntitySink entities_sink
    );


    /* ═════════════════ 追加区 —— 只追加，不重排 ═════════════════
     * Everything below: SERVER THREAD ONLY unless noted. All calls return
     * false / do nothing while the level is not ready. Unknown enum keys
     * return false (forward-compat negotiation).                        */

    /* ── §A world read/write & clock ── */

    /** Read one block: sink called once with (x,y,z, type name, full SNBT). */
    bool (*get_block)(int32_t dim, int32_t x, int32_t y, int32_t z, void* ctx, PierBlockSink sink);
    /** Place a block via /setblock (version-stable path). block_spec = id or id [states]. */
    bool (*set_block)(int32_t dim, int32_t x, int32_t y, int32_t z, PierStr block_spec);
    /** World time (Level::getTime). */
    bool (*get_time)(int64_t* out);
    /** Set world time via /time set. */
    bool (*set_time)(int64_t t);
    /** 0=clear 1=rain 2=thunder, via /weather. */
    bool (*set_weather)(int32_t weather);

    /* ── §B player management ── */

    /** One SNBT per online player: {name,xuid,uuid,dim,x,y,z}. */
    void (*list_players)(void* ctx, PierStrSink snbt_sink);
    /** Resolve a player selector to their ActorUniqueID (bridges into the actor_* API). */
    bool (*player_resolve)(PierPlayerSel sel, PierActorId* out);
    bool (*player_send_message)(PierPlayerSel sel, PierStr msg);
    bool (*player_disconnect)(PierPlayerSel sel, PierStr reason);
    /** sendMessage to every online player. */
    void (*broadcast_message)(PierStr msg);
    /** 0=survival 1=creative 2=adventure 6=spectator, via /gamemode. */
    bool (*player_set_gamemode)(PierPlayerSel sel, int32_t mode);
    /** Teleport via /execute in <dim> run tp. */
    bool (*player_teleport)(PierPlayerSel sel, int32_t dim, double x, double y, double z);
    bool (*player_get_num)(PierPlayerSel sel, int32_t prop, double* out);
    bool (*player_get_str)(PierPlayerSel sel, int32_t prop, void* ctx, PierStrSink sink);
    bool (*player_set_num)(PierPlayerSel sel, int32_t prop, double v);
    bool (*player_action)(
        PierPlayerSel sel,
        int32_t action,
        PierStr sarg,
        double a,
        double b,
        double c,
        void* ctx,
        PierStrSink out
    );

    /* ── §C actors (players resolve here too, via player_resolve) ── */

    /** Enumerate live actors; dim = -1 for all dimensions. */
    void (*list_actors)(int32_t dim, void* ctx, PierActorSink sink);
    /** Full Actor::save NBT as SNBT. */
    bool (*actor_snapshot)(PierActorId id, void* ctx, PierStrSink snbt_sink);
    bool (*actor_get_num)(PierActorId id, int32_t prop, double* out);
    bool (*actor_get_str)(PierActorId id, int32_t prop, void* ctx, PierStrSink sink);
    bool (*actor_action)(
        PierActorId id,
        int32_t action,
        PierStr sarg,
        double a,
        double b,
        double c,
        void* ctx,
        PierStrSink out
    );
    /** Spawn a mob (Spawner::spawnMob); on success *out = its ActorUniqueID. */
    bool (*spawn_mob)(int32_t dim, PierStr type_name, double x, double y, double z, PierActorId* out);
    /** Level::explode. source may be 0 (no source actor). */
    bool (*explode)(
        int32_t dim,
        double x,
        double y,
        double z,
        float radius,
        float max_resistance,
        PierActorId source,
        bool fire,
        bool breaks_blocks,
        bool allow_underwater
    );

    /* ── §D blocks & block entities ── */

    bool (*block_get_num)(int32_t dim, int32_t x, int32_t y, int32_t z, int32_t prop, double* out);
    bool (*block_get_str)(int32_t dim, int32_t x, int32_t y, int32_t z, int32_t prop, void* ctx, PierStrSink sink);
    bool (*block_action)(
        int32_t dim,
        int32_t x,
        int32_t y,
        int32_t z,
        int32_t action,
        PierStr sarg,
        void* ctx,
        PierStrSink out
    );
    /** BlockActor::save (with default SaveContext) as SNBT; false if none there. */
    bool (*block_entity_snbt)(int32_t dim, int32_t x, int32_t y, int32_t z, void* ctx, PierStrSink sink);

    /* ── §E items (SNBT value objects) & containers ── */

    bool (*item_get_num)(PierStr item_snbt, int32_t prop, double* out);
    bool (*item_get_str)(PierStr item_snbt, int32_t prop, void* ctx, PierStrSink sink);
    /** Rebuild → mutate → serialize; out receives the NEW item SNBT. */
    bool (*item_transform)(PierStr item_snbt, int32_t op, PierStr sarg, double narg, void* ctx, PierStrSink out);
    bool (*container_size)(PierContainerRef ref, int32_t* out);
    /** Slot content as item SNBT (empty slots yield the air item's SNBT). */
    bool (*container_get_item)(PierContainerRef ref, int32_t slot, void* ctx, PierStrSink sink);
    bool (*container_set_item)(PierContainerRef ref, int32_t slot, PierStr item_snbt);
    bool (*container_add_item)(PierContainerRef ref, PierStr item_snbt);
    bool (*container_remove_item)(PierContainerRef ref, int32_t slot, int32_t count);
    bool (*container_clear)(PierContainerRef ref);

    /* ── §F scoreboard ── */

    bool (*scoreboard_op)(int32_t op, PierStr a, PierStr b, int64_t n, void* ctx, PierStrSink out);

    /* ── §G forms (async result callback) ── */

    /**
     * kind: 0=SimpleForm 1=CustomForm 2=ModalForm. form_snbt describes the
     * form (see docs/api/gui). The callback fires once, on the server thread,
     * and is muted if the mod is disabled before the player responds.
     */
    bool (*form_send)(
        PierModHandle mod,
        PierPlayerSel sel,
        int32_t kind,
        PierStr form_snbt,
        PierFormResultCb cb,
        void* user
    );

    /* ── §H parameterized commands & enums ── */

    /**
     * Like register_command, but with typed overloads. overloads_snbt:
     *   {overloads:[[{name:"target",kind:"player",optional:0b}, …], …]}
     * kinds: int|bool|float|string|enum|soft_enum|actor|player|block_pos|vec3|
     *        raw_text|message|json|item|block_name|effect|actor_type|command|
     *        relative_float|file_path (enum/soft_enum also need "enum":"Name").
     * The callback's `args` receives the parse result as SNBT
     *   {overload:N, args:{<name>: …}}   and `origin_name` becomes origin SNBT
     *   {name,type,dim,x,y,z}.
     */
    bool (*register_command_ex)(
        PierModHandle mod,
        PierStr name,
        PierStr description,
        int32_t permission,
        PierStr overloads_snbt,
        PierCommandCb cb,
        void* user
    );
    /** values_snbt = {values:[["name",1L], …]}  → tryRegisterRuntimeEnum. */
    bool (*register_command_enum)(PierStr name, PierStr values_snbt);
    /** values_snbt = {values:["a","b"]}         → tryRegisterSoftEnum. */
    bool (*register_command_soft_enum)(PierStr name, PierStr values_snbt);
    /** op: 0=set 1=add 2=remove. */
    bool (*update_command_soft_enum)(PierStr name, int32_t op, PierStr values_snbt);

    /* ── §I NBT binary, KvDb (thread-safe), system & server info ── */

    /** fmt: 0=disk little-endian, 1=network. */
    bool (*nbt_snbt_to_binary)(PierStr snbt, int32_t fmt, void* ctx, PierBytesSink sink);
    bool (*nbt_binary_to_snbt)(uint8_t const* data, size_t len, int32_t fmt, void* ctx, PierStrSink sink);

    /* KvDb: THREAD-SAFE (internal mutex). Paths are confined to the mod's
     * own data directory; ".." and absolute paths are rejected. Handles are
     * owned by the loader and force-closed (with a warning) at mod unload. */
    PierKvDbHandle (*kvdb_open)(PierModHandle mod, PierStr path, bool create_if_missing);
    void (*kvdb_close)(PierKvDbHandle h);
    bool (*kvdb_get)(PierKvDbHandle h, PierStr key, void* ctx, PierStrSink sink);
    bool (*kvdb_set)(PierKvDbHandle h, PierStr key, PierStr value);
    bool (*kvdb_del)(PierKvDbHandle h, PierStr key);
    bool (*kvdb_has)(PierKvDbHandle h, PierStr key);
    bool (*kvdb_is_empty)(PierKvDbHandle h);
    void (*kvdb_iter)(PierKvDbHandle h, void* ctx, PierKvSink sink);

    /* System info: THREAD-SAFE (plain OS calls). */
    bool (*sys_info_str)(int32_t prop, void* ctx, PierStrSink sink);
    bool (*sys_get_env)(PierStr name, void* ctx, PierStrSink sink);
    bool (*sys_set_env)(PierStr name, PierStr value);
    bool (*sys_is_wine)();

    /* Server / world-level settings. */
    bool (*get_difficulty)(int32_t* out); /* Level::getDifficulty */
    bool (*set_difficulty)(int32_t d); /* /difficulty */
    bool (*get_seed)(int64_t* out); /* Level::getLevelSeed64 */
    /** out sink receives SNBT {type:"bool"|"int"|"float", value:…}; false if unknown rule. */
    bool (*game_rule_get)(PierStr name, void* ctx, PierStrSink sink);
    bool (*game_rule_set)(PierStr name, PierStr value); /* /gamerule */
    bool (*server_info_str)(int32_t prop, void* ctx, PierStrSink sink);

    /*
     * Per-player particle packet (additive, gated by struct_size).
     * Sends a SpawnParticleEffectPacket ONLY to the resolved player
     * (Player::sendNetworkPacket) instead of Level::spawnParticleEffect's
     * dimension-wide broadcast — other clients never receive it.
     * `dimension` is the vanilla dimension id carried in the packet; pass the
     * dimension the coordinates refer to (normally the player's own — clients
     * don't render particles for another dimension).
     * False if the player is offline / can't be resolved.
     */
    bool (*spawn_particle_for)(
        PierPlayerSel sel, int32_t dimension, PierStr effect_name, double x, double y, double z);

    /*
     * Raw per-connection packet send (additive, gated by struct_size) — the
     * generic primitive spawn_particle_for derives from.
     * `packet_id` is a MinecraftPacketIds value; `body`/`body_len` is the
     * packet's wire-format body for the CURRENT game version. The bridge
     * deserialises it into a real packet object (MinecraftPackets::createPacket
     * + Packet::read) and delivers it to the resolved player's connection only.
     * False if: player offline, unknown/unconstructible id, body fails to
     * parse, or bytes are left over after parsing (wrong shape for this
     * version). ESCAPE HATCH: the wire format is version-specific and is the
     * caller's responsibility; prefer typed entries when one exists.
     */
    bool (*send_packet)(PierPlayerSel sel, int32_t packet_id, uint8_t const* body, size_t body_len);

    /*
     * Tick control (additive, gated by struct_size). Backed by a bridge-owned
     * detour on Level::tick, installed lazily on the first control call and
     * left in place (idle cost: one predictable branch per frame — a control
     * call can arrive from a command handler that is executing INSIDE the
     * tick, where unpatching would not be safe). Server thread only.
     * While frozen, mobs/blocks/redstone/time stop; players can still move
     * and chat (movement is client-authoritative, network runs outside the
     * level tick).
     */
    bool (*tick_freeze)(bool on);
    /** Only while frozen: queue exactly n extra frames. False if not frozen or n == 0. */
    bool (*tick_step)(uint32_t n);
    /** 0 < factor <= 100. Fractional = slow motion (accumulator), 1.0 restores normal. */
    bool (*tick_warp)(double factor);

    /*
     * Per-subsystem MSPT profiler (additive, gated by struct_size). Backed by
     * five timing detours (Level/Dimension tick, redstone, chunk block ticks,
     * block entities), installed lazily on the first profile_begin and left
     * in place. One sampling window at a time. Server thread only.
     */
    /** Arm a window of `ticks` level ticks (1..12000). False if 0, too big, or already sampling. */
    bool (*profile_begin)(uint32_t ticks);
    /**
     * Poll for the finished report. False while sampling / nothing armed;
     * true exactly once per window, sinking one SNBT report:
     * {ticks:N, buckets:{level_tick:{us,calls}, dimension_tick:{…}, redstone:{…},
     *  chunk_blocks:{…}, block_entities:{…}}}. Bucket times are INCLUSIVE
     * (nested subsystems), report side by side, don't sum.
     */
    bool (*profile_take)(void* ctx, PierStrSink sink);

    /*
     * Simulated ("fake") players (additive, gated by struct_size).
     * sim_spawn creates a real ServerPlayer with that name — every existing
     * per-player entry (teleport, health, inventory, kick, …) works on it via
     * the usual name selector. sim_do multiplexes the simulate* verb family:
     * the action vocabulary grows bridge-side without new table slots
     * (verbs: despawn stop jump attack interact use_item drop respawn
     * move_to navigate_to look_at destroy_block destroy_look stop_destroy
     * interact_block sneak fly chat — args as SNBT, see docs). Gated on
     * isSimulatedPlayer(): a real player can never be puppeted. False on
     * unknown verb, malformed args, offline/non-sim target.
     */
    bool (*sim_spawn)(PierStr name, int32_t dimension, double x, double y, double z);
    bool (*sim_do)(PierPlayerSel sel, PierStr action, PierStr args_snbt);
    /** True if the selector resolves to a live simulated player. Lets a mod
     *  re-validate a bot after a restart (the SimulatedPlayer persists in the
     *  world, but in-memory handles don't). */
    bool (*sim_is)(PierPlayerSel sel);
    /** Enumerate the names of all live simulated players (sink receives each
     *  name). Rebuild a handle from a name to drive a bot that outlived the
     *  session that spawned it. */
    void (*sim_list)(void* ctx, PierStrSink name_sink);

    /*
     * Read-only world-data queries (additive, gated by struct_size). Both
     * stream one SNBT object per result through the sink; observational only.
     * Server thread only.
     */
    /** Enumerate villages in a dimension. Each: {uuid, center:[x,y,z],
     *  bounds:{min,max}, poi_count}. */
    void (*villages)(int32_t dimension, void* ctx, PierStrSink snbt_sink);
    /** Hardcoded spawn areas (nether fortress / witch hut / ocean monument /
     *  pillager outpost) whose chunks intersect a radius around (x,y,z). Each:
     *  {type, bounds:{min,max}}. Only LOADED chunks are inspected — a
     *  read-only query never force-loads. */
    void (*structures_near)(
        int32_t dimension, int32_t x, int32_t y, int32_t z, int32_t radius, void* ctx,
        PierStrSink snbt_sink);

    /*
     * Send a message of a specific TextPacketType to one player (additive,
     * gated by struct_size). `type` is a TextPacketType value:
     *   0 Raw · 1 Chat · 2 Translate · 3 Popup · 4 JukeboxPopup · 5 Tip ·
     *   6 SystemMessage · 7 Whisper · 8 Announcement · 9 TextObjectWhisper ·
     *   10 TextObject · 11 TextObjectAnnouncement.
     * Out-of-range falls back to Raw. Single-string body (like LSE tell): the
     * author/param kinds (Chat/Whisper/Translate) arrive as plain text.
     * plain `player_send_message` remains the Raw/Chat convenience path.
     */
    bool (*player_send_message_typed)(PierPlayerSel sel, PierStr msg, int32_t type);

    /* —— Money (追加) —— */
    int64_t (*get_money)(PierStr xuid);
    bool (*set_money)(PierStr xuid, int64_t money);
    bool (*add_money)(PierStr xuid, int64_t money);
    bool (*reduce_money)(PierStr xuid, int64_t money);
    bool (*trans_money)(PierStr from, PierStr to, int64_t val, PierStr note);
    void (*money_get_hist)(PierStr xuid, int32_t timediff, void* ctx, PierStrSink sink);
    void (*money_clear_hist)(int32_t difftime);
    void (*money_listen_before_event)(PierMoneyCb callback);
    void (*money_listen_after_event)(PierMoneyCb callback);
    void (*money_ranking)(uint16_t num, void* ctx, PierStrSink sink);

    /* ═════════════════ 追加 —— API 补齐（struct_size 把关） ═════════════════
     * All entries below are additive: older loaders (smaller struct_size)
     * simply won't have these fields. The SDK's init-time check rejects
     * mods built against a larger table than the loader provides. Unknown enum
     * keys return false. SERVER THREAD ONLY unless noted.                    */

    /* ── Player: equipment, cooldown, network (dedicated fns) ── */
    bool (*player_get_carried_item)(PierPlayerSel sel, void* ctx, PierStrSink sink);
    bool (*player_get_item)(PierPlayerSel sel, int32_t slot, void* ctx, PierStrSink sink);
    bool (*player_set_item)(PierPlayerSel sel, int32_t slot, PierStr item_snbt);
    /** All equipment as SNBT: [{slot, item_snbt}, …] slot: 0=mainhand 1=offhand 2-5=armor */
    bool (*player_get_equipment)(PierPlayerSel sel, void* ctx, PierStrSink sink);
    /** Ticks remaining for an item cooldown (-1 if not on cooldown / player offline). */
    int32_t (*player_get_cooldown)(PierPlayerSel sel, PierStr item_name);
    bool (*player_start_cooldown)(PierPlayerSel sel, PierStr item_name, int32_t ticks);
    bool (*player_get_network_status)(PierPlayerSel sel, void* ctx, PierStrSink sink);

    /* ── Actor: relationships, equipment, effects, geometry (dedicated fns) ── */
    bool (*actor_get_vehicle)(PierActorId id, PierActorId* out);
    bool (*actor_get_first_passenger)(PierActorId id, PierActorId* out);
    bool (*actor_get_owner)(PierActorId id, PierActorId* out);
    bool (*actor_get_target)(PierActorId id, PierActorId* out);
    /** slot: 0=mainhand 1=offhand 2=helmet 3=chestplate 4=leggings 5=boots */
    bool (*actor_get_equipped_item)(PierActorId id, int32_t slot, void* ctx, PierStrSink sink);
    bool (*actor_set_equipped_item)(PierActorId id, int32_t slot, PierStr item_snbt);
    /** SNBT [{id, ticks, amplifier, visible}, …] */
    bool (*actor_get_effects)(PierActorId id, void* ctx, PierStrSink sink);
    /** flag_index: ActorFlags enum value (0-based). */
    bool (*actor_get_status_flag)(PierActorId id, int32_t flag_index);
    bool (*actor_set_status_flag)(PierActorId id, int32_t flag_index, bool value);
    /** SNBT {type:"entity"|"block"|"none", pos:[x,y,z], entity_id?, block_name?} */
    bool (*actor_trace_ray)(PierActorId id, float max_dist, bool include_actors, bool include_blocks, void* ctx,
                            PierStrSink sink);
    bool (*actor_distance_to)(PierActorId id, PierActorId other, double* out);
    /** SNBT {min:[x,y,z], max:[x,y,z]} */
    bool (*actor_get_aabb)(PierActorId id, void* ctx, PierStrSink sink);
    bool (*actor_clone)(PierActorId id, int32_t dim, double x, double y, double z, PierActorId* out);

    /* ── Block: state get/set, collision shape (dedicated fns) ── */
    bool (*block_get_state)(int32_t dim, int32_t x, int32_t y, int32_t z, PierStr state_name, void* ctx,
                            PierStrSink sink);
    bool (*block_set_state)(int32_t dim, int32_t x, int32_t y, int32_t z, PierStr state_name, PierStr value);
    bool (*block_get_collision_shape)(int32_t dim, int32_t x, int32_t y, int32_t z, void* ctx, PierStrSink sink);

    /* ── Item: enchants, matching, NBT (dedicated fns) ── */
    /** SNBT [{id, level}, …] */
    bool (*item_get_enchants)(PierStr item_snbt, void* ctx, PierStrSink sink);
    /** enchants_snbt = [{id, level}, …]; out = new item SNBT. */
    bool (*item_set_enchants)(PierStr item_snbt, PierStr enchants_snbt, void* ctx, PierStrSink out);
    bool (*item_matches)(PierStr a, PierStr b);
    bool (*item_get_user_data)(PierStr item_snbt, void* ctx, PierStrSink sink);

    /* ── Level: biome, spawn, save, weather, path, sleep (dedicated fns) ── */
    bool (*level_get_biome)(int32_t dim, int32_t x, int32_t y, int32_t z, void* ctx, PierStrSink sink);
    bool (*level_get_default_spawn)(int32_t* x, int32_t* y, int32_t* z);
    bool (*level_set_default_spawn)(int32_t x, int32_t y, int32_t z);
    bool (*level_save)();
    /** SNBT {sleeping, total_players, active_sleeping} */
    bool (*level_get_sleep_status)(void* ctx, PierStrSink sink);
    bool (*level_update_weather)(float rain_level, int32_t rain_time, float lightning_level, int32_t lightning_time);
    /** SNBT {nodes:[{x,y,z}, …], reached:1b/0b} */
    bool (*level_find_path)(PierActorId id, int32_t x, int32_t y, int32_t z, void* ctx, PierStrSink sink);

    /* ═════════════════ 数据包拦截（追加，struct_size 把关） ═════════════════
     * Raw wire-format interception in both directions. This is the primitive
     * `send_packet` could not provide: it observes and rewrites bytes that
     * already exist, instead of manufacturing new ones.
     *
     * Delivery unit is exactly ONE packet — the leading unsigned-varint
     * header followed by the packet body. Batching and compression live
     * further down the peer chain (BatchedNetworkPeer splits inbound batches
     * and re-batches outbound ones), so a callback never sees a batch and
     * never has to produce a length prefix.
     *
     * The bridge decodes the header: `packet_id` is its low 10 bits,
     * `sender_sub_id` / `target_sub_id` the two 2-bit fields above it, and
     * `body`/`body_len` point PAST the header. A REPLACE verdict supplies a
     * new BODY only; the bridge re-encodes the header from `edit`, so a
     * rewrite never reproduces varint framing and packet-id remapping is a
     * field assignment rather than a byte-surgery exercise.
     *
     * Dispatch chains: with several subscribers, each one sees the output of
     * the previous, in registration order. The first DROP wins and the rest
     * are skipped. Subscriber lists are snapshotted before dispatch, so a
     * callback may register or unregister (including itself) safely.
     *
     * Threading — read this before touching game state. Inbound callbacks run
     * wherever the connection is pumped and outbound ones wherever the send
     * originates. In practice that is the server thread, but async flush
     * means it is not guaranteed. Treat these as "not necessarily the game
     * thread": keep them short, guard your own state, and route anything that
     * touches the world through `schedule`.
     *
     * Detours install lazily on the first subscriber and are never unpatched
     * (an unsubscribe can arrive from inside the hooked function). With no
     * subscribers the hook bodies fast-path straight to origin. */

    /**
     * Register a raw packet interceptor.
     * `dir_mask` is PIER_PKT_MASK_INBOUND | PIER_PKT_MASK_OUTBOUND (a
     * zero mask registers nothing and returns NULL). Returns NULL on failure.
     */
    PierPacketHookHandle (*packet_hook_register)(
        PierModHandle mod,
        int32_t dir_mask,
        PierPacketCb cb,
        void* user
    );

    /** Unregister. Safe to call from inside the callback. */
    bool (*packet_hook_unregister)(PierModHandle mod, PierPacketHookHandle handle);

    /**
     * Register a connection open/close observer. Returns NULL on failure.
     * The close notification is the only reliable signal for dropping
     * per-connection state: a connection that never finishes the login
     * handshake never becomes a Player, so no player event covers it.
     */
    PierPacketHookHandle (*packet_conn_hook_register)(PierModHandle mod, PierConnCb cb, void* user);

    /** Unregister. Safe to call from inside the callback. */
    bool (*packet_conn_hook_unregister)(PierModHandle mod, PierPacketHookHandle handle);

    /* 以下是两个**能力组**（客户端、维度）。它们无条件存在于布局中 ——
     * 能力包没编进宿主时对应槽位为 NULL（见文件头）。追加新槽仍然只去
     * 结构体真正的末尾，不插进能力组内部。 */

    /* ── 能力组：客户端（client_*）。服务端宿主全为 NULL。
     * 所有回调在**客户端线程**触发。 ── */
    /** Local player's name via ll::service::getClientInstance()->getLocalPlayer().
     *  sink receives the name, or the call returns false if not in a level. */
    bool (*client_get_local_player)(void* ctx, PierStrSink sink);

    /** True when the client is inside a level (a world is loaded). */
    bool (*client_is_in_level)();

    /** Current screen / UI name (e.g. "hud_screen", "pause_screen"). */
    bool (*client_get_screen_name)(void* ctx, PierStrSink sink);

    /** Register a key binding via ll::input::KeyRegistry::getOrCreateKey.
     *  Returns NULL on failure. The handle is owned by the caller; drop with
     *  client_unregister_key. down_cb/up_cb fire on the client thread. */
    PierKeyHandle (*client_register_key)(
        PierModHandle mod,
        PierStr name,
        int32_t const* key_codes,
        int32_t key_count,
        bool allow_remap,
        PierKeyCb down_cb,
        PierKeyCb up_cb,
        void* user
    );

    /** Unregister a key binding (destroys the ll::input::KeyHandle). */
    bool (*client_unregister_key)(PierKeyHandle handle);

    /** Currently assigned key codes (may differ from defaults if remapped).
     *  sink receives a JSON-style array string "[1,2,3]". */
    bool (*client_get_key_codes)(PierKeyHandle handle, void* ctx, PierStrSink sink);

    /* ── 能力组：自定义维度（md_*）。pier-dimensions 没编进宿主时全为
     * NULL；探测可用性 = 看 `md_is_available` 槽是否为 NULL（这个槽存在
     * 只为让「探测」这个动作有个名字，它被填上时恒返回 true）。 ── */
    /** Check whether MoreDimensions is available in this loader build. */
    bool (*md_is_available)(void);

    /** Add a SimpleCustomDimension.
     *
     *  generatorType is ::GeneratorType **verbatim** — 1=Overworld, 2=Flat,
     *  3=Nether, 4=TheEnd, 5=Void. (This comment used to claim
     *  "0=Overworld 1=Nether 2=TheEnd 3=Flat 4=Void", which is the numbering
     *  bug that made "superflat" generate a nether. Values outside 1..5 are
     *  rejected rather than silently building a void world.)
     *
     *  Returns dim id (>=3) or -1 on failure. */
    int32_t (*md_add_simple_dimension)(PierStr name, uint32_t seed, int32_t generatorType);

    /** Per-dimension rules, consulted by the loader's own hooks.
     *
     *  Why this exists instead of gamerules: Bedrock gamerules are
     *  server-wide. Setting `doMobSpawning=false` to quiet a creative plot
     *  world also stops spawning in the survival world. These flags are
     *  checked inside hooks on the actual call sites (Spawner::spawnMob,
     *  Level::explode, ...), so they really are per-dimension.
     *
     *  `rule` is one of PierDimRule. Setting a rule on a dimension the
     *  loader doesn't know about is harmless — the tables are keyed by raw
     *  dimension id and consulted only when that id shows up in a hook.
     *
     *  Dimensions with no entry are left completely alone: the hooks fall
     *  through to origin(), so vanilla dimensions keep vanilla behaviour
     *  without the caller having to opt out. */
    void (*md_set_dimension_rule)(int32_t dimension, int32_t rule, bool allow);

    /** Read back a rule. `outAllow` is only written when the dimension has an
     *  explicit entry for that rule; returns false otherwise. */
    bool (*md_get_dimension_rule)(int32_t dimension, int32_t rule, bool* outAllow);

    /** Drop every rule for a dimension (used when a world is deleted). */
    void (*md_clear_dimension_rules)(int32_t dimension);

    /** Resolve a dimension name to its id. Returns -1 if not found.
     *
     *  Only returns an id for names that are ACTUALLY registered: unknown
     *  names yield -1, never VanillaDimensions::Undefined() (whose numeric
     *  value is mutated at runtime and looks like a valid id).
     *
     *  Note this is rarely what you want. `md_add_simple_dimension` and
     *  `md_add_plot_dimension` are idempotent — re-registering the same name
     *  on a later boot returns the same persisted id — so callers should
     *  register unconditionally at startup instead of probing first. */
    int32_t (*md_get_dimension_id)(PierStr name);

    /** Add a plot-world dimension: a custom dimension whose chunk generator
     *  produces a plot grid (plots / roads / borders) at generation time,
     *  instead of the caller painting blocks afterwards.
     *
     *  `layout_snbt` is a CompoundTag SNBT string:
     *    {plotSize:64, roadWidth:7, borderWidth:1, floorY:64,
     *     floorBlock:"minecraft:grass_block", fillBlock:"minecraft:dirt",
     *     roadBlock:"minecraft:birch_planks",
     *     borderBlock:"minecraft:stone_block_slab", biome:"minecraft:plains"}
     *  Missing keys fall back to those defaults; all values are clamped to a
     *  safe range on the C++ side. The layout is persisted with the dimension,
     *  so it stays fixed across restarts even if the caller's config changes.
     *
     *  Grid convention (the SDK MUST match): with cell = plotSize +
     *  roadWidth, a column at world (x, z) is road when
     *  mod(x,cell) >= plotSize || mod(z,cell) >= plotSize; otherwise it is
     *  border when within borderWidth of the plot edge; otherwise plot.
     *
     *  Idempotent, like md_add_simple_dimension. Returns dim id (>=3) or -1. */
    int32_t (*md_add_plot_dimension)(PierStr name, uint32_t seed, PierStr layout_snbt);

    /* ═════════════ 追加尾（struct_size 把关）═════════════
     * 结构体唯一的追加点。SDK 镜像无条件声明每一个字段 —— 没有任何
     * cfg / ifdef 分支，因为布局在所有目标下相同。
     *
     * ── Mod-scoped scheduling ──
     * `schedule` / `schedule_after` above take a bare callback with no owner.
     * That is a use-after-free waiting to happen: a mod that schedules a task
     * and is then unloaded leaves the executor holding a function pointer into
     * a freed dylib. These replacements attribute each task to a mod, so the
     * loader can drop still-pending tasks when that mod goes away — the same
     * weak_ptr + ticket discipline the form callbacks already use.
     *
     * The old slots remain (ABI is additive) and still work, but they cannot
     * be made unload-safe: they carry no owner. Mods that want to survive
     * /pier unload or /pier reload must be rebuilt against the current pier SDK
     * that routes through the slots below. */

    /** Run `cb(user)` on the server (or client) thread ASAP, owned by `mod`.
     *  Thread-safe. Returns a task id (>0), or 0 if the task was rejected.
     *  If `mod` unloads before the task runs, the task is dropped and `cb` is
     *  never called — `user` is then leaked by design, because the only code
     *  that could free it lives in the dylib that just went away. */
    uint64_t (*schedule_for)(PierModHandle mod, PierTaskCb cb, void* user);

    /** As above, delayed by `delay_ms`. Thread-safe. Returns a task id (>0),
     *  or 0 if rejected. The timer itself is not cancelled on unload — it
     *  still expires — but the task is dropped when it does, so nothing calls
     *  into the freed dylib. */
    uint64_t (*schedule_after_for)(PierModHandle mod, PierTaskCb cb, void* user, uint64_t delay_ms);

    /** Drop a task scheduled by this mod if it has not run yet. Returns true
     *  if a pending task was actually dropped. Safe to call from any thread
     *  and from inside another task. Cancelling leaks `user` for the same
     *  reason as above, so prefer letting short tasks run. */
    bool (*schedule_cancel)(PierModHandle mod, uint64_t task_id);

    /** Number of tasks this mod still has pending. Intended for a mod to
     *  assert it has drained its own work in on_disable / on_unload, which is
     *  a precondition for being marked "reload_safe" in its manifest. */
    uint32_t (*schedule_pending_count)(PierModHandle mod);

    /* ── Client-side container resync ──
     * `container_set_item` / `_clear` / `_add_item` all write through
     * `Container::setItem`, which mutates the server's copy and sends nothing.
     * The client keeps rendering whatever it last received, so a bulk rewrite
     * (swapping a player's inventory on a cross-dimension teleport, say) looks
     * like it did nothing until the player clicks a slot and forces a resync.
     *
     * Call this once after a batch of writes. Batching matters: this pushes
     * the whole container, so calling it per-slot inside a loop is a packet
     * storm for no benefit. */

    /** Resend a player-owned container (which 0..3) to its owner. Returns
     *  false for block containers (which == 4) — a chest has no single owner
     *  to resend to; its viewers are refreshed by the engine's own container
     *  transaction path. */
    bool (*container_refresh)(PierContainerRef ref);

    /* ── Titles ──
     * `PACT_SET_TITLE` (player_action opcode 6) reaches the client by running
     * the console command `title "<name>" title <text>`. Three things are
     * wrong with that and none of them are theoretical:
     *   - the text is pasted into a command line unquoted, so a plot named
     *     `He said "hi"` truncates the command;
     *   - `title`'s text parameter is a `message`, which expands selectors —
     *     a plot named `@e` is a command injection, not a name;
     *   - `/title` has no way to set fade/stay for the same call, so timing is
     *     whatever the client last stored.
     * This slot builds a real SetTitlePacket instead. No wire format crosses
     * the FFI (the packet is constructed field-by-field on this side), so it
     * survives protocol bumps the way `spawn_particle_for` does.
     *
     * `type` is SetTitlePacketPayload::TitleType:
     *   0 Clear · 1 Reset · 2 Title · 3 Subtitle · 4 Actionbar · 5 Times
     * The TextObject variants (6..8) need a ResolvedTextObject and are refused.
     * `text` is ignored for Clear/Reset/Times.
     *
     * Durations are in TICKS. For 2/3/4, when all three are >= 0 a Times
     * packet is sent first so the timing is deterministic rather than
     * inherited from whatever the client last stored; pass -1 for all three to
     * keep the client's current timing. Mixing (-1 with >=0) is refused rather
     * than guessed at — a half-specified duration set has no sane meaning.
     * Server thread only. */
    bool (*player_send_title)(
        PierPlayerSel sel, int32_t type, PierStr text, int32_t fade_in_ticks,
        int32_t stay_ticks, int32_t fade_out_ticks);

    /* ── Cross-mod event bus ──
     * See PierBusCb above for why the loader owns the table instead of mods
     * exchanging pointers. All four are thread-safe; callbacks run on the
     * publishing thread.
     *
     * A mod does **not** receive its own publishes. Two reasons: a mod that
     * wants to notify itself has a direct function call available, and
     * self-delivery is the one loop shape that no depth limit can distinguish
     * from legitimate work. Cross-mod loops (A publishes → B's handler
     * publishes → A's handler publishes → …) are caught by a depth cap
     * instead; hitting it drops the innermost publish and logs once. */

    /** Subscribe `mod` to `topic`. Returns a subscription id (>0), or 0 if the
     *  topic is empty/oversized, the callback is null, or the mod is unknown.
     *  Subscriptions are dropped automatically when the mod unloads. */
    uint64_t (*bus_subscribe)(PierModHandle mod, PierStr topic, PierBusCb cb, void* user);

    /** Drop one of this mod's subscriptions. Scoped to the caller — a mod
     *  cannot unsubscribe another mod. Returns true if one was removed.
     *  Safe to call from inside a callback (including one's own). */
    bool (*bus_unsubscribe)(PierModHandle mod, uint64_t sub_id);

    /** Deliver `payload` to every *other* mod subscribed to `topic`. Returns
     *  how many subscribers actually ran (0 is normal — nobody is listening).
     *  Return values from subscribers are ignored. */
    uint32_t (*bus_publish)(PierModHandle mod, PierStr topic, PierStr payload);

    /** As above, but collects the veto bit: returns true when **any**
     *  subscriber returned true. Every subscriber still runs — no
     *  short-circuit — so observers see a consistent stream whether or not an
     *  earlier one refused. `out_delivered` may be NULL. */
    bool (*bus_publish_vetoable)(
        PierModHandle mod, PierStr topic, PierStr payload, uint32_t* out_delivered);

    /** How many subscribers a topic has right now, across all mods. Intended
     *  for skipping the cost of building a payload nobody will read. */
    uint32_t (*bus_subscriber_count)(PierStr topic);

    /* ── Plot-boundary confinement ──
     * Backing store for PIER_DIMRULE_PISTON_CROSS_PLOT and
     * PIER_DIMRULE_ENTITY_CROSS_PLOT. Those two rules ask "are these two
     * columns in the same plot?", and the answer needs the grid geometry plus
     * the merge markers. The question is asked from
     * `PistonBlockActor::_checkAttachedBlocks` and `Actor::move` — engine tick
     * paths, hundreds of calls a second — so the data is pushed here once and
     * read natively rather than queried back across the FFI.
     *
     * The ownership rule implemented on the loader side mirrors the plugin's
     * own `owning_plot`: a seam between two merged plots counts as plot, a
     * junction counts as plot only when all four surrounding edges are merged.
     * Divergence does not show up as "one column judged wrong" — it shows up as
     * an owner who can place a block by hand on their merged plot but whose
     * piston refuses to push there. Server thread only. */

    /** Register (or update) the plot grid of a dimension. `plot_size <= 0`
     *  clears it. Values are clamped loader-side — `cell = plot_size +
     *  road_width` is a modulus, and a caller-supplied 0 would divide by zero
     *  in a tick path. Clears the merge table when the geometry changes. */
    void (*md_set_plot_grid)(int32_t dimension, int32_t plot_size, int32_t road_width);

    /** Drop a dimension's grid and merge table (world deleted, or the world
     *  stopped using the plot model). */
    void (*md_clear_plot_grid)(int32_t dimension);

    /** Replace a dimension's merge markers wholesale. `entries` is `count`
     *  triples `(x, z, mask)`, i.e. `count * 3` int32s; `mask` is a bitset of
     *  1=north, 2=east, 4=south, 8=west matching the plugin's `merged[]`
     *  indices. Only plots that actually carry a marker need to be sent.
     *
     *  Wholesale, not incremental: incremental requires both sides to agree
     *  forever on what is currently in the table, and `unlink` clears the
     *  neighbour before storing itself — a failure in between leaves the two
     *  views apart with no way back. Replacing pulls them into agreement on
     *  every push. Call `md_set_plot_grid` first; a push for an unregistered
     *  dimension is dropped with a warning. */
    void (*md_set_plot_merges)(int32_t dimension, int32_t const* entries, int32_t count);

    /* ── Cross-mod service registry (query-style calls) ──
     * The bus is one-way broadcast; this is request/response. The shapes differ
     * on every axis, which is why they are separate tables rather than one:
     *
     *   - providers per name: bus any / service **exactly one**
     *   - nobody registered:  bus normal / service an error the caller handles
     *   - return value:       bus none / service the entire point
     *   - ordering:           bus undefined and must not matter / service n/a
     *
     * Registration is EXCLUSIVE. Two mods answering `plot:can` is not "both
     * run" — it is an ambiguous answer with no way for the caller to pick, so
     * the second registrar is refused loudly. Silent last-wins would make the
     * answer depend on mod load order, which nobody controls and which changes
     * when an unrelated mod is installed.
     *
     * Ownership follows the same weak_ptr + ticket discipline as the bus and
     * the forms: the loader keeps the table, and the call path revalidates the
     * provider immediately before crossing into its dylib.
     *
     * Synchronous, on the caller's thread, no timeout. A provider that blocks
     * blocks the server thread exactly like any other callback; returning
     * "timed out" while the callback kept running would hand the caller a wrong
     * answer AND leave the provider running. */

    /** Register `mod` as the provider of `name`. Returns a registration id
     *  (>0), or 0 if the name is empty/oversized/already taken, the callback is
     *  null, or the mod is unknown. Dropped automatically on unload. */
    uint64_t (*service_register)(
        PierModHandle mod, PierStr name, PierServiceCb cb, void* user);

    /** Drop one of this mod's registrations. Scoped to the caller — a mod
     *  cannot unregister another mod's service. */
    bool (*service_unregister)(PierModHandle mod, uint64_t reg_id);

    /** Call `name` with `request`; the provider's answer arrives through
     *  `reply`. Returns one of PIER_SERVICE_*. A mod cannot call its own
     *  service (it has a direct function call, and self-calls are the least
     *  legible loop shape). */
    int32_t (*service_call)(
        PierModHandle mod, PierStr name, PierStr request, void* ctx, PierStrSink reply);

    /** Every registered service as a JSON array of `{"name":…,"mod":…}`.
     *  For diagnostics and for a caller deciding whether to build a request
     *  nobody can answer. */
    void (*service_list)(void* ctx, PierStrSink sink);

    /* ═════════════════ 批量世界编辑（追加，struct_size 把关） ═════════════════
     * Native write paths that bypass the console-command route used by
     * set_block (`execute in <dim> run setblock …`). With these, block
     * states come from structured NBT instead of command-string splicing,
     * block entities can be written back, and entities can be respawned from
     * saved NBT — all via existing engine entry points.
     *
     * update_flags is a bitmask: 1 = notify neighbours, 2 = sync client,
     * 3 = both (equivalent to /setblock), 0 = neither (fastest for bulk
     * fills, but the caller must resync afterwards). Server thread only. */

    /** Write a block from serialized NBT ({name,states,version}, i.e. the
     *  shape get_block produces). */
    bool (*edit_set_block_nbt)(
        int32_t dim, int32_t x, int32_t y, int32_t z, PierStr snbt, int32_t update_flags);

    /** Write a block from a name + optional partial states. An empty
     *  states_snbt means all-default states; the version is taken from the
     *  default state on the loader side — the caller must not supply one. */
    bool (*edit_set_block_states)(
        int32_t dim, int32_t x, int32_t y, int32_t z, PierStr name, PierStr states_snbt,
        int32_t update_flags);

    /** Write a block entity's NBT back (BlockActor::load). The cell must
     *  already hold the matching block. */
    bool (*edit_set_block_entity)(int32_t dim, int32_t x, int32_t y, int32_t z, PierStr snbt);

    /** Spawn an entity from full NBT (the inverse of actor_snapshot). When
     *  use_pos is true, (x,y,z) overrides the Pos tag; the UniqueID is
     *  reassigned by the engine and returned via out. */
    bool (*edit_spawn_entity_nbt)(
        int32_t dim, PierStr snbt, bool use_pos, double x, double y, double z,
        PierActorId* out);

    /** Ray trace yielding the BLOCK coordinate and hit face:
     *  {type, block:[x,y,z], facing, pos:[x,y,z], entity}. */
    bool (*edit_trace_ray)(
        PierActorId id, float max_dist, bool include_actors, bool include_blocks, void* ctx,
        PierStrSink sink);

    /* ═════════════════ 同工具链快车道 (additive, struct_size-gated) ═════════════════
     * 追加五个槽位，**不动 PIER_ABI_VERSION** —— 按 docs/DESIGN.md §8 第 2 条：
     * 纯追加不算版本变更，`struct_size` 才是精确闸门。
     *
     * 两个方向都成立：
     *   - 新 loader 跑旧 mod：旧 mod 的表是新表的**逐字节前缀**，它够不到这五个
     *     槽，照常工作。
     *   - 新 mod 跑旧 loader：`the SDK runtime init` 比较 `struct_size`，发现 loader 的
     *     表比自己编译时的小，直接拒绝加载。这正是应该发生的事 —— 一个会去读
     *     `lane_publish` 那一格的 mod，在没有那一格的 loader 上只能读到越界内存。
     *
     * 换句话说：版本号管的是「语义变了」，`struct_size` 管的是「表长了」。这次
     * 只有后者。
     * 见文件上方 PierLaneDesc 处的长注释。一句话版本：service 是跨语言的
     * (名字, JSON) -> JSON；这条是「两边由同一次工具链编出」时才成立
     * 的直接函数表调用，指纹对不上就拿不到指针，消费方降级回 service。
     *
     * 全部服务器线程调用。 */

    /** 发布一条车道。**独占**，和 service_register 同一条纪律：名字被占就返回 0
     *  并在日志里点名占用者。返回发布 id (>0)，卸载时自动撤销。 */
    uint64_t (*lane_publish)(PierModHandle mod, PierStr name, PierLaneDesc const* desc);

    /** 撤销自己的一条车道。会替所有未归还的租约补调 `release`，并把存活标志
     *  清零 —— 消费方下一次检查就会看到车道没了，而不是跳进空指针。 */
    bool (*lane_unpublish)(PierModHandle mod, uint64_t pub_id);

    /** 取一条车道。`want_fingerprint` 必须是消费方自己算出来的那个值。
     *
     *  **0 是非法值，一律返回 PIER_LANE_FINGERPRINT。** 它曾经表示「不
     *  校验」并注明「只有诊断工具该这么用」，那是个洞：这个调用给出去的不是
     *  诊断数据，是完整的 vtable + data 裸指针，消费方随后按自己的
     *  `C::Table` 偏移去调它们。跳过校验 = 类型混淆。想看车道有哪些、指纹是
     *  多少，用 `lane_list`，它一个指针都不用给。
     *
     *  返回 PIER_LANE_*。拿到之后必须 `lane_release`，否则提供方的状态一直
     *  被 retain 着。 */
    int32_t (*lane_acquire)(
        PierModHandle mod, PierStr name, uint64_t want_fingerprint, PierLaneRef* out);

    /** 归还一条租约。只能归还自己的。提供方已经走掉时返回 false（那时 loader
     *  已经替你调过 release 了，再调一次就是 double free）。 */
    bool (*lane_release)(PierModHandle mod, uint64_t lease);

    /** 全部车道，JSON 数组：
     *  `[{"name":…,"mod":…,"fingerprint":"0x…","protocol":1,"leases":N,"alive":true}]`。 */
    void (*lane_list)(void* ctx, PierStrSink sink);

    /**
     * 列出**已经注册过的全部自定义维度**，JSON 数组：
     * `[{"name":"plot_world","dim":1000,"snbt":"{…}"}]`。
     *
     * # 为什么必须有这一格
     *
     * 在此之前 `md_*` 只能按名字问（`md_get_dimension_id`）——也就是说
     * **你必须先知道名字才能问**。于是一个接管既有存档的世界管理器完全看不见
     * 前一个插件建的维度：它们在 `dimension_config.json` 里，在引擎里活着，
     * 玩家能传送进去，而管理器的表里一条都没有。
     *
     * 后果不是「少列几个世界」。是那些维度**不受任何规则管辖**，而且新建世界时
     * 引擎分配的号可能和它们撞上——撞上之后两个世界共用一个维度号。
     *
     * 名字来自配置文件的键，`dim` 是引擎分配并持久化的号，`snbt` 是原样的
     * 生成参数（地皮世界是 `{layout:{…},seed:N}`，简单世界是
     * `{generatorType:Flat,seed:N}`），交给调用方自己解释。
     *
     * `md_is_available()` 为假时回调一次都不调。
     */
    void (*md_list_dimensions)(void* ctx, PierStrSink sink);

    /**
     * Delete every save-file key belonging to one chunk, so the engine
     * regenerates it from the generator on next load.
     *
     * # 为什么这一格值得存在
     *
     * 「把一块地恢复原状」用 set_block 逐格写是错的路：一块 32×32 的地皮
     * 乘上世界高度是几十万格，几十万次跨 FFI；而且它**会漏** —— 方块实体、
     * 生物、待办刻（红石、作物生长）都不在方块数据里，逐格写完之后箱子还在、
     * 红石还在跑。
     *
     * 抹存档键没有这两个问题：一次 forEachKeyWithPrefix 拿到这个区块的全部
     * 键（所有 tag、所有子区块、实体、方块实体、待办刻），一次删完，
     * 引擎下次加载时按生成器重新生成。
     *
     * # 键的形状
     *
     * BDS 的区块键前缀是 `<chunkX:i32 LE><chunkZ:i32 LE>`，非主世界再跟一个
     * `<dimension:i32 LE>`。前缀之后是 tag 字节和子区块序号，我们不解释 ——
     * 按前缀全删就是「这个区块的一切」。
     *
     * # ⚠ 区块必须是**未加载**的
     *
     * 加载中的区块在内存里有一份 `LevelChunk`，引擎会在卸载时把它写回去 ——
     * 那会把我们删掉的键原样重建。调用方要负责先让区块卸载（把人传走、
     * 等它离开刻范围），否则这次删除会被静默覆盖。
     *
     * 这一格**不替调用方做这件事**：判断「谁在附近、什么时候能卸载」需要
     * 调用方的业务知识（哪块地是谁的、能不能把人赶走），而这一层不该有。
     *
     * @return 删掉的键数；-1 = 存档层不可用。0 是正常结果（那个区块从没生成过）。
     *
     * 纯追加槽位 —— `PIER_ABI_VERSION` 不变，靠 `struct_size` 把关。
     */
    int32_t (*level_delete_chunk_keys)(int32_t dim, int32_t chunk_x, int32_t chunk_z);

    /**
     * Are the chunks covering [min..max] currently loaded in memory?
     *
     * 配 `level_delete_chunk_keys` 用：抹存档只对**未加载**的区块有效，
     * 加载中的那份在内存里，卸载时会把删掉的键原样写回去 —— 而那次抹除
     * 会「成功」并报出一个正的键数。没有这一格的话，调用方只能靠
     * 「附近没人」去猜，而猜错的表现是静默失效。
     *
     * @return 1 = 全部加载着，0 = 至少有一块没加载，-1 = 维度不可用。
     *
     * 纯追加槽位 —— `PIER_ABI_VERSION` 不变，靠 `struct_size` 把关。
     */
    int32_t (*level_chunks_loaded)(int32_t dim, int32_t min_x, int32_t min_z, int32_t max_x, int32_t max_z);

    /**
     * This player's connection id — the same number packet interceptors see
     * in the packet context.
     *
     * # 为什么需要它
     *
     * 拦包回调里只有 `conn_id`，**没有玩家**。于是任何「按玩家改写发出去的包」
     * 都做不了：改天色要知道这条连接的人在哪个维度，而那要先知道他是谁。
     *
     * 没有这一格的话只剩一条路 —— 自己周期性地发包去盖掉服务器发的那些。
     * 那条路是错的：服务器发真实时间、我们发锁定时间，两种包交替到达，
     * 客户端的天色**一亮一暗地跳**。改写才是对的，而改写需要这一格。
     *
     * @return 连接号；0 = 这个人不在线，或者拿不到他的网络标识。
     *
     * 纯追加槽位 —— `PIER_ABI_VERSION` 不变，靠 `struct_size` 把关。
     */
    uint64_t (*player_conn_id)(PierPlayerSel who);

    /**
     * List every save-file key belonging to one chunk. One callback per key.
     *
     * # 为什么拆成「列」和「删」两格
     *
     * 上一版是一格：C++ 里先把键收进 `std::vector<std::string>`，再逐个删。
     * 那个函数**在真机上崩了** —— 崩在返回时销毁那个 vector，寄存器里能看到
     * 字符串的内联缓冲被当成了堆指针。
     *
     * 根因没定位到（跨 DLL 的 `std::string` 生命周期，没有调试器查不出来）。
     * 但那一整类问题的来源是**在 C++ 侧攒一个 `std::string` 容器并跨一次
     * 虚调用**，所以这一版把容器搬到调用方：C++ 每次只做一件不持有任何东西的事。
     *
     * 键是二进制的（含 0 字节），所以走 `PierStr`（带长度）而不是 C 字符串。
     *
     * @return 报了几个键；-1 = 存档层不可用。
     */
    int32_t (*level_chunk_keys)(int32_t dim, int32_t chunk_x, int32_t chunk_z, void* ctx, PierStrSink sink);

    /**
     * Delete one chunk-category key, verbatim.
     *
     * 配 [`level_chunk_keys`] 用。**不解释键的内容** —— 传什么删什么，
     * 这正是它安全的原因：不需要懂子区块的格式。
     */
    bool (*level_delete_key)(PierStr key);

    /* ── 以下为追加槽（190）。只在表尾加，靠 struct_size 守卫。 ──
     *
     * 删实体和补血**已经有了** —— `actor_action` 的 `AACT_DESPAWN` /
     * `AACT_HEAL`。加独立的槽只是把同一件事做两遍，而两份实现迟早分岔。
     */

    /* 实体枚举**已经有了** —— `list_actors`（按维度列出全部，带类型名），
     * 配 `actor_get_num` 取坐标就能筛出一个盒子里的。加一个
     * `actors_in_box` 只是把同一件事做两遍，而两份实现迟早分岔。
     */

    /**
     * 设一片区域的生物群系。
     *
     * 按整列设（`setBiome3d` 逐 y 生效，但生物群系在基岩版是按列存的），
     * 所以不收 y。`biome` 是生物群系名，如 `"minecraft:plains"`。
     *
     * 返回成功设置了几列。0 表示一列都没设上 —— 区块没加载或者名字不认识。
     */
    int32_t (*level_set_biome)(int32_t dim,
                               int32_t minX, int32_t minZ,
                               int32_t maxX, int32_t maxZ,
                               PierStr biome);

    /* ═══════════ 追加 —— 液体层（含水方块） ═══════════
     *
     * Bedrock 的「含水」不是方块状态，而是**同一格上的第二个方块**：主层放
     * 楼梯/栅栏/珊瑚，液体层放 water。`get_block` / `set_block` 只看主层，所以
     * 复制一片含水的楼梯再粘出来，水会全部消失 —— 主层完全正确，缺的是另一层。
     *
     * 这两个槽位把液体层暴露出来。空的液体层返回 "minecraft:air"。
     */

    /** 读液体层。写进 sink 的是方块名（如 "minecraft:water"），空层为 air。 */
    bool (*get_extra_block)(int32_t dim, int32_t x, int32_t y, int32_t z,
                            void* ctx, PierStrSink sink);

    /** 写液体层。`block_spec` 收裸方块名或完整 SNBT，写 "minecraft:air" 清空。
     *  `update_flags` 同 edit_set_block_nbt：位 1 = 邻居更新，位 2 = 同步客户端。 */
    bool (*set_extra_block)(int32_t dim, int32_t x, int32_t y, int32_t z,
                            PierStr block_spec, int32_t update_flags);
} PierApi;

/**
 * 由模组在 pier_main 里填写。
 * `instance` 是模组自有的不透明指针；三个回调可为 NULL（视为恒成功）。
 * 本结构体遵循与 PierApi 相同的追加规则 —— 有了 struct_size，未来可以在
 * 尾部加新的生命周期回调而不升版本，宿主按「够得到才调」处理。
 */
typedef struct PierModVTable
{
    /** sizeof(PierModVTable)，由模组按自己编译出的定义填写。 */
    uint32_t struct_size;
    /** == 模组编译时的 PIER_ABI_VERSION。 */
    uint32_t abi_version;
    /** PIER_FLAG_* 的按位或。bit 0 = 按客户端目标编译。 */
    uint32_t mod_flags;
    /** 保留，恒为 0。 */
    uint32_t _reserved0;
    void* instance;
    bool (*on_enable)(void* instance);
    bool (*on_disable)(void* instance);
    bool (*on_unload)(void* instance);
} PierModVTable;

/**
 * The single symbol every mod must export:
 *
 *   bool pier_main(const PierApi* api, PierModHandle self,
 *                     PierModVTable* out_vtable);
 *
 * Called once on the server thread while the mod is being loaded.
 * Return false to abort loading.
 */
typedef bool (*PierMainFn)(const PierApi* api, PierModHandle self, PierModVTable* out_vtable);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PIER_SDK_ABI_H */
