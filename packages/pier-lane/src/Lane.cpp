/**
 * pier-lane/Lane.cpp: the same-toolchain fast lane.
 *
 * service_call has the shape (name, UTF-8) -> UTF-8, the greatest common divisor
 * across languages. Between two mods in one language it serializes on every decision,
 * loses all type information inside the string, and turns a misspelled field name
 * into "this player has no permission". This lane serves one case, where both sides
 * came out of the same toolchain build, so the C-layout function tables in the two
 * cdylibs are byte-identical and a pointer can be handed over directly.
 *
 * The host takes part for the reason Bus.cpp gives, one step further: Bus risks a
 * dangling callback pointer, this risks a whole dangling function table. It provides
 * a never-freed liveness cell on the host heap that FreeLibrary cannot reach, so
 * reading it after an unload stays legal, and a release call for every outstanding
 * lease at unload, before FreeLibrary. It interprets no byte of data or vtable.
 */
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
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
#include "pier/support/snbt.h"
#include "pier/support/str.h"

namespace pier::lane
{
    namespace
    {
        /** The same cap as a service name, for the same reason. Long enough for
         *  `some-mod:some-lane`, short enough that a wild pointer read as a string
         *  cannot become a multi-megabyte map key. */
        constexpr size_t kMaxName = 128;

        /** Minimal JSON string escaping. lane_list assembles JSON by hand and the
         *  names are external input. */
        std::string jsonEscape(std::string_view s)
        {
            std::string out;
            out.reserve(s.size() + 8);
            for (char c : s)
            {
                switch (c)
                {
                case '"':
                    out += "\\\"";
                    break;
                case '\\':
                    out += "\\\\";
                    break;
                case '\n':
                    out += "\\n";
                    break;
                case '\r':
                    out += "\\r";
                    break;
                case '\t':
                    out += "\\t";
                    break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20)
                    {
                        char buf[8];
                        std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                        out += buf;
                    }
                    else
                    {
                        out.push_back(c);
                    }
                    break;
                }
            }
            return out;
        }

        /**
         * The liveness flag of one lane.
         *
         * A separate heap allocation that is never deleted. The address must stay
         * readable after FreeLibrary on the provider, because reading it is how a
         * consumer discovers the provider is gone. Freeing the cell would turn that
         * check itself into a use-after-free.
         */
        struct AliveCell
        {
            std::atomic<uint32_t> flag{1};
            /** Number of consumers currently sitting inside an entry of this lane.
             *  See PierLaneRef::busy. It shares the cell with flag and is likewise
             *  never freed, because the unload path reads it at a moment when the
             *  consumer holding it may already be gone. */
            std::atomic<uint32_t> busy{0};
        };

        struct Lane
        {
            HostedMod* mod = nullptr; // Identity only. Compared, never dereferenced.
            // Liveness is rechecked through this weak_ptr and not through
            // `mod->shared_from_this()`, which would itself be a blind dereference.
            // On the main thread unload and calls are serialized so they cannot
            // collide, but acquire may be called from another thread.
            std::weak_ptr<HostedMod> owner;
            std::string name;
            PierLaneDesc desc{};
            AliveCell* alive = nullptr; // Leaked on purpose, see above
            uint32_t leases = 0;
        };

        struct Lease
        {
            HostedMod* holder = nullptr; // The consumer
            uint64_t laneId = 0;
        };

        std::mutex gMutex;
        std::unordered_map<uint64_t, Lane> gLanes;         // publish id -> lane
        std::unordered_map<std::string, uint64_t> gByName; // name -> publish id, exclusive
        std::unordered_map<uint64_t, Lease> gLeases;       // lease id -> lease
        uint64_t gNextLaneId = 1;
        uint64_t gNextLeaseId = 1;

