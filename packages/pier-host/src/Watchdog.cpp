/** Watchdog.cpp: per-thread records of the mod being run, and the thread that reads them.
 *
 * Each thread owns one slot and is its only writer, so an entry costs plain stores and no
 * lock. The monitor reads a slot as a seqlock: an odd sequence means a write is under way,
 * and a sequence that moved during the read means the pair it read may be torn.
 */
#include "pier/host/watchdog.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "ll/api/service/GamingStatus.h"
#include "ll/api/utils/ErrorUtils.h"

#include "pier/support/i18n.h"
#include "pier/support/log.h"

namespace pier::watchdog
{
    namespace
    {
        /** Exit status of a process this watchdog ended. EX_SOFTWARE from sysexits, so a
         *  supervisor log tells it apart from a crash and from a clean stop. */
        constexpr UINT kExitCode = 70;

        constexpr std::chrono::milliseconds kTick{100};

        struct Slot
        {
            std::atomic<HostedMod const*> mod{nullptr};
            std::atomic<char const*> site{nullptr};
            std::atomic<std::uint64_t> seq{0};
            DWORD tid = 0;
        };

        std::atomic<bool> gEnabled{false};
        std::atomic<bool> gShutdown{false};
        Settings gSettings;

        // Slots are never freed. A thread that exits leaves an idle slot behind, which the
        // thread count bounds; freeing one would race the monitor reading it.
        std::mutex gSlotsMu;
        std::vector<Slot*>& slots()
        {
            static auto* v = new std::vector<Slot*>();
            return *v;
        }

        std::mutex gNamesMu;
        std::unordered_map<HostedMod const*, std::string>& names()
        {
            static auto* m = new std::unordered_map<HostedMod const*, std::string>();
            return *m;
        }

        std::mutex gRunMu;
        std::condition_variable gRunCv;
        bool gStop = false;
        std::thread gThread;

        Slot* mySlot() noexcept
        {
            thread_local Slot* mine = nullptr;
            if (mine) return mine;
            Slot* fresh = nullptr;
            try
            {
                fresh = new Slot();
                fresh->tid = GetCurrentThreadId();
                std::lock_guard lock(gSlotsMu);
                slots().push_back(fresh);
            }
            catch (...)
            {
                // Out of memory: this thread goes unwatched rather than failing the call.
                delete fresh;
                return nullptr;
            }
            mine = fresh;
            return mine;
        }

        void publish(Slot& s, HostedMod const* mod, char const* site) noexcept
        {
            auto const v = s.seq.load(std::memory_order_relaxed);
            s.seq.store(v + 1, std::memory_order_relaxed);
            std::atomic_thread_fence(std::memory_order_release);
            s.mod.store(mod, std::memory_order_relaxed);
            s.site.store(site, std::memory_order_relaxed);
            s.seq.store(v + 2, std::memory_order_release);
        }

        std::string nameOf(HostedMod const* mod)
        {
            std::lock_guard lock(gNamesMu);
            auto it = names().find(mod);
            return it != names().end() ? it->second : std::string(tr("watchdog.unknown_mod"));
        }

        namespace console
        {
            /** Writes straight to the console handle, without allocating. The CRT stream
             *  and the logger both take a lock, and the stuck thread may hold it. */
            void log(std::string_view line) noexcept
            {
                HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
                if (h == nullptr || h == INVALID_HANDLE_VALUE) return;
                DWORD written = 0;
                for (std::string_view part : {std::string_view{"[pier watchdog] "}, line,
                                              std::string_view{"\r\n"}})
                {
                    WriteFile(h, part.data(), static_cast<DWORD>(part.size()), &written, nullptr);
                }
            }
        } // namespace console

        /** TerminateProcess and not ExitProcess: the latter runs DLL detach under the loader
         *  lock, which a mod stuck inside FreeLibrary already holds. */
        [[noreturn]] void endProcess(std::string const& line)
        {
            console::log(line);
            // The logger writes the same line to the log file. It gets half a second on a
            // thread of its own, since it may be blocked behind the stuck thread too.
            auto done = std::make_shared<std::promise<void>>();
            auto fut = done->get_future();
            std::thread(
                [line, done]
                {
                    try
                    {
                        hostLogger().fatal("[watchdog] {}", line);
                    }
                    catch (...)
                    {
                        console::log("the log file did not take the line above");
                    }
                    done->set_value();
                })
                .detach();
            fut.wait_for(std::chrono::milliseconds(500));
            TerminateProcess(GetCurrentProcess(), kExitCode);
            std::abort();
        }

        struct Seen
        {
            std::uint64_t seq = 0;
            std::uint64_t heldMs = 0;
            bool warned = false;
            bool spared = false;
            HostedMod const* mod = nullptr;
            char const* site = nullptr;
        };

