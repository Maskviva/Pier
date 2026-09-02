/** world/WorldInfo.cpp: read-only world data queries, walking villages and hardcoded
 * spawn areas (HSA). Separate from World.cpp, which reads and writes blocks, so that walking an
 * internal manager and serializing it stays away from the per-block hot path. Both entry points
 * stream SNBT objects through a sink, one per village or per area, the same pattern list_players
 * and scan_region use. Everything is read-only, changes no game state, and is server thread only.
 * Version note: the fields below were checked against the BDS 26.20.0 headers. Village exposes
 * getBounds, getCenter, getPOICount and getUniqueID, and the per-chunk HSA lives in
 * LevelChunk::mSpawningAreas with the shape {aabb, type}. Villager enumeration is deliberately
 * omitted: villagers hang off POIInstance weak_ptr arrays keyed by role, and walking them is both
 * fragile and version sensitive, while the POI count is the stable signal. A version that needs
 * villagers adds them here without touching the ABI shape, since the payload is data and not
 * layout. / */
#ifndef PIER_BUILD_CLIENT

#include <cstdint>
#include <cmath>
#include <memory>
#include <string>

#include "mc/deps/core/math/Vec3.h"
#include "mc/platform/UUID.h"
#include "mc/world/actor/ai/village/Village.h"
#include "mc/world/actor/ai/village/VillageManager.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/ChunkPos.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/chunk/LevelChunk.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/levelgen/structure/BoundingBox.h"
#include "mc/world/level/levelgen/v1/HardcodedSpawnAreaType.h"
#include "mc/world/phys/AABB.h"

#include "sdk/abi.h"

#include "pier/api/bridge.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/snbt.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        std::string hsaTypeName(HardcodedSpawnAreaType t)
        {
            switch (t)
            {
            case HardcodedSpawnAreaType::NetherFortress:
                return "nether_fortress";
            case HardcodedSpawnAreaType::WitchHut:
                return "witch_hut";
            case HardcodedSpawnAreaType::OceanMonument:
                return "ocean_monument";
            case HardcodedSpawnAreaType::PillagerOutpost:
                return "pillager_outpost";
            case HardcodedSpawnAreaType::VillageDeprecated:
            case HardcodedSpawnAreaType::NewVillageDeprecated:
                return "village_deprecated";
            default:
                return "none";
            }
        }

        void api_villages(int32_t dimension, void* ctx, PierStrSink snbtSink)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level || !snbtSink) return;
                auto dim = level->getDimension(DimensionType{dimension}).lock();
                if (!dim) return;

                // getVillageManager() returns a unique_ptr const&. It is object
                // storage, so the TypedStorage is the value itself and no .get()
                // gymnastics are needed on the member.
                auto const& mgr = dim->getVillageManager();
                if (!mgr) return;

                // mVillages is an unordered_map<UUID, shared_ptr<Village>> in object
                // storage, where .get() yields the map. What is read is a private
                // member of a live object held by reference, which carries no lifetime
                // risk on the server thread.
                for (auto const& [id, villagePtr] : mgr->mVillages.get())
                {
                    if (!villagePtr) continue;
                    Village& v = *villagePtr;
                    AABB const& b = v.getBounds();
                    Vec3 c = v.getCenter();

                    std::string snbt = "{\"uuid\":\"" + snbtEscape(v.getUniqueID().asString())
                        + "\",\"center\":[" + snbtDouble(c.x) + "," + snbtDouble(c.y)
                        + "," + snbtDouble(c.z) + "]"
                        + ",\"bounds\":{\"min\":[" + snbtDouble(b.min.x) + ","
                        + snbtDouble(b.min.y) + "," + snbtDouble(b.min.z)
                        + "],\"max\":[" + snbtDouble(b.max.x) + ","
                        + snbtDouble(b.max.y) + "," + snbtDouble(b.max.z) + "]}"
                        + ",\"poi_count\":" + snbtNum(v.getPOICount()) + "}";
                    snbtSink(ctx, ps(snbt));
                }
            PIER_API_GUARD_END_VOID
        }

        void api_structures_near(
            int32_t dimension, int32_t x, int32_t y, int32_t z, int32_t radius,
            void* ctx, PierStrSink snbtSink)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level || !snbtSink) return;
                if (radius < 0) return;
                // The radius is capped at 1024 blocks, which is 129 by 129 chunks, and
                // `x ± radius` is computed in 64 bits so an int overflow cannot turn
                // the square into a negative range.
                if (radius > 1024) return;
                auto dim = level->getDimension(DimensionType{dimension}).lock();
                if (!dim) return;

                BlockSource& region = dim->getBlockSourceFromMainChunkSource();

                // HSAs are stored per LevelChunk. The square of chunks covering the
                // radius is walked at 16 blocks per chunk, and only loaded chunks yield
                // data, which is the honest limit: reading an unloaded chunk would mean
                // generating it, and a read-only query must never do that.
                int cxMin = static_cast<int>((int64_t{x} - radius) >> 4);
                int cxMax = static_cast<int>((int64_t{x} + radius) >> 4);
                int czMin = static_cast<int>((int64_t{z} - radius) >> 4);
                int czMax = static_cast<int>((int64_t{z} + radius) >> 4);
                (void)y; // An HSA spans the full chunk height, so selection ignores y

                for (int cx = cxMin; cx <= cxMax; ++cx)
                {
                    for (int cz = czMin; cz <= czMax; ++cz)
                    {
                        LevelChunk* chunk = region.getChunk(ChunkPos{cx, cz});
                        if (!chunk) continue; // Not loaded, skipped and never force-loaded

                        for (auto const& area : chunk->mSpawningAreas.get())
                        {
                            // SpawningArea.aabb is a TypedStorage wrapping a
                            // BoundingBox, an object type that needs .get(), while
                            // .type is a scalar enum, where the scalar specialization
                            // makes the member the value itself. min and max of a
                            // BoundingBox are BlockPos, so integers.
                            BoundingBox const& bb = area.aabb.get();
                            std::string snbt = "{\"type\":\"" + hsaTypeName(area.type)
                                + "\",\"bounds\":{\"min\":[" + snbtNum(bb.min.x) + ","
                                + snbtNum(bb.min.y) + "," + snbtNum(bb.min.z)
                                + "],\"max\":[" + snbtNum(bb.max.x) + ","
                                + snbtNum(bb.max.y) + "," + snbtNum(bb.max.z) + "]}}";
                            snbtSink(ctx, ps(snbt));
                        }
                    }
                }
            PIER_API_GUARD_END_VOID
        }

        void fill(PierApi& api)
        {
            api.villages = &api_villages;
            api.structures_near = &api_structures_near;
        }

        spi::SlotPackReg reg{{"world-info", &fill}};
    } // namespace
} // namespace pier::api_impl

#endif // !PIER_BUILD_CLIENT
