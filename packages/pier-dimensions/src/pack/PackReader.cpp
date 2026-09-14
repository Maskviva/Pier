/** PackReader.cpp: reads a pack file and validates the container before any section is
 * handed out. */
#include "pier/dimensions/pack/pack_reader.h"

#include <algorithm>
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
        }

        /* Duplicate tags and overlapping spans, both by sorting rather than by comparing
         * every entry with every other. The section count is bounded only by the file
         * length and a zero-length section is legal, so a 100MB pack can declare about
         * 1.8 million of them; the pair-by-pair form is 3e12 comparisons and turns
         * opening one file into a hung startup.
         *
         * The overlap half is new here. rsw-pack has refused overlapping sections since
         * it was written and this side did not, which is the reader difference the
         * header of TemplatePack.cpp says must never exist: every section is decoded
         * independently, so an overlap means one run of bytes with two meanings, and
         * each of the two passing its own hash says nothing about that. Zero-length
         * sections sit out, they cover no byte and overlap nothing. */
        {
            std::vector<std::uint32_t> tags;
            tags.reserve(f.mTable.size());
            for (auto const& e : f.mTable) tags.push_back(e.type);
            std::sort(tags.begin(), tags.end());
            auto const dup = std::adjacent_find(tags.begin(), tags.end());
            if (dup != tags.end())
            {
                problems.push_back("section " + tagName(*dup) + " appears twice");
                return std::nullopt;
            }

            std::vector<SectionEntry const*> spans;
            spans.reserve(f.mTable.size());
            for (auto const& e : f.mTable)
                if (e.length) spans.push_back(&e);
            std::sort(spans.begin(), spans.end(), [](SectionEntry const* x, SectionEntry const* y) {
                return x->offset < y->offset;
            });
            for (std::size_t i = 1; i < spans.size(); ++i)
            {
                if (spans[i]->offset < spans[i - 1]->offset + spans[i - 1]->length)
                {
                    problems.push_back("section " + tagName(spans[i]->type) + " overlaps section "
                                       + tagName(spans[i - 1]->type));
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