        /**
         * Whether the provider mod is still there. As in Bus and Services, liveness is
         * rechecked through a weak_ptr and the raw pointer is not trusted.
         *
         * isEnabled() is deliberately not consulted. ModManager::enable() calls the
         * onEnable callback first and flips the state to Enabled only after it
         * returns, so isEnabled() is false whenever a mod publishes a lane from its
         * own on_load or on_enable, which are the only sensible moments to publish.
         * Consulting it would make publishing impossible and report a name clash.
         *
         * It would protect little anyway. Calling into an unmapped code section is
         * covered by the liveness flag and by retireLane, neither depending on enabled.
         * A mod wanting its lane stopped on disable unpublishes it in on_disable.
         */
        bool providerAlive(Lane const& lane)
        {
            if (!lane.mod) return false;
            auto mod = lane.owner.lock();
            return mod && mod.get() == lane.mod;
        }

        /**
         * Retires one lane. Clears the liveness flag, releases every outstanding
         * lease on the provider's behalf, then removes the table entries.
         *
         * gMutex must not be held across this call. `release` jumps into the
         * provider's dylib, which may well call back into `lane_*`, for instance
         * unpublishing another of its own lanes from a Drop. Holding a lock across a
         * dylib boundary deadlocks on the first re-entry.
         */
        void retireLane(uint64_t laneId)
        {
            PierLaneRefFn release = nullptr;
            void* data = nullptr;
            uint32_t outstanding = 0;
            std::string name;

            {
                std::lock_guard lock(gMutex);
                auto it = gLanes.find(laneId);
                if (it == gLanes.end()) return;
                Lane& lane = it->second;

                // Cut the power before the wiring. From this moment a consumer that
                // has not yet reached its call site sees the lane as gone rather
                // than as a table about to become invalid.
                if (lane.alive) lane.alive->flag.store(0, std::memory_order_release);

                release = lane.desc.release;
                data = lane.desc.data;
                name = lane.name;

                for (auto it2 = gLeases.begin(); it2 != gLeases.end();)
                {
                    if (it2->second.laneId == laneId)
                    {
                        ++outstanding;
                        it2 = gLeases.erase(it2);
                    }
                    else
                    {
                        ++it2;
                    }
                }

                auto byName = gByName.find(lane.name);
                if (byName != gByName.end() && byName->second == laneId) gByName.erase(byName);
                gLanes.erase(it);
                // The AliveCell is deliberately not deleted, see the file header.
            }

            // Outside the lock. release is called once per outstanding lease and
            // for nothing else, which is all abi.h promises under lane_publish and
            // lane_unpublish. The reference handed over at publish time is reclaimed
            // by the provider itself after unpublish. Releasing that one here too
            // would decrement a provider that implements refcounting from the
            // documentation once too often, underflowing into a use-after-free.
            if (release)
            {
                for (uint32_t i = 0; i < outstanding; ++i) release(data);
            }
            if (outstanding > 0)
            {
                hostLogger().warn(
                    "[lane] '{}' retired with {} lease(s) still outstanding, released on "
                    "behalf of the consumers; a consumer mod is expected to drop its lane "
                    "handles before unload and at least one did not",
                    name, outstanding
                );
            }
        }

