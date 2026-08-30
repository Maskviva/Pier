#pragma once
// 宿主的 SPI —— 能力包与宿主之间**唯一**的协作面（契约 §一 规则二）。
//
// 方向永远是「能力包注册进宿主，宿主在正确的时机回调」。宿主不 include、
// 不链接任何能力包的符号；能力包之间也互不认识。一个包不在，注册就不
// 发生，对应槽位保持 NULL —— 这就是「可选」的全部实现机制。
//
// 注册发生在文件级静态对象的构造里（各包 set_kind("object") 保证这些
// 对象必然存在，见契约 §一 规则四）。注册表本体是 Meyers 单例，静态
// 初始化顺序因此无关紧要。所有回调都在服务器线程触发。

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "sdk/abi.h"

namespace ll::io
{
    class Logger;
}

namespace pier
{
    class HostedMod;
}

class BlockSource;

namespace pier::spi
{
    /* ── 1. 槽位包：装载期把函数指针填进 PierApi ─────────────────────
     * 每个能力包注册一个（或几个）填充函数；宿主在 load() 里、任何模组
     * 装载之前统一执行。填充只许写自己域的槽位；表头四个标量由宿主填。 */
    struct SlotPack
    {
        std::string_view name;      // debug 日志用："填充槽位包 core, events, ..."
        void (*fill)(PierApi& api);
    };
    void addSlotPack(SlotPack pack);

    /** 静态注册用：`static spi::SlotPackReg reg{{"core", &fillCore}};` */
    struct SlotPackReg
    {
        explicit SlotPackReg(SlotPack p) { addSlotPack(p); }
    };

    /* ── 2. 引导步骤：表填好之后、任何模组装载之前，按 stage 升序 ────
     * 给需要「宿主起来就干活」的包用（dimensions 读配置并布 hook、
     * hooks 预热引擎）。放 SPI 而不放各包的静态构造里，是因为这些工作
     * 需要 LL 环境就绪（logger、配置目录），静态构造期太早。 */
    struct Bootstrap
    {
        int stage;                  // 升序执行；同 stage 顺序不保证
        std::string_view name;
        void (*run)();
    };
    void addBootstrap(Bootstrap step);

    struct BootstrapReg
    {
        explicit BootstrapReg(Bootstrap b) { addBootstrap(b); }
    };

    /* ── 3. 卸载否决：unload 之前逐个问「现在能卸它吗」───────────────
     * 返回 nullptr = 放行；返回原因字符串（静态存续期）= 否决。
     * lane 用它挡「还有别的模组正拿着这个提供方的车道」；未来的包
     * 有同类不变量也走这里，宿主一行不用改。 */
    struct UnloadVeto
    {
        std::string_view name;
        char const* (*why)(HostedMod* mod);
    };
    void addUnloadVeto(UnloadVeto veto);

    struct UnloadVetoReg
    {
        explicit UnloadVetoReg(UnloadVeto v) { addUnloadVeto(v); }
    };

    /* ── 4. 拆除步骤：模组死亡时按 stage 升序清资源 ──────────────────
     * 「死亡」= 主动卸载、装载中途失败回滚、服务器关停 —— 三条路都走
     * 这里，所以每个持有模组资源的包只需要写一次清理。
     *
     * stage 约定（间距留给后来者插队）：
     *   10 计划任务   20 总线   30 服务   40 车道   50 命令回调
     *   60 表单票据   70 KvDb   80 合成事件   90 数据包 hook
     *  100 经济回调  110 客户端资源
     * 顺序背后的不变量：先停「会再进模组代码」的东西（任务/事件），
     * 再撤「别人可能正引用」的注册（服务/车道），最后清纯数据。 */
    struct Teardown
    {
        int stage;
        std::string_view name;
        void (*run)(HostedMod* mod);
    };
    void addTeardown(Teardown step);

    struct TeardownReg
    {
        explicit TeardownReg(Teardown t) { addTeardown(t); }
    };

    /* ── 5. 事件提供方：合成事件接进 subscribe_event 的解析 ──────────
     * hooks（原生 detour 合成）和命令事件都不在 LL 的动态事件注册表里；
     * 它们以提供方身份挂进来，由 pier-api 的 Events 按契约 §六 的顺序
     * 解析。认领判定必须走 idMatches —— 匹配器只有一个。 */
    struct EventProvider
    {
        std::string_view name;

        /** 本提供方的 id 是否**对应**注册表里的同名条目。
         *
         *  true  —— 命令事件这类：注册表里躺着同一个事件的发射器条目，但
         *           LL 只派发给类型化监听器，动态路径接不到 —— 提供方替换
         *           注册表路径是**修复**，不是遮蔽，解析时不告警。
         *  false —— hooks 这类纯合成事件：注册表出现同后缀 id 意味着上游
         *           新增了真事件而我们的合成名撞了 —— 必须打 warn 让人看见
         *          （契约 §六：遮蔽必须可见）。 */
        bool covers_registry;

        /** wanted 是否属于本提供方（精确名或带分隔符的后缀）。 */
        bool (*claims)(std::string_view wanted);

