/** core/Bus.cpp —— 跨模组事件总线。
 *
 * # 为什么表归宿主
 *
 * 直觉的设计 —— 模组 A 导出一个 "subscribe"、模组 B 递回调 —— 在这里
 * 安全不起来。`ModHost::unload` 会 FreeLibrary，B 一卸载，A 手里就攥着
 * 指进已 unmap dylib 的函数指针，下一次 publish 就是一次没有任何诊断的
 * 崩溃。这个桥的每个异步面都解过一遍同样的题（Forms、按模组记账的
 * Scheduler），答案每次都一样：宿主持表、条目按票据编号、触发路径持
 * weak_ptr<HostedMod> 并在调用前重新验证。
 *
 * # 宿主不做什么
 *
 * 不解析载荷。`topic` 和 `payload` 是两个模组在带外约定的不透明 UTF-8。
 * 宿主定 schema 意味着每个发布者都要满足它、宿主还得给它做版本 ——
 * 零收益，因为宿主用不着内容。
 *
 * # 环
 *
 * 两道独立的闸，因为它们逮的形状不同：
 *
 *   - 模组永远收不到自己的发布。想通知自己有直接函数调用；自递送是唯一
 *     一种任何深度上限都分不清是不是真工作的环。
 *   - 嵌套派发的深度上限逮 A → B → A。触顶时丢掉最里层的 publish 并打
 *     一次日志，而不是把栈涨到服务器死掉。
 *
 * # 派发与锁
 *
 * 订阅者列表在锁内做快照，每个回调都在锁**释放后**调用。订阅者理所当然
 * 会重入（订阅、退订、发布别的主题），锁着调进 dylib，第一个重入的就把
 * 服务器线程锁死。每个条目在调用前按票据重新验证，所以派发**期间**被
 * 移除的订阅永远不会被调到。
 */
#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "sdk/abi.h"