        uint64_t api_lane_publish(PierModHandle modHandle, PierStr nameRaw, PierLaneDesc const* desc)
        {
            PIER_API_GUARD_BEGIN
                if (!modHandle || !desc) return 0;
                auto* raw = asMod(modHandle);
                if (!raw) return 0;

                if (desc->struct_size < sizeof(PierLaneDesc))
                {
                    hostLogger().error(
                        "[lane] publish refused, PierLaneDesc is smaller than the host "
                        "expects ({} < {}); the mod was built against an older ABI",
                        desc->struct_size, static_cast<uint32_t>(sizeof(PierLaneDesc))
                    );
                    return 0;
                }
                if (desc->protocol != PIER_LANE_PROTOCOL)
                {
                    hostLogger().error(
                        "[lane] publish refused, protocol {} does not match the host's {}; "
                        "only this lane is affected, the mod still loads and consumers fall "
                        "back to the service channel",
                        desc->protocol, PIER_LANE_PROTOCOL
                    );
                    return 0;
                }
                if (!desc->vtable) return 0;
                if (desc->fingerprint == 0)
                {
                    // On the acquire side 0 used to mean "do not verify". A provider
                    // reporting 0 would dismantle the gate itself, and the result of
                    // dismantling it is silent memory corruption rather than a crash.
                    hostLogger().error("[lane] publish refused, fingerprint 0 is reserved");
                    return 0;
                }

                std::string name = toString(nameRaw);
                if (name.empty() || name.size() > kMaxName) return 0;

                // Only the mod itself can publish, on the main thread, where it is
                // certainly alive. The weak_ptr is taken here and only here.
                std::weak_ptr<HostedMod> owner;
                try
                {
                    owner = raw->shared_from_this();
                }
                catch (std::bad_weak_ptr const&)
                {
                    return 0;
                }
                if (!owner.lock()) return 0;

                std::lock_guard lock(gMutex);
                auto taken = gByName.find(name);
                if (taken != gByName.end())
                {
                    // A hard failure, as with a service. Two mods providing the same
                    // lane is an ambiguous answer a consumer cannot choose between,
                    // not both of them running. Letting the later one win silently
                    // would make the result depend on mod load order, and that order
                    // changes as soon as any unrelated mod is installed.
                    auto held = gLanes.find(taken->second);
                    std::string holder = held != gLanes.end() && held->second.mod
                                             ? std::string(held->second.mod->getName())
                                             : std::string("<unknown>");
                    hostLogger().error(
                        "[lane] '{}' is already published by mod '{}', second publisher refused",
                        name, holder
                    );
                    return 0;
                }

                uint64_t id = gNextLaneId++;
                Lane lane;
                lane.mod = raw;
                lane.owner = owner;
                lane.name = name;
                lane.desc = *desc;
                lane.desc.struct_size = static_cast<uint32_t>(sizeof(PierLaneDesc));
                lane.alive = new AliveCell(); // Leaked on purpose
                gLanes.emplace(id, lane);
                gByName.emplace(name, id);

                hostLogger().debug(
                    "[lane] '{}' published, fingerprint 0x{:016x}", name, desc->fingerprint
                );
                return id;
            PIER_API_GUARD_END
        }

        bool api_lane_unpublish(PierModHandle modHandle, uint64_t pubId)
        {
            PIER_API_GUARD_BEGIN
                if (!modHandle || pubId == 0) return false;
                auto* raw = asMod(modHandle);
                {
                    std::lock_guard lock(gMutex);
                    auto it = gLanes.find(pubId);
                    // Scoped to the caller. A mod cannot unpublish another mod's lane.
                    if (it == gLanes.end() || it->second.mod != raw) return false;
                }
                retireLane(pubId);
                return true;
            PIER_API_GUARD_END
        }

