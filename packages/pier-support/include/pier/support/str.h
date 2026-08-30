#pragma once
// PierStr ↔ std::string_view 的零拷贝互转。
// 这是契约「不许有语言类型」的另一半：abi.h 里只有 {ptr,len}，
// C++ 的便利留在这里（契约 §一 规则五）。
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

    /** 要跨出当前调用帧就拷贝 —— 视图只在回调期间有效（契约 §三）。 */
    [[nodiscard]] inline std::string toString(PierStr s)
    {
        return std::string{s.ptr, s.len};
    }

    // 布局哨兵：abi.h 自己定义布局，不需要运行时验证；这两行只是把
    // 「有人往 PierStr 里加字段」在编译期变成一个明确的错误。
    static_assert(sizeof(PierStr) == sizeof(void*) + sizeof(size_t));
    static_assert(offsetof(PierStr, ptr) == 0);
} // namespace pier
