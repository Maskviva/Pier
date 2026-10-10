## f9fc90b8c3

> Common::getGameVersionString

取自 `Common::getGameVersionString`

## 1a723b6081

> LevelData::mNetworkVersion: the protocol the level was last written by. Says how old the save is, not what the server speaks.

`LevelData::mNetworkVersion`：最后一次写这个存档时用的协议版本。它说明存档有多旧，和服务器现在使用的协议无关。

## 215e16fdee

> "major.minor.patch" of the running build, from CurrentGameSemVersion. Lets a mod carry its own version→protocol table instead of waiting for a Pier release.

正在运行的构建的版本号 `"major.minor.patch"`，取自 `CurrentGameSemVersion`。模组可以据此自带一张版本到协议的对照表，不必等 Pier 发新版。

## 78441d619e

> sys_utils::getSystemName

取自 `sys_utils::getSystemName`

## 799228fdb2

> sys_utils::getSystemVersion → string

取自 `sys_utils::getSystemVersion`，返回字符串

## b5e6b792dd

> sys_utils::getSystemLocaleCode

取自 `sys_utils::getSystemLocaleCode`

## 2f1099953b

> sys_utils::getLocalTime → SNBT {year,month,day,hour,minute,second,ms}

取自 `sys_utils::getLocalTime`，返回 SNBT `{year,month,day,hour,minute,second,ms}`

## 851480bb97

> (G) Player::getPlayerGameType; write via player_set_gamemode

(G) `Player::getPlayerGameType`；写入用 `player_set_gamemode`

## 2823184b39

> (S) attribute Player::LEVEL()

(S) 属性 `Player::LEVEL()`

## 22f52257d5

> (S) attribute Player::EXPERIENCE() (progress 0..1)

(S) 属性 `Player::EXPERIENCE()`（经验条进度，0 到 1）

## a72aa8d2f9

> (S) attribute Player::HUNGER()

(S) 属性 `Player::HUNGER()`

## 63464f909e

> (S) attribute Player::SATURATION()

(S) 属性 `Player::SATURATION()`

## b6a0423e9d

> (S) attribute Player::EXHAUSTION()

(S) 属性 `Player::EXHAUSTION()`

## c00f6e728d

> (G) Player::getXpNeededForNextLevel

(G) `Player::getXpNeededForNextLevel`

## 7d1de3aa62

> (G) Player::getLuck

(G) `Player::getLuck`

## 8d91b1639d

> (G) Player::getSelectedItemSlot; set via PIER_PACT_SET_SELECTED_SLOT

(G) `Player::getSelectedItemSlot`；设置用 `PIER_PACT_SET_SELECTED_SLOT`

## 76a176b86c

> (G) Player::isOperator

(G) `Player::isOperator`

## 1715b41901

> (G) Player::canUseOperatorBlocks

(G) `Player::canUseOperatorBlocks`

## f2f511c3ae

> (G) Player::isFlying

(G) `Player::isFlying`

## 08eeb2ac2a

> (G) Player::canJump

(G) `Player::canJump`

## 31789c463f

> (G) Player::isEmoting

(G) `Player::isEmoting`

## 6821e32790

> (G) Player::isInRaid

(G) `Player::isInRaid`

## 58a480a207

> (G) Player::isHurt

(G) `Player::isHurt`

## d1e32c6147

> (G) Player::isScoping

(G) `Player::isScoping`

## 4905ef4618

> (G) Player::canSleep

(G) `Player::canSleep`

## c2dfa847ab

> (G) Player::hasRespawnPosition

(G) `Player::hasRespawnPosition`

## 7e32e6167c

> (G) Player::getClientSubId

(G) `Player::getClientSubId`

## 5d635bebac

> (G) Player::canUseAbility; the ability index is passed through the player_action GET path, see PIER_PACT_CAN_USE_ABILITY

(G) `Player::canUseAbility`；能力编号要经 `player_action` 的读取路径传入，见 `PIER_PACT_CAN_USE_ABILITY`

## bab1fe832b

> (G) Player::getDirection (0=S,1=W,2=N,3=E)

(G) `Player::getDirection`（0=南，1=西，2=北，3=东）

## d43dafb7c7

> (G) Player::getChunkRadius

(G) `Player::getChunkRadius`

## 7ad3aa9148

> (G) getNetworkStatus().mPing (ms)

(G) `getNetworkStatus().mPing`（毫秒）

## 37712bdf9a

> (G) Player::getPlatform

(G) `Player::getPlatform`

## 90775d85bb

> (G) Player::getEnchantmentSeed

(G) `Player::getEnchantmentSeed`

## f4f470fd2d

> (G) Player::isUsingItem

(G) `Player::isUsingItem`

## a070dac9e7

> (G) Player::isBlocking

(G) `Player::isBlocking`

## df858047b2

> (G) Player::isGliding

(G) `Player::isGliding`

## 8b8d184735

> (G) Player::isSwimming

(G) `Player::isSwimming`

## d5f28d8863

> (G) Player::getPlayerPermissionLevel

(G) `Player::getPlayerPermissionLevel`

## ead0f892a2

> (G) Player::getScore

(G) `Player::getScore`

## 475eb6b6a8

> (G) Actor::getFallDistance

(G) `Actor::getFallDistance`

## a201077dc3

> (G) Actor::isDead

(G) `Actor::isDead`

## 72dab0c24a

> (G) Player::hasDiedBefore

(G) `Player::hasDiedBefore`

## ca66b4be22

> (G) Actor::getDimensionId

(G) `Actor::getDimensionId`

## 51cceb0401

> Player::getRealName

取自 `Player::getRealName`

## f447b7a21d

> Player::getUuid().asString()

取自 `Player::getUuid().asString()`

## 598288b0bd

> Player::getXuid

取自 `Player::getXuid`

## d3699a9e8d

> Player::getIPAndPort

取自 `Player::getIPAndPort`

## c0e083072a

> Player::getLocaleCode

取自 `Player::getLocaleCode`

## 4671625dbd

> Actor::getNameTag (display name)

取自 `Actor::getNameTag`（显示名）

## 8d800c1cb5

> SNBT {x,y,z} or "" if none

SNBT `{x,y,z}`；没有时为 `""`

## ac7b8aaa7d

> dimension id as string

维度 id，以字符串表示

## 182768fa3e

> SNBT {ping,avg_ping,packet_loss,max_ping}

SNBT `{ping,avg_ping,packet_loss,max_ping}`

## 98a0ba3875

> Player::getPlatformOnlineId

取自 `Player::getPlatformOnlineId`

## f9295d53e9

> SNBT {x,y,z,dim}: where this player would respawn. Never empty; a player with no bed reports the world spawn, and the two are not distinguishable.

SNBT `{x,y,z,dim}`：这名玩家会在哪里重生。不会为空：没有床的玩家报告的是世界出生点，这两种情况区分不开。

## 8c9e518c80

> a=AbilitiesIndex, b=0/1 (bool slots) or float (FlySpeed etc.). Restores PlayerPermissionLevel to its pre-write value afterwards: the engine's LayeredAbilities::setAbility is the "switch to custom permissions" path and pushes the player to Custom, and that level ships to the client inside UpdateAbilitiesPacket together with the ability layer. To change the level, use PIER_PACT_SET_PERMISSION_LEVEL. Refused (false) until the player has finished joining (Player::isPlayerInitialized): an ability written while the client is still loading desynchronizes until the player rejoins. Simulated players are exempt.

`a` 为 `AbilitiesIndex`，`b` 为 0 或 1（布尔类的能力）或者浮点数（`FlySpeed` 等）。写完以后会把 `PlayerPermissionLevel` 恢复成写之前的值：引擎的 `LayeredAbilities::setAbility` 走的是「切换到自定义权限」那条路，会把玩家推到 Custom，而这个等级会和能力层一起放进 `UpdateAbilitiesPacket` 发给客户端。要改权限等级，用 `PIER_PACT_SET_PERMISSION_LEVEL`。玩家加入完成之前（`Player::isPlayerInitialized`）调用会被拒绝，返回 false：客户端还在加载时写入的能力会一直不同步，直到玩家重新进入。模拟玩家不受这个限制。

## 1b7850483b

> a=AbilitiesIndex → out "0"/"1" Player::canUseAbility

`a` 为 `AbilitiesIndex`，输出 `"0"` 或 `"1"`，调用 `Player::canUseAbility`

## 6bfa068ca2

> a=slot Player::setSelectedSlot

`a` 为槽位，调用 `Player::setSelectedSlot`

## f01b0f3863

> sarg=item SNBT ItemStack::fromTag + Player::addAndRefresh

`sarg` 为物品 SNBT，经 `ItemStack::fromTag` 和 `Player::addAndRefresh` 给出

## 9dcd22cb9c

> a,b,c=pos, sarg=dim id (any registered dim); native Player::setRespawnPosition

`a`、`b`、`c` 为坐标，`sarg` 为维度 id（任何已注册的维度都可以）；原生调用 `Player::setRespawnPosition`

## df906e92c4

> native SetTitlePacket(Clear)

原生发送 `SetTitlePacket(Clear)`

## a9b201667d

> sarg=text, a=slot(0 title,1 subtitle,2 actionbar); native SetTitlePacket, text sent verbatim

`sarg` 为文本，`a` 为位置（0 主标题，1 副标题，2 动作栏）；原生发送 `SetTitlePacket`，文本原样发送

## 056066bfc3

> a=xp Player::addExperience

`a` 为经验值，调用 `Player::addExperience`

## 2c5f5274c9

> a=levels Player::addLevels

`a` 为等级数，调用 `Player::addLevels`

## 8c7329f8af

> sarg=item name, a=ticks Player::startItemCooldown

`sarg` 为物品名，`a` 为刻数，调用 `Player::startItemCooldown`

## ab7f6de715

> a=vehicle ActorUniqueID (lower 64b) Player::startRiding

`a` 为坐骑的 `ActorUniqueID`（低 64 位），调用 `Player::startRiding`

## 91662e1972

> Player::stopRiding

调用 `Player::stopRiding`

## dc799386e8

> a=target ActorUniqueID (lower 64b) Player::attack

`a` 为目标的 `ActorUniqueID`（低 64 位），调用 `Player::attack`

## 4517e7085a

> sarg=item SNBT, a=random(0/1) Player::drop

`sarg` 为物品 SNBT，`a` 为是否随机抛出（0 或 1），调用 `Player::drop`

## 8b07f436f7

> a=target ActorUniqueID Player::interact

`a` 为目标的 `ActorUniqueID`，调用 `Player::interact`

## ae755d1bcf

> sarg=item SNBT, a=duration Player::startUsingItem

`sarg` 为物品 SNBT，`a` 为持续时间，调用 `Player::startUsingItem`

## 32e2b2c443

> Player::stopUsingItem

调用 `Player::stopUsingItem`

## 3611818b2e

> a=radius Player::setChunkRadius

`a` 为半径，调用 `Player::setChunkRadius`

## a0dc9dfa21

> a=seed Player::setEnchantmentSeed

`a` 为种子，调用 `Player::setEnchantmentSeed`

## eb0d5de91d

> a=boss ActorUniqueID Player::registerTrackedBoss

`a` 为 Boss 的 `ActorUniqueID`，调用 `Player::registerTrackedBoss`

## 8b9b8dab49

> a=boss ActorUniqueID Player::unRegisterTrackedBoss

`a` 为 Boss 的 `ActorUniqueID`，调用 `Player::unRegisterTrackedBoss`

## f02608b089

> sarg=piece id Player::playEmote

`sarg` 为表情的 id，调用 `Player::playEmote`

## bf73e5301a

> Player::resendAllChunks

调用 `Player::resendAllChunks`

## cca59793e5

> Player::openInventory

调用 `Player::openInventory`

## 6fb999acb7

> sarg="obj\ntitle\nline…" per-player sidebar

`sarg` 为 `"obj\ntitle\nline…"`，只对这名玩家显示的侧边栏

## 57484db635

> sarg=objective RemoveObjectivePacket

`sarg` 为计分项，发送 `RemoveObjectivePacket`

## 4207203a16

> a=PlayerPermissionLevel (0 Visitor, 1 Member, 2 Operator, 3 Custom). LayeredAbilities::setPlayerPermissions plus UpdateAbilitiesPacket. The read side is PIER_PPROP_PERMISSION_LEVEL. Refused until the player has finished joining, like PIER_PACT_SET_ABILITY.

`a` 为 `PlayerPermissionLevel`（0 Visitor，1 Member，2 Operator，3 Custom）。调用 `LayeredAbilities::setPlayerPermissions` 并发送 `UpdateAbilitiesPacket`。读取一侧是 `PIER_PPROP_PERMISSION_LEVEL`。和 `PIER_PACT_SET_ABILITY` 一样，玩家加入完成之前调用会被拒绝。

## 6435f2ec13

> (G) Actor::getPosition().x (feet: getFeetPos for players; POS_* uses getPosition)

(G) `Actor::getPosition().x`（玩家的脚下位置要用 `getFeetPos`；`POS_*` 用的是 `getPosition`）

## 0413a3e656

> (G)

(G)

## 996de39ff5

> (G) Actor::getRotation().x

(G) `Actor::getRotation().x`

## 7c8804f6ca

> (G) Actor::getRotation().y

(G) `Actor::getRotation().y`

## 0e82e6df83

> (G) Actor::getHealth; heal/hurt via actions

(G) `Actor::getHealth`；治疗和伤害用动作完成

## c71f87a071

> (G) Actor::getMaxHealth

(G) `Actor::getMaxHealth`

## b0c3904e3c

> (G) Actor::isAlive

(G) `Actor::isAlive`

## 5cff7de3f3

> (G) Actor::isOnGround

(G) `Actor::isOnGround`

## bc5b3601b7

> (G) Actor::isInWater

(G) `Actor::isInWater`

## c0b1e6b87f

> (G) Actor::isInLava

(G) `Actor::isInLava`

## 690a646c2d

> (G) Actor::isOnFire

(G) `Actor::isOnFire`

## 602a2776da

> (G) Actor::isInvisible

(G) `Actor::isInvisible`

## c4a5b0f733

> (G) Actor::isSneaking

(G) `Actor::isSneaking`

## fa806ebc18

> (G) Actor::isBaby

(G) `Actor::isBaby`

## a171f97aec

> (G) Actor::isRiding

(G) `Actor::isRiding`

## 4d95faffa7

> (G) Actor::isTame

(G) `Actor::isTame`

## ce34d14c87

> (G) Actor::getSpeedInMetersPerSecond

(G) `Actor::getSpeedInMetersPerSecond`

## e6113d2816

> (G) Actor::getViewVector().x

(G) `Actor::getViewVector().x`

## 616be379ff

> (G) Actor::getViewVector().y

(G) `Actor::getViewVector().y`

## 7fe195ccac

> (G) Actor::getViewVector().z

(G) `Actor::getViewVector().z`

## 22c560766b

> (G) Actor::getVelocity().x

(G) `Actor::getVelocity().x`

## 28cc205fae

> (G) Actor::getVelocity().y

(G) `Actor::getVelocity().y`

## 98594d0954

> (G) Actor::getVelocity().z

(G) `Actor::getVelocity().z`

## e538b25388

> (G) Actor::getHeadPos().x

(G) `Actor::getHeadPos().x`

## 418994743b

> (G) Actor::getHeadPos().y

(G) `Actor::getHeadPos().y`

## 22568fdad5

> (G) Actor::getHeadPos().z

(G) `Actor::getHeadPos().z`

## 3d799915ac

> (G) Actor::getFeetPos().x

(G) `Actor::getFeetPos().x`

## b623725478

> (G) Actor::getFeetPos().y

(G) `Actor::getFeetPos().y`

## 99b111a459

> (G) Actor::getFeetPos().z

(G) `Actor::getFeetPos().z`

## 5e5a908f98

> (G) Actor::isPersistent

(G) `Actor::isPersistent`

## 51810650d4

> (G) Actor::isLeashed

(G) `Actor::isLeashed`

## 9ca57c068f

> (G) Actor::isInvulnerable

(G) `Actor::isInvulnerable`

## 4ac19ceb97

> (G) Actor::getVariant

(G) `Actor::getVariant`

## 3b58872217

> (G) Actor::getMarkVariant

(G) `Actor::getMarkVariant`

## f4fdd04b13

> (G) Actor::getScaleFactor

(G) `Actor::getScaleFactor`

## 4dfe0cfc46

> (G) Actor::getBrightness

(G) `Actor::getBrightness`

## 990dcc47ae

> (G) Actor::getRadius

(G) `Actor::getRadius`

## 84ae2a5e92

> (G) Actor::hasTotemEquipped

(G) `Actor::hasTotemEquipped`

## 659dab255e

> (G) Actor::isInRain

(G) `Actor::isInRain`

## cb9b96ab24

> (G) Actor::isInSnow

(G) `Actor::isInSnow`

## 7c4ff3804d

> (G) Actor::isInThunderstorm

(G) `Actor::isInThunderstorm`

## fe2861e156

> (G) Actor::isFrozen

(G) `Actor::isFrozen`

## b1a49596cc

> (G) unsupported since BDS 1.26.40: Actor::isInLove is gone

(G) 从 BDS 1.26.40 起不再支持：`Actor::isInLove` 已被移除

## 14edd68f1a

> (G) Actor::getDeathTime

(G) `Actor::getDeathTime`

## 6fd08847b3

> (G) Actor::hasPassenger

(G) `Actor::hasPassenger`

## 86e2171377

> Actor::getTypeName

取自 `Actor::getTypeName`

## c33ad607ea

> Actor::getNameTag

取自 `Actor::getNameTag`

## a17208fdb2

> Actor::getScoreTag

取自 `Actor::getScoreTag`

## c31d00855f

> Actor::getFilteredNameTag

取自 `Actor::getFilteredNameTag`

## d0e5337fb7

> Actor::kill

调用 `Actor::kill`

## 6799e7fe89

> Actor::despawn

调用 `Actor::despawn`

## 6bc5fd4ef7

> a=amount Actor::heal

`a` 为治疗量，调用 `Actor::heal`

## bd3c5135ac

> a=seconds Actor::setOnFire

`a` 为秒数，调用 `Actor::setOnFire`

## 8be5696aba

> a,b,c=pos, sarg=dim ("0".."2") Actor::teleport

`a`、`b`、`c` 为坐标，`sarg` 为维度（`"0"` 到 `"2"`），调用 `Actor::teleport`

## 31a7c22e47

> sarg=name Actor::setNameTag

`sarg` 为名字，调用 `Actor::setNameTag`

## 0eac5d7ab7

> sarg=tag → out "0"/"1" Actor::addTag

`sarg` 为标签，输出 `"0"` 或 `"1"`，调用 `Actor::addTag`

## ff84723399

> sarg=tag → out "0"/"1" Actor::removeTag

`sarg` 为标签，输出 `"0"` 或 `"1"`，调用 `Actor::removeTag`

## dabf2173fd

> sarg=tag → out "0"/"1" Actor::hasTag

`sarg` 为标签，输出 `"0"` 或 `"1"`，调用 `Actor::hasTag`

## 141a0e8d05

> sarg=effect name, a=ticks, b=amplifier, c=visible(0/1) MobEffect::getByName + Actor::addEffect

`sarg` 为效果名，`a` 为刻数，`b` 为效果等级，`c` 为是否显示粒子（0 或 1），经 `MobEffect::getByName` 和 `Actor::addEffect` 完成

## 08bb4765c7

> sarg=effect name Actor::removeEffect(id)

`sarg` 为效果名，调用 `Actor::removeEffect(id)`

## 7943b90b3c

> Actor::removeAllEffects

调用 `Actor::removeAllEffects`

## c6b960afc3

> a=damage (generic damage source) Actor::hurt

`a` 为伤害值（通用伤害来源），调用 `Actor::hurt`

## 818af586a2

> sarg=attribute name ("minecraft:health"…) → out value

`sarg` 为属性名（`"minecraft:health"` 等），输出属性值

## 9fa6f51a72

> a=variant Actor::setVariant

`a` 为变种值，调用 `Actor::setVariant`

## c1caa72d1c

> a=variant Actor::setMarkVariant

`a` 为变种值，调用 `Actor::setMarkVariant`

## 35e46b3af0

> Actor::setPersistent

调用 `Actor::setPersistent`

## 726bc2a0b8

> a=holder ActorUniqueID Actor::setLeashHolder

`a` 为牵引者的 `ActorUniqueID`，调用 `Actor::setLeashHolder`

## 8b86736a41

> a=0/1 Actor::setInvisible

`a` 为 0 或 1，调用 `Actor::setInvisible`

## ea39f5a846

> a=0/1 Actor::setSneaking

`a` 为 0 或 1，调用 `Actor::setSneaking`

## 80eb29fa8c

> a=0/1 Actor::setNameTagVisible

`a` 为 0 或 1，调用 `Actor::setNameTagVisible`

## 403e4f7285

> a=target ActorUniqueID Actor::setTarget

`a` 为目标的 `ActorUniqueID`，调用 `Actor::setTarget`

## 89dcb51f3e

> a=owner ActorUniqueID Actor::setOwner

`a` 为主人的 `ActorUniqueID`，调用 `Actor::setOwner`

## f3fe535096

> a=damage Actor::burn

`a` 为伤害值，调用 `Actor::burn`

## af3eae60f2

> Actor::extinguishFire

调用 `Actor::extinguishFire`

## 232af31d8c

> a,b,c=vel Actor::setVelocity

`a`、`b`、`c` 为速度，调用 `Actor::setVelocity`

## 1e1ec74323

> a,b,c=impulse Actor::applyImpulse

`a`、`b`、`c` 为冲量，调用 `Actor::applyImpulse`

## ca36c95f43

> sarg=text Actor::setScoreTag

`sarg` 为文本，调用 `Actor::setScoreTag`

## 70a6d8939a

> a=skin id Actor::setSkinID

`a` 为皮肤 id，调用 `Actor::setSkinID`

## c338856148

> a=strength Actor::setStrength

`a` 为强度，调用 `Actor::setStrength`

## 320b0bbfcc

> Actor::removeAllPassengers

调用 `Actor::removeAllPassengers`

## 91a842ba3d

> sarg=event name Actor::executeEvent

`sarg` 为事件名，调用 `Actor::executeEvent`

## 134fe1be74

> a=pitch b=yaw Actor::setRotationWrapped

`a` 为俯仰角，`b` 为偏航角，调用 `Actor::setRotationWrapped`

## 8f1aea6cbe

> Block::isAir

取自 `Block::isAir`

## b8c45f0dec

> Block::getData (legacy data value)

取自 `Block::getData`（旧版的数据值）

## 5c476745e3

> Block::getBlockItemId

取自 `Block::getBlockItemId`

## 15b8c1ebe0

> Block::isCraftingBlock

取自 `Block::isCraftingBlock`

## 81a566051d

> Block::isInteractiveBlock

取自 `Block::isInteractiveBlock`

## 0e434b7526

> BlockSource::getBlockEntity(pos) != null

`BlockSource::getBlockEntity(pos) != null`

## dcd454c4ca

> Block::getLight

取自 `Block::getLight`

## 2e23d5c14a

> Block::getLightEmission

取自 `Block::getLightEmission`

## 6e723ab7af

> Block::getDestroySpeed

取自 `Block::getDestroySpeed`

## 7155607ecf

> Block::getExplosionResistance

取自 `Block::getExplosionResistance`

## 8f8b59c8d2

> Block::getFriction

取自 `Block::getFriction`

## 3da6f55452

> Block::isContainerBlock

取自 `Block::isContainerBlock`

## 14136c28c0

