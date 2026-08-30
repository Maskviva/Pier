#include "pier/api/money_guard.h"

#ifndef PIER_BUILD_CLIENT

#include <atomic>
#include <string_view>

#include "ll/api/memory/Symbol.h"
#include "ll/api/mod/Mod.h"
#include "ll/api/mod/ModManagerRegistry.h"

#include "pier/support/log.h"

namespace pier::api_impl
{
    namespace
    {
        // LegacyMoney 在 LeviLamina manifest 里的名字。
        constexpr std::string_view kMoneyModName = "LegacyMoney";

        // LegacyMoney 的一个稳定导出符号。它声明为 extern "C"，x64 MSVC 上
        // 导出名不修饰 —— 正是这串字符。LLMoney_Get 在，整个 LLMoney_* 家
        // 族就都在（它们一起发布）。
        constexpr std::string_view kProbeSymbol = "LLMoney_Get";

        // -1 = 还没探测；0 = 符号缺失；1 = 符号在。
        // 拥有符号的 DLL 没法在进程中途换掉，所以观测到一次就永远不用再
        // 解析。
        std::atomic<int> gSymbolState{-1};

        // 不管哪个检查先失败，每进程只告警一次。
        std::atomic_flag gWarned = ATOMIC_FLAG_INIT;

        bool symbolPresent()
        {
            int cached = gSymbolState.load(std::memory_order_acquire);
            if (cached >= 0)
            {
                return cached == 1;
            }
            // resolve() 带 disableErrorOutput=true 时未命中返回 nullptr 且
            // **不**刷日志 —— 措辞归我们这边管。
            void* addr = ll::memory::SymbolView{kProbeSymbol}.resolve(true);
            int result = addr != nullptr ? 1 : 0;
            // 良性竞争：两个线程可能都解析一遍；答案相同，后写者赢。
            gSymbolState.store(result, std::memory_order_release);
            return result == 1;
        }

        bool modLoadedAndEnabled()
        {
            auto& registry = ll::mod::ModManagerRegistry::getInstance();
            if (!registry.hasMod(kMoneyModName))
            {
                return false;
            }
            auto mod = registry.getMod(kMoneyModName);
            return mod && mod->isEnabled();
        }

        void warnOnce(std::string_view reason)
        {
            if (gWarned.test_and_set(std::memory_order_relaxed))
            {
                return; // 已经警告过
            }
            hostLogger().warn(
                "找不到可用的 LLMoney 后端（{}）。money::* 接口本次全部空转："
                "读取返回 0、写入返回失败。请检查是否安装并启用了 LegacyMoney"
                "（模组名 \"{}\"）。",
                reason,
                kMoneyModName
            );
        }
    } // namespace

    bool moneyBackendReady() noexcept
    {
        // 先做便宜的检查（模组表 + 状态），再做 memoize 过的符号探测。
        // 任一失败即「未就绪」。
        if (!modLoadedAndEnabled())
        {
            warnOnce("模组列表里没有已启用的 LegacyMoney");
            return false;
        }
        if (!symbolPresent())
        {
            warnOnce("LegacyMoney 已加载，但解析不到导出符号 LLMoney_Get（版本不匹配或 DLL 损坏？）");
            return false;
        }
        return true;
    }
} // namespace pier::api_impl

#endif // !PIER_BUILD_CLIENT
