/**
 * pier-lane/Lane.cpp —— 同工具链快车道。
 *
 * # 这条车道解决的问题
 *
 * `service_call` 的形状是 `(名字, UTF-8) -> UTF-8`。那是**跨语言**的最大公
 * 约数，所以它必须存在。但两个同语言模组之间用它，等于每次判定都序列化一
 * 遍，而且类型信息在字符串里全丢了 —— 字段名打错的表现是「这个人没权
 * 限」，一个永远不会被当成 bug 报上来的现象。
 *
 * 这条车道只服务一个特例：**两边由同一次工具链编出来**。那时两个 cdylib 里
 * `#[repr(C)]` 函数表的布局逐字节相同，指针可以直接递。
 *
 * # 宿主为什么必须掺一脚
 *
 * 直觉方案是「模组 A 导出一个 symbol，模组 B dlsym 过去」。做不到，理由和
 * Bus.cpp 开头写的一模一样，但更狠一层：
 *
 *   - Bus 的问题是**回调指针**悬垂；
 *   - 这里的问题是**整张函数表**悬垂。`ModHost::unload` 调 `FreeLibrary`，
 *     提供方的内存可以靠引用计数活着，但它的**代码段会被 unmap**。
 *
 * 而消费方没有任何办法自己发现这件事 —— 它手里只有一个指针，指针不会变。
 *
 * 所以宿主提供两样东西，只有两样：
 *
 *   1. **一格永不释放的存活标志。** 消费方每次调用前读一次。提供方走掉时宿
 *      主写 0。这一格在宿主自己的堆上，`FreeLibrary` 碰不到它，所以卸载之
 *      后读它仍然合法 —— 那正是它存在的全部理由。
 *
 *      「永不释放」是有意的：车道数量是几十，泄漏几十个 `atomic<uint32_t>`
 *      换掉一整类 use-after-free。反过来（引用计数它、最后一个租约还了就释
 *      放）要求消费方在归还租约之后**再也不读**那个指针，而那正是它会忘记
 *      做的事。
 *
 *   2. **卸载时替所有未归还的租约补调 `release`。** 在 `FreeLibrary` 之前跑
 *      （Teardown 阶段的顺序保证了这一点），所以提供方是在自己的 dylib 里、
 *      用自己的分配器释放自己的东西。这条不做的话，「消费方忘了归还」就变成
 *      「提供方永远卸载不干净」。
 *
 * # 宿主不做的事
 *
 * 不解释 `data` 和 `vtable` 里的任何一个字节，和总线不解析载荷完全一样。也
 * 不解释指纹的**含义** —— 只做相等比较。否则「指纹里多加一项」就成了一次
 * ABI 变更，而那一项是 SDK 侧的实现细节。
 *
 * # 热路径上宿主一行代码都不跑
 *
 * `lane_acquire` 一次，之后消费方手里是 `{data, vtable, alive}` 三个裸指
 * 针。每次调用 = 一次原子读 + 一次间接调用。这就是「高速」的实际含义 ——
 * 不是「JSON 解析快一点」，是**根本没有宿主参与**。
 */
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "sdk/abi.h"

#include "pier/host/hosted_mod.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/log.h"
#include "pier/support/snbt.h"
#include "pier/support/str.h"

namespace pier::lane
{
    namespace
    {
        /** 和服务名一样的上限，理由也一样：够写 `some-mod:some-lane`，又短到
         *  一个野指针被当成字符串读时变不成几兆的 map 键。 */
        constexpr size_t kMaxName = 128;

        /** 最小 JSON 字符串转义。lane_list 是手拼 JSON 的，名字是外部输入。 */
        std::string jsonEscape(std::string_view s)
        {
            std::string out;
            out.reserve(s.size() + 8);
            for (char c : s)
            {
                switch (c)
                {
                case '"':
                    out += "\\\"";
                    break;
                case '\\':
                    out += "\\\\";
                    break;
                case '\n':
                    out += "\\n";
                    break;
                case '\r':
                    out += "\\r";
                    break;
                case '\t':
                    out += "\\t";
                    break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20)
                    {
                        char buf[8];
                        std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                        out += buf;
                    }
                    else
                    {
                        out.push_back(c);
                    }
                    break;
                }
            }
            return out;
        }