        int32_t api_lane_acquire(
            PierModHandle modHandle, PierStr nameRaw, uint64_t wantFingerprint, PierLaneRef* out)
        {
            PIER_API_GUARD_BEGIN
                // Only up to `alive` is required, which is the minimum shape a lane
                // needs to be usable. Requiring sizeof(PierLaneRef) would cut off
                // every older consumer on each appended field, which is what the rule
                // at the top of abi.h forbids: an additive change must not narrow the
                // loadable range.
                constexpr uint32_t kMinRefSize =
                    offsetof(PierLaneRef, alive) + sizeof(uint32_t const*);
                if (!out || out->struct_size < kMinRefSize) return PIER_LANE_REFUSED;

                // Clear it first. A half-filled out plus an ignored return code hands
                // the caller a wild pointer.
                out->lease = 0;
                out->fingerprint = 0;
                out->data = nullptr;
                out->vtable = nullptr;
                out->alive = nullptr;
                if (out->struct_size >= offsetof(PierLaneRef, busy) + sizeof(uint32_t*))
                {
                    out->busy = nullptr;
                }

                if (!modHandle) return PIER_LANE_REFUSED;
                auto* consumer = asMod(modHandle);
                if (!consumer) return PIER_LANE_REFUSED;

                std::string name = toString(nameRaw);
                if (name.empty() || name.size() > kMaxName) return PIER_LANE_REFUSED;

                PierLaneRefFn retain = nullptr;
                void* data = nullptr;
                uint64_t leaseId = 0;

                {
                    std::lock_guard lock(gMutex);
                    auto byName = gByName.find(name);
                    if (byName == gByName.end()) return PIER_LANE_NOT_FOUND;
                    auto it = gLanes.find(byName->second);
                    if (it == gLanes.end()) return PIER_LANE_NOT_FOUND;
                    Lane& lane = it->second;

                    // Acquiring from itself is pointless. Within one mod the function
                    // can be called directly, without two FFI hops and a lock, and a
                    // real cycle here produces one of the least readable stack shapes.
                    if (lane.mod == consumer) return PIER_LANE_REFUSED;
                    if (!providerAlive(lane)) return PIER_LANE_NOT_FOUND;

                    // The fingerprint comes before everything else. It is the premise
                    // the whole lane rests on: not one pointer is handed over before
                    // the layout is confirmed identical. fingerprint is filled in so
                    // the consumer can log something a server operator can act on,
                    // which the word "mismatch" on its own is not.
                    out->fingerprint = lane.desc.fingerprint;

                    // Fingerprint 0 does not mean "do not verify". Such an opening
                    // hands out raw vtable and data pointers rather than diagnostic
                    // data, and the consumer dereferences them as its own function
                    // table through its own offsets. Handing over pointers with an
                    // unverified layout is what the file header forbids, and this is
                    // the only path that could bypass it. Diagnostics go through
                    // lane_list, which gives out no pointer. The SDK turns a computed
                    // fingerprint of 0 into 1, so this refusal never reaches a caller.
                    if (wantFingerprint == 0 || wantFingerprint != lane.desc.fingerprint)
                    {
                        return PIER_LANE_FINGERPRINT;
                    }

                    leaseId = gNextLeaseId++;
                    gLeases.emplace(leaseId, Lease{consumer, byName->second});
                    ++lane.leases;

                    retain = lane.desc.retain;
                    data = lane.desc.data;

                    out->lease = leaseId;
                    out->data = lane.desc.data;
                    out->vtable = lane.desc.vtable;
                    out->alive =
                        lane.alive ? reinterpret_cast<uint32_t const*>(&lane.alive->flag) : nullptr;
                    // An appended field. The struct_size an older consumer fills in
                    // does not reach this far, so it simply is not written.
                    if (out->struct_size >= offsetof(PierLaneRef, busy) + sizeof(uint32_t*))
                    {
                        out->busy =
                            lane.alive ? reinterpret_cast<uint32_t*>(&lane.alive->busy) : nullptr;
                    }
                }

                // Outside the lock, for the reason retireLane gives: retain jumps
                // into the provider's dylib.
                if (retain) retain(data);
                return PIER_LANE_OK;
                // 0 is LANE_OK, so an exception must never report success. REFUSED
                // matches the other refusal paths of this entry point.
            PIER_API_GUARD_END_VAL(PIER_LANE_REFUSED)
        }

        bool api_lane_release(PierModHandle modHandle, uint64_t leaseId)
        {
            PIER_API_GUARD_BEGIN
                if (!modHandle || leaseId == 0) return false;
                auto* consumer = asMod(modHandle);

                PierLaneRefFn release = nullptr;
                void* data = nullptr;
                {
                    std::lock_guard lock(gMutex);
                    auto it = gLeases.find(leaseId);
                    // When the provider went away the host already called release for
                    // this lease and removed it. Returning false here rather than
                    // calling again avoids a double free.
                    if (it == gLeases.end()) return false;
                    if (it->second.holder != consumer) return false;

                    auto lane = gLanes.find(it->second.laneId);
                    if (lane != gLanes.end())
                    {
                        if (lane->second.leases > 0) --lane->second.leases;
                        release = lane->second.desc.release;
                        data = lane->second.desc.data;
                    }
                    gLeases.erase(it);
                }
                if (release) release(data);
                return true;
            PIER_API_GUARD_END
        }

