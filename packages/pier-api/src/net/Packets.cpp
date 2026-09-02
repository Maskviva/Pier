/** net/Packets.cpp: per-connection packet delivery. Appended slots gated by
 *  struct_size.
 * Two layers. api_send_packet is the raw primitive: any MinecraftPacketIds plus a wire
 * format body, deserialized into a real packet object and handed to one player's
 * connection, so a plain send-a-packet need never touch the bridge again.
 * api_spawn_particle_for and api_player_send_title are typed derivatives of the same
 * delivery path, with the packet built on the C++ side, so no wire format crosses the
 * FFI and they are version safe, and they reuse the same delivery helper.
 * api_player_send_title exists because PACT_SET_TITLE on player_action would
 * shell out to /title "<name>" title <text>, which breaks on a quote in the name,
 * expands selectors inside the text, and cannot set the durations. No broadcast
 * variant is exposed on purpose; a mod that wants everyone loops over players.
 * Reading and editing existing packets lives in PacketHooks. This file only makes new
 * ones.
 */
#ifndef PIER_BUILD_CLIENT

#include <memory>
#include <set>
#include <string>
#include <string_view>

#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/core/utility/ReadOnlyBinaryStream.h"
#include "mc/network/MinecraftPacketIds.h"
#include "mc/network/MinecraftPackets.h"
#include "mc/network/Packet.h"
#include "mc/network/packet/SetTitlePacket.h"
#include "mc/network/packet/SetTitlePacketPayload.h"
#include "mc/network/packet/SpawnParticleEffectPacket.h"
#include "mc/world/actor/player/Player.h"

#include "sdk/abi.h"

#include "pier/api/bridge.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/log.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        /// Shared delivery: resolves the target and hands a ready packet to that one
        /// connection. Both the raw and the typed entry points end here.
        bool sendToPlayer(PierPlayerSel sel, Packet& pkt)
        {
            Player* p = bridge::resolvePlayer(sel);
            if (!p) return false;
            p->sendNetworkPacket(pkt);
            return true;
        }

        bool api_send_packet(PierPlayerSel sel, int32_t packetId, uint8_t const* body, size_t bodyLen)
        {
            PIER_API_GUARD_BEGIN
                if (!body && bodyLen != 0) return false;

                auto pkt = MinecraftPackets::createPacket(static_cast<MinecraftPacketIds>(packetId));
                if (!pkt) return false;

                // Deserializes the caller's body into the packet object. The stream
                // borrows the bytes with copyBuffer=false and is valid for this frame.
                std::string_view raw{reinterpret_cast<char const*>(body), bodyLen};
                ReadOnlyBinaryStream stream{raw, /*copyBuffer=*/false};
                if (!pkt->read(stream)) return false;
                // The body must be exactly one packet. Trailing garbage means the
                // caller serialized the wrong shape for this game version, so it is
                // refused early rather than sending a half-parsed packet to a client.
                if (!stream.ensureReadCompleted()) return false;

                return sendToPlayer(sel, *pkt);
            PIER_API_GUARD_END
        }

        bool api_player_send_title(
            PierPlayerSel sel, int32_t type, PierStr text, int32_t fadeInTicks, int32_t stayTicks,
            int32_t fadeOutTicks)
        {
            PIER_API_GUARD_BEGIN
                using TitleType = SetTitlePacketPayload::TitleType;

                // 6..8 are the TextObject variants, whose payload constructors take a
                // ResolvedTextObject, which is meaningless across this FFI. They are
                // refused rather than degraded silently to the plain string variant,
                // which is not what the caller asked for.
                if (type < 0 || type > 5) return false;
                auto const kind = static_cast<TitleType>(type);

                // The three durations are given all together or not at all. A mixed
                // set has no defensible reading: a 5-tick fade-in with whatever stay
                // time the client happens to hold is a bug at the call site, not a
                // request.
                int const specified =
                    (fadeInTicks >= 0 ? 1 : 0) + (stayTicks >= 0 ? 1 : 0) + (fadeOutTicks >= 0 ? 1 : 0);
                if (specified != 0 && specified != 3) return false;
                bool const withTimes = (specified == 3);

                Player* p = bridge::resolvePlayer(sel);
                if (!p) return false;

                // The durations, when requested, go first as a packet of their own,
                // which is what `/title <who> times a b c` sends, and the client applies
                // them to titles arriving afterwards. Putting the durations only into
                // the content packet works on some versions and not others, while
                // sending a Times packet is behavior vanilla itself relies on.
                if (withTimes)
                {
                    SetTitlePacket times;
                    times.mType = TitleType::Times;
                    times.mFadeInTime = fadeInTicks;
                    times.mStayTime = stayTicks;
                    times.mFadeOutTime = fadeOutTicks;
                    p->sendNetworkPacket(times);
                    // type == 5 means the caller only wants to change the durations.
                    if (kind == TitleType::Times) return true;
                }
                else if (kind == TitleType::Times)
                {
                    // A Times without durations is an empty request, not a valid packet.
                    return false;
                }

                // ll::PayloadPacket<T> derives from T (mc/network/Packet.h:204), so the
                // payload fields sit directly on the packet, the same access pattern
                // SpawnParticleEffectPacket uses above. No wire format is involved.
                SetTitlePacket pkt;
                pkt.mType = kind;
                if (kind == TitleType::Title || kind == TitleType::Subtitle
                    || kind == TitleType::Actionbar)
                {
                    pkt.mTitleText = toString(text);
                }
                if (withTimes)
                {
                    pkt.mFadeInTime = fadeInTicks;
                    pkt.mStayTime = stayTicks;
                    pkt.mFadeOutTime = fadeOutTicks;
                }
                return sendToPlayer(sel, pkt);
            PIER_API_GUARD_END
        }

        bool api_spawn_particle_for(
            PierPlayerSel sel, int32_t dimension, PierStr effectName, double x, double y, double z)
        {
            PIER_API_GUARD_BEGIN
                // Typed construction. The MCAPI default constructor initializes the
                // packet in serialization mode and gives the payload its defaults, with
                // mActorId invalid and mMolangVariables nullopt, so only the three
                // fields that matter are filled here. No wire format is involved, so a
                // version bump needs no follow-up the way an api_send_packet caller
                // does.
                if (dimension < 0 || dimension > 255)
                {
                    static std::set<int32_t> warned;
                    if (warned.insert(dimension).second)
                    {
                        hostLogger().warn(
                            "[packet] spawn_particle_for: dimension {} does not fit the "
                            "single-byte dimension id of SpawnParticleEffectPacket and is "
                            "truncated to {}; a targeted particle in a custom dimension may "
                            "not appear, use spawn_particle instead",
                            dimension, static_cast<int>(static_cast<uchar>(dimension)));
                    }
                }
                SpawnParticleEffectPacket pkt;
                pkt.mVanillaDimensionId = static_cast<uchar>(dimension);
                pkt.mPos = Vec3{(float)x, (float)y, (float)z};
                pkt.mEffectName = toString(effectName);
                return sendToPlayer(sel, pkt);
            PIER_API_GUARD_END
        }

        void fill(PierApi& api)
        {
            api.send_packet = &api_send_packet;
            api.player_send_title = &api_player_send_title;
            api.spawn_particle_for = &api_spawn_particle_for;
        }

        spi::SlotPackReg reg{{"packets", &fill}};
    } // namespace
} // namespace pier::api_impl

#endif // !PIER_BUILD_CLIENT
