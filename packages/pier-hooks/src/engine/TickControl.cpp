/** hooks/engine/TickControl.cpp —— carpet 风格的世界时钟控制
 *（tick_freeze / tick_step / tick_warp），背后是 Level::$tick 上**一个**
 * detour。hook_events.h 的生命周期规矩适用：第一次控制调用时懒安装、永不
 * 卸补丁（控制调用来自**tick 内部**执行的命令处理器）、空闲开销 = 一个可
 * 预测的分支。 */
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
        /** 仅服务器线程（控制调用和钩子都跑在那里）。 */
        struct TickState
        {
            bool hooked = false;
            bool frozen = false;
            double warp = 1.0; // 每真实帧的 tick 数；小数 = 慢动作
            double acc = 0.0;  // warp 的小数累加器
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
                // 冻结：只跑显式排队的步进帧。
                uint32_t n = st.pendingSteps;
                st.pendingSteps = 0;
                for (uint32_t i = 0; i < n; ++i) origin();
                return;
            }
            if (st.warp == 1.0)
            {
                origin(); // 快路径：钩子装着但空闲
                return;
            }
            // warp：累加小数 tick；>1 连跑额外帧（加速），<1 跳帧（慢动作）。
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
                if (!on && !gTick.hooked) return true; // 没什么可撤销的
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
                if (!gTick.hooked || !gTick.frozen) return false; // 步进只在冻结时有意义
                // V-38：冻结态下一帧内执行全部待步进 tick；无上限等于一次调用冻住线程。
                if (n > 1200 || gTick.pendingSteps > 1200 - n) return false;
                gTick.pendingSteps += n;
                return true;
            PIER_API_GUARD_END
        }

        bool api_tick_warp(double factor)
        {
            PIER_API_GUARD_BEGIN
                if (!(factor > 0.0) || factor > 100.0) return false; // 顺带拒掉 NaN
                if (factor == 1.0 && !gTick.hooked) return true;     // 没什么可撤销的
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
