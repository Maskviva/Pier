/** actors/Actors.cpp —— 实体枚举、快照、属性、动作与生成。
 *  实体句柄就是 ActorUniqueID，每次调用经 Level::fetchEntity 重新解析。 */
#include <string>

#include "mc/deps/core/math/Vec2.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/legacy/ActorUniqueID.h"
#include "mc/util/VariantParameterList.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorDefinitionIdentifier.h"
#include "mc/world/actor/Mob.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/effect/MobEffect.h"
#include "mc/world/effect/MobEffectInstance.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/Spawner.h"

#include "sdk/abi.h"

#include "pier/api/bridge.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        void api_list_actors(int32_t dim, void* ctx, PierActorSink sink)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level || !sink) return;
                for (auto* actor : level->getRuntimeActorList())
                {
                    if (!actor) continue;
                    if (dim >= 0 && static_cast<int>(actor->getDimensionId()) != dim) continue;
                    sink(ctx, actor->getOrCreateUniqueID().rawID, ps(actor->getTypeName()));
                }
            PIER_API_GUARD_END_VOID
        }

        bool api_actor_snapshot(PierActorId id, void* ctx, PierStrSink snbtSink)
        {
            PIER_API_GUARD_BEGIN
                Actor* actor = bridge::resolveActor(id);
                if (!actor || !snbtSink) return false;
                CompoundTag tag;
                if (!actor->save(tag)) return false;
                snbtSink(ctx, ps(tag.toSnbt(SnbtFormat::Minimize)));
                return true;
            PIER_API_GUARD_END
        }

        bool api_actor_get_num(PierActorId id, int32_t prop, double* out)
        {
            PIER_API_GUARD_BEGIN
                Actor* actor = bridge::resolveActor(id);
                if (!actor || !out) return false;
                switch (prop)
                {
                case PIER_APROP_POS_X:
                    *out = actor->getPosition().x;
                    return true;
                case PIER_APROP_POS_Y:
                    *out = actor->getPosition().y;
                    return true;
                case PIER_APROP_POS_Z:
                    *out = actor->getPosition().z;
                    return true;
                case PIER_APROP_ROT_PITCH:
                    *out = actor->getRotation().x;
                    return true;
                case PIER_APROP_ROT_YAW:
                    *out = actor->getRotation().y;
                    return true;
                case PIER_APROP_DIMENSION:
                    *out = static_cast<double>(static_cast<int>(actor->getDimensionId()));
                    return true;
                case PIER_APROP_HEALTH:
                    *out = static_cast<double>(actor->getHealth());
                    return true;
                case PIER_APROP_MAX_HEALTH:
                    *out = static_cast<double>(actor->getMaxHealth());
                    return true;
                case PIER_APROP_IS_ALIVE:
                    *out = actor->isAlive() ? 1.0 : 0.0;
                    return true;
                case PIER_APROP_IS_ON_GROUND:
                    *out = actor->isOnGround() ? 1.0 : 0.0;
                    return true;
                case PIER_APROP_IS_IN_WATER:
                    *out = actor->isInWater() ? 1.0 : 0.0;
                    return true;
                case PIER_APROP_IS_IN_LAVA:
                    *out = actor->isInLava() ? 1.0 : 0.0;
                    return true;
                case PIER_APROP_IS_ON_FIRE:
                    *out = actor->isOnFire() ? 1.0 : 0.0;
                    return true;
                case PIER_APROP_IS_INVISIBLE:
                    *out = actor->isInvisible() ? 1.0 : 0.0;
                    return true;
                case PIER_APROP_IS_SNEAKING:
                    *out = actor->isSneaking() ? 1.0 : 0.0;
                    return true;
                case PIER_APROP_IS_BABY:
                    *out = actor->isBaby() ? 1.0 : 0.0;
                    return true;
                case PIER_APROP_IS_RIDING:
                    *out = actor->isRiding() ? 1.0 : 0.0;
                    return true;
                case PIER_APROP_IS_TAME:
                    *out = actor->isTame() ? 1.0 : 0.0;
                    return true;
                case PIER_APROP_SPEED:
                    *out = static_cast<double>(actor->getSpeedInMetersPerSecond());
                    return true;
                /*  追加：实体补漏  */
                case PIER_APROP_VIEW_X:
                    *out = actor->getViewVector().x;
                    return true;
                case PIER_APROP_VIEW_Y:
                    *out = actor->getViewVector().y;
                    return true;
                case PIER_APROP_VIEW_Z:
                    *out = actor->getViewVector().z;
                    return true;
                case PIER_APROP_VEL_X:
                    *out = actor->getVelocity().x;
                    return true;
                case PIER_APROP_VEL_Y:
                    *out = actor->getVelocity().y;
                    return true;
                case PIER_APROP_VEL_Z:
                    *out = actor->getVelocity().z;
                    return true;
                case PIER_APROP_HEAD_X:
                    *out = actor->getHeadPos().x;
                    return true;
                case PIER_APROP_HEAD_Y:
                    *out = actor->getHeadPos().y;
                    return true;
                case PIER_APROP_HEAD_Z:
                    *out = actor->getHeadPos().z;
                    return true;
                case PIER_APROP_FEET_X:
                    *out = actor->getFeetPos().x;
                    return true;
                case PIER_APROP_FEET_Y:
                    *out = actor->getFeetPos().y;
                    return true;
                case PIER_APROP_FEET_Z:
                    *out = actor->getFeetPos().z;
                    return true;
                case PIER_APROP_FALL_DISTANCE:
                    *out = static_cast<double>(actor->getFallDistance());
                    return true;
                case PIER_APROP_IS_PERSISTENT:
                    *out = actor->isPersistent() ? 1.0 : 0.0;
                    return true;
                case PIER_APROP_IS_LEASHED:
                    *out = actor->isLeashed() ? 1.0 : 0.0;
                    return true;
                case PIER_APROP_IS_INVULNERABLE:
                    // Actor 只有 isInvulnerableTo(ActorDamageSource const&)，没有
                    // 无参形式，ActorFlags::Invulnerable 也不存在。报「不支持」。
                    return false;
                case PIER_APROP_VARIANT:
                    *out = static_cast<double>(actor->getVariant());
                    return true;
                case PIER_APROP_MARK_VARIANT:
                    *out = static_cast<double>(actor->getMarkVariant());
                    return true;
                case PIER_APROP_SCALE:
                    // getScaleFactor(float) 在 #ifdef LL_PLAT_C 后面 —— 不可用。
                    return false;
                case PIER_APROP_BRIGHTNESS:
                    *out = static_cast<double>(actor->getBrightness());
                    return true;
                case PIER_APROP_RADIUS:
                    *out = static_cast<double>(actor->getRadius());
                    return true;
                case PIER_APROP_HAS_TOTEM:
                    *out = actor->hasTotemEquipped() ? 1.0 : 0.0;
                    return true;
                case PIER_APROP_IS_IN_RAIN:
                    *out = actor->isInRain() ? 1.0 : 0.0;
                    return true;
                case PIER_APROP_IS_IN_SNOW:
                    *out = actor->isInSnow() ? 1.0 : 0.0;
                    return true;
                case PIER_APROP_IS_IN_THUNDERSTORM:
                    *out = actor->isInThunderstorm() ? 1.0 : 0.0;
                    return true;
                case PIER_APROP_IS_FROZEN:
                    // Actor 上没有 isFrozen()；ActorFlags::Frozen 也不存在。
                    // isImmobile() 覆盖的不可动原因太宽，用它会假阳性，所以
                    // 不用。报「不支持」。
                    return false;
                case PIER_APROP_IS_IN_LOVE:
                    *out = actor->isInLove() ? 1.0 : 0.0;
                    return true;
                case PIER_APROP_DEATH_TIME:
                    *out = static_cast<double>(actor->getDeathTime());
                    return true;
                case PIER_APROP_HAS_PASSENGER:
                    *out = actor->hasPassenger() ? 1.0 : 0.0;
                    return true;
                default:
                    return false;
                }
            PIER_API_GUARD_END
        }

        bool api_actor_get_str(PierActorId id, int32_t prop, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                Actor* actor = bridge::resolveActor(id);
                if (!actor || !sink) return false;
                switch (prop)
                {
                case PIER_ASTR_TYPE_NAME:
                    sink(ctx, ps(actor->getTypeName()));
                    return true;
                case PIER_ASTR_NAME_TAG:
                    sink(ctx, ps(actor->getNameTag()));
                    return true;
                /*  追加  */
                case PIER_ASTR_SCORE_TAG:
                    // getScoreTag() 在 #ifdef LL_PLAT_C 后面 —— 服务端拿不到。
                    // setScoreTag() 存在，但 getter 是客户端专属。
                    return false;
                case PIER_ASTR_FILTERED_NAME:
                    // getFilteredNameTag() 同样在 LL_PLAT_C 后面；直接读公开成员
                    // mFilteredNameTag（TypedStorage<string>）。先绑到
                    // std::string const&，让 string_view 的构造看到一个真正的
                    // std::string —— TypedStorage → string → string_view 要显式
                    // 中转一跳（两次隐式 UDC 不被允许）。
                    {
                        std::string const& name = actor->mFilteredNameTag;
                        sink(ctx, ps(name));
                        return true;
                    }
                default:
                    return false;
                }
            PIER_API_GUARD_END
        }

        bool api_actor_action(
            PierActorId id,
            int32_t action,
            PierStr sarg,
            double a,
            double b,
            double c,
            void* ctx,
            PierStrSink out)
        {
            PIER_API_GUARD_BEGIN
                Actor* actor = bridge::resolveActor(id);
                if (!actor) return false;
                switch (action)
                {
                case PIER_AACT_KILL:
                    actor->kill();
                    return true;
                case PIER_AACT_DESPAWN:
                    actor->despawn();
                    return true;
                case PIER_AACT_HEAL:
                    actor->heal(static_cast<int>(a));
                    return true;
                case PIER_AACT_SET_ON_FIRE:
                    actor->setOnFire(static_cast<int>(a));
                    return true;
                case PIER_AACT_TELEPORT:
                {
                    std::string dimStr = toString(sarg);
                    int dim = static_cast<int>(actor->getDimensionId());
                    if (!dimStr.empty())
                    {
                        try
                        {
                            dim = std::stoi(dimStr);
                        }
                        catch (...)
                        {
                            return false;
                        }
                    }
                    // 与 player_teleport 同一道闸。维度桥必须能建出实例且
                    // 引擎自报 id 与请求一致，否则区块线程抛未捕获异常 → fastfail。
                    if (!bridge::blockSourceOf(dim)) return false;
                    // teleport(pos, dim, rotation) —— 保留实体当前朝向。
                    actor->teleport(
                        Vec3{(float)a, (float)b, (float)c}, DimensionType{dim}, actor->getRotation());
                    return true;
                }
                case PIER_AACT_SET_NAME_TAG:
                    actor->setNameTag(toString(sarg));
                    return true;
                case PIER_AACT_ADD_TAG:
                {
                    bool ok = actor->addTag(toString(sarg));
                    if (out) out(ctx, ps(std::string_view{ok ? "1" : "0"}));
                    return true;
                }
                case PIER_AACT_REMOVE_TAG:
                {
                    bool ok = actor->removeTag(toString(sarg));
                    if (out) out(ctx, ps(std::string_view{ok ? "1" : "0"}));
                    return true;
                }
                case PIER_AACT_HAS_TAG:
                {
                    bool has = actor->hasTag(toString(sarg));
                    if (out) out(ctx, ps(std::string_view{has ? "1" : "0"}));
                    return true;
                }
                case PIER_AACT_ADD_EFFECT:
                {
                    auto* effect = MobEffect::getByName(toString(sarg));
                    if (!effect) return false;
                    MobEffectInstance inst{effect->getId()};
                    inst.mDuration.get().mValue = static_cast<int>(a);
                    inst.mAmplifier = static_cast<int>(b);
                    inst.mEffectVisible = (c != 0.0);
                    actor->addEffect(inst);
                    return true;
                }
                case PIER_AACT_REMOVE_EFFECT:
                {
                    auto* effect = MobEffect::getByName(toString(sarg));
                    if (!effect) return false;
                    actor->removeEffect(static_cast<int>(effect->getId()));
                    return true;
                }
                case PIER_AACT_CLEAR_EFFECTS:
                    actor->removeAllEffects();
                    return true;
                case PIER_AACT_HURT:
                {
                    // 走 Actor::hurtByCause：它接受一个 ActorDamageCause，引擎侧
                    // 的伤害记账照常。按 Actor* 打而不按名字，任何实体都行，也不必
                    // 把玩家名拼进带引号的命令（名字里有引号就撕开命令）。Override
                    // 是「不归因于任何具体来源」的通用伤害，正是这个槽位的语义：调
                    // 用方只给了一个伤害值。
                    return actor->hurtByCause(
                        static_cast<float>(a), ::SharedTypes::Legacy::ActorDamageCause::Override);
                }
                case PIER_AACT_ATTRIBUTE_GET:
                    return false; // 预留：按名取通用属性（v1.0.0 之后）
                /*  追加  */
                case PIER_AACT_SET_VARIANT:
                    actor->setVariant(static_cast<int>(a));
                    return true;
                case PIER_AACT_SET_MARK_VARIANT:
                    actor->setMarkVariant(static_cast<int>(a));
                    return true;
                case PIER_AACT_SET_PERSISTENT:
                    actor->setPersistent();
                    return true;
                case PIER_AACT_SET_LEASH_HOLDER:
                    actor->setLeashHolder(ActorUniqueID{static_cast<int64_t>(a)});
                    return true;
                case PIER_AACT_SET_INVISIBLE:
                    actor->setInvisible(a != 0.0);
                    return true;
                case PIER_AACT_SET_SNEAKING:
                    actor->setSneaking(a != 0.0);
                    return true;
                case PIER_AACT_SET_NAME_TAG_VISIBLE:
                    actor->setNameTagVisible(a != 0.0);
                    return true;
                case PIER_AACT_SET_TARGET:
                {
                    auto* target = bridge::resolveActor(static_cast<PierActorId>(a));
                    if (!target) return false;
                    actor->setTarget(target);
                    return true;
                }
                case PIER_AACT_SET_OWNER:
                    actor->setOwner(ActorUniqueID{static_cast<int64_t>(a)});
                    return true;
                case PIER_AACT_BURN:
                    // burn(int damage, bool inFire) —— inFire=true 表示来源是火焰
                    // 方块（相对于火焰附魔 / 岩浆 tick）。
                    actor->burn(static_cast<int>(a), true);
                    return true;
                case PIER_AACT_STOP_FIRE:
                    // Actor 没有 extinguishFire()；stopFire() 是 LL 暴露的接口。
                    actor->stopFire();
                    return true;
                case PIER_AACT_SET_VELOCITY:
                    actor->setVelocity(
                        Vec3{static_cast<float>(a), static_cast<float>(b), static_cast<float>(c)});
                    return true;
                case PIER_AACT_APPLY_IMPULSE:
                    actor->applyImpulse(
                        Vec3{static_cast<float>(a), static_cast<float>(b), static_cast<float>(c)});
                    return true;
                case PIER_AACT_SET_SCORE_TAG:
                    actor->setScoreTag(toString(sarg));
                    return true;
                case PIER_AACT_SET_SKIN_ID:
                    actor->setSkinID(static_cast<int>(a));
                    return true;
                case PIER_AACT_SET_STRENGTH:
                    actor->setStrength(static_cast<int>(a));
                    return true;
                case PIER_AACT_REMOVE_ALL_PASSENGERS:
                    // removeAllPassengers(bool actorIsBeingDestroyed, bool exitFromPassenger)
                    actor->removeAllPassengers(false, true);
                    return true;
                case PIER_AACT_EXECUTE_EVENT:
                    // 按名字触发行为包实体事件 —— 和 `/event entity <target>
                    // <event>` 同一件事。模组要改任何只以组件组形式存在的东西
                    //（体型、碰撞箱、AI 切换）只有这一条路：那些没有 setter，
                    // 这个游戏版本里 scale 连 synched data id 都没有。
                    if (sarg.len == 0) return false;
                    return actor->executeEvent(toString(sarg), VariantParameterList{});
                case PIER_AACT_SET_ROTATION:
                    // Vec2 是 (x = pitch, y = yaw) —— 与 ROT_* 属性读出的顺序一
                    // 致。用 Wrapped 不用 Directly：setRotationDirectly 跳过
                    // -180..180 的规整，yaw 给 400 会渲染成拧过头的脑袋。
                    actor->setRotationWrapped(Vec2{static_cast<float>(a), static_cast<float>(b)});
                    return true;
                default:
                    return false;
                }
            PIER_API_GUARD_END
        }

        bool api_spawn_mob(int32_t dim, PierStr typeName, double x, double y, double z, PierActorId* out)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                auto* bs = bridge::blockSourceOf(dim);
                if (!level || !bs) return false;
                ActorDefinitionIdentifier ident{toString(typeName)};
                auto* mob = level->getSpawner().spawnMob(
                    *bs,
                    ident,
                    /*spawner*/ nullptr,
                    Vec3{(float)x, (float)y, (float)z},
                    /*naturalSpawn*/ false,
                    /*surface*/ true,
                    /*fromSpawner*/ false
                );
                if (!mob) return false;
                if (out) *out = mob->getOrCreateUniqueID().rawID;
                return true;
            PIER_API_GUARD_END
        }

        void fill(PierApi& api)
        {
            api.list_actors = &api_list_actors;
            api.actor_snapshot = &api_actor_snapshot;
            api.actor_get_num = &api_actor_get_num;
            api.actor_get_str = &api_actor_get_str;
            api.actor_action = &api_actor_action;
            api.spawn_mob = &api_spawn_mob;
        }

        spi::SlotPackReg reg{{"actors", &fill}};
    } // namespace
} // namespace pier::api_impl
