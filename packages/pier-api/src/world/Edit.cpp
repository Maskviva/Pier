/** world/Edit.cpp —— 批量世界编辑的原生入口。
 *
 * # 这个文件解决的是什么
 *
 * 在它之前，SDK 侧往世界里写一个方块只有一条路：`api_set_block`，而那条路
 * 底层曾是 `execute in <dim> run setblock …` **一条控制台命令**。于是三件
 * 事做不了：
 *
 *   1. 方块状态只能靠把序列化 NBT 翻译成 `["k"=v]` 命令语法。翻错一处 =
 *      整条命令失败 = 那一格静默不变。楼梯朝向、原木轴向、门的左右开全在
 *      这条路上丢过。
 *   2. 方块实体写不回去。`block_entity_snbt` 只读不写，箱子里的东西、
 *      告示牌的字、刷怪笼的怪，复制过去就没了。
 *   3. 实体放不回去。`spawn_mob` 只认类型名，快照里的变种 / 装备 / 年龄全丢。
 *
 * 引擎侧这三件事都有现成入口，只是没接出来：
 *
 *   | 要做的事 | 引擎入口 |
 *   |---|---|
 *   | 写方块（带状态） | `BlockSerializationUtils::tryGetBlockFromNBT` + `BlockSource::setBlock` |
 *   | 写方块实体 | `BlockSource::getBlockEntity` + `BlockActor::load` |
 *   | 从 NBT 放实体 | `ActorFactory::loadActor` + `Level::addEntity` |
 *
 * # 顺带的量级变化
 *
 * 一次 setblock 命令要过命令解析、权限检查、命令分发；`BlockSource::setBlock`
 * 是一次直接调用。这不是「快一点」，是把「两百万格要分帧跑几十秒」变成
 * 「同一批格子跑一遍就完了」。SDK 侧的分帧引擎仍然要留 —— 但它现在限制的
 * 是**每 tick 的时间预算**，不再是命令分发的吞吐。
 *
 * # 为什么不顺手把 `/setblock` 那条路删掉
 *
 * 因为它还有用：玩家手写的方块规格（`//set 'wool ["color"="red"]'`）走命令
 * 解析是最省事的，而且那条路已经被验证了很久。这个文件加的是**新的**入口，
 * 不是替换 —— 旧模组一行不改照样跑。
 */
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
                    // 没有状态要覆盖：默认状态就是答案，一次解析都不用做。
                    return bs->setBlock(
                        BlockPos{x, y, z}, *def, update_flags, nullptr, bridge::blockEditContext());
                }

                // 从默认方块的序列化标签出发，**只覆盖调用方给出的那几个状态**。
                //
                // 这样做而不是让调用方自己拼整个 {name,states,version}：version
                // 必须是当前版本，而调用方没有可靠办法知道它。填错（或者不填）
                // 会让引擎把这次写入当成远古存档跑一遍升级表 —— 表现是「我明明
                // 写的是这个状态，放出来却是另一个」。
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
                // 那一格没有方块实体 —— 调用方的顺序错了（应该先放方块再填内
                // 容），或者放的方块本来就没有方块实体。报 false，别装作成功。
                if (!be) return false;

                auto parsed = CompoundTag::fromSnbt(sv(snbt));
                if (!parsed) return false;

                // 快照里的 x/y/z 是**源位置**。不改的话，某些方块实体（活塞、
                // 命令方块）会按那个坐标去找自己，结果是「内容对了，行为错
                // 了」。
                (*parsed)["x"] = x;
                (*parsed)["y"] = y;
                (*parsed)["z"] = z;

                DefaultDataLoadHelper helper{};
                be->load(*level, *parsed, helper);
                be->setChanged();
                // setChanged 只标脏。少了这一步，服务端是对的、客户端还是空箱
                // 子，直到区块重载 —— 而那时玩家早就以为复制失败了。
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

                // NewUniqueIdsDataLoadHelper：把 NBT 里的 UniqueID 映射成**新
                // 的** id。这正是 /structure load 放实体时走的东西。沿用快照里
                // 的 id 会和源实体撞号，而撞号的表现是两个实体被引擎当成同一个
                // —— 一个凭空消失、另一个行为错乱，且没有任何日志。
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

                // 老的 actor_trace_ray 只发 mPos（一个浮点命中点）。命中点正好
                // 落在方块的**面**上，所以 floor() 有一半概率落到隔壁那一格 ——
                // 任何「照着准星选方块」的功能都因此做不了。mBlock 和 mFacing
                // 一直都在 HitResult 里，只是没往外发。
                std::string out = "{type:" + snbtNum(static_cast<int>(hr.mType));
                out += ",block:[" + snbtNum(hr.mBlock.x) + "," + snbtNum(hr.mBlock.y) + ","
                    + snbtNum(hr.mBlock.z) + "]";
                out += ",facing:" + snbtNum(static_cast<int>(hr.mFacing));
                out += ",pos:[" + snbtNum(hr.mPos.x) + "," + snbtNum(hr.mPos.y) + ","
                    + snbtNum(hr.mPos.z) + "]";

                int64_t entityId = 0;
                if (hr.mType == HitResultType::Entity)
                {
                    // mEntity 是 WeakEntityRef；tryUnwrap<Actor>() 是 LL 给的安
                    // 全解引用（实体已经消失时返回空，而不是给一个悬垂指针）。
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

        // ───────────────────── 液体层（含水） ─────────────────────
        //
        // Bedrock 的「含水」是同一格上的第二个方块，不是方块状态：主层放楼
        // 梯，液体层放 water。get_block / set_block 只看主层，所以含水的方块
        // 复制过去水会消失 —— 主层完全正确，缺的是另一层。

        bool api_get_extra_block(
            int32_t dim, int32_t x, int32_t y, int32_t z, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                if (!sink) return false;
                auto* bs = bridge::blockSourceOf(dim);
                if (!bs) return false;
                auto const& block = bs->getExtraBlock(BlockPos{x, y, z});
                // 空液体层返回的是 air，如实传出去 —— 调用方据此判断「这格没有
                // 含水」。
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