        /**
         * 一条车道的存活标志。
         *
         * 单独一个堆分配、**永不 delete**。地址必须在提供方 `FreeLibrary` 之
         * 后依然可读 —— 消费方就是靠读它来发现提供方走了的，如果这一格本身
         * 被释放了，那个检查自己就成了 use-after-free。
         */
        struct AliveCell
        {
            std::atomic<uint32_t> flag{1};
            /** 消费方正停在这条车道的表项里的次数。见 PierLaneRef::busy。
             *  和 flag 同住一格、同样永不释放 —— 卸载路径读它的时候，持有它
             *  的那个消费方可能已经没了。 */
            std::atomic<uint32_t> busy{0};
        };

        struct Lane
        {
            HostedMod* mod = nullptr; // 只作身份用：只比较指针值，绝不解引用
            // W13：存活复核走这个 weak_ptr，而不是 `mod->shared_from_this()`
            // —— 后者本身就是一次盲解引用；主线程上 unload 与调用串行所以撞
            // 不上，但 acquire 允许别的线程调。
            std::weak_ptr<HostedMod> owner;
            std::string name;
            PierLaneDesc desc{};
            AliveCell* alive = nullptr; // 泄漏的，见上
            uint32_t leases = 0;
        };

        struct Lease
        {
            HostedMod* holder = nullptr; // 消费方
            uint64_t laneId = 0;
        };

        std::mutex gMutex;
        std::unordered_map<uint64_t, Lane> gLanes;         // publish id -> 车道
        std::unordered_map<std::string, uint64_t> gByName; // 名字 -> publish id（独占）
        std::unordered_map<uint64_t, Lease> gLeases;       // lease id -> 租约
        uint64_t gNextLaneId = 1;
        uint64_t gNextLeaseId = 1;

        /**
         * 提供方的模组还在不在。和 Bus/Services 一样：拿 weak_ptr 复核，别信
         * 裸指针。
         *
         * # 这里**故意不查 `isEnabled()`**
         *
         * 查过。结果是两条车道永远发布不出去，而错误信息说的是「名字被占
         * 了」。
         *
         * 原因在 LeviLamina 那边：`ModManager::enable()` 先调 `onEnable` 回
         * 调，**回调返回之后**才把模组的状态翻成 Enabled。所以一个模组在自己
         * 的 `on_load` 或 `on_enable` 里发布车道时，`isEnabled()` 一律是
         * `false` —— 而那两个地方正是发布车道唯一合理的时机。
         *
         * 而且这个检查本来也保护不了什么。它想挡的是「别调进一段已经 unmap
         * 的代码」，但那件事由两个东西挡着，都和 enabled 无关：
         *
         * 1. 存活标志（宿主持有、永不释放的那格 atomic），卸载时写 0
         * 2. `retireLane`，在 `lib.free()` 之前替所有租约补调 `release`
         *
         * 「模组被禁用但还没卸载」时它的代码段仍然是映射着的，函数表仍然可
         * 调。想在禁用时停掉车道的模组应该在自己的 `on_disable` 里撤销，那是
         * 它的决定，不该由宿主替它做 —— 尤其不该以「发布失败」这种形式。
         */
        bool providerAlive(Lane const& lane)
        {
            if (!lane.mod) return false;
            auto mod = lane.owner.lock();
            return mod && mod.get() == lane.mod;
        }

