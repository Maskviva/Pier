#pragma once
// PierApi 表的所有者。表只有一张、住在宿主；能力包经 SPI 往里填，
// 模组拿到的是 const 指针 —— 谁都不许在 load() 之后再改它。
#include "sdk/abi.h"

namespace pier
{
    /** 可写引用。**只许** spi::buildApi 在 load() 里用；表随后冻结。 */
    [[nodiscard]] PierApi& mutableApi() noexcept;

    /** 冻结后的表，传给每个 pier_main。 */
    [[nodiscard]] PierApi const* bridgeApi() noexcept;
} // namespace pier
