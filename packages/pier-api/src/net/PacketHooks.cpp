/** net/PacketHooks.cpp —— 裸线格式的数据包拦截。
 *
 * # 钩的是什么、为什么是这两个函数
 *
 * 一个 Bedrock 包出站要穿过好几层：
 *
 *     Packet 对象
 *       -> NetworkSystem::send            序列化包头 + 包体
 *       -> NetworkSystem::_sendInternal   (id, packet, std::string data)
 *       -> BatchedNetworkPeer::sendPacket 追加 [uvarint 长度][data]
 *       -> CompressedNetworkPeer          压缩整批
 *       -> EncryptedNetworkPeer           加密
 *       -> RakNet
 *
 * 入站是镜像，终点是 `NetworkConnection::receivePacket` —— 每次调用从对端链
 * 里取出**一个**已解密、已解压、已拆批的包（拆批是
 * `BatchedNetworkPeer::_receivePacket` 在走 `mIncomingData`）。
 *
 * 所以 `_sendInternal` 和 `receivePacket` 是包以明文字节、且恰好一个包的形
 * 态存在的最窄两点。往上钩（Packet::write）要重新序列化；往下钩（各 peer）
 * 要拆批、还得跟压缩缠斗。哪头都不划算。
 *
 * # 包头
 *
 * 两个方向都带同一个前导 unsigned varint：
 *
 *     bits 0..9   包 id（MinecraftPacketIds）
 *     bits 10..11 发送方 sub client id
 *     bits 12..13 接收方 sub client id
 *
 * 桥解码它、把**包体**交给订阅者、回程再从 `PierPacketEdit` 重编码。调用方
 * 永远不碰 varint 装帧，改包 id 是一次赋值而不是字节手术。
 *
 * # 出站侧的交叉校验
 *
 * 出站解出来的 id 会和 `packet.getId()` 对一次。两者只会在未来某个 BDS 上
 * 这套布局假设失效时不一致 —— 那时打一次日志、然后全部原样放行，绝不弄脏
 * 流。
 *
 * # 锁
 *
 * 这个桥其余部分的口径是「一切都跑在服务器线程上，注册表不用锁」。这里不
 * 能这么假设：`enableAsyncFlush` 存在，发送路径不止一处可达。所以注册表放
 * 在 shared_mutex 后面，派发先取一份 shared_ptr **快照**、放锁、再调用任何
 * 东西。一举两得 —— 回调可以在派发中途（反）注册自己，另一个线程释放的条
 * 目也不会从我们脚下被抽走。
 *
 * # 生命周期
 *
 * detour 在第一个订阅者出现时懒安装、**永不**卸补丁：退订可能来自被钩函数
 * 内部，在那里卸补丁不安全。空闲的钩子靠一次原子读快速路由回 origin。
 */
