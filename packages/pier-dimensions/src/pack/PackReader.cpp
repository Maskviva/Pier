/** PackReader.cpp: reads a pack file and validates the container before any section is
 * handed out. */
#include "pier/dimensions/pack/pack_reader.h"

#include <filesystem>
#include <fstream>
#include <iterator>

namespace pier::dimensions::pack
{
    std::string tagName(std::uint32_t type)
    {
        std::string s(4, '?');
        for (int i = 0; i < 4; ++i)
        {
            char c = static_cast<char>((type >> (8 * i)) & 0xFF);
            s[i] = (c >= 0x20 && c < 0x7F) ? c : '?';
        }
        return s;
    }

    std::optional<PackFile> PackFile::load(std::string const& path, std::vector<std::string>& problems)
    {
        // The path is UTF-8; on Windows a narrow ifstream would read it as ANSI.
        std::ifstream in(std::filesystem::path(std::u8string(path.begin(), path.end())), std::ios::binary);
        if (!in)
        {
            problems.push_back("the pack file cannot be opened");
            return std::nullopt;
        }
        std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (!in.eof() && in.fail())
        {
            problems.push_back("the pack file cannot be read to the end");
            return std::nullopt;
        }
        return parse(std::move(bytes), problems);
    }

    std::optional<PackFile> PackFile::parse(std::vector<std::uint8_t> bytes, std::vector<std::string>& problems)
    {
        PackFile f;
        f.mBytes = std::move(bytes);
        auto const& b = f.mBytes;
        if (b.size() < kHeaderSize)
        {
            problems.push_back("the file is shorter than a pack header");
            return std::nullopt;
        }
        Header h{};
        std::memcpy(&h, b.data(), sizeof h);
        if (std::memcmp(h.magic, kMagicTemplate, kMagicSize) == 0) f.mKind = PackKind::Template;
        else if (std::memcmp(h.magic, kMagicVolume, kMagicSize) == 0) f.mKind = PackKind::Volume;
        else
        {
            problems.push_back("the file is not a Pier terrain pack");
            return std::nullopt;
        }
        if (h.formatVersion > kFormatVersion)
        {
            problems.push_back("the pack format version " + std::to_string(h.formatVersion)
                               + " is newer than this host reads (" + std::to_string(kFormatVersion) + ")");
            return std::nullopt;
        }
        if (h.headerSize != kHeaderSize || h.totalSize != b.size())
        {
            problems.push_back("the pack header does not describe this file: size fields disagree");
            return std::nullopt;
        }
        if (h.sectionCount > (b.size() - kHeaderSize) / kSectionEntrySize)
        {
            problems.push_back("the section table runs past the end of the file");
            return std::nullopt;
        }
        f.mTable.resize(h.sectionCount);
        if (h.sectionCount) std::memcpy(f.mTable.data(), b.data() + kHeaderSize, h.sectionCount * kSectionEntrySize);
        for (auto const& e : f.mTable)
        {
            if (e.offset % kSectionAlign || e.offset < kHeaderSize || e.offset > b.size() || e.length > b.size() - e.offset)
            {
                problems.push_back("section " + tagName(e.type) + " lies outside the file or is misaligned");
                return std::nullopt;
            }
            auto actual = sha256(b.data() + e.offset, static_cast<std::size_t>(e.length));
            if (std::memcmp(actual.data(), e.sha256, 32) != 0)
            {
                problems.push_back("section " + tagName(e.type) + " fails its hash");
                return std::nullopt;
            }
            for (auto const& o : f.mTable)
            {
                if (&o != &e && o.type == e.type)
                {
                    problems.push_back("section " + tagName(e.type) + " appears twice");
                    return std::nullopt;
                }
            }
        }
        f.mFileHash = sha256(b.data(), b.size());
        auto strs = f.section(kSecStrings);
        if (!strs)
        {
            problems.push_back("the pack has no string table");
            return std::nullopt;
        }
        std::uint32_t count = 0;
        std::vector<StringRef> refs;
        if (!strs->read(0, count) || !strs->readArray(4, count, refs))
        {
            problems.push_back("the string table is truncated");
            return std::nullopt;
        }
        std::size_t blob = 4 + std::size_t(count) * sizeof(StringRef);
        f.mStrings.reserve(count);
        for (auto const& r : refs)
        {
            if (!strs->has(blob + r.offset, r.length))
            {
                problems.push_back("a string lies outside the string table");
                return std::nullopt;
            }
            f.mStrings.emplace_back(reinterpret_cast<char const*>(strs->data + blob + r.offset), r.length);
        }
        if (f.mStrings.empty() || !f.mStrings[0].empty())
        {
            problems.push_back("string 0 of the pack is not the empty string");
            return std::nullopt;
        }
        return f;
    }

    bool PackFile::has(std::uint32_t type) const
    {
        for (auto const& e : mTable)
            if (e.type == type) return true;
        return false;
    }

    std::optional<Section> PackFile::section(std::uint32_t type) const
    {
        for (auto const& e : mTable)
        {
            if (e.type == type) return Section{mBytes.data() + e.offset, static_cast<std::size_t>(e.length)};
        }
        return std::nullopt;
    }

    std::string const& PackFile::str(std::uint32_t idx) const
    {
        static std::string const empty;
        return idx < mStrings.size() ? mStrings[idx] : empty;
    }
} // namespace pier::dimensions::pack
