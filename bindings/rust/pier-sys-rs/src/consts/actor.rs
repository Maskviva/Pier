//! Actor properties, string properties and action codes, value by value against `sdk/abi.h`.
//!
//! All are `pub const i32` and not a Rust `enum`. The reason: on the ABI they are integers, and the
//! host may be newer than the mod and hand back a value this side does not recognize. An `enum`
//! meeting an unlisted discriminant is undefined behavior, while a constant is only a number that
//! matched nothing, and the latter can be handled gracefully as the host reporting an unknown
//! property while the former is memory unsafety.
//!
//! The values are ABI too and may only be appended, never reordered (contract §2.2). The agreement
//! of names and values is guarded one by one by the `sys-mirrors-abi` check.
//!
//! Comment ownership has to be right as well: a trailing comment that wraps in `abi.h` belongs to
//! the item above it and not the one below. A converter moving them in place attaches every wrapped
//! trailing comment to the wrong constant, and the last one dangles into a compile error, `expected
//! item after doc comment`. That compile error is luck; the ones attached wrongly before it give no
//! hint at all.

// ── PierActorNumProp ──────────────────────────────────────────

/// (G) Actor::getPosition().x (feet: getFeetPos for players; POS_* uses getPosition)
pub const PIER_APROP_POS_X: i32 = 0;
/// (G)
pub const PIER_APROP_POS_Y: i32 = 1;
/// (G)
pub const PIER_APROP_POS_Z: i32 = 2;
/// (G) Actor::getRotation().x
pub const PIER_APROP_ROT_PITCH: i32 = 3;
/// (G) Actor::getRotation().y
pub const PIER_APROP_ROT_YAW: i32 = 4;
/// (G) Actor::getDimensionId
pub const PIER_APROP_DIMENSION: i32 = 5;
/// (G) Actor::getHealth; heal/hurt via actions
pub const PIER_APROP_HEALTH: i32 = 6;
/// (G) Actor::getMaxHealth
pub const PIER_APROP_MAX_HEALTH: i32 = 7;
/// (G) Actor::isAlive
pub const PIER_APROP_IS_ALIVE: i32 = 8;
/// (G) Actor::isOnGround
pub const PIER_APROP_IS_ON_GROUND: i32 = 9;
/// (G) Actor::isInWater
pub const PIER_APROP_IS_IN_WATER: i32 = 10;
/// (G) Actor::isInLava
pub const PIER_APROP_IS_IN_LAVA: i32 = 11;
/// (G) Actor::isOnFire
pub const PIER_APROP_IS_ON_FIRE: i32 = 12;
/// (G) Actor::isInvisible
pub const PIER_APROP_IS_INVISIBLE: i32 = 13;
/// (G) Actor::isSneaking
pub const PIER_APROP_IS_SNEAKING: i32 = 14;
/// (G) Actor::isBaby
pub const PIER_APROP_IS_BABY: i32 = 15;
/// (G) Actor::isRiding
pub const PIER_APROP_IS_RIDING: i32 = 16;
/// (G) Actor::isTame
pub const PIER_APROP_IS_TAME: i32 = 17;
/// (G) Actor::getSpeedInMetersPerSecond
pub const PIER_APROP_SPEED: i32 = 18;
/// Appended: actor gap fill.
/// (G) Actor::getViewVector().x
pub const PIER_APROP_VIEW_X: i32 = 19;
/// (G) Actor::getViewVector().y
pub const PIER_APROP_VIEW_Y: i32 = 20;
/// (G) Actor::getViewVector().z
pub const PIER_APROP_VIEW_Z: i32 = 21;
/// (G) Actor::getVelocity().x
pub const PIER_APROP_VEL_X: i32 = 22;
/// (G) Actor::getVelocity().y
pub const PIER_APROP_VEL_Y: i32 = 23;
/// (G) Actor::getVelocity().z
pub const PIER_APROP_VEL_Z: i32 = 24;
/// (G) Actor::getHeadPos().x
pub const PIER_APROP_HEAD_X: i32 = 25;
/// (G) Actor::getHeadPos().y
pub const PIER_APROP_HEAD_Y: i32 = 26;
/// (G) Actor::getHeadPos().z
pub const PIER_APROP_HEAD_Z: i32 = 27;
/// (G) Actor::getFeetPos().x
pub const PIER_APROP_FEET_X: i32 = 28;
/// (G) Actor::getFeetPos().y
pub const PIER_APROP_FEET_Y: i32 = 29;
/// (G) Actor::getFeetPos().z
pub const PIER_APROP_FEET_Z: i32 = 30;
/// (G) Actor::getFallDistance
pub const PIER_APROP_FALL_DISTANCE: i32 = 31;
/// (G) Actor::isPersistent
pub const PIER_APROP_IS_PERSISTENT: i32 = 32;
/// (G) Actor::isLeashed
pub const PIER_APROP_IS_LEASHED: i32 = 33;
/// (G) Actor::isInvulnerable
pub const PIER_APROP_IS_INVULNERABLE: i32 = 34;
/// (G) Actor::getVariant
pub const PIER_APROP_VARIANT: i32 = 35;
/// (G) Actor::getMarkVariant
pub const PIER_APROP_MARK_VARIANT: i32 = 36;
/// (G) Actor::getScaleFactor
pub const PIER_APROP_SCALE: i32 = 37;
/// (G) Actor::getBrightness
pub const PIER_APROP_BRIGHTNESS: i32 = 38;
/// (G) Actor::getRadius
pub const PIER_APROP_RADIUS: i32 = 39;
/// (G) Actor::hasTotemEquipped
pub const PIER_APROP_HAS_TOTEM: i32 = 40;
/// (G) Actor::isInRain
pub const PIER_APROP_IS_IN_RAIN: i32 = 41;
/// (G) Actor::isInSnow
pub const PIER_APROP_IS_IN_SNOW: i32 = 42;
/// (G) Actor::isInThunderstorm
pub const PIER_APROP_IS_IN_THUNDERSTORM: i32 = 43;
/// (G) Actor::isFrozen
pub const PIER_APROP_IS_FROZEN: i32 = 44;
/// (G) Actor::isInLove
pub const PIER_APROP_IS_IN_LOVE: i32 = 45;
/// (G) Actor::getDeathTime
pub const PIER_APROP_DEATH_TIME: i32 = 46;
/// (G) Actor::hasPassenger
pub const PIER_APROP_HAS_PASSENGER: i32 = 47;