#ifndef PIER_BUILD_CLIENT

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
        /* ───────────────────────── 注册表 ───────────────────────── */

        struct PacketSub
        {
            HostedMod* mod;
            int32_t dirMask;
            PierPacketCb cb;
            void* user;
        };

        struct ConnSub
        {
            HostedMod* mod;
            PierConnCb cb;
            void* user;
        };

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
         * 热路径的门。读时不上锁：陈旧的 `false` 让订阅者出现的那一 tick 漏
         * 拦一个包，陈旧的 `true` 换来一次空快照。两样都不值得每个包上一把
         * 锁。
         */
        std::atomic<bool> gInboundLive{false};
        std::atomic<bool> gOutboundLive{false};
        std::atomic<bool> gConnLive{false};

        void refreshGatesLocked()
        {
            bool in = false;
            bool out = false;
            for (auto const& s : packetSubs())
            {
                if (s->dirMask & PIER_PKT_MASK_INBOUND) in = true;
                if (s->dirMask & PIER_PKT_MASK_OUTBOUND) out = true;
            }
            gInboundLive.store(in, std::memory_order_relaxed);
            gOutboundLive.store(out, std::memory_order_relaxed);
            gConnLive.store(!connSubs().empty(), std::memory_order_relaxed);
        }

        /**
         * 地址缓存。`getIPAndPort()` 每调一次建一个 std::string，而包路径是服
         * 务器最热的环 —— 所以每连接解析一次，之后发的是指向缓存副本的视图。
         *
         * 用 shared_ptr 持有、不按值：派发需要一个在整条回调链里都有效的视
         * 图，而另一个线程可能在链跑着时把条目逐出（连接关闭）。引用计数加一
         * 让字符串活着，又不用每个包拷一遍。
         *
         * 连接打开时填入、关闭时丢弃。兜底路径（钩子会话中途才装、错过了
         * open）第一次见到就补上，而不是永远重解析。
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

        /** 某个 identifier 缓存的 "host:port"；没见过就顺手插入。 */
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
            // 并发插入无妨：同键、等价值。留先落地的那份，两个线程报同一个指
            // 针。
            return addressCache().emplace(hash, std::move(ptr)).first->second;
        }

        void forgetAddress(uint64_t hash)
        {
            std::lock_guard<std::mutex> g{addressLock()};
            addressCache().erase(hash);
        }

        /* ───────────────────────── 包头编解码 ───────────────────────── */

        constexpr uint32_t kPacketIdMask = 0x3FF; // bits 0..9
        constexpr uint32_t kSubIdMask = 0x3;

        /** LEB128 解码。截断或超长 varint 返回 false。 */
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
                if (shift >= 35) return false; // 比 uint32 能有的还长
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
            size_t size; // 吃掉的字节数
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

        /* ───────────────────────── 派发 ───────────────────────── */

        /** PIER_PKT_REPLACE 的收纳目标。 */
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
         * 重入闸。订阅者允许在回调里发包（这本来就是钩子存在的一半意义）；
         * 没有这道闸，随之而来的 `_sendInternal` 会直接递归回派发、最坏情况
         * 里递归回自己直到永远。从回调里发出的包原样出门。
         */
        thread_local int tlDispatchDepth = 0;

        struct DepthGuard
        {
            DepthGuard() { ++tlDispatchDepth; }
            ~DepthGuard() { --tlDispatchDepth; }
        };

        PacketSubs snapshotFor(int32_t direction)
        {
            int32_t const mask = 1 << direction;
            PacketSubs out;
            std::shared_lock<std::shared_mutex> g{registryLock()};
            out.reserve(packetSubs().size());
            for (auto const& s : packetSubs())
            {
                if (s->dirMask & mask) out.push_back(s);
            }
            return out;
        }

        /**
         * 把 `in`（包头 + 包体）按注册顺序过一遍所有感兴趣的订阅者，每个看到
         * 上一个的输出。
         *
         * Verdict::Replaced 时重建的包落在 `out`、`in` 不动；Pass 和 Drop 时
         * `out` 无意义。把输入和输出分开正是未改写路径零分配的原因：只有当
         * 某个订阅者真的重写包体时才会拷贝一次 —— 这很要紧，一个区块包几十
         * KB，而压倒性多数的包是原样转发的。
         *
         * 任何畸形输入都让包保持原样 —— 解析不了的翻译者不许有能力弄脏它。
         *
         * 前置条件：`out` 不得与 `in` 别名。重建读的视图可能仍指向 `in`。
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

            // 出站侧的布局体检 —— 这一侧手里有包对象可以对。不一致意味着上面
            // 假设的包头编码不再成立；说一次、然后停手不碰任何包，绝不静默把
            // 它们绞坏。
            if (expectedId >= 0 && header.packetId != expectedId)
            {
                static std::atomic<bool> warned{false};
                if (!warned.exchange(true))
                {
                    hostLogger().error(
                        "[PacketHooks] 包头解析结果与 Packet::getId() 不一致"
                        "（解析得到 {}，实际 {}）。说明这个 BDS 版本的包头布局变了，"
                        "拦截已自动降级为全部放行 —— 请把这条日志报告给宿主维护者。",
                        header.packetId,
                        expectedId
                    );
                }
                return Verdict::Pass;
            }

            auto subs = snapshotFor(direction);
            if (subs.empty()) return Verdict::Pass;

            AddressPtr const address = addressOf(id);
            uint64_t const connId = id.getHash();

            // `curPtr`/`curLen` 是下一个订阅者看到的东西。起始是指进 `in` 的
            // 视图，只有当有人重写包体后才挪进 `owned`。
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
                        // 回调抛异常是模组侧的 bug，但它不许把连接一起带走。
                        // W11：……也不许悄无声息。异常每次都打印；「判定被强制
                        // 改成 PASS」的警告每进程一次。
                        verdict = PIER_PKT_PASS;
                        ll::error_utils::printCurrentException(hostLogger());
                        static std::atomic<bool> warned{false};
                        if (!warned.exchange(true))
                        {
                            hostLogger().warn(
                                "包钩子：某个模组回调抛了异常，其判定被强制改成 PASS。"
                                "这条警告只打一次；上面的异常每次都打。"
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
            out.reserve(curLen + 5); // 5 = varint32 头的上限
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
                try
                {
                    sub->cb(sub->user, connId, ps(*address), opened);
                }
                catch (...)
                {
                    // 与包路径同一条规矩：绝不让模组的异常逃进网络栈。
                    // W11：但要打日志 —— 静默的 catch(...) 会让 bug 永久隐形。
                    ll::error_utils::printCurrentException(hostLogger());
                }
            }
        }

        /* ───────────────────────── detour ───────────────────────── */

        /**
         * 出站。`data` 是完整序列化好的包（头 + 体），还没组批、没压缩。改写
         * = 用自己的字符串调 origin；丢弃 = 干脆不调。
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
         * 入站。每次 HasData 一个包。被丢弃的包不许把泵停下 —— 返回 NoData
         * 会把这一 tick 还排着队的所有包都困死 —— 所以改成拉下一个，直到有
         * 幸存者（或对端见底）才返回。
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

            // 跨迭代复用：一阵改写的包共用一个缓冲，而不是每包重分配。
            std::string rewritten;

            for (;;)
            {
                auto const status = origin(receiveBuffer, timepointPtr);
                if (status != ::NetworkPeer::DataStatus::HasData) return status;

                switch (dispatch(
                    PIER_PKT_INBOUND, this->mId.get(), receiveBuffer, rewritten, /*expectedId=*/-1))
                {
                case Verdict::Drop:
                    // 这里**不许**返回 NoData：那会把这一 tick 还排着队的包全
                    // 困死。向对端要下一个。
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

        /** 连接被接受 —— conn_id 存在的最早时刻。 */
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
         * 连接关闭。先通知、再逐出地址条目 —— 订阅者在 close 处理器里仍然想
         * 要一个能解析的地址。
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
         * 任意一种订阅第一次出现时把所有 detour 装齐一次。永不卸钩：退订可能
         * 来自钩体内部。
         *
         * 用函数局部 static 初始化，而不是这个桥里其他懒钩子用的裸 `bool` 标
         * 志 —— 那些全跑在服务器线程上，这里的注册不必是。
         */
        void ensureInstalled()
        {
            static bool const installed = []
            {
                PierPacketSendHook::hook();
                PierPacketReceiveHook::hook();
                PierConnOpenHook::hook();
                PierConnCloseHook::hook();
                hostLogger().debug("[PacketHooks] 已安装收发包 detour");
                return true;
            }();
            (void)installed;
        }

        /* ───────────────────────── ABI 入口 ───────────────────────── */

        PierPacketHookHandle
        api_packet_hook_register(PierModHandle mod, int32_t dirMask, PierPacketCb cb, void* user)
        {
            PIER_API_GUARD_BEGIN
                if (!cb) return nullptr;
                // 零掩码注册的是一个永远不会触发的东西；拒绝它，别递回去一个
                // 什么都不做的句柄。
                if ((dirMask & (PIER_PKT_MASK_INBOUND | PIER_PKT_MASK_OUTBOUND)) == 0) return nullptr;

                auto sub = std::make_shared<PacketSub>(PacketSub{asMod(mod), dirMask, cb, user});
                PacketSub* raw = sub.get();
                {
                    std::unique_lock<std::shared_mutex> g{registryLock()};
                    packetSubs().push_back(std::move(sub));
                    refreshGatesLocked();
                }
                ensureInstalled();
                return static_cast<PierPacketHookHandle>(raw);
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
                    // 归属检查：模组只能撤自己的拦截器。
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

                auto sub = std::make_shared<ConnSub>(ConnSub{asMod(mod), cb, user});
                ConnSub* raw = sub.get();
                {
                    std::unique_lock<std::shared_mutex> g{registryLock()};
                    connSubs().push_back(std::move(sub));
                    refreshGatesLocked();
                }
                ensureInstalled();
                return static_cast<PierPacketHookHandle>(raw);
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

        /** 拆除（stage 90）：清掉该模组名下的全部包/连接订阅。detour 留着
         *  （见文件头「生命周期」）—— 门会在下一次派发时把空注册表短路掉。 */
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
            api.packet_hook_unregister = &api_packet_hook_unregister;
            api.packet_conn_hook_register = &api_packet_conn_hook_register;
            api.packet_conn_hook_unregister = &api_packet_conn_hook_unregister;
        }

        spi::SlotPackReg regSlots{{"packet-hooks", &fill}};
        spi::TeardownReg regDown{{90, "packet-hooks", &teardown}};
    } // namespace
} // namespace pier::api_impl

#endif // !PIER_BUILD_CLIENT
