//! 方块 / 物品 / 计分板 / 维度规则 / 系统与服务器信息 —— 逐值对着 `sdk/abi.h`。
//!
//! 全部是 `pub const i32` 而不是 Rust 的 `enum`。理由：ABI 上它们就是整数，
//! 宿主可能比模组**新**，传回一个这一侧还不认识的值。`enum` 遇到未列出的
//! 判别值是未定义行为，而常量只是一个没匹配上的数字 —— 后者可以被优雅地
//! 处理成「这个宿主报了我不认识的属性」，前者是内存不安全。
//!
//! 值也是 ABI，只能追加、不能重排（契约 §2.2）。名字与值的一致性由
//! `sys-mirrors-abi` 机检逐个守着。
//!
//! 注释的**归属**也要对：`abi.h` 里跨行的尾注属于**上一项**，不是下一项。
//! 第一版的转换器把它们原地搬了过来，于是每一条跨行尾注都挂到了错误的常量
//! 上，最后一条还悬空成了编译错误（`expected item after doc comment`）——
//! 那个编译错误是运气，前面那些挂错的没有任何提示。

// ── PierBlockNumProp ──────────────────────────────────────────

/// Block::isAir
pub const PIER_BPROP_IS_AIR: i32 = 0;
/// Block::getData (legacy data value)
pub const PIER_BPROP_DATA: i32 = 1;
/// Block::getBlockItemId
pub const PIER_BPROP_BLOCK_ITEM_ID: i32 = 2;
/// Block::isCraftingBlock
pub const PIER_BPROP_IS_CRAFTING_BLOCK: i32 = 3;
/// Block::isInteractiveBlock
pub const PIER_BPROP_IS_INTERACTIVE_BLOCK: i32 = 4;
/// BlockSource::getBlockEntity(pos) != null
pub const PIER_BPROP_HAS_BLOCK_ENTITY: i32 = 5;
/// ── 追加：block gap fill ──
/// Block::getLight
pub const PIER_BPROP_LIGHT: i32 = 6;
/// Block::getLightEmission
pub const PIER_BPROP_LIGHT_EMISSION: i32 = 7;
/// Block::getDestroySpeed
pub const PIER_BPROP_DESTROY_SPEED: i32 = 8;
/// Block::getExplosionResistance
pub const PIER_BPROP_EXPLOSION_RESISTANCE: i32 = 9;
/// Block::getFriction
pub const PIER_BPROP_FRICTION: i32 = 10;
/// Block::isContainerBlock
pub const PIER_BPROP_IS_CONTAINER: i32 = 11;
/// Block::isDoorBlock
pub const PIER_BPROP_IS_DOOR: i32 = 12;
/// Block::isFenceBlock
pub const PIER_BPROP_IS_FENCE: i32 = 13;
/// Block::isRailBlock
pub const PIER_BPROP_IS_RAIL: i32 = 14;
/// Block::isSlabBlock
pub const PIER_BPROP_IS_SLAB: i32 = 15;
/// Block::isStairBlock
pub const PIER_BPROP_IS_STAIR: i32 = 16;
/// Block::isWallBlock
pub const PIER_BPROP_IS_WALL: i32 = 17;
/// Block::isCropBlock
pub const PIER_BPROP_IS_CROP: i32 = 18;
/// Block::isUnbreakable
pub const PIER_BPROP_IS_UNBREAKABLE: i32 = 19;
/// Block::getDirectSignal
pub const PIER_BPROP_REDSTONE_SIGNAL: i32 = 20;
/// Block::getComparatorSignal
pub const PIER_BPROP_COMPARATOR_SIGNAL: i32 = 21;
/// Block::isSignalSource
pub const PIER_BPROP_IS_SIGNAL_SOURCE: i32 = 22;
/// Block::getVariant
pub const PIER_BPROP_VARIANT: i32 = 23;
/// Block::getBurnOdds
pub const PIER_BPROP_BURN_ODDS: i32 = 24;
/// Block::getFlameOdds
pub const PIER_BPROP_FLAME_ODDS: i32 = 25;
/// Block::getBounciness
pub const PIER_BPROP_BOUNCINESS: i32 = 26;
/// Block::isSolid
pub const PIER_BPROP_IS_SOLID: i32 = 27;
/// Block::requiresCorrectToolForDrops
pub const PIER_BPROP_REQUIRES_TOOL: i32 = 28;