        void reportReturn(Seen const& o)
        {
            hostLogger().warn("[watchdog] {}",
                              trf("watchdog.returned", nameOf(o.mod), o.site ? o.site : "?", o.heldMs));
        }

        void examine(Slot& s, Seen& o, std::uint64_t stepMs)
        {
            auto const s1 = s.seq.load(std::memory_order_acquire);
            auto const* mod = s.mod.load(std::memory_order_relaxed);
            auto const* site = s.site.load(std::memory_order_relaxed);
            std::atomic_thread_fence(std::memory_order_acquire);
            auto const s2 = s.seq.load(std::memory_order_relaxed);
            if ((s1 & 1) != 0 || s1 != s2) return;

            if (s1 != o.seq)
            {
                if (o.warned) reportReturn(o);
                o = Seen{s1, 0, false, false, mod, site};
                return;
            }
            if (mod == nullptr) return;

            o.heldMs += stepMs;
            bool const down =
                gShutdown.load(std::memory_order_relaxed) || ll::getGamingStatus() == ll::GamingStatus::Stopping;
            std::uint64_t const limit = down ? gSettings.shutdownMs : gSettings.hangMs;
            std::string_view const phase = down ? tr("watchdog.phase.shutdown") : tr("watchdog.phase.runtime");

            if (!o.warned && gSettings.warnMs != 0 && o.heldMs >= gSettings.warnMs)
            {
                o.warned = true;
                hostLogger().warn("[watchdog] {}", trf("watchdog.slow", nameOf(mod), s.tid, site, o.heldMs));
            }
            if (limit == 0 || o.heldMs < limit || o.spared) return;
            if (IsDebuggerPresent())
            {
                o.spared = true;
                hostLogger().warn("[watchdog] {}", trf("watchdog.debugger", nameOf(mod), site, phase));
                return;
            }
            endProcess(trf("watchdog.hang", nameOf(mod), s.tid, site, o.heldMs, phase, limit, kExitCode));
        }

        void run()
        {
            std::unordered_map<Slot*, Seen> seen;
            auto last = std::chrono::steady_clock::now();
            std::unique_lock lock(gRunMu);
            while (!gRunCv.wait_for(lock, kTick, [] { return gStop; }))
            {
                auto const now = std::chrono::steady_clock::now();
                auto const raw = std::chrono::duration_cast<std::chrono::milliseconds>(now - last);
                last = now;
                // Capped at two ticks, so a resume from suspension charges the stuck call
                // at most 200 ms instead of the whole time the process was frozen.
                auto const step = static_cast<std::uint64_t>(std::min(raw, 2 * kTick).count());

                std::vector<Slot*> all;
                {
                    std::lock_guard g(gSlotsMu);
                    all = slots();
                }
                lock.unlock();
                try
                {
                    for (auto* s : all) examine(*s, seen[s], step);
                }
                catch (...)
                {
                    // A report that failed to format is lost; the next tick checks again.
                    ll::error_utils::printCurrentException(hostLogger());
                }
                lock.lock();
            }
        }
    } // namespace

    void start(Settings const& settings)
    {
        if (!settings.enabled)
        {
            hostLogger().info("[watchdog] {}", tr("watchdog.off"));
            return;
        }
        std::lock_guard lock(gRunMu);
        if (gThread.joinable()) return;
        gSettings = settings;
        gStop = false;
        gThread = std::thread(&run);
        gEnabled.store(true, std::memory_order_release);
        hostLogger().info("[watchdog] {}",
                          trf("watchdog.on", settings.warnMs, settings.hangMs, settings.shutdownMs));
    }

    void stop()
    {
        gEnabled.store(false, std::memory_order_release);
        {
            std::lock_guard lock(gRunMu);
            gStop = true;
        }
        gRunCv.notify_all();
        if (gThread.joinable() && gThread.get_id() != std::this_thread::get_id()) gThread.join();
    }

    void enterShutdown() { gShutdown.store(true, std::memory_order_relaxed); }

    void track(HostedMod const* mod, std::string name)
    {
        std::lock_guard lock(gNamesMu);
        names()[mod] = std::move(name);
    }

    void untrack(HostedMod const* mod)
    {
        std::lock_guard lock(gNamesMu);
        names().erase(mod);
    }

    Scope::Scope(HostedMod const* mod, char const* site) noexcept
    {
        if (mod == nullptr || !gEnabled.load(std::memory_order_relaxed)) return;
        auto* s = mySlot();
        if (s == nullptr) return;
        mSlot = s;
        mPrevMod = s->mod.load(std::memory_order_relaxed);
        mPrevSite = s->site.load(std::memory_order_relaxed);
        publish(*s, mod, site);
    }

    Scope::~Scope()
    {
        if (mSlot == nullptr) return;
        publish(*static_cast<Slot*>(mSlot), mPrevMod, mPrevSite);
    }
} // namespace pier::watchdog
