#pragma once
// The host's own Logger, not that of any hosted mod.
// It is a package-level entry point because guard.h and every capability package
// need to log, and none of them may include pier-host (contract §1). Logging must
// therefore live in a layer below the host.
#include "ll/api/io/Logger.h"

namespace pier
{
    ll::io::Logger& hostLogger();
} // namespace pier
