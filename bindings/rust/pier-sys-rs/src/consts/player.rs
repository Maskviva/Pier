//! 玩家属性、字符串属性与动作码 —— 逐值对着 `sdk/abi.h`。
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

// ── PierPlayerNumProp ──────────────────────────────────────────

/// (G) Player::getPlayerGameType; write via player_set_gamemode
pub const PIER_PPROP_GAME_TYPE: i32 = 0;
/// (S) attribute Player::LEVEL()
pub const PIER_PPROP_LEVEL: i32 = 1;
/// (S) attribute Player::EXPERIENCE() (progress 0..1)
pub const PIER_PPROP_EXPERIENCE: i32 = 2;
/// (S) attribute Player::HUNGER()
pub const PIER_PPROP_HUNGER: i32 = 3;
/// (S) attribute Player::SATURATION()
pub const PIER_PPROP_SATURATION: i32 = 4;
/// (S) attribute Player::EXHAUSTION()
pub const PIER_PPROP_EXHAUSTION: i32 = 5;
/// (G) Player::getXpNeededForNextLevel
pub const PIER_PPROP_XP_NEEDED_NEXT_LEVEL: i32 = 6;
/// (G) Player::getLuck
pub const PIER_PPROP_LUCK: i32 = 7;
/// (G) Player::getSelectedItemSlot; set via PIER_PACT_SET_SELECTED_SLOT
pub const PIER_PPROP_SELECTED_SLOT: i32 = 8;
/// (G) Player::isOperator
pub const PIER_PPROP_IS_OPERATOR: i32 = 9;
/// (G) Player::canUseOperatorBlocks
pub const PIER_PPROP_CAN_USE_OPERATOR_BLOCKS: i32 = 10;
/// (G) Player::isFlying
pub const PIER_PPROP_IS_FLYING: i32 = 11;
/// (G) Player::canJump
pub const PIER_PPROP_CAN_JUMP: i32 = 12;
/// (G) Player::isEmoting
pub const PIER_PPROP_IS_EMOTING: i32 = 13;
/// (G) Player::isInRaid
pub const PIER_PPROP_IS_IN_RAID: i32 = 14;
/// (G) Player::isHurt
pub const PIER_PPROP_IS_HURT: i32 = 15;
/// (G) Player::isScoping
pub const PIER_PPROP_IS_SCOPING: i32 = 16;
/// (G) Player::canSleep
pub const PIER_PPROP_CAN_SLEEP: i32 = 17;
/// (G) Player::hasRespawnPosition
pub const PIER_PPROP_HAS_RESPAWN_POSITION: i32 = 18;
/// (G) Player::getClientSubId
pub const PIER_PPROP_CLIENT_SUB_ID: i32 = 19;
pub const PIER_PPROP_CAN_USE_ABILITY: i32 = 20;
/// (G) Player::canUseAbility; ability index passed via player_action GET path — see PIER_PACT_CAN_USE_ABILITY
/// ── 追加：player gap fill ──
/// (G) Player::getDirection (0=S,1=W,2=N,3=E)
pub const PIER_PPROP_DIRECTION: i32 = 21;
/// (G) Player::getChunkRadius
pub const PIER_PPROP_CHUNK_RADIUS: i32 = 22;
/// (G) getNetworkStatus().mPing (ms)
pub const PIER_PPROP_NETWORK_RTT: i32 = 23;
/// (G) Player::getPlatform
pub const PIER_PPROP_PLATFORM: i32 = 24;
/// (G) Player::getEnchantmentSeed
pub const PIER_PPROP_ENCHANTMENT_SEED: i32 = 25;
/// (G) Player::isUsingItem
pub const PIER_PPROP_IS_USING_ITEM: i32 = 26;
/// (G) Player::isBlocking
pub const PIER_PPROP_IS_BLOCKING: i32 = 27;
/// (G) Player::isGliding
pub const PIER_PPROP_IS_GLIDING: i32 = 28;
/// (G) Player::isSwimming
pub const PIER_PPROP_IS_SWIMMING: i32 = 29;
/// (G) Player::getPlayerPermissionLevel
pub const PIER_PPROP_PERMISSION_LEVEL: i32 = 30;
/// (G) Player::getScore
pub const PIER_PPROP_SCORE: i32 = 31;
/// (G) Actor::getFallDistance
pub const PIER_PPROP_FALL_DISTANCE: i32 = 32;
/// (G) Actor::isDead
pub const PIER_PPROP_IS_DEAD: i32 = 33;
/// (G) Player::hasDiedBefore
pub const PIER_PPROP_HAS_DIED_BEFORE: i32 = 34;
/// (G) Actor::getDimensionId
pub const PIER_PPROP_DIMENSION: i32 = 35;

// ── PierPlayerStrProp ──────────────────────────────────────────