        /**
         * 撤销一条车道：清存活标志 → 替所有未归还的租约补调 release → 摘表。
         *
         * **调用时不许持 gMutex**：`release` 会跳进提供方的 dylib，而那边完全
         * 可能再调回 `lane_*`（比如在 Drop 里撤销自己另一条车道）。持锁跨过
         * dylib 边界的第一次重入就是死锁 —— 总线的派发路径已经踩过一次了。
         */
        void retireLane(uint64_t laneId)
        {
            PierLaneRefFn release = nullptr;
            void* data = nullptr;
            uint32_t outstanding = 0;
            std::string name;

            {
                std::lock_guard lock(gMutex);
                auto it = gLanes.find(laneId);
                if (it == gLanes.end()) return;
                Lane& lane = it->second;

                // 先断电再拆线：任何还没跑到调用点的消费方从这一刻起看到的是
                // 「没了」，而不是一张即将失效的表。
                if (lane.alive) lane.alive->flag.store(0, std::memory_order_release);

                release = lane.desc.release;
                data = lane.desc.data;
                name = lane.name;

                for (auto it2 = gLeases.begin(); it2 != gLeases.end();)
                {
                    if (it2->second.laneId == laneId)
                    {
                        ++outstanding;
                        it2 = gLeases.erase(it2);
                    }
                    else
                    {
                        ++it2;
                    }
                }

                auto byName = gByName.find(lane.name);
                if (byName != gByName.end() && byName->second == laneId) gByName.erase(byName);
                gLanes.erase(it);
                // AliveCell 有意不 delete —— 见文件头。
            }

            // 锁外。发布时自己持有的那一份 + 每个未归还的租约各一份。
            if (release)
            {
                release(data); // publish 时的那一份
                for (uint32_t i = 0; i < outstanding; ++i) release(data);
            }
            if (outstanding > 0)
            {
                hostLogger().warn(
                    "快车道 '{}'：撤销时还有 {} 个租约没归还，已代为释放。消费方模组在卸载前"
                    "应该自己 drop 掉车道句柄 —— 这条日志说明有一个没做到。",
                    name, outstanding
                );
            }
        }

        uint64_t api_lane_publish(PierModHandle modHandle, PierStr nameRaw, PierLaneDesc const* desc)
        {
            PIER_API_GUARD_BEGIN
                if (!modHandle || !desc) return 0;
                auto* raw = asMod(modHandle);
                if (!raw) return 0;

                if (desc->struct_size < sizeof(PierLaneDesc))
                {
                    hostLogger().error(
                        "快车道：PierLaneDesc 比宿主认识的小（{} < {}）—— "
                        "模组是针对更旧的 ABI 编的，拒绝发布。",
                        desc->struct_size, static_cast<uint32_t>(sizeof(PierLaneDesc))
                    );
                    return 0;
                }
                if (desc->protocol != PIER_LANE_PROTOCOL)
                {
                    hostLogger().error(
                        "快车道：协议版本 {} != 宿主的 {}，拒绝发布。这一条拒绝只影响这条车道，"
                        "模组本身照常加载，消费方会降级回 service 通道。",
                        desc->protocol, PIER_LANE_PROTOCOL
                    );
                    return 0;
                }
                if (!desc->vtable) return 0;
                if (desc->fingerprint == 0)
                {
                    // 0 在 acquire 那一侧是「不校验」的意思。提供方报 0 等于把
                    // 闸门自己拆了，而拆闸门的后果是静默的内存错乱，不是崩溃。
                    hostLogger().error("快车道：指纹为 0 是保留值，拒绝发布。");
                    return 0;
                }

                std::string name = toString(nameRaw);
                if (name.empty() || name.size() > kMaxName) return 0;

                // W13：发布只能由模组自己在主线程上做，此时它一定活着；
                // weak_ptr 就在这里、这一次拿。
                std::weak_ptr<HostedMod> owner;
                try
                {
                    owner = raw->shared_from_this();
                }
                catch (std::bad_weak_ptr const&)
                {
                    return 0;
                }
                if (!owner.lock()) return 0;

                std::lock_guard lock(gMutex);
                auto taken = gByName.find(name);
                if (taken != gByName.end())
                {
                    // 和 service 一样是**硬失败**。两个模组提供同一条车道不是
                    // 「都跑」，是一个消费方没法挑选的歧义答案；静默后来居上会
                    // 让结果取决于模组加载顺序，而那个顺序在装了任何一个不相干
                    // 的模组之后就会变。
                    auto held = gLanes.find(taken->second);
                    std::string holder = held != gLanes.end() && held->second.mod
                                             ? std::string(held->second.mod->getName())
                                             : std::string("<unknown>");
                    hostLogger().error(
                        "快车道 '{}' 已经被模组 '{}' 占用，拒绝第二个发布者。", name, holder
                    );
                    return 0;
                }

                uint64_t id = gNextLaneId++;
                Lane lane;
                lane.mod = raw;
                lane.owner = owner;
                lane.name = name;
                lane.desc = *desc;
                lane.desc.struct_size = static_cast<uint32_t>(sizeof(PierLaneDesc));
                lane.alive = new AliveCell(); // 有意泄漏
                gLanes.emplace(id, lane);
                gByName.emplace(name, id);

                hostLogger().debug(
                    "快车道 '{}' 上线（指纹 0x{:016x}）", name, desc->fingerprint
                );
                return id;
            PIER_API_GUARD_END
        }

