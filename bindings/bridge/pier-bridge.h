/** pier-bridge.h: calling a Pier mod's JSON services from a mod that is not one.
 *
 *  Header-only and dependency-free. Copy this file into your project, include it, call
 *  `levilamina::bridge::Client::open()` once, and ask.
 *
 *      auto client = levilamina::bridge::Client::open();
 *      if (!client) { logger.warn("Pier is not installed"); return; }
 *
 *      auto reply = client->call("rsw:perm:check",
 *          R"({"subject":{"kind":"player","id":"2535...."},"node":"rcc.fly"})");
 *      if (reply.ok()) parseYourJson(reply.body);
 *
 *  # What this is and is not
 *
 *  It is a door into Pier's cross-mod service registry: request in, reply out, both UTF-8
 *  strings whose shape each service defines. Every Pier mod that registers a service is
 *  reachable, with no change on its side.
 *
 *  It is **not** the Pier mod ABI. It gives no events, no hooks, no forms, no lane. A mod
 *  that wants those is a Pier mod and includes `sdk/abi.h` instead. This file exists for
 *  the mod that already has its own loader and wants to ask one question.
 *
 *  # The caller is anonymous
 *
 *  A Pier mod's calls carry a handle the loader issued, so a provider can be told who is
 *  asking. A bridge caller has none and never will: it is a different dll on a different
 *  loader. A provider that judges by caller cannot judge you, so anything you need it to
 *  know goes in the request. `rsw-perms` already works this way -- its operator endpoints
 *  take `by` in the body and do not infer authority from the connection.
 *
 *  Do not read that as "the bridge is a back door". It reaches the same registry with the
 *  same locking and the same revalidation; what it lacks is a name to put in a log line.
 *
 *  # Platform
 *
 *  Windows only, because LeviLamina is. The header does not compile elsewhere rather than
 *  compiling into something that always answers "Pier is not installed".
 *
 *  Server only: a client build of Pier exports none of these symbols, so `open()` returns
 *  null there and the caller takes its "not installed" path.
 *
 *  # Threading
 *
 *  The call is synchronous on your thread and has no timeout, exactly like a Pier mod's.
 *  A provider that blocks blocks you. Most Pier services expect the server thread; calling
 *  one from a worker is between you and that service's own contract.
 *
 *  # Long replies
 *
 *  A Pier that exports `pier_bridge_call_sink` hands the reply over whole, and the
 *  provider runs exactly once. An older Pier only has the buffer form: a reply longer than
 *  2047 bytes costs a second call, and the provider runs twice. `Client::runsOnce()` says
 *  which one this host offers, for a service whose calls are not safe to repeat.
 */
