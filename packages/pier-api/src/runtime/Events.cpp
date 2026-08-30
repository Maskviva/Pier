/** runtime/Events.cpp —— subscribe_event / unsubscribe_event / list_events。
 *
 * 解析顺序（契约 §六）：
 *   1. 事件提供方按 spi::idMatches 认领 —— 认领即负责：订失败在这里报错，
 *      不下落。提供方排在注册表之前，因为命令事件的发射器**在**注册表里
 *      但 LL 只派发给类型化监听器 —— 动态路径查得到、接不到。
 *   2. 注册表精确名，再唯一后缀。
 *   3. 全失败 → 报错并列出相近 id。
 * 匹配器全仓库只有 spi::idMatches 一份；这里没有任何子串判断。
 */
#include <cctype>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ll/api/event/DynamicListener.h"
#include "ll/api/event/EventBus.h"

#include "mc/deps/nbt/CompoundTag.h"

#include "sdk/abi.h"

#include "pier/api/bridge.h"
#include "pier/host/hosted_mod.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/log.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        /**
         * 解析事件 id，允许唯一后缀匹配（顺手性）。
         *
         * # 为什么按名字去重
         *
         * `bus.events()` 吐的是 **(mod, id) 对**，不是去重后的 id：每个注册
         * 过发射器的 mod 都贡献自己一条。早先按条目数数，服务器上第二个 mod
         * 一碰同一个事件，一个毫无歧义的名字就解析成「歧义」、订阅失败。
         *
         * 那种失败静默得最要命：调用方只看到一个 Err，用 `?` 串着注册监听器
         * 的 mod 从那一点起丢掉后面的每一个监听器 —— 土地保护最后只架起一道
         * 岗哨，还不知道为什么。
         *
         * 同名的两条不是歧义；真正不同的名字才是。
         */
        std::optional<ll::event::EventId> resolveEventId(std::string_view wanted)
        {
            auto& bus = ll::event::EventBus::getInstance();
            if (bus.hasEvent(ll::event::EventIdView{wanted}))
            {
                return ll::event::EventId{wanted};
            }
            std::optional<ll::event::EventId> hit;
            for (auto&& [modName, id] : bus.events())
            {
                std::string_view name = id.name;
                // 角色说明：注册表里的 `name` 是长的全名，用户给的 `wanted`
                // 是短名 —— 所以这里问的是「name 是不是 wanted 的带分隔符
                // 全名」，参数顺序与常见调用相反。
                if (spi::idMatches(name, wanted) || name == wanted)
                {
                    // 另一个 mod 注册的同名条目 —— 不算歧义。
                    if (hit && std::string_view(hit->name) != name) return std::nullopt;
                    if (!hit) hit.emplace(ll::event::EventId{name});
                }
            }
            return hit;
        }

        /**
         * 订阅失败时，把注册表里所有看着相关的 id 打出来 —— 让错误信息说出
         * 引擎实际管这个事件叫什么，而不是只有一句 "unknown"。一行，只走
         * 失败路径。
         */
        void reportSimilarEvents(HostedMod* mod, std::string_view wanted)
        {
            // 取末尾最长的 CamelCase 词做针：对 "PlayerPlacingBlockEvent"
            // 是 "Block"——松到能兜住改名，紧到不至于倒出整个注册表。
            std::string needle;
            for (size_t i = 0; i < wanted.size(); ++i)
            {
                if (i && std::isupper(static_cast<unsigned char>(wanted[i]))) needle.clear();
                needle.push_back(wanted[i]);
            }
            if (needle.size() < 3) needle = std::string(wanted.substr(0, 6));

            std::string found;
            size_t n = 0;
            for (auto&& [modName, id] : ll::event::EventBus::getInstance().events())
            {
                std::string_view name = id.name;
                if (name.find(needle) == std::string_view::npos) continue;
                if (found.find(name) != std::string::npos) continue; // 去重
                if (n++) found += ", ";
                found += name;
                if (n >= 12) break;
            }
            if (found.empty()) found = "(注册表里没有包含 '" + needle + "' 的 id)";
            mod->getLogger().error("subscribe_event: 含 '{}' 的 id：{}", needle, found);
        }

        ll::event::EventPriority mapPriority(int32_t priority)
        {
            // ABI 说 0..4（Highest..Lowest）；LeviLamina 用 0/100/200/300/400。
            switch (priority)
            {
            case 0:
                return ll::event::EventPriority::Highest;
            case 1:
                return ll::event::EventPriority::High;
            case 3:
                return ll::event::EventPriority::Low;
            case 4:
                return ll::event::EventPriority::Lowest;
            case 2:
            default:
                return ll::event::EventPriority::Normal;
            }
        }

        /** 纯合成提供方（covers_registry=false）的遮蔽告警：注册表里出现了
         *  能按 idMatches 对上提供方某个 canonical 的真事件。每对只吼一次
         *  —— 遮蔽必须可见，但不必每次订阅都可见。 */
        void warnIfShadowing(HostedMod* mod, spi::EventProvider const& provider,
                             std::string_view wanted)
        {
            static std::unordered_set<std::string> warned; // 服务器线程专用

            std::vector<std::string> canon;
            provider.list(&canon,
                          [](void* ctx, PierStr s)
                          { static_cast<std::vector<std::string>*>(ctx)->push_back(toString(s)); });

            for (auto&& [modName, id] : ll::event::EventBus::getInstance().events())
            {
                std::string_view name = id.name;
                for (auto const& c : canon)
                {
                    if (name != c && !spi::idMatches(name, c)) continue;
                    std::string key = std::string(name) + "|" + c;
                    if (!warned.insert(key).second) continue;
                    mod->getLogger().warn(
                        "subscribe_event('{}')：合成事件 '{}'（提供方 {}）遮蔽了注册表里的 "
                        "'{}' —— 上游新增了同名真事件。本次仍按合成事件订阅；要订真事件，"
                        "请用它的完整 id。",
                        wanted,
                        c,
                        provider.name,
                        name
                    );
                }
            }
        }

        PierListenerHandle api_subscribe_event(
            PierModHandle modHandle, PierStr eventId, int32_t priority, PierEventCb cb, void* user)
        {
            PIER_API_GUARD_BEGIN
                auto* mod = asMod(modHandle);
                if (!mod || !cb) return nullptr;

                std::string_view wanted = sv(eventId);

                // ── 1. 提供方（契约 §六）─────────────────────────────
                struct FindCtx
                {
                    std::string_view wanted;
                    spi::EventProvider const* hit;
                } fctx{wanted, nullptr};
                spi::forEachEventProvider(
                    [](spi::EventProvider const& p, void* raw)
                    {
                        auto* c = static_cast<FindCtx*>(raw);
                        if (!p.claims(c->wanted)) return false;
                        c->hit = &p;
                        return true;
                    },
                    &fctx
                );
                if (fctx.hit)
                {
                    if (!fctx.hit->covers_registry)
                    {
                        warnIfShadowing(mod, *fctx.hit, wanted);
                    }
                    auto h = fctx.hit->subscribe(mod, wanted, priority, cb, user);
                    if (!h)
                    {
                        // 认领即负责：不下落到注册表 —— 半路换一个载荷形状
                        // 完全不同的事件，比明确失败糟得多。
                        mod->getLogger().error(
                            "subscribe_event: 提供方 {} 认领了 '{}' 但订阅失败（原因见上方日志）",
                            fctx.hit->name,
                            wanted
                        );
                    }
                    return h;
                }

                // ── 2. 注册表 + DynamicListener ──────────────────────
                auto resolved = resolveEventId(wanted);
                if (!resolved)
                {
                    mod->getLogger().error("subscribe_event: 未知或歧义的事件 id '{}'", wanted);
                    reportSimilarEvents(mod, wanted);
                    return nullptr;
                }

                std::string idName = resolved->name;
                auto listener = ll::event::DynamicListener::create(
                    [cb, user, idName](CompoundTag& data)
                    {
                        std::string snbt = bridge::enrichEventData(data);

                        struct WriteCtx
                        {
                            CompoundTag* data;
                            std::string const* snapshot; // 正是交给 cb 的那一份
                        } wctx{&data, &snbt};

                        cb(
                            user,
                            ps(idName),
                            ps(snbt),
                            &wctx,
                            [](void* c, PierStr newSnbt)
                            {
                                auto* w = static_cast<WriteCtx*>(c);
                                auto edited = CompoundTag::fromSnbt(sv(newSnbt));
                                if (!edited) return;

                                // 只写这个调用方相对**它自己拿到的那份快照**
                                // 真正改动过的字段。
                                //
                                // 整棵树替换（原来的做法）会在两个 mod 订阅同
                                // 一事件时丢更新：第二个回调拿到的是第一个编
                                // 辑**之前**的样子，把自己那份写回去就把前一
                                // 个的改动还原了。不报错、不打日志 —— 一个聊
                                // 天过滤和一个土地保护挂在同一个事件上，会按
                                // 优先级顺序互相抵消。
                                //
                                // 按快照做差量让两者互不干扰：没碰过的字段永
                                // 远不写，于是保留前一个监听器留下的值。同字
                                // 段冲突仍是后写者赢，那是任何 merge 能做到的
                                // 上限。
                                //
                                // 多解析一次快照的代价只在真的写回时才付；纯
                                // 观察的监听器根本走不到这个 lambda。
                                auto base = CompoundTag::fromSnbt(*w->snapshot);
                                if (!base)
                                {
                                    // 快照解析不了（它来自 toSnbt，基本不可
                                    // 能）。退回旧的整树语义，总比把这次编辑
                                    // 丢掉强。
                                    *w->data = std::move(*edited);
                                    return;
                                }

                                for (auto const& [key, value] : edited->mTags)
                                {
                                    auto it = base->mTags.find(key);
                                    if (it == base->mTags.end() || !(it->second == value))
                                    {
                                        w->data->mTags[key] = value;
                                    }
                                }
                                for (auto const& [key, value] : base->mTags)
                                {
                                    (void)value;
                                    if (!edited->mTags.contains(key)) w->data->erase(key);
                                }
                            }
                        );
                    },
                    mapPriority(priority),
                    mod->shared_from_this()
                );

                if (!ll::event::EventBus::getInstance().addListener(
                        listener, ll::event::EventIdView{resolved->name}))
                {
                    mod->getLogger().error(
                        "subscribe_event: '{}' 解析成功但 addListener 失败", idName
                    );
                    return nullptr;
                }
                std::uint64_t id = spi::nextListenerId();
                mod->listeners.push_back({id, listener});
                return spi::handleOf(id);
            PIER_API_GUARD_END
        }

        bool api_unsubscribe_event(PierModHandle modHandle, PierListenerHandle handle)
        {
            PIER_API_GUARD_BEGIN
                auto* mod = asMod(modHandle);
                if (!mod || !handle) return false;

                // 提供方自持句柄的（hooks）先问一圈。
                struct UnsubCtx
                {
                    HostedMod* mod;
                    PierListenerHandle handle;
                    bool done;
                } uctx{mod, handle, false};
                spi::forEachEventProvider(
                    [](spi::EventProvider const& p, void* raw)
                    {
                        auto* c = static_cast<UnsubCtx*>(raw);
                        if (!p.unsubscribe(c->mod, c->handle)) return false;
                        c->done = true;
                        return true;
                    },
                    &uctx
                );
                if (uctx.done) return true;

                // 挂在总线上的（动态监听器 + 命令事件的类型化监听器都记在
                // mod->listeners 里）。
                auto wantedId = spi::idOf(handle);
                for (auto it = mod->listeners.begin(); it != mod->listeners.end(); ++it)
                {
                    if (it->id == wantedId)
                    {
                        // W-EV2：摘不下来时也要把自己这份记录丢掉（它是我们
                        // 的），但必须说出来 —— 监听器仍在总线上，而它的回调
                        // 指向这个 dylib。静默返回 false 的话，卸载时那一轮
                        // removeListener 连试都不会试到它。
                        bool ok = ll::event::EventBus::getInstance().removeListener(it->listener);
                        if (!ok)
                        {
                            mod->getLogger().error(
                                "unsubscribe_event: 监听器 {} 没能从事件总线摘下 —— "
                                "它可能仍在被派发，而回调指向本 dylib。",
                                wantedId
                            );
                        }
                        mod->listeners.erase(it);
                        return ok;
                    }
                }
                return false;
            PIER_API_GUARD_END
        }

        void api_list_events(void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                if (!sink) return;
                for (auto&& [modName, id] : ll::event::EventBus::getInstance().events())
                {
                    sink(ctx, ps(std::string_view{id.name}));
                }
                // 提供方合成的事件同样可订阅，一并列出。
                struct ListCtx
                {
                    void* ctx;
                    PierStrSink sink;
                } lctx{ctx, sink};
                spi::forEachEventProvider(
                    [](spi::EventProvider const& p, void* raw)
                    {
                        auto* c = static_cast<ListCtx*>(raw);
                        p.list(c->ctx, c->sink);
                        return false; // 走完全部提供方
                    },
                    &lctx
                );
            PIER_API_GUARD_END_VOID
        }

        /**
         * 拆除（stage 80）：把这个模组名下、由**提供方**自持的订阅摘干净。
         *
         * W-EV1 的另一半。ModHost 在卸载路径上只摘得掉记在 `mod->listeners`
         * 里的那些（挂在 LL 事件总线上的动态监听器和命令事件的类型化监听
         * 器）；提供方（hooks 那类合成事件）的订阅住在提供方自己的表里，宿主
         * 不认识它们。不在这里显式 dropMod，那些条目就会活过 FreeLibrary ——
         * 而它们持有的回调指针指向刚被 unmap 的代码段，下一次那个钩点触发就
         * 是一次没有任何诊断的崩溃，且崩在**别的**玩家的动作上。
         *
         * 阶段 80 在 forms(60)/kvdb(70) 之后、packet-hooks(90) 之前：合成事件
         * 的回调可能在被摘掉的过程中反过来碰表单或 kv 库，那两个先清完更安
         * 全；而它和 packet-hooks 之间没有相互依赖，排在前面只是为了让「事件
         * 类」的清理集中在一起。
         */
        void teardown(HostedMod* mod)
        {
            spi::forEachEventProvider(
                [](spi::EventProvider const& p, void* raw)
                {
                    p.dropMod(static_cast<HostedMod*>(raw));
                    return false; // 走完全部提供方
                },
                mod
            );
        }

        void fill(PierApi& api)
        {
            api.subscribe_event = &api_subscribe_event;
            api.unsubscribe_event = &api_unsubscribe_event;
            api.list_events = &api_list_events;
        }

        spi::SlotPackReg regSlots{{"events", &fill}};
        spi::TeardownReg regDown{{80, "events", &teardown}};
    } // namespace
} // namespace pier::api_impl