        bool api_lane_unpublish(PierModHandle modHandle, uint64_t pubId)
        {
            PIER_API_GUARD_BEGIN
                if (!modHandle || pubId == 0) return false;
                auto* raw = asMod(modHandle);
                {
                    std::lock_guard lock(gMutex);
                    auto it = gLanes.find(pubId);
                    // 限定在调用方自己名下：一个模组不能撤销另一个模组的车道。
                    if (it == gLanes.end() || it->second.mod != raw) return false;
                }
                retireLane(pubId);
                return true;
            PIER_API_GUARD_END
        }

        int32_t api_lane_acquire(
            PierModHandle modHandle, PierStr nameRaw, uint64_t wantFingerprint, PierLaneRef* out)
        {
            PIER_API_GUARD_BEGIN
                // 只要求到 `alive` 为止 —— 那是这条车道能用的最小形状。要求
                // sizeof(PierLaneRef) 会让每次追加字段都把老消费方一刀切掉，
                // 正是 abi.h 顶上那条「追加式变更不该收窄可加载范围」在说的事。
                constexpr uint32_t kMinRefSize =
                    offsetof(PierLaneRef, alive) + sizeof(uint32_t const*);
                if (!out || out->struct_size < kMinRefSize) return PIER_LANE_REFUSED;

                // 先清干净。半填的 out 加上一个被忽略的返回码，等于把野指针交
                // 出去。
                out->lease = 0;
                out->fingerprint = 0;
                out->data = nullptr;
                out->vtable = nullptr;
                out->alive = nullptr;
                if (out->struct_size >= offsetof(PierLaneRef, busy) + sizeof(uint32_t*))
                {
                    out->busy = nullptr;
                }

                if (!modHandle) return PIER_LANE_REFUSED;
                auto* consumer = asMod(modHandle);
                if (!consumer) return PIER_LANE_REFUSED;

                std::string name = toString(nameRaw);
                if (name.empty() || name.size() > kMaxName) return PIER_LANE_REFUSED;

                PierLaneRefFn retain = nullptr;
                void* data = nullptr;
                uint64_t leaseId = 0;

                {
                    std::lock_guard lock(gMutex);
                    auto byName = gByName.find(name);
                    if (byName == gByName.end()) return PIER_LANE_NOT_FOUND;
                    auto it = gLanes.find(byName->second);
                    if (it == gLanes.end()) return PIER_LANE_NOT_FOUND;
                    Lane& lane = it->second;

                    // 自取没有意义：同一个模组里直接调那个函数就行，不用绕两次
                    // FFI 加一把锁，而且真构成循环时那是最难读的一种栈形状。
                    if (lane.mod == consumer) return PIER_LANE_REFUSED;
                    if (!providerAlive(lane)) return PIER_LANE_NOT_FOUND;

                    // 指纹先于一切。这是整条车道存在的前提：布局没确认相同之
                    // 前，一个指针都不能递出去。填 fingerprint 是为了让消费方能
                    // 打出一条**能指导下一步**的日志 ——「不匹配」四个字对服主
                    // 没用。
                    out->fingerprint = lane.desc.fingerprint;

                    // 0 以前的含义是「不校验」，注释里写着「只有诊断工具该这么
                    // 用」。那个口子必须堵上，因为它给出去的不是诊断数据，是完
                    // 整的 vtable + data 裸指针 —— 消费方随后会把它当成自己的
                    // 函数表解引用、按自己的偏移调函数指针。布局没核对过就递指
                    // 针，正是本文件开头那句「布局没确认相同之前一个指针都不能
                    // 递出去」要防的事，而这是唯一能绕过它的路径。
                    //
                    // 诊断需求由 lane_list 覆盖（名字 / 模组 / 指纹 / 协议 /
                    // 租约数），它一个指针都不用给。
                    //
                    // SDK 侧的指纹函数保证永远不会送 0 上来（算出 0 就换 1），
                    // 所以这个拒绝对正常调用方不可见。
                    if (wantFingerprint == 0 || wantFingerprint != lane.desc.fingerprint)
                    {
                        return PIER_LANE_FINGERPRINT;
                    }

                    leaseId = gNextLeaseId++;
                    gLeases.emplace(leaseId, Lease{consumer, byName->second});
                    ++lane.leases;

                    retain = lane.desc.retain;
                    data = lane.desc.data;

                    out->lease = leaseId;
                    out->data = lane.desc.data;
                    out->vtable = lane.desc.vtable;
                    out->alive =
                        lane.alive ? reinterpret_cast<uint32_t const*>(&lane.alive->flag) : nullptr;
                    // 追加字段：老消费方填的 struct_size 到不了这里，不写就是了。
                    if (out->struct_size >= offsetof(PierLaneRef, busy) + sizeof(uint32_t*))
                    {
                        out->busy =
                            lane.alive ? reinterpret_cast<uint32_t*>(&lane.alive->busy) : nullptr;
                    }
                }

                // 锁外，理由同 retireLane：retain 跳进提供方的 dylib。
                if (retain) retain(data);
                return PIER_LANE_OK;
                // 0 是 LANE_OK：异常时绝不能说「拿到了」—— REFUSED 和它其余的
                // 拒绝路径一致。
            PIER_API_GUARD_END_VAL(PIER_LANE_REFUSED)
        }

