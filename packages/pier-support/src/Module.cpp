#include "pier/support/module.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace pier
{
    bool addressOwnedBy(void const* moduleBase, void const* fn) noexcept
    {
#ifdef _WIN32
        if (!moduleBase || !fn) return false;
        HMODULE owner = nullptr;
        if (!::GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(fn),
                &owner))
        {
            return false;
        }
        return static_cast<void const*>(owner) == moduleBase;
#else
        (void)moduleBase;
        (void)fn;
        return false;
#endif
    }
} // namespace pier
