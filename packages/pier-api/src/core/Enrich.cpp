/** core/Enrich.cpp: event payload enrichment, resolving reflection pointer stubs into
 *  fields a consumer can read.
 *
 * serializeRefObj in LL emits a {_type_:"Player", _pointer_:<i64>} stub for every
 * reference field it cannot serialize. Such a stub is useless across the ABI: the
 * pointer is valid only in this process and the type name is a C++ static type.
 *
 * The test matches any Actor shape, not "Player" alone. The inheritance chain decides
 * the static type of self: PlayerEvent emits Player, MobEvent emits Mob, ActorEvent
 * emits Actor. So ActorHurtEvent and MobDieEvent carry "Actor" in the stub even when
 * the subject is a player. Matching "Player" alone would inject neither _player nor
 * dim, and a consumer reading a missing dim as 0 treats every event in a custom
 * dimension as the overworld, so land protection allows everything elsewhere and logs
 * nothing. Only addresses in the live player table are dereferenced, because a mob may
 * be destroyed within the same tick, so an Actor stub outside the table is left alone.
 */
#include "pier/api/bridge.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/deps/nbt/ListTag.h"
#include "mc/deps/nbt/StringTag.h"
#include "mc/platform/UUID.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorDamageSource.h"
#include "mc/world/actor/ActorDefinitionIdentifier.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/dimension/Dimension.h"

namespace pier::bridge
{
    namespace
    {
        /** Address table of online players. Only a pointer listed here is
         *  dereferenced. */
        std::unordered_set<uintptr_t> livePlayerAddrs()
        {
            std::unordered_set<uintptr_t> addrs;
            auto* level = levelReady();
            if (!level) return addrs;
            level->forEachPlayer([&](Player& p)
            {
                addrs.insert(reinterpret_cast<uintptr_t>(&p));
                return true;
            });
            return addrs;
        }

        struct Stub
        {
            std::string key;
            std::string type;
            uintptr_t addr;
        };

        std::vector<Stub> findStubs(CompoundTag const& data)
        {
            std::vector<Stub> out;
            for (auto const& entry : data.mTags)
            {
                auto const& value = entry.second;
                if (!value.is_object()) continue;
                auto const& obj = value.get<CompoundTag>();
                if (!obj.contains("_type_") || !obj.contains("_pointer_")) continue;

                auto const& typeVar = obj.at("_type_");
                auto const& ptrVar = obj.at("_pointer_");
                if (!typeVar.is_string() || !ptrVar.is_number()) continue;

                auto addr = static_cast<uintptr_t>(static_cast<int64_t>(ptrVar));
                // Drop anything that is plainly not a valid object address, meaning
                // near the null page or not 8-byte aligned.
                if (addr < 0x10000 || (addr & 0x7) != 0) continue;
                out.push_back({entry.first, std::string(std::string_view(typeVar)), addr});
            }
            return out;
        }

        bool isActorLike(std::string_view type)
        {
            return type == "Player" || type == "ServerPlayer" || type == "Mob" || type == "Actor";
        }

        CompoundTagVariant describeActor(Actor& a)
        {
            return CompoundTagVariant::object(
                {
                    {"type", CompoundTagVariant(a.getTypeName())},
                    {"name", CompoundTagVariant(a.getNameTag())},
                    {"isPlayer", CompoundTagVariant(a.isPlayer())},
                    {"dim", CompoundTagVariant(static_cast<int>(a.getDimensionId()))}
                }
            );
        }
    } // namespace

