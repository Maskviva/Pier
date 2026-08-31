/** core/Services.cpp —— 跨模组服务注册表，问答式调用。
 *
 * 与 Bus.cpp 的单向广播在每个轴上都相反：同名提供方恰好一个而不是任意多个；没人
 * 注册是调用方必须处理的错误而不是正常；返回值是全部意义所在。两个提供方都应答
 * plot:can 不是「都跑一遍」，是一个调用方没法选的歧义答案，所以注册独占，同名的
 * 第二个注册者被大声拒绝。静默的后来者赢会让答案取决于模组装载顺序。
 *
 * 所有权纪律同本桥其余异步面：ModHost::unload 会 FreeLibrary，攥着别人的函数指针
 * 等于等着下一次调用崩，而且崩在调用方身上、日志里没有任何东西指向刚离场的那个。
 * 宿主持表、条目按票据编号、调用路径持 weak_ptr<HostedMod> 并在调用前一刻复核。
 *
 * 宿主不解析 request 与 reply，它们是带外约定的不透明 UTF-8。service_call 同步、
 * 就地跑提供方、无超时：阻塞的提供方阻塞的就是服务器线程，返回「超时」而回调继续
 * 跑等于递给调用方一个错答案还留着提供方在跑。环靠深度上限终止，自调用直接拒绝。
 */
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

#include "sdk/abi.h"

