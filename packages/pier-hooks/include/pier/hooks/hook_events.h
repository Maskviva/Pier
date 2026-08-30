/**
 * pier/hooks/hook_events.h —— 「桥合成事件」背后的注册表：由原生 detour 撑
 * 起的合成事件 id，经普通的 subscribe_event ABI 订阅（按名解析）。
 *
 * 模块布局（一个关注点一个 TU、自注册 —— 加一个合成事件不碰这个头、不碰
 * Events.cpp、不碰任何表）：
 *
 *     hooks/hook_events.h/.cpp  注册表 + 派发 + EventProvider 接线
 *     engine/TickControl.cpp    Level::tick detour + tick_freeze/step/warp
 *     engine/Profiler.cpp       Level/Dimension/LevelChunk 计时 detour
 *     world/HopperEvents.cpp    "HopperTransferEvent"
 *     world/DestroyEvents.cpp   "PlayerStartDestroyBlockEvent"
 *     …（player/ protect/ world/ 下每文件一到数个事件）
 *
 * 共享生命周期规矩（每个 hook 文件都遵守）：
 *  - detour 懒安装（第一个订阅者 / 第一次控制调用），**永不**卸补丁：退订
 *    可能来自被钩函数内部，在那里卸补丁不安全。空闲的钩子靠一次
 *    subs-empty / not-armed 判断快速路由回 origin。
 *  - 一切都跑在服务器线程上（所有 ABI 调用和所有被钩函数都是），注册表无
 *    需加锁。
 *
 * # 与宿主的接线（新架构）
 *
 * 本包不再被 Events.cpp 点名调用四个函数；它注册一个
 * spi::EventProvider{name="hooks", covers_registry=false}。covers_registry
 * 为 false 的含义（契约 §六）：这些是纯合成事件，注册表里若出现同后缀 id
 * 说明上游新增了真事件、我们的合成名撞了 —— 解析时必须打 warn 让人看见。
 * 认领判定走 spi::idMatches（精确名或带分隔符的唯一后缀），**不做子串匹配**
 * —— 旧版的 `find(name) != npos` 会让 "xxPlayerAttackEventxx" 也命中。
 */
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "sdk/abi.h"

class Player;

namespace pier
{
    class HostedMod;
}

namespace pier::hooks
{
    /**
     * 把玩家身份拼成消费方约定的 `_player` 子对象（形如
     * `"_player":{"name":…,"xuid":…,"uuid":…}`，不含前导逗号）。
     *
     * 十四个事件 TU 曾各自抄一遍这三行拼接。抄多了的代价不是行数，是**漂
     * 移**：其中一个改了字段名或漏了转义，另一侧按事件名分支解析时只有那
     * 一个事件会解不出玩家，而且症状（"这个事件收得到，就是取不到玩家"）
     * 看不出根因在拼接侧。收成一个函数，形状对所有合成事件强制一致。
     */
    [[nodiscard]] std::string playerRefSnbt(::Player const& p);

    struct HookSub
    {
        HostedMod* mod;
        PierEventCb cb;
        void* user;
        /** 与宿主的动态事件路径共用 spi::nextListenerId() 这一个 id 源 ——
         *  订阅为什么不能用地址做身份：条目会被释放重分配，回收的地址会把
         *  一张旧票据变成能退掉别人订阅的钥匙。共用一个源，两条路径永远不
         *  会发出同一个句柄。 */
        std::uint64_t id;
        /** 数值小者先派发（与宿主侧 LL 优先级映射同序）。 */
        int32_t priority;
    };

    struct HookEventDef
    {
        std::string_view name;
        /** 装原生 detour；只在第一个订阅者出现时调一次。 */
        void (*install)();
        bool installed = false;
        std::vector<std::unique_ptr<HookSub>> subs;

        /** 钩体的快速路由判据。 */
        bool live() const { return !subs.empty(); }
    };

    /**
     * 自注册：每个 hook TU 把自己的 HookEventDef 存成 static，用一个文件级
     * 注册器对象登记。只在运行期（订阅时）被消费，TU 间静态初始化顺序因此
     * 不是问题。
     */
    struct HookEventRegistrar
    {
        explicit HookEventRegistrar(HookEventDef& def);
    };

    /**
     * 把一份 SNBT 载荷投给 `def` 的每个订阅者。
     * 快照安全：回调可以在派发中途（反）订阅 —— 自我退订的回调仍收到当前
     * 这个事件，中途加入的从下一个开始。合成事件是只观察的；写回 sink 是
     * no-op。回调抛出的异常被就地接住并打印（W11：静默的 catch 会让 bug
     * 永久隐形），绝不让它顺着栈回卷进引擎的被钩函数。
     */
    void dispatchHookEvent(HookEventDef& def, std::string const& snbt);

    /**
     * 同 dispatchHookEvent，但写回 sink 是活的：任一订阅者以含真值
     * `cancelled` 字段的 SNBT 应答即返回 true。
     *
     * 只用于 origin 真的可以跳过的钩点 —— 「取消」会让引擎停在半更新状态的
     * 地方不许用它。有人取消后其余订阅者仍会被调到，行为不依赖监听器的注册
     * 顺序。
     */
    bool dispatchHookEventCancellable(HookEventDef& def, std::string const& snbt);
} // namespace pier::hooks
