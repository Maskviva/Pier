/** pe_exports.h: reads the export names of a PE image that is already mapped into memory. */
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace pier::pe
{
    namespace detail
    {
        /** Copies `n` bytes at `at` out of `image`, if they lie inside its first `size` bytes. */
        inline bool readBytes(
            std::byte const* image, std::size_t size, std::size_t at, void* out, std::size_t n) noexcept
        {
            if (at > size || n > size - at) return false;
            std::memcpy(out, image + at, n);
            return true;
        }

        template <class T>
        bool readValue(std::byte const* image, std::size_t size, std::size_t at, T& out) noexcept
        {
            return readBytes(image, size, at, &out, sizeof(T));
        }
    } // namespace detail

    /** Most names read from one image. A real module exports far fewer. */
    inline constexpr std::uint32_t kMaxExportNames = 65536;

    /** Longest export name accepted. A longer one is taken for damage and skipped. */
    inline constexpr std::size_t kMaxNameLength = 512;

    /**
     * The names a module exports, in table order. Empty when the image is not a PE file or
     * has no export directory.
     *
     * `image` is the address the loader mapped the module at, so an RVA is an offset from it.
     * `size` is how many bytes may be read; 0 takes SizeOfImage from the headers, which are
     * always mapped. Every offset is checked against that bound and a bad one is skipped or
     * ends the walk, so a damaged image gives a shorter list and never a read outside it.
     */
    inline std::vector<std::string> exportNames(std::byte const* image, std::size_t size = 0)
    {
        using detail::readValue;

        std::vector<std::string> names;
        if (image == nullptr) return names;

        std::size_t limit = size != 0 ? size : 0x1000;
        std::uint16_t mz = 0;
        std::uint32_t peOffset = 0;
        if (!readValue(image, limit, 0, mz) || mz != 0x5A4D) return names;
        if (!readValue(image, limit, 0x3C, peOffset)) return names;

        std::uint32_t signature = 0;
        if (!readValue(image, limit, peOffset, signature) || signature != 0x00004550) return names;

        // The optional header follows the 4-byte signature and the 20-byte file header. Its
        // data directories sit after 96 bytes in a PE32 image and after 112 in a PE32+ one.
        std::size_t const optional = std::size_t{peOffset} + 24;
        std::uint16_t magic = 0;
        if (!readValue(image, limit, optional, magic)) return names;
        std::size_t countAt = 0;
        std::size_t dirsAt = 0;
        if (magic == 0x20B)
        {
            countAt = optional + 108;
            dirsAt = optional + 112;
        }
        else if (magic == 0x10B)
        {
            countAt = optional + 92;
            dirsAt = optional + 96;
        }
        else
        {
            return names;
        }

        if (size == 0)
        {
            std::uint32_t imageSize = 0;
            if (!readValue(image, limit, optional + 56, imageSize)) return names;
            limit = imageSize;
        }

        std::uint32_t directories = 0;
        std::uint32_t exportRva = 0;
        if (!readValue(image, limit, countAt, directories) || directories == 0) return names;
        if (!readValue(image, limit, dirsAt, exportRva) || exportRva == 0) return names;

        std::uint32_t count = 0;
        std::uint32_t namesRva = 0;
        if (!readValue(image, limit, std::size_t{exportRva} + 24, count)) return names;
        if (!readValue(image, limit, std::size_t{exportRva} + 32, namesRva)) return names;
        if (count > kMaxExportNames) count = kMaxExportNames;

        for (std::uint32_t i = 0; i < count; ++i)
        {
            std::uint32_t nameRva = 0;
            if (!readValue(image, limit, std::size_t{namesRva} + std::size_t{i} * 4, nameRva)) break;
            if (nameRva == 0 || nameRva >= limit) continue;

            std::string name;
            std::size_t at = nameRva;
            while (at < limit && name.size() < kMaxNameLength)
            {
                char const c = static_cast<char>(image[at]);
                if (c == '\0') break;
                name.push_back(c);
                ++at;
            }
            if (at >= limit || name.empty() || name.size() >= kMaxNameLength) continue;
            names.push_back(std::move(name));
        }
        return names;
    }

    /** Whether any name in `names` contains `part`. */
    inline bool anyContains(std::vector<std::string> const& names, std::string_view part)
    {
        for (auto const& n : names)
        {
            if (n.find(part) != std::string::npos) return true;
        }
        return false;
    }

    /** The names in `names` that contain `part`, at most `limit` of them. */
    inline std::vector<std::string> namesContaining(
        std::vector<std::string> const& names, std::string_view part, std::size_t limit)
    {
        std::vector<std::string> out;
        for (auto const& n : names)
        {
            if (out.size() >= limit) break;
            if (n.find(part) != std::string::npos) out.push_back(n);
        }
        return out;
    }
} // namespace pier::pe
