/** runtime/Events.cpp: subscribe_event, unsubscribe_event and list_events.
 *
 * Resolution order (contract §6):
 *   1. An event provider claims through spi::idMatches. Claiming means owning the
 *      outcome, so a failed subscription is reported here and does not fall through.
 *      Providers come before the registry because the emitter of a command event is
 *      in the registry while LL dispatches only to typed listeners, so the dynamic
 *      path finds it but never receives it.
 *   2. The registry by exact name, then by unique suffix.
 *   3. All of them failing reports an error and lists similar ids.
 * spi::idMatches is the only matcher in the repository and nothing here matches on a
 * substring.
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
         * Resolves an event id, allowing a unique suffix match.
         *
         * Deduplicated by name. bus.events() yields (mod, id) pairs rather than unique
         * ids, and every mod that registered an emitter contributes one entry. Counting
         * entries instead would make an unambiguous name resolve as ambiguous as soon
         * as a second mod on the server touches the same event, and the subscription
         * fails. The caller sees only an Err, and a mod chaining listener registrations
         * with ? loses every listener after that point.
         *
         * Two entries under the same name are not ambiguity. Genuinely different names
         * are.
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
                // The roles are reversed from the usual call. `name` from the registry
                // is the long full name and `wanted` from the caller is the short one,
                // so the question is whether name is the separated full form of
                // wanted.
                if (spi::idMatches(name, wanted) || name == wanted)
                {
                    // An entry of the same name registered by another mod, which is
                    // not ambiguity.
                    if (hit && std::string_view(hit->name) != name) return std::nullopt;
                    if (!hit) hit.emplace(ll::event::EventId{name});
                }
            }
            return hit;
        }

        /**
         * On a failed subscription, prints every registry id that looks related, so the
         * error names what the engine actually calls this event instead of saying only
         * "unknown". One line, on the failure path only.
         */
        void reportSimilarEvents(HostedMod* mod, std::string_view wanted)
        {
            // The needle is the last long CamelCase word, which for
            // "PlayerPlacingBlockEvent" is "Block". Loose enough to survive a rename,
            // tight enough not to dump the whole registry.
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
                if (found.find(name) != std::string::npos) continue; // Deduplicate
                if (n++) found += ", ";
                found += name;
                if (n >= 12) break;
            }
            if (found.empty()) found = "(no id in the registry contains '" + needle + "')";
            mod->getLogger().error("[api] subscribe_event: ids containing '{}': {}", needle, found);
        }

        ll::event::EventPriority mapPriority(int32_t priority)
        {
            // The ABI states 0..4, Highest through Lowest, while LeviLamina uses 0,
            // 100, 200, 300 and 400.
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

        /** Shadowing warning for a purely synthetic provider, one with
         *  covers_registry=false. It fires when the registry holds a real event that
         *  idMatches against one of the provider's canonical names. Once per pair:
         *  shadowing must be visible, but not on every subscription. */
        void warnIfShadowing(HostedMod* mod, spi::EventProvider const& provider,
                             std::string_view wanted)
        {
            static std::unordered_set<std::string> warned; // Server thread only

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
                        "[api] subscribe_event('{}'): synthetic event '{}' from provider "
                        "{} shadows '{}' in the registry, which means upstream introduced "
                        "a real event of that name; this subscription still binds the "
                        "synthetic one, and the real event needs its full id",
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

                //  1. Providers (contract §6)
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
                        // Claiming means owning the outcome, so this does not fall
                        // through to the registry. Silently switching to an event with
                        // a completely different payload shape is worse than an
                        // explicit failure.
                        mod->getLogger().error(
                            "[api] subscribe_event: provider {} claimed '{}' but the subscription failed, see the lines above",
                            fctx.hit->name,
                            wanted
                        );
                    }
                    return h;
                }

                //  2. Registry plus DynamicListener
                auto resolved = resolveEventId(wanted);
                if (!resolved)
                {
                    mod->getLogger().error("[api] subscribe_event: unknown or ambiguous event id '{}'", wanted);
                    reportSimilarEvents(mod, wanted);
                    return nullptr;
                }

                std::string idName = resolved->name;
                std::weak_ptr<HostedMod> weakMod = mod->shared_from_this();
                auto listener = ll::event::DynamicListener::create(
                    [cb, user, idName, weakMod](CompoundTag& data)
                    {
                        // Recheck plus callback count. If the mod is already unloaded
                        // the listener should have been removed, and this is the last
                        // gate. If it is alive, an unload is held off until the
                        // callback returns.
                        auto owner = weakMod.lock();
                        if (!owner) return;
                        CallbackScope scope{owner.get()};

                        std::string snbt = bridge::enrichEventData(data);

                        struct WriteCtx
                        {
                            CompoundTag* data;
                            std::string const* snapshot; // Exactly what was handed to cb
                            HostedMod* mod;
                            std::string const* eventId;
                        } wctx{&data, &snbt, owner.get(), &idName};

                        cb(
                            user,
                            ps(idName),
                            ps(snbt),
                            &wctx,
                            [](void* c, PierStr newSnbt)
                            {
                                auto* w = static_cast<WriteCtx*>(c);
                                auto edited = CompoundTag::fromSnbt(sv(newSnbt));
                                if (!edited)
                                {
                                    // Never silent. The mod believes it cancelled or
                                    // rewrote the event while nothing happened, which
                                    // is exactly where a reported-but-not-actual block
                                    // comes from. The parse error is printed together
                                    // with the event name.
                                    w->mod->getLogger().error(
                                        "[api] write-back SNBT for event '{}' failed to parse, the edit is ignored: {}",
                                        *w->eventId,
                                        edited.error().message()
                                    );
                                    return;
                                }

                                // Only fields this caller changed relative to its own
                                // snapshot are written. Replacing the whole tree loses
                                // updates when two mods subscribe to one event: the
                                // second callback sees the state from before the first
                                // edit and writing it back reverts that edit, with no
                                // error and no log line. Diffing keeps them independent
                                // and never writes an untouched field; a conflict on one
                                // field is still last-writer-wins.
                                auto base = CompoundTag::fromSnbt(*w->snapshot);
                                if (!base)
                                {
                                    // The snapshot failed to parse, which is close to
                                    // impossible since it came from toSnbt. Falling back
                                    // to whole-tree semantics beats dropping the edit.
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
                                // A key present in the snapshot and absent from the
                                // write-back counts as unchanged and is never deleted.
                                // Event::deserialize in LL reads keys through
                                // CompoundTag::operator[] const, and a missing key
                                // throws std::out_of_range and aborts the whole
                                // deserialization, so a mod writing back partial SNBT
                                // such as {cancelled:1b} would lose even the cancel bit.
                                // Deleting a key is never a valid event edit.
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
                        "[api] subscribe_event: '{}' resolved but addListener failed", idName
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

                // Providers holding their own handles, such as hooks, are asked first.
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

                // Those on the bus. Both dynamic listeners and the typed listeners of
                // command events are recorded in mod->listeners.
                auto wantedId = spi::idOf(handle);
                for (auto it = mod->listeners.begin(); it != mod->listeners.end(); ++it)
                {
                    if (it->id == wantedId)
                    {
                        // When removal fails the host's own record is dropped anyway,
                        // since that record belongs to the host, but it must be
                        // reported: the listener is still on the bus and its callback
                        // points into this dylib. Returning false silently would keep
                        // the unload-time removeListener pass from ever trying it.
                        bool ok = ll::event::EventBus::getInstance().removeListener(it->listener);
                        if (!ok)
                        {
                            mod->getLogger().error(
                                "[api] unsubscribe_event: listener {} could not be removed "
                                "from the event bus; it may still be dispatched while its "
                                "callback points into this dylib",
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
                // Events synthesized by a provider are subscribable too and are listed
                // alongside.
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
                        return false; // Visit every provider
                    },
                    &lctx
                );
            PIER_API_GUARD_END_VOID
        }

        /**
         * Teardown. Clears the subscriptions providers hold under this mod.
         * On the unload path ModHost can only remove what is recorded in
         * mod->listeners, meaning the dynamic listeners on the LL event bus and the
         * typed listeners of command events. Subscriptions belonging to a provider,
         * such as the synthetic events of hooks, live in the provider's own table and
         * the host does not know them. Without an explicit dropMod here those entries
         * outlive FreeLibrary while holding callback pointers into a code section that
         * was just unmapped, and the next time that hook point fires it crashes without
         * diagnostics, on some unrelated player's action.
         * Stage 80 runs after forms at 60 and kvdb at 70, because a synthetic event
         * callback may touch a form or a kv database while being removed. There is no
         * dependency between it and packet-hooks at 90.
         */
        void teardown(HostedMod* mod)
        {
            spi::forEachEventProvider(
                [](spi::EventProvider const& p, void* raw)
                {
                    p.dropMod(static_cast<HostedMod*>(raw));
                    return false; // Visit every provider
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
