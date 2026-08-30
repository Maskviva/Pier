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
     * 为什么需要这个：客户端是通过 DimensionDataPacket 里的
     * DimensionDefinition 知道维度多高的，而服务端判一个子区块请求越不越界走
     * 的是 `Dimension::isSubChunkHeightWithinRange`，读的是
     * `Dimension::mHeightRange`。这是**两份独立的数据**。两边对不上时，客户
     * 端会按它拿到的高度去请求子区块，服务端按自己那份判定，全部回
     * `IndexOutOfBounds`，于是方块数据一块都过不去 —— 玩家看到的就是「区块全
     * 是空的，但单个方块更新能显示」。
     *
     * 实测就是这个症状：27/27 全是越界。
     *
     * 传进来的 expectedMin/Max 必须就是注册时写进 DimensionDefinition 的那一
     * 份。
     */
    void verifyHeightRange(::Dimension& dim, int expectedMin, int expectedMax, char const* who);
} // namespace pier::dimensions
