/** hooks/engine/Profiler.cpp —— 分子系统的 MSPT 采样。
 *
 * profile_begin(ticks) 武装一个 N 个 level tick 的采样窗口，profile_take() 轮询
 * 取完成的报告（SNBT）。五个计时 detour 覆盖 level_tick、dimension_tick、
 * redstone、chunk_blocks、block_entities，全部遵守 hook_events.h 的生命周期规矩：
 * 第一次 profile_begin 时一起安装、永不卸补丁、未武装时走快路径分支。
 *
 * 时间是包含式墙钟时间（steady_clock）：dimension_tick 跑在 level_tick 里，
 * redstone 与 chunk 两桶跑在 dimension_tick 里。并排报告，不要求和。与 TickControl
 * 在同一个 Level::$tick 上的 detour 共存，LeviLamina 把钩子串成链，每个真正执行
 * 的 tick 只被量一次，所以 /tick warp 5 显示 5 倍样本数而每 tick 数字依旧真实。
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

        /** 仅服务器线程，与所有钩子状态一样。 */
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
            ll::memory::HookPriority::High, // 最外层：包住 TickControl 的 Normal 优先级 detour
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
                if (ticks == 0 || ticks > 12000) return false; // 上限：20 TPS 下十分钟
                if (st.sampling) return false;                 // 一次一个窗口
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
