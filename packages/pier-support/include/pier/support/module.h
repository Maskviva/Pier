#pragma once
// Ownership lookup from a function address to the module that contains it.
//
// It lives in support rather than in one domain because it is the general remedy
// for a legacy slot shape. Slots that predate the mod-scoped convention, of which
// the money event listener is a surviving example, take a bare function pointer
// with no mod handle, so the host cannot tell which mod owns a callback and cannot
// clear it on unload. Resolving the module from the address is the only way to
// recover that ownership, and any future slot of the same shape needs it too.
#include <cstddef>

namespace pier
{
    /** Whether `fn` lies inside the loaded module based at `moduleBase`.
     *  Always false on non-Windows platforms, since BDS ships only a Windows target.
     *  Returning false makes the caller treat ownership as unresolved instead of
     *  acting on a resolution that was never made. */
    [[nodiscard]] bool addressOwnedBy(void const* moduleBase, void const* fn) noexcept;

    /** Base address of the loaded module containing `fn`, or nullptr when none does or
     *  the platform cannot tell. One call answers what a loop of addressOwnedBy over every
     *  mod answered with one loader-lock acquisition per mod. */
    [[nodiscard]] void const* moduleContaining(void const* fn) noexcept;
} // namespace pier
