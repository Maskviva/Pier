#pragma once
// Minimal helpers for assembling SNBT. Every composite payload that crosses a
// boundary, such as event data and player summaries, is SNBT text. There is exactly
// one set of helpers, because two escaping rules that disagree are an injection
// point.
#include <format>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>

namespace pier
{
    /** Escapes the content of an SNBT string literal. \ and " are escaped as
     *  themselves, \n \r \t become escape sequences.
     *  A player can paste anything into chat or a form. A raw control character
     *  inside a literal makes the parsing side either misread it or fail. This is
     *  the only line of defense, so call sites must not assemble quotes by hand. */
    [[nodiscard]] std::string snbtEscape(std::string_view s);

    /** Number to SNBT text. Integers go through std::to_string, which has the full
     *  set of signed and unsigned overloads. Floating point goes through the
     *  shortest round-trip form of std::format, because %g loses precision and
     *  %.17g prints 0.1 as 17 digits. It is a template and not a pair of overloads
     *  because call sites mix int, size_t, int64 and float, and an overload set is
     *  ambiguous for size_t between long long and double. */
    template <class T>
    [[nodiscard]] std::string snbtNum(T v)
    {
        if constexpr (std::is_floating_point_v<T>)
        {
            return std::format("{}", static_cast<double>(v));
        }
        else
        {
            static_assert(std::is_integral_v<T>, "snbtNum takes numbers only");
            return std::to_string(v);
        }
    }

    /** A complete quoted string literal, "...", with the content escaped. */
    /**
     * Always emits a `d` suffix on a floating point value. `snbtNum(100.0)` yields
     * `100`, which SNBT reads as an Int and a consumer reading a double then fails
     * on. Every site handing out a coordinate, an AABB or a ratio uses this instead.
     * SNBT cannot represent a non-finite value, so NaN and Inf land as 0d and the
     * caller decides whether to reject them beforehand.
     */
    template <class T>
    [[nodiscard]] std::string snbtDouble(T v)
    {
        static_assert(std::is_arithmetic_v<T>, "snbtDouble takes numbers only");
        double d = static_cast<double>(v);
        if (!(d == d) || d == std::numeric_limits<double>::infinity()
            || d == -std::numeric_limits<double>::infinity())
        {
            d = 0.0;
        }
        std::string out = std::format("{}", d);
        // Force it to look like a float, so `100` becomes `100.0d`. SNBT does not
        // accept scientific notation such as `1e+21`, so that case switches to fixed
        // point output.
        if (out.find_first_of("e.") == std::string::npos) out += ".0";
        else if (out.find('e') != std::string::npos) out = std::format("{:.1f}", d);
        return out + "d";
    }

    [[nodiscard]] std::string snbtStr(std::string_view s);
} // namespace pier
