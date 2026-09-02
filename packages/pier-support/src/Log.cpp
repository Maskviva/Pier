#include "pier/support/log.h"

#include "ll/api/mod/NativeMod.h"

namespace pier
{
    ll::io::Logger& hostLogger()
    {
        // NativeMod::current() resolves by the image of the caller. Every package
        // is compiled into the same pier.dll, so this always yields the loader's own
        // mod.
        return ll::mod::NativeMod::current()->getLogger();
    }
} // namespace pier
