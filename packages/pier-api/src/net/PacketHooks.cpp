/** net/PacketHooks.cpp: packet interception at the raw wire format.
 * Hooks NetworkSystem::_sendInternal and NetworkConnection::receivePacket, the two narrowest
 * points where a packet exists as plaintext bytes and as exactly one packet. Higher, at
 * Packet::write, would require reserializing; lower, at the peers, would mean unbatching and
 * fighting compression. Both directions carry the same leading unsigned varint header: bits 0..9
 * are the packet id, bits 10..11 the sender sub client id and bits 12..13 the receiver. The bridge
 * decodes it, hands the body to subscribers and re-encodes from PierPacketEdit on the way back, so
 * a caller never touches varint framing. Outbound, the decoded id is cross-checked against
 * packet.getId(); a mismatch means the layout assumption no longer holds on this BDS, and it is
 * logged before the packet passes through. The rule that everything runs on the server thread does
 * not apply: enableAsyncFlush exists and the send path is reachable from more than one place. The
 * registry sits behind a shared_mutex and dispatch takes a shared_ptr snapshot before releasing
 * it, so a callback may register or unregister itself mid-dispatch and an entry released by
 * another thread is not pulled away. Detours install lazily, are never unpatched, and route back
 * to origin on one atomic read while idle. */
#ifndef PIER_BUILD_CLIENT

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ll/api/memory/Hook.h"
#include "ll/api/utils/ErrorUtils.h"

#include "mc/network/NetworkConnection.h"
#include "mc/network/NetworkIdentifier.h"
#include "mc/network/NetworkPeer.h"
#include "mc/network/NetworkSystem.h"
#include "mc/network/Packet.h"

#include "sdk/abi.h"