> unsupported since BDS 1.26.40: BlockType::isDoorBlock is gone

从 BDS 1.26.40 起不再支持：`BlockType::isDoorBlock` 已被移除

## 69c5f48722

> Block::isFenceBlock

取自 `Block::isFenceBlock`

## ea876f642d

> Block::isRailBlock

取自 `Block::isRailBlock`

## 8718c1902c

> Block::isSlabBlock

取自 `Block::isSlabBlock`

## 902b098d77

> unsupported since BDS 1.26.40: BlockType::isStairBlock is gone

从 BDS 1.26.40 起不再支持：`BlockType::isStairBlock` 已被移除

## ea62c5f0ce

> Block::isWallBlock

取自 `Block::isWallBlock`

## 990a3fbfe2

> Block::isCropBlock

取自 `Block::isCropBlock`

## 35187fd9ea

> Block::isUnbreakable

取自 `Block::isUnbreakable`

## fa0f57528d

> Block::getDirectSignal

取自 `Block::getDirectSignal`

## 86f6b341f4

> Block::getComparatorSignal

取自 `Block::getComparatorSignal`

## 83a12d259c

> Block::isSignalSource

取自 `Block::isSignalSource`

## 071553658c

> Block::getVariant

取自 `Block::getVariant`

## ea16e84208

> Block::getBurnOdds

取自 `Block::getBurnOdds`

## e9cabae08f

> Block::getFlameOdds

取自 `Block::getFlameOdds`

## 3be0170272

> Block::getBounciness

取自 `Block::getBounciness`

## 348f187502

> Block::isSolid

取自 `Block::isSolid`

## d6155e90ee

> Block::requiresCorrectToolForDrops

取自 `Block::requiresCorrectToolForDrops`

## 2e722f1422

> Block::getTypeName

取自 `Block::getTypeName`

## 410b999ce8

> Block::mSerializationId → SNBT {name,states,version}

取自 `Block::mSerializationId`，以 SNBT `{name,states,version}` 给出

## 673b5085d3

> Block::getDescriptionId

取自 `Block::getDescriptionId`

## 0ce026fd0f

> Block::toDebugString

取自 `Block::toDebugString`

## 6cd5716125

> Block::mTags → SNBT string list ["a","b"]

取自 `Block::mTags`，以 SNBT 字符串列表 `["a","b"]` 给出

## c28dee47a3

> SNBT {state_name:value,…} all block states

SNBT `{state_name:value,…}`，全部方块状态

## b1accbf99f

> SNBT [{min:[x,y,z],max:[x,y,z]},…]

SNBT `[{min:[x,y,z],max:[x,y,z]},…]`

## 6167051505

> SNBT [{min,max}] render outline

SNBT `[{min,max}]`，渲染用的轮廓

## 517f58f5f7

> Block::getDisplayName

取自 `Block::getDisplayName`

## 0c244dbf80

> sarg=tag → out "0"/"1" Block::hasTag

`sarg` 为标签，输出 `"0"` 或 `"1"`，调用 `Block::hasTag`

## 7afafb112c

> sarg=state name → out value string Block::getState

`sarg` 为状态名，输出状态值的字符串，调用 `Block::getState`

## 0196cf2b8c

> sarg=item SNBT → pop resource at pos Block::popResource

`sarg` 为物品 SNBT，在这个位置掉落该物品，调用 `Block::popResource`

## 67a38a6dd4

> → out item SNBT Block::asItemInstance

输出物品 SNBT，调用 `Block::asItemInstance`

## 2801d0cd95

> ItemStackBase::mCount

取自 `ItemStackBase::mCount`

## ae85cc79b9

> ItemStackBase::getMaxStackSize

取自 `ItemStackBase::getMaxStackSize`

## 1fb3fadb19

> ItemStackBase::getAuxValue

取自 `ItemStackBase::getAuxValue`

## 4fdb6f5949

> ItemStackBase::getId

取自 `ItemStackBase::getId`

## 7a8b8047b2

> ItemStackBase::getDamageValue

取自 `ItemStackBase::getDamageValue`

## 8a3dc8400d

> ItemStackBase::isNull

取自 `ItemStackBase::isNull`

## 04851e1969

> ItemStackBase::isBlock

取自 `ItemStackBase::isBlock`

## 1a9e410828

> ItemStackBase::isEnchanted

取自 `ItemStackBase::isEnchanted`

## 260f284392

> ItemStackBase::isArmorItem

取自 `ItemStackBase::isArmorItem`

## ac9d8ad3b9

> ItemStackBase::isDamageableItem

取自 `ItemStackBase::isDamageableItem`

## b01d23452c

> ItemStackBase::isDamaged

取自 `ItemStackBase::isDamaged`

## d32cbc4561

> ItemStackBase::getMaxDamage

取自 `ItemStackBase::getMaxDamage`

## 4ed1f0e6df

> ItemStackBase::isUnbreakable

取自 `ItemStackBase::isUnbreakable`

## eff205c884

> ItemStackBase::hasDurability

取自 `ItemStackBase::hasDurability`

## 604e3fbfad

> ItemStackBase::isPotionItem

取自 `ItemStackBase::isPotionItem`

## 773606afc9

> ItemStackBase::isThrowable

取自 `ItemStackBase::isThrowable`

## 06ef1840d7

> ItemStackBase::isFireResistant

取自 `ItemStackBase::isFireResistant`

## 4674d985df

> ItemStackBase::getAttackDamage

取自 `ItemStackBase::getAttackDamage`

## b803756eb4

> ItemStackBase::getBaseRepairCost

取自 `ItemStackBase::getBaseRepairCost`

## 7b98dfb484

> ItemStackBase::getEnchantValue

取自 `ItemStackBase::getEnchantValue`

## c862596e1b

> ItemStackBase::isStackable

取自 `ItemStackBase::isStackable`

## 672e5cfa9f

> ItemStackBase::isMusicDiscItem

取自 `ItemStackBase::isMusicDiscItem`

## 1991c5749a

> ItemStackBase::isOffhandItem

取自 `ItemStackBase::isOffhandItem`

## 7866e9a673

> ItemStackBase::getMaxUseDuration

取自 `ItemStackBase::getMaxUseDuration`

## 3fa86f4c38

> ItemStackBase::isGlint

取自 `ItemStackBase::isGlint`

## 66df19d29f

> ItemStackBase::isBundle

取自 `ItemStackBase::isBundle`

## 07ae4c9f0e

> ItemStackBase::hasUserData

取自 `ItemStackBase::hasUserData`

## 03c75e4f3c

> ItemStackBase::hasCustomHoverName

取自 `ItemStackBase::hasCustomHoverName`

## 524f9e6b85

> ItemStackBase::getTypeName ("minecraft:apple")

取自 `ItemStackBase::getTypeName`（形如 `"minecraft:apple"`）

## 22569745e2

> ItemStackBase::getName (display)

取自 `ItemStackBase::getName`（显示名）

## f3717d0647

> ItemStackBase::getCustomName

取自 `ItemStackBase::getCustomName`

## 94f065ba53

> ItemStackBase::getRawNameId

取自 `ItemStackBase::getRawNameId`

## 73e03852a1

> SNBT list ["l1","l2"] ItemStackBase::getCustomLore

SNBT 列表 `["l1","l2"]`，取自 `ItemStackBase::getCustomLore`

## 0e9ae0d695

> SNBT list ["minecraft:stone",…]

SNBT 列表 `["minecraft:stone",…]`

## 1e87262139

> SNBT list

SNBT 列表

## 70def02daa

> full NBT user data as SNBT

完整的 NBT 用户数据，以 SNBT 表示

## 61d2b87c14

> ItemStackBase::getHoverName

取自 `ItemStackBase::getHoverName`

## 8e048829a2

> ItemStackBase::getEffectName

取自 `ItemStackBase::getEffectName`

## ed7911487b

> SNBT {r,g,b} ItemStackBase::getColor

SNBT `{r,g,b}`，取自 `ItemStackBase::getColor`

## 86b16f4019

> Log a message through the mod's own LeviLamina logger.
> level: -1=Off, 0=Fatal, 1=Error, 2=Warn, 3=Info, 4=Debug, 5=Trace
> (mirrors ll::io::LogLevel). Thread-safe.

通过模组自己的 LeviLamina 日志器写一条日志。`level`：-1=Off，0=Fatal，1=Error，2=Warn，3=Info，4=Debug，5=Trace（对应 `ll::io::LogLevel`）。线程安全。

## 1de24a7072

> Current gaming status: 0=Default, 1=Starting, 2=Running, 3=Stopping
> (mirrors ll::GamingStatus). Thread-safe.

当前的运行状态：0=Default，1=Starting，2=Running，3=Stopping（对应 `ll::GamingStatus`）。线程安全。

## a3af02ef9d

> Drop a task scheduled by this mod if it has not run yet. Returns true
> if a pending task was actually dropped. Safe to call from any thread
> and from inside another task. Cancelling leaks `user` for the same
> reason as above, so prefer letting short tasks run.

这个模组排的某个任务如果还没运行，就把它丢掉。真的丢掉了一个待执行的任务时返回 true。可以从任何线程调用，也可以在另一个任务里调用。取消同样会泄漏 `user`，原因和上面一样，所以短任务最好让它跑完。

## fd067b65d1

> Number of tasks this mod still has pending. Intended for a mod to
> assert it has drained its own work in on_disable / on_unload, which is
> a precondition for being marked "reload_safe" in its manifest.

这个模组还有多少个待执行的任务。供模组在 `on_disable` / `on_unload` 里确认自己的工作已经清空，这是在清单里标记 `"reload_safe"` 的前提。

## 7bc41b5832

> Remove a listener previously returned by subscribe_event. Server thread only.

移除之前由 `subscribe_event` 返回的监听器。只能在服务器线程调用。

## df875e344a

> Enumerate all currently registered event ids. Server thread only.

列出当前已注册的所有事件 id。只能在服务器线程调用。

## 54ef17a294

> values_snbt = {values:[["name",1L],…]}  → tryRegisterRuntimeEnum.

`values_snbt` 形如 `{values:[["name",1L],…]}`，交给 `tryRegisterRuntimeEnum`。

## 87be740417

> values_snbt = {values:["a","b"]}         → tryRegisterSoftEnum.

`values_snbt` 形如 `{values:["a","b"]}`，交给 `tryRegisterSoftEnum`。

## d23e53f319

> op: 0=set 1=add 2=remove.

`op`：0=设置，1=添加，2=移除。

## 3ae5e56124

> Current server tick (the tickID from Level::getCurrentTick()).
> Returns 0 when the level is not ready. Server thread only.

当前的服务器刻（`Level::getCurrentTick()` 里的 tickID）。世界还没就绪时返回 0。只能在服务器线程调用。

## 53a15efb73

> Wall-clock period of the last frame in seconds (mTickDeltaTime; 0.05 at 20
> TPS). It includes the sleep the server inserts to hold 20 Hz, so it is not
> the time spent computing a tick and its reciprocal is not a tick rate: it
> is one noisy sample of the frame rate. For TPS and MSPT use get_tps and
> get_mspt. Returns -1.0 if unavailable. Server thread only.

上一帧实际经过的时间，单位秒（`mTickDeltaTime`；20 TPS 时是 0.05）。它包含服务器为维持 20 Hz 插入的休眠，所以它和计算一刻所花的时间是两回事，它的倒数也不能当刻率用：它只是帧率的一个带噪声的样本。要 TPS 和 MSPT，请用 `get_tps` 和 `get_mspt`。取不到时返回 -1.0。只能在服务器线程调用。

## 4f4dce85f1

> Number of currently connected players
> (Level::getActivePlayerCount()). Server thread only.

当前连接的玩家数（`Level::getActivePlayerCount()`）。只能在服务器线程调用。

## 9d27574ed7

> Whether the simulation is currently paused
> (Level::getSimPaused()). Server thread only.

模拟当前是否暂停（`Level::getSimPaused()`）。只能在服务器线程调用。

## b0de256497

> System info: THREAD-SAFE (plain OS calls).

系统信息：线程安全（只是普通的操作系统调用）。

## 7df1d8ae0e

> Tick control (additive, gated by struct_size). Backed by a bridge-owned
> detour on Level::tick, installed lazily on the first control call and
> left in place (idle cost: one predictable branch per frame — a control
> call can arrive from a command handler that is executing INSIDE the
> tick, where unpatching would not be safe). Server thread only.
> While frozen, mobs/blocks/redstone/time stop; players can still move
> and chat (movement is client-authoritative, network runs outside the
> level tick).

控制刻的推进（追加的槽位，受 `struct_size` 约束）。实现方式是 bridge 在 `Level::tick` 上挂一个钩子：第一次调用控制接口时才安装，装上以后一直留着，空闲时每帧多一次可以预测的分支。一直留着的原因是，控制调用可能来自正在刻**内部**执行的命令处理函数，在那里卸下钩子并不安全。只能在服务器线程调用。

冻结期间，生物、方块、红石和时间都停下；玩家仍然可以移动和聊天，因为移动由客户端决定，网络在关卡的刻之外运行。

## 920c69e041

> Only while frozen: queue exactly n extra frames. False if not frozen or n == 0.

只在冻结时有效：额外排入正好 n 帧。没有冻结或 n == 0 时返回 false。

## fded4d79ae

> 0 < factor <= 100. Fractional = slow motion (accumulator), 1.0 restores normal.

0 < factor <= 100。小数表示慢动作（用累加器实现），1.0 恢复正常。

## d709a2ca4f

> Arm a window of `ticks` level ticks (1..12000). False if 0, too big, or already sampling.

开启一个长度为 `ticks` 个关卡刻（1 到 12000）的采样窗口。为 0、太大，或者已经在采样时返回 false。

## e5accbd927

> Ticks per second over the last `window_seconds` (1..60, clipped) of wall
> clock: the number of Level::tick calls that really ran divided by elapsed
> time. Stays correct under the tick warp (reads above 20), the tick freeze
> (reads 0) and lag (reads below 20). Returns -1.0 before the first frame
> has been sampled. Server thread only.

最近 `window_seconds` 秒（1 到 60，超出会被截断）的实际时间里，每秒运行的刻数：真正运行过的 `Level::tick` 次数除以经过的时间。刻加速时（读数高于 20）、刻冻结时（读数为 0）和卡顿时（读数低于 20）都是准确的。第一帧还没采样时返回 -1.0。只能在服务器线程调用。

## 2664acb4ba

> Milliseconds spent inside Level::tick per tick, averaged over the last
> `window_seconds` (1..60, clipped). This is the time the server computes,
> excluding the idle sleep between frames; a healthy server reads a few
> milliseconds and only approaches 50 when saturated. Returns -1.0 when no
> tick ran in the window. Server thread only.

最近 `window_seconds` 秒（1 到 60，超出范围会被截断）里，每一刻在 `Level::tick` 内花费的毫秒数的平均值。这是服务器计算所用的时间，不含帧与帧之间空闲的休眠；健康的服务器读数是几毫秒，只有满负荷时才接近 50。窗口内一刻都没有运行时返回 -1.0。只能在服务器线程调用。

## 1688a0352b

> Spawn a particle effect at a world coordinate. Used to outline a
> selection box edge-by-edge. Server thread only. Returns false if the
> level/dimension is not ready.
> dimension   : 0 = overworld, 1 = nether, 2 = the end.
> effect_name : e.g. "minecraft:basic_flame_particle" or
> "minecraft:redstone_wire_dust_particle".

在一个世界坐标上生成粒子效果，可以用来逐条边地勾出选区的轮廓。只能在服务器线程调用。世界或维度还没就绪时返回 false。

- `dimension`：0 为主世界，1 为下界，2 为末地。
- `effect_name`：例如 `"minecraft:basic_flame_particle"` 或 `"minecraft:redstone_wire_dust_particle"`。

## 13337e25e2

> Place a block natively (BlockSource::setBlock, DEFAULT update flags).
> block_spec = "minecraft:stone" / "stone" (default state) or a full
> {name,states,...} SNBT. Unknown names fail instead of placing a placeholder.

原生放置方块（`BlockSource::setBlock`，默认的更新标志）。`block_spec` 可以是 `"minecraft:stone"` 或 `"stone"`（使用默认状态），也可以是完整的 `{name,states,...}` SNBT。名字认不出来时调用失败，不会放一个占位方块。

## abfee0cf26

> World time (Level::getTime).

世界时间（`Level::getTime`）。

## fbe2d388ac

> Set world time natively (Level::setTime).

原生设置世界时间（`Level::setTime`）。

## 33fba40aa8

> 0=clear 1=rain 2=thunder, native (Level::updateWeather).

0=晴，1=雨，2=雷雨，原生调用（`Level::updateWeather`）。

## 7894ad1f1f

> Level::explode. source may be 0 (no source actor).

调用 `Level::explode`。`source` 可以为 0，表示没有来源实体。

## bad8569811

> Server / world-level settings.

服务器和世界级别的设置。

## dee116820d

> out sink receives SNBT {type:"bool"|"int"|"float", value:…}; false if unknown rule.

输出回调收到 SNBT `{type:"bool"|"int"|"float", value:…}`；规则不存在时返回 false。

## 8265d4506c

