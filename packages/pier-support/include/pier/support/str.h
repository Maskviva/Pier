#pragma once
// Zero-copy conversion between PierStr and std::string_view.
// This is the other half of the contract rule that forbids language types. abi.h
// carries only {ptr,len} and the C++ convenience stays here (contract §1 rule 5).
#include <string>
#include <string_view>

#include "sdk/abi.h"

namespace pier
{
    [[nodiscard]] inline std::string_view sv(PierStr s) noexcept
    {
        return {s.ptr, s.len};
    }

    [[nodiscard]] inline PierStr ps(std::string_view s) noexcept
    {
        return PierStr{s.data(), s.size()};
    }

    /** Copy before leaving the current call frame. The view is valid only for the
     *  duration of the callback (contract §3). */
    [[nodiscard]] inline std::string toString(PierStr s)
    {
        return std::string{s.ptr, s.len};
    }

    // Layout sentinels. abi.h defines the layout itself and needs no runtime check.
    // These two lines only turn a field added to PierStr into an explicit compile
    // time error.
    static_assert(sizeof(PierStr) == sizeof(void*) + sizeof(size_t));
    static_assert(offsetof(PierStr, ptr) == 0);
} // namespace pier
