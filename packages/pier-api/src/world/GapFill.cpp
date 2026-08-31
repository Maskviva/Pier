/** world/GapFill.cpp —— 追加补漏槽（struct_size 把门）。
 *
 * 三十多个专用函数以追加字段的形式挂进 PierApi。没有实现的话这些槽位就是
 * NULL（值初始化），另一侧一调用就崩。这里的桩返回 false / -1 / 0 ——
 * SDK 安全层把它翻成 Err("unsupported")。MC/LL 侧直白的就地实现；其余保持
 * 桩，等相应的 BDS API 得到确认再补。
 */
#ifndef PIER_BUILD_CLIENT

#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/core/string/HashedString.h"
#include "mc/deps/shared_types/legacy/EquipmentSlot.h"
#include "mc/network/NetworkIdentifier.h"
#include "mc/network/NetworkPeer.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorFlags.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/effect/MobEffectInstance.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/item/enchanting/EnchantmentInstance.h"
#include "mc/world/item/enchanting/ItemEnchants.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/PlayerSleepStatus.h"
#include "mc/world/level/biome/Biome.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/BlockChangeContext.h"
#include "mc/world/level/storage/LevelStorage.h"
#include "mc/world/level/storage/db_helpers/Category.h"
#include "mc/world/phys/AABB.h"
#include "mc/world/phys/HitResult.h"

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
        /*  玩家：装备、冷却、网络  */

        bool api_player_get_carried_item(PierPlayerSel sel, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p || !sink) return false;
                sink(ctx, ps(bridge::itemToSnbt(p->getCarriedItem())));
                return true;
            PIER_API_GUARD_END
        }

        bool api_player_get_item(PierPlayerSel sel, int32_t slot, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p || !sink || slot < 0) return false;
                auto& inv = p->getInventory();
                if (slot >= inv.getContainerSize()) return false;
                sink(ctx, ps(bridge::itemToSnbt(inv.getItem(slot))));
                return true;
            PIER_API_GUARD_END
        }

        bool api_player_set_item(PierPlayerSel sel, int32_t slot, PierStr item_snbt)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p || slot < 0) return false;
                auto opt = bridge::itemFromSnbt(sv(item_snbt));
                if (!opt) return false;
                auto& inv = p->getInventory();
                if (slot >= inv.getContainerSize()) return false;
                inv.setItem(slot, std::move(*opt));
                return true;
            PIER_API_GUARD_END
        }

        bool api_player_get_equipment(PierPlayerSel sel, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p || !sink) return false;
                // slot：0=主手 1=副手 2=头 3=胸 4=腿 5=脚。
                // Actor::getEquippedSlot(EquipmentSlot) 是六个槽位的统一读法；
                // 返回 ItemStack const&（槽空时是空物品，itemToSnbt 给 "{}"）。
                namespace Equip = ::SharedTypes::Legacy;
                std::string out = "[";
                out += "{\"slot\":0,\"item\":"
                    + bridge::itemToSnbt(p->getEquippedSlot(Equip::EquipmentSlot::Mainhand)) + "}";
                out += ",{\"slot\":1,\"item\":"
                    + bridge::itemToSnbt(p->getEquippedSlot(Equip::EquipmentSlot::Offhand)) + "}";
                out += ",{\"slot\":2,\"item\":"
                    + bridge::itemToSnbt(p->getEquippedSlot(Equip::EquipmentSlot::Head)) + "}";
                out += ",{\"slot\":3,\"item\":"
                    + bridge::itemToSnbt(p->getEquippedSlot(Equip::EquipmentSlot::Torso)) + "}";
                out += ",{\"slot\":4,\"item\":"
                    + bridge::itemToSnbt(p->getEquippedSlot(Equip::EquipmentSlot::Legs)) + "}";
                out += ",{\"slot\":5,\"item\":"
                    + bridge::itemToSnbt(p->getEquippedSlot(Equip::EquipmentSlot::Feet)) + "}";
                out += "]";
                sink(ctx, ps(out));
                return true;
            PIER_API_GUARD_END
        }

        int32_t api_player_get_cooldown(PierPlayerSel sel, PierStr item_name)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p) return -1;
                // getItemCooldownLeft 吃 HashedString 类别。返回剩余 tick 数
                //（0 = 不在冷却中）。
                return p->getItemCooldownLeft(HashedString{toString(item_name)});
                // -1 是这一族约定的失败值；0 会被当成真实答案。
            PIER_API_GUARD_END_VAL(-1)
        }

        bool api_player_start_cooldown(PierPlayerSel sel, PierStr item_name, int32_t ticks)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p) return false;
                // startItemCooldown(HashedString const&, int tickDuration, bool updateClient)
                p->startItemCooldown(HashedString{toString(item_name)}, ticks, true);
                return true;
            PIER_API_GUARD_END
        }

        bool api_player_get_network_status(PierPlayerSel sel, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p || !sink) return false;
                // getNetworkStatus() 返回 optional<NetworkPeer::NetworkStatus>；
                // 缺值意味着对端已消失（正在断开）。mCurrentPing/mAveragePing
                // 是包了一层的 chrono::milliseconds → 用 ->count()。
                auto opt = p->getNetworkStatus();
                if (!opt) return false;
                auto const& ns = *opt;
                std::string snbt = "{ping:" + snbtNum(ns.mCurrentPing->count());
                snbt += ",avg_ping:" + snbtNum(ns.mAveragePing->count());
                snbt += ",packet_loss:" + snbtNum(ns.mCurrentPacketLoss);
                snbt += ",avg_packet_loss:" + snbtNum(ns.mAveragePacketLoss);
                snbt += ",max_bps:" + snbtNum(ns.mApproximateMaxBps) + "}";
                sink(ctx, ps(snbt));
                return true;
            PIER_API_GUARD_END
        }

        /*  实体：关系、装备、效果、几何  */

        bool api_actor_get_vehicle(PierActorId id, PierActorId* out)
        {
            PIER_API_GUARD_BEGIN
                Actor* a = bridge::resolveActor(id);
                if (!a || !out) return false;
                auto* v = a->getVehicle();
                if (!v) return false;
                *out = v->getOrCreateUniqueID().rawID;
                return true;
            PIER_API_GUARD_END
        }

        bool api_actor_get_first_passenger(PierActorId id, PierActorId* out)
        {
            PIER_API_GUARD_BEGIN
                Actor* a = bridge::resolveActor(id);
                if (!a || !out) return false;
                // Actor 没有 getPassengers()；getFirstPassenger() 直接给队首乘客
                //（没人骑时 nullptr）。
                Actor* p = a->getFirstPassenger();
                if (!p) return false;
                *out = p->getOrCreateUniqueID().rawID;
                return true;
            PIER_API_GUARD_END
        }

        bool api_actor_get_owner(PierActorId id, PierActorId* out)
        {
            PIER_API_GUARD_BEGIN
                Actor* a = bridge::resolveActor(id);
                if (!a || !out) return false;
                auto* owner = a->getOwner();
                if (!owner) return false;
                *out = owner->getOrCreateUniqueID().rawID;
                return true;
            PIER_API_GUARD_END
        }

        bool api_actor_get_target(PierActorId id, PierActorId* out)
        {
            PIER_API_GUARD_BEGIN
                Actor* a = bridge::resolveActor(id);
                if (!a || !out) return false;
                auto* target = a->getTarget();
                if (!target) return false;
                *out = target->getOrCreateUniqueID().rawID;
                return true;
            PIER_API_GUARD_END
        }

        bool api_actor_get_equipped_item(PierActorId id, int32_t slot, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                Actor* a = bridge::resolveActor(id);
                if (!a || !sink || slot < 0 || slot > 5) return false;
                // slot：0=主手 1=副手 2=头 3=胸 4=腿 5=脚。
                // Actor::getEquippedSlot 经 EquipmentSlot 覆盖全部六个。
                namespace Equip = ::SharedTypes::Legacy;
                auto es = static_cast<Equip::EquipmentSlot>(slot);
                sink(ctx, ps(bridge::itemToSnbt(a->getEquippedSlot(es))));
                return true;
            PIER_API_GUARD_END
        }

        bool api_actor_set_equipped_item(PierActorId id, int32_t slot, PierStr item_snbt)
        {
            PIER_API_GUARD_BEGIN
                Actor* a = bridge::resolveActor(id);
                if (!a || slot < 0 || slot > 5) return false;
                auto opt = bridge::itemFromSnbt(sv(item_snbt));
                if (!opt) return false;
                namespace Equip = ::SharedTypes::Legacy;
                auto es = static_cast<Equip::EquipmentSlot>(slot);
                a->setEquippedSlot(es, *opt);
                return true;
            PIER_API_GUARD_END
        }

        bool api_actor_get_effects(PierActorId id, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                Actor* a = bridge::resolveActor(id);
                if (!a || !sink) return false;
                // getAllEffects() 返回 vector<MobEffectInstance> const&。每个实
                // 例有 getId()、getAmplifier()、getDuration().getValue()
                //（optional —— 无限时长时为空）。
                std::string out = "[";
                bool first = true;
                for (auto const& e : a->getAllEffects())
                {
                    if (!first) out += ",";
                    first = false;
                    auto dur = e.getDuration().getValue();
                    out += "{id:" + snbtNum(e.getId());
                    out += ",amp:" + snbtNum(e.getAmplifier());
                    out += ",duration:" + (dur ? snbtNum(*dur) : std::string{"-1"});
                    out += "}";
                }
                out += "]";
                sink(ctx, ps(out));
                return true;
            PIER_API_GUARD_END
        }

        bool api_actor_get_status_flag(PierActorId id, int32_t flag_index)
        {
            PIER_API_GUARD_BEGIN
                Actor* a = bridge::resolveActor(id);
                if (!a) return false;
                // 越界下标会让引擎的位集读写落到别的数据项上。
                if (flag_index < 0 || flag_index >= static_cast<int32_t>(ActorFlags::Count)) return false;
                return a->getStatusFlag(static_cast<ActorFlags>(flag_index));
            PIER_API_GUARD_END
        }

        bool api_actor_set_status_flag(PierActorId id, int32_t flag_index, bool value)
        {
            PIER_API_GUARD_BEGIN
                Actor* a = bridge::resolveActor(id);
                if (!a) return false;
                if (flag_index < 0 || flag_index >= static_cast<int32_t>(ActorFlags::Count)) return false;
                a->setStatusFlag(static_cast<ActorFlags>(flag_index), value);
                return true;
            PIER_API_GUARD_END
        }

        bool api_actor_trace_ray(
            PierActorId id, float max_dist, bool include_actors, bool include_blocks,
            void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                Actor* a = bridge::resolveActor(id);
                if (!a || !sink) return false;
                // Actor::traceRay(tMax, includeActor, includeBlock, blockCheckFn)。
                // 返回 HitResult：mType（Tile/Entity/NoHit）、mPos、mEntity。
                // 只发 mPos 是这个老槽位的既有形状；带方块坐标与朝向的版本见
                // edit_trace_ray（Edit.cpp），那边的注释解释了为什么 floor(mPos)
                // 会选错格。
                auto hr = a->traceRay(max_dist, include_actors, include_blocks);
                std::string out = "{type:" + snbtNum(static_cast<int>(hr.mType));
                out += ",pos:[" + snbtDouble(hr.mPos.x) + "," + snbtDouble(hr.mPos.y)
                    + "," + snbtDouble(hr.mPos.z) + "]";
                out += "}";
                sink(ctx, ps(out));
                return true;
            PIER_API_GUARD_END
        }

        bool api_actor_distance_to(PierActorId id, PierActorId other, double* out)
        {
            PIER_API_GUARD_BEGIN
                Actor* a = bridge::resolveActor(id);
                Actor* b = bridge::resolveActor(other);
                if (!a || !b || !out) return false;
                auto pa = a->getPosition();
                auto pb = b->getPosition();
                float dx = pa.x - pb.x, dy = pa.y - pb.y, dz = pa.z - pb.z;
                *out = std::sqrt(static_cast<double>(dx * dx + dy * dy + dz * dz));
                return true;
            PIER_API_GUARD_END
        }

        bool api_actor_get_aabb(PierActorId id, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                Actor* a = bridge::resolveActor(id);
                if (!a || !sink) return false;
                auto aabb = a->getAABB();
                std::string snbt = "{min:[" + snbtDouble(aabb.min.x) + "," + snbtDouble(aabb.min.y) + ","
                    + snbtDouble(aabb.min.z) + "],max:[" + snbtDouble(aabb.max.x) + ","
                    + snbtDouble(aabb.max.y) + "," + snbtDouble(aabb.max.z) + "]}";
                sink(ctx, ps(snbt));
                return true;
            PIER_API_GUARD_END
        }

        bool api_actor_clone(PierActorId id, int32_t dim, double x, double y, double z, PierActorId* out)
        {
            PIER_API_GUARD_BEGIN
                Actor* a = bridge::resolveActor(id);
                if (!a || !out) return false;
                // 与 player_teleport 同一道闸 —— 目标维度必须能经维度桥建出
                // 且 id 一致，否则引擎会在区块线程抛未捕获异常直接 fastfail。
                if (!bridge::blockSourceOf(dim)) return false;
                // Actor::clone(Vec3 const& pos, optional<DimensionType>) 返回
                // optional_ref<Actor>。克隆体继承 NBT 状态（血量、装备、名字
                // 等）；落点由调用方定。
                auto opt = a->clone(Vec3{(float)x, (float)y, (float)z}, DimensionType{dim});
                if (!opt) return false;
                *out = opt->getOrCreateUniqueID().rawID;
                return true;
            PIER_API_GUARD_END
        }

        /*  方块：状态读写、碰撞形状  */

        bool api_block_get_state(
            int32_t dim, int32_t x, int32_t y, int32_t z, PierStr state_name,
            void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                auto* bs = bridge::blockSourceOf(dim);
                if (!bs || !sink) return false;
                auto const& block = bs->getBlock(BlockPos{x, y, z});
                // 经 BlockType 按名字查 BlockState，再读它的值。getState<T> 按
                // 值类型做模板；这里用 int 当公分母 —— BDS 的状态模型里状态值
                // 都是整数（布尔、枚举、整数全坍缩到 int）。
                auto const* state =
                    block.getBlockType().getBlockState(HashedString{toString(state_name)});
                if (!state) return false;
                auto v = block.getState<int>(*state);
                if (!v) return false;
                sink(ctx, ps(snbtNum(*v)));
                return true;
            PIER_API_GUARD_END
        }

        bool api_block_set_state(
            int32_t dim, int32_t x, int32_t y, int32_t z, PierStr state_name, PierStr value)
        {
            PIER_API_GUARD_BEGIN
                auto* bs = bridge::blockSourceOf(dim);
                if (!bs) return false;
                auto const& block = bs->getBlock(BlockPos{x, y, z});
                // setState 返回 optional_ref<Block const> —— 新的 permutation
                //（状态/值不合法时为空）。必须由调用侧经 BlockSource::setBlock
                // 写回世界。
                auto const* state =
                    block.getBlockType().getBlockState(HashedString{toString(state_name)});
                if (!state) return false;
                int v;
                try
                {
                    v = std::stoi(toString(value));
                }
                catch (...)
                {
                    return false;
                }
                auto opt = block.setState(*state, v);
                if (!opt) return false;
                // 这里以前是个假成功：算完新的 permutation 直接 return true，理由
                // 写的是「BlockSource 没有公开的 setBlock(Block) 重载」。它有 ——
                // setBlock(pos, block, updateFlags, syncMsg, changeContext) 是公开虚
                // 函数。后果不是少一个功能，是报告成功但世界没变：调用方看到 Ok，
                // 方块纹丝不动，且没有任何日志。
                return bs->setBlock(
                    BlockPos{x, y, z}, *opt, 3, nullptr, BlockChangeContext::commandsChange());
            PIER_API_GUARD_END
        }

        bool api_block_get_collision_shape(
            int32_t dim, int32_t x, int32_t y, int32_t z, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                auto* bs = bridge::blockSourceOf(dim);
                if (!bs || !sink) return false;
                BlockPos pos{x, y, z};
                auto const& block = bs->getBlock(pos);
                // Block::getCollisionShape 往出参填一个 AABB，方块有碰撞箱时返回
                // true。多盒形状要走 BlockSource::fetchCollisionShapes；这个补漏
                // 槽只报主形状（对多数「这里能不能走」的问题够用）。
                AABB aabb;
                bool has = block.getCollisionShape(aabb, *bs, pos, nullptr);
                if (!has)
                {
                    sink(ctx, ps(std::string_view{"[]"}));
                    return true;
                }
                std::string out = "[{min:[" + snbtDouble(aabb.min.x) + "," + snbtDouble(aabb.min.y)
                    + "," + snbtDouble(aabb.min.z) + "],max:[" + snbtDouble(aabb.max.x) + ","
                    + snbtDouble(aabb.max.y) + "," + snbtDouble(aabb.max.z) + "]}]";
                sink(ctx, ps(out));
                return true;
            PIER_API_GUARD_END
        }

        /*  物品：附魔、匹配、NBT  */

        bool api_item_get_enchants(PierStr item_snbt, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                auto opt = bridge::itemFromSnbt(sv(item_snbt));
                if (!opt || !sink) return false;
                // constructItemEnchantsFromUserData() 从物品存下的 NBT 建一个
                // ItemEnchants；getAllEnchants() 把三个激活类型向量摊平成一个。
                auto enchants = opt->constructItemEnchantsFromUserData();
                auto list = enchants.getAllEnchants();
                std::string out = "[";
                bool first = true;
                for (auto const& e : list)
                {
                    if (!first) out += ",";
                    first = false;
                    // mEnchantType 是 Enchant::Type（uchar 枚举）；mLevel 是 int。
                    out += "{type:" + snbtNum(static_cast<int>(static_cast<uchar>(e.mEnchantType)));
                    out += ",level:" + snbtNum(e.mLevel) + "}";
                }
                out += "]";
                sink(ctx, ps(out));
                return true;
            PIER_API_GUARD_END
        }

        bool api_item_set_enchants(PierStr item_snbt, PierStr enchants_snbt, void* ctx, PierStrSink out)
        {
            PIER_API_GUARD_BEGIN
                auto opt = bridge::itemFromSnbt(sv(item_snbt));
                if (!opt || !out) return false;
                // 从零散的 {type,level} 对拼一个 ItemEnchants 需要：要么 (a) 一
                // 个恰好符合构造函数期望的 NBT 格式的 ListTag（id+lvl 对作为复
                // 合条目），要么 (b) EnchantUtils::applyEnchant 那条路 —— 从公
                // 开 API 的角度都不直白。眼下报「不支持」，别冒损坏物品
                // user-data 的险。上面的 get_enchants 只读、安全；写入等附魔管
                // 线接好再开。
                (void)enchants_snbt;
                return false;
            PIER_API_GUARD_END
        }

        bool api_item_matches(PierStr a, PierStr b)
        {
            PIER_API_GUARD_BEGIN
                auto oa = bridge::itemFromSnbt(sv(a));
                auto ob = bridge::itemFromSnbt(sv(b));
                if (!oa || !ob) return false;
                return oa->matches(*ob);
            PIER_API_GUARD_END
        }

        bool api_item_get_user_data(PierStr item_snbt, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                auto opt = bridge::itemFromSnbt(sv(item_snbt));
                if (!opt || !sink) return false;
                auto* ud = opt->getUserData();
                if (!ud)
                {
                    sink(ctx, ps(std::string_view{"{}"}));
                    return true;
                }
                sink(ctx, ps(ud->toSnbt(SnbtFormat::Minimize)));
                return true;
            PIER_API_GUARD_END
        }

        /*  Level：群系、出生点、存档、天气、寻路、睡眠  */

        bool api_level_get_biome(
            int32_t dim, int32_t x, int32_t y, int32_t z, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                auto* bs = bridge::blockSourceOf(dim);
                if (!bs || !sink) return false;
                // tryGetBiome 返回 Biome const*（可空）；getBiome 返回的引用在
                // 良构区块里永不为空。用可空的那个，让未加载的区块报失败而不是
                // 解引用垃圾。
                auto const* biome = bs->tryGetBiome(BlockPos{x, y, z});
                if (!biome) return false;
                // Biome 没有 getName()；id 字符串住在公开成员 mHash 里
                //（TypedStorage<HashedString>）。先绑到 HashedString const&，再
                // 调 getString()（TypedStorage → HashedString → string →
                // string_view 要显式中转一跳；两次隐式 UDC 不被允许）。
                ::HashedString const& hash = biome->mHash;
                sink(ctx, ps(hash.getString()));
                return true;
            PIER_API_GUARD_END
        }

        bool api_level_get_default_spawn(int32_t* x, int32_t* y, int32_t* z)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level || !x || !y || !z) return false;
                auto pos = level->getDefaultSpawn();
                *x = pos.x;
                *y = pos.y;
                *z = pos.z;
                return true;
            PIER_API_GUARD_END
        }

        bool api_level_set_default_spawn(int32_t x, int32_t y, int32_t z)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level) return false;
                level->setDefaultSpawn(BlockPos{x, y, z});
                return true;
            PIER_API_GUARD_END
        }

        bool api_level_save()
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level) return false;
                level->save();
                return true;
            PIER_API_GUARD_END
        }

        /**
         * 拼一个区块的键前缀。
         *
         * `<chunkX:i32 LE><chunkZ:i32 LE>`，非主世界再跟 `<dimension:i32 LE>`。
         * 主世界（dim 0）没有那第三段 —— 加上的话前缀匹配不到任何键，
         * 调用方会以为「这个区块本来就是空的」。
         */
        std::string chunkKeyPrefix(int32_t dim, int32_t chunk_x, int32_t chunk_z)
        {
            std::string out;
            auto put_le = [&out](int32_t v)
            {
                uint32_t u = static_cast<uint32_t>(v);
                out.push_back(static_cast<char>(u & 0xFF));
                out.push_back(static_cast<char>((u >> 8) & 0xFF));
                out.push_back(static_cast<char>((u >> 16) & 0xFF));
                out.push_back(static_cast<char>((u >> 24) & 0xFF));
            };
            put_le(chunk_x);
            put_le(chunk_z);
            if (dim != 0) put_le(dim);
            return out;
        }

        /**
         * ⚠ 退役。 一律返回 -1。
         *
         * 这一格原来是「列出这个区块的键并全删」一步做完，而它**在真机上把服
         * 务器打崩了** —— 崩在 C++ 侧那个 `std::vector<std::string>` 销毁的时
         * 候。
         *
         * 槽位不能删（ABI 只能追加，删了后面每一格都会错位），所以它留在这儿
         * 明确失效，功能搬到 level_chunk_keys + level_delete_key。
         *
         * 返回 -1 而不是 0：0 是「这个区块本来就是空的」，会让调用方以为抹成
         * 功了。
         */
        int32_t api_level_delete_chunk_keys(int32_t, int32_t, int32_t) { return -1; }

        /**
         * 列出一个区块的全部存档键，一个键一次回调。
         *
         * 这里什么都不攒。在 C++ 侧把键收进 std::vector<std::string> 再逐个删会在函
         * 数返回、销毁那个 vector 时崩，寄存器里看得到字符串的内联缓冲被当成了堆指
         * 针。根因未定位（跨 DLL 的 std::string 生命周期，没有调试器查不出来），但
         * 那一类问题的来源是「在 C++ 侧攒一个字符串容器、跨一次虚调用、再在自己的
         * 栈上销毁它」，所以拿到一个就交出去，容器活在另一侧。
         *
         * 键是二进制的，含 0 字节。PierStr 是 {ptr,len}，ps(k) 只包指针和长度，不拷
         * 贝也不看 0 结尾；这条流水线上没有任何字符串在 C++ 侧被拥有。它只在这次回
         * 调里有效，另一侧负责拷走。
         */
        /**
         * 区块键的布局是
         *   `<x:i32 LE><z:i32 LE>[<dim:i32 LE>]<tag:u8>[<subY:u8>]`
         * 主世界没有 dim 段（长 9 或 10），其余维度有（长 13 或 14）。
         *
         * 这直接决定了前缀匹配的陷阱：主世界 (x,z) 的 8 字节前缀是所有维度
         * 同坐标区块键的公共前缀 —— 只按前缀列出再逐键删，会把下界/末地/自定义
         * 维度同坐标的区块一起抹掉。所以列表结果必须按长度 + 维度段过滤，删除
         * 也只接受符合布局的键。
         */
        bool isChunkKeyFor(std::string_view key, int32_t dim, std::string_view prefix)
        {
            if (key.size() < prefix.size() || key.compare(0, prefix.size(), prefix) != 0) return false;
            size_t const expectMin = (dim == 0) ? 9 : 13;
            return key.size() == expectMin || key.size() == expectMin + 1;
        }

        /** 只看布局（不解释内容）：长度 9/10 或 13/14 才可能是区块键。 */
        bool looksLikeChunkKey(std::string_view key)
        {
            return key.size() == 9 || key.size() == 10 || key.size() == 13 || key.size() == 14;
        }

        int32_t api_level_chunk_keys(
            int32_t dim, int32_t chunk_x, int32_t chunk_z, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level || !level->hasLevelStorage() || !sink) return -1;
                auto& storage = level->getLevelStorage();
                std::string const prefix = chunkKeyPrefix(dim, chunk_x, chunk_z);

                int32_t n = 0;
                storage.forEachKeyWithPrefix(
                    prefix, ::DBHelpers::Category::Chunk,
                    [ctx, sink, &n, dim, &prefix](std::string_view k, std::string_view)
                    {
                        // 过滤掉同前缀但属于其他维度（或 tag 字节恰好撞上维度号）的键。
                        if (!isChunkKeyFor(k, dim, prefix)) return;
                        sink(ctx, ps(k));
                        ++n;
                    }
                );
                return n;
                // -1 是这一族约定的失败值；0 会被当成真实答案。
            PIER_API_GUARD_END_VAL(-1)
        }

        /**
         * 原样删掉一个区块类别的键。
         *
         * 不解释键的内容 —— 传什么删什么，这正是抹整块之所以安全的原因：
         * 不需要懂子区块的调色板和位打包格式。
         */
        bool api_level_delete_key(PierStr key)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level || !level->hasLevelStorage() || key.len == 0) return false;
                // 这个槽以前「传什么删什么」—— 存档里任何键（player_*、
                // scoreboard、portals、LevelChunkMetaDataDictionary…）都能被一次
                // 误调用抹掉。只接受符合区块键布局的键。
                if (!looksLikeChunkKey(sv(key)))
                {
                    hostLogger().error(
                        "level_delete_key：键长 {} 不符合区块键布局（9/10/13/14 字节），拒绝删除",
                        key.len);
                    return false;
                }
                // `deleteData` 收 `std::string const&`。这个临时对象活到本语句
                // 结束，而删除是同步提交进写批的 —— 上一版的问题不在这里，在那
                // 个 vector。
                level->getLevelStorage().deleteData(toString(key), ::DBHelpers::Category::Chunk);
                return true;
            PIER_API_GUARD_END
        }

        /**
         * 这一片的区块加载着吗。
         *
         * 签名是「中心 + 半径」而不是两个角：BlockSource::hasChunksAt 这个版本只有
         * (BlockPos const&, int, bool) 一个重载，传两个 BlockPos 编译不过，所以把调
         * 用方给的方框换算成中心点加半径。
         *
         * 半径要往里缩一格。区块整块加载，方框内部任何一点都能代表整块的答案；按原
         * 样的半径查时，一个 16 宽的方框会正好碰到相邻区块的第一格，于是「邻居加载
         * 着」被读成「我这块加载着」，抹除永远等不到时机。
         *
         * 第三个参数 ignoreClientChunk 传 true：问的是服务端有没有这块地的数据，客
         * 户端缓存与「卸载时会不会把键写回去」无关。
         */
        int32_t api_level_chunks_loaded(
            int32_t dim, int32_t min_x, int32_t min_z, int32_t max_x, int32_t max_z)
        {
            PIER_API_GUARD_BEGIN
                auto* bs = bridge::blockSourceOf(dim);
                if (!bs) return -1;
                int32_t cx = (min_x + max_x) / 2;
                int32_t cz = (min_z + max_z) / 2;
                int32_t half_x = (max_x - min_x) / 2;
                int32_t half_z = (max_z - min_z) / 2;
                int32_t r = half_x < half_z ? half_x : half_z;
                if (r > 0) --r; // 往里缩一格，别碰到邻居
                if (r < 0) r = 0;
                // y 取地面高度：区块是整列加载的，y 取多少不影响答案，但一个越
                // 界的 y 会让某些版本直接返回 false。
                BlockPos at{cx, 0, cz};
                return bs->hasChunksAt(at, r, true) ? 1 : 0;
                // -1 是这一族约定的失败值；0 会被当成真实答案。
            PIER_API_GUARD_END_VAL(-1)
        }

        /**
         * 玩家的连接号。
         *
         * 必须和 PacketHooks 里 `connId = id.getHash()` 算的是同一个数 ——
         * 那边是拦包回调看到的号，这边是按名字问出来的号，对不上的话
         * 「改写这个人的包」会安静地一个包都改不到。
         */
        uint64_t api_player_conn_id(PierPlayerSel who)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(who);
                if (!p) return 0;
                return static_cast<uint64_t>(p->getNetworkIdentifier().getHash());
            PIER_API_GUARD_END
        }

        bool api_level_get_sleep_status(void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level || !sink) return false;
                // PlayerSleepStatus 的字段：mSleepingPlayerCount、
                // mRequiredSleepingPlayerCount、mAbleToSleep（全是标量 ——
                // TypedStorage 坍缩成裸 int/int/bool）。
                auto ss = level->getSleepStatus();
                std::string snbt = "{sleeping:" + snbtNum(ss.mSleepingPlayerCount);
                snbt += ",required:" + snbtNum(ss.mRequiredSleepingPlayerCount);
                snbt += ",able_to_sleep:" + snbtNum(ss.mAbleToSleep ? 1 : 0) + "}";
                sink(ctx, ps(snbt));
                return true;
            PIER_API_GUARD_END
        }

        bool api_level_update_weather(
            float rain_level, int32_t rain_time, float lightning_level, int32_t lightning_time)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level) return false;
                level->updateWeather(rain_level, rain_time, lightning_level, lightning_time);
                return true;
            PIER_API_GUARD_END
        }

        bool api_level_find_path(
            PierActorId id, int32_t x, int32_t y, int32_t z, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                (void)id;
                (void)x;
                (void)y;
                (void)z;
                (void)ctx;
                (void)sink;
                return false; // 桩：等 PathFinder 管线接好
            PIER_API_GUARD_END
        }

        void fill(PierApi& api)
        {
            api.player_get_carried_item = &api_player_get_carried_item;
            api.player_get_item = &api_player_get_item;
            api.player_set_item = &api_player_set_item;
            api.player_get_equipment = &api_player_get_equipment;
            api.player_get_cooldown = &api_player_get_cooldown;
            api.player_start_cooldown = &api_player_start_cooldown;
            api.player_get_network_status = &api_player_get_network_status;
            api.actor_get_vehicle = &api_actor_get_vehicle;
            api.actor_get_first_passenger = &api_actor_get_first_passenger;
            api.actor_get_owner = &api_actor_get_owner;
            api.actor_get_target = &api_actor_get_target;
            api.actor_get_equipped_item = &api_actor_get_equipped_item;
            api.actor_set_equipped_item = &api_actor_set_equipped_item;
            api.actor_get_effects = &api_actor_get_effects;
            api.actor_get_status_flag = &api_actor_get_status_flag;
            api.actor_set_status_flag = &api_actor_set_status_flag;
            api.actor_trace_ray = &api_actor_trace_ray;
            api.actor_distance_to = &api_actor_distance_to;
            api.actor_get_aabb = &api_actor_get_aabb;
            api.actor_clone = &api_actor_clone;
            api.block_get_state = &api_block_get_state;
            api.block_set_state = &api_block_set_state;
            api.block_get_collision_shape = &api_block_get_collision_shape;
            api.item_get_enchants = &api_item_get_enchants;
            api.item_set_enchants = &api_item_set_enchants;
            api.item_matches = &api_item_matches;
            api.item_get_user_data = &api_item_get_user_data;
            api.level_get_biome = &api_level_get_biome;
            api.level_get_default_spawn = &api_level_get_default_spawn;
            api.level_set_default_spawn = &api_level_set_default_spawn;
            api.level_save = &api_level_save;
            api.level_delete_chunk_keys = &api_level_delete_chunk_keys;
            api.level_chunk_keys = &api_level_chunk_keys;
            api.level_delete_key = &api_level_delete_key;
            api.level_chunks_loaded = &api_level_chunks_loaded;
            api.player_conn_id = &api_player_conn_id;
            api.level_get_sleep_status = &api_level_get_sleep_status;
            api.level_update_weather = &api_level_update_weather;
            api.level_find_path = &api_level_find_path;
        }

        spi::SlotPackReg reg{{"gap-fill", &fill}};
    } // namespace
} // namespace pier::api_impl

#endif // !PIER_BUILD_CLIENT
