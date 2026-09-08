/** actors/Actors.cpp: actor enumeration, snapshots, properties, actions and spawning.
 *  An actor handle is an ActorUniqueID, re-resolved through Level::fetchEntity on
 *  every call. */
#include <cmath>
#include <string>

#include "mc/deps/core/math/Vec2.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/legacy/ActorUniqueID.h"
#include "mc/util/VariantParameterList.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorDefinitionIdentifier.h"
#include "mc/deps/vanilla_components/StateVectorComponent.h"
#include "mc/entity/components/ActorRotationComponent.h"
#include "mc/world/actor/ActorDataIDs.h"
#include "mc/world/actor/ActorFlags.h"
#include "mc/world/actor/Mob.h"
#include "mc/world/actor/SynchedActorDataEntityWrapper.h"
#include "mc/world/actor/provider/SynchedActorDataAccess.h"
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
                    // Inlined away. It tested a liquid-contact flag that is not one of the
                    // ActorFlags reachable through SynchedActorDataAccess.
                    return false;
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
                    // The no-argument overload is inlined away. The surviving one asks
                    // whether this actor rides a particular vehicle, and a null vehicle is
                    // how it is asked about any of them.
                    *out = actor->isRiding(nullptr) ? 1.0 : 0.0;
                    return true;
                case PIER_APROP_IS_TAME:
                    *out = actor->isTame() ? 1.0 : 0.0;
                    return true;
                case PIER_APROP_SPEED:
                {
                    // getSpeedInMetersPerSecond() was inlined away in 26.32 and has no
                    // symbol left. mPosDelta is the per-tick movement the accessor read,
                    // and the engine runs at 20 ticks per second.
                    auto const& d = actor->getPosDelta();
                    *out = static_cast<double>(std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z) * 20.0f);
                    return true;
                }
                /*  Appended: actor gap fills  */
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
                    // Inlined away with no reachable field; setPersistent still works, so
                    // the flag can be written but not read back.
                    return false;
                case PIER_APROP_IS_LEASHED:
                    *out = actor->isLeashed() ? 1.0 : 0.0;
                    return true;
                case PIER_APROP_IS_INVULNERABLE:
                    // Actor offers only isInvulnerableTo(ActorDamageSource const&),
                    // there is no argument-free form, and ActorFlags::Invulnerable does
                    // not exist. Reported as unsupported.
                    return false;
                case PIER_APROP_VARIANT:
                    *out = static_cast<double>(actor->getVariant());
                    return true;
                case PIER_APROP_MARK_VARIANT:
                    *out = static_cast<double>(actor->getMarkVariant());
                    return true;
                case PIER_APROP_SCALE:
                    // getScaleFactor(float) sits behind #ifdef LL_PLAT_C and is
                    // unavailable.
                    return false;
                case PIER_APROP_BRIGHTNESS:
                {
                    // The no-argument overload is inlined away. The virtual survives and
                    // wants the region to read the light from, which is the one this actor
                    // stands in.
                    *out = static_cast<double>(
                        actor->getBrightness(0.0f, actor->getDimensionBlockSource()));
                    return true;
                }
                case PIER_APROP_RADIUS:
                    // getRadius() is gone; the bounding box size it derived from is
                    // reachable through the ECS. x is the width, and the radius is half.
                    *out = static_cast<double>(
                        SynchedActorDataAccess::getBoundingBoxSize(actor->getEntityContext()).x / 2.0f);
                    return true;
                case PIER_APROP_HAS_TOTEM:
                    // Inlined away. It walked the hand containers looking for the item,
                    // which needs an item identity this side cannot name.
                    return false;
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
                    // Actor has no isFrozen() and ActorFlags::Frozen does not exist.
                    // isImmobile() covers too many reasons for being unable to move and
                    // would produce false positives, so it is not used. Reported as
                    // unsupported.
                    return false;
                case PIER_APROP_IS_IN_LOVE:
                    // Actor::isInLove is gone in 26.40 and nothing replaces it. The
                    // mInLovePartner field survives, and reading it would be a guess at
                    // what the accessor tested, so the property is reported as
                    // unsupported instead of answering with a value that may be wrong.
                    return false;
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
                /*  Appended  */
                case PIER_ASTR_SCORE_TAG:
                    // getScoreTag() sits behind #ifdef LL_PLAT_C and is unavailable on
                    // the server. setScoreTag() exists, but the getter is client only.
                    return false;
                case PIER_ASTR_FILTERED_NAME:
                    // getFilteredNameTag() is likewise behind LL_PLAT_C, so the public
                    // member mFilteredNameTag, a TypedStorage<string>, is read
                    // directly. It is bound to a std::string const& first so that the
                    // string_view constructor sees a real std::string. Going from
                    // TypedStorage to string to string_view needs that explicit hop,
                    // because two implicit user-defined conversions are not allowed.
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
                    // The same gate player_teleport uses. The dimension bridge must be
                    // able to build the instance and the id the engine reports must
                    // match the request, otherwise a chunk thread throws an uncaught
                    // exception and the process fastfails.
                    if (!bridge::blockSourceOf(dim)) return false;
                    // teleport(pos, dim, rotation) keeps the actor's current
                    // orientation.
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
                    // 26.40 leaves the class with no constructor of its own and no
                    // declaration blocking the implicit one, so the instance is built
                    // empty and the four fields the caller supplies are assigned. Every
                    // other field is value-initialized, which is what the removed
                    // two-argument constructor left them as.
                    MobEffectInstance inst{};
                    inst.mId = effect->mId;
                    inst.mDuration = ::EffectDuration{static_cast<int>(a)};
                    inst.mAmplifier = static_cast<int>(b);
                    inst.mEffectVisible = (c != 0.0);
                    actor->addEffect(inst);
                    return true;
                }
                case PIER_AACT_REMOVE_EFFECT:
                {
                    auto* effect = MobEffect::getByName(toString(sarg));
                    if (!effect) return false;
                    actor->removeEffect(static_cast<int>(effect->mId));
                    return true;
                }
                case PIER_AACT_CLEAR_EFFECTS:
                    actor->removeAllEffects();
                    return true;
                case PIER_AACT_HURT:
                {
                    // Through Actor::hurtByCause, which takes an ActorDamageCause and
                    // keeps the engine's damage accounting intact. Damage is applied by
                    // Actor* and not by name, so it works for any actor and no player
                    // name has to be concatenated into a quoted command, where a quote
                    // in the name would tear the command apart. Override is generic
                    // damage attributed to no particular source, which is exactly the
                    // meaning of this slot: the caller supplied only an amount.
                    return actor->hurtByCause(
                        static_cast<float>(a), ::SharedTypes::Legacy::ActorDamageCause::Override);
                }
                case PIER_AACT_ATTRIBUTE_GET:
                    return false; // Reserved for generic attributes by name
                /*  Appended  */
                /*
                 * The synched-data writes below reach the field the removed setters wrote.
                 * set is a template declared MCAPI with no explicit instantiation, so which
                 * specializations exist is decided by what the engine binary happens to
                 * export, not by the header. set<std::string> is exported and set<int> is
                 * not, so the score tag is written and the three ids that are integers are
                 * refused.
                 */
                case PIER_AACT_SET_VARIANT:
                case PIER_AACT_SET_MARK_VARIANT:
                    // Both want set<int>, which the engine does not export.
                    return false;
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
                    // setNameTagVisible() wrote this one flag. setActorFlag is the same
                    // write and is the route Actor::getStatusFlag now reads back through.
                    SynchedActorDataAccess::setActorFlag(
                        actor->getEntityContext(), ::ActorFlags::CanShowName, a != 0.0);
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
                    // burn(int damage, bool inFire), where inFire=true means the source
                    // is a fire block rather than a fire enchantment or a lava tick.
                    actor->burn(static_cast<int>(a), true);
                    return true;
                case PIER_AACT_STOP_FIRE:
                    // Actor has no extinguishFire(). stopFire() is what LL exposes.
                    actor->stopFire();
                    return true;
                case PIER_AACT_SET_VELOCITY:
                    // setVelocity and applyImpulse are inlined away. mPosDelta is the
                    // per-tick movement both wrote, reachable through the component the
                    // surviving getPosDelta reads, so a set replaces the first and an add
                    // the second.
                    actor->mBuiltInComponents->mStateVectorComponent->mPosDelta =
                        Vec3{static_cast<float>(a), static_cast<float>(b), static_cast<float>(c)};
                    return true;
                case PIER_AACT_APPLY_IMPULSE:
                {
                    auto& delta = actor->mBuiltInComponents->mStateVectorComponent->mPosDelta.get();
                    delta.x += static_cast<float>(a);
                    delta.y += static_cast<float>(b);
                    delta.z += static_cast<float>(c);
                    return true;
                }
                case PIER_AACT_SET_SCORE_TAG:
                    actor->mEntityData->set<std::string>(
                        static_cast<ushort>(::ActorDataIDs::Score), toString(sarg));
                    return true;
                case PIER_AACT_SET_SKIN_ID:
                    // set<int> again.
                    return false;
                case PIER_AACT_SET_STRENGTH:
                    actor->setStrength(static_cast<int>(a));
                    return true;
                case PIER_AACT_REMOVE_ALL_PASSENGERS:
                    // removeAllPassengers(bool actorIsBeingDestroyed, bool exitFromPassenger)
                    actor->removeAllPassengers(false, true);
                    return true;
                case PIER_AACT_EXECUTE_EVENT:
                    // Fires a behavior pack entity event by name, the same thing
                    // `/event entity <target> <event>` does. It is the only route a mod
                    // has to anything that exists solely as a component group, such as
                    // size, collision box or an AI switch. Those have no setter, and in
                    // this game version scale has no synched data id at all.
                    if (sarg.len == 0) return false;
                    return actor->executeEvent(toString(sarg), VariantParameterList{});
                case PIER_AACT_SET_ROTATION:
                    // Vec2 is (x = pitch, y = yaw), matching the order the ROT_*
                    // properties read. setRotationWrapped is inlined away; mRot is the
                    // field it wrote, reachable through the component the surviving
                    // getRotation reads. Its normalization to -180..180 is done here too,
                    // because a yaw of 400 renders as an over-twisted head.
                    {
                        auto wrap = [](double v)
                        {
                            double r = std::fmod(v + 180.0, 360.0);
                            if (r < 0.0) r += 360.0;
                            return static_cast<float>(r - 180.0);
                        };
                        actor->mBuiltInComponents->mActorRotationComponent->mRot =
                            Vec2{wrap(a), wrap(b)};
                    }
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