#include "pier/host/hosted_mod.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/log.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        /*  Registry  */

        /** 1024 packet ids, one bit each. The header carries the id in ten bits, so
         *  this covers every value the wire can express. */
        struct IdSet
        {
            std::array<uint64_t, 16> words{};

            void set(int32_t id) noexcept
            {
                if (id < 0 || id >= 1024) return;
                words[static_cast<size_t>(id) >> 6] |= uint64_t{1} << (id & 63);
            }
            bool has(int32_t id) const noexcept
            {
                if (id < 0 || id >= 1024) return false;
                return (words[static_cast<size_t>(id) >> 6] >> (id & 63)) & 1u;
            }
            void fill() noexcept { words.fill(~uint64_t{0}); }
        };

        struct PacketSub
        {
            HostedMod* mod;
            /** Weak reference taken at registration. A snapshot only keeps the entry
             *  alive, while the code section the callback targets disappears with
             *  FreeLibrary, so it is rechecked through this before dispatch. */
            std::weak_ptr<HostedMod> owner;
            int32_t dirMask;
            /** Which packet ids this subscriber wants. The legacy register slot sets
             *  every bit, so such a subscriber is called for every packet;
             *  packet_hook_register_ids sets only the listed ones. */
            IdSet ids;
            PierPacketCb cb;
            void* user;
        };

        struct ConnSub
        {
            HostedMod* mod;
            std::weak_ptr<HostedMod> owner;
            PierConnCb cb;
            void* user;
        };

        /** Rechecks that the subscriber is still alive and returns its shared_ptr,
         *  held until the callback returns. An entry with a null mod, meaning no
         *  ownership was recorded, is dispatched as before. */
        template <class Sub>
        std::shared_ptr<HostedMod> liveOwner(Sub const& sub, bool& skip)
        {
            skip = false;
            if (!sub.mod) return {};
            auto mod = sub.owner.lock();
            if (!mod || mod.get() != sub.mod || mod->unloading.load(std::memory_order_acquire))
            {
                skip = true; // Unloaded or unloading: the entry is in the snapshot, the code is not
                return {};
            }
            return mod;
        }

        using PacketSubs = std::vector<std::shared_ptr<PacketSub>>;
        using ConnSubs = std::vector<std::shared_ptr<ConnSub>>;

        std::shared_mutex& registryLock()
        {
            static std::shared_mutex m;
            return m;
        }

        PacketSubs& packetSubs()
        {
            static PacketSubs v;
            return v;
        }

        ConnSubs& connSubs()
        {
            static ConnSubs v;
            return v;
        }

        /**
         * The gate on the hot path. Read without a lock: a stale `false` misses one
         * packet on the tick a subscriber appears, and a stale `true` costs one empty
         * snapshot. Neither is worth a lock on every packet.
         */
        std::atomic<bool> gInboundLive{false};
        std::atomic<bool> gOutboundLive{false};
        std::atomic<bool> gConnLive{false};

        /**
         * The union of every subscriber's id set, per direction, read lock-free on the
         * hot path. A packet whose id no subscriber wants is passed through before any
         * lock, snapshot or address lookup. The same staleness argument as the gates
         * applies: one packet may be missed or one empty snapshot taken on the tick a
         * subscription changes.
         */
        std::array<std::atomic<uint64_t>, 16> gInboundIds{};
        std::array<std::atomic<uint64_t>, 16> gOutboundIds{};

        bool anyoneWants(int32_t direction, int32_t packetId) noexcept
        {
            if (packetId < 0 || packetId >= 1024) return false;
            auto const& words = direction == PIER_PKT_INBOUND ? gInboundIds : gOutboundIds;
            uint64_t const w = words[static_cast<size_t>(packetId) >> 6].load(std::memory_order_relaxed);
            return (w >> (packetId & 63)) & 1u;
        }

        void refreshGatesLocked()
        {
            bool in = false;
            bool out = false;
            IdSet inIds, outIds;
            for (auto const& s : packetSubs())
            {
                if (s->dirMask & PIER_PKT_MASK_INBOUND)
                {
                    in = true;
                    for (size_t i = 0; i < inIds.words.size(); ++i) inIds.words[i] |= s->ids.words[i];
                }
                if (s->dirMask & PIER_PKT_MASK_OUTBOUND)
                {
                    out = true;
                    for (size_t i = 0; i < outIds.words.size(); ++i) outIds.words[i] |= s->ids.words[i];
                }
            }
            for (size_t i = 0; i < 16; ++i)
            {
                gInboundIds[i].store(inIds.words[i], std::memory_order_relaxed);
                gOutboundIds[i].store(outIds.words[i], std::memory_order_relaxed);
            }
            gInboundLive.store(in, std::memory_order_relaxed);
            gOutboundLive.store(out, std::memory_order_relaxed);
            gConnLive.store(!connSubs().empty(), std::memory_order_relaxed);
        }

        /**
         * Address cache. getIPAndPort() builds a std::string on every call and the
         * packet path is the hottest loop on the server, so it is resolved once per
         * connection and a view onto the cached copy is handed out afterwards.
         * Held as a shared_ptr rather than by value, because dispatch needs a view that
         * stays valid across the whole callback chain while another thread may evict
         * the entry mid-chain when the connection closes. Filled on connection open and
         * dropped on close. When the hooks installed mid-session and missed the open,
         * it is filled on first sight.
         */
        using AddressPtr = std::shared_ptr<std::string const>;

        std::unordered_map<uint64_t, AddressPtr>& addressCache()
        {
            static std::unordered_map<uint64_t, AddressPtr> m;
            return m;
        }

        std::mutex& addressLock()
        {
            static std::mutex m;
            return m;
        }

        /** The cached "host:port" for an identifier, inserted on first sight. */
        AddressPtr addressOf(::NetworkIdentifier const& id)
        {
            uint64_t const hash = id.getHash();
            {
                std::lock_guard<std::mutex> g{addressLock()};
                auto it = addressCache().find(hash);
                if (it != addressCache().end()) return it->second;
            }
            std::string addr;
            try
            {
                addr = id.getIPAndPort();
            }
            catch (...)
            {
                addr.clear();
            }
            auto ptr = std::make_shared<std::string const>(std::move(addr));
            std::lock_guard<std::mutex> g{addressLock()};
            // A concurrent insert is harmless, since the key is the same and the
            // values are equivalent. The first one to land is kept and both threads
            // report the same pointer.
            return addressCache().emplace(hash, std::move(ptr)).first->second;
        }

        void forgetAddress(uint64_t hash)
        {
            std::lock_guard<std::mutex> g{addressLock()};
            addressCache().erase(hash);
        }

        /*  Header encode and decode  */

        constexpr uint32_t kPacketIdMask = 0x3FF; // bits 0..9
        constexpr uint32_t kSubIdMask = 0x3;

        /** LEB128 decode. Returns false on a truncated or overlong varint. */
        bool readUVarInt(uint8_t const* data, size_t len, size_t& pos, uint32_t& out)
        {
            uint32_t result = 0;
            uint32_t shift = 0;
            while (pos < len)
            {
                uint8_t const b = data[pos++];
                result |= static_cast<uint32_t>(b & 0x7F) << shift;
                if ((b & 0x80) == 0)
                {
                    out = result;
                    return true;
                }
                shift += 7;
                if (shift >= 35) return false; // Longer than any uint32 can be
            }
            return false;
        }

        void writeUVarInt(std::string& out, uint32_t value)
        {
            while (value >= 0x80)
            {
                out.push_back(static_cast<char>((value & 0x7F) | 0x80));
                value >>= 7;
            }
            out.push_back(static_cast<char>(value));
        }

        struct Header
        {
            uint32_t raw;
            int32_t packetId;
            uint8_t senderSubId;
            uint8_t targetSubId;
            size_t size; // Bytes consumed
        };

        bool decodeHeader(std::string const& packet, Header& out)
        {
            auto const* data = reinterpret_cast<uint8_t const*>(packet.data());
            size_t pos = 0;
            uint32_t raw = 0;
            if (!readUVarInt(data, packet.size(), pos, raw)) return false;
            out.raw = raw;
            out.packetId = static_cast<int32_t>(raw & kPacketIdMask);
            out.senderSubId = static_cast<uint8_t>((raw >> 10) & kSubIdMask);
            out.targetSubId = static_cast<uint8_t>((raw >> 12) & kSubIdMask);
            out.size = pos;
            return true;
        }

        void encodeHeader(std::string& out, PierPacketEdit const& edit)
        {
            uint32_t const raw = (static_cast<uint32_t>(edit.packet_id) & kPacketIdMask)
                | ((static_cast<uint32_t>(edit.sender_sub_id) & kSubIdMask) << 10)
                | ((static_cast<uint32_t>(edit.target_sub_id) & kSubIdMask) << 12);
            writeUVarInt(out, raw);
        }

        /*  Dispatch  */

        /** Where a PIER_PKT_REPLACE lands. */
        struct ReplaceBuf
        {
            std::string bytes;
            bool written = false;
        };

        void replaceSink(void* ctx, uint8_t const* data, size_t len)
        {
            auto* buf = static_cast<ReplaceBuf*>(ctx);
            if (!buf) return;
            buf->written = true;
            buf->bytes.assign(reinterpret_cast<char const*>(data), len);
        }

        enum class Verdict
        {
            Pass,
            Replaced,
            Drop
        };

        /**
         * Re-entry gate. A subscriber may send a packet from inside a callback, which
         * is half the reason the hooks exist. Without this gate the resulting
         * `_sendInternal` recurses straight back into dispatch and, at worst, into
         * itself forever. A packet sent from a callback goes out unchanged.
         */
        thread_local int tlDispatchDepth = 0;

        struct DepthGuard
        {
            DepthGuard() { ++tlDispatchDepth; }
            ~DepthGuard() { --tlDispatchDepth; }
        };

        PacketSubs snapshotFor(int32_t direction, int32_t packetId)
        {
            int32_t const mask = 1 << direction;
            PacketSubs out;
            std::shared_lock<std::shared_mutex> g{registryLock()};
            out.reserve(packetSubs().size());
            for (auto const& s : packetSubs())
            {
                if ((s->dirMask & mask) && s->ids.has(packetId)) out.push_back(s);
            }
            return out;
        }

        /**
         * Runs in, meaning header plus body, past every interested subscriber in
         * registration order, each seeing the output of the previous one. On
         * Verdict::Replaced the rebuilt packet lands in out and in is untouched; on
         * Pass and Drop out is meaningless. Separating input from output is what makes
         * the unmodified path allocation-free: a copy happens only when a subscriber
         * really rewrites the body, and a chunk packet is tens of kilobytes while the
         * overwhelming majority of packets are forwarded unchanged.
         * Malformed input always leaves the packet as it was. Precondition: out must
         * not alias in, since the rebuild reads views that may still point into in.
         */
        Verdict dispatch(
            int32_t direction,
            ::NetworkIdentifier const& id,
            std::string const& in,
            std::string& out,
            int32_t expectedId)
        {
            if (tlDispatchDepth > 0) return Verdict::Pass;

            Header header{};
            if (!decodeHeader(in, header)) return Verdict::Pass;

            // The layout check on the outbound side, which is the side holding a
            // packet object to compare against. A mismatch means the header encoding
            // assumed above no longer holds, so it is reported once and no packet is
            // touched afterwards, rather than mangling them silently.
            if (expectedId >= 0 && header.packetId != expectedId)
            {
                static std::atomic<bool> warned{false};
                if (!warned.exchange(true))
                {
                    hostLogger().error(
                        "[packet] header decode disagrees with Packet::getId(), decoded "
                        "{} but the packet reports {}; the header layout of this BDS "
                        "version changed, interception has degraded to passing every "
                        "packet through, report this line to the host maintainers",
                        header.packetId,
                        expectedId
                    );
                }
                return Verdict::Pass;
            }

            // The lock-free id gate. Most packets on a busy server are movement,
            // chunk and entity data that no subscriber asked for, and they leave here
            // without touching the registry lock or the address cache.
            if (!anyoneWants(direction, header.packetId)) return Verdict::Pass;

            auto subs = snapshotFor(direction, header.packetId);
            if (subs.empty()) return Verdict::Pass;

            AddressPtr const address = addressOf(id);
            uint64_t const connId = id.getHash();

            // `curPtr` and `curLen` are what the next subscriber sees. They start as a
            // view into `in` and move into `owned` only once someone rewrites the
            // body.
            auto const* curPtr = reinterpret_cast<uint8_t const*>(in.data()) + header.size;
            size_t curLen = in.size() - header.size;
            std::string owned;

            PierPacketEdit edit{};
            edit.struct_size = static_cast<uint32_t>(sizeof(PierPacketEdit));
            edit.packet_id = header.packetId;
            edit.sender_sub_id = header.senderSubId;
            edit.target_sub_id = header.targetSubId;

            bool changed = false;
            {
                DepthGuard depth;
                for (auto const& sub : subs)
                {
                    bool skip = false;
                    auto owner = liveOwner(*sub, skip);
                    if (skip) continue;
                    CallbackScope scope{owner.get()};

                    PierPacketEvent ev{};
                    ev.struct_size = static_cast<uint32_t>(sizeof(PierPacketEvent));
                    ev.direction = direction;
                    ev.conn_id = connId;
                    ev.address = ps(*address);
                    ev.packet_id = edit.packet_id;
                    ev.sender_sub_id = edit.sender_sub_id;
                    ev.target_sub_id = edit.target_sub_id;
                    ev.body = curLen == 0 ? nullptr : curPtr;
                    ev.body_len = curLen;

                    ReplaceBuf buf;
                    PierPacketEdit pending = edit;
                    int32_t verdict = PIER_PKT_PASS;
                    try
                    {
                        verdict = sub->cb(sub->user, &ev, &pending, &buf, &replaceSink);
                    }
                    catch (...)
                    {
                        // An exception from a callback is a bug on the mod side, and it
                        // must take neither the connection nor the silence with it. The
                        // exception is printed every time; the warning that the verdict
                        // was forced to PASS is printed once per process.
                        verdict = PIER_PKT_PASS;
                        ll::error_utils::printCurrentException(hostLogger());
                        static std::atomic<bool> warned{false};
                        if (!warned.exchange(true))
                        {
                            hostLogger().warn(
                                "[packet] a mod callback threw, its verdict was forced to "
                                "PASS; this warning prints once, the exception above prints "
                                "every time"
                            );
                        }
                    }

                    if (verdict == PIER_PKT_DROP) return Verdict::Drop;
                    if (verdict != PIER_PKT_REPLACE) continue;

                    owned = buf.written ? std::move(buf.bytes) : std::string{};
                    curPtr = reinterpret_cast<uint8_t const*>(owned.data());
                    curLen = owned.size();
                    pending.struct_size = static_cast<uint32_t>(sizeof(PierPacketEdit));
                    edit = pending;
                    changed = true;
                }
            }

            if (!changed) return Verdict::Pass;

            out.clear();
            out.reserve(curLen + 5); // 5 is the maximum varint32 header size
            encodeHeader(out, edit);
            out.append(reinterpret_cast<char const*>(curPtr), curLen);
            return Verdict::Replaced;
        }

        void dispatchConn(::NetworkIdentifier const& id, bool opened)
        {
            if (!gConnLive.load(std::memory_order_relaxed)) return;

            ConnSubs subs;
            {
                std::shared_lock<std::shared_mutex> g{registryLock()};
                subs = connSubs();
            }
            if (subs.empty()) return;

            uint64_t const connId = id.getHash();
            AddressPtr const address = addressOf(id);

            for (auto const& sub : subs)
            {
                bool skip = false;
                auto owner = liveOwner(*sub, skip);
                if (skip) continue;
                CallbackScope scope{owner.get()};
                try
                {
                    sub->cb(sub->user, connId, ps(*address), opened);
                }
                catch (...)
                {
                    // The same rule as the packet path: a mod exception never escapes
                    // into the network stack. It is logged, because a silent catch(...)
                    // makes the bug invisible forever.
                    ll::error_utils::printCurrentException(hostLogger());
                }
            }
        }

        /*  detour  */

        /**
         * Outbound. `data` is the fully serialized packet, header plus body, before
         * batching and compression. Rewriting means calling origin with a different
         * string; dropping means not calling it at all.
         */
        LL_TYPE_INSTANCE_HOOK(
            PierPacketSendHook,
            ll::memory::HookPriority::Normal,
            NetworkSystem,
            &NetworkSystem::_sendInternal,
            void,
            ::NetworkIdentifier const& id,
            ::Packet const& packet,
            ::std::string const& data)
        {
            if (!gOutboundLive.load(std::memory_order_relaxed)) return origin(id, packet, data);

            int32_t expectedId = -1;
            try
            {
                expectedId = static_cast<int32_t>(packet.getId());
            }
            catch (...)
            {
                expectedId = -1;
            }
            // The packet object already knows its id, so the id gate runs before the
            // header is even decoded. When the id cannot be read the decode path below
            // applies the same gate.
            if (expectedId >= 0 && !anyoneWants(PIER_PKT_OUTBOUND, expectedId))
            {
                return origin(id, packet, data);
            }

            std::string rewritten;
            switch (dispatch(PIER_PKT_OUTBOUND, id, data, rewritten, expectedId))
            {
            case Verdict::Drop:
                return;
            case Verdict::Replaced:
                return origin(id, packet, rewritten);
            case Verdict::Pass:
            default:
                return origin(id, packet, data);
            }
        }

        /**
         * Inbound. One packet per HasData. A dropped packet must not stall the pump,
         * because returning NoData strands every packet still queued for this tick, so
         * the next one is pulled instead and the hook returns only on a survivor or
         * once the peer runs dry.
         */
        LL_TYPE_INSTANCE_HOOK(
            PierPacketReceiveHook,
            ll::memory::HookPriority::Normal,
            NetworkConnection,
            &NetworkConnection::receivePacket,
            ::NetworkPeer::DataStatus,
            ::std::string& receiveBuffer,
            ::std::shared_ptr<::std::chrono::steady_clock::time_point> const& timepointPtr)
        {
            if (!gInboundLive.load(std::memory_order_relaxed))
                return origin(receiveBuffer, timepointPtr);

            // Reused across iterations, so a burst of rewritten packets shares one
            // buffer instead of reallocating per packet.
            std::string rewritten;

            for (;;)
            {
                auto const status = origin(receiveBuffer, timepointPtr);
                if (status != ::NetworkPeer::DataStatus::HasData) return status;

                switch (dispatch(
                    PIER_PKT_INBOUND, this->mId.get(), receiveBuffer, rewritten, /*expectedId=*/-1))
                {
                case Verdict::Drop:
                    // NoData must not be returned here, since it strands every packet
                    // still queued for this tick. The next one is requested instead.
                    continue;
                case Verdict::Replaced:
                    receiveBuffer = rewritten;
                    return status;
                case Verdict::Pass:
                default:
                    return status;
                }
            }
        }

        /** Connection accepted, the earliest moment conn_id exists. */
        LL_TYPE_INSTANCE_HOOK(
            PierConnOpenHook,
            ll::memory::HookPriority::Normal,
            NetworkSystem,
            &NetworkSystem::$onNewIncomingConnection,
            bool,
            ::NetworkIdentifier const& id,
            ::std::shared_ptr<::NetworkPeer>&& peer)
        {
            bool const accepted = origin(id, std::move(peer));
            if (accepted) dispatchConn(id, /*opened=*/true);
            return accepted;
        }

        /**
         * Connection closed. Subscribers are notified before the address entry is
         * evicted, because a close handler still wants a resolvable address.
         */
        LL_TYPE_INSTANCE_HOOK(
            PierConnCloseHook,
            ll::memory::HookPriority::Normal,
            NetworkSystem,
            &NetworkSystem::$onConnectionClosed,
            void,
            ::NetworkIdentifier const& id,
            ::Connection::DisconnectFailReason const discoReason,
            ::std::string const& messageFromServer,
            ::std::string const& messageBodyOverride,
            bool skipDisconnectMessage,
            ::Json::Value const& sessionSummary)
        {
            dispatchConn(id, /*opened=*/false);
            forgetAddress(id.getHash());
            origin(id, discoReason, messageFromServer, messageBodyOverride, skipDisconnectMessage,
                   sessionSummary);
        }

        /**
         * Installs every detour once, when the first subscription of any kind appears.
         * They are never unhooked, because an unsubscribe can arrive from inside a
         * hooked function.
         * A function-local static is used instead of the bare `bool` flag the other
         * lazy hooks in this bridge use, since those all run on the server thread while
         * registration here need not.
         */
        void ensureInstalled()
        {
            static bool const installed = []
            {
                PierPacketSendHook::hook();
                PierPacketReceiveHook::hook();
                PierConnOpenHook::hook();
                PierConnCloseHook::hook();
                hostLogger().debug("[packet] send and receive detours installed");
                return true;
            }();
            (void)installed;
        }

        /*  ABI entry points  */

        PierPacketHookHandle registerPacketSub(
            PierModHandle mod, int32_t dirMask, IdSet ids, PierPacketCb cb, void* user)
        {
            if (!cb) return nullptr;
            // A zero mask registers something that can never fire. It is refused
            // rather than answered with a handle that does nothing.
            if ((dirMask & (PIER_PKT_MASK_INBOUND | PIER_PKT_MASK_OUTBOUND)) == 0) return nullptr;

            auto* raw = asMod(mod);
            std::weak_ptr<HostedMod> owner;
            if (raw)
            {
                try
                {
                    owner = raw->shared_from_this();
                }
                catch (...)
                {
                    return nullptr; // Refuse a mod not yet owned by a shared_ptr, as in Bus
                }
            }
            auto sub = std::make_shared<PacketSub>(PacketSub{raw, std::move(owner), dirMask, ids, cb, user});
            PacketSub* rawSub = sub.get();
            {
                std::unique_lock<std::shared_mutex> g{registryLock()};
                packetSubs().push_back(std::move(sub));
                refreshGatesLocked();
            }
            ensureInstalled();
            return static_cast<PierPacketHookHandle>(rawSub);
        }

        PierPacketHookHandle
        api_packet_hook_register(PierModHandle mod, int32_t dirMask, PierPacketCb cb, void* user)
        {
            PIER_API_GUARD_BEGIN
                // The legacy shape: every id, which is one callback per packet in that
                // direction. A caller that knows its ids uses packet_hook_register_ids.
                IdSet all;
                all.fill();
                return registerPacketSub(mod, dirMask, all, cb, user);
            PIER_API_GUARD_END
        }

        PierPacketHookHandle api_packet_hook_register_ids(
            PierModHandle mod, int32_t dirMask, int32_t const* ids, size_t count, PierPacketCb cb, void* user)
        {
            PIER_API_GUARD_BEGIN
                // An empty list is refused for the reason a zero mask is: a subscription
                // that can never fire is a bug at the call site, not a request.
                if (!ids || count == 0) return nullptr;
                IdSet set;
                bool any = false;
                for (size_t i = 0; i < count; ++i)
                {
                    if (ids[i] < 0 || ids[i] >= 1024)
                    {
                        hostLogger().warn(
                            "[packet] packet_hook_register_ids: id {} is outside the ten-bit "
                            "range of the wire header and is ignored", ids[i]);
                        continue;
                    }
                    set.set(ids[i]);
                    any = true;
                }
                if (!any) return nullptr;
                return registerPacketSub(mod, dirMask, set, cb, user);
            PIER_API_GUARD_END
        }

        bool api_packet_hook_unregister(PierModHandle mod, PierPacketHookHandle handle)
        {
            PIER_API_GUARD_BEGIN
                if (!handle) return false;
                auto* target = static_cast<PacketSub*>(handle);
                std::unique_lock<std::shared_mutex> g{registryLock()};
                auto& subs = packetSubs();
                for (auto it = subs.begin(); it != subs.end(); ++it)
                {
                    if (it->get() != target) continue;
                    // Ownership check. A mod may only remove its own interceptor.
                    if ((*it)->mod != asMod(mod)) return false;
                    subs.erase(it);
                    refreshGatesLocked();
                    return true;
                }
                return false;
            PIER_API_GUARD_END
        }

        PierPacketHookHandle api_packet_conn_hook_register(PierModHandle mod, PierConnCb cb, void* user)
        {
            PIER_API_GUARD_BEGIN
                if (!cb) return nullptr;

                auto* raw = asMod(mod);
                std::weak_ptr<HostedMod> owner;
                if (raw)
                {
                    try
                    {
                        owner = raw->shared_from_this();
                    }
                    catch (...)
                    {
                        return nullptr;
                    }
                }
                auto sub = std::make_shared<ConnSub>(ConnSub{raw, std::move(owner), cb, user});
                ConnSub* rawSub = sub.get();
                {
                    std::unique_lock<std::shared_mutex> g{registryLock()};
                    connSubs().push_back(std::move(sub));
                    refreshGatesLocked();
                }
                ensureInstalled();
                return static_cast<PierPacketHookHandle>(rawSub);
            PIER_API_GUARD_END
        }

        bool api_packet_conn_hook_unregister(PierModHandle mod, PierPacketHookHandle handle)
        {
            PIER_API_GUARD_BEGIN
                if (!handle) return false;
                auto* target = static_cast<ConnSub*>(handle);
                std::unique_lock<std::shared_mutex> g{registryLock()};
                auto& subs = connSubs();
                for (auto it = subs.begin(); it != subs.end(); ++it)
                {
                    if (it->get() != target) continue;
                    if ((*it)->mod != asMod(mod)) return false;
                    subs.erase(it);
                    refreshGatesLocked();
                    return true;
                }
                return false;
            PIER_API_GUARD_END
        }

        /** Teardown at stage 90. Clears every packet and connection subscription held
         *  under this mod. The detours stay, as the file header states, and the gate
         *  short-circuits an empty registry on the next dispatch. */
        void teardown(HostedMod* mod)
        {
            std::unique_lock<std::shared_mutex> g{registryLock()};
            auto& psubs = packetSubs();
            for (auto it = psubs.begin(); it != psubs.end();)
            {
                it = ((*it)->mod == mod) ? psubs.erase(it) : it + 1;
            }
            auto& csubs = connSubs();
            for (auto it = csubs.begin(); it != csubs.end();)
            {
                it = ((*it)->mod == mod) ? csubs.erase(it) : it + 1;
            }
            refreshGatesLocked();
        }

        void fill(PierApi& api)
        {
            api.packet_hook_register = &api_packet_hook_register;
            api.packet_hook_register_ids = &api_packet_hook_register_ids;
            api.packet_hook_unregister = &api_packet_hook_unregister;
            api.packet_conn_hook_register = &api_packet_conn_hook_register;
            api.packet_conn_hook_unregister = &api_packet_conn_hook_unregister;
        }

        spi::SlotPackReg regSlots{{"packet-hooks", &fill}};
        spi::TeardownReg regDown{{90, "packet-hooks", &teardown}};
    } // namespace
} // namespace pier::api_impl

#endif // !PIER_BUILD_CLIENT