        /** 认领后订阅。失败返回 NULL —— 调用方（Events）负责报错，
         *  **不会**下落到别的解析路径（认领即负责，契约 §六）。 */
        PierListenerHandle (*subscribe)(
            HostedMod* mod, std::string_view wanted, int32_t priority, PierEventCb cb, void* user);

        /** 找到并退掉返回 true；不是本提供方的句柄返回 false（让下一家试）。 */
        bool (*unsubscribe)(HostedMod* mod, PierListenerHandle handle);

        /** 模组死亡：丢掉它名下的全部订阅（W-EV1 的提供方侧）。 */
        void (*dropMod)(HostedMod* mod);

        /** 把本提供方的全部事件 id 逐个喂给 sink（/pier events、报错提示用）。 */
        void (*list)(void* ctx, PierStrSink sink);
    };
    void addEventProvider(EventProvider provider);

    struct EventProviderReg
    {
        explicit EventProviderReg(EventProvider p) { addEventProvider(p); }
    };

    /* ── 6. 维度桥：dimensions 能力包的单槽扩展点 ─────────────────────
     * api 的世界函数需要两件只有 dimensions 才知道的事：自定义 id 叫什么
     * 名字、怎么把还没建出来的自定义维度逼出来。这不是列表而是单槽 ——
     * 同时存在两套维度台账本身就是错误。包缺席时为空，api 按「只认原版」
     * 降级并各自打一次 warn。诊断细节（注册台账、配置漂移）由**实现方**
     * 在自己的失败路径里打日志 —— 它才知道台账长什么样。 */
    struct DimensionBridge
    {
        /** `/execute in` 认的维度名；未注册返回空串（并自打诊断）。 */
        std::string (*selectorNameOf)(int32_t dim);
        /** 强制建出自定义维度并取 BlockSource；失败 nullptr（自打诊断）。
         *
         *  实现方**必须**校验建出的引擎实例 id == 请求的 dim，不一致时报错
         *  并返回 nullptr。台账 id 与引擎 id 漂移时：方块写入会静默落进错
         *  的维度；而把玩家传送进 dim 会让引擎在区块工作线程抛未捕获异常，
         *  整个进程 fastfail(0xC0000409) —— 不是调用方一句「失败」能兜住
         *  的。校验所需的知识（注册台账、按名建维度）只在 dimensions 包，
         *  所以门必须设在实现方这一头；api 侧把「blockSourceOf 非空」当作
         *  唯一的放行条件。 */
        ::BlockSource* (*blockSourceOf)(int32_t dim);
    };
    void setDimensionBridge(DimensionBridge const* bridge);
    [[nodiscard]] DimensionBridge const* dimensionBridge() noexcept;

    /* ── 宿主消费面（pier-host 和 pier-api 的 Events 用）─────────────── */

    /** 填表头 + 按注册顺序执行全部槽位包；debug 日志列出包名。
     *  只在 load() 里调一次；之后表冻结。 */
    void buildApi(PierApi& api, ll::io::Logger& log);

    /** 按 stage 升序跑全部拆除步骤。 */
    void runTeardown(HostedMod* mod);

    /** 逐个问否决者；第一个说不的返回 {包名, 原因}，全放行返回空。 */
    struct VetoAnswer
    {
        std::string_view who;
        char const* reason;
    };
    std::optional<VetoAnswer> askUnloadVetoes(HostedMod* mod);

    /** 按 stage 升序跑全部引导步骤（load() 里、buildApi 之后）。 */
    void runBootstrap(ll::io::Logger& log);

    /** 遍历事件提供方。visit 返回 true 表示「处理完了，停止遍历」。 */
    bool forEachEventProvider(bool (*visit)(EventProvider const&, void* ctx), void* ctx);

    /* ── id 匹配（契约 §六：全仓库唯一的一份判定）──────────────────── */

    /** wanted 精确等于 canonical，或以「分隔符 + canonical」结尾
     *（分隔符 ∈ {"::", ":", "."}）。**永远不做子串匹配** —— 子串意味着
     *  上游哪天新增一个含相同词干的事件，订阅就被静默劫持。 */
    [[nodiscard]] bool idMatches(std::string_view wanted, std::string_view canonical) noexcept;

    /* ── 监听句柄的 id 空间（进程内单调，永不复用）────────────────────
     * 住在 SPI 是因为三个消费方（Events、hooks 提供方、命令事件提供方）
     * 都要发句柄，而它们互不 include —— id 空间必须只有一个，否则两家
     * 发出同一个「唯一」id 只是时间问题。 */
    [[nodiscard]] std::uint64_t nextListenerId() noexcept;

    [[nodiscard]] inline PierListenerHandle handleOf(std::uint64_t id) noexcept
    {
        return reinterpret_cast<PierListenerHandle>(static_cast<std::uintptr_t>(id));
    }

    [[nodiscard]] inline std::uint64_t idOf(PierListenerHandle h) noexcept
    {
        return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(h));
    }
} // namespace pier::spi