#ifndef PIER_BRIDGE_H
#define PIER_BRIDGE_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace levilamina::bridge
{
    /** What the host answered. The numbers are Pier's own `PIER_SERVICE_*`. */
    enum class Status : int32_t
    {
        Ok = 0,
        /** Nobody provides this name, or the provider is unloaded. Not an error on your
         *  side: a service you rely on being absent is a normal deployment. */
        NotFound = 1,
        /** The provider ran and failed. `body` holds its message. */
        Error = 2,
        /** The name was rejected, or the call nested too deep. */
        Refused = 3,
        /** Pier is not loaded, or its bridge ABI is not one this header speaks. Distinct
         *  from NotFound on purpose: one means "install the mod you wanted" and the other
         *  means "install Pier". */
        Unavailable = -1,
    };

    struct Reply
    {
        Status status = Status::Unavailable;
        std::string body;

        bool ok() const { return status == Status::Ok; }
    };

    /** The bridge ABI this header speaks. `open()` refuses a host that reports another.
     *
     *  Refusing rather than trying: a signature that changed under a caller is the one
     *  failure that crashes instead of returning, and the check costs one call at load. */
    inline constexpr uint32_t kAbi = 1u;

    class Client
    {
    public:
        /** Finds an already-loaded Pier, takes a reference to it, and binds the symbols.
         *
         *  Returns null when Pier is absent or speaks another ABI.
         *
         *  It never brings in a second copy: Pier is a LeviLamina mod, and a second copy
         *  would be a second, empty registry rather than the running one. What it does
         *  take is a **reference**, held for the life of this object. Without one the
         *  function pointers below outlive the mapping the moment Pier is unloaded, and
         *  the next call jumps into unmapped memory. Holding the mapping does not keep
         *  Pier's mod alive: it unregisters its services on disable, and calls after that
         *  answer NotFound, which is the degradation a caller can handle. */
        static std::unique_ptr<Client> open()
        {
            auto handle = acquire();
            if (!handle) return nullptr;

            auto abi = reinterpret_cast<uint32_t (*)()>(symbol(handle, "pier_bridge_abi"));
            auto call = reinterpret_cast<CallFn>(symbol(handle, "pier_bridge_call"));
            auto list = reinterpret_cast<ListFn>(symbol(handle, "pier_bridge_list"));
            if (!abi || !call || !list || abi() != kAbi)
            {
                release(handle);
                return nullptr;
            }
            auto client = std::unique_ptr<Client>(new Client(handle, call, list));
            // Optional: added to Pier without raising the bridge ABI, so absent on older hosts.
            client->mCallSink = reinterpret_cast<CallSinkFn>(symbol(handle, "pier_bridge_call_sink"));
            client->mListSink = reinterpret_cast<ListSinkFn>(symbol(handle, "pier_bridge_list_sink"));
            return client;
        }

        ~Client() { release(mHandle); }

        Client(const Client&) = delete;
        Client& operator=(const Client&) = delete;

        /** Asks one service. `request` is whatever that service documents, usually JSON. */
        Reply call(std::string_view name, std::string_view request) const
        {
            if (mCallSink)
            {
                Collect got;
                auto code = mCallSink(name.data(), name.size(), request.data(), request.size(), &got,
                                      &Collect::take);
                return got.finish(code);
            }
            // The buffer form takes NUL-terminated text.
            std::string const n(name);
            std::string const r(request);
            return run([&](char* buf, uint32_t cap, uint32_t* len)
                       { return mCall(n.c_str(), r.c_str(), buf, cap, len); });
        }

        /** Whether call() runs the provider exactly once for any reply length. False only
         *  on a Pier older than the sink form, see "Long replies" above. */
        bool runsOnce() const { return mCallSink != nullptr; }

        /** Every registered service, as `[{"name":…,"mod":…}]`.
         *
         *  For deciding at load whether to build a request nobody can answer, which reads
         *  better in a log than a NotFound at the moment somebody needed the answer. */
        Reply services() const
        {
            if (mListSink)
            {
                Collect got;
                return got.finish(mListSink(&got, &Collect::take));
            }
            return run([&](char* buf, uint32_t cap, uint32_t* len) { return mList(buf, cap, len); });
        }

    private:
        using CallFn = int32_t (*)(const char*, const char*, char*, uint32_t, uint32_t*);
        using ListFn = int32_t (*)(char*, uint32_t, uint32_t*);
        using SinkFn = void (*)(void*, const char*, std::size_t);
        using CallSinkFn = int32_t (*)(const char*, std::size_t, const char*, std::size_t, void*, SinkFn);
        using ListSinkFn = int32_t (*)(void*, SinkFn);

        /** Receives the reply inside Pier's call. Nothing may throw back through Pier and the
         *  provider's frames, so an allocation failure is recorded and reported afterwards. */
        struct Collect
        {
            std::string body;
            bool lost = false;

            static void take(void* ctx, const char* data, std::size_t len) noexcept
            {
                auto* self = static_cast<Collect*>(ctx);
                try
                {
                    self->body.assign(data, len); // The last write wins, as in the buffer form
                }
                catch (...)
                {
                    self->lost = true;
                }
            }

            Reply finish(int32_t code)
            {
                Reply out;
                out.status = static_cast<Status>(code);
                if (lost)
                {
                    // The service ran; only its answer could not be kept here.
                    out.status = Status::Error;
                    out.body = "the reply was too large to store";
                    return out;
                }
                out.body = std::move(body);
                return out;
            }
        };

        Client(void* handle, CallFn call, ListFn list)
            : mHandle(handle), mCall(call), mList(list)
        {
        }

        /** One attempt with a stack buffer, a second sized exactly when it did not fit.
         *
         *  Two calls at most. The host writes the length it needed whether or not the
         *  reply fit, so growing by guessing is never necessary; a provider answering
         *  something enormous costs one extra call and not a loop. */
        template <class F>
        static Reply run(F&& invoke)
        {
            char small[2048];
            uint32_t len = 0;
            int32_t code = invoke(small, sizeof(small), &len);
            Reply out;
            out.status = static_cast<Status>(code);
            if (len < sizeof(small))
            {
                out.body.assign(small, len);
                return out;
            }
            // The provider runs a second time here; see "Long replies" in the file header.
            std::string big(len + 1, '\0');
            uint32_t again = 0;
            code = invoke(big.data(), static_cast<uint32_t>(big.size()), &again);
            out.status = static_cast<Status>(code);
            out.body.assign(big.data(), again < big.size() ? again : big.size() - 1);
            return out;
        }

        /** `GetModuleHandleEx` without UNCHANGED_REFCOUNT, so this holds a reference.
         *  Plain `GetModuleHandleA` returns a handle that does not: the module can be
         *  unloaded under it and every bound pointer becomes a jump into nothing. */
        static void* acquire()
        {
            HMODULE h = nullptr;
            if (!GetModuleHandleExA(0, "Pier.dll", &h)) return nullptr;
            return reinterpret_cast<void*>(h);
        }
        static void release(void* h)
        {
            if (h) FreeLibrary(static_cast<HMODULE>(h));
        }
        static void* symbol(void* h, const char* n)
        {
            return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(h), n));
        }

        void* mHandle;
        CallFn mCall;
        ListFn mList;
        CallSinkFn mCallSink = nullptr;
        ListSinkFn mListSink = nullptr;
    };
} // namespace levilamina::bridge

// The namespace of releases before 26.51.2, kept as an alias so that code written against it
// still builds (contract section 2.5). The compiler warns wherever it is used.
namespace [[deprecated("pier::bridge is levilamina::bridge since 26.51.2")]] pier
{
    namespace bridge = ::levilamina::bridge;
}

#endif // PIER_BRIDGE_H