#include "pier/host/hosted_mod.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/log.h"
#include "pier/support/snbt.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        /** 收下的最长服务名。理由同总线的主题上限：够写
         *  `some-long-mod:some-query`，又短到垃圾指针变不成巨型 map 键。 */
        constexpr size_t kMaxName = 128;

        /** 嵌套调用上限。A → B → A 是深度 2；过了这个数的是环，不是调用链。 */
        constexpr int kMaxDepth = 8;

        struct Service
        {
            HostedMod* mod = nullptr; // 只作身份比对，永不解引用
            // 存活性经这个 weak_ptr 复核，不经 mod->shared_from_this()，后者本身
            // 就是一次盲解引用。service_call 可能跑在任何线程上，卸载期间那次解
            // 引用就是一次 UAF。
            std::weak_ptr<HostedMod> owner;
            std::string name;
            PierServiceCb cb = nullptr;
            void* user = nullptr;
        };

        std::mutex gMutex;
        /** 注册 id → 服务 */
        std::unordered_map<uint64_t, Service> gServices;
        /** 名字 → 注册 id。按构造恰好一个。 */
        std::unordered_map<std::string, uint64_t> gByName;
        uint64_t gNextId = 1;

        /** 按线程计的嵌套深度。thread_local 而非全局：两个线程并发调用不是
         *  环，共享计数会把它们看成环。 */
        thread_local int gDepth = 0;

        struct DepthGuard
        {
            DepthGuard() { ++gDepth; }
            ~DepthGuard() { --gDepth; }
        };

        /** 每个服务只喊一次「太深」—— 环转得和 CPU 一样快，每次触发都打
         *  日志会把一个 bug 变成一场事故。 */
        void warnDepthOnce(std::string const& name)
        {
            static std::mutex mu;
            static std::unordered_map<std::string, bool> seen;
            std::lock_guard lock(mu);
            if (seen[name]) return;
            seen[name] = true;
            hostLogger().error(
                "服务注册表：'{}' 超过调用深度 {} —— 拒绝最里层的调用。"
                "这是一个调用环：这个服务的提供方又把它调了一遍，直接调，"
                "或经由另一个绕回来的服务。",
                name, kMaxDepth
            );
        }

        uint64_t api_service_register(
            PierModHandle modHandle, PierStr nameRaw, PierServiceCb cb, void* user)
        {
            PIER_API_GUARD_BEGIN
                auto* mod = asMod(modHandle);
                if (!mod || !cb) return 0;

                std::string name = toString(nameRaw);
                if (name.empty() || name.size() > kMaxName) return 0;

                std::lock_guard lock(gMutex);
                if (auto it = gByName.find(name); it != gByName.end())
                {
                    // 拒绝并点出在位者的名字。只说「注册失败」会让读日志的人去自
                    // 己代码里找一个不存在的重复注册。
                    auto const& held = gServices[it->second];
                    char const* holder = held.mod ? held.mod->getName().c_str() : "?";
                    mod->getLogger().error(
                        "service_register('{}') 被拒：已由 '{}' 提供。服务名是独占的 —— "
                        "两个提供方会让答案取决于模组装载顺序。",
                        name, holder
                    );
                    return 0;
                }

                uint64_t const id = gNextId++;
                std::weak_ptr<HostedMod> owner;
                try
                {
                    owner = mod->shared_from_this(); // 注册跑在主线程、模组活着
                }
                catch (std::bad_weak_ptr const&)
                {
                    return 0;
                }
                gServices.emplace(id, Service{mod, owner, name, cb, user});
                gByName.emplace(name, id);
                return id;
            PIER_API_GUARD_END
        }

        bool api_service_unregister(PierModHandle modHandle, uint64_t regId)
        {
            PIER_API_GUARD_BEGIN
                auto* mod = asMod(modHandle);
                if (!mod || regId == 0) return false;

                std::lock_guard lock(gMutex);
                auto it = gServices.find(regId);
                if (it == gServices.end()) return false;
                // 只限本人：一个模组不许注销另一个的服务。和 bus_unsubscribe、
                // schedule_cancel 同一条规矩。
                if (it->second.mod != mod) return false;
                gByName.erase(it->second.name);
                gServices.erase(it);
                return true;
            PIER_API_GUARD_END
        }

        int32_t api_service_call(
            PierModHandle modHandle, PierStr nameRaw, PierStr requestRaw, void* ctx, PierStrSink reply)
        {
            PIER_API_GUARD_BEGIN
                std::string name = toString(nameRaw);
                if (name.empty() || name.size() > kMaxName) return PIER_SERVICE_REFUSED;

                if (gDepth >= kMaxDepth)
                {
                    warnDepthOnce(name);
                    return PIER_SERVICE_REFUSED;
                }

                auto* caller = modHandle ? asMod(modHandle) : nullptr;

                // 条目在锁内拷出，跨进 dylib 时锁已释放。提供方会重入（调别的服
                // 务、往总线发布、注册表单），持锁调进另一个模组时第一个重入的
                // 就锁死服务器线程。
                Service svc;
                {
                    std::lock_guard lock(gMutex);
                    auto byName = gByName.find(name);
                    if (byName == gByName.end()) return PIER_SERVICE_NOT_FOUND;
                    auto it = gServices.find(byName->second);
                    if (it == gServices.end()) return PIER_SERVICE_NOT_FOUND;
                    svc = it->second;
                }
                if (!svc.cb || !svc.mod) return PIER_SERVICE_NOT_FOUND;
                if (caller && svc.mod == caller) return PIER_SERVICE_REFUSED; // 不自调

                // 调用前一刻经 weak_ptr 复核：查表之后提供方可能已卸载，而票据表
                // 只在卸载路径上清理。锁的是注册时捕获的那个 weak_ptr，不解引用
                // 裸指针。
                auto provider = svc.owner.lock();
                if (!provider || provider.get() != svc.mod) return PIER_SERVICE_NOT_FOUND;

                // 这里不能查 isEnabled()。ModManager::enable() 在 onEnable 回调返
                // 回之后才把状态翻成 Enabled，而整个 load 阶段所有模组都还没
                // enable；查了会让模组在 on_load 里调 service::call 探测别人必然得
                // 到 NOT_FOUND。要防的「调进已 unmap 的代码」由上面的 weak_ptr 复
                // 核加指针相等挡着，与 enabled 无关。「禁用应表现为不存在」由提供
                // 方在 on_disable 里注销服务来表达。

                DepthGuard depth;
                bool ok = false;
                try
                {
                    ok = svc.cb(svc.user, ps(name), requestRaw, ctx, reply);
                }
                catch (...)
                {
                    // 提供方把异常抛过 FFI 边界在它那一侧已经是未定义行为。在这
                    // 里接住至少让调用方活着，并给它一个能据以行动的状态码。
                    hostLogger().error("服务 '{}' 把异常抛过了 FFI 边界", name);
                    return PIER_SERVICE_ERROR;
                }
                return ok ? PIER_SERVICE_OK : PIER_SERVICE_ERROR;
                // 0 是 SERVICE_OK，异常绝不能报成功；报 ERROR，即调用发生并失败。
            PIER_API_GUARD_END_VAL(PIER_SERVICE_ERROR)
        }

        void api_service_list(void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                std::string out = "[";
                {
                    std::lock_guard lock(gMutex);
                    bool first = true;
                    for (auto const& [name, id] : gByName)
                    {
                        auto it = gServices.find(id);
                        if (it == gServices.end()) continue;
                        if (!first) out += ',';
                        first = false;
                        char const* owner = it->second.mod ? it->second.mod->getName().c_str() : "?";
                        out += "{\"name\":\"";
                        out += snbtEscape(name);
                        out += "\",\"mod\":\"";
                        out += snbtEscape(owner);
                        out += "\"}";
                    }
                }
                out += ']';
                if (sink) sink(ctx, ps(out));
            PIER_API_GUARD_END_VOID
        }

        /** 拆除：注销该模组名下的全部服务。 */
        void teardown(HostedMod* mod)
        {
            if (!mod) return;
            std::lock_guard lock(gMutex);
            for (auto it = gServices.begin(); it != gServices.end();)
            {
                if (it->second.mod == mod)
                {
                    gByName.erase(it->second.name);
                    it = gServices.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        void fill(PierApi& api)
        {
            api.service_register = &api_service_register;
            api.service_unregister = &api_service_unregister;
            api.service_call = &api_service_call;
            api.service_list = &api_service_list;
        }

        spi::SlotPackReg regSlots{{"services", &fill}};
        spi::TeardownReg regDown{{30, "services", &teardown}};
    } // namespace
} // namespace pier::api_impl