// ── PierActorStrProp ──────────────────────────────────────────

/// Actor::getTypeName
pub const PIER_ASTR_TYPE_NAME: i32 = 0;
/// Actor::getNameTag
pub const PIER_ASTR_NAME_TAG: i32 = 1;
/// Appended.
/// Actor::getScoreTag
pub const PIER_ASTR_SCORE_TAG: i32 = 2;
/// Actor::getFilteredNameTag
pub const PIER_ASTR_FILTERED_NAME: i32 = 3;

// ── PierActorAction ──────────────────────────────────────────

/// Actor::kill
pub const PIER_AACT_KILL: i32 = 0;
/// Actor::despawn
pub const PIER_AACT_DESPAWN: i32 = 1;
/// a=amount                            Actor::heal
pub const PIER_AACT_HEAL: i32 = 2;
/// a=seconds                           Actor::setOnFire
pub const PIER_AACT_SET_ON_FIRE: i32 = 3;
/// a,b,c=pos, sarg=dim ("0".."2")      Actor::teleport
pub const PIER_AACT_TELEPORT: i32 = 4;
/// sarg=name                           Actor::setNameTag
pub const PIER_AACT_SET_NAME_TAG: i32 = 5;
/// sarg=tag → out "0"/"1"              Actor::addTag
pub const PIER_AACT_ADD_TAG: i32 = 6;
/// sarg=tag → out "0"/"1"              Actor::removeTag
pub const PIER_AACT_REMOVE_TAG: i32 = 7;
/// sarg=tag → out "0"/"1"              Actor::hasTag
pub const PIER_AACT_HAS_TAG: i32 = 8;
/// sarg=effect name, a=ticks, b=amplifier, c=visible(0/1)
/// MobEffect::getByName + Actor::addEffect
pub const PIER_AACT_ADD_EFFECT: i32 = 9;
/// sarg=effect name                    Actor::removeEffect(id)
pub const PIER_AACT_REMOVE_EFFECT: i32 = 10;
/// Actor::removeAllEffects
pub const PIER_AACT_CLEAR_EFFECTS: i32 = 11;
/// a=damage (generic damage source)    Actor::hurt
pub const PIER_AACT_HURT: i32 = 12;
/// sarg=attribute name ("minecraft:health" …) → out value
pub const PIER_AACT_ATTRIBUTE_GET: i32 = 13;
/// Appended.
/// a=variant             Actor::setVariant
pub const PIER_AACT_SET_VARIANT: i32 = 14;
/// a=variant             Actor::setMarkVariant
pub const PIER_AACT_SET_MARK_VARIANT: i32 = 15;
/// Actor::setPersistent
pub const PIER_AACT_SET_PERSISTENT: i32 = 16;
/// a=holder ActorUniqueID Actor::setLeashHolder
pub const PIER_AACT_SET_LEASH_HOLDER: i32 = 17;
/// a=0/1                 Actor::setInvisible
pub const PIER_AACT_SET_INVISIBLE: i32 = 18;
/// a=0/1                 Actor::setSneaking
pub const PIER_AACT_SET_SNEAKING: i32 = 19;
/// a=0/1                 Actor::setNameTagVisible
pub const PIER_AACT_SET_NAME_TAG_VISIBLE: i32 = 20;
/// a=target ActorUniqueID Actor::setTarget
pub const PIER_AACT_SET_TARGET: i32 = 21;
/// a=owner ActorUniqueID  Actor::setOwner
pub const PIER_AACT_SET_OWNER: i32 = 22;
/// a=damage              Actor::burn
pub const PIER_AACT_BURN: i32 = 23;
/// Actor::extinguishFire
pub const PIER_AACT_STOP_FIRE: i32 = 24;
/// a,b,c=vel             Actor::setVelocity
pub const PIER_AACT_SET_VELOCITY: i32 = 25;
/// a,b,c=impulse         Actor::applyImpulse
pub const PIER_AACT_APPLY_IMPULSE: i32 = 26;
/// sarg=text             Actor::setScoreTag
pub const PIER_AACT_SET_SCORE_TAG: i32 = 27;
/// a=skin id             Actor::setSkinID
pub const PIER_AACT_SET_SKIN_ID: i32 = 28;
/// a=strength            Actor::setStrength
pub const PIER_AACT_SET_STRENGTH: i32 = 29;
/// Actor::removeAllPassengers
pub const PIER_AACT_REMOVE_ALL_PASSENGERS: i32 = 30;
/// sarg=event name       Actor::executeEvent
pub const PIER_AACT_EXECUTE_EVENT: i32 = 31;
/// a=pitch b=yaw         Actor::setRotationWrapped
pub const PIER_AACT_SET_ROTATION: i32 = 32;