// ── PierBlockStrProp ──────────────────────────────────────────

/// Block::getTypeName
pub const PIER_BSTR_TYPE_NAME: i32 = 0;
/// Block::mSerializationId → SNBT {name,states,version}
pub const PIER_BSTR_SNBT: i32 = 1;
/// Block::getDescriptionId
pub const PIER_BSTR_DESCRIPTION_ID: i32 = 2;
/// Block::toDebugString
pub const PIER_BSTR_DEBUG_STRING: i32 = 3;
/// Block::mTags → SNBT string list ["a","b"]
pub const PIER_BSTR_TAGS: i32 = 4;
/// ── 追加 ──
/// SNBT {state_name:value, …} all block states
pub const PIER_BSTR_STATE: i32 = 5;
/// SNBT [{min:[x,y,z],max:[x,y,z]}, …]
pub const PIER_BSTR_COLLISION_SHAPE: i32 = 6;
/// SNBT [{min,max}] render outline
pub const PIER_BSTR_OUTLINE_SHAPE: i32 = 7;
/// Block::getDisplayName
pub const PIER_BSTR_DISPLAY_NAME: i32 = 8;

// ── PierBlockAction ──────────────────────────────────────────

/// sarg=tag → out "0"/"1"  Block::hasTag
pub const PIER_BACT_HAS_TAG: i32 = 0;
/// ── 追加 ──
/// sarg=state name → out value string  Block::getState
pub const PIER_BACT_GET_STATE: i32 = 1;
/// sarg=item SNBT → pop resource at pos  Block::popResource
pub const PIER_BACT_POP_RESOURCE: i32 = 2;
/// → out item SNBT   Block::asItemInstance
pub const PIER_BACT_AS_ITEM: i32 = 3;

// ── PierItemNumProp ──────────────────────────────────────────

/// ItemStackBase::mCount
pub const PIER_IPROP_COUNT: i32 = 0;
/// ItemStackBase::getMaxStackSize
pub const PIER_IPROP_MAX_STACK_SIZE: i32 = 1;
/// ItemStackBase::getAuxValue
pub const PIER_IPROP_AUX_VALUE: i32 = 2;
/// ItemStackBase::getId
pub const PIER_IPROP_ID: i32 = 3;
/// ItemStackBase::getDamageValue
pub const PIER_IPROP_DAMAGE: i32 = 4;
/// ItemStackBase::isNull
pub const PIER_IPROP_IS_NULL: i32 = 5;
/// ItemStackBase::isBlock
pub const PIER_IPROP_IS_BLOCK: i32 = 6;
/// ItemStackBase::isEnchanted
pub const PIER_IPROP_IS_ENCHANTED: i32 = 7;
/// ItemStackBase::isArmorItem
pub const PIER_IPROP_IS_ARMOR: i32 = 8;
/// ItemStackBase::isDamageableItem
pub const PIER_IPROP_IS_DAMAGEABLE: i32 = 9;
/// ItemStackBase::isDamaged
pub const PIER_IPROP_IS_DAMAGED: i32 = 10;
/// ── 追加：item gap fill ──
/// ItemStackBase::getMaxDamage
pub const PIER_IPROP_MAX_DAMAGE: i32 = 11;
/// ItemStackBase::isUnbreakable
pub const PIER_IPROP_IS_UNBREAKABLE: i32 = 12;
/// ItemStackBase::hasDurability
pub const PIER_IPROP_HAS_DURABILITY: i32 = 13;
/// ItemStackBase::isPotionItem
pub const PIER_IPROP_IS_POTION: i32 = 14;
/// ItemStackBase::isThrowable
pub const PIER_IPROP_IS_THROWABLE: i32 = 15;
/// ItemStackBase::isFireResistant
pub const PIER_IPROP_IS_FIRE_RESISTANT: i32 = 16;
/// ItemStackBase::getAttackDamage
pub const PIER_IPROP_ATTACK_DAMAGE: i32 = 17;
/// ItemStackBase::getBaseRepairCost
pub const PIER_IPROP_REPAIR_COST: i32 = 18;
/// ItemStackBase::getEnchantValue
pub const PIER_IPROP_ENCHANT_VALUE: i32 = 19;
/// ItemStackBase::isStackable
pub const PIER_IPROP_IS_STACKABLE: i32 = 20;
/// ItemStackBase::isMusicDiscItem
pub const PIER_IPROP_IS_MUSIC_DISC: i32 = 21;
/// ItemStackBase::isOffhandItem
pub const PIER_IPROP_IS_OFFHAND: i32 = 22;
/// ItemStackBase::getMaxUseDuration
pub const PIER_IPROP_USE_DURATION: i32 = 23;
/// ItemStackBase::isGlint
pub const PIER_IPROP_IS_GLINT: i32 = 24;
/// ItemStackBase::isBundle
pub const PIER_IPROP_IS_BUNDLE: i32 = 25;
/// ItemStackBase::hasUserData
pub const PIER_IPROP_HAS_USER_DATA: i32 = 26;
/// ItemStackBase::hasCustomHoverName
pub const PIER_IPROP_HAS_CUSTOM_NAME: i32 = 27;

