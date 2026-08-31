/** actors/Money.cpp —— 经济入口，背靠可选的 LLMoney（LegacyMoney）插件。
 *
 * LegacyMoney.dll 延迟加载，所以本 TU 照常编译、宿主在 LegacyMoney 未安装
 * 时照常启动。每个入口都由 moneyBackendReady() 把门（模组表 + 符号双查，
 * 见 money_guard.h）。后端缺席/禁用时返回安全默认值，而不是去调一个没解
 * 析的 LLMoney_* 桩 —— 那会抛延迟加载结构化异常，把 BDS 带走。 */
#ifndef PIER_BUILD_CLIENT

#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "LLMoney.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/Listener.h"
#include "ll/api/event/server/ServerStartedEvent.h"
#include "ll/api/mod/NativeMod.h"

#include "sdk/abi.h"

#include "pier/api/money_guard.h"
#include "pier/host/hosted_mod.h"
#include "pier/host/mod_host.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/log.h"
#include "pier/support/module.h"
#include "pier/support/snbt.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        /** 前向声明：蹦床的安装是惰性的（见下方「money 事件监听器」一节的
         *  ensureTrampolines），而每个经济入口在确认后端就绪之后都要顺手补装
         *  一次 —— 那些入口写在前面。 */
        void ensureTrampolines();

        /**
         * 余额。失败返回 -1，不是 0。
         *
         * 这是对着 LegacyMoney 的源码定的：`LLMoney_Get` 自己在 xuid 为空或
         * 数据库出错时就返回 -1，而正常余额不会是负数（`LLMoney_Trans` 拒绝
         * 负值、并在结果为负时回滚）。所以「< 0 = 问不出来」是这个槽位**已经
         * 存在**的约定 —— 后端缺席时也返回 -1 才是自洽的。
         *
         * 旧行为是缺席返回 0，那和「余额确实是 0」无法区分（契约 §5.2 点名
         * 反对的正是这种）。
         *
         * 另一件值得知道的事：`LLMoney_Get` 对没见过的 xuid 会建账（按
         * 配置的 def_money 插一行），所以这个调用不是无副作用的只读查询。
         */
        long long api_get_money(PierStr xuid)
        {
            PIER_API_GUARD_BEGIN
                if (!moneyBackendReady()) return -1;
                ensureTrampolines();
                return LLMoney_Get(toString(xuid));
            PIER_API_GUARD_END_VAL(-1)
        }

        bool api_set_money(PierStr xuid, long long money)
        {
            PIER_API_GUARD_BEGIN
                if (!moneyBackendReady()) return false;
                ensureTrampolines();
                return LLMoney_Set(toString(xuid), money);
            PIER_API_GUARD_END
        }

        bool api_add_money(PierStr xuid, long long money)
        {
            PIER_API_GUARD_BEGIN
                if (!moneyBackendReady()) return false;
                ensureTrampolines();
                return LLMoney_Add(toString(xuid), money);
            PIER_API_GUARD_END
        }

        bool api_reduce_money(PierStr xuid, long long money)
        {
            PIER_API_GUARD_BEGIN
                if (!moneyBackendReady()) return false;
                ensureTrampolines();
                return LLMoney_Reduce(toString(xuid), money);
            PIER_API_GUARD_END
        }

        bool api_trans_money(PierStr from, PierStr to, long long val, PierStr note)
        {
            PIER_API_GUARD_BEGIN
                if (!moneyBackendReady()) return false;
                ensureTrampolines();
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

        /*  money 事件监听器。
         *
         * LLMoney_ListenBeforeEvent 只 append，整个 LegacyMoney API 没有任何反注
         * 册，所以宿主只装一个常驻蹦床再自己扇出。扇出时不 break，让每个订阅者都看
         * 到这次变动，与 pier-hooks 的可取消事件同口径：判定不依赖注册顺序。
         * before 任一返回 false 即否决，after 全部通知。
         *
         * 这两个槽位早于 mod-scoped 约定，只收一个裸函数指针，没有模组句柄也没有
         * user 上下文；归属靠 addressOwnedBy() 从函数地址反查模块恢复。
         *
         * 蹦床要能补装：LL 装载阶段 LegacyMoney 尚未 enable，只在注册当时
         * moneyBackendReady() 为真才装的话，on_load 里注册的否决器永远不会生效。每
         * 个经济入口和每次注册都尝试补装，ServerStartedEvent 之后再补一次。
         */
        struct MoneyListener
        {
            PierMoneyCb cb = nullptr;
            void const* moduleBase = nullptr; // 注册时反查到的模块基址；查不到为 null
        };

        std::mutex gMoneyMutex;
        std::vector<MoneyListener> g_before;
        std::vector<MoneyListener> g_after;
        bool g_beforeHooked = false;
        bool g_afterHooked = false;

        /** 反查回调所属模块的基址：Pier 模组的回调落在它自己的 DLL 里。 */
        void const* moduleBaseOf(PierMoneyCb cb)
        {
            auto* host = ModHost::instance();
            if (!host) return nullptr;
            for (auto const& hosted : host->hostedMods())
            {
                void const* base = hosted->lib.handle();
                if (base && addressOwnedBy(base, reinterpret_cast<void const*>(cb))) return base;
            }
            return nullptr;
        }

        std::vector<PierMoneyCb> snapshotListeners(std::vector<MoneyListener> const& v)
        {
            std::vector<PierMoneyCb> out;
            std::lock_guard lock(gMoneyMutex);
            out.reserve(v.size());
            for (auto const& l : v) out.push_back(l.cb);
            return out;
        }

        /** 后端一旦就绪就把两个蹦床装上（幂等；每个经济入口都调）。 */
        void ensureTrampolines()
        {
            // 快路径：两个蹦床都装好之后这个函数在每次经济调用上都会被调到，
            // 不该再去翻一遍模组注册表（moneyBackendReady 会查 ModManagerRegistry）。
            if (g_beforeHooked && g_afterHooked) return;
            if (!moneyBackendReady()) return;
            if (!g_beforeHooked)
            {
                g_beforeHooked = true;
                LLMoney_ListenBeforeEvent(
                    [](::LLMoneyEvent t, std::string f, std::string to, long long v)
                    {
                        bool allow = true;
                        for (auto cb : snapshotListeners(g_before))
                        {
                            if (cb && !cb(static_cast<PierMoneyEvent>(t), ps(f), ps(to), v)) allow = false;
                        }
                        return allow;
                    });
            }
            if (!g_afterHooked)
            {
                g_afterHooked = true;
                LLMoney_ListenAfterEvent(
                    [](::LLMoneyEvent t, std::string f, std::string to, long long v)
                    {
                        for (auto cb : snapshotListeners(g_after))
                        {
                            if (cb) (void)cb(static_cast<PierMoneyEvent>(t), ps(f), ps(to), v);
                        }
                        return true;
                    });
            }
        }

        void addListener(std::vector<MoneyListener>& list, PierMoneyCb cb)
        {
            if (!cb) return;
            void const* base = moduleBaseOf(cb);
            if (!base)
            {
                hostLogger().warn(
                    "money_listen_*: 回调 {:p} 不属于任何已装载的 pier 模组，卸载时无法清理 —— "
                    "它会一直存活到进程结束。", reinterpret_cast<void const*>(cb));
            }
            std::lock_guard lock(gMoneyMutex);
            for (auto const& l : list)
            {
                if (l.cb == cb) return; // 幂等：同一回调不重复登记
            }
            list.push_back(MoneyListener{cb, base});
        }

        void api_money_listen_before_event(PierMoneyCb callback)
        {
            PIER_API_GUARD_BEGIN
                addListener(g_before, callback);
                ensureTrampolines();
            PIER_API_GUARD_END_VOID
        }

        void api_money_listen_after_event(PierMoneyCb callback)
        {
            PIER_API_GUARD_BEGIN
                addListener(g_after, callback);
                ensureTrampolines();
            PIER_API_GUARD_END_VOID
        }

        /** 服务器启动完成后再补装一次：此时 LegacyMoney 已 enable。 */
        std::shared_ptr<ll::event::ListenerBase> gStartedListener;

        void bootstrap()
        {
            auto listener = ll::event::Listener<ll::event::ServerStartedEvent>::create(
                [](ll::event::ServerStartedEvent&) { ensureTrampolines(); },
                ll::event::EventPriority::Normal,
                ll::mod::NativeMod::current()
            );
            if (ll::event::EventBus::getInstance().addListener<ll::event::ServerStartedEvent>(listener))
            {
                gStartedListener = listener;
            }
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

        /** 拆除（stage 100）：按注册时记下的模块基址清掉属于该模组的回调。
         *  蹦床留着不动 —— LegacyMoney 没有反注册。 */
        void teardown(HostedMod* mod)
        {
            if (!mod) return;
            void const* base = mod->lib.handle();
            std::lock_guard lock(gMoneyMutex);
            auto drop = [&](std::vector<MoneyListener>& v)
            {
                std::erase_if(v, [&](MoneyListener const& l)
                {
                    return l.moduleBase == base
                        || addressOwnedBy(base, reinterpret_cast<void const*>(l.cb));
                });
            };
            drop(g_before);
            drop(g_after);
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
        spi::BootstrapReg regBoot{{100, "money-trampoline", &bootstrap}};
    } // namespace
} // namespace pier::api_impl

#endif // !PIER_BUILD_CLIENT
