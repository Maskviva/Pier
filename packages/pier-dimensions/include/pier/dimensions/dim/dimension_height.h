#pragma once

namespace pier::dimensions
{
    /**
     * The vertical range of a custom dimension, in one place. Two consumers read it. The
     * DimensionArguments in the Dimension constructor decide how many subchunks the
     * server generates, stores and sends. The
     * DimensionDefinitionGroup::DimensionDefinition goes whole into DimensionDataPacket
     * and decides how tall a chunk buffer the client allocates and which subchunk indices
     * it requests. When the two disagree the client receives subchunks outside its own
     * buffer, which shows up as a crash right after the dimension finishes loading, so
     * both must read the same constant.
     *
     * Supporting a different height per dimension means having both consumers take the
     * value from the same payload, which addDimension prepares before registration,
     * rather than one reading NBT while the other uses a hardcoded constant.
     */

    /** The bottom is -512 and not -64, because a client does not use the geometry the
     * server sends for a custom dimension. Under -64..320, 0..384 and 0..256 alike the
     * client always requests subchunks -32..-24: it falls back to the largest possible
     * world and takes the bottom as subchunk -32, y = -512. With the server validating
     * against -64, every request is judged IndexOutOfBounds and no block data gets
     * through. The control is the overworld, where the same client requests -4..4 and all
     * succeed, because it knows the vanilla dimensions itself. The server therefore moves
     * to match, the top stays at 320, and the dimension is 832 blocks tall, 52 subchunks.
     * The cost is 28 extra pure-air subchunks per column, whose palette holds one entry
     * and which serialize small while still occupying memory. Whether the engine caps
     * dimension height is unconfirmed; setting this back to -64 restores the previous
     * state if a boot fails or a dimension cannot be built. Chunks in an existing save
     * were written against a bottom of -64 and no longer line up, so a test world is best
     * deleted and recreated. */
    inline constexpr int kWorldMinY = -512;
    inline constexpr int kWorldMaxY = 320;

    /**
     * The y of the bedrock layer.
     *
     * With the dimension bottom moved to -512, keeping bedrock at buffer index 0, which
     * is y=-512, would fill 576 layers with dirt between it and the surface, costing
     * memory for nothing. Bedrock therefore stays at y=-64, the 448 blocks below it are
     * all air, and the world a player sees is exactly as it was.
     */
    inline constexpr int kBedrockY = -64;

    static_assert(kWorldMinY % 16 == 0, "the dimension bottom must align to a subchunk boundary");
    static_assert(kWorldMaxY % 16 == 0, "the dimension top must align to a subchunk boundary");
    static_assert(kWorldMinY < kWorldMaxY, "the dimension height range is empty");
    static_assert(kWorldMinY < kBedrockY && kBedrockY < kWorldMaxY, "the bedrock layer must lie inside the dimension range");
} // namespace pier::dimensions
