/** pe_exports_test.cpp: checks pe_exports.h against PE images built by hand. */
#include "pier/api/pe_exports.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace
{
    int failures = 0;

#define CHECK(cond)                                                                      \
    do                                                                                   \
    {                                                                                    \
        if (!(cond))                                                                     \
        {                                                                                \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);         \
            ++failures;                                                                  \
        }                                                                                \
    } while (0)

    using Image = std::vector<std::byte>;

    void put16(Image& b, std::size_t at, std::uint16_t v) { std::memcpy(&b[at], &v, 2); }
    void put32(Image& b, std::size_t at, std::uint32_t v) { std::memcpy(&b[at], &v, 4); }

    struct Built
    {
        Image bytes;
        std::uint32_t nameTable{};
        std::uint32_t exportDir{};
        std::size_t dirsAt{};
        std::size_t optional{};
    };

    /** A mapped image: RVA equals offset, 0x2000 bytes, the export directory at 0x400. */
    Built build(bool plus, std::vector<std::string> const& names)
    {
        Built r;
        r.bytes.assign(0x2000, std::byte{0});
        Image& b = r.bytes;
        put16(b, 0, 0x5A4D);
        put32(b, 0x3C, 0x80);
        put32(b, 0x80, 0x00004550);
        r.optional = 0x80 + 24;
        put16(b, r.optional, plus ? 0x20B : 0x10B);
        put32(b, r.optional + 56, 0x2000);
        put32(b, r.optional + (plus ? 108 : 92), 16);
        r.dirsAt = r.optional + (plus ? 112 : 96);
        r.exportDir = 0x400;
        put32(b, r.dirsAt, r.exportDir);
        put32(b, r.dirsAt + 4, 0x200);

        auto const n = static_cast<std::uint32_t>(names.size());
        std::uint32_t const funcs = r.exportDir + 40;
        r.nameTable = funcs + 4 * n;
        std::uint32_t const ordinals = r.nameTable + 4 * n;
        std::uint32_t at = ordinals + 2 * n;
        put32(b, r.exportDir + 20, n);
        put32(b, r.exportDir + 24, n);
        put32(b, r.exportDir + 28, funcs);
        put32(b, r.exportDir + 32, r.nameTable);
        put32(b, r.exportDir + 36, ordinals);
        for (std::uint32_t i = 0; i < n; ++i)
        {
            put32(b, r.nameTable + 4 * i, at);
            std::memcpy(&b[at], names[i].c_str(), names[i].size() + 1);
            at += static_cast<std::uint32_t>(names[i].size()) + 1;
            put16(b, ordinals + 2 * i, static_cast<std::uint16_t>(i));
        }
        return r;
    }

    std::vector<std::string> const kNames = {
        "LLMoney_Get",
        "LLMoney_Reduce",
        "?LLMoney_Ranking@@YA?AV?$vector@U?$pair@V?$basic_string@DU?$char_traits@D@std@@V?$"
        "allocator@D@2@@std@@_J@std@@V?$allocator@U?$pair@V?$basic_string@DU?$char_traits@D@"
        "std@@V?$allocator@D@2@@std@@_J@std@@@2@@std@@G@Z",
        "unrelated_export",
    };

    void testBothImageFlavorsGiveTheNamesInOrder()
    {
        for (bool plus : {true, false})
        {
            auto img = build(plus, kNames);
            auto got = pier::pe::exportNames(img.bytes.data());
            CHECK(got == kNames);
            auto sized = pier::pe::exportNames(img.bytes.data(), img.bytes.size());
            CHECK(sized == kNames);
        }
    }

    void testWhatIsNotAnExportTableGivesNothing()
    {
        CHECK(pier::pe::exportNames(nullptr).empty());

        auto noDir = build(true, kNames);
        put32(noDir.bytes, noDir.dirsAt, 0);
        CHECK(pier::pe::exportNames(noDir.bytes.data()).empty());

        auto noDirs = build(true, kNames);
        put32(noDirs.bytes, noDirs.optional + 108, 0);
        CHECK(pier::pe::exportNames(noDirs.bytes.data()).empty());

        auto notMz = build(true, kNames);
        put16(notMz.bytes, 0, 0x1234);
        CHECK(pier::pe::exportNames(notMz.bytes.data()).empty());

        auto notPe = build(true, kNames);
        put32(notPe.bytes, 0x80, 0xDEADBEEF);
        CHECK(pier::pe::exportNames(notPe.bytes.data()).empty());

        auto oddMagic = build(true, kNames);
        put16(oddMagic.bytes, oddMagic.optional, 0x107);
        CHECK(pier::pe::exportNames(oddMagic.bytes.data()).empty());

        auto empty = build(true, {});
        CHECK(pier::pe::exportNames(empty.bytes.data()).empty());
    }

    void testDamageGivesAShorterListAndNeverReadsOutside()
    {
        // A name pointing past the image or at nothing is skipped; the others survive.
        auto img = build(true, kNames);
        put32(img.bytes, img.nameTable + 4, 0xFFFFFF00);
        put32(img.bytes, img.nameTable + 12, 0);
        auto got = pier::pe::exportNames(img.bytes.data());
        CHECK(got.size() == 2);
        CHECK(got.size() == 2 && got[0] == "LLMoney_Get");
        CHECK(got.size() == 2 && got[1] == kNames[2]);

        // A count of billions is capped, and the image ending ends the walk. What lies past the
        // real table is junk, so only the real names are checked, and they come first.
        auto huge = build(true, kNames);
        put32(huge.bytes, huge.exportDir + 24, 0xFFFFFFF0);
        auto all = pier::pe::exportNames(huge.bytes.data());
        CHECK(all.size() >= kNames.size() && all.size() <= pier::pe::kMaxExportNames);
        CHECK(all.size() >= kNames.size() && std::equal(kNames.begin(), kNames.end(), all.begin()));
        for (auto const& n : all) CHECK(!n.empty());

        // A name that never ends before the image does is dropped.
        auto open = build(true, {"LLMoney_Get"});
        for (std::size_t i = 0x1FF0; i < 0x2000; ++i) open.bytes[i] = std::byte{'A'};
        put32(open.bytes, open.nameTable, 0x1FF0);
        CHECK(pier::pe::exportNames(open.bytes.data()).empty());

        // A name over the length limit is dropped.
        auto longName = build(true, {std::string(600, 'x')});
        CHECK(pier::pe::exportNames(longName.bytes.data()).empty());

        // A readable size that stops inside the strings shortens the list and stays inside.
        auto cut = build(true, kNames);
        auto few = pier::pe::exportNames(cut.bytes.data(), 0x440);
        CHECK(few.size() < kNames.size());
        auto tiny = pier::pe::exportNames(cut.bytes.data(), 0x90);
        CHECK(tiny.empty());
    }

    void testTheNameHelpers()
    {
        CHECK(pier::pe::anyContains(kNames, "LLMoney_Ranking"));
        CHECK(!pier::pe::anyContains(kNames, "LLMoney_Set"));
        auto two = pier::pe::namesContaining(kNames, "LLMoney", 2);
        CHECK(two.size() == 2);
        CHECK(two.size() == 2 && two[0] == "LLMoney_Get" && two[1] == "LLMoney_Reduce");
        CHECK(pier::pe::namesContaining(kNames, "LLMoney", 10).size() == 3);
        CHECK(pier::pe::namesContaining(kNames, "nothing", 10).empty());
    }
} // namespace

int main()
{
    testBothImageFlavorsGiveTheNamesInOrder();
    testWhatIsNotAnExportTableGivesNothing();
    testDamageGivesAShorterListAndNeverReadsOutside();
    testTheNameHelpers();
    if (failures == 0) std::puts("pe_exports: all checks passed");
    return failures == 0 ? 0 : 1;
}
