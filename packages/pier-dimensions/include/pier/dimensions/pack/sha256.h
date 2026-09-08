/** sha256.h: SHA-256 over a byte range, for pack section and file hashes. */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace pier::dimensions::pack
{
    using Sha256 = std::array<std::uint8_t, 32>;

    Sha256 sha256(std::uint8_t const* data, std::size_t len);

    /** Lower-case hex, 64 characters. */
    std::string hex(Sha256 const& h);

    /** The reverse of hex; false when the text is not 64 hex digits. */
    bool parseHex(std::string const& text, Sha256& out);
} // namespace pier::dimensions::pack
