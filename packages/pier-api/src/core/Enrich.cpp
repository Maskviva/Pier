/** core/Enrich.cpp —— 事件载荷富化：把反射指针桩解成消费方读得懂的字段。
 *
 * LL 的 serializeRefObj 对每个不可序列化的引用字段都发一个
 * `{_type_:"Player", _pointer_:<i64>}` 桩（见 ll/api/event/EventRefObjSerializer.h）。
 * 桩对 ABI 另一侧毫无用处 —— 指针进程内有效、类型名是 C++ 静态类型 ——
 * 这里把它们换成身份字段。
 *
 * # 为什么按「Actor 形」而不是只认 "Player"
 *
 * 继承链决定了 `self` 的**静态**类型：PlayerEvent 发 Player、MobEvent 发
 * Mob、ActorEvent 发 Actor（每层都覆写 nbt["self"]）。于是
 * ActorHurtEvent / MobDieEvent 这类挂在 ActorEvent 上的事件，即使当事人就
 * 是玩家，桩上写的也是 "Actor" —— 只认 "Player" 时 `_player` 和 `dim` 一个
 * 都不会注入。消费方把缺失的 dim 当 0，自定义维度里的每个事件都被当成发生
 * 在主世界（rsw 土地保护因此在别的维度全部放行，零日志）。
 *
 * # 为什么只解引用「活玩家表」里的地址
 *
 * 生物随时可能在这一 tick 内被销毁；玩家指针则由在线表背书。不在表里的
 * Actor 桩一律不碰 —— 宁可少一个字段，不解引用悬垂指针。
 */
#include "pier/api/bridge.h"

#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/nbt/CompoundTag.h"
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

        for (auto const& stub : stubs)
        {
            if (isActorLike(stub.type))
            {
                // 只解引用当前在线玩家的指针；别的 Actor 指针不碰
                //（生物随时可能在这一 tick 内被销毁）。
                if (!isLivePlayer(stub.addr))
                {
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
                // 读公开成员而不是 getCause()：后者是 MCFOLD，不保证导出。
                //
                // `mCause` 是 `TypedStorage<..., ActorDamageCause>`，而
                // ActorDamageCause 是**枚举** —— 按 TypedStorage 的坍缩规则
                // （标量和引用都坍缩，只有类类型的值才保持包装，见
                // hooks/world/UseItemOnEvent.cpp 的推导）它就是那个枚举本身。
                // 写 `.get()` 是编译错误（C2228：左边必须有类/结构/联合）。
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
                        // `mDimension` 装的是 `Dimension&` —— TypedStorage 对
                        // 引用有特化，成员本身就是那个引用，写 `.get()` 是编译
                        // 错误（真机报 C2039：get 不是 Dimension 的成员）。
                        // 先落一个具名引用而不是链式点号：TypedStorage 装引用时
                        // 不保证有 operator->（ChunkTrace.cpp 里同一条结论）。
                        //
                        // 取 id 走 `getDimensionId().value()` 而不是直接读
                        // `mId`。原注释说「走公开成员，不调虚函数」，那个偏好
                        // 是对的，但 `Dimension::mId` 的 TypedStorage 形状我
                        // **没有验证过**，而这个写法在 ChunkTrace.cpp 里已经
                        // 随整个 pier-dimensions 编译通过。用没验证过的写法去
                        // 省一次虚调用，不划算。
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

        return (changed ? copy : data).toSnbt(SnbtFormat::Minimize);
    }
} // namespace pier::bridge
