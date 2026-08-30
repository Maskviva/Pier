/** runtime/CommandEvents.cpp —— 命令事件提供方（服务端专属）。
 *
 * ExecutingCommandEvent / ExecutedCommandEvent 的发射器**在**动态注册表里，
 * 但 LeviLamina 只把这两个事件派发给类型化监听器 —— DynamicListener 挂上去
 * 一个回调都收不到。所以它们以事件提供方（spi §5）的身份接进解析：
 * `covers_registry = true` —— 替换注册表路径是修复，不是遮蔽，不告警。
 *
 * 类型化监听器要自己建、再用注册表里解析出来的真 id 注册：这两个事件类型
 * 住在 inline namespace（ll::event::inline command）里，getEventId<T> 算出
 * 的 id 带 "command::" 段，而 LL 的发射器注册在**去 inline** 的名字下
 * （ll::event::ExecutingCommandEvent，/pier events 可见）。emplaceListener<T>
 * 按前者查、按前者挂，于是失败。手建 Listener<T> + 非模板 addListener
 * 保住类型化回调（直接从 CommandContext 的 origin 读玩家和命令文本），
 * 又修掉 id 错位。
 */
#ifndef PIER_BUILD_CLIENT

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "ll/api/event/EventBus.h"
#include "ll/api/event/Listener.h"
#include "ll/api/event/command/ExecuteCommandEvent.h"
#include "ll/api/utils/ErrorUtils.h"

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/platform/UUID.h"
#include "mc/server/commands/CommandOrigin.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/player/Player.h"

#include "sdk/abi.h"

