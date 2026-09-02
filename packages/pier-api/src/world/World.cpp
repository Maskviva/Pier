/** world/World.cpp: world reads and writes. Particles, region scans, single block
 *  reads and writes, block properties and actions, block entity snapshots, explosions.
 *
 *  A block handle is a (dimension, coordinate) pair, re-resolved against the live
 *  BlockSource on every call.
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
                // Aligned with the single player identity model: resolvePlayer matches
                // getRealName() first and falls back to getNameTag(), the display name.
                // For an ordinary player the two are identical.
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

                // A volume cap plus 64-bit loop variables, so a bound at INT32_MAX
                // cannot loop forever.
                constexpr int64_t kMaxVolume = int64_t{1} << 24; // 16M cells, about 256x256x256
                int64_t const volume = (int64_t{maxX} - minX + 1) * (int64_t{maxY} - minY + 1)
                    * (int64_t{maxZ} - minZ + 1);
                if (blocksSink && volume > kMaxVolume)
                {
                    hostLogger().error(
                        "[api] scan_region refused, a region of {} cells exceeds the limit of {}; scan in blocks", volume, kMaxVolume);
                    return false;
                }

                // Blocks: the whole box cell by cell, bottom to top, then x, then z.
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
                                // This LL version has no getSerializationId() accessor,
                                // so the public member mSerializationId is read
                                // directly. It is the same tag:
                                // {name, states, version}.
                                std::string snbt =
                                    block.mSerializationId.get().toSnbt(SnbtFormat::Minimize);
                                blocksSink(ctx, static_cast<int>(x), static_cast<int>(y), static_cast<int>(z),
                                           ps(block.getTypeName()), ps(snbt));
                            }
                        }
                    }
                }

                // Actors: the runtime entity table filtered by the box and mapped
                // onto cells.
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

        //  Single block reads and writes

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
         * A native block write, not /execute in ... run setblock. What the command path costs:
         * failure is a bare bool with no reason, so a misspelled block name, an unloaded dimension
         * and a coordinate in an ungenerated chunk all look the same; every write goes through
         * command parsing, a permission check and origin construction, while one bulk edit touches
         * hundreds of thousands of blocks; update flags cannot be controlled, so not producing
         * drops while pasting is inexpressible; and the side effects are visible to command events
         * and to the log. BlockSource::setBlock is used directly, the same path as
         * api_edit_set_block_nbt. blockSpec accepts both forms: minecraft:stone or stone takes the
         * default state, while {name:"minecraft:stone",states:{...}} is full serialized NBT and
         * runs the engine version upgrade table. An unrecognized block name returns false rather
         * than quietly filling in a placeholder the way getDefaultBlockState does, which would let
         * a fill with a misspelled name wipe an entire area. / */
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

                // DEFAULT is NEIGHBORS | NETWORK, which matches what /setblock feels
                // like: neighbors update and the change syncs to clients. Anything else
                // goes through edit_set_block_nbt, which takes flags. The fifth
                // argument is a BlockChangeContext reference and cannot be nullptr. The
                // same change source as a fill is used so that a hook another plugin
                // installed on block changes sees what it saw before.
                return bs->setBlock(BlockPos{x, y, z}, *block, 3, nullptr, bridge::blockEditContext());
            PIER_API_GUARD_END
        }

        //  Block properties

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
                    // Block::isCraftingBlock() and isInteractiveBlock() are not present
                    // in the generated headers of every BDS 26.20.x point release. They
                    // follow the Mojang export symbols and moved between 26.20.0 and
                    // 26.20.2. This reports unsupported instead of failing to compile,
                    // and the safety layer turns that into an error for these two
                    // properties only.
                    return false;
                case PIER_BPROP_HAS_BLOCK_ENTITY:
                    *out = (bs->getBlockEntity(BlockPos{x, y, z}) != nullptr) ? 1.0 : 0.0;
                    return true;
                /*  Appended: block gap fills  */
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
                    // getComparatorSignal(BlockSource&, BlockPos const&, uchar dir).
                    // dir=0, downward, is the safe default, and a caller needing a
                    // specific direction uses the block action API.
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
                    // getBounciness(IConstBlockSource const&, BlockPos const&).
                    // Bounciness that depends on the region, as for a slime block,
                    // needs the context.
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
                /*  Appended  */
                case PIER_BSTR_STATE:
                {
                    // Emits every block state as SNBT of the form {name:value,...}.
                    sink(ctx, ps(block->mSerializationId.get().toSnbt(SnbtFormat::Minimize)));
                    return true;
                }
                case PIER_BSTR_COLLISION_SHAPE:
                {
                    // getCollisionShape(AABB& out, IConstBlockSource const&,
                    // BlockPos const&, optional_ref) fills an AABB and returns true
                    // when the block has a collision box. A multi-box shape needs
                    // BlockSource::fetchCollisionShapes; only the primary shape is
                    // reported here.
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
                    // getOutline(IConstBlockSource const&, BlockPos const&,
                    // AABB& buffer) returns a const reference to buffer, which stays
                    // valid with buffer on the stack.
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
                /*  Appended  */
                case PIER_BACT_GET_STATE:
                {
                    // Reading a single block state by name. This LL version has no
                    // point getState(name), so it reports unsupported until a proper
                    // BlockState API is available.
                    return false;
                }
                case PIER_BACT_POP_RESOURCE:
                {
                    // Native drops rather than `/setblock ... air destroy`.
                    // Level::destroyBlock is what the command does underneath, and it
                    // returns whether the block was really destroyed, where the command
                    // path can only report that the command ran.
                    auto* level = bridge::levelReady();
                    if (!level) return false;
                    return level->destroyBlock(
                        *bs, BlockPos{x, y, z}, /*dropResources=*/true, bridge::blockEditContext());
                }
                case PIER_BACT_AS_ITEM:
                {
                    if (!out) return false;
                    // asItemInstance(BlockSource&, BlockPos const&) returns an
                    // ItemInstance and not an ItemStack. ItemInstance has no SNBT
                    // serializer compatible with the itemToSnbt signature, so its
                    // user-data CompoundTag is serialized instead.
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

        //  Explosions

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
                // Without a cap, one call blows up every loaded chunk and freezes the
                // thread. The largest vanilla explosion, from a wither or an end
                // crystal, has a radius of at most 8; this allows up to 64.
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
