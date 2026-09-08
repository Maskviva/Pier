/** vol_main.cpp: drives the engine-free volume layer for the equivalence test.
 * Usage: vol_check <pack> <seed> <chunkX> <chunkZ>
 * Prints one line per column: the biome index, then run-length encoded material names. */
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "pier/dimensions/pack/pack_reader.h"
#include "pier/dimensions/pack/volume_pack.h"

using namespace pier::dimensions::pack;

int main(int argc, char** argv)
{
    if (argc < 5) { std::fprintf(stderr, "usage\n"); return 2; }
    std::vector<std::string> problems;
    auto file = PackFile::load(argv[1], problems);
    if (!file) { for (auto& p : problems) std::fprintf(stderr, "load: %s\n", p.c_str()); return 1; }
    auto pack = decodeVolume(*file, problems);
    if (!pack) { for (auto& p : problems) std::fprintf(stderr, "decode: %s\n", p.c_str()); return 1; }
    auto shared = std::make_shared<VolumePack const>(std::move(*pack));
    SeededVolume seeded(shared, std::strtoull(argv[2], nullptr, 10));
    std::vector<std::uint16_t> out;
    std::vector<std::uint32_t> biomes;
    seeded.generateChunk(std::atoi(argv[3]), std::atoi(argv[4]), out, biomes);
    auto h = static_cast<std::size_t>(seeded.height());
    auto const& mats = seeded.materials();
    std::string line;
    for (std::size_t c = 0; c < 256; ++c)
    {
        line = std::to_string(biomes[c]) + " ";
        std::uint16_t run = out[c * h];
        std::size_t n = 0;
        for (std::size_t y = 0; y < h; ++y)
        {
            if (out[c * h + y] == run) { ++n; continue; }
            line += mats[run] + "*" + std::to_string(n) + " ";
            run = out[c * h + y]; n = 1;
        }
        line += mats[run] + "*" + std::to_string(n);
        std::puts(line.c_str());
    }
    return 0;
}
