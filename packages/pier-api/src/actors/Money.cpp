/** actors/Money.cpp —— 经济入口，背靠**可选的** LLMoney（LegacyMoney）插件。
 *
 * LegacyMoney.dll 延迟加载，所以本 TU 照常编译、宿主在 LegacyMoney 未安装
 * 时照常启动。每个入口都由 moneyBackendReady() 把门（模组表 + 符号双查，
 * 见 money_guard.h）。后端缺席/禁用时返回安全默认值，而不是去调一个没解
 * 析的 LLMoney_* 桩 —— 那会抛延迟加载结构化异常，把 BDS 带走。 */
#ifndef PIER_BUILD_CLIENT

#include <string>

#include "LLMoney.h"

#include "sdk/abi.h"

#include "pier/api/money_guard.h"
#include "pier/host/hosted_mod.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/module.h"
#include "pier/support/snbt.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        long long api_get_money(PierStr xuid)
        {
            PIER_API_GUARD_BEGIN
                if (!moneyBackendReady()) return 0;
                return LLMoney_Get(toString(xuid));
            PIER_API_GUARD_END
        }

        bool api_set_money(PierStr xuid, long long money)
        {
            PIER_API_GUARD_BEGIN
                if (!moneyBackendReady()) return false;
                return LLMoney_Set(toString(xuid), money);
            PIER_API_GUARD_END
        }

        bool api_add_money(PierStr xuid, long long money)
        {
            PIER_API_GUARD_BEGIN
                if (!moneyBackendReady()) return false;
                return LLMoney_Add(toString(xuid), money);
            PIER_API_GUARD_END
        }

        bool api_reduce_money(PierStr xuid, long long money)
        {
            PIER_API_GUARD_BEGIN
                if (!moneyBackendReady()) return false;
                return LLMoney_Reduce(toString(xuid), money);
            PIER_API_GUARD_END
        }

        bool api_trans_money(PierStr from, PierStr to, long long val, PierStr note)
        {
            PIER_API_GUARD_BEGIN
                if (!moneyBackendReady()) return false;
                return LLMoney_Trans(toString(from), toString(to), val, toString(note));
            PIER_API_GUARD_END
        }

        void api_money_get_hist(PierStr xuid, int timediff, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                // 后端缺席 → 没有记录。干脆不调 sink，让另一侧看到空历史
                //（等价于「什么都没查到」）。
                if (!moneyBackendReady()) return;
                sink(ctx, ps(LLMoney_GetHist(toString(xuid), timediff)));
            PIER_API_GUARD_END_VOID
        }

        void api_money_clear_hist(int difftime)
        {
            PIER_API_GUARD_BEGIN
                if (!moneyBackendReady()) return;
                LLMoney_ClearHist(difftime);
            PIER_API_GUARD_END_VOID
        }

        /* ── money 事件监听器 ─────────────────────────────────────────────
         *
         * LegacyMoney 的实现（它自己的 src/Event.cpp）是：
         *
         *     void LLMoney_ListenBeforeEvent(cb) { beforeCallbacks.push_back(cb); }
         *
         * 只 append，整个 LegacyMoney API 里没有任何反注册。两个推论：
         *
         *   1. 宿主只能装**一个常驻蹦床**、自己扇出。早先的写法每次注册都往
         *      LegacyMoney 再 push 一个蹦床，注册两次就是每笔交易派发两遍。
         *   2. 这两个槽位早于 mod-scoped 约定，只收一个裸函数指针，没有模组
         *      句柄也没有 user 上下文。宿主因此不知道回调属于谁，卸载时清不
         *      掉 —— 指针活过 FreeLibrary，下一笔交易跳进未映射内存。归属靠
         *      addressOwnedBy() 从函数地址反查模块来恢复。
         *
         * 遗留形状还带来一个改不了的限制：每种只能有一个监听器，第二个模组
         * 注册会静默顶掉第一个。签名不能变（ABI 只能追加），要修得追加一对
         * 带模组句柄和 user 的新槽位。
         */
        PierMoneyCb g_before = nullptr;
        PierMoneyCb g_after = nullptr;
        bool g_beforeHooked = false;
        bool g_afterHooked = false;

        void api_money_listen_before_event(PierMoneyCb callback)
        {
            PIER_API_GUARD_BEGIN
                // 无条件存下：后端可能稍后才出现，而蹦床是在派发时读
                // g_before 的，不是在安装时。
                g_before = callback;
                if (g_beforeHooked || !moneyBackendReady()) return;
                g_beforeHooked = true;
                LLMoney_ListenBeforeEvent(
                    [](::LLMoneyEvent t, std::string f, std::string to, long long v)
                    {
                        return g_before
                            ? g_before(static_cast<PierMoneyEvent>(t), ps(f), ps(to), v)
                            : true;
                    });
            PIER_API_GUARD_END_VOID
        }

        void api_money_listen_after_event(PierMoneyCb callback)
        {
            PIER_API_GUARD_BEGIN
                g_after = callback;
                if (g_afterHooked || !moneyBackendReady()) return;
                g_afterHooked = true;
                LLMoney_ListenAfterEvent(
                    [](::LLMoneyEvent t, std::string f, std::string to, long long v)
                    {
                        return g_after
                            ? g_after(static_cast<PierMoneyEvent>(t), ps(f), ps(to), v)
                            : true;
                    });
            PIER_API_GUARD_END_VOID
        }

        void api_money_ranking(unsigned short num, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                if (!moneyBackendReady()) return;
                for (auto const& [x, m] : LLMoney_Ranking(num))
                {
                    sink(ctx, ps(x + ":" + snbtNum(m)));
                }
            PIER_API_GUARD_END_VOID
        }

        /** 拆除（stage 100）：按函数地址反查归属，清掉属于该模组的回调。
         *  蹦床留着不动 —— LegacyMoney 没有反注册。回调为空时它返回 true
         *（= 不取消），这是正确的中立答案。 */
        void teardown(HostedMod* mod)
        {
            if (!mod) return;
            void const* base = mod->lib.handle();
            if (addressOwnedBy(base, reinterpret_cast<void const*>(g_before))) g_before = nullptr;
            if (addressOwnedBy(base, reinterpret_cast<void const*>(g_after))) g_after = nullptr;
        }

        void fill(PierApi& api)
        {
            api.get_money = &api_get_money;
            api.set_money = &api_set_money;
            api.add_money = &api_add_money;
            api.reduce_money = &api_reduce_money;
            api.trans_money = &api_trans_money;
            api.money_get_hist = &api_money_get_hist;
            api.money_clear_hist = &api_money_clear_hist;
            api.money_listen_before_event = &api_money_listen_before_event;
            api.money_listen_after_event = &api_money_listen_after_event;
            api.money_ranking = &api_money_ranking;
        }

        spi::SlotPackReg regSlots{{"money", &fill}};
        spi::TeardownReg regDown{{100, "money", &teardown}};
    } // namespace
} // namespace pier::api_impl

#endif // !PIER_BUILD_CLIENT
