/** hooks/engine/TickControl.cpp: carpet-style control of the world clock.
 *
 * tick_freeze, tick_step and tick_warp, backed by one detour on Level::$tick. The
 * lifetime rules of hook_events.h apply: installed lazily on the first control call and
 * never unpatched, since a control call arrives from a command handler executing inside
 * the tick, and the idle cost is one predictable branch. */
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
        /** Server thread only, where both the control calls and the hook run. */
        struct TickState
        {
            bool hooked = false;
            bool frozen = false;
            double warp = 1.0; // Ticks per real frame; a fraction means slow motion
            double acc = 0.0;  // Fractional accumulator for warp
            uint32_t pendingSteps = 0;
        };

        TickState gTick;

        LL_TYPE_INSTANCE_HOOK(
            LevelTickHook,
            ll::memory::HookPriority::Normal,
            Level,
            &Level::$tick,
            void)
        {
            auto& st = gTick;
            if (st.frozen)
            {
                // Frozen: only explicitly queued step frames run.
                uint32_t n = st.pendingSteps;
                st.pendingSteps = 0;
                for (uint32_t i = 0; i < n; ++i) origin();
                return;
            }
            if (st.warp == 1.0)
            {
                origin(); // Fast path: the hook is installed but idle
                return;
            }
            // warp accumulates fractional ticks. Above 1 runs extra frames, which speeds
            // the world up, and below 1 skips frames, which is slow motion.
            st.acc += st.warp;
            int n = static_cast<int>(st.acc);
            st.acc -= n;
            for (int i = 0; i < n; ++i) origin();
        }

        void ensureTickHooked()
        {
            if (!gTick.hooked)
            {
                LevelTickHook::hook();
                gTick.hooked = true;
            }
        }

        bool api_tick_freeze(bool on)
        {
            PIER_API_GUARD_BEGIN
                if (!on && !gTick.hooked) return true; // Nothing to undo
                ensureTickHooked();
                gTick.frozen = on;
                if (!on) gTick.pendingSteps = 0;
                return true;
            PIER_API_GUARD_END
        }

        bool api_tick_step(uint32_t n)
        {
            PIER_API_GUARD_BEGIN
                if (n == 0) return false;
                // Stepping only means something while frozen.
                if (!gTick.hooked || !gTick.frozen) return false;
                // While frozen every pending step tick runs within one frame, so without
                // a cap one call freezes the thread.
                if (n > 1200 || gTick.pendingSteps > 1200 - n) return false;
                gTick.pendingSteps += n;
                return true;
            PIER_API_GUARD_END
        }

        bool api_tick_warp(double factor)
        {
            PIER_API_GUARD_BEGIN
                if (!(factor > 0.0) || factor > 100.0) return false; // Also rejects NaN
                if (factor == 1.0 && !gTick.hooked) return true;     // Nothing to undo
                ensureTickHooked();
                gTick.warp = factor;
                if (factor == 1.0) gTick.acc = 0.0;
                return true;
            PIER_API_GUARD_END
        }

        void fill(PierApi& api)
        {
            api.tick_freeze = &api_tick_freeze;
            api.tick_step = &api_tick_step;
            api.tick_warp = &api_tick_warp;
        }

        spi::SlotPackReg reg{{"tick-control", &fill}};
    } // namespace
} // namespace pier::hooks
