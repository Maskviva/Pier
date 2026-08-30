/** core/Scheduler.cpp —— 计划任务：无主槽（历史）+ 按模组记账的槽 + 卸载清扫。 */
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "sdk/abi.h"

// 服务端构建排进服务器线程，客户端构建排进客户端线程。两个执行器接口相同
//（execute / executeAfter，继承自 ll::coro::Executor）。
#ifdef PIER_BUILD_CLIENT
#include "ll/api/thread/ClientThreadExecutor.h"
#define PIER_THREAD_EXEC ll::thread::ClientThreadExecutor
#else
#include "ll/api/thread/ServerThreadExecutor.h"
#define PIER_THREAD_EXEC ll::thread::ServerThreadExecutor
#endif

#include "pier/host/hosted_mod.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/log.h"

namespace pier::api_impl
{
    /* ───────────────────── 按模组记账的任务表 ─────────────────────
     * 为什么存在：一个计划任务就是一根指进模组 dylib 的裸函数指针。模组在
     * 任务触发前卸载，触发就跳进已释放的内存。历史的 `schedule` /
     * `schedule_after` 槽治不了这个 —— 它们根本不知道任务是谁排的 ——
     * 所以下面带模组的槽先把每个任务登记进宿主自己的表。
     *
     * 纪律与表单回调完全一致：执行器闭包只捕获 weak_ptr<HostedMod> 和一个
     * 整数票据，**永远不捕获回调本身**。触发时从表里取票；票没了（卸载时
     * 清掉、或已取消）、模组没了、模组被禁用 —— 任何一种情况都不碰 dylib。
     *
     * 刻意**不**持有 executeAfter 返回的 CancellableCallback：在它自己的
     * 调用里丢掉最后一个引用会析构正在运行的 std::function。让定时器对着
     * 一张死票过期，代价是一次空唤醒，没有任何这类险。 */
    namespace
    {
        struct PendingTask
        {
            HostedMod* mod = nullptr; // 只作身份比对；永不盲目解引用
            PierTaskCb cb = nullptr;
            void* user = nullptr;
        };

        std::mutex gTaskMutex;
        std::unordered_map<uint64_t, PendingTask> gPendingTasks;
        uint64_t gNextTaskId = 1;

        uint64_t registerTask(HostedMod* mod, PierTaskCb cb, void* user)
        {
            std::lock_guard lock(gTaskMutex);
            uint64_t id = gNextTaskId++;
            gPendingTasks[id] = PendingTask{mod, cb, user};
            return id;
        }

        /** 取票并恰好触发一次，或者静默丢弃。跑在服务器/客户端线程上。
         *  锁在调进模组代码之前释放：模组完全可能在任务里重入 schedule_*。 */
        void runTask(std::weak_ptr<HostedMod> const& weakMod, uint64_t id)
        {
            PendingTask task;
            {
                std::lock_guard lock(gTaskMutex);
                auto it = gPendingTasks.find(id);
                if (it == gPendingTasks.end()) return; // 卸载清掉了，或已取消
                task = it->second;
                gPendingTasks.erase(it);
            }
            auto mod = weakMod.lock();
            if (!mod || mod.get() != task.mod) return; // 模组没了；dylib 可能已 unmap
            if (!mod->isEnabled()) return;             // 禁用期间静音
            if (task.cb) task.cb(task.user);
        }

        /** 两个入口的共享主体。 */
        uint64_t submit(PierModHandle modHandle, PierTaskCb cb, void* user, bool delayed, uint64_t delayMs)
        {
            if (!cb || !modHandle) return 0;
            auto* raw = asMod(modHandle);
            if (!raw) return 0;

            std::weak_ptr<HostedMod> weakMod;
            try
            {
                weakMod = raw->shared_from_this();
            }
            catch (...)
            {
                return 0; // 还没被 shared_ptr 接管 —— 拒绝而不是赌
            }

            uint64_t id = registerTask(raw, cb, user);
            auto fire = [weakMod, id] { runTask(weakMod, id); };

            if (delayed)
            {
                // Executor::Duration = steady_clock::duration；毫秒隐式转换。
                (void)PIER_THREAD_EXEC::getDefault().executeAfter(fire, std::chrono::milliseconds(delayMs));
            }
            else
            {
                PIER_THREAD_EXEC::getDefault().execute(fire);
            }
            return id;
        }

