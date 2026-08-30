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
            "{} 的 mHeightRange 是 {}..{}（{} 个子区块），但发给客户端的定义是 {}..{}"
            "（{} 个子区块）—— 两边对不上，子区块请求会被判越界，方块数据一块都到不了客户端。"
            "现已就地纠正为 {}..{}。",
            who, actualMin, actualMax, (actualMax - actualMin) / 16,
            expectedMin, expectedMax, (expectedMax - expectedMin) / 16,
            expectedMin, expectedMax
        );

        range.mMin = static_cast<short>(expectedMin);
        range.mMax = static_cast<short>(expectedMax);
    }
} // namespace pier::dimensions
