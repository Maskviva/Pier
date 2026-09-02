/** core/Services.cpp: the cross-mod service registry, a request and answer call.
 * It is the opposite of the one-way broadcast in Bus.cpp on every axis: exactly one
 * provider per name, nobody registered is an error the caller must handle, and the
 * return value is the entire point. Two providers answering plot:can is an ambiguous
 * answer the caller cannot choose between, so registration is exclusive and a second
 * registrant is refused loudly. Letting the later one win silently would make the
 * answer depend on mod load order.
 * ModHost::unload calls FreeLibrary, so holding another mod's function pointer means
 * waiting for the next call to crash, in the caller, with nothing in the log pointing
 * at the mod that left. The host therefore owns the table, entries are numbered by
 * ticket, and the call path holds a weak_ptr<HostedMod> and rechecks immediately
 * before the call. request and reply are opaque UTF-8 agreed out of band.
 * service_call is synchronous, runs the provider in place and has no timeout, since a
 * timeout would hand back a wrong answer while the provider keeps running. Cycles end
 * on the depth limit and a self-call is refused.
 */
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

#include "sdk/abi.h"

#include "pier/host/hosted_mod.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/log.h"
#include "pier/support/snbt.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        /** Longest service name accepted, for the reason the bus topic limit gives.
         *  Long enough for `some-long-mod:some-query`, short enough that a garbage
         *  pointer cannot become an enormous map key. */
        constexpr size_t kMaxName = 128;

        /** Nesting limit for calls. A to B to A is depth 2. Anything beyond this
         *  number is a cycle and not a call chain. */
        constexpr int kMaxDepth = 8;

        struct Service
        {
            HostedMod* mod = nullptr; // Identity comparison only, never dereferenced
            // Liveness is rechecked through this weak_ptr and not through
            // mod->shared_from_this(), which would itself be a blind dereference.
            // service_call may run on any thread, and during an unload that dereference
            // is a use-after-free.
            std::weak_ptr<HostedMod> owner;
            std::string name;
            PierServiceCb cb = nullptr;
            void* user = nullptr;
        };

        std::mutex gMutex;
        /** Registration id to service. */
        std::unordered_map<uint64_t, Service> gServices;
        /** Name to registration id. Exactly one by construction. */
        std::unordered_map<std::string, uint64_t> gByName;
        uint64_t gNextId = 1;

        /** Nesting depth per thread. thread_local rather than global, because two
         *  threads calling concurrently are not a cycle and a shared counter would
         *  read them as one. */
        thread_local int gDepth = 0;

        struct DepthGuard
        {
            DepthGuard() { ++gDepth; }
            ~DepthGuard() { --gDepth; }
        };

        /** Warns about excessive depth once per service. A cycle spins as fast as the
         *  CPU, and logging on every hit turns a bug into an incident. */
        void warnDepthOnce(std::string const& name)
        {
            static std::mutex mu;
            static std::unordered_map<std::string, bool> seen;
            std::lock_guard lock(mu);
            if (seen[name]) return;
            seen[name] = true;
            hostLogger().error(
                "[service] '{}' exceeded call depth {}, innermost call refused; this is "
                "a call cycle, where the provider of this service calls it again, either "
                "directly or through another service that loops back",
                name, kMaxDepth
            );
        }

        uint64_t api_service_register(
            PierModHandle modHandle, PierStr nameRaw, PierServiceCb cb, void* user)
        {
            PIER_API_GUARD_BEGIN
                auto* mod = asMod(modHandle);
                if (!mod || !cb) return 0;

                std::string name = toString(nameRaw);
                if (name.empty() || name.size() > kMaxName) return 0;

                std::lock_guard lock(gMutex);
                if (auto it = gByName.find(name); it != gByName.end())
                {
                    // Refuse and name the incumbent. Reporting only that registration
                    // failed sends whoever reads the log looking for a duplicate
                    // registration in their own code that does not exist.
                    auto const& held = gServices[it->second];
                    char const* holder = held.mod ? held.mod->getName().c_str() : "?";
                    mod->getLogger().error(
                        "[service] service_register('{}') refused, already provided by "
                        "'{}'; a service name is exclusive, and two providers would make "
                        "the answer depend on mod load order",
                        name, holder
                    );
                    return 0;
                }

                uint64_t const id = gNextId++;
                std::weak_ptr<HostedMod> owner;
                try
                {
                    owner = mod->shared_from_this(); // Registration runs on the main thread
                }
                catch (std::bad_weak_ptr const&)
                {
                    return 0;
                }
                gServices.emplace(id, Service{mod, owner, name, cb, user});
                gByName.emplace(name, id);
                return id;
            PIER_API_GUARD_END
        }

        bool api_service_unregister(PierModHandle modHandle, uint64_t regId)
        {
            PIER_API_GUARD_BEGIN
                auto* mod = asMod(modHandle);
                if (!mod || regId == 0) return false;

                std::lock_guard lock(gMutex);
                auto it = gServices.find(regId);
                if (it == gServices.end()) return false;
                // Scoped to the caller. A mod may not unregister another mod's
                // service, the same rule bus_unsubscribe and schedule_cancel follow.
                if (it->second.mod != mod) return false;
                gByName.erase(it->second.name);
                gServices.erase(it);
                return true;
            PIER_API_GUARD_END
        }

        int32_t api_service_call(
            PierModHandle modHandle, PierStr nameRaw, PierStr requestRaw, void* ctx, PierStrSink reply)
        {
            PIER_API_GUARD_BEGIN
                std::string name = toString(nameRaw);
                if (name.empty() || name.size() > kMaxName) return PIER_SERVICE_REFUSED;

                if (gDepth >= kMaxDepth)
                {
                    warnDepthOnce(name);
                    return PIER_SERVICE_REFUSED;
                }

                auto* caller = modHandle ? asMod(modHandle) : nullptr;

                // The entry is copied out under the lock and the lock is released
                // before crossing into the dylib. Providers re-enter, by calling
                // another service, publishing on the bus or registering a form, and
                // calling into another mod under the lock deadlocks the server thread
                // on the first re-entry.
                Service svc;
                {
                    std::lock_guard lock(gMutex);
                    auto byName = gByName.find(name);
                    if (byName == gByName.end()) return PIER_SERVICE_NOT_FOUND;
                    auto it = gServices.find(byName->second);
                    if (it == gServices.end()) return PIER_SERVICE_NOT_FOUND;
                    svc = it->second;
                }
                if (!svc.cb || !svc.mod) return PIER_SERVICE_NOT_FOUND;
                if (caller && svc.mod == caller) return PIER_SERVICE_REFUSED; // No self-call

                // Rechecked through the weak_ptr immediately before the call. The
                // provider may have been unloaded since the lookup, and the ticket
                // table is cleaned only on the unload path. What is locked is the
                // weak_ptr captured at registration, and no raw pointer is
                // dereferenced.
                auto provider = svc.owner.lock();
                if (!provider || provider.get() != svc.mod) return PIER_SERVICE_NOT_FOUND;

                // isEnabled() must not be consulted here. ModManager::enable() flips
                // the state to Enabled only after the onEnable callback returns, and
                // during the whole load phase no mod is enabled yet, so consulting it
                // would make a mod probing others through service::call from its
                // on_load always receive NOT_FOUND. Calling into unmapped code is
                // guarded by the weak_ptr recheck and the pointer equality above, which
                // do not depend on enabled. A provider that wants disabled to look like
                // absent unregisters its service in on_disable.

                DepthGuard depth;
                bool ok = false;
                try
                {
                    ok = svc.cb(svc.user, ps(name), requestRaw, ctx, reply);
                }
                catch (...)
                {
                    // A provider throwing across the FFI boundary is already undefined
                    // behavior on its own side. Catching it here at least keeps the
                    // caller alive and gives it a status code it can act on.
                    hostLogger().error("[service] '{}' threw an exception across the FFI boundary", name);
                    return PIER_SERVICE_ERROR;
                }
                return ok ? PIER_SERVICE_OK : PIER_SERVICE_ERROR;
                // 0 is SERVICE_OK and an exception must never report success. ERROR
                // says the call happened and failed.
            PIER_API_GUARD_END_VAL(PIER_SERVICE_ERROR)
        }

        void api_service_list(void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                std::string out = "[";
                {
                    std::lock_guard lock(gMutex);
                    bool first = true;
                    for (auto const& [name, id] : gByName)
                    {
                        auto it = gServices.find(id);
                        if (it == gServices.end()) continue;
                        if (!first) out += ',';
                        first = false;
                        char const* owner = it->second.mod ? it->second.mod->getName().c_str() : "?";
                        out += "{\"name\":\"";
                        out += snbtEscape(name);
                        out += "\",\"mod\":\"";
                        out += snbtEscape(owner);
                        out += "\"}";
                    }
                }
                out += ']';
                if (sink) sink(ctx, ps(out));
            PIER_API_GUARD_END_VOID
        }

        /** Teardown. Unregisters every service held under this mod. */
        void teardown(HostedMod* mod)
        {
            if (!mod) return;
            std::lock_guard lock(gMutex);
            for (auto it = gServices.begin(); it != gServices.end();)
            {
                if (it->second.mod == mod)
                {
                    gByName.erase(it->second.name);
                    it = gServices.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        void fill(PierApi& api)
        {
            api.service_register = &api_service_register;
            api.service_unregister = &api_service_unregister;
            api.service_call = &api_service_call;
            api.service_list = &api_service_list;
        }

        spi::SlotPackReg regSlots{{"services", &fill}};
        spi::TeardownReg regDown{{30, "services", &teardown}};
    } // namespace
} // namespace pier::api_impl
