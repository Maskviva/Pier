/** hooks/engine/HookEvents.cpp —— 注册表存储、派发，以及作为
 *  spi::EventProvider 挂进宿主动态事件路径的接线。模块契约见 hook_events.h。
 */
#include "pier/hooks/hook_events.h"

#include <algorithm>
#include <atomic>
#include <string>
#include <utility>
#include <vector>

#include "ll/api/utils/ErrorUtils.h"

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/platform/UUID.h"
#include "mc/world/actor/player/Player.h"

#include "pier/host/hosted_mod.h"
#include "pier/host/spi.h"
#include "pier/support/log.h"
#include "pier/support/snbt.h"
#include "pier/support/str.h"

namespace pier::hooks
{
    namespace
    {
        /** Meyers 单例：任何 TU 的静态注册器往里填都安全。 */
        std::vector<HookEventDef*>& table()
        {
            static std::vector<HookEventDef*> t;
            return t;
        }

        /** 句柄 ↔ 共享 id。id 来自 spi::nextListenerId（从 1 起的小整数），
         *  和别的提供方的堆指针句柄在数值上天然不相交；退订还要过
         *  「在我表里 + mod 相符」两道检查，误认不可能变成误退。 */
        PierListenerHandle toHandle(std::uint64_t id)
        {
            return reinterpret_cast<PierListenerHandle>(static_cast<uintptr_t>(id));
        }

        std::uint64_t fromHandle(PierListenerHandle h)
        {
            return static_cast<std::uint64_t>(reinterpret_cast<uintptr_t>(h));
        }

        /** 一次回调，异常就地接住。
         *  W11：接住还得看得见 —— 异常每次都打印；「已被隔离」的提醒每进程
         *  一次。绝不让模组的异常顺着栈回卷进引擎的被钩函数（那会把一次逻辑
         *  bug 升级成半更新状态下的引擎崩溃）。 */
        void callOne(PierEventCb cb, void* user, std::string const& id, std::string const& snbt,
                     void* wctx, PierStrSink sink)
        {
            try
            {
                cb(user, ps(id), ps(snbt), wctx, sink);
            }
            catch (...)
            {
                ll::error_utils::printCurrentException(hostLogger());
                static std::atomic<bool> warned{false};
                if (!warned.exchange(true))
                {
                    hostLogger().warn(
                        "合成事件：某个模组回调抛了异常，已就地隔离。"
                        "这条警告只打一次；上面的异常每次都打。"
                    );
                }
            }
        }

        /**
         * 应答里的取消位。**解析，不搜子串。**
         *
         * 旧实现在应答字符串里找三种写法（`cancelled:1b`、`"cancelled":1`、
         * `cancelled:1 `）—— 另一侧走 NbtValue 往返和走字符串替换两条路会产
         * 出不同形状，漏配任何一种，取消就静默失效，而那正是最难被发现的坏
         * 法（其余一切照常工作）。三种写法都是合法 SNBT，交给
         * CompoundTag::fromSnbt 统一解析后按标签真值判断，形状差异从此与判
         * 定无关。解析失败按「未取消」处理 —— 一个连自己应答都拼不对的订阅
         * 者不该拿到否决权。
         */
        bool replyCancelled(std::string const& reply)
        {
            if (reply.empty()) return false;
            auto tag = CompoundTag::fromSnbt(reply);
            if (!tag || !tag->contains("cancelled")) return false;
            auto const& v = tag->at("cancelled");
            if (v.is_number()) return static_cast<double>(v) != 0.0;
            return false;
        }

        /** 派发前的订阅快照（回调可在派发中途改 def.subs，直接迭代是 UB）。
         *  subs 本身按优先级保持有序（见 subscribe），快照顺序即派发顺序。 */
        std::vector<std::pair<PierEventCb, void*>> snapshot(HookEventDef& def)
        {
            std::vector<std::pair<PierEventCb, void*>> snap;
            snap.reserve(def.subs.size());
            for (auto& sub : def.subs) snap.emplace_back(sub->cb, sub->user);
            return snap;
        }
    } // namespace

