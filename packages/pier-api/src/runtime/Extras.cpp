/** runtime/Extras.cpp: appended slots.
 *
 * One slot so far, level_set_biome, which changes the biome and corresponds to the
 * a worldedit setbiome command. It exists so callers do not reach for a gamerule. A
 * gamerule is one value for the whole save, so using it for a per-world setting never
 * works: an operator changing one world changes the other.
 *
 * Appended slots live in their own file because their lifecycle differs from the
 * existing ones. They are guarded by struct_size and may be adjusted on their own when
 * BDS changes a signature. Mixed into another domain, which slots were appended could
 * only be recovered from git. Compiled into both targets.
 *
 * Read the existing 190 slots before adding one. Despawning an actor and setting
 * health are covered by AACT_DESPAWN and AACT_HEAL on actor_action, and a separate
 * slot would only fork the implementation.
 */
#include <cstdint>
#include <algorithm>
#include <string>

// `ChunkBlockPos.h` sits under `mc/world/level/` and not `.../level/chunk/`, which
// holds `LevelChunk.h`. Both names contain chunk while the directories differ.
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/ChunkBlockPos.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/biome/Biome.h"
#include "mc/world/level/biome/registry/BiomeRegistry.h"
#include "mc/world/level/chunk/LevelChunk.h"

#include "sdk/abi.h"

#include "pier/api/bridge.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/log.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        int32_t api_level_set_biome(
            int32_t dim, int32_t minX, int32_t minZ, int32_t maxX, int32_t maxZ, PierStr biome)
        {
            PIER_API_GUARD_BEGIN
                BlockSource* bs = bridge::blockSourceOf(dim);
                if (!bs) return 0;

                std::string const name = toString(biome);
                if (name.empty()) return 0;

                auto* registry =
                    bridge::levelReady() ? &bridge::levelReady()->getBiomeRegistry() : nullptr;
                if (!registry) return 0;
                Biome const* target = registry->lookupByName(name);
                // An unrecognized name sets no column at all rather than the default
                // biome. Silently substituting plains is worse than reporting failure,
                // because the operator sees the terrain change into something other
                // than what was asked for.
                if (!target) return 0;

                if (minX > maxX) std::swap(minX, maxX);
                if (minZ > maxZ) std::swap(minZ, maxZ);

                // Without an area cap one call can freeze the server thread, since
                // callers often pass a player selection straight through, and with
                // maxX == INT32_MAX the `++x` overflows and the loop never ends. The
                // count is 64-bit and the area is capped.
                constexpr int64_t kMaxColumns = int64_t{4096} * 4096;
                int64_t const area = (int64_t{maxX} - minX + 1) * (int64_t{maxZ} - minZ + 1);
                if (area > kMaxColumns)
                {
                    hostLogger().error(
                        "[api] level_set_biome refused, area of {} columns exceeds the limit of {}; split the call", area, kMaxColumns);
                    return -1;
                }

                // One chunk lookup per chunk and not per column: getChunkAt is a virtual
                // call plus a map find, and a 4096x4096 area is sixteen million of them
                // column-wise against sixty-five thousand chunk-wise. An unloaded chunk is
                // skipped and never force-loaded, because force-loading a large area stalls
                // the main thread for seconds. setBiome2d fills every y of the column with
                // one biome, which is what the contract specifies, while setBiome3d writes
                // only the sample at pos.y.
                auto const floorDiv16 = [](int64_t v)
                { return static_cast<int>(v >= 0 ? v >> 4 : -((-v + 15) >> 4)); };
                int const cxMin = floorDiv16(minX), cxMax = floorDiv16(maxX);
                int const czMin = floorDiv16(minZ), czMax = floorDiv16(maxZ);
                int32_t done = 0;
                for (int cx = cxMin; cx <= cxMax; ++cx)
                {
                    int const xLo = std::max<int>(minX, cx * 16), xHi = std::min<int>(maxX, cx * 16 + 15);
                    for (int cz = czMin; cz <= czMax; ++cz)
                    {
                        int const zLo = std::max<int>(minZ, cz * 16), zHi = std::min<int>(maxZ, cz * 16 + 15);
                        LevelChunk* chunk = bs->getChunkAt(BlockPos(xLo, 0, zLo));
                        if (!chunk) continue;
                        for (int x = xLo; x <= xHi; ++x)
                        {
                            for (int z = zLo; z <= zHi; ++z)
                            {
                                // ChunkBlockPos(x, 0, z) does not compile: the middle
                                // parameter is a ChunkLocalHeight and does not convert
                                // implicitly from int. A column needs no y, so from2D.
                                chunk->setBiome2d(
                                    *target,
                                    ChunkBlockPos::from2D(
                                        static_cast<uint8_t>(x & 15), static_cast<uint8_t>(z & 15)));
                                ++done;
                            }
                        }
                    }
                }
                return done;
            PIER_API_GUARD_END
        }

        void fill(PierApi& api) { api.level_set_biome = &api_level_set_biome; }

        spi::SlotPackReg reg{{"extras", &fill}};
    } // namespace
} // namespace pier::api_impl