// ── PierItemStrProp ──────────────────────────────────────────

/// ItemStackBase::getTypeName ("minecraft:apple")
pub const PIER_ISTR_TYPE_NAME: i32 = 0;
/// ItemStackBase::getName (display)
pub const PIER_ISTR_NAME: i32 = 1;
/// ItemStackBase::getCustomName
pub const PIER_ISTR_CUSTOM_NAME: i32 = 2;
/// ItemStackBase::getRawNameId
pub const PIER_ISTR_RAW_NAME_ID: i32 = 3;
/// ── 追加 ──
/// SNBT list ["l1","l2"]  ItemStackBase::getCustomLore
pub const PIER_ISTR_LORE: i32 = 4;
/// SNBT list ["minecraft:stone", …]
pub const PIER_ISTR_CAN_DESTROY: i32 = 5;
/// SNBT list
pub const PIER_ISTR_CAN_PLACE_ON: i32 = 6;
/// full NBT user data as SNBT
pub const PIER_ISTR_USER_DATA: i32 = 7;
/// ItemStackBase::getHoverName
pub const PIER_ISTR_HOVER_NAME: i32 = 8;
/// ItemStackBase::getEffectName
pub const PIER_ISTR_EFFECT_NAME: i32 = 9;
/// SNBT {r,g,b}  ItemStackBase::getColor
pub const PIER_ISTR_COLOR: i32 = 10;

// ── PierItemOp ──────────────────────────────────────────

/// sarg=name             ItemStackBase::setCustomName
pub const PIER_IOP_SET_CUSTOM_NAME: i32 = 0;
/// narg=damage           ItemStackBase::setDamageValue
pub const PIER_IOP_SET_DAMAGE: i32 = 1;
/// narg=count            ItemStackBase::mCount
pub const PIER_IOP_SET_COUNT: i32 = 2;
/// sarg=SNBT list ["l1","l2"]  ItemStackBase::setCustomLore
pub const PIER_IOP_SET_LORE: i32 = 3;
/// ── 追加 ──
/// narg=0/1               ItemStackBase::setUnbreakable
pub const PIER_IOP_SET_UNBREAKABLE: i32 = 4;
/// narg=damage            ItemStackBase::hurtAndBreak
pub const PIER_IOP_HURT_AND_BREAK: i32 = 5;
/// narg=cost              ItemStackBase::setRepairCost
pub const PIER_IOP_SET_REPAIR_COST: i32 = 6;
/// sarg="name:level"      saveEnchantsToUserData
pub const PIER_IOP_ADD_ENCHANT: i32 = 7;
/// ItemStackBase::removeEnchants
pub const PIER_IOP_REMOVE_ENCHANTS: i32 = 8;
/// ItemStackBase::clearCustomLore
pub const PIER_IOP_CLEAR_LORE: i32 = 9;
/// ItemStackBase::resetHoverName
pub const PIER_IOP_RESET_NAME: i32 = 10;
/// sarg=SNBT list         ItemStackBase::setCanDestroy
pub const PIER_IOP_SET_CAN_DESTROY: i32 = 11;
/// sarg=SNBT list         ItemStackBase::setCanPlaceOn
pub const PIER_IOP_SET_CAN_PLACE_ON: i32 = 12;

// ── PierScoreboardOp ──────────────────────────────────────────