        void api_schedule(PierTaskCb cb, void* user)
        {
            PIER_API_GUARD_BEGIN
                // 历史的无主槽：为 schedule_for 出现之前编译的模组保留。
                // 跨卸载天然不安全 —— 这样的模组不许标 reload_safe。
                if (!cb) return;
                PIER_THREAD_EXEC::getDefault().execute([cb, user] { cb(user); });
            PIER_API_GUARD_END_VOID
        }

        void api_schedule_after(PierTaskCb cb, void* user, uint64_t delayMs)
        {
            PIER_API_GUARD_BEGIN
                if (!cb) return;
                // 发后不管：返回的 CancellableCallback 刻意丢弃。
                (void)PIER_THREAD_EXEC::getDefault().executeAfter(
                    [cb, user] { cb(user); },
                    std::chrono::milliseconds(delayMs)
                );
            PIER_API_GUARD_END_VOID
        }

        uint64_t api_schedule_for(PierModHandle mod, PierTaskCb cb, void* user)
        {
            PIER_API_GUARD_BEGIN
                return submit(mod, cb, user, false, 0);
            PIER_API_GUARD_END
        }

        uint64_t api_schedule_after_for(PierModHandle mod, PierTaskCb cb, void* user, uint64_t delayMs)
        {
            PIER_API_GUARD_BEGIN
                return submit(mod, cb, user, true, delayMs);
            PIER_API_GUARD_END
        }

        bool api_schedule_cancel(PierModHandle mod, uint64_t taskId)
        {
            PIER_API_GUARD_BEGIN
                if (!mod || taskId == 0) return false;
                auto* raw = asMod(mod);
                std::lock_guard lock(gTaskMutex);
                auto it = gPendingTasks.find(taskId);
                // 只限本人：一个模组不许取消别人的任务。
                if (it == gPendingTasks.end() || it->second.mod != raw) return false;
                gPendingTasks.erase(it);
                return true;
            PIER_API_GUARD_END
        }

        uint32_t api_schedule_pending_count(PierModHandle mod)
        {
            PIER_API_GUARD_BEGIN
                if (!mod) return 0;
                auto* raw = asMod(mod);
                std::lock_guard lock(gTaskMutex);
                uint32_t n = 0;
                for (auto const& [id, task] : gPendingTasks)
                {
                    if (task.mod == raw) ++n;
                }
                return n;
            PIER_API_GUARD_END
        }

        /** 拆除（stage 10，最先跑）：表里还剩的任务都会调进马上要 unmap 的
         *  dylib，全部丢掉。`user` 载荷按设计泄漏 —— 唯一能释放它的代码就在
         *  即将消失的那个 dylib 里。 */
        void teardown(HostedMod* mod)
        {
            size_t dropped = 0;
            {
                std::lock_guard lock(gTaskMutex);
                for (auto it = gPendingTasks.begin(); it != gPendingTasks.end();)
                {
                    if (it->second.mod == mod)
                    {
                        it = gPendingTasks.erase(it);
                        ++dropped;
                    }
                    else
                    {
                        ++it;
                    }
                }
            }
            if (dropped > 0)
            {
                hostLogger().warn(
                    "[scheduler] '{}' 卸载时仍有 {} 个待执行任务被丢弃 —— "
                    "该模组应在 on_disable/on_unload 里自己取消定时器（schedule_cancel）",
                    mod ? mod->getName() : std::string{"?"},
                    dropped
                );
            }
        }

        void fill(PierApi& api)
        {
            api.schedule = &api_schedule;
            api.schedule_after = &api_schedule_after;
            api.schedule_for = &api_schedule_for;
            api.schedule_after_for = &api_schedule_after_for;
            api.schedule_cancel = &api_schedule_cancel;
            api.schedule_pending_count = &api_schedule_pending_count;
        }

        spi::SlotPackReg regSlots{{"scheduler", &fill}};
        spi::TeardownReg regDown{{10, "scheduler", &teardown}};
    } // namespace
} // namespace pier::api_impl
