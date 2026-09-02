/** world/Edit.cpp: native entry points for bulk world editing.
 * Three existing engine entry points are exposed, bypassing the setblock console command
 * underneath api_set_block. A stateful block write goes through
 * BlockSerializationUtils::tryGetBlockFromNBT plus BlockSource::setBlock, a block entity write
 * through BlockSource::getBlockEntity plus BlockActor::load, and placing an actor from NBT through
 * ActorFactory::loadActor plus Level::addEntity. Three things the command path cannot do. Block
 * states must be translated from serialized NBT into the ["k"=v] command syntax, and one bad
 * translation fails the whole command while the cell silently stays as it was, which has cost
 * stair facing, log axis and door hinge side. Block entities cannot be written back at all, so
 * chest contents, sign text and the mob inside a spawner are lost on a copy. Actors are addressed
 * by type name only, so variant, equipment and age are lost. It also removes the three layers of
 * command parsing, permission checking and dispatch. The /setblock path stays, because a block
 * spec typed by a player is easiest through command parsing. This file adds entry points rather
 * than replacing them, and an existing mod runs unchanged. / */
#ifndef PIER_BUILD_CLIENT

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "mc/dataloadhelper/DefaultDataLoadHelper.h"
#include "mc/dataloadhelper/NewUniqueIdsDataLoadHelper.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/core/string/HashedString.h"
#include "mc/deps/ecs/gamerefs_entity/EntityContext.h"
#include "mc/deps/ecs/gamerefs_entity/OwnerStorageEntity.h"
#include "mc/deps/game_refs/OwnerPtr.h"
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/deps/nbt/CompoundTagVariant.h"
#include "mc/deps/nbt/FloatTag.h"
#include "mc/deps/nbt/ListTag.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorFactory.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/BlockChangeContext.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/phys/HitResult.h"

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
        bool api_edit_set_block_nbt(
            int32_t dim, int32_t x, int32_t y, int32_t z, PierStr snbt, int32_t update_flags)
        {
            PIER_API_GUARD_BEGIN
                auto* bs = bridge::blockSourceOf(dim);
                if (!bs) return false;
                auto parsed = CompoundTag::fromSnbt(sv(snbt));
                if (!parsed) return false;
                Block const* block = bridge::blockFromTag(*parsed);
                if (!block) return false;
                return bs->setBlock(
                    BlockPos{x, y, z}, *block, update_flags, nullptr, bridge::blockEditContext());
            PIER_API_GUARD_END
        }

        bool api_edit_set_block_states(
            int32_t dim,
            int32_t x,
            int32_t y,
            int32_t z,
            PierStr name,
            PierStr states_snbt,
            int32_t update_flags)
        {
            PIER_API_GUARD_BEGIN
                auto* bs = bridge::blockSourceOf(dim);
                if (!bs) return false;
                Block const* def = bridge::defaultBlockNamed(sv(name));
                if (!def) return false;

                std::string_view states = sv(states_snbt);
                if (states.empty())
                {
                    // No state to override, so the default state is the answer and
                    // no parsing is needed at all.
                    return bs->setBlock(
                        BlockPos{x, y, z}, *def, update_flags, nullptr, bridge::blockEditContext());
                }

                // Starts from the serialization tag of the default block and overrides
                // only the states the caller supplied, rather than having the caller
                // assemble the whole {name,states,version}. version must be the current
                // one and a caller has no reliable way to know it. A wrong or missing
                // version makes the engine treat the write as an ancient save and run
                // the upgrade table, which shows up as writing one state and getting a
                // different one.
                CompoundTag tag = def->getSerializationId();
                auto extra = CompoundTag::fromSnbt(states);
                if (!extra) return false;
                auto& target = tag["states"];
                if (target.hold<CompoundTag>())
                {
                    auto& base = target.get<CompoundTag>();
                    for (auto const& kv : extra->mTags) base[kv.first] = kv.second;
                }
                else
                {
                    tag["states"] = std::move(*extra);
                }

                Block const* block = bridge::blockFromTag(tag);
                if (!block) return false;
                return bs->setBlock(
                    BlockPos{x, y, z}, *block, update_flags, nullptr, bridge::blockEditContext());
            PIER_API_GUARD_END
        }

        bool api_edit_set_block_entity(int32_t dim, int32_t x, int32_t y, int32_t z, PierStr snbt)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                auto* bs = bridge::blockSourceOf(dim);
                if (!level || !bs) return false;
                BlockPos pos{x, y, z};
                auto* be = bs->getBlockEntity(pos);
                // There is no block entity at that cell, either because the caller has
                // the order wrong and should place the block before filling it, or
                // because the block placed has no block entity at all. Reported as
                // false rather than pretending to succeed.
                if (!be) return false;

                auto parsed = CompoundTag::fromSnbt(sv(snbt));
                if (!parsed) return false;

                // The x, y and z in the snapshot are the source position. Left
                // unchanged, some block entities such as pistons and command blocks
                // look themselves up at that coordinate, and the result is right
                // contents with wrong behavior.
                (*parsed)["x"] = x;
                (*parsed)["y"] = y;
                (*parsed)["z"] = z;

                DefaultDataLoadHelper helper{};
                be->load(*level, *parsed, helper);
                be->setChanged();
                // setChanged only marks it dirty. Without this step the server is
                // correct while the client still shows an empty chest until the chunk
                // reloads, by which time the player has concluded the copy failed.
                be->onChanged(*bs);
                return true;
            PIER_API_GUARD_END
        }

        bool api_edit_spawn_entity_nbt(
            int32_t dim,
            PierStr snbt,
            bool use_pos,
            double x,
            double y,
            double z,
            PierActorId* out)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                auto* bs = bridge::blockSourceOf(dim);
                if (!level || !bs) return false;

                auto parsed = CompoundTag::fromSnbt(sv(snbt));
                if (!parsed) return false;
                CompoundTag tag = std::move(*parsed);

                if (use_pos)
                {
                    ListTag pos;
                    pos.add(std::make_unique<FloatTag>(static_cast<float>(x)));
                    pos.add(std::make_unique<FloatTag>(static_cast<float>(y)));
                    pos.add(std::make_unique<FloatTag>(static_cast<float>(z)));
                    tag["Pos"] = std::move(pos);
                }

                // NewUniqueIdsDataLoadHelper maps the UniqueID in the NBT onto a new
                // id, which is what /structure load uses when it places actors. Keeping
                // the id from the snapshot collides with the source actor, and a
                // collision makes the engine treat two actors as one: one vanishes and
                // the other misbehaves, with nothing in the log.
                NewUniqueIdsDataLoadHelper helper{*level};
                auto owner = level->getActorFactory().loadActor(&tag, helper);
                if (!owner) return false;

                Actor* actor = level->addEntity(*bs, std::move(owner));
                if (!actor) return false;
                if (out) *out = actor->getOrCreateUniqueID().rawID;
                return true;
            PIER_API_GUARD_END
        }

        bool api_edit_trace_ray(
            PierActorId id,
            float max_dist,
            bool include_actors,
            bool include_blocks,
            void* ctx,
            PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                Actor* a = bridge::resolveActor(id);
                if (!a || !sink) return false;
                auto hr = a->traceRay(max_dist, include_actors, include_blocks);

                // actor_trace_ray emits only mPos, a floating point hit point. That
                // point lands exactly on the face of a block, so floor() picks the
                // neighboring cell about half the time, which makes any select-the-
                // block-under-the-crosshair feature impossible. mBlock and mFacing are
                // in HitResult already and are emitted here.
                std::string out = "{type:" + snbtNum(static_cast<int>(hr.mType));
                out += ",block:[" + snbtNum(hr.mBlock.x) + "," + snbtNum(hr.mBlock.y) + ","
                    + snbtNum(hr.mBlock.z) + "]";
                out += ",facing:" + snbtNum(static_cast<int>(hr.mFacing));
                out += ",pos:[" + snbtDouble(hr.mPos.x) + "," + snbtDouble(hr.mPos.y) + ","
                    + snbtDouble(hr.mPos.z) + "]";

                int64_t entityId = 0;
                if (hr.mType == HitResultType::Entity)
                {
                    // mEntity is a WeakEntityRef and tryUnwrap<Actor>() is the safe
                    // dereference LL provides, yielding empty when the actor is already
                    // gone rather than a dangling pointer.
                    if (auto hit = hr.mEntity.tryUnwrap<Actor>())
                    {
                        entityId = hit->getOrCreateUniqueID().rawID;
                    }
                }
                out += ",entity:" + snbtNum(entityId) + "L}";
                sink(ctx, ps(out));
                return true;
            PIER_API_GUARD_END
        }

        //  The liquid layer, meaning waterlogging
        //
        // Waterlogging in Bedrock is a second block on the same cell and not a block
        // state: the main layer holds the stairs and the liquid layer holds water.
        // get_block and set_block see the main layer only, so copying a waterlogged
        // block loses the water. The main layer is entirely correct and the other layer
        // is what is missing.

        bool api_get_extra_block(
            int32_t dim, int32_t x, int32_t y, int32_t z, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                if (!sink) return false;
                auto* bs = bridge::blockSourceOf(dim);
                if (!bs) return false;
                auto const& block = bs->getExtraBlock(BlockPos{x, y, z});
                // An empty liquid layer returns air and that is passed through as is,
                // which is how a caller tells that the cell is not waterlogged.
                sink(ctx, ps(block.getTypeName()));
                return true;
            PIER_API_GUARD_END
        }

        bool api_set_extra_block(
            int32_t dim, int32_t x, int32_t y, int32_t z, PierStr blockSpec, int32_t updateFlags)
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
                return bs->setExtraBlock(BlockPos{x, y, z}, *block, updateFlags);
            PIER_API_GUARD_END
        }

        void fill(PierApi& api)
        {
            api.edit_set_block_nbt = &api_edit_set_block_nbt;
            api.edit_set_block_states = &api_edit_set_block_states;
            api.edit_set_block_entity = &api_edit_set_block_entity;
            api.edit_spawn_entity_nbt = &api_edit_spawn_entity_nbt;
            api.edit_trace_ray = &api_edit_trace_ray;
            api.get_extra_block = &api_get_extra_block;
            api.set_extra_block = &api_set_extra_block;
        }

        spi::SlotPackReg reg{{"edit", &fill}};
    } // namespace
} // namespace pier::api_impl

#endif // !PIER_BUILD_CLIENT
