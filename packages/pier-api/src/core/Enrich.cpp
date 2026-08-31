/** core/Enrich.cpp —— 事件载荷富化：把反射指针桩解成消费方读得懂的字段。
 *
 * LL 的 serializeRefObj 对每个不可序列化的引用字段发一个
 * {_type_:"Player", _pointer_:<i64>} 桩。桩对 ABI 另一侧毫无用处：指针只在进程内
 * 有效，类型名是 C++ 静态类型。这里把它们换成身份字段。
 *
 * 判据按「Actor 形」而不是只认 "Player"。继承链决定 self 的静态类型：PlayerEvent
 * 发 Player，MobEvent 发 Mob，ActorEvent 发 Actor。于是 ActorHurtEvent、MobDieEvent
 * 这类挂在 ActorEvent 上的事件，即使当事人就是玩家，桩上写的也是 "Actor"；只认
 * "Player" 时 _player 与 dim 一个都不会注入，而消费方把缺失的 dim 当 0 之后，自定
 * 义维度里的每个事件都被当成发生在主世界，土地保护在别的维度全部放行且零日志。
 *
 * 只解引用活玩家表里的地址：生物随时可能在这一 tick 内被销毁，玩家指针则由在线表
 * 背书。不在表里的 Actor 桩一律不碰，宁可少一个字段也不解引用悬垂指针。
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
        /** 在线玩家的地址表。只有出现在这里的指针才会被解引用。 */
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
                // 明显不是合法对象地址的直接丢：空页附近、没按 8 对齐。
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

        // 只在真的有 Actor 桩时才建活玩家表：绝大多数事件根本没有玩家字段，
        // 每个事件都建一次是纯浪费。
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

        // 桩解析不出来时必须留下痕迹。契约 §5.1：「问不出来」和
        // 「答案是否」必须是不同的值 —— 缺席的 dim 会被消费方 unwrap_or(0)
        // 当成主世界，这正是自定义维度土地保护被绕过的事故原型。
        std::vector<std::string> unresolved;

        // 非玩家 Actor（ActorHurt/MobDie 等的当事人是生物）：只在运行时实体表
        // 里找得到时才解引用 —— 事件是在引擎调用栈里同步派发的，此刻它一定
        // 活着；表里没有的指针一律不碰。线性扫描、零分配，只在遇到非玩家桩
        // 时才做。
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
                // 只解引用当前在线玩家的指针；别的 Actor 指针只在运行时实体表
                // 里命中时才碰（生物随时可能在这一 tick 内被销毁）。
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
                // 读公开成员而不是 getCause()，后者是 MCFOLD，不保证导出。mCause
                // 是 TypedStorage<..., ActorDamageCause>，而 ActorDamageCause 是枚
                // 举，按坍缩规则（见 tools/typed-storage.py）它就是那个枚举本身，
                // 写 .get() 是编译错误 C2228。
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
                // WorldEvent 那一族（SpawningMobEvent / FireSpreadEvent /
                // BlockChangedEvent）没有玩家字段，dim 只能从这里来。
                if (!haveDim)
                {
                    if (auto* bs = reinterpret_cast<BlockSource*>(stub.addr))
                    {
                        // mDimension 装的是 Dimension&，TypedStorage 对引用有坍缩
                        // 特化，写 .get() 是编译错误 C2039。先落一个具名引用而不
                        // 用链式点号：装引用时不保证有 operator->。取 id 走
                        // getDimensionId().value() 而不直接读 mId，后者的
                        // TypedStorage 形状未经验证，省一次虚调用不划算。
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
            // 显式标记：消费方据此 fail-closed，而不是把缺席当成主世界。
            ListTag list;
            for (auto const& k : unresolved) list.add(std::make_unique<StringTag>(k));
            copy["_unresolved"] = std::move(list);
            changed = true;
        }
        return (changed ? copy : data).toSnbt(SnbtFormat::Minimize);
    }
} // namespace pier::bridge
