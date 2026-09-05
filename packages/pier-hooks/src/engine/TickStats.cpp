/** hooks/engine/TickStats.cpp: TPS and MSPT measured the way a server monitor expects.
 *
 * mTickDeltaTime, which get_tick_delta_time exposes, is the wall-clock period of the
 * last frame as measured by a Stopwatch in TickDeltaTimeManagerProxy, sleep included.
 * Its reciprocal is one noisy sample of the frame rate and not a tick rate: it reads
 * above 20 on roughly half the frames, keeps reading 20 while the world is frozen, and
 * disagrees with the tick warp in both directions.
 *
 * Two detours on Level::$tick replace that. The outer one at Highest priority records
 * per frame the wall time since the previous frame and the time inside origin(). The
 * inner one at Lowest priority counts each Level::tick that really runs, the only count
 * that stays right under warp (N ticks per frame) and freeze (none). TPS is real ticks
 * over wall time in a sliding window, MSPT is time inside the tick over real ticks.
 * Both install at bootstrap and stay, since a query needs history. The idle cost is
 * two steady_clock reads and a few stores per frame. Server thread only.
 */
#include <array>
#include <chrono>
#include <cstdint>

#include "ll/api/memory/Hook.h"

#include "mc/world/level/Level.h"

#include "sdk/abi.h"

#include "pier/host/spi.h"
#include "pier/support/guard.h"

namespace pier::hooks
{
    namespace
    {
        using Clock = std::chrono::steady_clock;

        /** One frame: how many ticks really ran, how long they took, and the wall time
         *  since the frame before. */
        struct Frame
        {
            uint32_t ticks = 0;
            uint64_t busyNs = 0;
            uint64_t wallNs = 0;
        };

        /** A minute of frames at 20 Hz. The window a caller asks for is clipped to it. */
        constexpr size_t kCapacity = 1200;
        constexpr int32_t kMaxWindowSeconds = 60;

        struct TickStatsState
        {
            bool hooked = false;
            bool primed = false; // The first frame has no predecessor to measure against
            Clock::time_point lastFrameStart{};
            uint32_t innerTicks = 0; // Counted by the innermost detour during one frame
            std::array<Frame, kCapacity> ring{};
            size_t head = 0;  // Next write position
            size_t count = 0; // Frames recorded, up to kCapacity
        };

        TickStatsState gStats;

        void pushFrame(Frame f)
        {
            auto& st = gStats;
            st.ring[st.head] = f;
            st.head = (st.head + 1) % kCapacity;
            if (st.count < kCapacity) ++st.count;
        }

        /** Sums the newest frames that fit into `windowSeconds` of wall time. Returns
         *  the number of frames included, which is 0 when nothing was sampled yet. */
        size_t sumWindow(int32_t windowSeconds, uint64_t& ticks, uint64_t& busyNs, uint64_t& wallNs)
        {
            ticks = busyNs = wallNs = 0;
            auto& st = gStats;
            if (windowSeconds < 1) windowSeconds = 1;
            if (windowSeconds > kMaxWindowSeconds) windowSeconds = kMaxWindowSeconds;
            uint64_t const limitNs = static_cast<uint64_t>(windowSeconds) * 1'000'000'000ull;
            size_t used = 0;
            for (size_t i = 0; i < st.count; ++i)
            {
                // Newest first. head points past the newest entry.
                size_t const idx = (st.head + kCapacity - 1 - i) % kCapacity;
                Frame const& f = st.ring[idx];
                if (used > 0 && wallNs + f.wallNs > limitNs) break;
                ticks += f.ticks;
                busyNs += f.busyNs;
                wallNs += f.wallNs;
                ++used;
            }
            return used;
        }

        /** Innermost: counts each Level::tick that really executes. Under freeze it never
         *  runs, under warp it runs N times per frame, which is exactly the count wanted. */
        LL_TYPE_INSTANCE_HOOK(
            TickStatsInnerHook,
            ll::memory::HookPriority::Lowest,
            Level,
            &Level::$tick,
            void)
        {
            ++gStats.innerTicks;
            origin();
        }

        /** Outermost: one entry per frame, wrapping the profiler and the tick control. */
        LL_TYPE_INSTANCE_HOOK(
            TickStatsOuterHook,
            ll::memory::HookPriority::Highest,
            Level,
            &Level::$tick,
            void)
        {
            auto& st = gStats;
            auto const t0 = Clock::now();
            uint64_t wall = 0;
            if (st.primed)
            {
                wall = static_cast<uint64_t>(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(t0 - st.lastFrameStart).count());
            }
            st.lastFrameStart = t0;
            st.innerTicks = 0;
            origin();
            auto const t1 = Clock::now();
            uint64_t const busy = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
            if (st.primed) pushFrame(Frame{st.innerTicks, busy, wall});
            st.primed = true;
        }

        void ensureHooked()
        {
            if (gStats.hooked) return;
            TickStatsOuterHook::hook();
            TickStatsInnerHook::hook();
            gStats.hooked = true;
        }

        double api_get_tps(int32_t windowSeconds)
        {
            PIER_API_GUARD_BEGIN
                uint64_t ticks = 0, busy = 0, wall = 0;
                if (sumWindow(windowSeconds, ticks, busy, wall) == 0 || wall == 0) return -1.0;
                (void)busy;
                return static_cast<double>(ticks) * 1e9 / static_cast<double>(wall);
            PIER_API_GUARD_END_VAL(-1.0)
        }

        double api_get_mspt(int32_t windowSeconds)
        {
            PIER_API_GUARD_BEGIN
                uint64_t ticks = 0, busy = 0, wall = 0;
                if (sumWindow(windowSeconds, ticks, busy, wall) == 0 || ticks == 0) return -1.0;
                (void)wall;
                return static_cast<double>(busy) / 1e6 / static_cast<double>(ticks);
            PIER_API_GUARD_END_VAL(-1.0)
        }

        void fill(PierApi& api)
        {
            api.get_tps = &api_get_tps;
            api.get_mspt = &api_get_mspt;
        }

        spi::SlotPackReg reg{{"tick-stats", &fill}};
        // Stage 40: after the dimensions package has read its config and before the money
        // trampoline at 100. The hook only needs the Level symbol, which is resolved at
        // hook() time, so any stage works; a low one means history starts early.
        spi::BootstrapReg boot{{40, "tick-stats", &ensureHooked}};
    } // namespace
} // namespace pier::hooks