#include "pier/host/hosted_mod.h"
#include "pier/host/spi.h"
#include "pier/support/log.h"
#include "pier/support/snbt.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        constexpr std::string_view kExecuting = "ExecutingCommandEvent";
        constexpr std::string_view kExecuted = "ExecutedCommandEvent";

        ll::event::EventPriority mapPriority(int32_t priority)
        {
            // 与 Events.cpp 同一张映射（ABI 0..4 → LL 0/100/200/300/400）。
            // 十行的重复换提供方自含 —— 两边真分岔时症状是优先级错乱，
            // 一眼可见，比共享一个「事件域杂项头」划算。
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

        /** 在注册表里找与 canonical 同族的真 id（去 inline 的全名）。 */
        std::string resolveRegistryName(std::string_view canonical)
        {
            for (auto&& [modName, id] : ll::event::EventBus::getInstance().events())
            {
                std::string_view name = id.name;
                if (name == canonical || spi::idMatches(name, canonical))
                {
                    return std::string(name);
                }
            }
            return {};
        }

        /** 返回 true = 有订阅方要求拒绝这条命令。
         *
         *  写回曾在这里被一句 `write-back ignored` 丢掉。那让
         *  ExecutingCommandEvent 静默变成**只读**，尽管 LL 侧它是
         *  `Cancellable<ExecuteCommandEvent>`：订阅方调 `ev.cancel()`、
         *  哪儿都看不到报错、然后眼看着命令照跑。架在它上面的任何闸 ——
         *  比如命令白名单 —— 报告自己已就位，实际什么都不拦：这是安全检查
         *  最糟的失败形状。 */
        bool dispatchCommand(
            PierEventCb cb,
            void* user,
            std::string const& idName,
            std::string const& playerName,
            std::string const& xuid,
            std::string const& uuid,
            std::string const& command)
        {
            // 控制台、命令方块和一切非玩家来源**完全不上报**。这是刻意的，
            // 而且是拿命令做闸的调用方赖以生存的：把控制台也拒了，服主会被
            // 锁在自己的服务器外面，没有任何退路。
            if (playerName.empty()) return false;

            std::string snbt = "{\"eventId\":\"" + idName
                + "\",\"name\":\"" + snbtEscape(playerName)
                + "\",\"command\":\"" + snbtEscape(command)
                + "\",\"_player\":{\"name\":\"" + snbtEscape(playerName)
                + "\",\"xuid\":\"" + snbtEscape(xuid)
                + "\",\"uuid\":\"" + snbtEscape(uuid) + "\"}}";

            struct WriteCtx
            {
                bool cancelled = false;
            } wctx{};
            cb(user, ps(idName), ps(snbt), &wctx,
               [](void* c, PierStr newSnbt)
               {
                   // 另一侧把整个事件带着翻转的 `cancelled` 写回来（和动态
                   // 路径同一个动作）。这里只读那一个字段：命令事件上没有别
                   // 的值得写的，而「从 SNBT 重建 CommandContext」不存在。
                   auto* w = static_cast<WriteCtx*>(c);
                   auto tag = CompoundTag::fromSnbt(sv(newSnbt));
                   if (!tag || !tag->contains("cancelled")) return;
                   try
                   {
                       if (static_cast<uchar>(tag->at("cancelled")) != 0) w->cancelled = true;
                   }
                   catch (...)
                   {
                       // W11：`cancelled` 不是字节 tag（或取值抛了）。事件按
                       // 未取消继续 —— 但要说出来，不许藏。
                       ll::error_utils::printCurrentException(hostLogger());
                   }
               });
            return wctx.cancelled;
        }

        struct OriginIdentity
        {
            std::string name;
            std::string xuid;
            std::string uuid;
        };

        template <typename Ctx>
        OriginIdentity identityOf(Ctx const& ctx)
        {
            OriginIdentity id;
            // 基类 ExecuteCommandEvent::commandContext() 返回 const 引用；
            // mOrigin 是指针成员，const 的是指针不是指向物 —— 非 const 的
            // getEntity() 照常可用。
            if (ctx.mOrigin && ctx.mOrigin->getEntity())
            {
                auto* entity = ctx.mOrigin->getEntity();
                if (entity->isPlayer())
                {
                    auto* p = static_cast<Player*>(entity);
                    id.name = p->getRealName();
                    id.xuid = p->getXuid();
                    id.uuid = p->getUuid().asString();
                }
            }
            return id;
        }

        bool claims(std::string_view wanted)
        {
            return spi::idMatches(wanted, kExecuting) || spi::idMatches(wanted, kExecuted);
        }

        PierListenerHandle subscribe(
            HostedMod* mod, std::string_view wanted, int32_t priority, PierEventCb cb, void* user)
        {
            bool const isExecuting = spi::idMatches(wanted, kExecuting);
            auto const canonical = isExecuting ? kExecuting : kExecuted;

            std::string idName = resolveRegistryName(canonical);
            if (idName.empty())
            {
                mod->getLogger().error(
                    "subscribe_event: 命令事件 '{}' 在注册表里找不到发射器条目 —— "
                    "这个 BDS/LL 版本上它可能改了名（/pier events 可核对）",
                    wanted
                );
                return nullptr;
            }

            auto prio = mapPriority(priority);
            std::shared_ptr<ll::event::ListenerBase> typedListener;
            if (isExecuting)
            {
                typedListener = ll::event::Listener<ll::event::command::ExecutingCommandEvent>::create(
                    [cb, user, idName](ll::event::command::ExecutingCommandEvent& ev)
                    {
                        auto who = identityOf(ev.commandContext());
                        if (dispatchCommand(cb, user, idName, who.name, who.xuid, who.uuid,
                                            ev.commandContext().mCommand))
                        {
                            ev.cancel();
                        }
                    },
                    prio,
                    mod->shared_from_this()
                );
            }
            else
            {
                typedListener = ll::event::Listener<ll::event::command::ExecutedCommandEvent>::create(
                    [cb, user, idName](ll::event::command::ExecutedCommandEvent& ev)
                    {
                        auto who = identityOf(ev.commandContext());
                        // ExecutedCommandEvent 不可取消 —— 命令已经跑完了。
                        // 否决位丢弃而非记日志：在这里 cancel 的订阅方要的是
                        // 事件表达不了的东西，而每条命令一行日志是日志洪水，
                        // 不是诊断。
                        (void)dispatchCommand(cb, user, idName, who.name, who.xuid, who.uuid,
                                              ev.commandContext().mCommand);
                    },
                    prio,
                    mod->shared_from_this()
                );
            }

            // 用注册表里的真 id 注册（不是 getEventId<T> 算出来的那个）。
            if (!typedListener
                || !ll::event::EventBus::getInstance().addListener(
                    typedListener, ll::event::EventIdView{idName}))
            {
                mod->getLogger().error(
                    "subscribe_event: 类型化命令监听器挂 '{}' 失败", idName
                );
                return nullptr;
            }
            // 记进 mod->listeners：卸载时宿主的 W-EV1 清扫和通用退订路径
            // 就都覆盖它 —— 提供方自己不必再管生命周期。
            std::uint64_t id = spi::nextListenerId();
            mod->listeners.push_back({id, typedListener});
            return spi::handleOf(id);
        }

        bool unsubscribe(HostedMod*, PierListenerHandle)
        {
            // 句柄在 mod->listeners 里，Events 的通用路径负责摘 —— 这里
            // 说「不是我的」，让它继续走。
            return false;
        }

        void dropMod(HostedMod*)
        {
            // 同上：宿主卸载时对 mod->listeners 逐个 removeListener（W-EV1），
            // 命令监听器一并被摘。无自持状态。
        }

        void list(void* ctx, PierStrSink sink)
        {
            sink(ctx, ps(kExecuting));
            sink(ctx, ps(kExecuted));
        }

        spi::EventProviderReg reg{{
            "command-events",
            /*covers_registry=*/true,
            &claims,
            &subscribe,
            &unsubscribe,
            &dropMod,
            &list,
        }};
    } // namespace
} // namespace pier::api_impl

#endif // !PIER_BUILD_CLIENT
