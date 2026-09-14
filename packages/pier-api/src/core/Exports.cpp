/** core/Exports.cpp: the symbols a mod outside Pier calls in through.
 *
 * Pier exports `pier_main` and nothing else, and it points the wrong way: the host calls a
 * mod through it. The service registry is reached by a PierModHandle the loader issues, so
 * a native LeviLamina mod cannot ask a Pier mod anything.
 * Beside the registry rather than in a package of its own: a package would need an edge
 * from a capability package to pier-api, and capability packages are siblings with no edge
 * between them (contract §1).
 * `pier_bridge_call` is `service_call` with no caller. `api_service_call` already takes a
 * null handle and its self-call check is already written as `if (caller && ...)`, so an
 * anonymous caller was a supported shape before this file. Nothing here loosens it.
 * What it costs is identity: a bridge caller has no handle and never will, so a provider
 * that judges by caller cannot judge it. Anything a provider must know goes in the
 * request, which is how rsw-perms' operator endpoints already work.
 */
#ifndef PIER_BUILD_CLIENT

#include <cstdint>
#include <cstring>
#include <string>

#include "pier/api/bridge.h"

namespace
{
/** Copies the provider's reply into the caller's buffer.
     *
     *  `needed` is written whether or not the reply fit, so one call with a short buffer
     *  tells the caller exactly how long to make the next one. Returning only "too small"
     *  would make every caller grow a buffer by guessing. */
struct Sink
{
        char* buf;
        uint32_t cap;
        uint32_t needed;
};

void writeBack(void* ctx, PierStr s)
{
        auto* out = static_cast<Sink*>(ctx);
        out->needed = static_cast<uint32_t>(s.len);
        if (!out->buf || out->cap == 0) return;
        uint32_t n = s.len < out->cap ? static_cast<uint32_t>(s.len) : out->cap - 1;
        if (s.ptr && n > 0) std::memcpy(out->buf, s.ptr, n);
        out->buf[n] = '\0';
}
} // namespace

/** The bridge ABI. Raised when the meaning or the signature of an exported symbol changes,
 *  never when a service is added: services are data over this, not part of it.
 *
 *  A consumer calls `pier_bridge_abi()` first and refuses to go on when it disagrees.
 *  Calling into a signature that changed under it is the one failure that crashes rather
 *  than returning, and the check costs one call at load. */
#define PIER_BRIDGE_ABI 1u

#define PIER_BRIDGE_EXPORT extern "C" __declspec(dllexport)

PIER_BRIDGE_EXPORT uint32_t pier_bridge_abi(void) { return PIER_BRIDGE_ABI; }

PIER_BRIDGE_EXPORT int32_t pier_bridge_call(
        const char* name,
        const char* request,
        char* reply,
        uint32_t reply_cap,
        uint32_t* reply_len)
{
        if (reply_len) *reply_len = 0;
        if (!name) return PIER_SERVICE_REFUSED;

        Sink sink{reply, reply_cap, 0};
        PierStr n{name, std::strlen(name)};
        PierStr r{request ? request : "", request ? std::strlen(request) : 0};

        // A null mod handle. The registry treats it as an anonymous caller, which is what
        // this is: the caller is not a Pier mod and has no handle to present.
        int32_t code = pier::bridge::callService(nullptr, n, r, &sink, &writeBack);
        if (reply_len) *reply_len = sink.needed;
        return code;
}

PIER_BRIDGE_EXPORT int32_t pier_bridge_list(char* reply, uint32_t reply_cap, uint32_t* reply_len)
{
        if (reply_len) *reply_len = 0;
        Sink sink{reply, reply_cap, 0};
        pier::bridge::listServices(&sink, &writeBack);
        if (reply_len) *reply_len = sink.needed;
return PIER_SERVICE_OK;
}

#endif // PIER_BUILD_CLIENT