        void api_lane_list(void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                std::string out = "[";
                {
                    std::lock_guard lock(gMutex);
                    bool first = true;
                    for (auto const& [id, lane] : gLanes)
                    {
                        if (!first) out += ',';
                        first = false;
                        char fp[32];
                        std::snprintf(fp, sizeof(fp), "0x%016llx",
                                      static_cast<unsigned long long>(lane.desc.fingerprint));
                        // Escaped, because the lane name comes from the provider and
                        // the mod name from a manifest. Both are external input and a
                        // single quote concatenated straight into the JSON would tear
                        // this output apart.
                        out += "{\"name\":\"";
                        out += jsonEscape(lane.name);
                        out += "\",\"mod\":\"";
                        out += jsonEscape(lane.mod ? std::string(lane.mod->getName())
                                                   : std::string("?"));
                        out += "\",\"fingerprint\":\"";
                        out += fp;
                        out += "\",\"protocol\":";
                        out += snbtNum(lane.desc.protocol);
                        out += ",\"leases\":";
                        out += snbtNum(lane.leases);
                        out += ",\"alive\":";
                        out += (lane.alive && lane.alive->flag.load(std::memory_order_acquire))
                                   ? "true"
                                   : "false";
                        out += '}';
                    }
                }
                out += ']';
                if (sink) sink(ctx, ps(out));
            PIER_API_GUARD_END_VOID
        }

        /**
         * Unload veto. Reports whether any lane this mod provides is currently inside
         * a call.
         *
         * The rule that every call happens on the server thread blocks a concurrent
         * unload but not a re-entrant one. An entry of the provider triggers a command
         * dispatch and that command unloads the provider, so FreeLibrary runs beneath
         * a frame still sitting in provider code. The liveness flag cannot help, since
         * the consumer read it long before.
         *
         * The refusal therefore happens here rather than crashing after the unload.
         * The lane name is returned for the caller's error message. It lives in
         * gLanes, is owned by the host and is independent of the provider's dylib.
         */
        char const* vetoWhy(HostedMod* mod)
        {
            static std::string held;
            std::lock_guard lock(gMutex);
            for (auto const& [id, lane] : gLanes)
            {
                if (lane.mod != mod || !lane.alive) continue;
                if (lane.alive->busy.load(std::memory_order_acquire) != 0)
                {
                    held = "fast lane '" + lane.name + "' is currently inside a call";
                    return held.c_str();
                }
            }
            return nullptr;
        }

        /** Teardown at stage 40. Two things, and the order matters. */
        void teardown(HostedMod* mod)
        {
            // 1. Return the leases this mod consumes. The provider is still alive so
            //    `release` runs normally. Without this the provider's state stays
            //    retained by a mod that no longer exists.
            std::vector<uint64_t> mine;
            {
                std::lock_guard lock(gMutex);
                for (auto const& [id, lease] : gLeases)
                {
                    if (lease.holder == mod) mine.push_back(id);
                }
            }
            for (uint64_t leaseId : mine)
            {
                PierLaneRefFn release = nullptr;
                void* data = nullptr;
                {
                    std::lock_guard lock(gMutex);
                    auto it = gLeases.find(leaseId);
                    if (it == gLeases.end()) continue;
                    auto lane = gLanes.find(it->second.laneId);
                    if (lane != gLanes.end())
                    {
                        if (lane->second.leases > 0) --lane->second.leases;
                        release = lane->second.desc.release;
                        data = lane->second.desc.data;
                    }
                    gLeases.erase(it);
                }
                if (release) release(data);
            }

            // 2. Retire the lanes it publishes. This must complete before
            //    FreeLibrary. ModHost calls lib.free() only after every teardown stage
            //    has run, which guarantees the order. Otherwise `release` would jump
            //    into an already unmapped code section.
            std::vector<uint64_t> published;
            {
                std::lock_guard lock(gMutex);
                for (auto const& [id, lane] : gLanes)
                {
                    if (lane.mod == mod) published.push_back(id);
                }
            }
            for (uint64_t id : published) retireLane(id);
        }

        void fill(PierApi& api)
        {
            api.lane_publish = &api_lane_publish;
            api.lane_unpublish = &api_lane_unpublish;
            api.lane_acquire = &api_lane_acquire;
            api.lane_release = &api_lane_release;
            api.lane_list = &api_lane_list;
        }

        spi::SlotPackReg regSlots{{"lane", &fill}};
        spi::UnloadVetoReg regVeto{{"lane", &vetoWhy}};
        spi::TeardownReg regDown{{40, "lane", &teardown}};
    } // namespace
} // namespace pier::lane