/// a=name, b=display name → out "1"      Scoreboard::addObjective("dummy")
pub const PIER_SB_ADD_OBJECTIVE: i32 = 0;
/// a=name                                Scoreboard::removeObjective
pub const PIER_SB_REMOVE_OBJECTIVE: i32 = 1;
/// → out SNBT [{name,display}, …]        Scoreboard::getObjectives
pub const PIER_SB_LIST_OBJECTIVES: i32 = 2;
/// a=objective, b=fake-player name → out value  Objective::getPlayerScore
pub const PIER_SB_GET_SCORE: i32 = 3;
/// a=objective, b=name, n=value          Scoreboard::modifyPlayerScore(Set)
pub const PIER_SB_SET_SCORE: i32 = 4;
/// a=objective, b=name, n=value          … (Add)
pub const PIER_SB_ADD_SCORE: i32 = 5;
/// a=objective, b=name, n=value          … (Subtract)
pub const PIER_SB_REDUCE_SCORE: i32 = 6;
/// a=objective, b=name                   Scoreboard::resetPlayerScore
pub const PIER_SB_RESET_SCORE: i32 = 7;
/// a=slot("sidebar"/"list"/"belowname"), b=objective  setDisplayObjective
pub const PIER_SB_SET_DISPLAY: i32 = 8;
/// a=slot                                clearDisplayObjective
pub const PIER_SB_CLEAR_DISPLAY: i32 = 9;

// ── PierDimRule ──────────────────────────────────────────

/// natural hostile spawns
pub const PIER_DIMRULE_SPAWN_MONSTER: i32 = 0;
/// natural passive spawns
pub const PIER_DIMRULE_SPAWN_ANIMAL: i32 = 1;
/// spawns from mob spawners
pub const PIER_DIMRULE_SPAWN_SPAWNER: i32 = 2;
/// explosions damaging terrain
pub const PIER_DIMRULE_EXPLODE_BLOCKS: i32 = 3;
/// fire spreading to neighbours
pub const PIER_DIMRULE_FIRE_SPREAD: i32 = 4;
/// mobs changing blocks
pub const PIER_DIMRULE_MOB_GRIEFING: i32 = 5;
/// projectile spawns
pub const PIER_DIMRULE_PROJECTILE: i32 = 6;
/// ── 第二批（挂载点参考 LegacyScriptEngine 的同名事件） ──
/// pistons moving blocks
pub const PIER_DIMRULE_PISTON_PUSH: i32 = 7;
/// water/lava spreading
pub const PIER_DIMRULE_LIQUID_FLOW: i32 = 8;
/// farmland trampled back to dirt
pub const PIER_DIMRULE_FARMLAND_DECAY: i32 = 9;
/// mounting boats/minecarts/animals
pub const PIER_DIMRULE_RIDE: i32 = 10;
/// ── Plot-boundary confinement (needs md_set_plot_grid) ──
/// Pistons moving blocks ACROSS a plot boundary. Distinct from
/// PIER_DIMRULE_PISTON_PUSH, which disables pistons for the whole
/// dimension: this one leaves them working inside a plot and only refuses
/// the push that would cross the edge. Both apply — either one denying is
/// enough to stop the push. Inert in dimensions with no registered grid.
pub const PIER_DIMRULE_PISTON_CROSS_PLOT: i32 = 11;
/// Entities crossing a plot boundary. Players and ridden vehicles are
/// never confined — see PlotConfine.cpp for why.
pub const PIER_DIMRULE_ENTITY_CROSS_PLOT: i32 = 12;

// ── PierSysInfoProp ──────────────────────────────────────────

/// sys_utils::getSystemName
pub const PIER_SYS_OS_NAME: i32 = 0;
/// sys_utils::getSystemVersion → string
pub const PIER_SYS_OS_VERSION: i32 = 1;
/// sys_utils::getSystemLocaleCode
pub const PIER_SYS_LOCALE: i32 = 2;
/// sys_utils::getLocalTime → SNBT {year,month,day,hour,minute,second,ms}
pub const PIER_SYS_LOCAL_TIME: i32 = 3;

// ── PierServerInfoProp ──────────────────────────────────────────

/// Common::getGameVersionString
pub const PIER_SRV_BDS_VERSION: i32 = 0;
/// SharedConstants::NetworkProtocolVersion → string
pub const PIER_SRV_PROTOCOL_VERSION: i32 = 1;
