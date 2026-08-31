/**
 * pier/hooks/hook_events.h —— 合成事件注册表：由原生 detour 撑起的事件 id，
 * 经普通的 subscribe_event ABI 按名订阅。
 *
 * 一个关注点一个 TU 且自注册，加一个合成事件不碰这个头、不碰 Events.cpp、不碰
 * 任何表。共享的生命周期规矩，每个 hook 文件都遵守：
 *  - detour 懒安装（第一个订阅者或第一次控制调用），永不卸补丁；退订可能来自被
 *    钩函数内部，在那里卸补丁不安全。空闲钩子靠一次 subs-empty / not-armed 判断
 *    快速路由回 origin。
 *  - 一切都跑在服务器线程上，注册表无需加锁。
 *
 * 本包注册 spi::EventProvider{name="hooks", covers_registry=false}。false 表示
 * 这些是纯合成事件，注册表里出现同后缀 id 意味着上游新增了真事件而合成名撞了，
 * 解析时必须打 warn。认领走 spi::idMatches（精确名或带分隔符的唯一后缀），不做
 * 子串匹配：find(name) != npos 会让 "xxPlayerAttackEventxx" 也命中。
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
     * 把玩家身份拼成消费方约定的 _player 子对象（形如
     * "_player":{"name":…,"xuid":…,"uuid":…}，不含前导逗号）。
     *
     * 唯一出处，形状对所有合成事件强制一致。逐 TU 各抄一份的代价是漂移：某一份
     * 改了字段名或漏了转义时，只有那一个事件解不出玩家，而症状看不出根因。
     */
    [[nodiscard]] std::string playerRefSnbt(::Player const& p);

    struct HookSub
    {
        HostedMod* mod;
        PierEventCb cb;
        void* user;
        /** 与宿主的动态事件路径共用 spi::nextListenerId() 这一个 id 源。订阅
         *  不能用地址做身份：条目会被释放重分配，回收的地址会把一张旧票据变成
         *  能退掉别人订阅的钥匙。共用一个源，两条路径不会发出同一个句柄。 */
        std::uint64_t id;
        /** 数值小者先派发（与宿主侧 LL 优先级映射同序）。 */
        int32_t priority;
    };

    struct HookEventDef
    {
        std::string_view name;
        /** 装原生 detour，只在第一个订阅者出现时调一次。返回是否全部主钩点安装
         *  成功；失败时订阅被拒绝，而不是发一个永不触发的句柄。 */
        bool (*install)();
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
     * 把一份 SNBT 载荷投给 def 的每个订阅者。
     *
     * 快照安全：回调可以在派发中途订阅或退订，自我退订的回调仍收到当前这个事件，
     * 中途加入的从下一个开始。合成事件只观察，写回 sink 是 no-op。回调抛出的异常
     * 就地接住并打印，绝不让它顺栈回卷进引擎的被钩函数；静默的 catch 会让 bug
     * 永久隐形。
     */
    void dispatchHookEvent(HookEventDef& def, std::string const& snbt);

    /**
     * 同 dispatchHookEvent，但写回 sink 是活的：任一订阅者以含真值 cancelled
     * 字段的 SNBT 应答即返回 true。
     *
     * 只用于 origin 真的可以跳过的钩点；会让引擎停在半更新状态的地方不许用它。
     * 有人取消后其余订阅者仍会被调到，行为不依赖监听器的注册顺序。
     */
    bool dispatchHookEventCancellable(HookEventDef& def, std::string const& snbt);
} // namespace pier::hooks