    std::string playerRefSnbt(::Player const& p)
    {
        return "\"_player\":{\"name\":\"" + snbtEscape(p.getRealName())
            + "\",\"xuid\":\"" + snbtEscape(p.getXuid())
            + "\",\"uuid\":\"" + snbtEscape(p.getUuid().asString()) + "\"}";
    }

    HookEventRegistrar::HookEventRegistrar(HookEventDef& def) { table().push_back(&def); }

    void dispatchHookEvent(HookEventDef& def, std::string const& snbt)
    {
        auto snap = snapshot(def);
        std::string id{def.name};
        struct WCtx
        {
        } w; // 只观察：写回是 no-op
        for (auto& [cb, user] : snap)
        {
            callOne(cb, user, id, snbt, &w, [](void*, PierStr) {});
        }
    }

    bool dispatchHookEventCancellable(HookEventDef& def, std::string const& snbt)
    {
        // 与 dispatchHookEvent 同一套快照纪律，外加一个活的写回 sink：订阅者
        // 以含取消旗的 SNBT 应答即否决这次动作。
        auto snap = snapshot(def);
        std::string id{def.name};
        bool cancelled = false;
        for (auto& [cb, user] : snap)
        {
            std::string reply;
            callOne(cb, user, id, snbt, &reply, [](void* ctx, PierStr v)
            {
                if (ctx) *static_cast<std::string*>(ctx) = toString(v);
            });
            if (replyCancelled(reply))
            {
                cancelled = true;
                // 继续走：每个订阅者都得看到事件。提前停会让「我被调到了吗」
                // 取决于监听器注册顺序。
            }
        }
        return cancelled;
    }

    namespace
    {
        /* ─────────────── spi::EventProvider 接线 ─────────────── */

        HookEventDef* findDef(std::string_view wanted)
        {
            for (auto* def : table())
            {
                // 精确名或带分隔符的唯一后缀（spi::idMatches）。不做子串匹配
                // —— 旧版的 find(name) != npos 会让 "xxFooEventxx" 也命中。
                if (spi::idMatches(wanted, def->name)) return def;
            }
            return nullptr;
        }

        bool providerClaims(std::string_view wanted) { return findDef(wanted) != nullptr; }

        PierListenerHandle providerSubscribe(
            HostedMod* mod, std::string_view wanted, int32_t priority, PierEventCb cb, void* user)
        {
            auto* def = findDef(wanted);
            if (!def || !cb) return nullptr;
            if (!def->installed)
            {
                def->install();
                def->installed = true;
            }
            std::uint64_t id = spi::nextListenerId();
            auto sub = std::make_unique<HookSub>(HookSub{mod, cb, user, id, priority});
            // 插入即保序：数值小者先派发；同优先级按到达顺序（稳定）。
            auto pos = std::find_if(def->subs.begin(), def->subs.end(),
                                    [&](auto const& s) { return s->priority > priority; });
            def->subs.insert(pos, std::move(sub));
            return toHandle(id);
        }

        bool providerUnsubscribe(HostedMod* mod, PierListenerHandle handle)
        {
            auto wanted = fromHandle(handle);
            if (wanted == 0) return false;
            for (auto* def : table())
            {
                for (auto it = def->subs.begin(); it != def->subs.end(); ++it)
                {
                    if ((*it)->id == wanted && (*it)->mod == mod)
                    {
                        def->subs.erase(it);
                        return true;
                    }
                }
            }
            return false; // 不是本提供方的句柄 —— 让下一家试
        }

        void providerDropMod(HostedMod* mod)
        {
            for (auto* def : table())
            {
                std::erase_if(def->subs, [&](auto& s) { return s->mod == mod; });
            }
        }

        void providerList(void* ctx, PierStrSink sink)
        {
            for (auto* def : table())
            {
                sink(ctx, ps(def->name));
            }
        }

        spi::EventProviderReg reg{spi::EventProvider{
            /*name*/ "hooks",
            /*covers_registry*/ false, // 纯合成事件：注册表同后缀 = 上游新增真事件，遮蔽必须打 warn
            &providerClaims,
            &providerSubscribe,
            &providerUnsubscribe,
            &providerDropMod,
            &providerList,
        }};
    } // namespace
} // namespace pier::hooks