        bool api_lane_release(PierModHandle modHandle, uint64_t leaseId)
        {
            PIER_API_GUARD_BEGIN
                if (!modHandle || leaseId == 0) return false;
                auto* consumer = asMod(modHandle);

                PierLaneRefFn release = nullptr;
                void* data = nullptr;
                {
                    std::lock_guard lock(gMutex);
                    auto it = gLeases.find(leaseId);
                    // 提供方走掉时宿主已经替这条租约调过 release 并把它摘了。
                    // 这里返回 false 而不是再调一次 —— 再调一次就是 double
                    // free。
                    if (it == gLeases.end()) return false;
                    if (it->second.holder != consumer) return false;

                    auto lane = gLanes.find(it->second.laneId);
                    if (lane != gLanes.end())
                    {
                        if (lane->second.leases > 0) --lane->second.leases;
                        release = lane->second.desc.release;
                        data = lane->second.desc.data;
                    }
                    gLeases.erase(it);
                }
                if (release) release(data);
                return true;
            PIER_API_GUARD_END
        }

        void api_lane_list(void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                std::string out = "[";
                {
                    std::lock_guard lock(gMutex);
                    bool first = true;
                    for (auto const& [id, lane] : gLanes)
                    {
                        if (!first) out += ',';
                        first = false;
                        char fp[32];
                        std::snprintf(fp, sizeof(fp), "0x%016llx",
                                      static_cast<unsigned long long>(lane.desc.fingerprint));
                        // 转义：车道名由提供方给，模组名来自 manifest —— 两个
                        // 都是外部输入，直接拼进 JSON 里一个引号就能把这份输出
                        // 撕开。
                        out += "{\"name\":\"";
                        out += jsonEscape(lane.name);
                        out += "\",\"mod\":\"";
                        out += jsonEscape(lane.mod ? std::string(lane.mod->getName())
                                                   : std::string("?"));
                        out += "\",\"fingerprint\":\"";
                        out += fp;
                        out += "\",\"protocol\":";
                        out += snbtNum(lane.desc.protocol);
                        out += ",\"leases\":";
                        out += snbtNum(lane.leases);
                        out += ",\"alive\":";
                        out += (lane.alive && lane.alive->flag.load(std::memory_order_acquire))
                                   ? "true"
                                   : "false";
                        out += '}';
                    }
                }
                out += ']';
                if (sink) sink(ctx, ps(out));
            PIER_API_GUARD_END_VOID
        }

