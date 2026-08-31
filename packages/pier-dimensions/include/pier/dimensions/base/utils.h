#pragma once

#include <string>
#include <string_view>

class Dimension;

namespace pier::dimensions::utils
{
    std::string compress(std::string_view sv);
    std::string decompress(std::string_view sv);
} // namespace pier::dimensions::utils

namespace pier::dimensions
{
    /**
     * 核对（并在必要时纠正）一个维度对象自己的竖直范围。
     *
     * 客户端通过 DimensionDataPacket 里的 DimensionDefinition 知道维度多高，而服务
     * 端判一个子区块请求越不越界走的是 Dimension::isSubChunkHeightWithinRange，读的
     * 是 Dimension::mHeightRange。这是两份独立的数据，对不上时客户端按它拿到的高度
     * 请求子区块、服务端按自己那份判定，全部回 IndexOutOfBounds，方块数据一块都过不
     * 去，症状是「区块全是空的，但单个方块更新能显示」。
     *
     * 传进来的 expectedMin/Max 必须就是注册时写进 DimensionDefinition 的那一份。
     */
    void verifyHeightRange(::Dimension& dim, int expectedMin, int expectedMax, char const* who);
} // namespace pier::dimensions
