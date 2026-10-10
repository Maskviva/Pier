/** watchdog.h: names the pier mod a thread is stuck in, and ends a process it has hung.
 *
 * A mod is native code on the caller's thread, so nothing can stop it from outside short of
 * ending the process. Unwinding a thread out of foreign code leaves every lock it holds
 * taken and the heap in whatever state it was mid-way through. The watchdog therefore does
 * two things only: it logs which mod, through which entry, has held a thread for how long,
 * and past the limit it terminates the process so a supervisor can restart the server.
 *
 * Time is counted from the monitor's own ticks, capped per tick, and not from timestamps.
 * A process that was suspended, or a machine that slept, resumes with no time charged.
 */
#pragma once

#include <cstdint>
#include <string>

namespace pier
{
    class HostedMod;
} // namespace pier

namespace pier::watchdog
{
    /** config.json `watchdog.*`. A limit of 0 turns that one action off. */
    struct Settings
    {
        bool enabled = true;
        std::uint32_t warnMs = 2000;
        std::uint32_t hangMs = 30000;
        std::uint32_t shutdownMs = 10000;
    };

    /** Starts the monitor thread. A second call, or one with enabled false, starts nothing
     *  and leaves every Scope a no-op. */
    void start(Settings const& settings);

    /** Stops and joins the monitor. Called from the host's own unload and never from a
     *  static destructor, where the join would wait under the loader lock. */
    void stop();

    /** Switches every later check to the shutdown limit. The engine's Stopping status
     *  switches it as well; this covers the part of shutdown after that flag is read. */
    void enterShutdown();

    /** Records the name a report uses for `mod`. A report about a mod untracked since then
     *  says so instead of reading a destroyed object. */
    void track(HostedMod const* mod, std::string name);
    void untrack(HostedMod const* mod);

    /**
     * Marks the current thread as inside `mod` through the entry `site` until destruction.
     *
     * Nests: an inner scope is reported while it lasts and the outer one is restored after,
     * so a provider stuck under another mod's service call is the one named. `site` must be
     * a string literal, because the monitor reads it from another thread.
     */
    class Scope
    {
    public:
        Scope(HostedMod const* mod, char const* site) noexcept;
        ~Scope();
        Scope(Scope const&) = delete;
        Scope& operator=(Scope const&) = delete;

    private:
        void* mSlot = nullptr;
        HostedMod const* mPrevMod = nullptr;
        char const* mPrevSite = nullptr;
    };
} // namespace pier::watchdog
