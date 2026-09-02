#pragma once
// Owner of the PierApi table. There is exactly one table and it lives in the host.
// Capability packages fill it through the SPI and mods receive a const pointer.
// Nothing may modify it after load().
#include "sdk/abi.h"

namespace pier
{
    /** Writable reference. Only spi::buildApi may use it, from inside load().
     *  The table is frozen afterwards. */
    [[nodiscard]] PierApi& mutableApi() noexcept;

    /** The frozen table, passed to every pier_main. */
    [[nodiscard]] PierApi const* bridgeApi() noexcept;
} // namespace pier
