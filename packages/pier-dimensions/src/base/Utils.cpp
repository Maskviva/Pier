#include "pier/dimensions/base/utils.h"

#include <snappy.h>

#include "mc/world/level/dimension/Dimension.h"

#include "pier/support/log.h"

namespace pier::dimensions::utils
{
    std::string compress(std::string_view sv)
    {
        std::string out;
        snappy::Compress(sv.data(), sv.size(), &out);
        return out;
    }

    std::string decompress(std::string_view sv)
    {
        std::string out;
        snappy::Uncompress(sv.data(), sv.size(), &out);
        return out;
    }
} // namespace pier::dimensions::utils

namespace pier::dimensions
{
    void verifyHeightRange(::Dimension& dim, int expectedMin, int expectedMax, char const* who)
    {
        auto& range = dim.mHeightRange.get();
        int const actualMin = range.mMin;
        int const actualMax = range.mMax;

        if (actualMin == expectedMin && actualMax == expectedMax)
        {
            return;
        }

        pier::hostLogger().error(
            "[dim] {} has mHeightRange {}..{} ({} subchunks) while the definition sent to "
            "the client says {}..{} ({} subchunks); the two disagree, subchunk requests are "
            "judged out of range and no block data reaches the client. Corrected in place "
            "to {}..{}",
            who, actualMin, actualMax, (actualMax - actualMin) / 16,
            expectedMin, expectedMax, (expectedMax - expectedMin) / 16,
            expectedMin, expectedMax
        );

        range.mMin = static_cast<short>(expectedMin);
        range.mMax = static_cast<short>(expectedMax);
    }
} // namespace pier::dimensions
