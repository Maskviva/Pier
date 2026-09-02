/** hooks/engine/Profiler.cpp: MSPT sampling broken down by subsystem.
 *
 * profile_begin(ticks) arms a sampling window of N level ticks and profile_take() polls
 * for the finished report as SNBT. Five timing detours cover level_tick, dimension_tick,
 * redstone, chunk_blocks and block_entities, and all follow the lifetime rules of
 * hook_events.h: installed together on the first profile_begin, never unpatched, and
 * taking a fast-path branch while unarmed.
 *
 * The times are inclusive wall-clock times from steady_clock: dimension_tick runs inside
 * level_tick, and the redstone and chunk buckets run inside dimension_tick. They are
 * reported side by side and are not meant to sum. It coexists with the TickControl detour
 * on the same Level::$tick, since LeviLamina chains hooks, and each tick that really runs
 * is measured once, so /tick warp 5 shows five times the samples while the per-tick
 * numbers stay true.
 */
#include <chrono>
#include <cstdint>
#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/level/Level.h"
#include "mc/world/level/chunk/LevelChunk.h"
#include "mc/world/level/dimension/Dimension.h"

#include "sdk/abi.h"

#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/str.h"

namespace pier::hooks
{
    namespace
    {
        using Clock = std::chrono::steady_clock;

        struct Bucket
        {
            uint64_t ns = 0;
            uint64_t calls = 0;

            void add(Clock::duration d)
            {
                ns += static_cast<uint64_t>(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(d).count());
                ++calls;
            }

            void reset() { ns = 0, calls = 0; }
        };

        /** Server thread only, as with every hook state. */
        struct ProfState
        {
            bool hooked = false;
            bool sampling = false;
            bool reportReady = false;
            uint32_t remaining = 0;
            uint32_t window = 0;
            Bucket levelTick, dimTick, redstone, chunkBlocks, blockEntities;
            std::string report;
        };

        ProfState gProf;

        std::string bucketSnbt(char const* name, Bucket const& b)
        {
            return std::string{"\""} + name + "\":{\"us\":" + std::to_string(b.ns / 1000)
                + ",\"calls\":" + std::to_string(b.calls) + "}";
        }

        void finishWindow()
        {
            auto& st = gProf;
            st.sampling = false;
            st.report = "{\"ticks\":" + std::to_string(st.window)
                + ",\"buckets\":{" + bucketSnbt("level_tick", st.levelTick)
                + "," + bucketSnbt("dimension_tick", st.dimTick)
                + "," + bucketSnbt("redstone", st.redstone)
                + "," + bucketSnbt("chunk_blocks", st.chunkBlocks)
                + "," + bucketSnbt("block_entities", st.blockEntities) + "}}";
            st.reportReady = true;
        }

        LL_TYPE_INSTANCE_HOOK(
            ProfLevelTickHook,
            // Outermost, wrapping the Normal-priority TickControl detour.
            ll::memory::HookPriority::High,
            Level,
            &Level::$tick,
            void)
        {
            auto& st = gProf;
            if (!st.sampling)
            {
                origin();
                return;
            }
            auto t0 = Clock::now();
            origin();
            st.levelTick.add(Clock::now() - t0);
            if (st.remaining > 0 && --st.remaining == 0) finishWindow();
        }

        LL_TYPE_INSTANCE_HOOK(
            ProfDimensionTickHook,
            ll::memory::HookPriority::Normal,
            Dimension,
            &Dimension::$tick,
            void)
        {
            auto& st = gProf;
            if (!st.sampling)
            {
                origin();
                return;
            }
            auto t0 = Clock::now();
            origin();
            st.dimTick.add(Clock::now() - t0);
        }

        LL_TYPE_INSTANCE_HOOK(
            ProfRedstoneTickHook,
            ll::memory::HookPriority::Normal,
            Dimension,
            &Dimension::$tickRedstone,
            void)
        {
            auto& st = gProf;
            if (!st.sampling)
            {
                origin();
                return;
            }
            auto t0 = Clock::now();
            origin();
            st.redstone.add(Clock::now() - t0);
        }

        LL_TYPE_INSTANCE_HOOK(
            ProfChunkBlocksHook,
            ll::memory::HookPriority::Normal,
            LevelChunk,
            &LevelChunk::tickBlocks,
            void,
            ::BlockSource& region)
        {
            auto& st = gProf;
            if (!st.sampling)
            {
                origin(region);
                return;
            }
            auto t0 = Clock::now();
            origin(region);
            st.chunkBlocks.add(Clock::now() - t0);
        }

        LL_TYPE_INSTANCE_HOOK(
            ProfBlockEntitiesHook,
            ll::memory::HookPriority::Normal,
            LevelChunk,
            &LevelChunk::tickBlockEntities,
            void,
            ::BlockSource& region)
        {
            auto& st = gProf;
            if (!st.sampling)
            {
                origin(region);
                return;
            }
            auto t0 = Clock::now();
            origin(region);
            st.blockEntities.add(Clock::now() - t0);
        }

        void ensureProfilerHooked()
        {
            if (gProf.hooked) return;
            ProfLevelTickHook::hook();
            ProfDimensionTickHook::hook();
            ProfRedstoneTickHook::hook();
            ProfChunkBlocksHook::hook();
            ProfBlockEntitiesHook::hook();
            gProf.hooked = true;
        }

        bool api_profile_begin(uint32_t ticks)
        {
            PIER_API_GUARD_BEGIN
                auto& st = gProf;
                if (ticks == 0 || ticks > 12000) return false; // Cap: ten minutes at 20 TPS
                if (st.sampling) return false;                 // One window at a time
                ensureProfilerHooked();
                st.levelTick.reset();
                st.dimTick.reset();
                st.redstone.reset();
                st.chunkBlocks.reset();
                st.blockEntities.reset();
                st.reportReady = false;
                st.report.clear();
                st.window = ticks;
                st.remaining = ticks;
                st.sampling = true;
                return true;
            PIER_API_GUARD_END
        }

        bool api_profile_take(void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                auto& st = gProf;
                if (!st.reportReady || !sink) return false;
                sink(ctx, ps(st.report));
                st.reportReady = false;
                st.report.clear();
                return true;
            PIER_API_GUARD_END
        }

        void fill(PierApi& api)
        {
            api.profile_begin = &api_profile_begin;
            api.profile_take = &api_profile_take;
        }

        spi::SlotPackReg reg{{"profiler", &fill}};
    } // namespace
} // namespace pier::hooks
