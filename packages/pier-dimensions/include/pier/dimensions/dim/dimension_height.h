#pragma once

namespace pier::dimensions
{
    /**
     * 自定义维度的竖直范围，唯一的一份。两个消费者：Dimension 构造函数里的 DimensionArguments 决定服务端实际生成、存
     * 储、发送多少个子区块；DimensionDefinitionGroup::DimensionDefinition 会被
     * DimensionDataPacket 整个发给客户端，决定客户端为这个维度分配多高的区块缓冲、
     * 以及它会请求哪些子区块索引。两者对不上时客户端会收到落在自己缓冲之外的子区
     * 块，表现是进维度加载完直接闪退，所以必须读同一个常量。
     *
     * 要支持「每个维度不同高度」的话，正确做法是让这两个消费者从同一份 payload 取
     * 值（payload 在 addDimension 里已先于注册准备好），而不是一边读 NBT、一边用写
     * 死的常量。
     */

    /**
     * 底部是 -512 而不是 -64，因为客户端在自定义维度里不用服务端发过去的几何信息。
     * 三组定义（-64..320、0..384、0..256）下客户端一律请求子区块 -32..-24：它退回到
     * 「最大可能世界」，底部恒定取子区块 -32，也就是 y = -512。服务端按 -64 校验时
     * 每一个子区块请求都判 IndexOutOfBounds，方块数据一块都过不去。对照组是主世界，
     * 同一个客户端请求 -4..4 全部成功，因为原版维度它自己认识。
     *
     * 所以服务端搬过去对齐，顶部保持 320，维度总高 832 格即 52 个子区块。代价是每列
     * 多出 28 个纯空气子区块，调色板只有一项、序列化后很小，但内存里确实多占一些。
     * 引擎对维度高度是否有上限未确认，开服报错或维度建不出来时改回 -64 即可回到原状。
     * 已有存档的区块按 -64 的底部写成，换底部之后对不上，测试世界建议删掉重建。
     */
    inline constexpr int kWorldMinY = -512;
    inline constexpr int kWorldMaxY = 320;

    /**
     * 基岩层所在的 y。
     *
     * 维度底部下移到 -512 之后，如果还把基岩放在缓冲区索引 0（也就是
     * y=-512），上面到地表之间会被填土填掉 576 层 —— 又费内存又没意义。所以基
     * 岩仍然放在 y=-64，它下面那 448 格全是空气，玩家看到的世界和以前**完全一
     * 样**。
     */
    inline constexpr int kBedrockY = -64;

    static_assert(kWorldMinY % 16 == 0, "维度底部必须对齐到子区块边界");
    static_assert(kWorldMaxY % 16 == 0, "维度顶部必须对齐到子区块边界");
    static_assert(kWorldMinY < kWorldMaxY, "维度高度范围为空");
    static_assert(kWorldMinY < kBedrockY && kBedrockY < kWorldMaxY, "基岩层必须在维度范围内");
} // namespace pier::dimensions
