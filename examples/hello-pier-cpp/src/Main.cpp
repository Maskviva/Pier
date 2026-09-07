/**
 * hello-pier-cpp — the smallest mod that does something.
 *
 * It registers one command, `/hello`, and logs a line at each lifecycle step. That is
 * enough to show every part of the contract a mod actually touches: the entry point, the
 * vtable, the capability check before a call, and a callback the host invokes.
 *
 * One header, no library, no build system required. The four build files beside this one
 * produce the same DLL and exist so you can use whichever your own project already uses.
 */
#include <cstring>
#include <string>

#include "sdk/abi.h"

namespace
{
    /** The table the host handed over, and this mod's own handle. Both are valid for the
     *  whole lifetime of the mod, so keeping them is safe; nothing else here is. */
    PierApi const* gApi = nullptr;
    PierModHandle gSelf = nullptr;

    PierStr str(char const* s) { return PierStr{s, std::strlen(s)}; }

    void log(int level, char const* msg)
    {
        // log is one of the four slots every host has, so no capability check is needed.
        // Every other call in this file checks first.
        if (gApi != nullptr && gApi->log != nullptr) gApi->log(gSelf, level, str(msg));
    }

    /** True when the host's table is long enough to hold the byte at `end`.
     *
     *  A host older than the SDK the mod compiled against has a shorter table, and its
     *  last slot is wherever it happened to stop. Reading past struct_size reads memory
     *  the host never wrote, so this is checked at the call site of every slot that is
     *  not one of the four in the header. */
    bool has(std::size_t end) { return gApi != nullptr && gApi->struct_size >= end; }

#define HAS_SLOT(member) (has(offsetof(PierApi, member) + sizeof(void*)) && gApi->member != nullptr)

    /** What `/hello` does. The host calls this on the server thread.
     *
     *  There are two sinks, one for output and one for errors, and both are sinks rather
     *  than buffers: a string handed to them is copied by the host during this call and
     *  nothing may be kept past the return. `args` is the raw text after the command
     *  name, and `originName` is who ran it. */
    void onHello(void*, PierStr args, PierStr originName, void* ctx,
                 PierStrSink outSuccess, PierStrSink outError)
    {
        if (outSuccess == nullptr) return;
        (void)outError;

        std::string reply = "hello from a C++ mod";
        if (originName.ptr != nullptr && originName.len > 0)
        {
            reply += ", ";
            reply.append(originName.ptr, originName.len);
        }
        if (args.ptr != nullptr && args.len > 0)
        {
            reply += " (you said: ";
            reply.append(args.ptr, args.len);
            reply += ")";
        }
        outSuccess(ctx, PierStr{reply.data(), reply.size()});
    }

    bool onEnable(void*)
    {
        log(3, "hello-pier-cpp enabled");

        if (!HAS_SLOT(register_command))
        {
            // Refusing to enable would be wrong here: the mod still logs, and a host
            // without commands is a host this mod can run on with less to offer. What is
            // not acceptable is registering nothing and saying nothing.
            log(2, "this host has no register_command, so /hello is not available");
            return true;
        }

        // permission 0 is Any, so every player can run it. The last argument is the
        // user pointer handed back to the callback; this mod keeps no state, so it is
        // null.
        if (!gApi->register_command(gSelf, str("hello"), str("Says hello."), 0, &onHello, nullptr))
        {
            log(1, "registering /hello failed; another mod may already own that name");
        }
        return true;
    }

    bool onDisable(void*)
    {
        // Nothing to undo by hand. Bedrock cannot unregister a command, so /hello stays
        // registered for the life of the server; the loader mutes the callback of a
        // disabled mod, which is what makes that safe.
        log(3, "hello-pier-cpp disabled");
        return true;
    }

    bool onUnload(void*)
    {
        log(3, "hello-pier-cpp unloaded");
        return true;
    }
} // namespace

/*
 * PIER_MAIN_EXPORT and not a bare extern "C". Naming the symbol is not the same as
 * exporting it: a Windows DLL exports nothing unless asked, and a mod that only declares
 * pier_main builds cleanly and is then refused at load. The macro comes from abi.h and
 * carries the C linkage too.
 */
PIER_MAIN_EXPORT bool pier_main(PierApi const* api, PierModHandle self, PierModVTable* out)
{
    if (api == nullptr || out == nullptr) return false;

    gApi = api;
    gSelf = self;

    // The mod fills its own four header scalars. struct_size is how the host knows how
    // much of this table it may read, which is the mirror of what `has` does above.
    out->struct_size = sizeof(PierModVTable);
    out->abi_version = PIER_ABI_VERSION;
    out->mod_flags = 0;
    out->_reserved0 = 0;

    out->instance = nullptr;
    out->on_enable = &onEnable;
    out->on_disable = &onDisable;
    out->on_unload = &onUnload;

    log(3, "hello-pier-cpp loaded");
    return true;
}
