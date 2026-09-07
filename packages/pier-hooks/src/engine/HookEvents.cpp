/** hooks/engine/HookEvents.cpp: registry storage, dispatch, and the wiring that attaches
 *  this package to the host dynamic event path as an spi::EventProvider. The module
 *  contract is in hook_events.h.
 */
#include "pier/hooks/hook_events.h"

#include <algorithm>
#include <atomic>
#include <string>
// ll/api/utils/StacktraceUtils.h, reached through ErrorUtils.h below, names
// std::thread::id without including <thread> in 26.32. Pulling it in first is the
// smallest fix on this side of the boundary.
#include <thread>
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
        /** A Meyers singleton, so a static registrar in any TU can fill it safely. */
        std::vector<HookEventDef*>& table()
        {
            static std::vector<HookEventDef*> t;
            return t;
        }

        /** Handle to shared id. The id comes from spi::nextListenerId, a small integer
         *  from 1 upward, which cannot numerically collide with the heap-pointer handles
         *  of another provider. An unsubscribe also passes two checks, being in this
         *  table and matching the mod, so a misread cannot become a wrong removal. */
        PierListenerHandle toHandle(std::uint64_t id)
        {
            return reinterpret_cast<PierListenerHandle>(static_cast<uintptr_t>(id));
        }

        std::uint64_t fromHandle(PierListenerHandle h)
        {
            return static_cast<std::uint64_t>(reinterpret_cast<uintptr_t>(h));
        }

        /** One callback, with an exception caught on the spot. Catching must stay
         *  visible: the exception prints every time and the note that it was contained
         *  prints once per process. A mod exception never unwinds back into the hooked
         *  engine function, which would turn one logic bug into an engine crash in a
         *  half-updated state. */
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
                        "[hooks] a mod callback for a synthetic event threw and was "
                        "contained; this warning prints once, the exception above prints "
                        "every time"
                    );
                }
            }
        }

        /**
         * Extracts the cancel bit from a reply. Parsed, not substring-searched.
         *
         * cancelled:1b, "cancelled":1 and cancelled:1 are all valid SNBT, and the other
         * side produces different shapes depending on whether it round-trips an NbtValue
         * or edits a string. Searching for a substring would mean enumerating every
         * spelling, and missing one makes that form of cancel fail silently while
         * everything else keeps working. CompoundTag::fromSnbt parses it and the tag
         * truth value decides, so the shape difference is irrelevant. A failed parse
         * counts as not cancelled.
         */
        bool replyCancelled(std::string const& reply, std::string_view eventId)
        {
            if (reply.empty()) return false;
            auto tag = CompoundTag::fromSnbt(reply);
            if (!tag)
            {
                // An unparsable reply must not silently count as not cancelled, which is
                // the most dangerous direction: a protection decision reporting a block
                // while it actually lets the action through.
                hostLogger().error(
                    "[hooks] write-back SNBT for synthetic event '{}' failed to parse, treated as not cancelled: {}", eventId,
                    tag.error().message()
                );
                return false;
            }
            if (!tag->contains("cancelled")) return false;
            auto const& v = tag->at("cancelled");
            if (v.is_number()) return static_cast<double>(v) != 0.0;
            return false;
        }

        /** Reads the optional redirect fields out of one reply. Silence means no
         *  redirect: a subscriber that only wants to observe answers with nothing, and
         *  every field here being optional is what lets it. */
        Redirect replyRedirect(std::string const& reply)
        {
            Redirect out;
            if (reply.empty()) return out;
            auto tag = CompoundTag::fromSnbt(reply);
            if (!tag) return out; // replyCancelled already reported the parse failure.
            if (tag->contains("to"))
            {
                auto const& v = tag->at("to");
                if (v.is_number())
                {
                    out.hasDimension = true;
                    out.dimension = static_cast<int>(static_cast<double>(v));
                }
            }
            // The three coordinates move together. A partial position would silently
            // mix a caller axis with an engine axis, and the player lands somewhere
            // neither side asked for.
            if (tag->contains("to_x") && tag->contains("to_y") && tag->contains("to_z"))
            {
                auto const& x = tag->at("to_x");
                auto const& y = tag->at("to_y");
                auto const& z = tag->at("to_z");
                if (x.is_number() && y.is_number() && z.is_number())
                {
                    out.hasPosition = true;
                    out.x = static_cast<float>(static_cast<double>(x));
                    out.y = static_cast<float>(static_cast<double>(y));
                    out.z = static_cast<float>(static_cast<double>(z));
                }
            }
            return out;
        }

        bool sameRedirect(Redirect const& a, Redirect const& b)
        {
            return a.hasDimension == b.hasDimension && a.dimension == b.dimension
                && a.hasPosition == b.hasPosition && a.x == b.x && a.y == b.y && a.z == b.z;
        }

        /** A snapshot of the subscriptions taken before dispatch, since a callback may
         *  modify def.subs mid-dispatch and iterating it directly is undefined behavior.
         *  subs itself stays ordered by priority (see subscribe), so the snapshot order
         *  is the dispatch order. */
        struct SnapEntry
        {
            PierEventCb cb;
            void* user;
            HostedMod* mod;
        };

        std::vector<SnapEntry> snapshot(HookEventDef& def)
        {
            std::vector<SnapEntry> snap;
            snap.reserve(def.subs.size());
            for (auto& sub : def.subs) snap.push_back({sub->cb, sub->user, sub->mod});
            return snap;
        }
    } // namespace

    std::string playerRefSnbt(::Player const& p)
    {
        return "\"_player\":{\"name\":\"" + snbtEscape(p.getRealName())
            + "\",\"xuid\":\"" + snbtEscape(p.getXuid())
            + "\",\"uuid\":\"" + snbtEscape(p.getUuid().asString()) + "\"}";
    }

    HookEventRegistrar::HookEventRegistrar(HookEventDef& def)
    {
        def.idText.assign(def.name);
        table().push_back(&def);
    }

    void dispatchHookEvent(HookEventDef& def, std::string const& snbt)
    {
        auto snap = snapshot(def);
        struct WCtx
        {
        } w; // Observation only: the write-back is a no-op
        for (auto& [cb, user, mod] : snap)
        {
            CallbackScope scope{mod}; // Veto unload during the callback
            callOne(cb, user, def.idText, snbt, &w, [](void*, PierStr) {});
        }
    }

    bool dispatchHookEventCancellable(HookEventDef& def, std::string const& snbt)
    {
        // The same snapshot discipline dispatchHookEvent uses, plus a live write-back
        // sink: a subscriber answering with SNBT carrying the cancel flag vetoes the
        // action.
        auto snap = snapshot(def);
        bool cancelled = false;
        for (auto& [cb, user, mod] : snap)
        {
            CallbackScope scope{mod};
            std::string reply;
            callOne(cb, user, def.idText, snbt, &reply, [](void* ctx, PierStr v)
            {
                if (ctx) *static_cast<std::string*>(ctx) = toString(v);
            });
            if (replyCancelled(reply, def.idText))
            {
                cancelled = true;
                // Keep going: every subscriber must see the event. Stopping early would
                // make whether a listener was called depend on registration order.
            }
        }
        return cancelled;
    }

    Decision dispatchHookEventDecided(HookEventDef& def, std::string const& snbt)
    {
        // The same snapshot discipline as dispatchHookEventCancellable, reading one more
        // thing out of each reply.
        auto snap = snapshot(def);
        Decision decision;
        bool haveRedirect = false;
        for (auto& [cb, user, mod] : snap)
        {
            CallbackScope scope{mod};
            std::string reply;
            callOne(cb, user, def.idText, snbt, &reply, [](void* ctx, PierStr v)
            {
                if (ctx) *static_cast<std::string*>(ctx) = toString(v);
            });
            if (replyCancelled(reply, def.idText))
            {
                decision.cancelled = true;
                // Keep going: every subscriber must see the event. Stopping early would
                // make whether a listener was called depend on registration order.
            }
            Redirect const r = replyRedirect(reply);
            if (!r.any()) continue;
            if (!haveRedirect)
            {
                decision.redirect = r;
                haveRedirect = true;
                continue;
            }
            if (!sameRedirect(decision.redirect, r))
            {
                // Two mods steering the same transfer apart is a configuration problem
                // and not something to resolve silently. The first one keeps winning so
                // the behavior stays deterministic while it is being fixed.
                hostLogger().warn(
                    "[hooks] two subscribers of '{}' asked for different redirects; keeping the first", def.idText
                );
            }
        }
        // A cancel means nothing happens at all, so a redirect alongside it is void.
        if (decision.cancelled) decision.redirect = Redirect{};
        return decision;
    }

    namespace
    {
        /* spi::EventProvider wiring. */

        HookEventDef* findDef(std::string_view wanted)
        {
            for (auto* def : table())
            {
                // An exact name or a unique separated suffix, through spi::idMatches.
                // Never a substring: find(name) != npos would also match
                // "xxFooEventxx".
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
                // A detour that fails to install refuses the subscription, failing
                // closed. Logging an error and issuing a handle anyway would let a mod
                // believe its protection is in place while it never fires once.
                if (!def->install())
                {
                    hostLogger().error(
                        "[hooks] synthetic event '{}': the native detour failed to install, so "
                        "the subscription is refused rather than letting a mod assume it is "
                        "active",
                        def->name
                    );
                    return nullptr;
                }
                def->installed = true;
            }
            std::uint64_t id = spi::nextListenerId();
            auto sub = std::make_unique<HookSub>(HookSub{mod, cb, user, id, priority});
            // Inserted in order: the lower value dispatches first, and equal priorities
            // keep arrival order, so the sort is stable.
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
            return false; // Not this provider's handle, let the next one try
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
            // Purely synthetic; a same-suffix registry id is shadowing and must warn.
            /*covers_registry*/ false,
            &providerClaims,
            &providerSubscribe,
            &providerUnsubscribe,
            &providerDropMod,
            &providerList,
        }};
    } // namespace
} // namespace pier::hooks