    std::string enrichEventData(CompoundTag const& data)
    {
        auto stubs = findStubs(data);
        if (stubs.empty()) return data.toSnbt(SnbtFormat::Minimize);

        CompoundTag copy = data;
        bool changed = false;
        bool haveDim = copy.contains("dim");

        // The live player table is built only when an Actor stub is actually present.
        // Most events have no player field at all and building it every time is pure
        // waste.
        std::unordered_set<uintptr_t> live;
        bool liveReady = false;
        auto isLivePlayer = [&](uintptr_t addr) -> bool
        {
            if (!liveReady)
            {
                live = livePlayerAddrs();
                liveReady = true;
            }
            return live.find(addr) != live.end();
        };

        // An unresolved stub must leave a trace. Contract §5.1 requires that "cannot
        // be determined" and "the answer is no" be different values. A consumer runs
        // unwrap_or(0) on a missing dim and reads it as the overworld, which is the
        // exact shape of the incident where land protection in a custom dimension was
        // bypassed.
        std::vector<std::string> unresolved;

        // A non-player Actor, as in ActorHurt or MobDie where the subject is a mob,
        // is dereferenced only when the runtime entity table lists it. Events are
        // dispatched synchronously inside the engine call stack, so at that moment it
        // is certainly alive. A pointer absent from the table is never touched. Linear
        // scan, no allocation, and only when a non-player stub appears.
        std::vector<Actor*> actors;
        bool actorsReady = false;
        auto liveActor = [&](uintptr_t addr) -> Actor*
        {
            if (!actorsReady)
            {
                if (auto* level = bridge::levelReady()) actors = level->getRuntimeActorList();
                actorsReady = true;
            }
            for (auto* a : actors)
            {
                if (reinterpret_cast<uintptr_t>(a) == addr) return a;
            }
            return nullptr;
        };

        for (auto const& stub : stubs)
        {
            if (isActorLike(stub.type))
            {
                // Only a pointer to a currently online player is dereferenced. Another
                // Actor pointer is touched only on a hit in the runtime entity table,
                // since a mob may be destroyed within this tick.
                if (!isLivePlayer(stub.addr))
                {
                    if (auto* actor = liveActor(stub.addr))
                    {
                        copy["_" + stub.key] = describeActor(*actor);
                        if (stub.key == "self" && !haveDim)
                        {
                            copy["dim"] = CompoundTagVariant(static_cast<int>(actor->getDimensionId()));
                            haveDim = true;
                        }
                        changed = true;
                    }
                    else
                    {
                        unresolved.push_back(stub.key);
                    }
                    continue;
                }
                auto* player = reinterpret_cast<Player*>(stub.addr);
                if (!player) continue;

                if (stub.key == "self")
                {
                    Vec3 const& ppos = player->getPosition();
                    copy["_player"] = CompoundTagVariant::object(
                        {
                            {"name", CompoundTagVariant(player->getRealName())},
                            {"xuid", CompoundTagVariant(player->getXuid())},
                            {"uuid", CompoundTagVariant(player->getUuid().asString())},
                            {
                                "pos", CompoundTagVariant::object(
                                    {
                                        {"x", CompoundTagVariant(static_cast<int>(std::floor(ppos.x)))},
                                        {"y", CompoundTagVariant(static_cast<int>(std::floor(ppos.y)))},
                                        {"z", CompoundTagVariant(static_cast<int>(std::floor(ppos.z)))}
                                    }
                                )
                            }
                        }
                    );
                    if (!haveDim)
                    {
                        copy["dim"] = CompoundTagVariant(static_cast<int>(player->getDimensionId()));
                        haveDim = true;
                    }
                }
                else
                {
                    copy["_" + stub.key] = describeActor(*player);
                }
                changed = true;
                continue;
            }

            if (stub.type == "ActorDefinitionIdentifier")
            {
                if (auto* id = reinterpret_cast<ActorDefinitionIdentifier*>(stub.addr))
                {
                    copy["_identifier"] = CompoundTagVariant::object(
                        {
                            {"full", CompoundTagVariant(id->mFullName.get())},
                            {"namespace", CompoundTagVariant(id->mNamespace.get())},
                            {"name", CompoundTagVariant(id->mIdentifier.get())}
                        }
                    );
                    changed = true;
                }
                continue;
            }

            if (stub.type == "ActorDamageSource")
            {
                // Reads the public member rather than getCause(), which is MCFOLD and
                // not guaranteed to be exported. mCause is
                // TypedStorage<..., ActorDamageCause> and ActorDamageCause is an enum,
                // so by the collapse rules in tools/typed-storage.py it is that enum
                // itself and writing .get() is compile error C2228.
                if (auto* src = reinterpret_cast<ActorDamageSource*>(stub.addr))
                {
                    copy["_" + stub.key] = CompoundTagVariant::object(
                        {{"cause", CompoundTagVariant(static_cast<int>(src->mCause))}}
                    );
                    changed = true;
                }
                continue;
            }

            if (stub.type == "BlockSource")
            {
                // The WorldEvent family, such as SpawningMobEvent, FireSpreadEvent and
                // BlockChangedEvent, has no player field, so dim can only come from
                // here.
                if (!haveDim)
                {
                    if (auto* bs = reinterpret_cast<BlockSource*>(stub.addr))
                    {
                        // mDimension holds a Dimension&, and TypedStorage has a
                        // collapse specialization for references, so writing .get() is
                        // compile error C2039. A named reference is taken first rather
                        // than chaining member access, because operator-> is not
                        // guaranteed when a reference is stored. The id comes from
                        // getDimensionId().value() rather than mId directly, whose
                        // TypedStorage shape is unverified and not worth one saved
                        // virtual call.
                        Dimension& dim = bs->mDimension;
                        copy["dim"] = CompoundTagVariant(dim.getDimensionId().value());
                        haveDim = true;
                        changed = true;
                    }
                }
                continue;
            }

            if (stub.type == "Block")
            {
                if (auto* b = reinterpret_cast<Block const*>(stub.addr))
                {
                    copy["_" + stub.key] = CompoundTagVariant::object(
                        {{"name", CompoundTagVariant(b->getTypeName())}}
                    );
                    changed = true;
                }
                continue;
            }

            if (stub.type == "ItemStack" || stub.type == "ItemStackBase")
            {
                if (auto* it = reinterpret_cast<ItemStack const*>(stub.addr))
                {
                    copy["_" + stub.key] = CompoundTagVariant::object(
                        {{"name", CompoundTagVariant(it->getTypeName())}}
                    );
                    changed = true;
                }
                continue;
            }
        }

        if (!unresolved.empty())
        {
            // An explicit marker, so a consumer fails closed instead of reading the
            // absence as the overworld.
            ListTag list;
            for (auto const& k : unresolved) list.add(std::make_unique<StringTag>(k));
            copy["_unresolved"] = std::move(list);
            changed = true;
        }
        return (changed ? copy : data).toSnbt(SnbtFormat::Minimize);
    }
} // namespace pier::bridge
