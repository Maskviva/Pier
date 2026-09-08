/** main.cpp: drives the engine-free pack layer for the equivalence test.
 * Usage: pack_check <pack> <minY> <maxY> <chunkX> <chunkZ> [name=value ...] [role:name=block ...]
 * Prints one line per column: the run-length encoded material names of the column. */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "pier/dimensions/pack/pack_reader.h"
#include "pier/dimensions/pack/template_pack.h"

using namespace pier::dimensions::pack;

int main(int argc, char** argv)
{
    if (argc < 6) { std::fprintf(stderr, "usage\n"); return 2; }
    std::vector<std::string> problems;
    auto file = PackFile::load(argv[1], problems);
    if (!file) { for (auto& p : problems) std::fprintf(stderr, "load: %s\n", p.c_str()); return 1; }
    auto pack = decodeTemplate(*file, problems);
    if (!pack) { for (auto& p : problems) std::fprintf(stderr, "decode: %s\n", p.c_str()); return 1; }
    std::map<std::string, std::int64_t> params;
    std::map<std::string, std::string> roles;
    for (int i = 6; i < argc; ++i)
    {
        std::string a = argv[i];
        auto eq = a.find('=');
        if (eq == std::string::npos) continue;
        if (a.rfind("role:", 0) == 0) roles[a.substr(5, eq - 5)] = a.substr(eq + 1);
        else params[a.substr(0, eq)] = std::atoll(a.c_str() + eq + 1);
    }
    auto m = mountTemplate(*pack, params, roles, std::atoi(argv[2]), std::atoi(argv[3]), problems);
    if (!m) { for (auto& p : problems) std::fprintf(stderr, "mount: %s\n", p.c_str()); return 3; }
    std::vector<std::uint16_t> out;
    generateTemplateChunk(*m, std::atoi(argv[4]), std::atoi(argv[5]), out);
    auto h = static_cast<std::size_t>(m->height());
    std::string line;
    for (std::size_t c = 0; c < 256; ++c)
    {
        line.clear();
        std::uint16_t run = out[c * h];
        std::size_t n = 0;
        for (std::size_t y = 0; y < h; ++y)
        {
            if (out[c * h + y] == run) { ++n; continue; }
            line += m->materials[run] + "*" + std::to_string(n) + " ";
            run = out[c * h + y]; n = 1;
        }
        line += m->materials[run] + "*" + std::to_string(n);
        std::puts(line.c_str());
    }
    std::size_t statics = 0;
    for (auto s : m->staticLayer) statics += s;
    std::fprintf(stderr, "static=%zu period=%d,%d\n", statics, m->periodX, m->periodZ);
    return 0;
}
