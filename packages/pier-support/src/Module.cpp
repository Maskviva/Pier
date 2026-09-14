/** Module.cpp: which loaded module an address belongs to.
 *
 * Windows only, because LeviLamina is. A portable stub here would have to answer "no
 * module" for every address, and the callers read that as "this function is not owned by
 * the mod being unloaded", which is the answer that lets a stale pointer through.
 */
#include "pier/support/module.h"

#ifndef _WIN32
#error "Pier targets Windows; LeviLamina has no other build"
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace pier
{
    void const* moduleContaining(void const* fn) noexcept
    {
        if (!fn) return nullptr;
        HMODULE owner = nullptr;
        if (!::GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(fn),
                &owner))
        {
            return nullptr;
        }
        return static_cast<void const*>(owner);
    }

    bool addressOwnedBy(void const* moduleBase, void const* fn) noexcept
    {
        if (!moduleBase || !fn) return false;
        void const* owner = moduleContaining(fn);
        return owner != nullptr && owner == moduleBase;
    }
} // namespace pier
