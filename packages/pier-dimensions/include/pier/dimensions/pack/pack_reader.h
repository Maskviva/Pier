/** pack_reader.h: the container of a terrain pack, validated and held in memory.
 * A PackFile owns the bytes of one file. Loading checks the magic, the format version,
 * the section table bounds and alignment, and the sha256 of every section, so any
 * section handed out afterwards is known to be intact. Refusals are returned as
 * messages in `problems` and never as a partially usable object. Nothing here depends
 * on the engine, which is what lets the pack layer compile and run under a plain
 * compiler for the equivalence tests. */
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "pier/dimensions/pack/pack_format.h"
#include "pier/dimensions/pack/sha256.h"

namespace pier::dimensions::pack
{
    enum class PackKind
    {
        Template,
        Volume,
    };

    /** A view of one section: bytes and a bounds-checked cursor for reading arrays. */
    struct Section
    {
        std::uint8_t const* data = nullptr;
        std::size_t size = 0;

        [[nodiscard]] bool has(std::size_t off, std::size_t len) const { return off <= size && len <= size - off; }

        template <class T>
        [[nodiscard]] bool read(std::size_t off, T& out) const
        {
            if (!has(off, sizeof(T))) return false;
            std::memcpy(&out, data + off, sizeof(T));
            return true;
        }

        /** Copies count structs starting at off into out; false when they do not fit. */
        template <class T>
        [[nodiscard]] bool readArray(std::size_t off, std::size_t count, std::vector<T>& out) const
        {
            if (count > size / sizeof(T) || !has(off, count * sizeof(T))) return false;
            out.resize(count);
            if (count) std::memcpy(out.data(), data + off, count * sizeof(T));
            return true;
        }
    };

    class PackFile
    {
        std::vector<std::uint8_t> mBytes;
        Sha256 mFileHash{};
        PackKind mKind = PackKind::Template;
        std::vector<SectionEntry> mTable;
        std::vector<std::string> mStrings;

    public:
        /** Reads and validates a whole file; nullopt with the reasons in problems. */
        static std::optional<PackFile> load(std::string const& path, std::vector<std::string>& problems);

        /** The same over bytes already in memory, for tests and for the inspect slot. */
        static std::optional<PackFile> parse(std::vector<std::uint8_t> bytes, std::vector<std::string>& problems);

        [[nodiscard]] PackKind kind() const { return mKind; }
        [[nodiscard]] Sha256 const& fileHash() const { return mFileHash; }
        [[nodiscard]] std::size_t byteCount() const { return mBytes.size(); }
        [[nodiscard]] bool has(std::uint32_t type) const;
        [[nodiscard]] std::optional<Section> section(std::uint32_t type) const;
        [[nodiscard]] std::vector<SectionEntry> const& table() const { return mTable; }

        [[nodiscard]] std::size_t stringCount() const { return mStrings.size(); }
        /** The string at a STRS index; empty when the index is out of range, which a
         *  decoder checks with validString first. */
        [[nodiscard]] std::string const& str(std::uint32_t idx) const;
        [[nodiscard]] bool validString(std::uint32_t idx) const { return idx < mStrings.size(); }
    };

    /** The section tag as four characters, for messages. */
    std::string tagName(std::uint32_t type);
} // namespace pier::dimensions::pack
