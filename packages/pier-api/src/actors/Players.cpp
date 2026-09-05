/** actors/Players.cpp: player management, properties and actions.
 *
 * A player handle is a selector, by name, xuid or uuid, re-resolved against the online
 * table on every call. A pointer is never cached. Version-sensitive writes go through
 * a native call or a native packet.
 */
#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "mc/deps/core/math/Vec2.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/core/string/HashedString.h"
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/network/MinecraftPacketIds.h"
#include "mc/network/MinecraftPackets.h"
#include "mc/network/NetworkPeer.h"
#include "mc/network/packet/RemoveObjectivePacket.h"
#include "mc/network/packet/ScorePacketInfo.h"
#include "mc/network/packet/ScorePacketType.h"
#include "mc/network/packet/SetDisplayObjectivePacket.h"
#include "mc/network/packet/SetScorePacket.h"
#include "mc/network/packet/SetTitlePacket.h"
#include "mc/network/packet/SetTitlePacketPayload.h"
#include "mc/network/packet/TextPacket.h"
#include "mc/network/packet/TextPacketPayload.h"
#include "mc/network/packet/TextPacketType.h"
#include "mc/network/packet/UpdateAbilitiesPacket.h"
#include "mc/platform/UUID.h"
#include "mc/server/commands/PlayerPermissionLevel.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorHurtResult.h"
#include "mc/world/actor/player/Abilities.h"
#include "mc/world/actor/player/AbilitiesIndex.h"
#include "mc/world/actor/player/LayeredAbilities.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/attribute/Attribute.h"
#include "mc/world/attribute/AttributeInstance.h"
#include "mc/world/attribute/AttributeInstanceConstRef.h"
#include "mc/world/attribute/AttributeInstanceForwarder.h"
#include "mc/world/attribute/MutableAttributeWithContext.h"
#include "mc/world/gamemode/InteractionResult.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/DimensionType.h"
#include "mc/world/scores/IdentityDefinition.h"
#include "mc/world/scores/ObjectiveSortOrder.h"
#include "mc/world/scores/ScoreboardId.h"

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
        /**
         * Objective name to an exclusive band of ScoreboardId slot numbers.
         *
         * Every sidebar row is a FakePlayer score entry keyed by ScoreboardId. A single
         * constant band for all objectives makes row N of two plugins the same entry,
         * so they overwrite each other and the screen shows interleaved content with
         * two sets of scores on the right.
         *
         * FNV-1a spreads the name over [0, 2^18) and the result is multiplied by a
         * stride of 4096 rows. The same name always lands in the same band, so setting
         * it repeatedly is idempotent and does not drift, and different names almost
         * always land in different bands.
         */
        uint32_t objectiveSlotHash(std::string_view name)
        {
            uint32_t h = 2166136261u;
            for (char c : name)
            {
                h ^= static_cast<unsigned char>(c);
                h *= 16777619u;
            }
            // 2^18 bands times 4096 rows is 2^30, which exactly fills one quadrant
            // above 0x40000000.
            return h & 0x3FFFFu;
        }

        /// Splits on '\n'. Used by the two sidebar actions, whose entire payload is one
        /// newline-joined string. One FFI string beats N calls, and a sidebar is
        /// rebuilt as a whole anyway.
        std::vector<std::string> splitLines(std::string_view text)
        {
            std::vector<std::string> out;
            while (true)
            {
                auto const nl = text.find('\n');
                if (nl == std::string_view::npos)
                {
                    out.emplace_back(text);
                    return out;
                }
                out.emplace_back(text.substr(0, nl));
                text.remove_prefix(nl + 1);
            }
        }

        /**
         * What each player's client currently shows per objective: the title and the
         * rows. Knowing it, an unchanged call sends nothing, a changed row sends one
         * SetScore for that row, and only a change in the row count rebuilds the
         * objective, so a per-second refresh costs neither three packets nor a visible
         * flicker. Keyed by unique id and objective name, server thread only. An entry
         * is dropped on clear and the table is bounded, since a departed player leaves
         * one behind.
         */
        struct SidebarState
        {
            /** The Player object the rows were sent to. A rejoin creates a new object
             *  for the same unique id, and its client starts empty, so a state recorded
             *  for the old object must not count as already shown. */
            Player const* owner = nullptr;
            std::string title;
            std::vector<std::string> rows;
        };
        using SidebarKey = std::pair<int64_t, std::string>;
        struct SidebarKeyHash
        {
            size_t operator()(SidebarKey const& k) const noexcept
            {
                return std::hash<int64_t>{}(k.first) ^ (std::hash<std::string>{}(k.second) << 1);
            }
        };
        std::unordered_map<SidebarKey, SidebarState, SidebarKeyHash> gSidebars;

        /** Drops every entry at once past a bound. Nothing tells this file that a player
         *  left, so without it the table grows by one entry per visitor for the lifetime
         *  of the process. Losing an entry costs one full rebuild of that sidebar. */
        void forgetOldSidebars()
        {
            if (gSidebars.size() > 4096) gSidebars.clear();
        }

        void api_list_players(void* ctx, PierStrSink snbtSink)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level || !snbtSink) return;
                level->forEachPlayer([&](Player& p)
                {
                    snbtSink(ctx, ps(bridge::playerSummarySnbt(p)));
                    return true;
                });
            PIER_API_GUARD_END_VOID
        }

        bool api_player_resolve(PierPlayerSel sel, PierActorId* out)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p || !out) return false;
                *out = p->getOrCreateUniqueID().rawID;
                return true;
            PIER_API_GUARD_END
        }

        bool api_player_send_message(PierPlayerSel sel, PierStr msg)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p) return false;
                p->sendMessage(sv(msg));
                return true;
            PIER_API_GUARD_END
        }

        bool api_player_send_message_typed(PierPlayerSel sel, PierStr msg, int32_t type)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p) return false;

                // Maps the ABI integer onto TextPacketType. An out-of-range value falls
                // back to Raw, one plain line of text on the client, rather than being
                // refused.
                auto ptype = TextPacketType::Raw;
                if (type >= 0 && type <= 11)
                    ptype = static_cast<TextPacketType>(static_cast<uchar>(type));

                // The body shape has to match the type on the wire: Chat and Whisper
                // carry an author, Translate carries parameters, and everything else is
                // a bare message. A MessageOnly body under a Chat type makes the client
                // read the message as the author and the next field as the text.
                TextPacket pkt{};
                switch (ptype)
                {
                case TextPacketType::Chat:
                case TextPacketType::Whisper:
                {
                    // No author was given, so the server speaks; the client shows
                    // "<Server> text" for Chat and the whisper form for Whisper.
                    TextPacketPayload::AuthorAndMessage body{ptype, std::string{"Server"}, toString(msg)};
                    pkt.mBody = body;
                    break;
                }
                case TextPacketType::Translate:
                {
                    TextPacketPayload::MessageAndParams body{ptype, toString(msg), std::vector<std::string>{}};
                    pkt.mBody = body;
                    break;
                }
                default:
                {
                    TextPacketPayload::MessageOnly body;
                    body.mType = ptype;
                    body.mMessage->assign(sv(msg));
                    pkt.mBody = body;
                    break;
                }
                }

                p->sendNetworkPacket(pkt);
                return true;
            PIER_API_GUARD_END
        }

        bool api_player_disconnect(PierPlayerSel sel, PierStr reason)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p) return false;
                p->disconnect(sv(reason));
                return true;
            PIER_API_GUARD_END
        }

        void api_broadcast_message(PierStr msg)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level) return;
                std::string_view text = sv(msg);
                level->forEachPlayer([&](Player& p)
                {
                    p.sendMessage(text);
                    return true;
                });
            PIER_API_GUARD_END_VOID
        }

        bool api_player_set_gamemode(PierPlayerSel sel, int32_t mode)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p) return false;
                // mode carries the engine GameType discriminant directly, where 0 is
                // survival, 1 creative, 2 adventure and 6 spectator. Only a whitelist
                // check happens here and it is not translated into a command name.
                //
                // That also removes the hazard of the command path, where a player name
                // concatenated into a quoted command lets a quote or a backslash in the
                // name tear the command apart.
                switch (mode)
                {
                case 0:
                case 1:
                case 2:
                case 6:
                    break;
                default:
                    return false;
                }
                p->setPlayerGameType(static_cast<::GameType>(mode));
                return true;
            PIER_API_GUARD_END
        }

        /**
         * Teleports a player, across dimensions when needed.
         *
         * Through Actor::teleport, the LeviLamina cross-dimension helper that
         * api_actor_action already uses, and not through /execute in <name> run tp.
         * execute in consumes a command enum built from the vanilla set, where a custom
         * dimension name is not necessarily a valid token, so the command may fail to
         * parse or parse into the wrong dimension. The native path is all that is
         * needed: the client already knows the custom dimension from
         * DimensionDataPacket and no packet rewriting layer stands in the way.
         *
         * The dimension id has no 0..2 cap, because custom dimension ids start at 3.
         */
        bool api_player_teleport(PierPlayerSel sel, int32_t dim, double x, double y, double z)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p) return false;

                auto const name = bridge::dimensionSelector(dim);
                if (name.empty()) return false;

                // blockSourceOf is both the force-build and the gate. The dimension
                // bridge in the dimensions package must check that the engine instance
                // it built carries the requested dim as its id (spi.h §6). If they
                // differ, sending a player into dim makes the engine throw an uncaught
                // exception on a chunk worker thread and fastfail with 0xC0000409,
                // which no teleport failure return can contain. The ledger knowledge
                // that check needs exists only there, so the gate lives in the bridge
                // and a non-null result is the only condition here.
                if (!bridge::blockSourceOf(dim)) return false;

                p->teleport(Vec3{(float)x, (float)y, (float)z}, DimensionType{dim}, p->getRotation());
                return true;
            PIER_API_GUARD_END
        }

        //  Attribute helpers

        /** Reads the current value of one attribute. Never yields NaN; a missing
         *  attribute sets ok to false. */
        bool readAttribute(Player& p, Attribute const& attr, double* out)
        {
            auto cref = p.getAttribute(attr);
            // mPtr is a scalar TypedStorage, a bare pointer with no .get() wrapper.
            auto* inst = cref.mPtr;
            if (!inst) return false;
            *out = static_cast<double>(inst->getCurrentValue());
            return true;
        }

        /**
         * Writes the current value of an attribute through AttributeInstanceForwarder,
         * so listeners fire and player-synced attributes reach the client.
         */
        bool writeAttribute(Player& p, Attribute const& attr, float value)
        {
            // getMutableAttribute binds the instance to a modification context and
            // exposes the forwarder through operator->. Its bool test guards the
            // not-present case.
            auto mut = p.getMutableAttribute(attr);
            if (!mut) return false;
            mut->setCurrentValue(value);
            return true;
        }

        //  Properties

        bool api_player_get_num(PierPlayerSel sel, int32_t prop, double* out)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p || !out) return false;
                switch (prop)
                {
                case PIER_PPROP_GAME_TYPE:
                    *out = static_cast<double>(static_cast<int>(p->getPlayerGameType()));
                    return true;
                case PIER_PPROP_LEVEL:
                    return readAttribute(*p, Player::LEVEL(), out);
                case PIER_PPROP_EXPERIENCE:
                    return readAttribute(*p, Player::EXPERIENCE(), out);
                case PIER_PPROP_HUNGER:
                    return readAttribute(*p, Player::HUNGER(), out);
                case PIER_PPROP_SATURATION:
                    return readAttribute(*p, Player::SATURATION(), out);
                case PIER_PPROP_EXHAUSTION:
                    return readAttribute(*p, Player::EXHAUSTION(), out);
                case PIER_PPROP_XP_NEEDED_NEXT_LEVEL:
                    *out = static_cast<double>(p->getXpNeededForNextLevel());
                    return true;
                case PIER_PPROP_LUCK:
                    *out = static_cast<double>(p->getLuck());
                    return true;
                case PIER_PPROP_SELECTED_SLOT:
                    *out = static_cast<double>(p->getSelectedItemSlot());
                    return true;
                case PIER_PPROP_IS_OPERATOR:
                    *out = p->isOperator() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_CAN_USE_OPERATOR_BLOCKS:
                    *out = p->canUseOperatorBlocks() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_IS_FLYING:
                    *out = p->isFlying() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_CAN_JUMP:
                    *out = p->canJump() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_IS_EMOTING:
                    *out = p->isEmoting() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_IS_IN_RAID:
                    *out = p->isInRaid() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_IS_HURT:
                    *out = p->isHurt() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_IS_SCOPING:
                    // Player::isScoping() sits behind #ifdef LL_PLAT_C in the generated
                    // headers and is unavailable in an ordinary build. This one property
                    // reports unsupported rather than failing to compile.
                    return false;
                case PIER_PPROP_CAN_SLEEP:
                    *out = p->canSleep() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_HAS_RESPAWN_POSITION:
                    *out = p->hasRespawnPosition() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_CLIENT_SUB_ID:
                    *out = static_cast<double>(static_cast<int>(p->getClientSubId()));
                    return true;
                /*  Appended: player gap fills  */
                case PIER_PPROP_DIRECTION:
                    *out = static_cast<double>(p->getDirection());
                    return true;
                case PIER_PPROP_CHUNK_RADIUS:
                    *out = static_cast<double>(p->getChunkRadius());
                    return true;
                case PIER_PPROP_NETWORK_RTT:
                {
                    // getNetworkStatus() returns optional<NetworkPeer::NetworkStatus>
                    // and mCurrentPing wraps chrono::milliseconds, so ->count() is used.
                    auto opt = p->getNetworkStatus();
                    if (!opt) return false;
                    *out = static_cast<double>(opt->mCurrentPing->count());
                    return true;
                }
                case PIER_PPROP_PLATFORM:
                    *out = static_cast<double>(static_cast<int>(p->getPlatform()));
                    return true;
                case PIER_PPROP_ENCHANTMENT_SEED:
                    *out = static_cast<double>(p->getEnchantmentSeed());
                    return true;
                case PIER_PPROP_IS_USING_ITEM:
                    *out = p->isUsingItem() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_IS_BLOCKING:
                    *out = p->isBlocking() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_IS_GLIDING:
                    *out = p->isGliding() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_IS_SWIMMING:
                    *out = p->isSwimming() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_PERMISSION_LEVEL:
                    *out = static_cast<double>(static_cast<int>(p->getPlayerPermissionLevel()));
                    return true;
                case PIER_PPROP_SCORE:
                    // Player has no getScore(). The value lives in the public member
                    // mScore, a TypedStorage<int> that collapses to a bare int on use.
                    *out = static_cast<double>(p->mScore);
                    return true;
                case PIER_PPROP_FALL_DISTANCE:
                    *out = static_cast<double>(p->getFallDistance());
                    return true;
                case PIER_PPROP_IS_DEAD:
                    *out = p->isDead() ? 1.0 : 0.0;
                    return true;
                case PIER_PPROP_HAS_DIED_BEFORE:
                    *out = p->hasDiedBefore() ? 1.0 : 0.0;
                    return true;
                // The dimension the player is currently in. A custom dimension has an
                // id of 3 or above, so a caller must not assume only 0, 1 and 2. The
                // form matches the one already in Actors.cpp, since Player derives from
                // Actor.
                case PIER_PPROP_DIMENSION:
                    *out = static_cast<double>(static_cast<int>(p->getDimensionId()));
                    return true;
                default:
                    return false;
                }
            PIER_API_GUARD_END
        }

        bool api_player_get_str(PierPlayerSel sel, int32_t prop, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p || !sink) return false;
                switch (prop)
                {
                case PIER_PSTR_REAL_NAME:
                    sink(ctx, ps(p->getRealName()));
                    return true;
                case PIER_PSTR_UUID:
                    sink(ctx, ps(p->getUuid().asString()));
                    return true;
                case PIER_PSTR_XUID:
                    sink(ctx, ps(p->getXuid()));
                    return true;
                case PIER_PSTR_IP_AND_PORT:
                    sink(ctx, ps(p->getIPAndPort()));
                    return true;
                case PIER_PSTR_LOCALE_CODE:
                    sink(ctx, ps(p->getLocaleCode()));
                    return true;
                case PIER_PSTR_NAME_TAG:
                    sink(ctx, ps(p->getNameTag()));
                    return true;
                /*  Appended  */
                case PIER_PSTR_LAST_DEATH_POS:
                {
                    auto pos = p->getLastDeathPos();
                    if (!pos.has_value())
                    {
                        sink(ctx, ps(std::string_view{}));
                        return true;
                    }
                    std::string snbt = "{x:" + snbtNum(pos->x) + ",y:" + snbtNum(pos->y)
                        + ",z:" + snbtNum(pos->z) + "}";
                    sink(ctx, ps(snbt));
                    return true;
                }
                case PIER_PSTR_LAST_DEATH_DIMENSION:
                {
                    auto dim = p->getLastDeathDimension();
                    if (!dim.has_value())
                    {
                        sink(ctx, ps(std::string_view{}));
                        return true;
                    }
                    sink(ctx, ps(snbtNum(static_cast<int>(*dim))));
                    return true;
                }
                case PIER_PSTR_NETWORK_STATUS:
                {
                    // NetworkStatus fields: mCurrentPing and mAveragePing wrap
                    // chrono::milliseconds and need ->count(), while the packet loss
                    // fields are bare floats used directly. The return is an optional
                    // and must be tested.
                    auto opt = p->getNetworkStatus();
                    if (!opt) return false;
                    auto const& ns = *opt;
                    std::string snbt = "{ping:" + snbtNum(ns.mCurrentPing->count());
                    snbt += ",avg_ping:" + snbtNum(ns.mAveragePing->count());
                    snbt += ",packet_loss:" + snbtDouble(ns.mCurrentPacketLoss);
                    snbt += ",avg_packet_loss:" + snbtDouble(ns.mAveragePacketLoss);
                    snbt += ",max_bps:" + snbtDouble(ns.mApproximateMaxBps) + "}";
                    sink(ctx, ps(snbt));
                    return true;
                }
                case PIER_PSTR_PLATFORM_ONLINE_ID:
                    sink(ctx, ps(p->getPlatformOnlineId()));
                    return true;
                default:
                    return false;
                }
            PIER_API_GUARD_END
        }

        bool api_player_set_num(PierPlayerSel sel, int32_t prop, double v)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p) return false;
                switch (prop)
                {
                case PIER_PPROP_LEVEL:
                    return writeAttribute(*p, Player::LEVEL(), static_cast<float>(v));
                case PIER_PPROP_EXPERIENCE:
                    return writeAttribute(*p, Player::EXPERIENCE(), static_cast<float>(v));
                case PIER_PPROP_HUNGER:
                    return writeAttribute(*p, Player::HUNGER(), static_cast<float>(v));
                case PIER_PPROP_SATURATION:
                    return writeAttribute(*p, Player::SATURATION(), static_cast<float>(v));
                case PIER_PPROP_EXHAUSTION:
                    return writeAttribute(*p, Player::EXHAUSTION(), static_cast<float>(v));
                default:
                    return false; // Read-only or unknown
                }
            PIER_API_GUARD_END
        }

        //  Actions

        /**
         * Sets one ability bit, dispatching bool and float correctly.
         * AbilitiesIndex runs 0..19 and the float slots are 13 FlySpeed, 14 WalkSpeed
         * and 19 VerticalFlySpeed, so dispatch cannot be by index threshold. The float
         * path must use LayeredAbilities::setAbility(idx, float), because
         * Player::setAbility has only a bool overload a float silently converts to. An
         * UpdateAbilitiesPacket follows the write.
         * Writing any ability bit silently drops PlayerPermissionLevel to Custom and
         * ships it in the SerializedAbilitiesData of UpdateAbilitiesPacket, so the
         * client behaves as a visitor, with no block outline, no attacking and no local
         * placement prediction, while the server behaves normally and addSaveData
         * writes the level into the save. It is snapshotted, written and restored if
         * overridden; a caller that wants Custom asks through
         * PIER_PACT_SET_PERMISSION_LEVEL. */
        bool setPlayerPermissionLevel(Player& p, PlayerPermissionLevel level)
        {
            p.getAbilities().setPlayerPermissions(level);
            UpdateAbilitiesPacket pkt{p.getOrCreateUniqueID(), p.getAbilities()};
            p.sendNetworkPacket(pkt);
            return true;
        }

        bool setPlayerAbility(Player& p, int idx, double value)
        {
            if (idx < 0 || idx >= static_cast<int>(AbilitiesIndex::AbilityCount))
            {
                return false;
            }
            auto index = static_cast<AbilitiesIndex>(idx);
            auto const before = p.getPlayerPermissionLevel();

            switch (index)
            {
            case AbilitiesIndex::FlySpeed:
            case AbilitiesIndex::WalkSpeed:
            case AbilitiesIndex::VerticalFlySpeed:
            {
                if (!p.getAbilities().setAbility(index, static_cast<float>(value)))
                {
                    return false;
                }
                // Pushes the whole layered set to the client. Speeds apply on the
                // client and do not change without the push.
                UpdateAbilitiesPacket pkt{p.getOrCreateUniqueID(), p.getAbilities()};
                p.sendNetworkPacket(pkt);
                break;
            }
            default:
            {
                bool const want = value != 0.0;
                p.setAbility(index, want);

                if (p.canUseAbility(index) != want)
                {
                    UpdateAbilitiesPacket resync{p.getOrCreateUniqueID(), p.getAbilities()};
                    p.sendNetworkPacket(resync);

                    hostLogger().error(
                        "[api] setPlayerAbility read back a different value after writing "
                        "idx={} want={}, so the bit did not take effect; the usual cause is "
                        "writing an ability bit from PlayerJoinEvent, before the player's "
                        "ability layers exist. Write it a second or two after join, or not "
                        "at all. A current state has been resent to the client",
                        idx,
                        want
                    );

                    if (idx <= static_cast<int>(AbilitiesIndex::AttackMobs))
                    {
                        return false;
                    }
                }
                break;
            }
            }

            if (p.getPlayerPermissionLevel() != before)
            {
                hostLogger().warn(
                    "[api] setPlayerAbility on idx={} pushed the permission level from {} "
                    "to {} and it has been restored; this indicates the engine behavior "
                    "changed",
                    idx,
                    static_cast<int>(before),
                    static_cast<int>(p.getPlayerPermissionLevel())
                );
                setPlayerPermissionLevel(p, before);
            }
            return true;
        }

        bool api_player_action(
            PierPlayerSel sel,
            int32_t action,
            PierStr sarg,
            double a,
            double b,
            double c,
            void* ctx,
            PierStrSink out)
        {
            PIER_API_GUARD_BEGIN
                Player* p = bridge::resolvePlayer(sel);
                if (!p) return false;
                switch (action)
                {
                case PIER_PACT_SET_ABILITY:
                {
                    int idx = static_cast<int>(a);
                    return setPlayerAbility(*p, idx, b);
                }
                case PIER_PACT_CAN_USE_ABILITY:
                {
                    int idx = static_cast<int>(a);
                    if (idx < 0 || idx >= static_cast<int>(AbilitiesIndex::AbilityCount)) return false;
                    bool can = p->canUseAbility(static_cast<AbilitiesIndex>(idx));
                    if (out) out(ctx, ps(std::string_view{can ? "1" : "0"}));
                    return true;
                }
                case PIER_PACT_SET_SELECTED_SLOT:
                {
                    int slot = static_cast<int>(a);
                    if (slot < 0 || slot > 8) return false;
                    p->setSelectedSlot(slot);
                    return true;
                }
                case PIER_PACT_GIVE_ITEM:
                {
                    auto opt = bridge::itemFromSnbt(sv(sarg));
                    if (!opt) return false;
                    ItemStack item = std::move(*opt);
                    if (item.isNull()) return false;
                    return p->addAndRefresh(item);
                }
                case PIER_PACT_SET_SPAWN_POINT:
                {
                    std::string dimStr = toString(sarg);
                    int dim = 0;
                    if (!dimStr.empty())
                    {
                        try
                        {
                            // Not clamped to 0..2. Clamping moves a respawn point in a
                            // custom dimension silently into the end.
                            dim = std::stoi(dimStr);
                        }
                        catch (...)
                        {
                            return false;
                        }
                    }
                    // Native. As above, no player name is concatenated into a command
                    // string.
                    p->setRespawnPosition(
                        BlockPos{static_cast<int>(a), static_cast<int>(b), static_cast<int>(c)},
                        static_cast<::DimensionType>(dim)
                    );
                    return true;
                }
                case PIER_PACT_CLEAR_TITLE:
                {
                    // A native packet. The command path would concatenate the player
                    // name into a quoted string, which a quote in the name tears apart,
                    // and /title also runs command parsing and a permission check, all
                    // of which is wasted on sending one packet to one player.
                    SetTitlePacketPayload payload{SetTitlePacketPayload::TitleType::Clear};
                    SetTitlePacket{std::move(payload)}.sendTo(*p);
                    return true;
                }
                case PIER_PACT_SET_TITLE:
                {
                    auto kind = static_cast<int>(a);
                    auto type = kind == 1
                                    ? SetTitlePacketPayload::TitleType::Subtitle
                                    : kind == 2
                                    ? SetTitlePacketPayload::TitleType::Actionbar
                                    : SetTitlePacketPayload::TitleType::Title;
                    // filteredTitleText is nullopt. That is the fallback text for chat
                    // filtering, and the text on this path comes from a mod rather than
                    // player input, so there is nothing to filter.
                    SetTitlePacketPayload payload{type, toString(sarg), std::nullopt};
                    SetTitlePacket{std::move(payload)}.sendTo(*p);
                    return true;
                }
                /*  Appended  */
                case PIER_PACT_ADD_EXPERIENCE:
                    p->addExperience(static_cast<int>(a));
                    return true;
                case PIER_PACT_ADD_LEVELS:
                    p->addLevels(static_cast<int>(a));
                    return true;
                case PIER_PACT_START_COOLDOWN:
                    // startItemCooldown(HashedString const&, int ticks, bool updateClient)
                    p->startItemCooldown(HashedString{toString(sarg)}, static_cast<int>(a), true);
                    return true;
                case PIER_PACT_START_RIDING:
                {
                    auto* vehicle = bridge::resolveActor(static_cast<PierActorId>(a));
                    if (!vehicle) return false;
                    // startRiding(Actor&, bool forceRiding), where force=true makes the
                    // request succeed even when the vehicle is full.
                    return p->startRiding(*vehicle, true);
                }
                case PIER_PACT_STOP_RIDING:
                    // stopRiding(bool exitFromPassenger, bool actorIsBeingDestroyed,
                    //            bool switchingVehicles, bool isBeingTeleported)
                    p->stopRiding(true, false, false, false);
                    return true;
                case PIER_PACT_ATTACK:
                {
                    auto* target = bridge::resolveActor(static_cast<PierActorId>(a));
                    if (!target) return false;
                    // attack(Actor&, ActorDamageCause const&). The caller named no
                    // source, so Override, the generic cause, is used.
                    p->attack(*target, ::SharedTypes::Legacy::ActorDamageCause::Override);
                    return true;
                }
                case PIER_PACT_DROP:
                {
                    auto opt = bridge::itemFromSnbt(sv(sarg));
                    if (!opt) return false;
                    return p->drop(std::move(*opt), a != 0.0);
                }
                case PIER_PACT_INTERACT:
                {
                    auto* target = bridge::resolveActor(static_cast<PierActorId>(a));
                    if (!target) return false;
                    // interact(Actor&, Vec3 const& location) returns an
                    // InteractionResult, whose mSuccess bit is returned as the bool.
                    auto result = p->interact(*target, target->getPosition());
                    return result.mSuccess;
                }
                case PIER_PACT_START_USING_ITEM:
                {
                    auto opt = bridge::itemFromSnbt(sv(sarg));
                    if (!opt) return false;
                    p->startUsingItem(std::move(*opt), static_cast<int>(a));
                    return true;
                }
                case PIER_PACT_STOP_USING_ITEM:
                    p->stopUsingItem();
                    return true;
                case PIER_PACT_SET_CHUNK_RADIUS:
                    p->setChunkRadius(static_cast<int>(a));
                    return true;
                case PIER_PACT_SET_ENCHANTMENT_SEED:
                    p->setEnchantmentSeed(static_cast<int>(a));
                    return true;
                case PIER_PACT_REGISTER_TRACKED_BOSS:
                {
                    auto* boss = bridge::resolveActor(static_cast<PierActorId>(a));
                    if (!boss) return false;
                    // registerTrackedBoss takes an ActorUniqueID and not an Actor
                    // reference.
                    p->registerTrackedBoss(boss->getOrCreateUniqueID());
                    return true;
                }
                case PIER_PACT_UNREGISTER_TRACKED_BOSS:
                {
                    auto* boss = bridge::resolveActor(static_cast<PierActorId>(a));
                    if (!boss) return false;
                    p->unRegisterTrackedBoss(boss->getOrCreateUniqueID());
                    return true;
                }
                case PIER_PACT_PLAY_EMOTE:
                    // playEmote(string const& pieceId, bool playChatMessage)
                    p->playEmote(toString(sarg), false);
                    return true;
                case PIER_PACT_RESEND_ALL_CHUNKS:
                    p->resendAllChunks();
                    return true;
                case PIER_PACT_OPEN_INVENTORY:
                    p->openInventory();
                    return true;
                /*  Appended: sidebar  */
                case PIER_PACT_SIDEBAR_SET:
                {
                    // sarg is "objective\ntitle\nline1\nline2...", affecting only this
                    // player's sidebar. The server Scoreboard is global, so a per-player
                    // board can only be packets the client never ties to real scoreboard
                    // state. They are built here rather than passed across the FFI as
                    // bytes: SetDisplayObjective and RemoveObjective are PayloadPackets
                    // with reflection serialization, and no mod can assemble that wire
                    // shape by hand and keep it valid across versions.
                    // api_player_send_title exists for the same reason.
                    auto const lines = splitLines(sv(sarg));
                    if (lines.size() < 2)
                    {
                        hostLogger().error("[api] sidebar payload has fewer than two lines, objective and title are required");
                        return false;
                    }
                    std::string const& objective = lines[0];
                    if (objective.empty())
                    {
                        hostLogger().error("[api] sidebar objective is empty");
                        return false;
                    }

                    // The client keys rows by scoreboard id, so a row set of the same
                    // size can be updated in place and only a changed size needs the
                    // teardown, since stale ids are exactly where leftover rows come from.
                    SidebarKey const skey{static_cast<int64_t>(p->getOrCreateUniqueID().rawID), objective};
                    auto known = gSidebars.find(skey);
                    bool const sameShape = known != gSidebars.end() && known->second.owner == p
                        && known->second.rows.size() == lines.size() - 2;
                    if (sameShape && known->second.title == lines[1]
                        && std::equal(lines.begin() + 2, lines.end(), known->second.rows.begin()))
                    {
                        return true; // Nothing changed since the last send
                    }

                    if (!sameShape)
                    {
                        if (auto gone = MinecraftPackets::createPacket(MinecraftPacketIds::RemoveObjective))
                        {
                            static_cast<RemoveObjectivePacket*>(gone.get())->mObjectiveName = objective;
                            p->sendNetworkPacket(*gone);
                        }
                        else
                        {
                            hostLogger().error("[api] sidebar createPacket(RemoveObjective) returned null");
                        }
                    }

                    if (!sameShape || known->second.title != lines[1])
                    {
                        auto shown = MinecraftPackets::createPacket(MinecraftPacketIds::SetDisplayObjective);
                        if (!shown)
                        {
                            hostLogger().error("[api] sidebar createPacket(SetDisplayObjective) returned null");
                            return false;
                        }
                        auto* d = static_cast<SetDisplayObjectivePacket*>(shown.get());
                        d->mDisplaySlotName = std::string{"sidebar"};
                        d->mObjectiveName = objective;
                        d->mObjectiveDisplayName = lines[1];
                        d->mCriteriaName = std::string{"dummy"};
                        d->mSortOrder = ObjectiveSortOrder::Descending;
                        p->sendNetworkPacket(*shown);
                    }

                    if (lines.size() == 2)
                    {
                        forgetOldSidebars();
                        gSidebars[skey] = SidebarState{p, lines[1], {}};
                        return true;
                    }

                    // ScoreboardId bands are separated by a hash of the objective name.
                    // With one fixed base, row 1 of two plugins lands on the same entry
                    // and the later sender overwrites the earlier, leaving interleaved
                    // content and two sets of scores neither side can clear. The stride
                    // of 4096 rows is far above MAX_ROWS, a collision has probability
                    // 1/(2^30/4096) and affects only two sidebars open at once. The
                    // high bits are fixed at 0x4 to avoid the low id band the vanilla
                    // scoreboard uses.
                    int64_t const kSidebarIdBase = INT64_C(0x40000000)
                        + (static_cast<int64_t>(objectiveSlotHash(objective)) * INT64_C(4096));
                    std::vector<ScorePacketInfo> infos;
                    infos.reserve(lines.size() - 2);
                    int score = static_cast<int>(lines.size()) - 2;
                    for (size_t i = 2; i < lines.size(); ++i, --score)
                    {
                        // Same shape: only the rows whose text changed go out.
                        if (sameShape && known->second.rows[i - 2] == lines[i]) continue;
                        ScorePacketInfo info{};
                        info.mScoreboardId->mRawID = kSidebarIdBase + static_cast<int64_t>(i - 1);
                        info.mObjectiveName = objective;
                        info.mScoreValue = score;
                        info.mIdentityType = IdentityDefinition::Type::FakePlayer;
                        info.mFakePlayerName = lines[i].empty() ? std::string{" "} : lines[i];
                        infos.push_back(std::move(info));
                    }

                    auto const rows = infos.size();
                    if (!infos.empty())
                    {
                        auto scores = MinecraftPackets::createPacket(MinecraftPacketIds::SetScore);
                        if (!scores)
                        {
                            hostLogger().error("[api] sidebar createPacket(SetScore) returned null");
                            return false;
                        }
                        auto* sp = static_cast<SetScorePacket*>(scores.get());
                        sp->mType = ScorePacketType::Change;
                        sp->mScoreInfo = std::move(infos);
                        p->sendNetworkPacket(*scores);
                    }
                    forgetOldSidebars();
                    gSidebars[skey] = SidebarState{
                        p, lines[1], std::vector<std::string>(lines.begin() + 2, lines.end())};

                    // Proof of delivery, logged once per objective rather than once
                    // globally. When two sidebars overwrite each other, the only thing
                    // worth knowing is whether each was sent and which id band it used.
                    static std::set<std::string> announcedObjectives;
                    if (announcedObjectives.insert(objective).second)
                    {
                        hostLogger().debug(
                            "[api] sidebar '{}' sent {} row(s) as FakePlayer entries, id band 0x{:x}..0x{:x}",
                            objective, rows,
                            static_cast<uint64_t>(kSidebarIdBase + 1),
                            static_cast<uint64_t>(kSidebarIdBase + static_cast<int64_t>(rows)));
                    }
                    return true;
                }
                case PIER_PACT_SIDEBAR_CLEAR:
                {
                    if (sarg.len == 0) return false;

                    // The display slot is unbound before the objective is removed. The
                    // reverse order clears nothing: the client drops the score entries
                    // while the slot stays bound to that name, so the old content
                    // remains and the slot stays occupied. Another plugin calling
                    // SIDEBAR_SET then sends a RemoveObjective for its own name only,
                    // cannot touch the stale binding, and its sidebar never appears. A
                    // SetDisplayObjective with an empty mObjectiveName means the slot
                    // displays nothing.
                    if (auto blank =
                            MinecraftPackets::createPacket(MinecraftPacketIds::SetDisplayObjective))
                    {
                        auto* d = static_cast<SetDisplayObjectivePacket*>(blank.get());
                        d->mDisplaySlotName = std::string{"sidebar"};
                        d->mObjectiveName = std::string{};
                        d->mObjectiveDisplayName = std::string{};
                        d->mCriteriaName = std::string{"dummy"};
                        d->mSortOrder = ObjectiveSortOrder::Descending;
                        p->sendNetworkPacket(*d);
                    }

                    auto gone = MinecraftPackets::createPacket(MinecraftPacketIds::RemoveObjective);
                    if (!gone) return false;
                    static_cast<RemoveObjectivePacket*>(gone.get())->mObjectiveName = toString(sarg);
                    p->sendNetworkPacket(*gone);
                    gSidebars.erase(SidebarKey{static_cast<int64_t>(p->getOrCreateUniqueID().rawID), toString(sarg)});

                    hostLogger().debug("[api] sidebar '{}' cleared, slot unbound and objective removed", sv(sarg));
                    return true;
                }
                case PIER_PACT_SET_PERMISSION_LEVEL:
                {
                    int lvl = static_cast<int>(a);
                    if (lvl < static_cast<int>(PlayerPermissionLevel::Visitor)
                        || lvl > static_cast<int>(PlayerPermissionLevel::Custom))
                    {
                        return false;
                    }
                    return setPlayerPermissionLevel(*p, static_cast<PlayerPermissionLevel>(lvl));
                }

                default:
                    return false;
                }
            PIER_API_GUARD_END
        }

        void fill(PierApi& api)
        {
            api.list_players = &api_list_players;
            api.player_resolve = &api_player_resolve;
            api.player_send_message = &api_player_send_message;
            api.player_send_message_typed = &api_player_send_message_typed;
            api.player_disconnect = &api_player_disconnect;
            api.broadcast_message = &api_broadcast_message;
            api.player_set_gamemode = &api_player_set_gamemode;
            api.player_teleport = &api_player_teleport;
            api.player_get_num = &api_player_get_num;
            api.player_get_str = &api_player_get_str;
            api.player_set_num = &api_player_set_num;
            api.player_action = &api_player_action;
        }

        spi::SlotPackReg reg{{"players", &fill}};
    } // namespace
} // namespace pier::api_impl