#include "pier/host/hosted_mod.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/log.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        /** 收下的最长主题。够写 `some-long-mod:some-event`，又短到一个被当
         *  字符串读的垃圾指针变不成几兆字节的 map 键。 */
        constexpr size_t kMaxTopic = 128;

        /** 嵌套派发上限。8 远超任何真实链条（publish → handler → publish
         *  是深度 2）；比这深的都是环。 */
        constexpr int kMaxDepth = 8;

        struct Subscription
        {
            HostedMod* mod = nullptr; // 只作身份比对；永不盲目解引用
            /** 订阅时取得的弱引用（V-06）。fireOne 只经它复核存活：从裸指针
             *  `shared_from_this()` 本身就是一次盲解引用 —— 模组在另一线程
             *  卸载后那块内存已经不属于任何人（Services.cpp W13 早已如此）。 */
            std::weak_ptr<HostedMod> owner;
            std::string topic;
            PierBusCb cb = nullptr;
            void* user = nullptr;
        };

        std::mutex gBusMutex;
        std::unordered_map<uint64_t, Subscription> gSubs;
        /** topic → 订阅 id，让 publish 不用扫全表。和 gSubs 在同一把锁下
         *  保持同步。 */
        std::unordered_map<std::string, std::vector<uint64_t>> gByTopic;
        uint64_t gNextSubId = 1;

        /** 按线程计的嵌套深度。thread_local 而不是全局计数：两个线程并发
         *  发布不是环，共享计数会把它们看成环。 */
        thread_local int gDepth = 0;

        struct DepthGuard
        {
            DepthGuard() { ++gDepth; }
            ~DepthGuard() { --gDepth; }
        };

        /** 每个主题只喊一次「太深」，然后闭嘴。环转得和 CPU 一样快；每次
         *  触发都打日志会把一个 bug 变成一场事故。 */
        void warnDepthOnce(std::string const& topic)
        {
            static std::mutex mu;
            static std::unordered_map<std::string, bool> seen;
            std::lock_guard lock(mu);
            if (seen[topic]) return;
            seen[topic] = true;
            hostLogger().error(
                "跨模组总线：主题 '{}' 超过嵌套深度 {} —— 丢弃最里层的 publish。"
                "这是一个发布环：这个主题的某个订阅者又把它发布了一遍"
                "（直接发，或经由另一个绕回来的主题）。",
                topic, kMaxDepth
            );
        }

        /** 快照订阅了 `topic` 的 id，剔除发布者自己的。 */
        std::vector<uint64_t> idsFor(std::string const& topic, HostedMod* publisher)
        {
            std::vector<uint64_t> out;
            std::lock_guard lock(gBusMutex);
            auto it = gByTopic.find(topic);
            if (it == gByTopic.end()) return out;
            out.reserve(it->second.size());
            for (uint64_t id : it->second)
            {
                auto s = gSubs.find(id);
                if (s == gSubs.end()) continue;
                if (s->second.mod == publisher) continue; // 不自递送
                out.push_back(id);
            }
            return out;
        }

        /**
         * 按票据触发一个订阅。返回否决位；`ran` 在回调真的执行时置真。
         *
         * 查表和调用刻意分开：条目在锁内拷出、锁放掉，然后控制流才跨进
         * dylib。
         */
        bool fireOne(uint64_t id, std::string_view topic, std::string_view payload, bool& ran)
        {
            ran = false;
            Subscription sub;
            {
                std::lock_guard lock(gBusMutex);
                auto it = gSubs.find(id);
                // 没了：被本次派发里更早的订阅者退订了，或它的模组在派发
                // 中途卸载了。
                if (it == gSubs.end()) return false;
                sub = it->second;
            }
            if (!sub.cb || !sub.mod) return false;

            // 只经订阅时保存的 weak_ptr 复核；模组已析构时 lock() 得空，不碰
            // 任何已释放的内存。指针相等再比一次：防止新模组恰好落在旧地址。
            auto mod = sub.owner.lock();
            if (!mod || mod.get() != sub.mod) return false; // dylib 可能已 unmap
            if (!mod->isEnabled()) return false;            // 禁用期间静音

            // 持有 shared_ptr + 回调计数直到回调返回：卸载会被 ModHost 否决
            //（V-06/V-28），而不是在我们脚下把代码段抽走。
            CallbackScope scope{mod.get()};
            ran = true;
            return sub.cb(sub.user, ps(topic), ps(payload));
        }

        bool publishImpl(
            PierModHandle modHandle, PierStr topicRaw, PierStr payloadRaw, uint32_t* outDelivered)
        {
            if (outDelivered) *outDelivered = 0;

            std::string topic = toString(topicRaw);
            if (topic.empty() || topic.size() > kMaxTopic) return false;

            if (gDepth >= kMaxDepth)
            {
                warnDepthOnce(topic);
                return false;
            }
            DepthGuard depth;

            auto* publisher = modHandle ? asMod(modHandle) : nullptr;
            auto ids = idsFor(topic, publisher);

            std::string_view payload = sv(payloadRaw);
            bool vetoed = false;
            uint32_t delivered = 0;
            for (uint64_t id : ids)
            {
                bool ran = false;
                // 有人否决也不短路：观察者必须看到一致的流，不管更早的订阅
                // 者拒没拒绝。跳过剩下的会让一个模组观察到什么取决于订阅者
                // 顺序 —— 那是任何模组都控制不了的东西。
                bool v = fireOne(id, topic, payload, ran);
                if (ran)
                {
                    ++delivered;
                    vetoed = vetoed || v;
                }
            }
            if (outDelivered) *outDelivered = delivered;
            return vetoed;
        }

        uint64_t api_bus_subscribe(PierModHandle modHandle, PierStr topicRaw, PierBusCb cb, void* user)
        {
            PIER_API_GUARD_BEGIN
                if (!cb || !modHandle) return 0;
                auto* raw = asMod(modHandle);
                if (!raw) return 0;

                std::string topic = toString(topicRaw);
                if (topic.empty() || topic.size() > kMaxTopic) return 0;

                // 还没被 shared_ptr 接管的模组拒收：没有 weak_ptr 就没法在
                // 之后重新验证，而不经验证调进 dylib 正是这张表存在要防的。
                std::weak_ptr<HostedMod> owner;
                try
                {
                    owner = raw->shared_from_this();
                }
                catch (...)
                {
                    return 0;
                }

                std::lock_guard lock(gBusMutex);
                uint64_t id = gNextSubId++;
                gSubs[id] = Subscription{raw, std::move(owner), topic, cb, user};
                gByTopic[topic].push_back(id);
                return id;
            PIER_API_GUARD_END
        }

        bool api_bus_unsubscribe(PierModHandle modHandle, uint64_t subId)
        {
            PIER_API_GUARD_BEGIN
                if (!modHandle || subId == 0) return false;
                auto* raw = asMod(modHandle);

                std::lock_guard lock(gBusMutex);
                auto it = gSubs.find(subId);
                // 只限本人：一个模组不许让另一个闭嘴。
                if (it == gSubs.end() || it->second.mod != raw) return false;

                auto byTopic = gByTopic.find(it->second.topic);
                if (byTopic != gByTopic.end())
                {
                    auto& v = byTopic->second;
                    v.erase(std::remove(v.begin(), v.end(), subId), v.end());
                    if (v.empty()) gByTopic.erase(byTopic);
                }
                gSubs.erase(it);
                return true;
            PIER_API_GUARD_END
        }

        uint32_t api_bus_publish(PierModHandle modHandle, PierStr topic, PierStr payload)
        {
            PIER_API_GUARD_BEGIN
                uint32_t delivered = 0;
                (void)publishImpl(modHandle, topic, payload, &delivered);
                return delivered;
            PIER_API_GUARD_END
        }

        bool api_bus_publish_vetoable(
            PierModHandle modHandle, PierStr topic, PierStr payload, uint32_t* outDelivered)
        {
            PIER_API_GUARD_BEGIN
                return publishImpl(modHandle, topic, payload, outDelivered);
            PIER_API_GUARD_END
        }

        uint32_t api_bus_subscriber_count(PierStr topicRaw)
        {
            PIER_API_GUARD_BEGIN
                std::string topic = toString(topicRaw);
                if (topic.empty()) return 0;
                std::lock_guard lock(gBusMutex);
                auto it = gByTopic.find(topic);
                return it == gByTopic.end() ? 0u : static_cast<uint32_t>(it->second.size());
            PIER_API_GUARD_END
        }

        /** 拆除（stage 20）：清掉该模组名下的全部订阅。 */
        void teardown(HostedMod* mod)
        {
            std::lock_guard lock(gBusMutex);
            for (auto it = gSubs.begin(); it != gSubs.end();)
            {
                if (it->second.mod != mod)
                {
                    ++it;
                    continue;
                }
                auto byTopic = gByTopic.find(it->second.topic);
                if (byTopic != gByTopic.end())
                {
                    auto& v = byTopic->second;
                    uint64_t id = it->first;
                    v.erase(std::remove(v.begin(), v.end(), id), v.end());
                    if (v.empty()) gByTopic.erase(byTopic);
                }
                it = gSubs.erase(it);
            }
        }

        void fill(PierApi& api)
        {
            api.bus_subscribe = &api_bus_subscribe;
            api.bus_unsubscribe = &api_bus_unsubscribe;
            api.bus_publish = &api_bus_publish;
            api.bus_publish_vetoable = &api_bus_publish_vetoable;
            api.bus_subscriber_count = &api_bus_subscriber_count;
        }

        spi::SlotPackReg regSlots{{"bus", &fill}};
        spi::TeardownReg regDown{{20, "bus", &teardown}};
    } // namespace
} // namespace pier::api_impl