> Per-player particle packet (additive, gated by struct_size).
> Sends a SpawnParticleEffectPacket ONLY to the resolved player
> (Player::sendNetworkPacket) instead of Level::spawnParticleEffect's
> dimension-wide broadcast — other clients never receive it.
> `dimension` is the vanilla dimension id carried in the packet; pass the
> dimension the coordinates refer to (normally the player's own — clients
> don't render particles for another dimension).
> False if the player is offline / can't be resolved.

只给一名玩家发送粒子包（追加的槽位，受 `struct_size` 约束）。`SpawnParticleEffectPacket` **只**发给解析出来的那名玩家（`Player::sendNetworkPacket`）；`Level::spawnParticleEffect` 会向整个维度广播，这里其他客户端收不到这个包。`dimension` 是包里带的原版维度 id，传坐标所在的维度，通常就是这名玩家所在的维度，因为客户端不渲染别的维度的粒子。玩家不在线或解析不到时返回 false。

## 921d42d3e2

> Enumerate villages in a dimension. Each: {uuid, center:[x,y,z],
> bounds:{min,max}, poi_count}.

列出一个维度里的村庄。每一项是 `{uuid, center:[x,y,z], bounds:{min,max}, poi_count}`。

## cd5f7ba518

> Hardcoded spawn areas (nether fortress / witch hut / ocean monument /
> pillager outpost) whose chunks intersect a radius around (x,y,z). Each:
> {type, bounds:{min,max}}. Only LOADED chunks are inspected — a
> read-only query never force-loads.

以 (x,y,z) 为中心的一个半径内，所在区块与之相交的硬编码生成区：下界要塞、女巫小屋、海底神殿、掠夺者前哨站。每一项是 `{type, bounds:{min,max}}`。只检查**已加载**的区块，这个只读查询从不强制加载区块。

## 9b074132b3

> Level: biome, spawn, save, weather, path, sleep (dedicated fns)

关卡：生物群系、出生点、保存、天气、寻路、睡眠（专用函数）

## 61e4bd6ad5

> SNBT {sleeping, total_players, active_sleeping}

SNBT `{sleeping, total_players, active_sleeping}`

## 352d8c7b06

> SNBT {nodes:[{x,y,z},…], reached:1b/0b}. NULL on every current host:
> pathfinding is not implemented.

SNBT `{nodes:[{x,y,z},…], reached:1b/0b}`。当前所有宿主上这个槽位都是 NULL：寻路还没有实现。

## 76bb521743

> Read the liquid layer. The sink receives a block name such as
> "minecraft:water"; an empty layer gives air.

读取液体层。输出回调收到一个方块名，例如 `"minecraft:water"`；这一层为空时给出空气。

## 8f6f3ea6ba

> Write the liquid layer. block_spec takes a bare block name or full SNBT;
> write "minecraft:air" to clear it. update_flags is as in
> edit_set_block_nbt: bit 1 notifies neighbours, bit 2 syncs the client.

写入液体层。`block_spec` 可以是单纯的方块名，也可以是完整的 SNBT；写入 `"minecraft:air"` 就是清空。`update_flags` 的含义和 `edit_set_block_nbt` 一样：第 1 位通知相邻方块，第 2 位同步给客户端。

## 94efcc06ef

> Write a block from serialized NBT ({name,states,version}, i.e. the
> shape get_block produces).

用序列化的 NBT 写入一个方块，格式是 `{name,states,version}`，也就是 `get_block` 给出的形状。

## 4eed9ed490

> Write a block from a name + optional partial states. An empty
> states_snbt means all-default states; the version is taken from the
> default state on the loader side — the caller must not supply one.

用方块名加上可选的部分状态写入一个方块。`states_snbt` 为空表示全部用默认状态；版本号取自加载器一侧的默认状态，调用方不能自己提供。

## 0d4f184c87

> Write a block entity's NBT back (BlockActor::load). The cell must
> already hold the matching block.

把一个方块实体的 NBT 写回去（`BlockActor::load`）。这一格里必须已经是对应的方块。

## 0223c36eb1

> Spawn an entity from full NBT (the inverse of actor_snapshot). When
> use_pos is true, (x,y,z) overrides the Pos tag; the UniqueID is
> reassigned by the engine and returned via out.
> NULL on every current host: the engine helper that gives a loaded
> actor a fresh UniqueID is inlined, and reusing the snapshot's own id
> makes the engine treat two actors as one.

用完整的 NBT 生成一个实体，是 `actor_snapshot` 的反向操作。`use_pos` 为 true 时，(x,y,z) 覆盖 `Pos` 标签；引擎会重新分配 UniqueID，并通过 `out` 返回。

当前所有宿主上这个槽位都是 NULL：给读入的实体分配新 UniqueID 的那个引擎函数被内联掉了，而沿用快照里原来的 id 会让引擎把两个实体当成同一个。

## 4dea4d8da4

> Ray trace yielding the BLOCK coordinate and hit face:
> {type, block:[x,y,z], facing, pos:[x,y,z], entity}.

射线检测，给出命中的**方块**坐标和命中的面：`{type, block:[x,y,z], facing, pos:[x,y,z], entity}`。

## 4b2c1e22db

> Fills a box with one block. block_spec is a bare name such as
> "minecraft:stone" or full SNBT; it is resolved once for the whole box.
> update_flags is as in edit_set_block_nbt: bit 1 notifies neighbours, bit
> 2 syncs the client; 0 is the fastest and leaves the client to catch up on
> the next chunk send. Returns the number of cells written, or -1 if the
> dimension is not ready, the spec does not resolve, or the box exceeds
> 2^24 cells. Server thread only.

用同一种方块填满一个长方体。`block_spec` 可以是 `"minecraft:stone"` 这样的方块名，也可以是完整的 SNBT，整个长方体只解析一次。`update_flags` 的含义和 `edit_set_block_nbt` 一样：第 1 位通知相邻方块，第 2 位同步给客户端；传 0 最快，客户端要等下一次发送区块时才看到变化。返回写入的格子数；维度没有准备好、`block_spec` 解析不出来，或者长方体超过 2^24 个格子时返回 -1。只能在服务器线程调用。

## a29df2041e

> Look up a connected player's feet position and dimension by name.
> Used to pick selection corners from where the player is standing.
> Server thread only.

按名字查找一名已连接玩家的脚下位置和所在维度。用于根据玩家站的位置选取选区的角。只能在服务器线程调用。

## 24023099cb

> One SNBT per online player: {name,xuid,uuid,dim,x,y,z}.

每个在线玩家一段 SNBT：`{name,xuid,uuid,dim,x,y,z}`。

## 42478ae521

> Resolve a player selector to their ActorUniqueID (bridges into the actor_* API).

把玩家选择器解析成这名玩家的 `ActorUniqueID`，由此接到 `actor_*` 这组接口上。

## 1ae092e853

> sendMessage to every online player.

对每个在线玩家调用 `sendMessage`。

## 22a42d6015

> 0=survival 1=creative 2=adventure 6=spectator, native (Player::setPlayerGameType).

0=生存，1=创造，2=冒险，6=旁观，原生调用（`Player::setPlayerGameType`）。

## 906b4069f2

> Teleport natively (Actor::teleport). Custom dimensions (id >= 3) are allowed;
> the dimension bridge must produce an engine instance whose id matches, or the
> call fails instead of sending the player into a mismatched dimension.

原生传送（`Actor::teleport`）。可以传送到自定义维度（id >= 3），但维度桥接必须产出 id 一致的引擎实例；对不上时调用失败，不会把玩家送进一个对不上的维度。

## e98e8b57e1

> Send a message of a specific TextPacketType to one player (additive,
> gated by struct_size). `type` is a TextPacketType value:
> 0 Raw · 1 Chat · 2 Translate · 3 Popup · 4 JukeboxPopup · 5 Tip ·
> 6 SystemMessage · 7 Whisper · 8 Announcement · 9 TextObjectWhisper ·
> 10 TextObject · 11 TextObjectAnnouncement.
> Out-of-range falls back to Raw. Single-string body (like LSE tell): the
> author/param kinds (Chat/Whisper/Translate) arrive as plain text.
> plain `player_send_message` remains the Raw/Chat convenience path.

给一名玩家发送指定 `TextPacketType` 的消息（追加的槽位，受 `struct_size` 约束）。`type` 是 `TextPacketType` 的值：0 Raw · 1 Chat · 2 Translate · 3 Popup · 4 JukeboxPopup · 5 Tip · 6 SystemMessage · 7 Whisper · 8 Announcement · 9 TextObjectWhisper · 10 TextObject · 11 TextObjectAnnouncement。超出范围时按 Raw 处理。消息体只有一个字符串（和 LSE 的 tell 一样）：需要作者或参数的类型（Chat、Whisper、Translate）收到的是纯文本。普通的 `player_send_message` 仍然是发 Raw 或 Chat 消息的便捷路径。

## b09f2c42bc

> Player: equipment, cooldown, network (dedicated fns)

玩家：装备、冷却、网络（专用函数）

## f1d91d625c

> All equipment as SNBT: [{slot, item_snbt},…] slot: 0=mainhand 1=offhand 2-5=armor

全部装备，以 SNBT `[{slot, item_snbt},…]` 给出。`slot`：0 为主手，1 为副手，2 到 5 为盔甲。

## 0dd94c881c

> Ticks remaining for an item cooldown (-1 if not on cooldown / player offline).

一个物品的冷却还剩多少刻（不在冷却中或者玩家不在线时为 -1）。

## 13e7b0daba

> Full Actor::save NBT as SNBT.

完整的 `Actor::save` NBT，以 SNBT 给出。

## e53d32b71f

> Spawn a mob (Spawner::spawnMob); on success *out = its ActorUniqueID.

生成一个生物（`Spawner::spawnMob`）；成功时 `*out` 是它的 `ActorUniqueID`。

## 3dee7721fb

> Actor: relationships, equipment, effects, geometry (dedicated fns)

实体：关系、装备、效果、几何（专用函数）

## 4efeeac9b6

> slot: 0=mainhand 1=offhand 2=helmet 3=chestplate 4=leggings 5=boots

`slot`：0=主手，1=副手，2=头盔，3=胸甲，4=护腿，5=靴子

## 2cd3873d1b

> SNBT [{id, ticks, amplifier, visible},…]

SNBT `[{id, ticks, amplifier, visible},…]`

## 831e54ba61

> flag_index: ActorFlags enum value (0-based).

`flag_index`：`ActorFlags` 枚举的值（从 0 开始）。

## 34316648be

> SNBT {type:"entity"|"block"|"none", pos:[x,y,z], entity_id?, block_name?}

SNBT `{type:"entity"|"block"|"none", pos:[x,y,z], entity_id?, block_name?}`

## f9a657f015

> SNBT {min:[x,y,z], max:[x,y,z]}

SNBT `{min:[x,y,z], max:[x,y,z]}`

## 265ef68060

> §D blocks & block entities

§D 方块与方块实体

## d61f0ab2cf

> BlockActor::save (with default SaveContext) as SNBT; false if none there.

`BlockActor::save`（使用默认的 `SaveContext`）的结果，以 SNBT 给出；那个位置没有方块实体时返回 false。

## 1f2268a2d2

> Block: state get/set, collision shape (dedicated fns)

方块：读写状态、碰撞形状（专用函数）

## 072acab6d4

> §E items (SNBT value objects) & containers

§E 物品（SNBT 值对象）与容器

## 35bbb3d128

> Rebuild → mutate → serialize; out receives the NEW item SNBT.

先重建物品，再修改，最后序列化；`out` 收到的是**新的**物品 SNBT。

## b085129470

> Slot content as item SNBT (empty slots yield the air item's SNBT).

这一格的内容，以物品 SNBT 给出（空格子给出空气物品的 SNBT）。

## c98d6f72d8

> SNBT [{id, level},…]

SNBT `[{id, level},…]`

## 5662b22888

> enchants_snbt = [{id, level},…]; out = new item SNBT. NULL on every
> current host: writing enchantments is not implemented, and
> item_get_enchants is the read side.

`enchants_snbt` 形如 `[{id, level},…]`；`out` 收到新的物品 SNBT。当前所有宿主上这个槽位都是 NULL：写附魔还没有实现，读取一侧是 `item_get_enchants`。

## f03854d0e5

> Resend a player-owned container (which 0..3) to its owner. Returns
> false for block containers (which == 4) — a chest has no single owner
> to resend to; its viewers are refreshed by the engine's own container
> transaction path.

把玩家自己的容器（`which` 为 0 到 3）重新发给它的主人。方块容器（`which == 4`）返回 false：箱子没有唯一的主人可以重发，正在看它的玩家由引擎自己的容器事务流程刷新。

## 4692eaf9f0

> §F scoreboard

§F 计分板

## 5e9ba1160a

> KvDb: THREAD-SAFE (internal mutex). Paths are confined to the mod's
> own data directory; ".." and absolute paths are rejected. Handles are
> owned by the loader and force-closed (with a warning) at mod unload.

KvDb：线程安全（内部有互斥锁）。路径限定在模组自己的数据目录里，带 `..` 的路径和绝对路径会被拒绝。句柄归加载器所有，模组卸载时会被强制关闭，并记一条警告。

## 6e6b5896c0

> Balance. Returns -1 on failure (empty xuid, database error, or absent
> backend); a real balance is never negative, so < 0 means "cannot say".
> Note that it opens an account at the configured default for an unseen
> xuid, so this is not a side-effect-free read.

余额。失败时返回 -1（xuid 为空、数据库出错，或者后端不在）；真实的余额不会是负数，所以小于 0 表示「说不出来」。注意：遇到没见过的 xuid，它会按配置的默认值开一个账户，所以这次读取是有副作用的。

## b053527a18

> Set to money, which is a target balance rather than a delta.

把余额设为 `money`。它是目标余额，不是差额。

## 706695ba1b

> An empty from or to means created from or destroyed into nothing. What the
> payee receives is reduced by pay_tax (see above). from == to fails.

`from` 或 `to` 为空，表示钱是凭空生出的，或者转出后就消失了。收款方收到的金额会按 `pay_tax` 扣减（见上面的说明）。`from == to` 会失败。

## 630bb6aeb0

> Raw per-connection packet send (additive, gated by struct_size) — the
> generic primitive spawn_particle_for derives from.
> `packet_id` is a MinecraftPacketIds value; `body`/`body_len` is the
> packet's wire-format body for the CURRENT game version. The bridge
> deserializes it into a real packet object (MinecraftPackets::createPacket
> + Packet::read) and delivers it to the resolved player's connection only.
> False if: player offline, unknown/unconstructible id, body fails to
> parse, or bytes are left over after parsing (wrong shape for this
> version). ESCAPE HATCH: the wire format is version-specific and is the
> caller's responsibility; prefer typed entries when one exists.

按连接发送原始数据包（追加的槽位，受 `struct_size` 约束），`spawn_particle_for` 就是在它之上实现的。`packet_id` 是 `MinecraftPacketIds` 的值；`body`/`body_len` 是这个数据包在**当前**游戏版本下的线上格式。bridge 把它反序列化成真正的数据包对象（`MinecraftPackets::createPacket` 加 `Packet::read`），只发给解析出来的那名玩家的连接。

以下情况返回 false：玩家不在线；id 不认识或构造不出来；包体解析失败；解析完以后还剩字节（形状不符合这个版本）。这是一个**逃生口**：线上格式随版本变化，正确与否由调用方负责；有带类型的接口时请优先用那些。

## da1929bdaf

> Unregister. Safe to call from inside the callback.

注销。在回调内部调用也是安全的。

## 25d2efb84e

> Simulated ("fake") players (additive, gated by struct_size).
> sim_spawn creates a real ServerPlayer with that name — every existing
> per-player entry (teleport, health, inventory, kick,…) works on it via
> the usual name selector. sim_do multiplexes the simulate* verb family:
> the action vocabulary grows bridge-side without new table slots
> (verbs: despawn stop jump attack interact use_item drop respawn
> move_to navigate_to look_at destroy_block destroy_look stop_destroy
> interact_block sneak fly chat — args as SNBT, see docs). Gated on
> isSimulatedPlayer(): a real player can never be puppeted. False on
> unknown verb, malformed args, offline/non-sim target.

模拟（「假」）玩家（追加的槽位，受 `struct_size` 约束）。`sim_spawn` 用那个名字创建一个真正的 `ServerPlayer`，现有的每一个按玩家操作的接口（传送、生命值、物品栏、踢出……）都能通过普通的名字选择器作用在它身上。`sim_do` 复用 `simulate*` 这一族动作：动作的种类在 bridge 一侧增加，不需要新的表槽位（动作有 `despawn stop jump attack interact use_item drop respawn move_to navigate_to look_at destroy_block destroy_look stop_destroy interact_block sneak fly chat`，参数用 SNBT，见文档）。以 `isSimulatedPlayer()` 把关：真实玩家永远不会被操纵。动作不认识、参数格式不对、目标不在线或者不是模拟玩家时返回 false。

## aaa89bc904

> True if the selector resolves to a live simulated player. Lets a mod
> re-validate a bot after a restart (the SimulatedPlayer persists in the
> world, but in-memory handles don't).

选择器能解析到一个活着的模拟玩家时返回 true。模组可以借此在重启后重新确认一个机器人：`SimulatedPlayer` 会保存在世界里，内存里的句柄不会。

## ac9c845d0a

> Enumerate the names of all live simulated players (sink receives each
> name). Rebuild a handle from a name to drive a bot that outlived the
> session that spawned it.

列出所有活着的模拟玩家的名字（输出回调逐个收到名字）。用名字重建句柄，就能操纵一个比生成它的那次会话活得更久的机器人。

## 44f82a7038

> Local player's name via ll::service::getClientInstance()->getLocalPlayer().
> sink receives the name, or the call returns false if not in a level.

通过 `ll::service::getClientInstance()->getLocalPlayer()` 取本地玩家的名字。输出回调收到名字；不在世界里时调用返回 false。

## ce3f942694

> True when the client is inside a level (a world is loaded).

客户端在世界里（已经加载了一个世界）时为 true。

## 14e4877ed6

> Current screen / UI name (e.g. "hud_screen", "pause_screen"). NULL on
> every current host, client builds included: the engine exposes no
> stable accessor for it.

当前界面的名字（例如 `"hud_screen"`、`"pause_screen"`）。当前所有宿主上这个槽位都是 NULL，客户端构建也一样：引擎没有提供稳定的访问方式。

## b18a8760c8

> Unregister a key binding: this mod's handler stops firing and the handle
> is freed. The ll::input::KeyHandle itself is not destroyed, since the key
> registry keeps one key per name and offers no removal; registering the
> same name again reuses that key.

取消一个按键绑定：这个模组的处理函数不再触发，句柄被释放。`ll::input::KeyHandle` 本身不会被销毁，因为按键注册表每个名字只保留一个按键，也没有提供删除的办法；用同一个名字再注册时会复用那个按键。

## cf861ab141

> Currently assigned key codes (may differ from defaults if remapped).
> sink receives a JSON-style array string "[1,2,3]".

当前分配的按键码（被玩家重新映射过时可能和默认值不同）。输出回调收到一个 JSON 风格的数组字符串 `"[1,2,3]"`。

## 6acc1822b4

> Whether this host can register custom dimensions. NULL when pier-dimensions
> was not built in. Filled in, it answers from a probe of the engine's dimension
> definition table, so it can be false on an engine whose layout this build does
> not match; the answer is cached after the first call that can make it.

这个宿主能不能注册自定义维度。没有编入 pier-dimensions 时这个槽位是 NULL。有这个槽位时，它的回答来自对引擎维度定义表的一次探测，所以在内存布局和这次构建对不上的引擎上可能回答 false；第一次能给出回答之后，结果会被缓存。

## 37cdf4baa5

> Read back a rule. `outAllow` is only written when the dimension has an
> explicit entry for that rule; returns false otherwise. A rule number this
> host does not know is answered false as well, and md_set_dimension_rule
> ignores one, so a binding newer than the host cannot tell "not set" from
> "not supported" here.

读回一条规则。只有这个维度对这条规则有显式设置时才写入 `outAllow`，否则返回 false。宿主不认识的规则编号也回答 false，而 `md_set_dimension_rule` 会忽略这样的编号，所以比宿主新的绑定在这里分不清「没有设置」和「不支持」。

## 4a7f7fe7e5

> Drop every rule for a dimension (used when a world is deleted).

删除一个维度的所有规则（删除世界时使用）。

## f544eeff56

> What a pack asks for, without registering anything: the sink receives one
> JSON document, the same shape `pier-pack inspect` prints:
> {"ok":true,"kind":"template","pack":"...","sha256":"...","name":"...",
> "biome":"...","height":{"min":-512,"max":320,"fixed":false},
> "params":[{"name":"plot_size","kind":"free","default":64,"min":4,
> "max":512,"step":1},
> {"name":"wall_style","kind":"choice","default":0,"choices":[0,1]},
> {"name":"plot_depth","kind":"derived"},
> {"name":"size","kind":"fixed","value":256}],
> "roles":[{"name":"floor","default":"minecraft:grass_block"}],
> "zones":["interior","border","road"],
> "constraints":["..."], "shapes":23, "picks":1,
> "voxels":[{"size":[5,4,3],"palette":5}], "confine":true}
> A mod builds its form from "params": free is a slider, choice a list, fixed a
> read-only value, derived is not shown. On a refusal the sink receives
> {"ok":false,"status":<code>,"problems":["..."]} and the code is returned.
> Returns 0 or a PIER_PACK_* code. Server thread only.

查看一个包要求什么，不注册任何东西。输出回调收到一个 JSON 文档，形状和 `pier-pack inspect` 打印的一样：

```text
{"ok":true,"kind":"template","pack":"...","sha256":"...","name":"...",
"biome":"...","height":{"min":-512,"max":320,"fixed":false},
"params":[{"name":"plot_size","kind":"free","default":64,"min":4,
"max":512,"step":1},
{"name":"wall_style","kind":"choice","default":0,"choices":[0,1]},
{"name":"plot_depth","kind":"derived"},
{"name":"size","kind":"fixed","value":256}],
"roles":[{"name":"floor","default":"minecraft:grass_block"}],
"zones":["interior","border","road"],
"constraints":["..."], "shapes":23, "picks":1,
"voxels":[{"size":[5,4,3],"palette":5}], "confine":true}
```

模组按 `"params"` 构造表单：`free` 用滑块，`choice` 用列表，`fixed` 是只读的值，`derived` 不显示。被拒绝时，输出回调收到 `{"ok":false,"status":<code>,"problems":["..."]}`，并返回这个代码。返回 0 或某个 `PIER_PACK_*` 代码。只能在服务器线程调用。

## 312cf5f379

> Drop one of this mod's subscriptions. Scoped to the caller — a mod
> cannot unsubscribe another mod. Returns true if one was removed.
> Safe to call from inside a callback (including one's own).

删除这个模组的一个订阅。只作用于调用方自己，一个模组不能取消另一个模组的订阅。真的删除了一个订阅时返回 true。在回调内部（包括自己的回调里）调用也是安全的。

## 3bc29e4ad6

> Deliver `payload` to every *other* mod subscribed to `topic`. Returns
> how many subscribers actually ran (0 is normal — nobody is listening).
> Return values from subscribers are ignored.

把 `payload` 投递给订阅了 `topic` 的每一个**其他**模组。返回实际运行了多少个订阅者（0 是正常的，表示没有人在听）。订阅者的返回值被忽略。

## d376cfc739

> As above, but collects the veto bit: returns true when any
> subscriber returned true. Every subscriber still runs — no
> short-circuit — so observers see a consistent stream whether or not an
> earlier one refused. `out_delivered` may be NULL.

同上，但会收集否决位：任何一个订阅者返回 true，就返回 true。每个订阅者照样都会运行，不会提前结束，所以无论前面有没有订阅者拒绝，观察者看到的都是一样完整的消息流。`out_delivered` 可以为 NULL。

## ad491da260

> How many subscribers a topic has right now, across all mods. Intended
> for skipping the cost of building a payload nobody will read.

一个主题当前有多少订阅者，所有模组加在一起。用来在没人会读的时候，省掉构造载荷的开销。

## 78d1309208

> Drop one of this mod's registrations. Scoped to the caller — a mod
> cannot unregister another mod's service.

删除这个模组的一项注册。只作用于调用方自己，一个模组不能注销另一个模组的服务。

## 2ac274009a

> Call `name` with `request`; the provider's answer arrives through
> `reply`. Returns one of PIER_SERVICE_*. A mod cannot call its own
> service (it has a direct function call, and self-calls are the least
> legible loop shape).

用 `request` 调用名为 `name` 的服务，提供方的回答通过 `reply` 送回。返回某个 `PIER_SERVICE_*`。模组不能调用自己的服务：它可以直接调用自己的函数，而自己调用自己是最难看清的一种循环。

## 71ba076366

> Every registered service as a JSON array of `{"name":…,"mod":…}`.
> For diagnostics and for a caller deciding whether to build a request
> nobody can answer.

所有已注册的服务，以 JSON 数组给出，每一项是 `{"name":…,"mod":…}`。用于诊断，也方便调用方在构造请求之前先确认有没有人能回答。

## 1831114b12

> Withdraw a lane owned by this mod. Calls release for every outstanding
> lease and clears the liveness flag, so a consumer's next check sees the
> lane gone instead of jumping through a dead pointer.

撤回这个模组拥有的一个快速通道。对每一个还没归还的租约调用 `release`，并清除存活标记，这样使用方下一次检查就会发现通道已经没了，不会再通过一个失效的指针跳转。

## fea482ce7e

> Return a lease. Only a lease held by this mod. Returns false when the
> provider is already gone, since the loader has called release for it by
> then and calling again would be a double free.

归还一个租约，只能是这个模组持有的租约。提供方已经不在时返回 false：那时加载器已经替它调用过 `release`，再调用一次就是重复释放。

## 00ce97d68b

> Every lane, as a JSON array:
> [{"name":…,"mod":…,"fingerprint":"0x…","protocol":1,"leases":N,"alive":true}]

所有快速通道，以 JSON 数组给出：`[{"name":…,"mod":…,"fingerprint":"0x…","protocol":1,"leases":N,"alive":true}]`

## 9490beda0a

> Pier ABI — sdk/abi.h (ABI v2)
>
> This header is the product: the sole contract between the C++ host
> (pier-host plus the capability packages) and an SDK written in any language.
> The reference mirror is the pier-sys-rs crate under bindings/, hand-written with no
> bindgen, which doubles as readable annotation for this file.
>
> This file must parse as C. Consumers are "any language", so it uses C11 only:
> no std::string_view, no enum class, no nested types. The C++ convenience
> wrappers (PierStr to string_view and back) live in pier-support, not here; a
> language-specific type in the contract forces every other language to guess
> that type's layout. CI compiles this file once as C11 and once as C++20.
>
> Rules for changing this file. These are the only versioning rules anywhere.
>   1. Append at the end of PierApi only. Never reorder, remove, or change the
>      signature of an existing slot. Appending does NOT bump PIER_ABI_VERSION.
>   2. After appending, update every SDK mirror slot for slot; the
>      sys-mirrors-abi check enforces the ordering.
>   3. Only a non-append change (reorder, removal, signature change) advances
>      PIER_ABI_VERSION and PIER_ABI_MIN_SUPPORTED, both to the same number.
>
> Appending does not bump the version because the version answers "which
> already-compiled mods still load". An appended slot invalidates no old mod:
> the old table is a byte-identical prefix of the new one, and an old mod can
> never reach the new slot. Bumping for it would announce an incompatibility
> that does not exist. Each direction has its own gate instead: a new host with
> an old mod is covered by the version range (see PIER_ABI_MIN_SUPPORTED); an
> old host with a new mod is covered by the mod comparing struct_size slot by
> slot, reporting "host lacks this capability" for the one call that overruns.
>
> The layout is identical across all build targets. PierApi carries no
> conditional compilation: slots for client-only and dimension capabilities are
> always present in the layout and are simply NULL when that package was not
> built into the host. "Capability present" means "slot is non-NULL", and the
> SDK reports "host does not provide X" from that. This buys three things:
> mirrors need no conditional compilation, a cross-target mismatch cannot call
> the wrong slot, and the struct has exactly one append point, the end.
>
> Conventions for the whole file; per-slot comments record only the exceptions.
>   - Strings are UTF-8 (ptr, len) views and are NOT guaranteed NUL-terminated.
>   - A string passed into a callback is owned by the caller and valid only for
>     that call; copy it to keep it.
>   - A mod hands strings out through a sink callback within the current call
>     frame. Ownership never crosses the boundary: this ABI has no "returns a
>     pointer the other side must free".
>   - Threading: unless a slot says otherwise, call only on the server thread.
>     log, gaming_status, schedule and schedule_after are thread-safe. Every
>     callback (event, command, scheduled task) fires on the server thread.

**Pier ABI：sdk/abi.h（ABI v2）**

这个头文件就是产品本身：它是 C++ 宿主（pier-host 加上各个能力包）和任何语言写的 SDK 之间唯一的契约。参照的镜像是 bindings/ 下的 pier-sys-rs crate，手写，没有用 bindgen，它也可以当作这个文件的一份可读注解。

这个文件必须能当 C 解析。使用它的是「任何语言」，所以它只用 C11：没有 `std::string_view`，没有 `enum class`，没有嵌套类型。C++ 的便利封装（`PierStr` 和 `string_view` 互相转换）放在 pier-support 里，不在这里；契约里一旦出现某种语言特有的类型，其他所有语言都得去猜那个类型的内存布局。CI 会把这个文件按 C11 和 C++20 各编译一次。

修改这个文件的规则，整个项目里只有这几条版本规则：

1. 只能在 `PierApi` 的末尾追加。不能调整已有槽位的顺序，不能删除，不能改签名。追加**不**提升 `PIER_ABI_VERSION`。
2. 追加之后，逐个槽位更新所有 SDK 镜像；`sys-mirrors-abi` 检查会核对顺序。
3. 只有非追加的改动（调整顺序、删除、改签名）才提升 `PIER_ABI_VERSION` 和 `PIER_ABI_MIN_SUPPORTED`，两者提升到同一个数。

追加不提升版本，因为版本号回答的问题是「哪些已经编译好的模组还能加载」。追加的槽位不会让任何旧模组失效：旧表是新表逐字节相同的前缀，旧模组也永远够不着新槽位。为追加提升版本，等于宣布一个并不存在的不兼容。两个方向各有各的关卡：新宿主配旧模组，由版本范围把关（见 `PIER_ABI_MIN_SUPPORTED`）；旧宿主配新模组，由模组逐个槽位比较 `struct_size` 把关，对超出表长的那一次调用报告「宿主缺少这项能力」。

所有构建目标上的内存布局都一样。`PierApi` 里没有条件编译：仅客户端的能力和维度能力对应的槽位始终在布局里，宿主没有编入那个包时，槽位就是 NULL。「有这项能力」的意思就是「槽位不为 NULL」，SDK 据此报告「宿主没有提供 X」。这样做换来三点：镜像不需要条件编译；目标不匹配时不会调到错的槽位；结构体只有一个追加点，就是末尾。

整个文件通用的约定，每个槽位的注释只记录例外：

- 字符串是 UTF-8 的 (ptr, len) 视图，**不**保证以 NUL 结尾。
- 传进回调的字符串归调用方所有，只在这次调用期间有效；要保留就复制一份。
- 模组在当前调用帧内通过输出回调把字符串交出去。所有权从不跨越边界：这个 ABI 里没有「返回一个指针，由另一边释放」的写法。
- 线程：除非槽位另有说明，只能在服务器线程调用。`log`、`gaming_status`、`schedule` 和 `schedule_after` 是线程安全的。所有回调（事件、命令、定时任务）都在服务器线程上触发。

## 92aa109203

> World reads (scan)

读取世界（扫描）

## 95c6deda90

> Per-domain payload types

各个领域的数据类型

## 34691157fc

> Cross-mod event bus FFI types
> A mod cannot hand another mod a function pointer: `ModHost::unload`
> calls FreeLibrary, so the publisher would be left holding a pointer into an
> unmapped dylib. The loader therefore owns the subscription table, with the
> same weak_ptr + ticket discipline as Forms.cpp and the mod-scoped scheduler.
>
> The loader never parses `payload` — it is opaque UTF-8 (JSON, SNBT, or
> anything else the two mods agree on). Keeping the loader format-agnostic is
> deliberate: the alternative is a schema that every publisher has to satisfy
> and that the loader has to version.
>
> Topics are plain strings; namespace them (`plot:enter`, not `enter`).

**跨模组事件总线的 FFI 类型**

一个模组不能把函数指针交给另一个模组：`ModHost::unload` 会调用 `FreeLibrary`，发布方手里就会留下一个指向已卸载 dylib 的指针。所以订阅表由加载器持有，做法和 Forms.cpp、按模组归属的调度器一样，用 `weak_ptr` 加票据。

加载器从不解析 `payload`：它是不透明的 UTF-8，可以是 JSON、SNBT，或者两个模组约定的任何格式。加载器不关心格式是有意的，另一种做法是定一套每个发布方都要满足、加载器还得维护版本的格式规范。

主题就是普通的字符串，请加上命名空间：写 `plot:enter`，不要写 `enter`。

## c076447e45

> Same-toolchain fast lane
>
> bus and service are both "(name, UTF-8 payload) -> UTF-8 payload". That shape
> is the cross-language common denominator: a mod in any language can speak it.
> The price is a serialization round trip per call, with all type information
> lost inside the string.
>
> This lane serves one special case: both sides built by the same toolchain, so
> the C-layout function tables in the two dynamic libraries are byte-identical
> and pointers can be handed over directly.
>
> The loader owns the name -> lane table (exclusive, like service), validates
> the fingerprint, issues and collects leases, and holds a liveness flag that it
> clears the moment the provider goes away. It does not interpret a single byte
> of data or vtable; both pointers are opaque to it, exactly like a bus payload.
>
> The loader has to be involved because ModHost::unload calls FreeLibrary. The
> provider's memory can stay alive by reference counting, but its code section
> is unmapped, so the consumer's function pointer becomes a use-after-free. The
> crash then lands in the consumer, with nothing in the log pointing at the mod
> that just left. Hence:
>   1. alive points at one cell on the loader's own heap and is never freed
>      (lanes number in the dozens, so this leaks a few dozen uint32). The
>      loader writes 0 when the provider goes away, and the consumer reads the
>      cell before each call: one plain atomic read, no FFI, no lock. That is
>      what "fast" means here, the loader runs no code on the hot path.
>   2. When the provider goes away, the loader calls release for every
>      outstanding lease before FreeLibrary, so the provider frees its own
>      objects inside its own dylib with its own allocator.
>
> Most native languages have no stable ABI. The same contract type compiled
> twice into two cdylibs can end up with different field order when compiler
> metadata differs, and that is silent memory corruption rather than a crash.
> So the check is a fingerprint, not a version number: compiler version, target
> triple, contract name and version, and the type identity, size and alignment
> of the function table, all folded into one u64. Any difference yields a
> different fingerprint, lane_acquire returns PIER_LANE_FINGERPRINT, and not a
> single pointer is handed over.
>
> The failure mode is "slow" (the consumer falls back to the service channel),
> never undefined behavior. That property is the entire reason this lane is
> allowed to exist.
>
> The host compares fingerprints for equality and never interprets them; it has
> to be that way, or "add one more item to the fingerprint" would become an ABI
> change.

**同工具链快速通道**

总线和服务都是「(名字, UTF-8 载荷) -> UTF-8 载荷」这个形状。这是各种语言都能用的公共形状，任何语言写的模组都能用它通信。代价是每次调用都要序列化一个来回，类型信息全部埋在字符串里。

快速通道只服务一种特殊情况：两边由同一个工具链构建，两个动态库里按 C 布局的函数表逐字节相同，指针可以直接交过去。

加载器持有「名字 -> 通道」的表（和服务一样是独占的），核对指纹，发放和回收租约，并持有一个存活标记，提供方一离开就把它清零。数据和函数表它一个字节都不解读，两个指针对它都是不透明的，和总线的载荷一样。

加载器必须参与，是因为 `ModHost::unload` 会调用 `FreeLibrary`。提供方的内存可以靠引用计数继续存在，但它的代码段会被卸载，使用方手里的函数指针就成了释放后使用。崩溃会发生在使用方那里，日志里没有任何内容指向刚刚离开的那个模组。因此：

1. `alive` 指向加载器自己堆上的一个单元，永不释放（通道只有几十个，泄漏的是几十个 `uint32`）。提供方离开时加载器在里面写 0，使用方每次调用前先读这个单元：一次普通的原子读，没有 FFI，没有锁。这里说的「快」就是指热路径上加载器不运行任何代码。
2. 提供方离开时，加载器在 `FreeLibrary` 之前对每一个还没归还的租约调用 `release`，让提供方在自己的 dylib 里、用自己的分配器释放自己的对象。

大多数原生语言没有稳定的 ABI。同一个契约类型编译进两个 cdylib，编译器元数据不同时字段顺序可能不同，结果是没有任何报错的内存损坏，连崩溃都没有。所以检查用的是指纹，不用版本号：编译器版本、目标三元组、契约的名字和版本，以及函数表的类型标识、大小和对齐，全部折叠成一个 u64。任何一项不同，指纹就不同，`lane_acquire` 返回 `PIER_LANE_FINGERPRINT`，一个指针都不交出去。

失败时的结果是「变慢」（使用方退回到服务通道），永远不会是未定义行为。这个通道之所以被允许存在，正是因为这一点。

宿主只比较指纹是否相等，从不解读指纹的内容；必须这样，否则「在指纹里多加一项」就会变成一次 ABI 改动。

## f5ef710847

> Packet interception FFI types
> Used by packet_hook_register / packet_conn_hook_register. See the block
> comment on those fields in PierApi for the full contract.

**数据包拦截的 FFI 类型**

供 `packet_hook_register` 和 `packet_conn_hook_register` 使用。完整的约定见 `PierApi` 里这两个字段上方的注释。

## b3ba7ad032

> FFI types for the client capability group. The type declarations are always
> present (they take no layout); whether the capability is available is decided
> by whether the client_* slots in PierApi are NULL.

客户端能力组的 FFI 类型。这些类型声明始终存在（它们不占布局）；能力是否可用，看 `PierApi` 里的 `client_*` 槽位是不是 NULL。

## b63d5fdd47

> Property and action keys. APPEND-ONLY: never renumber or remove. Unknown
> values make the call return false; a safe SDK layer maps that to an
> "unsupported" error, which is the forward-compatibility negotiation.

属性和动作的键。**只能追加**：不能重新编号，也不能删除。不认识的值会让调用返回 false；安全的 SDK 层把它转成「不支持」的错误，向前兼容就是这样协商的。

## d9a930e073

> the core slots, present since ABI v1

核心槽位，从 ABI v1 起就有

## 3729149211

> Append tail, struct_size-gated.

末尾追加区，受 `struct_size` 约束

## e15fe3af21

> The struct's only append point. SDK mirrors declare every field
> unconditionally, with no cfg or ifdef branches, because the layout is the
> same on every target.
>
> Mod-scoped scheduling.
> `schedule` / `schedule_after` above take a bare callback with no owner.
> That is a use-after-free waiting to happen: a mod that schedules a task
> and is then unloaded leaves the executor holding a function pointer into
> a freed dylib. These replacements attribute each task to a mod, so the
> loader can drop still-pending tasks when that mod goes away — the same
> weak_ptr + ticket discipline the form callbacks already use.
>
> The old slots remain (ABI is additive) and still work. The loader now
> attributes them by the callback's module (address to DLL) and drops
> pending tasks at unload; mods should still prefer the owned slots below,
> because attribution by address cannot see a callback that lives in a
> different module.

结构体唯一的追加点。SDK 镜像无条件地声明每一个字段，不写 cfg 或 ifdef 分支，因为所有目标上的布局都一样。

**按模组归属的调度。** 上面的 `schedule` / `schedule_after` 接收的是没有主人的裸回调。模组排了一个任务然后被卸载，执行器手里就留着一个指向已释放 dylib 的函数指针，任务一执行就是释放后使用。下面这些替代的槽位把每个任务记在某个模组名下，这个模组离开时，加载器可以丢掉它还没执行的任务；做法和表单回调已经在用的一样，用 `weak_ptr` 加票据。

旧槽位仍然保留（ABI 只追加），也照样能用。加载器现在按回调所在的模块（由地址找到 DLL）给它们归属，卸载时丢掉还没执行的任务。模组仍然应当优先使用下面这些带归属的槽位，因为按地址归属看不到放在另一个模块里的回调。

## 47a9d09bbc

> sizeof(PierApi), filled in by the host from the table it compiled. This is
>  the whole basis of forward compatibility: the SDK compares against it at
>  every non-core slot's call site.

`sizeof(PierApi)`，由宿主按它编译时的表填入。向前兼容完全建立在它上面：SDK 在每个非核心槽位的调用处都拿它来比较。

## d11a3d6699

> Equals the host's PIER_ABI_VERSION.

等于宿主的 `PIER_ABI_VERSION`。

## b1c271941e

> Bitwise OR of PIER_FLAG_*. Bit 0 means a client build.

`PIER_FLAG_*` 的按位或。第 0 位表示客户端构建。

## d6795847ac

> Reserved, always 0. Rounds the header out to 16 bytes and leaves room for
>  future header scalars.

保留，始终为 0。它把表头凑成 16 字节，也为以后的表头标量留出位置。

## 51e0e86a5f

> Queue a task onto the server thread ASAP. Thread-safe.

把一个任务排到服务器线程上，尽快执行。线程安全。

## 6c6b67058a

> Queue a task onto the server thread after `delay_ms`. Thread-safe.

把一个任务排到服务器线程上，`delay_ms` 毫秒后执行。线程安全。

## 0a4066e7d8

> Run `cb(user)` on the server (or client) thread ASAP, owned by `mod`.
>  Thread-safe. Returns a task id (>0), or 0 if the task was rejected.
>  If `mod` unloads before the task runs, the task is dropped and `cb` is
>  never called — `user` is then leaked by design, because the only code
>  that could free it lives in the dylib that just went away.

在服务器（或客户端）线程上尽快运行 `cb(user)`，任务归 `mod` 所有。线程安全。返回任务 id（大于 0），任务被拒绝时返回 0。如果 `mod` 在任务运行之前卸载，任务会被丢弃，`cb` 永远不会被调用；这时 `user` 会泄漏，这是有意的，因为唯一能释放它的代码就在刚刚离开的那个 dylib 里。

## a74c54e829

> As above, delayed by `delay_ms`. Thread-safe. Returns a task id (>0),
>  or 0 if rejected. The timer itself is not cancelled on unload — it
>  still expires — but the task is dropped when it does, so nothing calls
>  into the freed dylib.

同上，延迟 `delay_ms` 毫秒。线程安全。返回任务 id（大于 0），被拒绝时返回 0。卸载时计时器本身不会取消，它照样会到期，但到期时任务被丢弃，所以不会有调用进入已释放的 dylib。

## 2b1dad4a7f

> See "Rules for changing this file" in the file header. Appending a slot does
>  not touch this.

见文件头的「修改这个文件的规则」。追加槽位不改动它。

## 5528783d1a

> Oldest mod ABI the host accepts. Moves only on a non-append change, and then
>  to the same number as PIER_ABI_VERSION. It is the switch for "a table older
>  than this is no longer a prefix of mine".

宿主接受的最旧的模组 ABI。只在非追加的改动时才变，而且变到和 `PIER_ABI_VERSION` 相同的数。它是「比这更旧的表已经不是我的前缀」的开关。

## f9c23c6afb

> The only entry symbol a mod must export. The host looks for this name alone
>  and refuses to load with an explicit error if it is missing; there is no
>  fallback and no historical alias.

模组唯一必须导出的入口符号。宿主只找这个名字，找不到就拒绝加载，并给出明确的错误；没有退路，也没有历史别名。

## 2b50f1e4c8

> What to write in front of a pier_main definition so the name is actually exported.
>
>  Naming the symbol is not the same as exporting it. A Windows DLL exports nothing
>  unless asked, so a plainly declared pier_main compiles, links, and then fails to
>  load with "does not export pier_main". An ELF build exports it by default, which
>  makes this a mistake a test on one platform cannot catch for the other.
>
>      PIER_MAIN_EXPORT bool pier_main(const PierApi* api, PierModHandle self,
>                                      PierModVTable* out_vtable) { ... }
>
>  The macro carries the C linkage as well, so the definition needs no separate
>  extern block. An SDK that emits the entry point from a macro of its own does not
>  need this one.

要让 `pier_main` 这个名字真的被导出，定义前面要写的东西。

给符号起名和导出它是两件事。Windows 的 DLL 不要求就什么都不导出，所以直接声明的 `pier_main` 能编译、能链接，加载时却报「does not export pier_main」。ELF 构建默认会导出，所以在一个平台上的测试发现不了另一个平台上的这个错误。

```c
PIER_MAIN_EXPORT bool pier_main(const PierApi* api, PierModHandle self,
                                PierModVTable* out_vtable) { ... }
```

这个宏同时带上了 C 链接方式，所以定义不需要再单独写 extern 块。用自己的宏生成入口点的 SDK 不需要这个宏。

## 75077ca648

> Bits for PierApi.host_flags and PierModVTable.mod_flags. Bit 0 must match on
>  both sides or the host refuses to load and says why: a server host cannot
>  load a client-built mod, and vice versa. All other bits are reserved and must
>  currently be 0.

`PierApi.host_flags` 和 `PierModVTable.mod_flags` 的位。第 0 位两边必须一致，否则宿主拒绝加载并说明原因：服务器宿主不能加载为客户端构建的模组，反过来也一样。其他位都是保留位，目前必须为 0。

## 6607872627

> Opaque handle to the HostedMod instance managed by the loader.

指向加载器管理的 `HostedMod` 实例的不透明句柄。

## 78a783cc2a

> Generic "run this" callback.

通用的「执行这个」回调。

## df04615d2d

>
> Filled in by the mod inside pier_main. instance is the mod's own opaque
> pointer; the three callbacks may be NULL, which counts as always succeeding.
> This struct follows the same append rules as PierApi: with struct_size, new
> lifecycle callbacks can be added at the tail without a version bump, and the
> host calls one only if it can reach it.
>
> out_vtable points at no fewer than 512 zeroed bytes, so a mod writes the whole
> struct it compiled even when the host's is shorter. The host reads only the
> prefix it knows and accepts any struct_size that covers on_unload.

由模组在 `pier_main` 里填写。`instance` 是模组自己的不透明指针；三个回调都可以是 NULL，NULL 视为总是成功。这个结构体和 `PierApi` 遵守同样的追加规则：有了 `struct_size`，新的生命周期回调可以追加在末尾而不提升版本，宿主只调用它够得着的回调。

`out_vtable` 指向至少 512 个清零的字节，所以即使宿主的结构体更短，模组也可以写入它编译时的整个结构体。宿主只读取它认识的前缀，接受任何覆盖到 `on_unload` 的 `struct_size`。

## 8ad0886d2d

>
> The single symbol every mod must export:
>
>   bool pier_main(const PierApi* api, PierModHandle self,
>                     PierModVTable* out_vtable);
>
> Called once on the server thread while the mod is being loaded.
> Return false to abort loading.
>
> Neither this nor any callback a mod hands the host may unwind back into it:
> an exception, a panic or any other unwinding has to be stopped at the mod's own
> boundary. A callback that holds the thread past the host's watchdog limit is
> named in the log, and past the hang limit the host ends the process.

每个模组都必须导出的唯一符号：

```c
bool pier_main(const PierApi* api, PierModHandle self,
               PierModVTable* out_vtable);
```

模组加载时，在服务器线程上调用一次。返回 false 会中止加载。

无论是它，还是模组交给宿主的任何回调，都不能把栈展开带回宿主：异常、panic 或其他任何形式的展开，都必须在模组自己的边界上截住。一个回调占住线程超过宿主看门狗的告警时限时，日志里会点名；超过卡死的时限，宿主会结束进程。

## b5a95ed6a5

>
> Subscribe to a LeviLamina event by id (server thread only).
>   event_id : full id, e.g. "ll::event::PlayerChatEvent". If no exact
>              match exists, the loader falls back to a unique suffix
>              match ("PlayerChatEvent" works if unambiguous).
>   priority : 0..4 (Highest..Lowest), 2 = Normal
>              (mirrors ll::event::EventPriority).
> Returns NULL on failure (unknown/ambiguous id).

按 id 订阅一个 LeviLamina 事件（只能在服务器线程调用）。

- `event_id`：完整的 id，例如 `"ll::event::PlayerChatEvent"`。没有完全匹配时，加载器退回到唯一的后缀匹配，不产生歧义时写 `"PlayerChatEvent"` 也可以。
- `priority`：0 到 4（Highest 到 Lowest），2 为 Normal（对应 `ll::event::EventPriority`）。

失败（id 不认识或有歧义）时返回 NULL。

## f7e43dd096

> Opaque handle to an event listener.

事件监听器的不透明句柄。

## 89ea697ab2

>
> Event callback.
>   event_id : the full event id this listener fired for.
>   snbt     : event data serialized as SNBT (CompoundTag). For cancellable
>              events it contains a `cancelled` byte field.
>   write_ctx / write_back : to mutate the event (e.g. cancel it, edit the
>              chat message), call write_back(write_ctx, new_snbt) with the
>              modified SNBT before returning. The loader deserializes it
>              back into the event. Calling it zero times leaves the event
>              untouched; the last call wins.

事件回调。

- `event_id`：这个监听器响应的事件的完整 id。
- `snbt`：序列化成 SNBT（`CompoundTag`）的事件数据。可取消的事件里有一个 `cancelled` 字节字段。
- `write_ctx` / `write_back`：要修改事件（比如取消它、改聊天内容），在返回之前调用 `write_back(write_ctx, new_snbt)`，传入改过的 SNBT，加载器会把它反序列化回事件里。一次都不调用，事件保持原样；调用多次，以最后一次为准。

## 235cb1a775

> §H parameterized commands & enums

§H 带参数的命令与枚举

## c180b1b162

>
> Execute a command as the server console (permission: Owner) and collect
> its output. Server thread only. Returns false if the level is not ready.

以服务器控制台的身份（权限：Owner）执行一条命令，并收集它的输出。只能在服务器线程调用。世界还没就绪时返回 false。

## d6ec8c7453

>
> Register a custom command `/name [args: raw text]`.
>   permission: 0=Any,1=GameDirectors,2=Admin,3=Host,4=Owner
>               (mirrors CommandPermissionLevel).
> Call during on_enable, on the server thread. The command stays
> registered for the lifetime of the server (Bedrock cannot unregister
> commands); callbacks for disabled mods are muted by the loader.

注册一条自定义命令 `/name [args: 原始文本]`。

- `permission`：0=Any，1=GameDirectors，2=Admin，3=Host，4=Owner（对应 `CommandPermissionLevel`）。

在 `on_enable` 里、在服务器线程上调用。命令在服务器整个运行期间都保持注册（基岩版不能注销命令）；被禁用的模组，它的回调由加载器屏蔽。

## eef6c78cc8

>
> Like register_command, but with typed overloads. overloads_snbt:
>   {overloads:[[{name:"target",kind:"player",optional:0b},…],…]}
> kinds: int|bool|float|string|enum|soft_enum|actor|player|block_pos|vec3|
>        raw_text|message|json|item|block_name|effect|actor_type|command|
>        relative_float|file_path (enum/soft_enum also need "enum":"Name").
> The callback's `args` receives the parse result as SNBT
>   {overload:N, args:{<name>:…}}   and `origin_name` becomes origin SNBT
>   {name,type,dim,x,y,z}.

和 `register_command` 一样，但带有类型化的重载。`overloads_snbt` 的形状：

```text
{overloads:[[{name:"target",kind:"player",optional:0b},…],…]}
```

`kind` 可以是 `int|bool|float|string|enum|soft_enum|actor|player|block_pos|vec3|raw_text|message|json|item|block_name|effect|actor_type|command|relative_float|file_path`，其中 `enum` 和 `soft_enum` 还需要 `"enum":"Name"`。

回调的 `args` 收到 SNBT 形式的解析结果 `{overload:N, args:{<name>:…}}`，`origin_name` 则变成来源的 SNBT `{name,type,dim,x,y,z}`。

## c3457574bf

>
> Custom command callback.
>   args        : raw text following the command name (may be empty).
>   origin_name : display name of the command origin (player name / "Server").
>   out_success / out_error : call any number of times to emit output lines.

自定义命令的回调。

- `args`：命令名后面的原始文本，可能为空。
- `origin_name`：命令来源的显示名（玩家名或 `"Server"`）。
- `out_success` / `out_error`：每调用一次输出一行，次数不限。

## 91296667bf

> Output sink for execute_command: full command output + success flag.

`execute_command` 的输出回调：完整的命令输出，加上是否成功的标志。

## 04799c5d3f

> §I NBT binary, KvDb (thread-safe), system & server info

§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息

## c9043454e6

> Appended: tick statistics

追加：刻的统计

## e4f5de8e98

>
> Per-subsystem MSPT profiler (additive, gated by struct_size). Backed by
> five timing detours (Level/Dimension tick, redstone, chunk block ticks,
> block entities), installed lazily on the first profile_begin and left
> in place. One sampling window at a time. Server thread only.

按子系统统计 MSPT 的分析器（追加的槽位，受 `struct_size` 约束）。实现是五个计时钩子（Level 和 Dimension 的刻、红石、区块的方块刻、方块实体），第一次调用 `profile_begin` 时才安装，之后一直留着。同一时间只有一个采样窗口。只能在服务器线程调用。

## 504edf4073

>
> Poll for the finished report. False while sampling / nothing armed;
> true exactly once per window, sinking one SNBT report:
> {ticks:N, buckets:{level_tick:{us,calls}, dimension_tick:{…}, redstone:{…},
>  chunk_blocks:{…}, block_entities:{…}}}. Bucket times are INCLUSIVE
> (nested subsystems), report side by side, don't sum.
>
> chunk_blocks is the drain of a chunk's pending block-tick queues, both the
> scheduled and the random one, summed over every chunk. Work a chunk does
> around that drain is outside the bucket, so the number is a floor on block
> ticking and not the whole of it. calls counts queue drains, of which a
> ticking chunk contributes two, not one.

取回已经完成的报告。还在采样或者没有开启窗口时返回 false；每个窗口正好返回一次 true，并通过输出回调给出一份 SNBT 报告：

```text
{ticks:N, buckets:{level_tick:{us,calls}, dimension_tick:{…}, redstone:{…},
 chunk_blocks:{…}, block_entities:{…}}}
```

各个桶的时间是**包含**关系（子系统之间有嵌套），请并排对照着看，不要相加。

`chunk_blocks` 统计的是清空区块待处理方块刻队列的时间，计划刻和随机刻两个队列都算，所有区块加在一起。区块在清空队列前后做的其他工作不在这个桶里，所以这个数是方块刻耗时的下限，并非全部。`calls` 统计的是清空队列的次数，一个正在运行刻的区块贡献两次。

## 6c02af8e33

> server_info_str keys.

`server_info_str` 的键。

## 979f5a8239

> Appended

追加

## 5a16be3996

> §A world read/write & clock

§A 世界的读写与时钟

## 0ac8a2cd1b

> §C actors (players resolve here too, via player_resolve)

§C 实体（玩家也可以经 `player_resolve` 解析到这里）

## 6741517677

> Same-toolchain fast lane, appended and struct_size-gated.

同工具链快速通道，追加的槽位，受 `struct_size` 约束

## d8c00f0e54

> Five slots appended without touching PIER_ABI_VERSION: a pure append is
> not a version change, and struct_size is the precise gate.
>
> Both directions hold. A new loader running an old mod: the old table is a
> byte-identical prefix of the new one, the mod cannot reach these five
> slots, and it works unchanged. A new mod on an old loader: SDK runtime
> init compares struct_size, finds the loader's table shorter than the one
> it was compiled against, and refuses to load. That is the right outcome,
> since a mod that reads the lane_publish cell on a loader without it would
> read out of bounds.
>
> In short: the version number tracks "semantics changed", struct_size
> tracks "the table grew". This change is only the latter.
>
> See the long comment at PierLaneDesc above. In one line: service is the
> cross-language (name, JSON) -> JSON channel, while this is a direct
> function-table call that holds only when both sides were built by the same
> toolchain; a fingerprint mismatch yields no pointer and the consumer falls
> back to service.
>
> Server thread only.

追加了五个槽位，没有改动 `PIER_ABI_VERSION`：单纯的追加不算版本变化，`struct_size` 才是准确的关卡。

两个方向都成立。新加载器运行旧模组：旧表是新表逐字节相同的前缀，模组够不着这五个槽位，照常工作。新模组配旧加载器：SDK 运行时初始化时比较 `struct_size`，发现加载器的表比它编译时用的短，于是拒绝加载。这是正确的结果，因为在没有 `lane_publish` 的加载器上读那个单元会越界。

简单地说：版本号记录「语义变了」，`struct_size` 记录「表变长了」。这次改动只属于后者。

见上面 `PierLaneDesc` 处的长注释。一句话概括：服务是跨语言的 (名字, JSON) -> JSON 通道；快速通道直接调用函数表，只在两边由同一个工具链构建时成立；指纹对不上时一个指针都不交出，使用方退回到服务通道。

只能在服务器线程调用。

## e60f4c4704

> Appended slots. Added at the tail only, guarded by struct_size.

追加的槽位。只加在末尾，由 `struct_size` 把关

## 8bf3a0a846

>
> Removing an actor and healing one already exist as actor_action's
> AACT_DESPAWN and AACT_HEAL. A separate slot would do the same job twice,
> and two implementations eventually drift.
>
>
> Actor enumeration already exists as list_actors (everything in a dimension,
> with type names); combined with actor_get_num for positions it filters to a
> box. An actors_in_box slot would do the same job twice, and two
> implementations eventually drift.

移除实体和治疗实体已经由 `actor_action` 的 `AACT_DESPAWN` 和 `AACT_HEAL` 提供。再单独加一个槽位就是同一件事做两遍，两份实现时间一长就会出现差异。

列出实体已经有 `list_actors`（一个维度里的全部实体，带类型名）；配合 `actor_get_num` 读取坐标，就能筛出一个长方体里的实体。再加一个 `actors_in_box` 槽位也是同一件事做两遍，两份实现时间一长就会出现差异。

## 4ed9f6e5a2

> Appended: the liquid layer (waterlogged blocks).

追加：液体层（含水方块）

## 2397ceb50c

>
> In Bedrock, waterlogging is not a block state but a second block in the
> same cell: stairs, fences or coral in the main layer and water in the
> liquid layer. get_block and set_block see the main layer only, so copying
> and pasting waterlogged stairs loses all the water: the main layer is
> exactly right and the other layer is missing.
>
> These two slots expose the liquid layer. An empty layer reads back as
> "minecraft:air".

基岩版的含水用的是同一格里的第二个方块，没有对应的方块状态：主层放楼梯、栅栏或珊瑚，液体层放水。`get_block` 和 `set_block` 只看得到主层，所以复制、粘贴含水楼梯会丢掉所有的水：主层完全正确，另一层没了。

这两个槽位用来读写液体层。空的液体层读出来是 `"minecraft:air"`。

## 35d2e8856a

> Appended: bulk block reads and writes

追加：批量读写方块

## 84397bbbc1

>
> Scan a cuboid region, corners inclusive (order-independent). For every
> cell in the box, blocks_sink is called with the block name + full SNBT.
> For every entity whose position lies within the box, entities_sink is
> called with the containing cell and the entity's SNBT. Both sinks run
> synchronously within this call; nothing is retained afterwards.
> Server thread only. Returns false if the level/dimension is not ready.

扫描一个长方体区域，两个角都包含在内，顺序不限。长方体里的每一格调用一次 `blocks_sink`，传入方块名和完整的 SNBT；位置落在长方体里的每个实体调用一次 `entities_sink`，传入它所在的格子和它的 SNBT。两个回调都在这次调用里同步执行，调用结束后什么都不保留。只能在服务器线程调用。世界或维度还没就绪时返回 false。

## 5f06367e3f

> Read one block: sink called once with (x,y,z, type name, full SNBT).

读取一个方块：输出回调被调用一次，传入 (x,y,z, 类型名, 完整的 SNBT)。

## f735ab32f3

> Level::getDifficulty

取自 `Level::getDifficulty`

## 627bd9a371

> native Level::setDifficulty

原生调用 `Level::setDifficulty`

## 046f78ca75

> Level::getLevelSeed64

取自 `Level::getLevelSeed64`

## 381a72dfba

> /gamerule

等同于 `/gamerule`

## f0ecb76b7a

>
> Read-only world-data queries (additive, gated by struct_size). Both
> stream one SNBT object per result through the sink; observational only.
> Server thread only.

只读的世界数据查询（追加的槽位，受 `struct_size` 约束）。两个槽位都通过输出回调逐个给出结果，每个结果一个 SNBT 对象；只观察，不做修改。只能在服务器线程调用。

## 8ef2ce43c1

>
> Delete every save-file key belonging to one chunk, so the engine
> regenerates it from the generator on next load.
>
> Restoring an area by writing every cell with set_block is the wrong
> approach: a 32x32 plot times the world height is hundreds of thousands of
> cells and as many FFI crossings, and it still misses things, because block
> entities, actors and pending ticks (redstone, crop growth) are not block
> data. After such a rewrite the chests are still there and the redstone is
> still running.
>
> Erasing the save keys has neither problem: one forEachKeyWithPrefix yields
> every key of the chunk (all tags, all subchunks, actors, block entities,
> pending ticks), one pass deletes them, and the engine regenerates from the
> generator on next load.
>
> Key shape: a BDS chunk key is prefixed with
> <chunkX:i32 LE><chunkZ:i32 LE>, followed by <dimension:i32 LE> outside the
> overworld. After the prefix come a tag byte and a subchunk index, which
> this slot does not interpret; deleting everything with the prefix is
> exactly "everything in this chunk".
>
> The chunk must be unloaded. A loaded chunk has a LevelChunk in memory that
> the engine writes back on unload, recreating the deleted keys verbatim, so
> the deletion is silently undone. The caller is responsible for getting the
> chunk unloaded first (move players away, wait for it to leave tick range).
> This slot does not do that: deciding who is nearby and when unloading is
> acceptable needs the caller's domain knowledge, which this layer must not
> have.
>
> @return number of keys deleted; -1 if the save layer is unavailable. 0 is
>         a normal result, meaning that chunk was never generated.
>
> Pure append: PIER_ABI_VERSION is unchanged, struct_size is the gate.

删除属于一个区块的所有存档键，下次加载时引擎会用生成器重新生成这个区块。

用 `set_block` 把每一格重写一遍来恢复一片区域，这条路走不通：一块 32x32 的地皮乘以世界高度就是几十万格，也就是几十万次跨 FFI 的调用；而且还会漏掉东西，因为方块实体、实体和待处理的刻（红石、作物生长）都不属于方块数据。这样重写之后，箱子还在，红石也还在运行。

删除存档键没有这两个问题：一次 `forEachKeyWithPrefix` 就能列出这个区块的所有键（所有标签、所有子区块、实体、方块实体、待处理的刻），一遍删完，下次加载时引擎用生成器重新生成。

键的形状：BDS 的区块键以 `<chunkX:i32 LE><chunkZ:i32 LE>` 开头，主世界以外再跟一个 `<dimension:i32 LE>`。前缀之后是一个标签字节和子区块编号，这个槽位不解读它们；删除带这个前缀的所有键，正好就是「这个区块里的一切」。

区块必须处于未加载的状态。已加载的区块在内存里有一个 `LevelChunk`，引擎卸载它时会写回存档，把删掉的键原样重建，删除就这样被悄悄撤销了。让区块先卸载是调用方的责任（把玩家移开，等它离开刻的范围）。这个槽位不做这件事：判断谁在附近、什么时候卸载可以接受，需要调用方的领域知识，这一层不应该掌握这些。

返回删除的键的数量；存档层不可用时返回 -1。返回 0 是正常结果，表示这个区块从来没有生成过。

纯追加：`PIER_ABI_VERSION` 不变，由 `struct_size` 把关。

## f6b7c0a8e3

>
> Are the chunks covering [min..max] currently loaded in memory?
>
> Companion to level_delete_chunk_keys: erasing save keys only works on
> unloaded chunks, since a loaded one lives in memory and writes the deleted
> keys back verbatim on unload, while the erase itself "succeeds" and
> reports a positive key count. Without this slot a caller can only guess
> from "nobody is nearby", and guessing wrong fails silently.
>
> @return 1 if all are loaded, 0 if at least one is not, -1 if the dimension
>         is unavailable.
>
> Pure append: PIER_ABI_VERSION is unchanged, struct_size is the gate.

覆盖 [min..max] 的区块当前是否都已加载到内存里。

这是 `level_delete_chunk_keys` 的配套槽位：删除存档键只对未加载的区块有效。已加载的区块活在内存里，卸载时会把删掉的键原样写回，而删除本身显示「成功」，还报告了一个正数的键数。没有这个槽位，调用方只能凭「附近没人」去猜，猜错了也不会有任何提示。

返回 1 表示全部已加载，0 表示至少有一个没加载，维度不可用时返回 -1。

纯追加：`PIER_ABI_VERSION` 不变，由 `struct_size` 把关。

## 6becabe55c

>
> List every save-file key belonging to one chunk. One callback per key.
>
> Listing and deleting are two slots: the host accumulates no container of
> keys across a call, so the caller keeps the keys it wants and deletes each
> through level_delete_key. A host-side string container held across the
> engine's virtual calls is the shape that corrupts the heap here.
>
> Keys are binary and contain 0 bytes, hence PierStr with an explicit length
> rather than a C string.
>
> @return how many keys were reported; -1 if the save layer is unavailable.

列出属于一个区块的所有存档键，每个键调用一次回调。

列出和删除分成两个槽位：宿主在一次调用里不积累任何存放键的容器，调用方留下它要的键，再逐个通过 `level_delete_key` 删除。在这里，宿主一侧跨越引擎的虚函数调用持有一个字符串容器，正是会破坏堆的那种写法。

键是二进制的，里面有 0 字节，所以用带显式长度的 `PierStr`，不用 C 字符串。

返回报告的键的数量；存档层不可用时返回 -1。

## 6688f12d55

>
> Delete one chunk-category key, verbatim.
>
> Companion to level_chunk_keys. The key's content is not interpreted:
> whatever is passed is what gets deleted, which is exactly why it is safe,
> since it need not understand the subchunk format.

原样删除一个区块类的键。

这是 `level_chunk_keys` 的配套槽位。键的内容不会被解读：传进来什么就删除什么。它安全也正是因为这一点，它不需要理解子区块的格式。

## 5bcc2aa08f

>
> Set the biome over an area.
>
> Applied per whole column, so no y is taken: setBiome3d works per y, but
> Bedrock stores biomes per column. biome is a biome name such as
> "minecraft:plains".
>
> Returns how many columns were set. 0 means none were, either because the
> chunks are not loaded or because the name was not recognized.

设置一片区域的生物群系。

按整列设置，所以不接收 y：`setBiome3d` 按 y 设置，但基岩版按列存储生物群系。`biome` 是生物群系名，例如 `"minecraft:plains"`。

返回设置了多少列。返回 0 表示一列都没设置，原因可能是区块没有加载，也可能是名字认不出来。

## d8eef8bcff

>
> As scan_region for blocks, but each distinct block state is serialized
> once through the palette sink and every cell reports only an index. A
> region of one million stone cells costs one SNBT serialization instead
> of one million. Entities are not covered; use scan_region with a null
> block sink for those. The same 2^24-cell limit applies. Server thread
> only. Returns false if the dimension is not ready or the region is too
> large.

和 `scan_region` 扫描方块的部分一样，但每种不同的方块状态只通过调色板回调序列化一次，每一格只报告一个索引。一百万格石头的区域只需要序列化一次 SNBT，不用一百万次。不包括实体；要实体请用 `scan_region`，方块回调传空。同样有 2^24 格的上限。只能在服务器线程调用。维度没有就绪或者区域太大时返回 false。

## 0d89f2f2de

> Bulk world editing, appended and struct_size-gated.

批量编辑世界，追加的槽位，受 `struct_size` 约束

## b9e041eb1c

> Native write paths that bypass the console-command route used by
> set_block (`execute in <dim> run setblock…`). With these, block
> states come from structured NBT instead of command-string splicing,
> block entities can be written back, and entities can be respawned from
> saved NBT — all via existing engine entry points.
>
> update_flags is a bitmask: 1 = notify neighbours, 2 = sync client,
> 3 = both (equivalent to /setblock), 0 = neither (fastest for bulk
> fills, but the caller must resync afterwards). Server thread only.

原生的写入路径，绕开 `set_block` 所用的控制台命令那条路（`execute in <dim> run setblock…`）。有了这些槽位，方块状态来自结构化的 NBT，不用拼接命令字符串；方块实体可以写回；实体可以从保存的 NBT 重新生成。全部通过引擎现有的入口完成。

`update_flags` 是一个位掩码：1 = 通知相邻方块，2 = 同步给客户端，3 = 两者都要（等同于 `/setblock`），0 = 都不要（批量填充最快，但调用方之后必须自己重新同步）。只能在服务器线程调用。

## 7991a1c406

>
> Writes many cells in one call. palette holds palette_count block specs,
> each resolved once; every cell names one by index. A cell whose index is
> out of range, or whose block did not resolve, is skipped and counted in
> the return value's complement. Returns the number of cells written, or -1
> if the dimension is not ready or a pointer is null with a non-zero count.
> Server thread only.

一次调用写入很多格。`palette` 里有 `palette_count` 个方块描述，每个只解析一次；每一格用索引指定其中一个。索引越界或方块解析失败的格子会被跳过，它们的数量就是总格数减去返回值。返回写入的格子数；维度没有就绪，或者数量不为零而指针为空时返回 -1。只能在服务器线程调用。

## c415da035f

> §B player management

§B 玩家管理

## a116f7255e

> Titles

标题

## 9be44b0e4c

> `PACT_SET_TITLE` (player_action opcode 6) reaches the client by running
> the console command `title "<name>" title <text>`. Three things are
> wrong with that and none of them are theoretical:
>   - the text is pasted into a command line unquoted, so a plot named
>     `He said "hi"` truncates the command;
>   - `title`'s text parameter is a `message`, which expands selectors —
>     a plot named `@e` is a command injection, not a name;
>   - `/title` has no way to set fade/stay for the same call, so timing is
>     whatever the client last stored.
> This slot builds a real SetTitlePacket instead. No wire format crosses
> the FFI (the packet is constructed field-by-field on this side), so it
> survives protocol bumps the way `spawn_particle_for` does.
>
> `type` is SetTitlePacketPayload::TitleType:
>   0 Clear · 1 Reset · 2 Title · 3 Subtitle · 4 Actionbar · 5 Times
> The TextObject variants (6..8) need a ResolvedTextObject and are refused.
> `text` is ignored for Clear/Reset/Times.
>
> Durations are in TICKS. For 2/3/4, when all three are >= 0 a Times
> packet is sent first so the timing is deterministic rather than
> inherited from whatever the client last stored; pass -1 for all three to
> keep the client's current timing. Mixing (-1 with >=0) is refused rather
> than guessed at — a half-specified duration set has no sane meaning.
> Server thread only.

`PACT_SET_TITLE`（`player_action` 的第 6 号操作）是通过执行控制台命令 `title "<name>" title <text>` 送到客户端的。这样做有三个问题，三个都会真的发生：

- 文本不加引号就拼进命令行，名叫 `He said "hi"` 的地皮会把命令截断；
- `title` 的文本参数类型是 `message`，会展开选择器，名叫 `@e` 的地皮就成了一次命令注入；
- `/title` 没法在同一次调用里设置淡入和停留时间，计时用的是客户端上一次存下的值。

这个槽位改为构造一个真正的 `SetTitlePacket`。没有任何线上格式跨过 FFI（数据包在这一侧逐个字段构造），所以它和 `spawn_particle_for` 一样，协议升级以后照样能用。

`type` 是 `SetTitlePacketPayload::TitleType`：0 Clear · 1 Reset · 2 Title · 3 Subtitle · 4 Actionbar · 5 Times。TextObject 的几种变体（6 到 8）需要 `ResolvedTextObject`，会被拒绝。Clear、Reset、Times 忽略 `text`。

时长的单位是**刻**。对 2、3、4，三个时长都 >= 0 时会先发一个 Times 包，计时是确定的，不沿用客户端上一次存下的值；三个都传 -1 则保留客户端当前的计时。混着传（有的是 -1，有的 >= 0）会被拒绝，不去猜：只指定一部分的时长没有合理的含义。只能在服务器线程调用。

## a9ef8ef453

> The four *_get_num slots share one convention. The return is whether the host has
>  an answer, and *out is the answer; a false leaves *out untouched.
>
>  False and "the value is zero" are different results and a caller must not collapse
>  them (contract §5.2). False has two causes it does not distinguish: the subject was
>  not found, and the property is one this engine version does not expose. Neither is
>  an error the host logs, because a caller polling a property it cannot read would
>  fill the log with it; the list of properties that always answer false on a given
>  version is in CHANGELOG.md under the release.

四个 `*_get_num` 槽位遵守同一个约定。返回值表示宿主有没有答案，`*out` 是答案；返回 false 时 `*out` 不会被改动。

false 和「值为零」是两种结果，调用方不能把它们合成一种（契约 §5.2）。false 有两个原因，宿主不加区分：对象没找到，或者这个属性在当前引擎版本上取不到。这两种情况宿主都不记日志，因为调用方轮询一个读不到的属性会把日志写满；在某个版本上总是回答 false 的属性，列在 CHANGELOG.md 里那个版本的条目下。

## dfe1b76240

>
> This player's connection id — the same number packet interceptors see
> in the packet context.
>
> A packet callback has only conn_id, not a player, so per-player rewriting
> of outbound packets is impossible without this: locking the sky color
> needs the dimension of the person on that connection, which needs to know
> who they are.
>
> The alternative is to periodically send packets that override the
> server's, and that is wrong: the server sends real time while the mod
> sends locked time, the two kinds interleave, and the client's sky flickers
> between them. Rewriting is correct, and rewriting needs this slot.
>
> @return the connection id; 0 if the player is offline or their network
>         identifier is unavailable.
>
> Pure append: PIER_ABI_VERSION is unchanged, struct_size is the gate.

这名玩家的连接 id，和数据包拦截器在数据包上下文里看到的是同一个数。

数据包回调手里只有 `conn_id`，没有玩家，所以没有这个槽位就没法按玩家改写发出的数据包。比如锁定天空的颜色，需要知道这个连接上的人在哪个维度，也就需要知道这个人是谁。

另一种做法是定时发包覆盖服务器发出的包，这样做是错的：服务器发的是真实时间，模组发的是锁定的时间，两种包交错到达，客户端的天空就在两者之间闪烁。正确的做法是改写，而改写需要这个槽位。

返回连接 id；玩家不在线，或者拿不到它的网络标识时返回 0。

纯追加：`PIER_ABI_VERSION` 不变，由 `struct_size` 把关。

## 52f789bb05

> player_get_num / player_set_num keys. (G)=get-only, (S)=settable.

`player_get_num` / `player_set_num` 的键。(G) 表示只能读，(S) 表示可以写。

## b4d90fec58

> player_get_str keys.

`player_get_str` 的键。

## 2cf86b033c

>
> player_action verbs.  Args are (sarg, a, b, c); unused args are ignored.
> `out` (when non-NULL) receives a result string where noted.

`player_action` 的动作。参数是 (sarg, a, b, c)，用不到的参数会被忽略。注明了有结果的动作，`out`（不为 NULL 时）会收到一个结果字符串。

## d9af4d1d26

> Enumerate live actors; dim = -1 for all dimensions.

列出活着的实体；`dim` 为 -1 表示所有维度。

## 5ef9bd9ddf

> actor_get_num / actor_set_num keys. (S)=settable via actor_set_num.

`actor_get_num` / `actor_set_num` 的键。(S) 表示可以用 `actor_set_num` 写入。

## 3923d9ca63

> actor_get_str keys.

`actor_get_str` 的键。

## c8718ede66

> actor_action verbs. Args (sarg, a, b, c); `out` receives a result where noted.

`actor_action` 的动作。参数是 (sarg, a, b, c)；注明了有结果的动作，`out` 会收到结果。

## 15f2d7b1c0

> block_get_num keys.

`block_get_num` 的键。

## 631683cb52

> block_get_str keys.

`block_get_str` 的键。

## c30c77f60d

> block_action verbs.

`block_action` 的动作。

## 75316f8574

> Item: enchants, matching, NBT (dedicated fns)

物品：附魔、匹配、NBT（专用函数）

## 3c32ead52c

> Client-side container resync

让客户端重新同步容器

## fe549dbf5b

> `container_set_item` / `_clear` / `_add_item` all write through
> `Container::setItem`, which mutates the server's copy and sends nothing.
> The client keeps rendering whatever it last received, so a bulk rewrite
> (swapping a player's inventory on a cross-dimension teleport, say) looks
> like it did nothing until the player clicks a slot and forces a resync.
>
> Call this once after a batch of writes. Batching matters: this pushes
> the whole container, so calling it per-slot inside a loop is a packet
> storm for no benefit.

`container_set_item`、`_clear`、`_add_item` 都经 `Container::setItem` 写入，它只修改服务器上的副本，不发送任何东西。客户端继续显示它最后收到的内容，所以批量重写（比如跨维度传送时换掉玩家的物品栏）看起来像什么都没发生，直到玩家点一下某个格子，强制重新同步。

在一批写入之后调用一次这个槽位。要成批调用：它推送的是整个容器，在循环里每改一格调一次，只会发出大量数据包，没有任何好处。

## 019cc23042

>
> Every slot of a container in one call: the sink receives (slot, item SNBT)
> for each slot in order, empty slots included as an empty-item snapshot.
> One container resolution and one FFI crossing replace one of each per
> slot. Returns false if the container cannot be resolved. Server thread
> only.

一次调用读出容器的每一格：输出回调按顺序对每一格收到 (槽位, 物品 SNBT)，空格子也在内，给的是空物品的快照。解析一次容器、跨一次 FFI，代替原来每一格各一次。容器解析不出来时返回 false。只能在服务器线程调用。

## 8d7018d118

> item_get_num keys (query a transient ItemStack rebuilt from SNBT).

`item_get_num` 的键（查询的是从 SNBT 临时重建的 `ItemStack`）。

## 4708fbf029

> item_get_str keys.

`item_get_str` 的键。

## b664ff1ee5

> item_transform ops: rebuild → mutate → serialize back (out = new SNBT).

`item_transform` 的操作：重建，修改，再序列化回去（`out` 收到新的 SNBT）。

## 2e099944a6

> sarg=name             ItemStackBase::setCustomName

`sarg` 为名字，调用 `ItemStackBase::setCustomName`

## 01fb168fdd

> narg=damage           ItemStackBase::setDamageValue

`narg` 为损耗值，调用 `ItemStackBase::setDamageValue`

## df0b238669

> narg=count            ItemStackBase::mCount

`narg` 为数量，写入 `ItemStackBase::mCount`

## c80e1bfcdc

> sarg=SNBT list ["l1","l2"]  ItemStackBase::setCustomLore

`sarg` 为 SNBT 列表 `["l1","l2"]`，调用 `ItemStackBase::setCustomLore`

## d2c776b519

> narg=0/1               ItemStackBase::setUnbreakable

`narg` 为 0 或 1，调用 `ItemStackBase::setUnbreakable`

## bcd1a19bf7

> narg=damage            ItemStackBase::hurtAndBreak

`narg` 为伤害值，调用 `ItemStackBase::hurtAndBreak`

## 1707cb5fd7

> narg=cost              ItemStackBase::setRepairCost

`narg` 为修复花费，调用 `ItemStackBase::setRepairCost`

## f2823cd6e1

> sarg="name:level"      saveEnchantsToUserData

`sarg` 为 `"name:level"`，经 `saveEnchantsToUserData` 写入

## 1f3d5a7eaa

> ItemStackBase::removeEnchants

调用 `ItemStackBase::removeEnchants`

## 6e5f27976c

> ItemStackBase::clearCustomLore

调用 `ItemStackBase::clearCustomLore`

## 7433556860

> ItemStackBase::resetHoverName

调用 `ItemStackBase::resetHoverName`

## e7117ba668

> sarg=SNBT list         ItemStackBase::setCanDestroy

`sarg` 为 SNBT 列表，调用 `ItemStackBase::setCanDestroy`

## 67a43c2495

> sarg=SNBT list         ItemStackBase::setCanPlaceOn

`sarg` 为 SNBT 列表，调用 `ItemStackBase::setCanPlaceOn`

## fc99e5aa46

> scoreboard_op verbs (args a=objective/slot, b=target, n=value).

`scoreboard_op` 的操作（参数：`a` 为计分项或显示位置，`b` 为目标，`n` 为数值）。

## 812fc52058

> a=name, b=display name → out "1"      Scoreboard::addObjective("dummy")

`a` 为名字，`b` 为显示名，输出 `"1"`，调用 `Scoreboard::addObjective("dummy")`

## 2e4a035028

> a=name                                Scoreboard::removeObjective

`a` 为名字，调用 `Scoreboard::removeObjective`

## 73575eed6f

> → out SNBT [{name,display},…]        Scoreboard::getObjectives

输出 SNBT `[{name,display},…]`，取自 `Scoreboard::getObjectives`

## d43de0e105

> a=objective, b=fake-player name → out value  Objective::getPlayerScore

`a` 为计分项，`b` 为虚拟玩家名，输出分数，取自 `Objective::getPlayerScore`

## 68466897b0

> a=objective, b=name, n=value          Scoreboard::modifyPlayerScore(Set)

`a` 为计分项，`b` 为名字，`n` 为数值，调用 `Scoreboard::modifyPlayerScore(Set)`

## da28cd7abb

> a=objective, b=name, n=value         … (Add)

`a` 为计分项，`b` 为名字，`n` 为数值，同上（Add）

## 8434709098

> a=objective, b=name, n=value         … (Subtract)

`a` 为计分项，`b` 为名字，`n` 为数值，同上（Subtract）

## 17c1e8453c

> a=objective, b=name                   Scoreboard::resetPlayerScore

`a` 为计分项，`b` 为名字，调用 `Scoreboard::resetPlayerScore`

## 24aeb81194

> a=slot("sidebar"/"list"/"belowname"), b=objective  setDisplayObjective

`a` 为显示位置（`"sidebar"`、`"list"` 或 `"belowname"`），`b` 为计分项，调用 `setDisplayObjective`

## b3251fe78a

> a=slot                                clearDisplayObjective

`a` 为显示位置，调用 `clearDisplayObjective`

## cb94d29fb7

> §G forms (async result callback)

§G 表单（异步的结果回调）

## bd63a6d867

>
> kind: 0=SimpleForm 1=CustomForm 2=ModalForm. form_snbt describes the
> form (see docs/api/gui). The callback fires once, on the server thread,
> and is muted if the mod is disabled before the player responds.

`kind`：0=SimpleForm，1=CustomForm，2=ModalForm。`form_snbt` 描述表单（见 docs/api/gui）。回调只触发一次，在服务器线程上；如果玩家回应之前模组被禁用了，回调会被屏蔽。

## 306913a099

>
> Form result callback. Invoked ONCE on the server thread when the player
> responds (or the form is cancelled). result_snbt:
>   cancelled       : {cancelled:1b, reason:N}
>   SimpleForm      : {button:N}
>   CustomForm      : {values:{<name>: string|double|int64…}}
>   ModalForm       : {button:"upper"|"lower"}
> Muted (never called) if the mod is disabled before the player responds.

表单的结果回调。玩家回应（或者表单被取消）时，在服务器线程上调用**一次**。`result_snbt` 的形状：

```text
cancelled       : {cancelled:1b, reason:N}
SimpleForm      : {button:N}
CustomForm      : {values:{<name>: string|double|int64…}}
ModalForm       : {button:"upper"|"lower"}
```

如果玩家回应之前模组被禁用了，回调会被屏蔽，不会被调用。

## e565d5cca4

> fmt: 0=disk little-endian, 1=network.

`fmt`：0=磁盘格式（小端），1=网络格式。

## ae9e3131d1

> Raw byte sink (binary NBT). Bytes valid only within the call frame.

原始字节的输出回调（二进制 NBT）。字节只在当前调用帧内有效。

## 9d9718825f

> Key/value sink (kvdb_iter). Views valid only within the call frame.

键值对的输出回调（`kvdb_iter`）。两个视图只在当前调用帧内有效。

## 6444adf4b1

> Opaque handle to an open key-value database owned by the loader.

指向一个已打开的键值数据库的不透明句柄，数据库归加载器所有。

## 59b3b9c503

> Money (appended)

经济（追加）

## 534a40d9e1

>
> Backed by LegacyMoney, which is delay-loaded. The whole family degrades
> rather than crashing when the backend is absent or disabled, returning
> each slot's failure value. The semantics below come from LegacyMoney's
> source, not from guesswork:
>
>   - Amounts are always non-negative. val < 0 is rejected by the backend
>     itself (the first check in LLMoney_Trans), and a negative set_money
>     fails because the balance cannot be reduced to that target.
>   - trans_money rejects from == to and applies the backend's configured
>     pay_tax: the payee receives val - val * pay_tax, not val. To hand
>     over the full amount, use add and reduce separately.
>   - set_money's money is a target balance; the backend turns it into a
>     single transfer internally.

由 LegacyMoney 提供，它是延迟加载的。后端不在或被禁用时，这一组槽位整体降级，不会崩溃，各自返回失败值。下面的语义取自 LegacyMoney 的源码，没有一条是猜的：

- 金额总是非负的。`val < 0` 由后端自己拒绝（`LLMoney_Trans` 的第一项检查）；`set_money` 传负数会失败，因为余额不可能减到那个目标值。
- `trans_money` 拒绝 `from == to`，并按后端配置的 `pay_tax` 扣税：收款方收到的是 `val - val * pay_tax`，不是 `val`。要转出全额，请分别加钱和减钱。
- `set_money` 的 `money` 是目标余额；后端在内部把它变成一笔转账。

## 9102e82c2b

> Register a before callback, which may veto. Several mods may each register
>  one without overwriting the others, and registering the same function
>  pointer twice is idempotent. The loader attributes each callback to its
>  module and removes it when that mod unloads; LegacyMoney itself has no
>  unregister interface, so this bookkeeping is the loader's. A callback
>  outside every loaded pier mod cannot be attributed: that is logged at
>  registration, and such a callback stays until the process exits. Registration
>  does not require the backend to be ready yet: the loader installs the
>  forwarding trampoline once it becomes available.

注册一个变动前的回调，它可以否决这次变动。多个模组可以各注册一个，互不覆盖；同一个函数指针注册两次只算一次。加载器把每个回调记在它所在的模块名下，那个模组卸载时就移除它；LegacyMoney 自己没有注销接口，所以这份记录由加载器负责。不在任何已加载的 Pier 模组里的回调无法归属：注册时会记一条日志，这样的回调一直留到进程退出。注册时不要求后端已经就绪：后端可用以后，加载器才装上转发用的跳板函数。

## f9846790c2

> As above, but invoked after the change has happened; the return value is
>  ignored.

同上，但在变动发生之后调用；返回值会被忽略。

## 14b66a8d8e

>
> legacymoney event callback. Return false to veto the change; only the before
> callback's return value is honoured, an after callback's is ignored.
>
> from and to are less obvious than their names suggest. This is LegacyMoney's
> own shape, forwarded as-is rather than "corrected":
>
>   - PIER_MONEY_TRANS: from is the payer, to the payee, both non-empty.
>   - PIER_MONEY_ADD / REDUCE / SET: from is always the empty string and to is
>     the xuid being operated on, including for REDUCE, where the money is
>     taken from to. To learn whose balance changed, always read to.
>
> value is the delta for ADD, REDUCE and TRANS, and the target balance (not a
> delta) for SET.
>
> Every mod's callback is invoked; an earlier veto does not skip the rest, so
> the outcome does not depend on registration order. The change is vetoed if
> any callback returns false.

LegacyMoney 的事件回调。返回 false 否决这次变动；只有变动前回调的返回值有效，变动后回调的返回值会被忽略。

`from` 和 `to` 的含义没有名字看起来那么直白。这是 LegacyMoney 自己的形状，原样转发，没有做「修正」：

- `PIER_MONEY_TRANS`：`from` 是付款方，`to` 是收款方，两者都不为空。
- `PIER_MONEY_ADD` / `REDUCE` / `SET`：`from` 总是空字符串，`to` 是被操作的 xuid；`REDUCE` 也是这样，钱是从 `to` 那里扣的。想知道谁的余额变了，一律读 `to`。

`value` 在 ADD、REDUCE 和 TRANS 时是变化量，在 SET 时是目标余额（不是变化量）。

每个模组的回调都会被调用；前面的否决不会跳过后面的回调，所以结果和注册顺序无关。只要有一个回调返回 false，这次变动就被否决。

## 072476257f

> legacymoney event types, used by the server-side economy capability.

LegacyMoney 的事件类型，供服务器侧的经济能力使用。

## 6acb2f37b2

> Packet interception, appended and struct_size-gated.

数据包拦截，追加的槽位，受 `struct_size` 约束

## 0db279fd23

> Raw wire-format interception in both directions. This is the primitive
> `send_packet` could not provide: it observes and rewrites bytes that
> already exist, instead of manufacturing new ones.
>
> Delivery unit is exactly ONE packet — the leading unsigned-varint
> header followed by the packet body. Batching and compression live
> further down the peer chain (BatchedNetworkPeer splits inbound batches
> and re-batches outbound ones), so a callback never sees a batch and
> never has to produce a length prefix.
>
> The bridge decodes the header: `packet_id` is its low 10 bits,
> `sender_sub_id` / `target_sub_id` the two 2-bit fields above it, and
> `body`/`body_len` point PAST the header. A REPLACE verdict supplies a
> new BODY only; the bridge re-encodes the header from `edit`, so a
> rewrite never reproduces varint framing and packet-id remapping is a
> field assignment rather than a byte-surgery exercise.
>
> Dispatch chains: with several subscribers, each one sees the output of
> the previous, in registration order. The first DROP wins and the rest
> are skipped. Subscriber lists are snapshotted before dispatch, so a
> callback may register or unregister (including itself) safely.
>
> Threading — read this before touching game state. Inbound callbacks run
> wherever the connection is pumped and outbound ones wherever the send
> originates. In practice that is the server thread, but async flush
> means it is not guaranteed. Treat these as "not necessarily the game
> thread": a handler stays short, guards its own state, and routes anything
> that touches the world through `schedule`.
>
> Detours install lazily on the first subscriber and are never unpatched
> (an unsubscribe can arrive from inside the hooked function). With no
> subscribers the hook bodies fast-path straight to origin.

双向拦截原始的线上格式。这是 `send_packet` 做不到的：它观察并改写已经存在的字节，不制造新的字节。

传递的单位正好是**一个**数据包：开头是无符号变长整数的包头，后面跟着包体。分批和压缩在对端链更下层（`BatchedNetworkPeer` 拆开收到的批次，把发出的包重新分批），所以回调永远看不到一批数据包，也永远不需要自己写长度前缀。

bridge 负责解码包头：`packet_id` 是它的低 10 位，`sender_sub_id` / `target_sub_id` 是再往上的两个 2 位字段，`body`/`body_len` 指向包头**之后**的位置。REPLACE 只提供新的**包体**；bridge 根据 `edit` 重新编码包头，所以改写时不需要重新拼变长整数的帧，改数据包 id 也只是给一个字段赋值，不需要去改字节。

分发是链式的：有多个订阅者时，按注册顺序，每一个看到的是前一个的输出。第一个 DROP 生效，后面的被跳过。分发前会给订阅者列表拍一个快照，所以回调里可以安全地注册或注销（包括注销自己）。

线程：碰游戏状态之前请先读这一段。收到的包，回调在连接被泵送的地方运行；发出的包，回调在发送发起的地方运行。实际上通常是服务器线程，但有异步刷新，所以并不保证。请把它们当作「不一定在游戏线程上」：处理函数要短，自己保护自己的状态，凡是碰世界的操作都通过 `schedule` 转过去。

钩子在出现第一个订阅者时才安装，之后永不卸下（注销可能发生在被钩住的函数内部）。没有订阅者时，钩子函数直接快速转到原函数。

## 95ff807957

> Appended: packet interception filtered by id

追加：按 id 过滤的数据包拦截

## 26084fddd7

>
> Register a raw packet interceptor.
> `dir_mask` is PIER_PKT_MASK_INBOUND | PIER_PKT_MASK_OUTBOUND (a
> zero mask registers nothing and returns NULL). Returns NULL on failure.

注册一个原始数据包拦截器。`dir_mask` 是 `PIER_PKT_MASK_INBOUND | PIER_PKT_MASK_OUTBOUND` 的组合（为 0 时什么都不注册，返回 NULL）。失败时返回 NULL。

## 1c7a347ae3

>
> Register a connection open/close observer. Returns NULL on failure.
> The close notification is the only reliable signal for dropping
> per-connection state: a connection that never finishes the login
> handshake never becomes a Player, so no player event covers it.

注册一个观察连接打开和关闭的回调。失败时返回 NULL。关闭通知是丢弃按连接保存的状态时唯一可靠的信号：没有完成登录握手的连接永远不会成为 Player，任何玩家事件都覆盖不到它。

## 54e03a8d79

>
> As packet_hook_register, but the callback fires only for the listed packet
> ids (MinecraftPacketIds values, 0..1023). packet_hook_register subscribes
> to every id and therefore costs one callback per packet in that direction,
> chunk data included; a mod watching a few ids should use this slot, since
> a packet no subscriber listed is passed through before any lock is taken.
> An empty list, or one whose ids are all out of range, is refused. The same
> unregister slot applies. Any thread.

和 `packet_hook_register` 一样，但回调只对列出的数据包 id（`MinecraftPacketIds` 的值，0 到 1023）触发。`packet_hook_register` 订阅所有 id，那个方向上每个数据包都要调用一次回调，区块数据也算在内；只关心几个 id 的模组应当用这个槽位，因为没有任何订阅者列出的数据包，会在加锁之前直接放行。列表为空，或者所有 id 都超出范围时会被拒绝。注销用同一个槽位。可以在任何线程调用。

## 0d7f808993

>
> One intercepted packet. Every pointer inside is borrowed and valid only for
> the duration of the callback. Anything kept past it must be copied.

一个被拦截的数据包。里面的每个指针都是借来的，只在回调期间有效。回调之后还要用的东西必须复制一份。

## fb02923ce7

>
> Mutable header fields, pre-filled from the event. Assignments here only take
> effect when the callback returns PIER_PKT_REPLACE.

可以修改的包头字段，预先填好了事件里的值。只有回调返回 `PIER_PKT_REPLACE` 时，这里的赋值才生效。

## 963db0b9f8

> Drop via packet_hook_unregister / packet_conn_hook_unregister.

用 `packet_hook_unregister` / `packet_conn_hook_unregister` 释放。

## 6590543bfa

>
> Packet interceptor. To rewrite, call `replace(replace_ctx, bytes, len)` with
> the NEW BODY (header excluded) and return PIER_PKT_REPLACE. Calling
> `replace` more than once keeps the last body; returning REPLACE without ever
> calling it means "empty body".

数据包拦截器。要改写，就用**新的包体**（不含包头）调用 `replace(replace_ctx, bytes, len)`，并返回 `PIER_PKT_REPLACE`。多次调用 `replace`，以最后一次的包体为准；返回 REPLACE 却一次都没调用 `replace`，表示「包体为空」。

## feb8c43372

> Connection lifecycle: `opened` is true on accept, false on close.

连接的生命周期：接受连接时 `opened` 为 true，关闭时为 false。

## 1cac9ac44d

> PierPacketEvent::direction, and the bit positions used by dir_mask.

`PierPacketEvent::direction` 的取值，也是 `dir_mask` 用到的位的位置。

## 8c3b8a3e0b

> client -> server

客户端 -> 服务器

## 4e578ac34d

> server -> client

服务器 -> 客户端

## ddc15f6889

> PierPacketCb return value. Anything else is treated as PASS.

`PierPacketCb` 的返回值。其他任何值都按 PASS 处理。

## 03f439eb1d

> forward unchanged; `replace` output ignored

原样转发；`replace` 的输出被忽略

## ddabbbeb94

> forward the body handed to `replace`

转发交给 `replace` 的包体

## d76a2cbcca

> swallow the packet entirely

整个吞掉这个数据包

## 047faf9566

> Capability group: client (client_*). All NULL on a server host.

能力组：客户端（`client_*`）。在服务器宿主上全部为 NULL

## 68b6f31df3

> Two capability groups follow, client and dimensions. They are unconditionally
> present in the layout; when the capability package was not built into the
> host, their slots are NULL (see the file header). New slots still go at the
> real end of the struct, never inside a capability group.
>
> Every callback fires on the client thread.

后面是两个能力组：客户端和维度。它们在布局里无条件存在；宿主没有编入对应的能力包时，它们的槽位为 NULL（见文件头）。新槽位仍然加在结构体真正的末尾，永远不加在能力组中间。

所有回调都在客户端线程上触发。

## 00d773eb9a

> Register a key binding via ll::input::KeyRegistry::getOrCreateKey.
>  Returns NULL on failure. The handle is owned by the caller; drop with
>  client_unregister_key. down_cb/up_cb fire on the client thread.

通过 `ll::input::KeyRegistry::getOrCreateKey` 注册一个按键绑定。失败时返回 NULL。句柄归调用方所有，用 `client_unregister_key` 释放。`down_cb`/`up_cb` 在客户端线程上触发。

## 6281a3d563

> Opaque handle to a registered key binding owned by the loader's
> ll::input::KeyRegistry. Drop via client_unregister_key.

指向一个已注册按键绑定的不透明句柄，绑定归加载器的 `ll::input::KeyRegistry` 所有。用 `client_unregister_key` 释放。

## 8f0562c34a

> Key action: 0 = released (up), 1 = pressed (down).
> Mirrors ll::event::KeyInputEvent::Action.

按键动作：0 = 松开（up），1 = 按下（down）。对应 `ll::event::KeyInputEvent::Action`。

## 2fe30a04f9

> Focus impact level: 0=Neutral 1=ActivateFocus 2=DeactivateFocus.
> Mirrors ::FocusImpact.

焦点影响级别：0=Neutral，1=ActivateFocus，2=DeactivateFocus。对应 `::FocusImpact`。

## 695caeac6d

> Callback for key press/release events. Runs on the client thread.
>  user   — pointer passed to client_register_key
>  action — 0=released 1=pressed (see PierKeyAction)
>  impact — current focus impact (see PierFocusImpact)

按键按下和松开事件的回调，在客户端线程上运行。

- `user`：传给 `client_register_key` 的指针
- `action`：0=松开，1=按下（见 `PierKeyAction`）
- `impact`：当前的焦点影响（见 `PierFocusImpact`）

## a2117b399a

> Capability group: custom dimensions (md_*). All NULL when pier-dimensions

能力组：自定义维度（`md_*`），宿主没有编入 pier-dimensions 时全部为 NULL

## 2257e43e02

> was not built into the host.

宿主没有编入 pier-dimensions 时，这一组槽位全部为 NULL。

## 65fbb4374f

> Plot-boundary confinement

地皮边界的约束

## 4bf7be6be7

> Backing store for PIER_DIMRULE_PISTON_CROSS_CELL and
> PIER_DIMRULE_ENTITY_CROSS_CELL. Those two rules ask "are these two
> columns in the same plot?", and the answer needs the grid geometry plus
> the merge markers. The question is asked from
> `PistonBlockActor::_checkAttachedBlocks` and `Actor::move` — engine tick
> paths, hundreds of calls a second — so the data is pushed here once and
> read natively rather than queried back across the FFI.
>
> The ownership rule implemented on the loader side mirrors the plugin's
> own `owning_plot`: a seam between two merged plots counts as plot, a
> junction counts as plot only when all four surrounding edges are merged.
> Divergence does not show up as "one column judged wrong" — it shows up as
> an owner who can place a block by hand on their merged plot but whose
> piston refuses to push there. Server thread only.

为 `PIER_DIMRULE_PISTON_CROSS_CELL` 和 `PIER_DIMRULE_ENTITY_CROSS_CELL` 提供数据。这两条规则要回答「这两列在不在同一块地皮里」，答案需要网格的几何形状，再加上合并标记。这个问题是在 `PistonBlockActor::_checkAttachedBlocks` 和 `Actor::move` 里提出的，都在引擎的刻路径上，每秒几百次调用，所以数据一次推送到这里，在原生代码里读取，不跨 FFI 回头查询。

加载器一侧实现的归属规则和插件自己的 `owning_plot` 一致：两块已合并的地皮之间的接缝算地皮；交叉口只有四周的边都合并了才算地皮。两边规则不一致时，表现出来的不会是「某一列判断错了」：地皮主人能在自己合并后的地皮上手动放方块，活塞却拒绝推到那里。只能在服务器线程调用。

## 189263b8ab

> Dimensions whose terrain belongs to the mod

地形归模组所有的维度

## 11f9e459e1

>
> The three vanilla generators and the void are the engine's own, and this host
> serves them because they cost it nothing to serve. Everything past that -- a
> layer stack, a grid of plots, a noise field, a binary terrain format and the
> code that reads it -- is a mod's, and this host does not want to know its shape.
> These two slots are the whole of what it needs to know.

三种原版生成器和虚空是引擎自己的，宿主提供它们，是因为提供它们几乎没有代价。除此之外的一切，比如层叠的地层、地皮网格、噪声场、二进制的地形格式和读取它的代码，都属于模组，宿主不想知道它们的形状。这两个槽位就是宿主需要知道的全部。

## 259622a805

> Per-dimension rules, consulted by the loader's own hooks.
>
>  Why this exists instead of gamerules: Bedrock gamerules are
>  server-wide. Setting `doMobSpawning=false` to quiet a creative plot
>  world also stops spawning in the survival world. These flags are
>  checked inside hooks on the actual call sites (Spawner::spawnMob,
>  Level::explode, ...), so they really are per-dimension.
>
>  `rule` is one of PierDimRule. Setting a rule on a dimension the
>  loader doesn't know about is harmless — the tables are keyed by raw
>  dimension id and consulted only when that id shows up in a hook.
>
>  Dimensions with no entry are left completely alone: the hooks fall
>  through to origin(), so vanilla dimensions keep vanilla behavior
>  without the caller having to opt out.

按维度设置的规则，由加载器自己的钩子查询。

为什么不用游戏规则：基岩版的游戏规则对整个服务器生效。为了让创造模式的地皮世界安静一点而设置 `doMobSpawning=false`，生存世界也会停止生成生物。这些标志在实际调用处的钩子里检查（`Spawner::spawnMob`、`Level::explode` 等），所以真正是按维度生效的。

`rule` 是 `PierDimRule` 中的一个。给加载器不知道的维度设置规则没有坏处：规则表按原始的维度 id 索引，只在钩子里出现那个 id 时才查。

没有设置任何规则的维度完全不受影响：钩子直接调用 `origin()`，原版维度保持原版行为，调用方不需要专门排除它们。

## bc2778bb87

> Resolve a dimension name to its id. Returns -1 if not found.
>
>  Only returns an id for names that are ACTUALLY registered: unknown
>  names yield -1, never VanillaDimensions::Undefined() (whose numeric
>  value is mutated at runtime and looks like a valid id).
>
>  This is rarely the right call. `md_add_dimension` and
>  `md_add_dimension_pack` are idempotent, so re-registering the same name
>  on a later boot returns the same persisted id, and a caller registers
>  unconditionally at startup instead of probing first.

把维度名解析成维度 id。找不到时返回 -1。

只对**真正**注册过的名字返回 id：不认识的名字得到 -1，永远不会得到 `VanillaDimensions::Undefined()`（它的数值会在运行时被修改，看起来像一个有效的 id）。

很少需要用到这个调用。`md_add_dimension` 和 `md_add_dimension_pack` 是幂等的，以后启动时用同一个名字重新注册，会返回同一个持久化的 id，所以调用方在启动时无条件注册就行，不必先探测。

## f363f18302

> Replace a dimension's merge markers wholesale. `entries` is `count`
>  triples `(x, z, mask)`, i.e. `count * 3` int32s; `mask` is a bitset of
>  1=north, 2=east, 4=south, 8=west matching the plugin's `merged[]`
>  indices. Only plots that actually carry a marker need to be sent.
>
>  Wholesale, not incremental: incremental requires both sides to agree
>  forever on what is currently in the table, and `unlink` clears the
>  neighbour before storing itself — a failure in between leaves the two
>  views apart with no way back. Replacing pulls them into agreement on
>  every push. The grid comes from the template pack's CONF section at
>  md_add_dimension_pack; a push for a dimension without one is dropped with
>  a warning. Cell geometry, which the mod side must match: with
>  period = cell + gap, a column at world (x, z) is inside a cell when
>  mod(x,period) < cell && mod(z,period) < cell.

整体替换一个维度的合并标记。`entries` 是 `count` 个三元组 `(x, z, mask)`，也就是 `count * 3` 个 int32；`mask` 是位集合，1=北，2=东，4=南，8=西，和插件的 `merged[]` 下标对应。只需要发送确实带有标记的地皮。

整体替换，不做增量：增量要求两边永远对表里现有的内容达成一致，而 `unlink` 会先清掉邻居再保存自己，两步之间出错，两边的视图就分开了，而且没有办法恢复。整体替换让每一次推送都把两边拉回一致。网格来自 `md_add_dimension_pack` 时模板包的 CONF 段；对没有网格的维度推送会被丢弃，并记一条警告。模组一侧必须对上的单元几何是：令 `period = cell + gap`，世界坐标 (x, z) 处的一列在单元内，当且仅当 `mod(x,period) < cell && mod(z,period) < cell`。

## 5cdd9e1e6a

>
> List every registered custom dimension as a JSON array:
> [{"name":"plot_world","dim":1000,"snbt":"{…}"}].
>
> Without this slot the md_* family can only be queried by name
> (md_get_dimension_id), so a caller must already know the name. A world
> manager taking over an existing save would then be blind to dimensions
> created by a previous plugin: they sit in dimension_config.json, they are
> alive in the engine, players can teleport into them, and the manager's
> table has no row for them. The consequence is not a short listing but
> dimensions governed by no rules at all, plus the risk that a newly created
> world is assigned a number that collides with one of them, leaving two
> worlds sharing a dimension id.
>
> name comes from the config file key, dim is the engine-assigned and
> persisted number, and snbt is the verbatim generation parameters (a plot
> world gives {layout:{…},seed:N}, a simple world {generatorType:Flat,
> seed:N}) for the caller to interpret.
>
> The sink is invoked ONCE PER DIMENSION, each with one JSON object -- not
> once with an array. Contrast lane_list above, which hands over a single
> JSON array. Both shapes exist in this table; each slot says which.
>
> The callback is not invoked at all when md_is_available() is false.

以 JSON 数组列出所有已注册的自定义维度：`[{"name":"plot_world","dim":1000,"snbt":"{…}"}]`。

没有这个槽位，`md_*` 这一组只能按名字查询（`md_get_dimension_id`），调用方必须事先知道名字。接管一个现有存档的世界管理器，就看不到以前的插件创建的维度：它们写在 dimension_config.json 里，在引擎里活着，玩家能传送进去，管理器的表里却没有它们。后果比列表少几行严重：这些维度不受任何规则约束，新建的世界还可能分到一个和它们冲突的编号，两个世界共用一个维度 id。

`name` 来自配置文件的键，`dim` 是引擎分配并持久化的编号，`snbt` 是原样的生成参数（地皮世界给出 `{layout:{…},seed:N}`，简单世界给出 `{generatorType:Flat,seed:N}`），由调用方解读。

输出回调**每个维度调用一次**，每次一个 JSON 对象，并不是调用一次给一个数组。对比上面的 `lane_list`，它交出的是一个 JSON 数组。这张表里两种形状都有，每个槽位都会写明是哪一种。

`md_is_available()` 为 false 时，回调一次都不会被调用。

## 9f83fa31e6

> Add a custom dimension with a native terrain from one declarative spec.
>
>  `spec_snbt` is a CompoundTag SNBT string:
>    {seed:123,
>     height:{min:-64,max:320},            optional, both multiples of 16
>     sky:{client:"overworld"|"nether"|"end", skylight:true, weather:true,
>          time:12000},                       optional, 0..23999
>     sky.time holds this dimension at one tick of day and leaves every other
>     dimension on the level clock. A nether or end client sky has no day cycle to
>     begin with, so the field only changes what an overworld sky shows.
>     terrain:{kind:"native",
>              generator:"overworld"|"nether"|"end"|"flat"|"void",
>              biome:"minecraft:plains"}       biome is read for void only
>
>  The three vanilla generators carry their structures (villages, fortresses, end
>  cities); what differs from the vanilla dimension is the seed, the height and
>  the sky.
>
>  `sky.client` is what the client is told in DimensionDefinition: nether and
>  end skies have no day/night, which is how those dimensions lock time. It is
>  independent of the server-side generator.
>
>  A terrain of kind template or volume is refused here with -1: those go through
>  md_add_dimension_pack, which verifies the pack before the spec is stored. A
>  height not on a subchunk boundary or a generator name the host does not know
>  refuses the same way; nothing falls back to a "close enough" generator, because
>  the spec is persisted with the dimension and terrain generated from a wrong
>  spec cannot be regenerated.
>
>  Idempotent by name. Returns dim id (>=3) or -1.

用一份声明式的描述，添加一个使用原生地形的自定义维度。

`spec_snbt` 是一个 `CompoundTag` 的 SNBT 字符串：

```text
{seed:123,
 height:{min:-64,max:320},            可选，上下限都是 16 的倍数
 sky:{client:"overworld"|"nether"|"end", skylight:true, weather:true,
      time:12000},                       可选，0 到 23999
 terrain:{kind:"native",
          generator:"overworld"|"nether"|"end"|"flat"|"void",
          biome:"minecraft:plains"}       biome 只在 void 时读取
```

`sky.time` 把这个维度固定在一天中的某一刻，其他维度仍然跟着关卡时钟走。下界和末地的客户端天空本来就没有昼夜循环，所以这个字段只影响主世界天空显示的内容。

三种原版生成器带有它们的结构（村庄、要塞、末地城）；和原版维度不同的是种子、高度和天空。

`sky.client` 是在 `DimensionDefinition` 里告诉客户端的天空：下界和末地的天空没有昼夜，那两个维度就是这样锁定时间的。它和服务器一侧的生成器互相独立。

`kind` 为 template 或 volume 的地形在这里会被拒绝，返回 -1：它们要走 `md_add_dimension_pack`，那里会先校验地形包，再保存描述。高度不在子区块边界上，或者生成器的名字宿主不认识，也同样拒绝；不会退回到一个「差不多」的生成器，因为描述会和维度一起持久化，按错误描述生成的地形没法重新生成。

按名字幂等。返回维度 id（>=3）或 -1。

## 98f597daea

> Add a custom dimension whose terrain is a terrain pack: a directory with a
>  config file and a binary built by tools/pier-pack. `config_path` names the
>  config relative to the server root, forward slashes, no ".." and not
>  absolute; the binary is named inside the config relative to it.
>
>  The host reads four keys of the config and nothing else:
>    {"pier_terrain":1, "type":"template"|"volume",
>     "binary":"terrain.ptpl", "sha256":"<64 hex digits>"}
>  Anything else in the file belongs to the mod (a title, a description, a
>  preview) and the host never looks at it.
>
>  `spec_snbt` has the shape of md_add_dimension with a pack terrain:
>    terrain:{kind:"template"|"volume",
>             params:{plot_size:64, road_width:7},   template parameters, ints
>             roles:{floor:"minecraft:stone"}}        template role overrides
>  A parameter the pack marks fixed cannot be given another value; one it marks
>  derived cannot be given at all; a free one must lie in its range and on its
>  step; a choice one must be one of its choices. A role not in the pack, or a
>  block that is not registered, refuses. The pack's own constraints are checked
>  with the bound values and a failing one refuses with its message in the log.
>
>  Three sources are compared before anything is stored: spec terrain.kind, the
>  config's type, and the binary's magic, and the binary must hash to the config's
>  sha256. The stored spec then holds the path, that hash, every bound parameter
>  and the role overrides, so the terrain is regenerable from the save alone.
>
>  On a later boot the stored spec wins: the same name returns the same id, the
>  parameters given now are ignored with a warning if they differ, and a config
>  whose sha256 is no longer the stored one refuses with
>  PIER_PACK_STORED_MISMATCH, since terrain from a different binary cannot
>  continue a world. Editing a pack means a new world or a new name.
>
>  A template pack with a CONF section registers the cell grid for the
>  confinement rules from the same mount that produced the terrain, so the two
>  can never disagree; md_set_plot_merges then applies.
>
>  Returns dim id (>=3), or one of the PIER_PACK_* codes below, all negative.

添加一个地形来自地形包的自定义维度。地形包是一个目录，里面有一个配置文件和一个由 tools/pier-pack 构建的二进制文件。`config_path` 指定配置文件，相对于服务器根目录，用正斜杠，不能有 `..`，也不能是绝对路径；二进制文件的名字写在配置里，相对于配置文件。

宿主只读配置里的四个键，别的都不看：

```text
{"pier_terrain":1, "type":"template"|"volume",
 "binary":"terrain.ptpl", "sha256":"<64 位十六进制数>"}
```

文件里的其他内容都属于模组（标题、描述、预览图），宿主从不读取。

`spec_snbt` 和 `md_add_dimension` 的形状一样，地形部分换成地形包：

```text
terrain:{kind:"template"|"volume",
         params:{plot_size:64, road_width:7},   模板参数，整数
         roles:{floor:"minecraft:stone"}}        模板角色的替换
```

地形包标记为 fixed 的参数不能给别的值；标记为 derived 的参数根本不能给；free 的参数必须在范围内并落在步长上；choice 的参数必须是选项之一。地形包里没有的角色，或者没有注册的方块，都会被拒绝。地形包自己的约束会用绑定后的值检查，不满足的约束会拒绝注册，并把它的消息写进日志。

保存任何东西之前，要比较三个来源：描述里的 `terrain.kind`、配置里的 `type`、二进制文件的魔数，而且二进制文件的哈希必须等于配置里的 `sha256`。保存下来的描述包含路径、这个哈希、每一个绑定的参数和角色替换，所以只凭存档就能重新生成地形。

以后启动时，保存下来的描述优先：同一个名字返回同一个 id；这次给的参数如果不同，会被忽略，并记一条警告；配置的 `sha256` 不再是保存的那个时，以 `PIER_PACK_STORED_MISMATCH` 拒绝，因为另一个二进制生成的地形不能接着一个已有的世界用。修改地形包，就意味着新建世界，或者换一个名字。

带 CONF 段的模板包会用生成地形的同一次挂载，为约束规则注册单元网格，所以两者永远一致；之后可以用 `md_set_plot_merges`。

返回维度 id（>=3），或者下面某个 `PIER_PACK_*` 代码，这些代码都是负数。

## 5e313f1e3b

> Retire a custom dimension: drop it from dimension_config.json, from the host's
>  own tables and from the dimension factory, so nothing registers it on the next
>  boot and it stops appearing in md_list_dimensions.
>
>  This is not a delete. The chunks the dimension wrote are still in the save, the
>  engine still holds the id it was given for this session, and a player standing
>  in it is not moved. What ends is the host's willingness to register the name
>  again from its own config.
>
>  The id is not returned to the pool. A retired name registered again is a new
>  dimension and gets a fresh id, which leaves the old chunks orphaned on disk and
>  costs space. Handing the number back instead would point the new dimension at
>  the terrain of the old one, and nothing about that is recoverable, so the
>  cheaper mistake is the one this makes.
>
>  True while the name was known and has been retired, false when the host had no
>  such dimension, which is also what a second call reports.

让一个自定义维度退役：把它从 dimension_config.json、宿主自己的表和维度工厂里去掉，下次启动时没有任何东西会注册它，它也不再出现在 `md_list_dimensions` 里。

这不是删除。这个维度写下的区块还在存档里，引擎在这次会话里仍然持有分给它的 id，站在里面的玩家也不会被移走。结束的只是宿主从自己的配置里再次注册这个名字的做法。

id 不会退回到池里。退役的名字再次注册时是一个新维度，会分到新的 id，旧区块留在磁盘上，没有任何维度引用，白占空间。如果把编号交还出去，新维度就会指向旧维度的地形，那样什么都恢复不了，所以这里选的是代价更小的那个错误。

名字已知并且已经退役时返回 true；宿主没有这个维度时返回 false，第二次调用报告的也是 false。

## 9b6d71dcab

> Register a dimension the calling mod fills itself.
>
>  `spec_snbt` is the shape of md_add_dimension without its terrain section:
>
>      {seed:<u32>,
>       height:{min:<int>,max:<int>},        multiples of 16, inside the world range
>       sky:{client:"overworld"|"nether"|"end", skylight:<bool>, weather:<bool>,
>            time:<0..23999, optional>}}
>
>  A terrain section here is refused: this host does not read one, and accepting a
>  field it ignores is how a spec comes to describe a world nobody generates.
>
>  `material_palette` and `biome_palette` are newline-separated names. Material
>  index 0 must be air and is what an untouched column holds. Every name is resolved
>  once, here: a name outside the block or biome registry refuses the registration
>  rather than becoming a hole in the terrain later.
>
>  `fn` is called on chunk worker threads; the contract is on PierGenerateChunkFn
>  and is not the usual one. `user` is passed back untouched and is never read.
>
>  The dimension, its id and its spec are persisted exactly as md_add_dimension
>  persists them, so it comes back on the next boot -- but the terrain does not
>  until the same mod registers it again. A dimension whose mod is gone loads as a
>  void, keeps its chunks and says so once.
>
>  Idempotent by name. Returns dim id (>=3) or -1.

注册一个地形由调用方模组自己填充的维度。

`spec_snbt` 是去掉 terrain 段的 `md_add_dimension` 的形状：

```text
{seed:<u32>,
 height:{min:<int>,max:<int>},        16 的倍数，在世界范围之内
 sky:{client:"overworld"|"nether"|"end", skylight:<bool>, weather:<bool>,
      time:<0..23999，可选>}}
```

这里带 terrain 段会被拒绝：宿主不读它，而接受一个会被忽略的字段，描述就会写着一个没有人生成的世界。

`material_palette` 和 `biome_palette` 是用换行分隔的名字。材料索引 0 必须是空气，没有动过的列里就是它。每个名字都在这里解析一次：方块或生物群系注册表里没有的名字会让注册失败，不会留到以后变成地形里的一个空洞。

`fn` 在区块工作线程上被调用；它的约定写在 `PierGenerateChunkFn` 上，和通常的约定不一样。`user` 原样传回，宿主从不读取。

维度、它的 id 和它的描述都像 `md_add_dimension` 那样持久化，所以下次启动时它会回来，但地形要等同一个模组再次注册它才会回来。模组已经不在的维度会作为虚空加载，保留它的区块，并提示一次。

按名字幂等。返回维度 id（>=3）或 -1。

## 9e3946b091

> Give a generated dimension the cell geometry its confinement rules use.
>
>  PIER_DIMRULE_PISTON_CROSS_CELL and PIER_DIMRULE_ENTITY_CROSS_CELL ask whether two
>  positions are in the same cell, and that needs the grid. The geometry belongs to
>  whatever the mod generated, so the mod states it; the hooks stay here because
>  they are engine hooks. `cell` and `gap` are in blocks, `cell` positive; a gap of
>  zero means the cells touch. Passing cell 0 removes the geometry and with it any
>  answer the two rules could give.
>
>  Returns false when the id is not a dimension this host registered.

给一个生成式维度设置约束规则所用的单元几何。

`PIER_DIMRULE_PISTON_CROSS_CELL` 和 `PIER_DIMRULE_ENTITY_CROSS_CELL` 要判断两个位置是否在同一个单元里，这需要网格。几何属于模组生成的东西，所以由模组来声明；钩子留在宿主这边，因为它们是引擎钩子。`cell` 和 `gap` 以方块为单位，`cell` 为正数；`gap` 为 0 表示单元彼此相接。`cell` 传 0 会移除几何，这两条规则也就不再给出任何回答。

id 不是这个宿主注册的维度时返回 false。

## 69f9a45eb8

>
> One chunk asked of a mod that supplies its own terrain.
>
> `out_materials` is 256 * height entries, indexed (x * 16 + z) * height + y, y counted
> up from `min_y`. `out_biomes` is 256 entries, one per column. Both hold indices into
> the palettes given at registration; index 0 of the material palette is air. The host
> owns both buffers and reuses them, so a callback writes and keeps nothing.
>
> The host resolves the indices to blocks and biomes once, at registration. That is why
> the terrain never crosses this boundary as block names: a chunk is 98k lookups, and
> doing them per chunk is the difference between a generator and a stall.

向自己提供地形的模组请求的一个区块。

`out_materials` 有 `256 * height` 项，下标是 `(x * 16 + z) * height + y`，y 从 `min_y` 往上数。`out_biomes` 有 256 项，每列一项。两者存的都是注册时给出的调色板里的索引；材料调色板的索引 0 是空气。两个缓冲区归宿主所有，会被重复使用，所以回调只写入，什么都不保留。

宿主在注册时把索引一次性解析成方块和生物群系。地形从不以方块名的形式跨过这条边界，原因就在这里：一个区块要查 9.8 万次，每个区块都这样查一遍，生成器就成了卡顿的来源。

## 1ee80fb806

>
> Fills one chunk. Non-zero means filled; zero means the mod could not, and the host
> writes air and says so once per dimension.
>
> THREADING, and this one is the opposite of the rest of this file: the host calls this
> on its CHUNK WORKER THREADS, several at once, for different chunks of the same
> dimension. The host serializes nothing.
>
>   - It must be safe to run concurrently with itself.
>   - It must give the same answer for the same (dim_id, chunk_x, chunk_z) forever:
>     chunks are generated once and saved, and a neighbour generated later from a
>     different answer leaves a seam that no later edit can remove.
>   - It must not call any other slot on this API. Every one of them is written for the
>     server thread, and a call from here reaches a state nothing is holding a lock on.
>   - It must not throw across the boundary.
>
> A generator that reads only what registration handed it satisfies all four. One that
> consults live world state satisfies none of them.

填充一个区块。返回非零表示已经填好；返回零表示模组填不了，宿主会写入空气，并且每个维度只提示一次。

**线程**，这一条和这个文件里的其他内容正好相反：宿主在它的**区块工作线程**上调用这个函数，同时可能有好几个，分别处理同一个维度的不同区块。宿主不做任何串行化。

- 它必须能和自己并发运行。
- 对同一个 (dim_id, chunk_x, chunk_z)，它必须永远给出同样的结果：区块生成一次就保存下来，以后生成的邻居如果用的是另一个结果，就会留下一条以后怎么编辑都去不掉的接缝。
- 它不能调用这个 API 上的任何其他槽位。那些槽位都是为服务器线程写的，从这里调用，会碰到一份没有任何人加锁保护的状态。
- 它不能让异常跨过边界。

只读取注册时交给它的数据的生成器，这四条都满足。查询实时世界状态的生成器，一条都不满足。

## 70cdc4c4f2

>
> Palette sink for scan_region_indexed: invoked once per distinct block state
> met in the region, before the first cell that uses it. Indices count from 0
> in the order of first appearance and are valid for that one call only.
>   name : block type name, e.g. "minecraft:stone".
>   snbt : full block serialization (name + states + version) as SNBT.

`scan_region_indexed` 的调色板回调：区域里遇到的每一种不同的方块状态调用一次，在第一个用到它的格子之前调用。索引按第一次出现的顺序从 0 开始计数，只在那一次调用内有效。

- `name`：方块类型名，例如 `"minecraft:stone"`。
- `snbt`：完整的方块序列化（名字、状态和版本），以 SNBT 表示。

## d2ae7a2e9c

> Cell sink for scan_region_indexed: one call per cell with its palette index.

`scan_region_indexed` 的格子回调：每一格调用一次，传入它的调色板索引。

## 9467504984

> One cell of edit_set_blocks: a position and an index into the palette the
>  same call passes.

`edit_set_blocks` 的一格：一个位置，加上同一次调用传入的调色板里的索引。

## 3bb8d1beb6

> Per-dimension behavior rules for md_set_dimension_rule.
>
>  These are deliberately NOT a mirror of any engine enum: they name things
>  the loader intercepts itself. Values are ABI — append only, never renumber.

`md_set_dimension_rule` 用的按维度行为规则。

它们有意**不**对应任何引擎枚举：它们命名的是加载器自己拦截的东西。取值属于 ABI，只能追加，不能重新编号。

## 637ebc44e7

> natural hostile spawns

敌对生物的自然生成

## 5eac866607

> natural passive spawns

友好生物的自然生成

## 742b9874b8

> spawns from mob spawners

刷怪笼的生成

## d4d96a32e9

> explosions damaging terrain

爆炸破坏地形

## 373153c76f

> fire spreading to neighbours

火焰向相邻方块蔓延

## 3d8606781e

> mobs changing blocks

生物改变方块

## 8f30e5aabb

> projectile spawns

生成弹射物

## ebf39f365e

> pistons moving blocks

活塞推动方块

## d463295cd5

> water/lava spreading

水和岩浆的流动扩散

## 45488b0a3c

> farmland trampled back to dirt

耕地被踩回泥土

## 11c73a2f21

> mounting boats/minecarts/animals

骑上船、矿车或动物

## 7deb3d919a

> The same value under the spelling a plot world uses. Both names are permanent:
> removing one is a deletion, which §2.2 makes advance both version numbers.

同一个值，用的是地皮世界的叫法。两个名字都是永久的：删掉其中一个就是一次删除，按 §2.2 要同时提升两个版本号。

## 51d4c5a950

> RETIRED since 26.20.3, use PIER_DIMRULE_PISTON_CROSS_CELL

从 26.20.3 起退役，请用 `PIER_DIMRULE_PISTON_CROSS_CELL`

## 67863c658a

> RETIRED since 26.20.3, use PIER_DIMRULE_ENTITY_CROSS_CELL

从 26.20.3 起退役，请用 `PIER_DIMRULE_ENTITY_CROSS_CELL`

## 981740a88d

> Return codes of md_add_dimension_pack and md_pack_inspect. A dimension id is
>  never negative, so a caller tells the two apart by sign.

`md_add_dimension_pack` 和 `md_pack_inspect` 的返回码。维度 id 从不为负，所以调用方靠正负号区分两者。

## 45184a9640

> absolute, contains "..", or leaves the server root

是绝对路径、含有 `..`，或者跑出了服务器根目录

## c0e86de4eb

> the config file cannot be opened

配置文件打不开

## 9fa6f7acc1

> not JSON, or a required key missing or malformed

不是 JSON，或者缺少某个必需的键，或者键的格式不对

## f60d75ba25

> the binary named by the config cannot be opened

配置里指定的二进制文件打不开

## 3cc896fcb6

> a section fails its hash or an index is out of range

某一段的哈希对不上，或者某个索引越界

## 11ccdd87ef

> spec kind, config type and binary magic disagree

描述里的 kind、配置里的 type 和二进制文件的魔数三者对不上

## 549d23dc4a

> the binary does not hash to the config's sha256

二进制文件的哈希和配置里的 sha256 不一致

## 53812573ba

> a pack kind or format version this host does not serve

这个宿主不支持的包类型或格式版本

## 35c6abe423

> a parameter or role outside what the pack allows

某个参数或角色超出了地形包允许的范围

## 0b1dc2263c

> a constraint of the pack fails with these values

用这些值检查时，地形包的某条约束不满足

## b95583c677

> the dimension height does not fit the pack

维度的高度和地形包不匹配

## fe865ea54b

> the name exists with another binary or terrain kind

这个名字已经存在，用的是另一个二进制文件或另一种地形

## 354b9a2cc1

> the spec could not be read or has no pack terrain

描述读不出来，或者里面没有地形包类的地形

## e288ffb6b2

> another host refusal; the log has the reason

宿主的其他拒绝；原因写在日志里

## 9e76f27da1

> Cross-mod event bus

跨模组事件总线

## 922e3a6a82

> See PierBusCb above for why the loader owns the table instead of mods
> exchanging pointers. All four are thread-safe; callbacks run on the
> publishing thread.
>
> A mod does not receive its own publishes. Two reasons: a mod that
> wants to notify itself has a direct function call available, and
> self-delivery is the one loop shape that no depth limit can distinguish
> from legitimate work. Cross-mod loops (A publishes → B's handler
> publishes → A's handler publishes →…) are caught by a depth cap
> instead; hitting it drops the innermost publish and logs once.

加载器为什么自己持有订阅表、不让模组之间交换指针，见上面的 `PierBusCb`。这四个槽位都是线程安全的；回调在发布方的线程上运行。

模组收不到自己发布的消息。原因有两个：想通知自己的模组可以直接调用自己的函数；而自己投递给自己，是唯一一种任何深度上限都分不清它和正常工作的循环。跨模组的循环（A 发布 → B 的处理函数发布 → A 的处理函数发布 → ……）由深度上限截住：碰到上限时，最内层的那次发布被丢弃，并记一次日志。

## f18a6d3fcd

> Cross-mod service registry (query-style calls)

跨模组服务注册表（查询式调用）

## ab9ded2fc2

> The bus is one-way broadcast; this is request/response. The shapes differ
> on every axis, which is why they are separate tables rather than one:
>
>   - providers per name: bus any / service exactly one
>   - nobody registered:  bus normal / service an error the caller handles
>   - return value:       bus none / service the entire point
>   - ordering:           bus undefined and must not matter / service n/a
>
> Registration is EXCLUSIVE. Two mods answering `plot:can` is not "both
> run" — it is an ambiguous answer with no way for the caller to pick, so
> the second registrar is refused loudly. Silent last-wins would make the
> answer depend on mod load order, which nobody controls and which changes
> when an unrelated mod is installed.
>
> Ownership follows the same weak_ptr + ticket discipline as the bus and
> the forms: the loader keeps the table, and the call path revalidates the
> provider immediately before crossing into its dylib.
>
> Synchronous, on the caller's thread, no timeout. A provider that blocks
> blocks the server thread exactly like any other callback; returning
> "timed out" while the callback kept running would hand the caller a wrong
> answer AND leave the provider running.

总线是单向广播；服务是请求和响应。两者在每个方面的形状都不同，所以分成两张表，没有合成一张：

- 每个名字的提供方：总线任意多个；服务恰好一个
- 没人注册时：总线一切正常；服务是调用方要处理的错误
- 返回值：总线没有；服务的全部意义就在返回值
- 顺序：总线没有定义，也不能依赖；服务不涉及

注册是**独占**的。两个模组都回答 `plot:can`，得到的是一个有歧义的答案，调用方没办法选，所以第二个注册的会被明确拒绝。如果悄悄地以最后一个为准，答案就取决于模组的加载顺序，而加载顺序没有人控制，装一个不相干的模组都可能改变它。

归属的做法和总线、表单一样，用 `weak_ptr` 加票据：加载器持有表，调用路径在进入提供方的 dylib 之前，立刻重新确认提供方还在。

同步执行，在调用方的线程上，没有超时。提供方阻塞时，和其他任何回调一样阻塞服务器线程；如果回调还在运行就返回「超时」，调用方拿到的是一个错误的答案，提供方也照样还在运行。

## bbcbca9bfe

> Subscribe `mod` to `topic`. Returns a subscription id (>0), or 0 if the
>  topic is empty/oversized, the callback is null, or the mod is unknown.
>  Subscriptions are dropped automatically when the mod unloads.

让 `mod` 订阅 `topic`。返回订阅 id（大于 0）；主题为空或过长、回调为空，或者模组不认识时返回 0。模组卸载时，订阅会自动删除。

## fb14e23ab0

> Register `mod` as the provider of `name`. Returns a registration id
>  (>0), or 0 if the name is empty/oversized/already taken, the callback is
>  null, or the mod is unknown. Dropped automatically on unload.

把 `mod` 注册为 `name` 的提供方。返回注册 id（大于 0）；名字为空、过长或者已被占用，回调为空，或者模组不认识时返回 0。卸载时自动删除。

## eaf4423a2d

> Publish a lane. Exclusive, same discipline as service_register: if the name
>  is taken, return 0 and name the incumbent in the log. Returns a publish
>  id (> 0), withdrawn automatically at unload.

发布一个快速通道。和 `service_register` 一样是独占的：名字已被占用时返回 0，并在日志里写出现在占着这个名字的模组。返回发布 id（大于 0），卸载时自动撤回。

## e976c3c58d

> Acquire a lane. want_fingerprint must be the value the consumer computed
>  itself.
>
>  0 is not a valid fingerprint and always yields PIER_LANE_FINGERPRINT.
>  It must not mean "skip the check": this call hands over raw vtable and
>  data pointers, which the consumer then calls through its own table
>  offsets, so skipping the check is type confusion. To inspect which lanes
>  exist and what their fingerprints are, use lane_list, which hands over
>  no pointers at all.
>
>  Returns a PIER_LANE_* value. After a successful acquire, lane_release is
>  mandatory, or the provider's state stays retained.

获取一个快速通道。`want_fingerprint` 必须是使用方自己算出来的值。

0 不是有效的指纹，总是得到 `PIER_LANE_FINGERPRINT`。它不能表示「跳过检查」：这个调用交出的是原始的函数表指针和数据指针，使用方随后按自己的表偏移去调用，跳过检查就是类型混淆。要查看有哪些通道、它们的指纹是什么，请用 `lane_list`，它不交出任何指针。

返回一个 `PIER_LANE_*` 值。获取成功以后必须调用 `lane_release`，否则提供方的状态会一直被保留。

## bc245ee1ea

> Who is calling the service callback that is running right now.
>
>  A service callback receives a request and nothing about its sender, so a provider
>  that keys anything on a name inside the request (an owner, an acting player) is
>  trusting the request to tell the truth. This slot lets the provider ask the host
>  instead: inside a PierServiceCb, the sink receives the manifest name of the mod
>  whose service_call is on the stack. Calls nest, and the innermost one is reported.
>
>  Outside a callback, or when the call came without a mod handle, the sink is not
>  called at all; a provider then knows it cannot attribute the request rather than
>  attributing it to an empty name. Reads a thread-local, so any thread; a callback
>  that hops threads loses the attribution, which is the correct answer there.

正在运行的这个服务回调，是谁调用的。

服务回调只收到请求，不知道发送方是谁，所以按请求里的某个名字（所有者、执行操作的玩家）做判断的提供方，等于相信请求说的是实话。这个槽位让提供方可以改为去问宿主：在 `PierServiceCb` 里，输出回调收到调用栈上那次 `service_call` 所属模组的清单名。调用会嵌套，报告的是最内层的那一次。

在回调之外，或者调用没有带模组句柄时，输出回调一次都不会被调用；提供方由此知道它无法确定请求来自谁，不会把请求算到一个空名字头上。它读的是线程局部变量，所以可以在任何线程调用；回调换了线程就会丢掉归属，在那种情况下这正是正确的回答。

## d949a2f39b

> Lane protocol version. Kept separate from PIER_ABI_VERSION: the lane shape
>  evolves independently, and a mismatch is handled differently (reject this one
>  lane, not the whole mod).

快速通道的协议版本。和 `PIER_ABI_VERSION` 分开：通道的形状独立演进，不匹配时的处理也不一样（只拒绝这一条通道，不拒绝整个模组）。

## 5df087b74f

>
> Subscriber callback. `topic` and `payload` are borrowed for the duration of
> the call. Anything kept past it must be copied.
>
> The return value is a veto, and only for `bus_publish_vetoable`:
>   true  = "refuse this",
>   false = "no opinion".
> It is ignored entirely by `bus_publish`. There is deliberately no way to
> turn a refusal back into an approval: a subscriber can only tighten, never
> loosen. Letting one mod override another's refusal means the *last*
> subscriber to run decides, and subscriber order is not something either mod
> controls.
>
> Called on the thread that published. Never called after the owning mod is
> unloaded or while it is disabled.

订阅者回调。`topic` 和 `payload` 在调用期间是借来的，调用之后还要用的东西必须复制一份。

返回值是否决，只对 `bus_publish_vetoable` 有效：

- true = 「拒绝这件事」
- false = 「没有意见」

`bus_publish` 完全忽略返回值。有意没有提供把拒绝变回同意的办法：订阅者只能收紧，永远不能放松。如果允许一个模组推翻另一个模组的拒绝，那就是**最后**运行的订阅者说了算，而订阅者的顺序，两个模组都控制不了。

在发布消息的那个线程上调用。所属模组卸载以后，或者被禁用期间，永远不会被调用。

## faaf8208cc

>
> Provider callback for the cross-mod service registry (query-style calls,
> as opposed to the bus's one-way broadcast).
>
> Write the answer through `reply(ctx, ...)` — exactly once — and return true.
> Return false to report failure; anything written first is handed to the
> caller as the error text, which is what makes "no such plot" and "the
> database is down" distinguishable at the call site.
>
> `request` and `reply` are opaque UTF-8 the two mods agree on out of band. The
> loader never looks inside either.
>
> Runs synchronously on the CALLING thread, inside `service_call`. Never called
> after the providing mod is unloaded. It IS still called while the provider is
> merely disabled: `service_call` deliberately does not consult isEnabled()
> (see Services.cpp) because LeviLamina enables mods only after every on_load
> has run, and a service that is unreachable during that window breaks every
> consumer that resolves it in its own on_load.

跨模组服务注册表的提供方回调（查询式调用，和总线的单向广播不同）。

通过 `reply(ctx, ...)` 写入回答，正好一次，然后返回 true。返回 false 表示失败；在此之前写入的内容会作为错误文本交给调用方，所以调用处能分清「没有这块地皮」和「数据库挂了」。

`request` 和 `reply` 是两个模组在别处约定好的不透明 UTF-8。加载器从不查看里面的内容。

在**调用方**的线程上、在 `service_call` 内部同步运行。提供方模组卸载以后永远不会被调用。提供方只是被禁用时**仍然**会被调用：`service_call` 有意不检查 `isEnabled()`（见 Services.cpp），因为 LeviLamina 要等所有的 `on_load` 都运行完才启用模组，服务如果在这段时间里够不着，每一个在自己的 `on_load` 里解析它的使用方都会出问题。

## de0ff16f36

>
> Reference-count hooks, executed inside the provider's own dylib.
>
> The loader calls them from lane_acquire and lane_release, and calls release
> for every outstanding lease when the provider unloads or calls
> lane_unpublish. Publishing itself does not hold a count: the loader never
> calls release for the data handed to lane_publish, and the provider reclaims
> that itself after unpublishing.
>
> These must not call back into the loader (any lane_* slot); that self-
> deadlocks. A typical implementation is one atomic increment or decrement on
> the provider's own refcount, touching no lock.

引用计数的钩子，在提供方自己的 dylib 里执行。

加载器在 `lane_acquire` 和 `lane_release` 里调用它们；提供方卸载或者调用 `lane_unpublish` 时，加载器对每一个还没归还的租约调用 `release`。发布本身不占一个计数：加载器从不为交给 `lane_publish` 的数据调用 `release`，提供方在撤回之后自己回收它。

它们不能回头调用加载器（任何 `lane_*` 槽位），那样会自己锁死自己。典型的实现是对提供方自己的引用计数做一次原子加或减，不碰任何锁。

## bba1859ab2

> How a provider describes a lane when publishing it. Every field is filled in
>  by the provider; the loader only carries it.

提供方发布快速通道时用来描述它的结构。每个字段都由提供方填写，加载器只负责传递。

## 71b36896f9

> What lane_acquire produces.

`lane_acquire` 产出的结果。

## e5f9beca26

> service_call return codes.

`service_call` 的返回码。

## f4d52155f2

> provider ran and wrote a reply

提供方运行了，并写了回答

## b6302cb636

> nobody provides this name (or is disabled/unloaded)

没有模组提供这个名字（或者提供方已被禁用、已卸载）

## 160d6a8758

> provider returned false; reply holds its message

提供方返回了 false；回答里是它的错误信息

## c3c3359c52

> bad name, self-call, or call-depth limit

名字不合法、调用了自己，或者超过了调用深度上限

## c3d7543156

> lane_acquire return values.

`lane_acquire` 的返回值。

## ce51fe80db

> acquired; out is filled in

获取成功；`out` 已经填好

## 8d357470b9

> nobody published this name (that mod is not installed)

没有模组发布这个名字（那个模组没有安装）

## c81e1f3afc

> published, but the fingerprint differs; degrade, hand over nothing

已经发布，但指纹不同；降级处理，什么都不交出

## d38e635392

> bad name, self-acquire, provider disabled, or protocol mismatch

名字不合法、获取了自己的通道、提供方被禁用，或者协议不匹配

## 103d9f3f36

> Registries

注册表

## 6c31567e0b

> Lists one of the engine's registries through `sink`, one JSON object per entry, in
>  no particular order. `kind` is a PIER_REGISTRY_* value.
>
>  A block entry carries the type name, the creative category as the engine numbers
>  it, whether the type is solid, vanilla, a container, a signal source, a fence, a
>  rail, a slab, a wall or a crop, whether it holds a block entity, the bare-hand
>  destroy speed of its default state (negative for a block nothing breaks), its
>  explosion resistance, the light it emits and its description id.
>
>  An item entry carries the full name, the base rarity (0 common to 3 epic), the
>  maximum stack size, the creative category and whether commands hide it.
>
>  An entity entry carries the identifier, whether it has a spawn egg, whether it is
>  summonable, whether an experiment gates it, and the engine's actor type number,
>  whose bits say whether it is a mob, a monster, an animal or a water animal.
>
>  What a listing contains is the running game's own content, so a mod that picks
>  "a random block" or "a random rare item" decides by rules over it rather than by a
>  list it maintains. Returns false for an unknown kind, a null sink, or a registry
>  that does not exist yet; nothing is sunk then.

通过 `sink` 列出引擎的某一个注册表，每一项一个 JSON 对象，顺序不定。`kind` 是某个 `PIER_REGISTRY_*` 值。

方块的每一项包含：类型名；引擎给的创造模式分类编号；这个类型是否是实心的、原版的、容器、信号源、栅栏、铁轨、台阶、墙或作物；是否带有方块实体；默认状态下徒手挖掘的速度（什么都挖不动的方块为负数）；爆炸抗性；发出的光照；以及描述 id。

物品的每一项包含：完整的名字；基础稀有度（0 普通到 3 史诗）；最大堆叠数；创造模式分类；以及命令是否隐藏它。

实体的每一项包含：标识符；是否有刷怪蛋；是否可以召唤；是否受实验性玩法控制；以及引擎的实体类型编号，编号的各个位表示它是不是生物、怪物、动物或水生动物。

列出的是正在运行的游戏自己的内容，所以要挑「一个随机方块」或者「一个随机稀有物品」的模组，可以按规则从里面挑，不必自己维护一份列表。`kind` 不认识、`sink` 为空，或者注册表还不存在时返回 false，这时什么都不会输出。

## d2a5d86be8

> registry_list kinds: which of the engine's registries to list.

`registry_list` 的种类：列出引擎的哪一个注册表。

## c9d0817f82

> every block type

所有方块类型

## 463e8173c8

> every item

所有物品

## f3a747d722

> every actor the level knows

关卡知道的所有实体

## fe3df06a55

>
> UTF-8 string view. An explicit {pointer, length} struct, not an alias for any
> language's string type: the layout is defined by this declaration alone and
> does not depend on either side's standard library. The zero-copy conversion
> to and from std::string_view lives in pier-support.

UTF-8 字符串视图。它是一个显式的 {指针, 长度} 结构体，没有用任何语言的字符串类型做别名：它的布局只由这一份声明决定，不依赖任何一侧的标准库。和 `std::string_view` 之间的零拷贝转换放在 pier-support 里。

## 5a0b3d1ddf

> Generic string sink: receives a string within the current call frame.

通用的字符串输出回调：在当前调用帧内收到一个字符串。

## 148be29421

> A player's feet position + dimension. `found` is false if no such player.

一名玩家的脚下位置加上所在维度。没有这名玩家时 `found` 为 false。

## 05511ab07d

>
> Block sink: invoked once per cell during scan_region.
>   x, y, z : the cell's world coordinates.
>   name    : block type name, e.g. "minecraft:redstone_wire".
>   snbt    : full block serialization (name + states + version) as SNBT.

方块回调：`scan_region` 期间每一格调用一次。

- `x, y, z`：这一格的世界坐标。
- `name`：方块类型名，例如 `"minecraft:redstone_wire"`。
- `snbt`：完整的方块序列化（名字、状态和版本），以 SNBT 表示。

## c7a08df6d5

>
> Entity sink: invoked once per entity whose position falls inside the region.
>   x, y, z : the block cell that contains the entity (floor of its position).
>   type    : entity type name, e.g. "minecraft:creeper".
>   snbt    : the entity's serialized NBT (Actor::save) as SNBT.

实体回调：位置落在区域里的每个实体调用一次。

- `x, y, z`：包含这个实体的方块格（它的位置向下取整）。
- `type`：实体类型名，例如 `"minecraft:creeper"`。
- `snbt`：这个实体序列化后的 NBT（`Actor::save`），以 SNBT 表示。

## f87aa348b6

> Slot sink for container_get_items: one call per slot, in slot order.

`container_get_items` 的槽位回调：每个槽位调用一次，按槽位的顺序。

## 7bca517113

>
> Player selector — the identifier half of the "handles are identifiers,
> not pointers" rule. Resolved against the live player list on every call.
>   kind: 0 = name (getRealName, falling back to getNameTag),
>         1 = xuid, 2 = uuid (canonical string form).

玩家选择器，是「句柄用标识符，不用指针」这条规则里标识符的那一半。每次调用都对照当前的在线玩家列表重新解析。

- `kind`：0 = 名字（`getRealName`，对不上时退回 `getNameTag`），1 = xuid，2 = uuid（标准的字符串形式）。

## 69147c172a

> ActorUniqueID raw value. 0 / negative-invalid never resolves.

`ActorUniqueID` 的原始值。0 和负数是无效值，永远解析不到。

## 4e5e2e46a6

>
> Container reference — "owner + which container".
>   which: 0=inventory 1=ender_chest 2=armor 3=offhand 4=block container.
>   player: valid for which 0..3.   dim/x/y/z: valid for which == 4.

容器引用，即「所有者 + 哪一个容器」。

- `which`：0=物品栏，1=末影箱，2=盔甲，3=副手，4=方块容器。
- `player`：`which` 为 0 到 3 时有效。`dim`/`x`/`y`/`z`：`which == 4` 时有效。

## eec3a9b631

> Actor sink (list_actors).

实体回调（`list_actors`）。
