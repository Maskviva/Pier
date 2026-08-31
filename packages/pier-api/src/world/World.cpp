/** world/World.cpp —— 世界读写：粒子、区域扫描、单方块读写、方块属性/动作、
 *  方块实体快照、爆炸。
 *
 *  方块「句柄」是（维度, 坐标），每次调用对着活的 BlockSource 重新解析。
 */
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <string>

#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/core/string/HashedString.h"
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/item/SaveContext.h"
#include "mc/world/item/SaveContextFactory.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/BlockChangeContext.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/phys/AABB.h"

#include "sdk/abi.h"

#include "pier/api/bridge.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/log.h"
#include "pier/support/snbt.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        bool api_spawn_particle(int32_t dimension, PierStr effectName, double x, double y, double z)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level) return false;
                auto dim = level->getDimension(DimensionType{dimension}).lock();
                if (!dim) return false;
                level->spawnParticleEffect(
                    toString(effectName), Vec3{(float)x, (float)y, (float)z}, dim.get());
                return true;
            PIER_API_GUARD_END
        }

        PierPlayerPos api_get_player_position(PierStr name)
        {
            PIER_API_GUARD_BEGIN
                PierPlayerPos out{0.0, 0.0, 0.0, 0, false};
                // 与统一的玩家身份模型对齐：resolvePlayer 先按 getRealName() 匹
                // 配，落空再按 getNameTag()（显示名）—— 普通玩家两者相同，旧行
                // 为得以保留。
                Player* p = bridge::resolvePlayer(PierPlayerSel{0, name});
                if (!p) return out;
                auto pos = p->getPosition();
                out.x = pos.x;
                out.y = pos.y;
                out.z = pos.z;
                out.dimension = static_cast<int>(p->getDimensionId());
                out.found = true;
                return out;
            PIER_API_GUARD_END_VAL((PierPlayerPos{0.0, 0.0, 0.0, 0, false}))
        }

        bool api_scan_region(
            int32_t dimension,
            int32_t x1,
            int32_t y1,
            int32_t z1,
            int32_t x2,
            int32_t y2,
            int32_t z2,
            void* ctx,
            PierBlockSink blocksSink,
            PierEntitySink entitiesSink)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level) return false;
                auto* bs = bridge::blockSourceOf(dimension);
                if (!bs) return false;

                int minX = std::min(x1, x2), maxX = std::max(x1, x2);
                int minY = std::min(y1, y2), maxY = std::max(y1, y2);
                int minZ = std::min(z1, z2), maxZ = std::max(z1, z2);

                // V-13：体积上限 + 64 位循环变量（INT32_MAX 边界不再死循环）。
                constexpr int64_t kMaxVolume = int64_t{1} << 24; // 16M 格，约 256×256×256
                int64_t const volume = (int64_t{maxX} - minX + 1) * (int64_t{maxY} - minY + 1)
                    * (int64_t{maxZ} - minZ + 1);
                if (blocksSink && volume > kMaxVolume)
                {
                    hostLogger().error(
                        "scan_region：区域 {} 格超过上限 {} —— 请分块扫描", volume, kMaxVolume);
                    return false;
                }

                // 方块：逐格走完整个盒子（先自下而上，再 x，再 z）。
                if (blocksSink)
                {
                    for (int64_t y = minY; y <= maxY; ++y)
                    {
                        for (int64_t x = minX; x <= maxX; ++x)
                        {
                            for (int64_t z = minZ; z <= maxZ; ++z)
                            {
                                auto const& block = bs->getBlock(
                                    BlockPos{static_cast<int>(x), static_cast<int>(y), static_cast<int>(z)});
                                // 这个 LL 版本没有 getSerializationId() 访问器；直
                                // 接读公开成员 mSerializationId（同一个标签：
                                // {name, states, version}）。
                                std::string snbt =
                                    block.mSerializationId.get().toSnbt(SnbtFormat::Minimize);
                                blocksSink(ctx, static_cast<int>(x), static_cast<int>(y), static_cast<int>(z),
                                           ps(block.getTypeName()), ps(snbt));
                            }
                        }
                    }
                }

                // 实体：按盒子过滤运行时实体表，落进格子。
                if (entitiesSink)
                {
                    for (auto* actor : level->getRuntimeActorList())
                    {
                        if (!actor) continue;
                        if (static_cast<int>(actor->getDimensionId()) != dimension) continue;
                        auto pos = actor->getPosition();
                        int ex = (int)std::floor(pos.x);
                        int ey = (int)std::floor(pos.y);
                        int ez = (int)std::floor(pos.z);
                        if (ex < minX || ex > maxX || ey < minY || ey > maxY || ez < minZ || ez > maxZ)
                            continue;
                        CompoundTag tag;
                        actor->save(tag);
                        std::string snbt = tag.toSnbt(SnbtFormat::Minimize);
                        entitiesSink(ctx, ex, ey, ez, ps(actor->getTypeName()), ps(snbt));
                    }
                }
                return true;
            PIER_API_GUARD_END
        }

        // ───────────────────── 单方块读写 ─────────────────────

        bool api_get_block(int32_t dim, int32_t x, int32_t y, int32_t z, void* ctx, PierBlockSink sink)
        {
            PIER_API_GUARD_BEGIN
                auto* bs = bridge::blockSourceOf(dim);
                if (!bs || !sink) return false;
                auto const& block = bs->getBlock(BlockPos{x, y, z});
                std::string snbt = block.mSerializationId.get().toSnbt(SnbtFormat::Minimize);
                sink(ctx, x, y, z, ps(block.getTypeName()), ps(snbt));
                return true;
            PIER_API_GUARD_END
        }

        /**
         * 原生写方块。**不走 `/execute in … run setblock`。**
         *
         * 命令路径当初图的是「跨 BDS 版本稳定」，代价却一直在付：
         *
         *   - 失败只有一个 bool，没有任何原因。方块名拼错、维度没加载、坐标
         *     在未生成的区块里 —— 全都长一样，出了问题无从查起。
         *   - 走命令等于每次写方块都过一遍命令解析、权限检查和 origin 构造。
         *     WorldEdit 一次操作几十万个方块，这层开销全是白付的。
         *   - **控制不了 update flags**，所以「粘贴时不要产生掉落物」这类需
         *     求在命令路径上根本无法表达。
         *   - 命令的副作用还会被别的系统看见（命令事件、日志），一次批量编
         *     辑能淹掉整个控制台。
         *
         * 直接走 `BlockSource::setBlock`，和 `api_edit_set_block_nbt` 同一条
         * 路。`blockSpec` 两种写法都收：
         *
         *   - `minecraft:stone` / `stone` —— 取默认状态
         *   - `{name:"minecraft:stone",states:{…}}` —— 完整序列化 NBT，会跑引
         *     擎的版本升级表
         *
         * 认不出的方块名返回 false，**不会**像 getDefaultBlockState 那样安静
         * 地填一个占位方块（那会让 `//set 拼错的名字` 把整片地区刷掉）。
         */
        bool api_set_block(int32_t dim, int32_t x, int32_t y, int32_t z, PierStr blockSpec)
        {
            PIER_API_GUARD_BEGIN
                auto* bs = bridge::blockSourceOf(dim);
                if (!bs) return false;

                std::string_view spec = sv(blockSpec);
                while (!spec.empty() && (spec.front() == ' ' || spec.front() == '\t'))
                    spec.remove_prefix(1);
                if (spec.empty()) return false;

                Block const* block = spec.front() == '{' ? bridge::blockFromSnbt(spec)
                                                         : bridge::defaultBlockNamed(spec);
                if (!block) return false;

                // DEFAULT = NEIGHBORS | NETWORK，和 /setblock 的观感一致：邻居
                // 会更新，变更会同步给客户端。要别的行为用 edit_set_block_nbt，
                // 那个收 flags。第 5 个参数是 BlockChangeContext 的**引用**，不
                // 能传 nullptr。用和 //set 同一个来源，别的插件挂在方块变更上
                // 的钩子看到的东西才不变。
                return bs->setBlock(BlockPos{x, y, z}, *block, 3, nullptr, bridge::blockEditContext());
            PIER_API_GUARD_END
        }

        // ───────────────────── 方块属性 ─────────────────────

        Block const* blockAt(int32_t dim, int32_t x, int32_t y, int32_t z, BlockSource** bsOut = nullptr)
        {
            auto* bs = bridge::blockSourceOf(dim);
            if (!bs) return nullptr;
            if (bsOut) *bsOut = bs;
            return &bs->getBlock(BlockPos{x, y, z});
        }

        bool api_block_get_num(int32_t dim, int32_t x, int32_t y, int32_t z, int32_t prop, double* out)
        {
            PIER_API_GUARD_BEGIN
                BlockSource* bs = nullptr;
                auto const* block = blockAt(dim, x, y, z, &bs);
                if (!block || !out) return false;
                switch (prop)
                {
                case PIER_BPROP_IS_AIR:
                    *out = block->isAir() ? 1.0 : 0.0;
                    return true;
                case PIER_BPROP_DATA:
                    *out = static_cast<double>(block->getData());
                    return true;
                case PIER_BPROP_BLOCK_ITEM_ID:
                    *out = static_cast<double>(block->getBlockItemId());
                    return true;
                case PIER_BPROP_IS_CRAFTING_BLOCK:
                case PIER_BPROP_IS_INTERACTIVE_BLOCK:
                    // Block::isCraftingBlock() / isInteractiveBlock() 不是每个
                    // BDS 26.20.x 小版本的生成头里都有（它们跟着 Mojang 的导出
                    // 符号走，26.20.0 和 26.20.2 之间挪过）。报「不支持」而不是
                    // 编译失败；安全层只对这两个属性把它翻成错误。
                    return false;
                case PIER_BPROP_HAS_BLOCK_ENTITY:
                    *out = (bs->getBlockEntity(BlockPos{x, y, z}) != nullptr) ? 1.0 : 0.0;
                    return true;
                /* ── 追加：方块补漏 ── */
                case PIER_BPROP_LIGHT:
                    *out = static_cast<double>(block->getLight().mValue);
                    return true;
                case PIER_BPROP_LIGHT_EMISSION:
                    *out = static_cast<double>(block->getLightEmission().mValue);
                    return true;
                case PIER_BPROP_DESTROY_SPEED:
                    *out = static_cast<double>(block->getDestroySpeed());
                    return true;
                case PIER_BPROP_EXPLOSION_RESISTANCE:
                    *out = static_cast<double>(block->getExplosionResistance());
                    return true;
                case PIER_BPROP_FRICTION:
                    *out = static_cast<double>(block->getFriction());
                    return true;
                case PIER_BPROP_IS_CONTAINER:
                    *out = block->isContainerBlock() ? 1.0 : 0.0;
                    return true;
                case PIER_BPROP_IS_DOOR:
                    *out = block->isDoorBlock() ? 1.0 : 0.0;
                    return true;
                case PIER_BPROP_IS_FENCE:
                    *out = block->isFenceBlock() ? 1.0 : 0.0;
                    return true;
                case PIER_BPROP_IS_RAIL:
                    *out = block->isRailBlock() ? 1.0 : 0.0;
                    return true;
                case PIER_BPROP_IS_SLAB:
                    *out = block->isSlabBlock() ? 1.0 : 0.0;
                    return true;
                case PIER_BPROP_IS_STAIR:
                    *out = block->isStairBlock() ? 1.0 : 0.0;
                    return true;
                case PIER_BPROP_IS_WALL:
                    *out = block->isWallBlock() ? 1.0 : 0.0;
                    return true;
                case PIER_BPROP_IS_CROP:
                    *out = block->isCropBlock() ? 1.0 : 0.0;
                    return true;
                case PIER_BPROP_IS_UNBREAKABLE:
                    *out = block->isUnbreakable() ? 1.0 : 0.0;
                    return true;
                case PIER_BPROP_REDSTONE_SIGNAL:
                    *out = static_cast<double>(
                        bs->getBlock(BlockPos{x, y, z}).getDirectSignal(*bs, BlockPos{x, y, z}, 0));
                    return true;
                case PIER_BPROP_COMPARATOR_SIGNAL:
                    // getComparatorSignal(BlockSource&, BlockPos const&, uchar dir)
                    // —— dir=0（向下）是安全默认；要指定方向的调用方走方块动作
                    // API。
                    *out = static_cast<double>(
                        block->getComparatorSignal(*bs, BlockPos{x, y, z}, 0));
                    return true;
                case PIER_BPROP_IS_SIGNAL_SOURCE:
                    *out = block->isSignalSource() ? 1.0 : 0.0;
                    return true;
                case PIER_BPROP_VARIANT:
                    *out = static_cast<double>(block->getVariant());
                    return true;
                case PIER_BPROP_BURN_ODDS:
                    *out = static_cast<double>(block->getBurnOdds());
                    return true;
                case PIER_BPROP_FLAME_ODDS:
                    *out = static_cast<double>(block->getFlameOdds());
                    return true;
                case PIER_BPROP_BOUNCINESS:
                    // getBounciness(IConstBlockSource const&, BlockPos const&) ——
                    // 依赖区域的弹性（比如史莱姆块）需要上下文。
                    *out = static_cast<double>(block->getBounciness(*bs, BlockPos{x, y, z}));
                    return true;
                case PIER_BPROP_IS_SOLID:
                    *out = block->isSolid() ? 1.0 : 0.0;
                    return true;
                case PIER_BPROP_REQUIRES_TOOL:
                    *out = block->requiresCorrectToolForDrops() ? 1.0 : 0.0;
                    return true;
                default:
                    return false;
                }
            PIER_API_GUARD_END
        }

        bool api_block_get_str(
            int32_t dim, int32_t x, int32_t y, int32_t z, int32_t prop, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                BlockSource* bs = nullptr;
                auto const* block = blockAt(dim, x, y, z, &bs);
                if (!block || !sink || !bs) return false;
                switch (prop)
                {
                case PIER_BSTR_TYPE_NAME:
                    sink(ctx, ps(block->getTypeName()));
                    return true;
                case PIER_BSTR_SNBT:
                    sink(ctx, ps(block->mSerializationId.get().toSnbt(SnbtFormat::Minimize)));
                    return true;
                case PIER_BSTR_DESCRIPTION_ID:
                    sink(ctx, ps(block->getDescriptionId()));
                    return true;
                case PIER_BSTR_DEBUG_STRING:
                    sink(ctx, ps(block->toDebugString()));
                    return true;
                case PIER_BSTR_TAGS:
                {
                    std::string out = "[";
                    for (auto const& tag : block->mTags.get())
                    {
                        out += "\"" + snbtEscape(tag.getString()) + "\",";
                    }
                    if (out.back() == ',') out.pop_back();
                    out += "]";
                    sink(ctx, ps(out));
                    return true;
                }
                /* ── 追加 ── */
                case PIER_BSTR_STATE:
                {
                    // 把全部方块状态按 SNBT {name:value, …} 序列化出去。
                    sink(ctx, ps(block->mSerializationId.get().toSnbt(SnbtFormat::Minimize)));
                    return true;
                }
                case PIER_BSTR_COLLISION_SHAPE:
                {
                    // getCollisionShape(AABB& out, IConstBlockSource const&,
                    // BlockPos const&, optional_ref) —— 填一个 AABB，方块有碰撞
                    // 箱时返回 true。多盒形状要走
                    // BlockSource::fetchCollisionShapes；这里只报主形状。
                    AABB aabb;
                    bool has = block->getCollisionShape(aabb, *bs, BlockPos{x, y, z}, nullptr);
                    std::string out = has
                        ? ("[{min:[" + snbtDouble(aabb.min.x) + "," + snbtDouble(aabb.min.y)
                           + "," + snbtDouble(aabb.min.z) + "],max:[" + snbtDouble(aabb.max.x)
                           + "," + snbtDouble(aabb.max.y) + "," + snbtDouble(aabb.max.z) + "]}]")
                        : "[]";
                    sink(ctx, ps(out));
                    return true;
                }
                case PIER_BSTR_OUTLINE_SHAPE:
                {
                    // getOutline(IConstBlockSource const&, BlockPos const&, AABB&
                    // buffer) —— 返回 buffer 的 const 引用（buffer 在栈上时调用
                    // 依然成立）。
                    AABB buffer;
                    auto const& aabb = block->getOutline(*bs, BlockPos{x, y, z}, buffer);
                    std::string out = "[{min:[" + snbtDouble(aabb.min.x) + "," + snbtDouble(aabb.min.y)
                        + "," + snbtDouble(aabb.min.z) + "],max:[" + snbtDouble(aabb.max.x) + ","
                        + snbtDouble(aabb.max.y) + "," + snbtDouble(aabb.max.z) + "]}]";
                    sink(ctx, ps(out));
                    return true;
                }
                case PIER_BSTR_DISPLAY_NAME:
                    sink(ctx, ps(block->getDisplayName()));
                    return true;
                default:
                    return false;
                }
            PIER_API_GUARD_END
        }

        bool api_block_action(
            int32_t dim,
            int32_t x,
            int32_t y,
            int32_t z,
            int32_t action,
            PierStr sarg,
            void* ctx,
            PierStrSink out)
        {
            PIER_API_GUARD_BEGIN
                BlockSource* bs = nullptr;
                auto const* block = blockAt(dim, x, y, z, &bs);
                if (!block || !bs) return false;
                switch (action)
                {
                case PIER_BACT_HAS_TAG:
                {
                    bool has = block->hasTag(HashedString{sv(sarg)});
                    if (out) out(ctx, ps(std::string_view{has ? "1" : "0"}));
                    return true;
                }
                /* ── 追加 ── */
                case PIER_BACT_GET_STATE:
                {
                    // 按名字取单个方块状态 —— 这个 LL 版本没有单点的
                    // getState(name)。在正经的 BlockState API 接进来之前报「不
                    // 支持」。
                    return false;
                }
                case PIER_BACT_POP_RESOURCE:
                {
                    // 原生掉落，不再走 `/setblock … air destroy`。
                    //
                    // Level::destroyBlock 就是命令背后做的事，而且它返回是否真
                    // 的破坏成功 —— 命令路径只能给一个「命令跑过了」。
                    auto* level = bridge::levelReady();
                    if (!level) return false;
                    return level->destroyBlock(
                        *bs, BlockPos{x, y, z}, /*dropResources=*/true, bridge::blockEditContext());
                }
                case PIER_BACT_AS_ITEM:
                {
                    if (!out) return false;
                    // asItemInstance(BlockSource&, BlockPos const&) 返回
                    // ItemInstance（不是 ItemStack）。ItemInstance 没有与
                    // itemToSnbt 签名兼容的 SNBT 序列化器，改序列化它的
                    // user-data CompoundTag。
                    auto item = block->asItemInstance(*bs, BlockPos{x, y, z});
                    auto* ud = item.getUserData();
                    out(ctx, ps(ud ? ud->toSnbt(SnbtFormat::Minimize) : std::string{"{}"}));
                    return true;
                }
                default:
                    return false;
                }
            PIER_API_GUARD_END
        }

        bool api_block_entity_snbt(
            int32_t dim, int32_t x, int32_t y, int32_t z, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                auto* bs = bridge::blockSourceOf(dim);
                if (!bs || !sink) return false;
                auto* be = bs->getBlockEntity(BlockPos{x, y, z});
                if (!be) return false;
                CompoundTag tag;
                auto saveCtx = SaveContextFactory::createCloneSaveContext();
                if (!be->save(tag, *saveCtx)) return false;
                sink(ctx, ps(tag.toSnbt(SnbtFormat::Minimize)));
                return true;
            PIER_API_GUARD_END
        }

        // ───────────────────── 爆炸 ─────────────────────

        bool api_explode(
            int32_t dim,
            double x,
            double y,
            double z,
            float radius,
            float maxResistance,
            PierActorId source,
            bool fire,
            bool breaksBlocks,
            bool allowUnderwater)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                auto* bs = bridge::blockSourceOf(dim);
                if (!level || !bs) return false;
                // V-13：半径无上限等于一次调用炸掉整片加载区块并冻住线程。原版
                // 最大的爆炸（凋灵/末影水晶）半径不超过 8；这里放宽到 64。
                if (!(radius >= 0.0f) || radius > 64.0f) return false;
                Actor* src = (source != 0) ? bridge::resolveActor(source) : nullptr;
                return level->explode(
                    *bs,
                    src,
                    Vec3{(float)x, (float)y, (float)z},
                    radius,
                    fire,
                    breaksBlocks,
                    maxResistance,
                    allowUnderwater
                );
            PIER_API_GUARD_END
        }

        void fill(PierApi& api)
        {
            api.spawn_particle = &api_spawn_particle;
            api.get_player_position = &api_get_player_position;
            api.scan_region = &api_scan_region;
            api.get_block = &api_get_block;
            api.set_block = &api_set_block;
            api.block_get_num = &api_block_get_num;
            api.block_get_str = &api_block_get_str;
            api.block_action = &api_block_action;
            api.block_entity_snbt = &api_block_entity_snbt;
            api.explode = &api_explode;
        }

        spi::SlotPackReg reg{{"world", &fill}};
    } // namespace
} // namespace pier::api_impl
