#include "pier/host/api_table.h"

namespace pier
{
    namespace
    {
        // Value initialization, so every function pointer starts as NULL. This line
        // carries the first half of the contract rule that an absent capability means
        // a NULL slot (§2.1). The second half is that an absent package never
        // registers a slot pack to overwrite it.
        PierApi gApi{};
    } // namespace

    PierApi& mutableApi() noexcept { return gApi; }
    PierApi const* bridgeApi() noexcept { return &gApi; }
} // namespace pier