        /**
         * 卸载否决：这个模组提供的车道里，有没有哪条正停在调用中。
         *
         * 「全部服务器线程调用」挡住了并发卸载，挡不住重入卸载：提供方的表项
         * 自己触发一次命令派发、那条命令把提供方卸了，于是 FreeLibrary 发生在
         * 一个仍然停在提供方代码里的栈帧下面。存活标志对此无能为力 —— 消费方
         * 早就读过它了。
         *
         * 所以在这里拒绝，而不是先卸再崩。返回车道名给调用方拼错误信息用；名
         * 字活在 gLanes 里，由宿主持有，和提供方的 dylib 无关。
         */
        char const* vetoWhy(HostedMod* mod)
        {
            static std::string held;
            std::lock_guard lock(gMutex);
            for (auto const& [id, lane] : gLanes)
            {
                if (lane.mod != mod || !lane.alive) continue;
                if (lane.alive->busy.load(std::memory_order_acquire) != 0)
                {
                    held = "快车道 '" + lane.name + "' 正停在调用中";
                    return held.c_str();
                }
            }
            return nullptr;
        }

        /** 拆除（stage 40）。两件事，顺序要紧。 */
        void teardown(HostedMod* mod)
        {
            // 一、先把这个模组**消费**的租约还掉。提供方还活着，`release` 能正
            //     常跑；不还的话提供方的状态会被一个已经不存在的模组一直
            //     retain 着。
            std::vector<uint64_t> mine;
            {
                std::lock_guard lock(gMutex);
                for (auto const& [id, lease] : gLeases)
                {
                    if (lease.holder == mod) mine.push_back(id);
                }
            }
            for (uint64_t leaseId : mine)
            {
                PierLaneRefFn release = nullptr;
                void* data = nullptr;
                {
                    std::lock_guard lock(gMutex);
                    auto it = gLeases.find(leaseId);
                    if (it == gLeases.end()) continue;
                    auto lane = gLanes.find(it->second.laneId);
                    if (lane != gLanes.end())
                    {
                        if (lane->second.leases > 0) --lane->second.leases;
                        release = lane->second.desc.release;
                        data = lane->second.desc.data;
                    }
                    gLeases.erase(it);
                }
                if (release) release(data);
            }

            // 二、再撤掉它**发布**的车道。这一步必须在 `FreeLibrary` 之前完成
            //     —— Teardown 全部阶段跑完之后 ModHost 才 lib.free()，顺序由那
            //     里保证；否则 `release` 就跳进了已经 unmap 的代码段。
            std::vector<uint64_t> published;
            {
                std::lock_guard lock(gMutex);
                for (auto const& [id, lane] : gLanes)
                {
                    if (lane.mod == mod) published.push_back(id);
                }
            }
            for (uint64_t id : published) retireLane(id);
        }

        void fill(PierApi& api)
        {
            api.lane_publish = &api_lane_publish;
            api.lane_unpublish = &api_lane_unpublish;
            api.lane_acquire = &api_lane_acquire;
            api.lane_release = &api_lane_release;
            api.lane_list = &api_lane_list;
        }

        spi::SlotPackReg regSlots{{"lane", &fill}};
        spi::UnloadVetoReg regVeto{{"lane", &vetoWhy}};
        spi::TeardownReg regDown{{40, "lane", &teardown}};
    } // namespace
} // namespace pier::lane
