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
     * Verifies, and corrects when needed, the vertical range a dimension object holds.
     *
     * The client learns the height of a dimension from the DimensionDefinition inside
     * DimensionDataPacket, while the server decides whether a subchunk request is out of range
     * through Dimension::isSubChunkHeightWithinRange, which reads Dimension::mHeightRange. These
     * are two independent values, and when they disagree the client requests subchunks at the
     * height it was given, the server judges them against its own, and every request comes back
     * IndexOutOfBounds so no block data gets through. The symptom is empty chunks where individual
     * block updates still display.
     *
     * expectedMin and expectedMax must be exactly the pair written into the DimensionDefinition at
     * registration. / */
    void verifyHeightRange(::Dimension& dim, int expectedMin, int expectedMax, char const* who);
} // namespace pier::dimensions
