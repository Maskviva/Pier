/** core/Scheduler.cpp: scheduled tasks. The ownerless legacy slots, the slots
 *  accounted per mod, and the sweep on unload. */
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "sdk/abi.h"

// The server build schedules onto the server thread and the client build onto the
// client thread. Both executors share the same interface, execute and executeAfter,
// inherited from ll::coro::Executor.
#ifdef PIER_BUILD_CLIENT
#include "ll/api/thread/ClientThreadExecutor.h"
#define PIER_THREAD_EXEC ll::thread::ClientThreadExecutor
#else
#include "ll/api/thread/ServerThreadExecutor.h"
#define PIER_THREAD_EXEC ll::thread::ServerThreadExecutor
#endif


#include "pier/host/hosted_mod.h"
#include "pier/host/mod_host.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/log.h"
#include "pier/support/module.h"

namespace pier::api_impl
{
    /*  Task table accounted per mod
     * A scheduled task is a raw function pointer into a mod dylib. If the mod is
     * unloaded before it fires, firing jumps into freed memory. The legacy `schedule`
     * and `schedule_after` slots never learn who scheduled a task, so the mod-scoped
     * slots below register every task in the host's own table first.
     *
     * The discipline matches form callbacks. The executor closure captures a
     * weak_ptr<HostedMod> and an integer ticket, never the callback itself. A missing
     * ticket, a missing mod or a disabled mod each mean the dylib is not touched.
     *
     * The CancellableCallback returned by executeAfter is deliberately not held.
     * Dropping its last reference from inside its own invocation destroys the running
     * std::function. Letting the timer expire against a dead ticket costs one empty
     * wakeup instead. */
    namespace
    {
        struct PendingTask
        {
            HostedMod* mod = nullptr; // Identity comparison only, never dereferenced
            PierTaskCb cb = nullptr;
            void* user = nullptr;
            /** Legacy-slot tasks only. Base address of the module holding the
             *  callback. */
            void const* legacyBase = nullptr;
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

        /** Takes the ticket and fires exactly once, or drops it silently. Runs on the
         *  server or client thread. The lock is released before calling into mod code,
         *  because a mod may well re-enter schedule_* from inside a task. */
        void runTask(std::weak_ptr<HostedMod> const& weakMod, uint64_t id)
        {
            PendingTask task;
            {
                std::lock_guard lock(gTaskMutex);
                auto it = gPendingTasks.find(id);
                if (it == gPendingTasks.end()) return; // Cleared on unload, or cancelled
                task = it->second;
                gPendingTasks.erase(it);
            }
            auto mod = weakMod.lock();
            if (!mod || mod.get() != task.mod) return; // Mod gone, dylib may be unmapped
            if (!mod->acceptsCallbacks()) return;             // Muted while disabled
            CallbackScope scope{mod.get()};            // Veto unload during the callback
            if (task.cb) task.cb(task.user);
        }

    /*  Recovering ownership for the ownerless legacy slots
     * `schedule` and `schedule_after` carry no mod handle. Left unaccounted, their
     * timers still fire after the mod is unloaded and jump into an unmapped code
     * section. They therefore also go through gPendingTasks with mod set to nullptr,
     * recording the base address of the module holding the callback. Teardown clears
     * by base address, and firing checks once more that the base still belongs to a
     * live mod. */
        void const* moduleBaseOfCb(PierTaskCb cb)
        {
            auto* host = ModHost::instance();
            if (!host) return nullptr;
            // One loader-lock acquisition for the owning module, then a comparison
            // against the table. A loop of addressOwnedBy over hostedMods() takes that
            // lock once per mod, on every legacy schedule and again when the task fires.
            void const* owner = moduleContaining(reinterpret_cast<void const*>(cb));
            if (!owner) return nullptr;
            for (auto const& hosted : host->hostedMods())
            {
                if (hosted->lib.handle() == owner) return owner;
            }
            return nullptr;
        }

        void runLegacyTask(uint64_t id)
        {
            PendingTask task;
            {
                std::lock_guard lock(gTaskMutex);
                auto it = gPendingTasks.find(id);
                if (it == gPendingTasks.end()) return; // Cleared on unload
                task = it->second;
                gPendingTasks.erase(it);
            }
            if (!task.cb) return;
            // The base address was resolved at registration. Before firing it is
            // confirmed to still belong to a live mod. A callback belonging to no mod,
            // meaning the host itself or an unknown source, still runs.
            if (task.legacyBase && moduleBaseOfCb(task.cb) != task.legacyBase) return;
            task.cb(task.user);
        }

        void submitLegacy(PierTaskCb cb, void* user, bool delayed, uint64_t delayMs)
        {
            void const* base = moduleBaseOfCb(cb);
            uint64_t id;
            {
                std::lock_guard lock(gTaskMutex);
                id = gNextTaskId++;
                gPendingTasks[id] = PendingTask{nullptr, cb, user, base};
            }
            auto fire = [id] { runLegacyTask(id); };
            if (delayed)
            {
                (void)PIER_THREAD_EXEC::getDefault().executeAfter(fire, std::chrono::milliseconds(delayMs));
            }
            else
            {
                PIER_THREAD_EXEC::getDefault().execute(fire);
            }
        }

        /** Shared body of the two entry points. */
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
                return 0; // Not yet owned by a shared_ptr, so refuse rather than gamble
            }

            uint64_t id = registerTask(raw, cb, user);
            auto fire = [weakMod, id] { runTask(weakMod, id); };

            if (delayed)
            {
                // Executor::Duration is steady_clock::duration and milliseconds
                // convert implicitly.
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
                // The ownerless legacy slots, kept for mods compiled before
                // schedule_for existed. They are inherently unsafe across an unload, so
                // such a mod must not declare reload_safe.
                if (!cb) return;
                submitLegacy(cb, user, /*delayed=*/false, 0);
            PIER_API_GUARD_END_VOID
        }

        void api_schedule_after(PierTaskCb cb, void* user, uint64_t delayMs)
        {
            PIER_API_GUARD_BEGIN
                if (!cb) return;
                submitLegacy(cb, user, /*delayed=*/true, delayMs);
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
                // Scoped to the caller. A mod may not cancel another mod's task.
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

        /** Teardown at stage 10, which runs first. Every task left in the table would
         *  call into a dylib about to be unmapped, so all of them are dropped. The
         *  `user` payload leaks by design, because the only code that could free it
         *  lives in the dylib that is about to disappear. */
        void teardown(HostedMod* mod)
        {
            size_t dropped = 0;
            {
                std::lock_guard lock(gTaskMutex);
                void const* base = mod ? mod->lib.handle() : nullptr;
                for (auto it = gPendingTasks.begin(); it != gPendingTasks.end();)
                {
                    bool const mine = it->second.mod == mod
                        || (it->second.mod == nullptr && base
                            && (it->second.legacyBase == base
                                || addressOwnedBy(base, reinterpret_cast<void const*>(it->second.cb))));
                    if (mine)
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
                    "[scheduler] '{}' was unloaded with {} pending task(s), all dropped; "
                    "a mod is expected to cancel its own timers through schedule_cancel "
                    "in on_disable or on_unload",
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
