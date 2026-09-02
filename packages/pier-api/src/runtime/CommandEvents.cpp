/** runtime/CommandEvents.cpp: the command event provider, server only.
 * The emitters of ExecutingCommandEvent and ExecutedCommandEvent are in the dynamic
 * registry, but LeviLamina dispatches these two only to typed listeners, so a
 * DynamicListener attached to them receives nothing. They are therefore spliced into
 * resolution as an event provider (spi §5) with covers_registry = true, since replacing
 * the registry path repairs it rather than shadowing it and must not warn.
 * The typed listener is built by hand and registered under the real id resolved from
 * the registry. Both event types live in an inline namespace,
 * ll::event::inline command, so the id getEventId<T> computes carries a "command::"
 * segment while the LL emitter is registered under the de-inlined name,
 * ll::event::ExecutingCommandEvent, which /pier events shows. emplaceListener<T> looks
 * up and attaches by the former and therefore fails. Building a Listener<T> by hand and
 * using the non-template addListener keeps the typed callback, which reads the player
 * and the command text straight from the CommandContext origin, and fixes the id
 * mismatch.
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
            // The same mapping Events.cpp uses, from ABI 0..4 to LL 0, 100, 200, 300
            // and 400. Ten duplicated lines buy a self-contained provider, and if the
            // two ever diverge the symptom is visibly scrambled priorities, which is a
            // better trade than a shared miscellaneous header for the event domain.
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

        /** Finds the real id in the registry belonging to the same family as
         *  canonical, meaning the de-inlined full name. */
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

        /** True when a subscriber asked for this command to be refused.
         *
         *  The write-back must be honored here. Dropping it makes
         *  ExecutingCommandEvent silently read-only even though LL declares it
         *  `Cancellable<ExecuteCommandEvent>`: a subscriber calls `ev.cancel()`, sees
         *  no error anywhere, and watches the command run. Any gate built on top of
         *  it, such as a command whitelist, then reports itself in place while
         *  blocking nothing, which is the worst failure shape a security check has. */
        bool dispatchCommand(
            PierEventCb cb,
            void* user,
            std::string const& idName,
            std::string const& playerName,
            std::string const& xuid,
            std::string const& uuid,
            std::string const& command)
        {
            // The console, command blocks and every non-player origin are not reported
            // at all. That is deliberate and is what a caller gating on commands
            // depends on: refusing the console too would lock an operator out of their
            // own server with no way back in.
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
                   // The other side writes the whole event back with `cancelled`
                   // flipped, the same action the dynamic path performs. Only that one
                   // field is read here: nothing else on a command event is worth
                   // writing, and rebuilding a CommandContext from SNBT does not
                   // exist.
                   auto* w = static_cast<WriteCtx*>(c);
                   auto tag = CompoundTag::fromSnbt(sv(newSnbt));
                   if (!tag || !tag->contains("cancelled")) return;
                   try
                   {
                       if (static_cast<uchar>(tag->at("cancelled")) != 0) w->cancelled = true;
                   }
                   catch (...)
                   {
                       // `cancelled` is not a byte tag, or reading it threw. The event
                       // continues as not cancelled, and that is reported rather than
                       // hidden.
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
            // The base ExecuteCommandEvent::commandContext() returns a const reference,
            // and mOrigin is a pointer member, so what is const is the pointer and not
            // the pointee, leaving the non-const getEntity() usable.
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
                    "[api] subscribe_event: no emitter entry for command event '{}' in "
                    "the registry; it may have been renamed on this BDS or LL version, "
                    "check /pier events",
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
                        // ExecutedCommandEvent cannot be cancelled, since the command
                        // has already run. The veto bit is discarded rather than
                        // logged: a subscriber cancelling here wants something the
                        // event cannot express, and one line per command is a flood
                        // rather than a diagnostic.
                        (void)dispatchCommand(cb, user, idName, who.name, who.xuid, who.uuid,
                                              ev.commandContext().mCommand);
                    },
                    prio,
                    mod->shared_from_this()
                );
            }

            // Registered under the real id from the registry, not the one
            // getEventId<T> computes.
            if (!typedListener
                || !ll::event::EventBus::getInstance().addListener(
                    typedListener, ll::event::EventIdView{idName}))
            {
                mod->getLogger().error(
                    "[api] subscribe_event: the typed command listener failed to attach to '{}'", idName
                );
                return nullptr;
            }
            // Recorded in mod->listeners, so the host's unload sweep and the general
            // unsubscribe path both cover it and the provider itself carries no
            // lifetime duty.
            std::uint64_t id = spi::nextListenerId();
            mod->listeners.push_back({id, typedListener});
            return spi::handleOf(id);
        }

        bool unsubscribe(HostedMod*, PierListenerHandle)
        {
            // The handle lives in mod->listeners and the general path in Events removes
            // it, so this reports that the handle is not its own and lets that path
            // continue.
            return false;
        }

        void dropMod(HostedMod*)
        {
            // As above: on unload the host calls removeListener for each entry in
            // mod->listeners, which takes the command listener with it. No state is
            // held here.
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