/// Player::getRealName
pub const PIER_PSTR_REAL_NAME: i32 = 0;
/// Player::getUuid().asString()
pub const PIER_PSTR_UUID: i32 = 1;
/// Player::getXuid
pub const PIER_PSTR_XUID: i32 = 2;
/// Player::getIPAndPort
pub const PIER_PSTR_IP_AND_PORT: i32 = 3;
/// Player::getLocaleCode
pub const PIER_PSTR_LOCALE_CODE: i32 = 4;
/// Actor::getNameTag (display name)
pub const PIER_PSTR_NAME_TAG: i32 = 5;
/// ── 追加 ──
/// SNBT {x,y,z} or "" if none
pub const PIER_PSTR_LAST_DEATH_POS: i32 = 6;
/// dimension id as string
pub const PIER_PSTR_LAST_DEATH_DIMENSION: i32 = 7;
/// SNBT {ping,avg_ping,packet_loss,max_ping}
pub const PIER_PSTR_NETWORK_STATUS: i32 = 8;
/// Player::getPlatformOnlineId
pub const PIER_PSTR_PLATFORM_ONLINE_ID: i32 = 9;

// ── PierPlayerAction ──────────────────────────────────────────

pub const PIER_PACT_SET_ABILITY: i32 = 0;
/// a=AbilitiesIndex → out "0"/"1" Player::canUseAbility
pub const PIER_PACT_CAN_USE_ABILITY: i32 = 1;
/// a=slot                          Player::setSelectedSlot
pub const PIER_PACT_SET_SELECTED_SLOT: i32 = 2;
/// sarg=item SNBT                  ItemStack::fromTag + Player::addAndRefresh
pub const PIER_PACT_GIVE_ITEM: i32 = 3;
/// a,b,c=pos, sarg=dim ("0".."2")  via /spawnpoint
pub const PIER_PACT_SET_SPAWN_POINT: i32 = 4;
/// via /title clear
pub const PIER_PACT_CLEAR_TITLE: i32 = 5;
/// sarg=text, a=slot(0 title,1 subtitle,2 actionbar) via /title
pub const PIER_PACT_SET_TITLE: i32 = 6;
/// ── 追加 ──
/// a=xp                  Player::addExperience
pub const PIER_PACT_ADD_EXPERIENCE: i32 = 7;
/// a=levels              Player::addLevels
pub const PIER_PACT_ADD_LEVELS: i32 = 8;
/// sarg=item name, a=ticks Player::startItemCooldown
pub const PIER_PACT_START_COOLDOWN: i32 = 9;
/// a=vehicle ActorUniqueID (lower 64b) Player::startRiding
pub const PIER_PACT_START_RIDING: i32 = 10;
/// Player::stopRiding
pub const PIER_PACT_STOP_RIDING: i32 = 11;
/// a=target ActorUniqueID (lower 64b) Player::attack
pub const PIER_PACT_ATTACK: i32 = 12;
/// sarg=item SNBT, a=random(0/1) Player::drop
pub const PIER_PACT_DROP: i32 = 13;
/// a=target ActorUniqueID        Player::interact
pub const PIER_PACT_INTERACT: i32 = 14;
/// sarg=item SNBT, a=duration    Player::startUsingItem
pub const PIER_PACT_START_USING_ITEM: i32 = 15;
/// Player::stopUsingItem
pub const PIER_PACT_STOP_USING_ITEM: i32 = 16;
/// a=radius              Player::setChunkRadius
pub const PIER_PACT_SET_CHUNK_RADIUS: i32 = 17;
/// a=seed                Player::setEnchantmentSeed
pub const PIER_PACT_SET_ENCHANTMENT_SEED: i32 = 18;
/// a=boss ActorUniqueID  Player::registerTrackedBoss
pub const PIER_PACT_REGISTER_TRACKED_BOSS: i32 = 19;
/// a=boss ActorUniqueID Player::unRegisterTrackedBoss
pub const PIER_PACT_UNREGISTER_TRACKED_BOSS: i32 = 20;
/// sarg=piece id         Player::playEmote
pub const PIER_PACT_PLAY_EMOTE: i32 = 21;
/// Player::resendAllChunks
pub const PIER_PACT_RESEND_ALL_CHUNKS: i32 = 22;
/// Player::openInventory
pub const PIER_PACT_OPEN_INVENTORY: i32 = 23;
/// sarg="obj\ntitle\nline…"  per-player sidebar
pub const PIER_PACT_SIDEBAR_SET: i32 = 24;
/// sarg=objective        RemoveObjectivePacket
pub const PIER_PACT_SIDEBAR_CLEAR: i32 = 25;
/// a=PlayerPermissionLevel（0 Visitor / 1 Member / 2 Operator / 3 Custom）
/// LayeredAbilities::setPlayerPermissions + UpdateAbilitiesPacket。
/// 读的那一侧是 PIER_PPROP_PERMISSION_LEVEL。
pub const PIER_PACT_SET_PERMISSION_LEVEL: i32 = 26;
