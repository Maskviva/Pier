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

    /** The bottom is the vanilla bottom, -64.
     *
     *  It was -512 for a while, to meet a client that requested subchunks -32..-24 for a
     *  custom dimension: with no definition of its own to go by, the client fell back to
     *  the largest possible world. That fallback existed because the definition the
     *  engine hands the client never carried a height; NativeDimensions now writes the
     *  height into it, so the workaround has nothing left to do.
     *
     *  It also had a cost. A dimension 52 subchunks tall is outside anything the engine
     *  ships, and a server entering one died on a chunk worker a third of a second later,
     *  every time, with the height the only thing that never varied across the runs. A
     *  spec may still ask for more; nothing here forbids it or vouches for it. */
    inline constexpr int kWorldMinY = -64;
    inline constexpr int kWorldMaxY = 320;

    /** The y of the bedrock layer: the vanilla bottom, which is now also the dimension
     *  bottom. Kept apart from kWorldMinY so a spec that lowers the bottom keeps its
     *  bedrock where the terrain expects it. */
    inline constexpr int kBedrockY = -64;

    static_assert(kWorldMinY % 16 == 0, "the dimension bottom must align to a subchunk boundary");
    static_assert(kWorldMaxY % 16 == 0, "the dimension top must align to a subchunk boundary");
    static_assert(kWorldMinY < kWorldMaxY, "the dimension height range is empty");
    static_assert(kWorldMinY <= kBedrockY && kBedrockY < kWorldMaxY, "the bedrock layer must lie inside the dimension range");
} // namespace pier::dimensions
