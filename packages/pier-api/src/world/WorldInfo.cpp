/** world/WorldInfo.cpp —— 只读世界数据查询：村庄与硬编码刷怪区（HSA）巡视。
 *
 * 与 World.cpp（方块读写）分开，让「走一遍内部管理器并序列化」这个关注点
 * 和逐方块热路径隔离。
 *
 * 两个入口都经 sink 流式吐 SNBT 对象（每村庄 / 每区域一条），与
 * list_players / scan_region 同一个模式。全部只读（不改游戏状态），服务器
 * 线程专用。
 *
 * 版本注记：下面的字段按 BDS 26.20.0 头核对过 —— Village 暴露
 * getBounds/getCenter/getPOICount/getUniqueID；逐区块的 HSA 住在
 * LevelChunk::mSpawningAreas，形状是 {aabb, type}。村民枚举刻意省略：村民
 * 挂在按角色分键的 POIInstance weak_ptr 数组上，走一遍既脆弱又对版本敏感
 * —— POI 数量才是稳定信号。以后哪个版本需要村民，就在这里加，ABI 形状
 * 不用动（载荷是数据，不是布局）。
 */
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

                // getVillageManager() 返回 unique_ptr const&（对象存储 ——
                // TypedStorage 本身就是值；成员上不用做 .get() 体操）。
                auto const& mgr = dim->getVillageManager();
                if (!mgr) return;

                // mVillages：unordered_map<UUID, shared_ptr<Village>>（对象存
                // 储，.get() 给出 map）。读的是按引用持有的活对象的私有成
                // 员 —— 服务器线程上没有生命期险。
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
                // 半径上限（1024 格 = 129×129 个区块）；`x ± radius` 用 64 位算，
                // 防止 int 溢出把方阵翻成负数区间。
                if (radius > 1024) return;
                auto dim = level->getDimension(DimensionType{dimension}).lock();
                if (!dim) return;

                BlockSource& region = dim->getBlockSourceFromMainChunkSource();

                // HSA 按 LevelChunk 存。走覆盖半径的区块方阵（16 格一区块）；
                // 只有已加载的区块产出数据 —— 这是诚实的上限（读未加载的区块
                // 就得生成它，而一个只读查询绝不能这么干）。
                int cxMin = static_cast<int>((int64_t{x} - radius) >> 4);
                int cxMax = static_cast<int>((int64_t{x} + radius) >> 4);
                int czMin = static_cast<int>((int64_t{z} - radius) >> 4);
                int czMax = static_cast<int>((int64_t{z} + radius) >> 4);
                (void)y; // HSA 按区块全高存在；选取用不到 y

                for (int cx = cxMin; cx <= cxMax; ++cx)
                {
                    for (int cz = czMin; cz <= czMax; ++cz)
                    {
                        LevelChunk* chunk = region.getChunk(ChunkPos{cx, cz});
                        if (!chunk) continue; // 未加载 —— 跳过，不强加载

                        for (auto const& area : chunk->mSpawningAreas.get())
                        {
                            // SpawningArea.aabb 是包着 BoundingBox 的
                            // TypedStorage（对象类型 —— 要 .get()）；.type 是标
                            // 量枚举（引用/标量特化 —— 成员本身就是值）。
                            // BoundingBox 的 min/max 是 BlockPos（整数）。
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
