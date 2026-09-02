/** world/GapFill.cpp: appended gap-fill slots, gated by struct_size.
 *
 * Some thirty specialized functions hang off PierApi as appended fields. Without an
 * implementation those slots are NULL by value initialization and the other side
 * crashes on the first call. The stubs here return false, -1 or 0, which the SDK
 * safety layer turns into Err("unsupported"). Whatever MC and LL express directly is
 * implemented in place; the rest stays a stub until the matching BDS API is
 * confirmed.
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
        /*  Player: equipment, cooldowns, network  */

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
                // slot is 0 main hand, 1 offhand, 2 head, 3 chest, 4 legs, 5 feet.
                // Actor::getEquippedSlot(EquipmentSlot) reads all six uniformly and
                // returns an ItemStack const&, an empty item for an empty slot, which
                // itemToSnbt renders as "{}".
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
                // getItemCooldownLeft takes a HashedString category and returns the
                // remaining ticks, where 0 means not on cooldown.
                return p->getItemCooldownLeft(HashedString{toString(item_name)});
                // -1 is the agreed failure value of this family. 0 would be read
                // as a real answer.
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
                // getNetworkStatus() returns optional<NetworkPeer::NetworkStatus>, and
                // an empty value means the peer is gone and disconnecting.
                // mCurrentPing and mAveragePing wrap chrono::milliseconds, so ->count()
                // is used.
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

        /*  Actors: relations, equipment, effects, geometry  */

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
                // Actor has no getPassengers(). getFirstPassenger() yields the first
                // rider directly, or nullptr when nobody is riding.
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
                // slot is 0 main hand, 1 offhand, 2 head, 3 chest, 4 legs, 5 feet.
                // Actor::getEquippedSlot covers all six through EquipmentSlot.
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
                // getAllEffects() returns vector<MobEffectInstance> const&. Each
                // instance offers getId(), getAmplifier() and
                // getDuration().getValue(), an optional that is empty for an infinite
                // duration.
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
                // An out-of-range index makes the engine's bitset read or write land
                // on a different data item.
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
                // Actor::traceRay(tMax, includeActor, includeBlock, blockCheckFn)
                // returns a HitResult carrying mType, one of Tile, Entity or NoHit,
                // plus mPos and mEntity. Emitting only mPos is the established shape of
                // this slot. The version carrying block coordinates and a face is
                // edit_trace_ray in Edit.cpp, where a comment explains why floor(mPos)
                // picks the wrong cell.
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
                // The same gate player_teleport uses. The target dimension must be
                // buildable through the dimension bridge with a matching id, otherwise
                // the engine throws an uncaught exception on a chunk thread and
                // fastfails.
                if (!bridge::blockSourceOf(dim)) return false;
                // Actor::clone(Vec3 const& pos, optional<DimensionType>) returns
                // optional_ref<Actor>. The clone inherits NBT state such as health,
                // equipment and name, and the caller chooses where it lands.
                auto opt = a->clone(Vec3{(float)x, (float)y, (float)z}, DimensionType{dim});
                if (!opt) return false;
                *out = opt->getOrCreateUniqueID().rawID;
                return true;
            PIER_API_GUARD_END
        }

        /*  Blocks: state reads and writes, collision shape  */

        bool api_block_get_state(
            int32_t dim, int32_t x, int32_t y, int32_t z, PierStr state_name,
            void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                auto* bs = bridge::blockSourceOf(dim);
                if (!bs || !sink) return false;
                auto const& block = bs->getBlock(BlockPos{x, y, z});
                // Looks up the BlockState by name through BlockType and reads its
                // value. getState<T> is templated on the value type, and int is the
                // common denominator here, because every state value in the BDS state
                // model is an integer, with booleans, enums and integers all collapsing
                // to int.
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
                // setState returns optional_ref<Block const>, the new permutation, and
                // is empty when the state or value is invalid. The caller must write it
                // back to the world through BlockSource::setBlock.
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
                // The new permutation has to be written back here.
                // setBlock(pos, block, updateFlags, syncMsg, changeContext) is a public
                // virtual on BlockSource. Computing the permutation and returning true
                // without writing it does not lose a feature, it reports success while
                // the world is unchanged: the caller sees Ok, the block does not move
                // and nothing is logged.
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
                // Block::getCollisionShape fills an AABB out parameter and returns true
                // when the block has a collision box. A multi-box shape needs
                // BlockSource::fetchCollisionShapes. This gap-fill slot reports the
                // primary shape only, which answers most questions about whether
                // something can pass.
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

        /*  Items: enchantments, matching, NBT  */

        bool api_item_get_enchants(PierStr item_snbt, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                auto opt = bridge::itemFromSnbt(sv(item_snbt));
                if (!opt || !sink) return false;
                // constructItemEnchantsFromUserData() builds an ItemEnchants from the
                // NBT stored on the item, and getAllEnchants() flattens the three
                // activation-type vectors into one.
                auto enchants = opt->constructItemEnchantsFromUserData();
                auto list = enchants.getAllEnchants();
                std::string out = "[";
                bool first = true;
                for (auto const& e : list)
                {
                    if (!first) out += ",";
                    first = false;
                    // mEnchantType is Enchant::Type, a uchar enum, and mLevel is int.
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
                // Assembling an ItemEnchants from loose {type,level} pairs needs either
                // a ListTag in exactly the NBT format the constructor expects, with
                // id and lvl pairs as compound entries, or the
                // EnchantUtils::applyEnchant route. Neither is straightforward through
                // the public API, so this reports unsupported rather than risking
                // corrupted item user-data. get_enchants above is read-only and safe;
                // writing waits until the enchantment pipeline is in place.
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

        /*  Level: biomes, spawn point, save data, weather, pathfinding, sleep  */

        bool api_level_get_biome(
            int32_t dim, int32_t x, int32_t y, int32_t z, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                auto* bs = bridge::blockSourceOf(dim);
                if (!bs || !sink) return false;
                // tryGetBiome returns a nullable Biome const*, while the reference from
                // getBiome is never null in a well-formed chunk. The nullable one is
                // used so an unloaded chunk reports failure instead of dereferencing
                // garbage.
                auto const* biome = bs->tryGetBiome(BlockPos{x, y, z});
                if (!biome) return false;
                // Biome has no getName(). The id string lives in the public member
                // mHash, a TypedStorage<HashedString>. It is bound to a
                // HashedString const& first and then getString() is called, because
                // going from TypedStorage to HashedString to string to string_view
                // needs an explicit hop: two implicit user-defined conversions are not
                // allowed.
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
         * Builds the key prefix of one chunk.
         *
         * `<chunkX:i32 LE><chunkZ:i32 LE>`, followed by `<dimension:i32 LE>` outside
         * the overworld. The overworld, dim 0, has no third field. Adding it there
         * makes the prefix match no key at all and the caller concludes the chunk was
         * empty to begin with.
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
         * Retired. Always returns -1.
         *
         * This slot listed the keys of a chunk and deleted them in one step, and it
         * crashed the server on real hardware, in the destructor of the C++-side
         * `std::vector<std::string>`.
         *
         * The slot cannot be removed, because the ABI is append-only and removing one
         * shifts every slot after it, so it stays here explicitly disabled and the
         * functionality lives in level_chunk_keys plus level_delete_key.
         *
         * It returns -1 and not 0, because 0 means the chunk was empty to begin with
         * and would let a caller believe the erase succeeded.
         */
        int32_t api_level_delete_chunk_keys(int32_t, int32_t, int32_t) { return -1; }

        /**
         * Lists every save key of one chunk, one callback per key.
         *
         * Nothing is accumulated here. Collecting the keys into a
         * std::vector<std::string> and deleting them afterwards crashes when that
         * vector is destroyed on return, with the inline buffer of a string treated as
         * a heap pointer in the registers. The root cause concerns std::string
         * lifetime across a DLL boundary, and the shape producing it is accumulating a
         * string container here, crossing a virtual call and destroying it on this
         * stack, so each key is handed over as found and the container lives elsewhere.
         *
         * Keys are binary and contain zero bytes. PierStr is {ptr,len} and ps(k) wraps
         * only the pointer and the length, without copying and without looking for a
         * terminator, so no string here is owned on the C++ side. */
        /**
         * The layout of a chunk key is
         *   `<x:i32 LE><z:i32 LE>[<dim:i32 LE>]<tag:u8>[<subY:u8>]`
         * The overworld has no dim field and is 9 or 10 bytes; every other dimension
         * has one and is 13 or 14.
         *
         * That is the prefix-matching trap. The 8-byte overworld prefix for (x,z) is
         * also the common prefix of the chunk keys at the same coordinates in every
         * dimension, so listing by prefix alone and deleting key by key erases the
         * matching chunks in the nether, the end and any custom dimension too. Listing
         * therefore filters by length and dim field, and deletion accepts only keys
         * matching the layout.
         */
        bool isChunkKeyFor(std::string_view key, int32_t dim, std::string_view prefix)
        {
            if (key.size() < prefix.size() || key.compare(0, prefix.size(), prefix) != 0) return false;
            size_t const expectMin = (dim == 0) ? 9 : 13;
            return key.size() == expectMin || key.size() == expectMin + 1;
        }

        /** Layout only, with no interpretation of the content. Only a length of 9,
         *  10, 13 or 14 can be a chunk key. */
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
                        // Filters out keys sharing the prefix that belong to another
                        // dimension, or whose tag byte happens to collide with a
                        // dimension number.
                        if (!isChunkKeyFor(k, dim, prefix)) return;
                        sink(ctx, ps(k));
                        ++n;
                    }
                );
                return n;
                // -1 is the agreed failure value of this family. 0 would be read
                // as a real answer.
            PIER_API_GUARD_END_VAL(-1)
        }

        /**
         * Deletes one key of the chunk category as given.
         *
         * The content of the key is not interpreted. That is what makes erasing a whole
         * chunk safe: it requires no understanding of the subchunk palette or the bit
         * packing format.
         */
        bool api_level_delete_key(PierStr key)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level || !level->hasLevelStorage() || key.len == 0) return false;
                // Only keys matching the chunk key layout are accepted. Accepting any
                // key would let a single mistaken call erase anything in the save,
                // including player_*, scoreboard, portals and
                // LevelChunkMetaDataDictionary.
                if (!looksLikeChunkKey(sv(key)))
                {
                    hostLogger().error(
                        "[api] level_delete_key refused, key length {} does not match the "
                        "chunk key layout of 9, 10, 13 or 14 bytes",
                        key.len);
                    return false;
                }
                // `deleteData` takes a `std::string const&`. The temporary lives to the
                // end of this statement and the deletion is committed synchronously
                // into the write batch.
                level->getLevelStorage().deleteData(toString(key), ::DBHelpers::Category::Chunk);
                return true;
            PIER_API_GUARD_END
        }

        /**
         * Whether the chunks covering this area are loaded.
         *
         * The signature is a center plus a radius and not two corners, because this
         * version of BlockSource::hasChunksAt has only the
         * (BlockPos const&, int, bool) overload, so the caller's box is converted.
         *
         * The radius is shrunk by one. A chunk loads as a whole, so any point inside
         * answers for the whole chunk. At the full radius a 16-wide box exactly touches
         * the first cell of the neighboring chunk, so a loaded neighbor reads as a
         * loaded chunk here and the erase never finds its moment.
         *
         * The third argument, ignoreClientChunk, is true: the question is whether the
         * server holds data here, and a client cache says nothing about it. */
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
                if (r > 0) --r; // Shrink by one so the neighbor is not touched
                if (r < 0) r = 0;
                // y is taken at ground level. A chunk loads as a full column so the
                // value of y does not change the answer, but an out-of-range y makes
                // some versions return false outright.
                BlockPos at{cx, 0, cz};
                return bs->hasChunksAt(at, r, true) ? 1 : 0;
                // -1 is the agreed failure value of this family. 0 would be read
                // as a real answer.
            PIER_API_GUARD_END_VAL(-1)
        }

        /**
         * The connection id of a player.
         *
         * It must be the same number `connId = id.getHash()` produces in PacketHooks.
         * That one is what a packet interception callback sees and this one is what a
         * lookup by name returns. If they disagree, rewriting the packets of one
         * player quietly rewrites nothing.
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
                // The fields of PlayerSleepStatus are mSleepingPlayerCount,
                // mRequiredSleepingPlayerCount and mAbleToSleep, all scalars, so
                // TypedStorage collapses them to a bare int, int and bool.
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
                return false; // Stub until the PathFinder pipeline is in place
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
