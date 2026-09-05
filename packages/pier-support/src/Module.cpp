#include "pier/support/module.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace pier
{
    void const* moduleContaining(void const* fn) noexcept
    {
#ifdef _WIN32
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
#else
        (void)fn;
        return nullptr;
#endif
    }

    bool addressOwnedBy(void const* moduleBase, void const* fn) noexcept
    {
        if (!moduleBase || !fn) return false;
        void const* owner = moduleContaining(fn);
        return owner != nullptr && owner == moduleBase;
    }
} // namespace pier
