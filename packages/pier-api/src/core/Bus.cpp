/** core/Bus.cpp: the cross-mod event bus.
 *
 * The table belongs to the host. The intuitive design, where mod A exports subscribe
 * and mod B hands over a callback, cannot be made safe: ModHost::unload calls
 * FreeLibrary, so once B is unloaded A holds a function pointer into an unmapped
 * dylib and the next publish crashes with no diagnostics. Every asynchronous surface
 * here answers that the same way. The host owns the table, entries are numbered by
 * ticket, and the firing path holds a weak_ptr<HostedMod> and revalidates immediately
 * before the call.
 *
 * topic and payload are opaque UTF-8 agreed out of band. Two cycle gates of different
 * shapes apply: a mod never receives its own publish, and a nesting depth limit
 * catches A to B to A, dropping the innermost publish and logging once. Subscribers
 * are snapshotted under the lock and every callback runs after it is released, since
 * a subscriber that re-enters while the lock is held deadlocks the server thread.
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
        /** Longest topic accepted. Long enough for `some-long-mod:some-event`, short
         *  enough that a garbage pointer read as a string cannot become a
         *  multi-megabyte map key. */
        constexpr size_t kMaxTopic = 128;

        /** Nesting limit for dispatch. 8 is far beyond any real chain, since publish
         *  to handler to publish is depth 2. Anything deeper is a cycle. */
        constexpr int kMaxDepth = 8;

        struct Subscription
        {
            HostedMod* mod = nullptr; // Identity comparison only, never dereferenced
            /** Weak reference taken at subscribe time. fireOne rechecks liveness only
             *  through it. Calling shared_from_this() from the raw pointer would itself
             *  be a blind dereference, and once the mod is unloaded on another thread
             *  that memory belongs to nobody. Same as in Services.cpp. */
            std::weak_ptr<HostedMod> owner;
            std::string topic;
            PierBusCb cb = nullptr;
            void* user = nullptr;
        };

        std::mutex gBusMutex;
        std::unordered_map<uint64_t, Subscription> gSubs;
        /** topic to subscription ids, so publish does not scan the whole table. Kept
         *  in sync with gSubs under the same lock. */
        std::unordered_map<std::string, std::vector<uint64_t>> gByTopic;
        uint64_t gNextSubId = 1;

        /** Nesting depth per thread. thread_local rather than a global counter,
         *  because two threads publishing concurrently are not a cycle and a shared
         *  counter would read them as one. */
        thread_local int gDepth = 0;

        struct DepthGuard
        {
            DepthGuard() { ++gDepth; }
            ~DepthGuard() { --gDepth; }
        };

        /** Warns about excessive depth once per topic and then stays quiet. A cycle
         *  spins as fast as the CPU, and logging on every hit turns a bug into an
         *  incident. */
        void warnDepthOnce(std::string const& topic)
        {
            static std::mutex mu;
            static std::unordered_map<std::string, bool> seen;
            std::lock_guard lock(mu);
            if (seen[topic]) return;
            seen[topic] = true;
            hostLogger().error(
                "[bus] topic '{}' exceeded nesting depth {}, innermost publish dropped; "
                "this is a publish cycle, where a subscriber of this topic publishes it "
                "again, either directly or through another topic that loops back",
                topic, kMaxDepth
            );
        }

        /** Snapshots the ids subscribed to `topic`, excluding the publisher's own. */
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
                if (s->second.mod == publisher) continue; // No self-delivery
                out.push_back(id);
            }
            return out;
        }

        /**
         * Fires one subscription by ticket. Returns the veto bit, and sets `ran` when
         * the callback really executed.
         *
         * The lookup and the call are deliberately separated. The entry is copied out
         * under the lock, the lock is released, and only then does control cross into
         * the dylib.
         */
        bool fireOne(uint64_t id, std::string_view topic, std::string_view payload, bool& ran)
        {
            ran = false;
            Subscription sub;
            {
                std::lock_guard lock(gBusMutex);
                auto it = gSubs.find(id);
                // Gone, either unsubscribed by an earlier subscriber in this same
                // dispatch or because its mod was unloaded mid-dispatch.
                if (it == gSubs.end()) return false;
                sub = it->second;
            }
            if (!sub.cb || !sub.mod) return false;

            // Rechecked only through the weak_ptr stored at subscribe time. When the
            // mod has been destroyed lock() yields empty and no freed memory is
            // touched. Pointer equality is compared as well, in case a new mod happens
            // to land at the old address.
            auto mod = sub.owner.lock();
            if (!mod || mod.get() != sub.mod) return false; // dylib may be unmapped
            if (!mod->acceptsCallbacks()) return false;            // Muted while disabled

            // The shared_ptr and the callback count are held until the callback
            // returns, so ModHost vetoes an unload rather than pulling the code section
            // away mid-dispatch.
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
                // A veto does not short-circuit, because observers must see a
                // consistent stream. Skipping the rest would make what a mod observes
                // depend on subscriber order, which nobody controls.
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

                // A mod not yet owned by a shared_ptr is refused. Without a weak_ptr
                // it cannot be revalidated later, and calling into a dylib without
                // revalidation is exactly what this table exists to prevent.
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
                // Scoped to the caller. A mod may not silence another one.
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

        /** Teardown. Clears every subscription held under this mod. */
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
