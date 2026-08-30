/** hooks/world/DimensionEvents.cpp —— "PlayerChangeDimensionEvent"：玩家即
 * 将被移往另一个维度时触发。
 *
 * # 为什么需要它
 *
 * 没有换维度事件，模组侧只能轮询：进服时记住玩家的维度、每次传送后重读、
 * 再比对。那会漏掉每一次不是模组自己发起的转移 —— 传送门、别的模组的传
 * 送、`/execute in`。任何以「玩家换世界了吗」为条件的功能（分世界背包、分
 * 世界游戏模式）在那些情况下都会静默失效。
 *
 * `Level::requestPlayerChangeDimension` 是每一次转移都要过的**唯一漏斗**，
 * 而 `ChangeDimensionRequest` 同时带着源维度和目标维度，所以一个钩子覆盖
 * 全部。这与 LegacyScriptEngine 的 `onChangeDim` 用的是同一个入口。
 *
 * # 在 origin **之前**派发
 *
 * 订阅者运行时玩家还在旧维度里。正是这个次序让「保存我正要离开的那个世界
 * 的背包」成为可能 —— origin 之后玩家已经在别处，背包可能已被引擎换过了。
 *
 * 合成事件是只观察的（见 hook_events.h）；这里不会取消转移。
 */
#include "pier/hooks/hook_events.h"

#include <string>
#include <utility>

#include "ll/api/memory/Hook.h"

#include "mc/world/actor/player/Player.h"
#include "mc/world/level/ChangeDimensionRequest.h"
#include "mc/world/level/Level.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& changeDimDef(); // 前向

        LL_TYPE_INSTANCE_HOOK(
            PlayerChangeDimensionHook,
            ll::memory::HookPriority::Normal,
            Level,
            &Level::$requestPlayerChangeDimension,
            void,
            ::Player& player,
            ::ChangeDimensionRequest&& changeRequest)
        {
            auto& def = changeDimDef();
            if (!def.live())
            {
                return origin(player, std::move(changeRequest));
            }

            // 转发**之前**把请求读出来：origin() 按右值引用接手，可以随意
            // 掏空它。
            int const from = changeRequest.mFromDimensionId->value();
            int const to = changeRequest.mToDimensionId->value();

            std::string snbt = "{\"eventId\":\"PlayerChangeDimensionEvent\""
                ",\"from\":" + snbtNum(from)
                + ",\"to\":" + snbtNum(to)
                + "," + playerRefSnbt(player) + "}";
            dispatchHookEvent(def, snbt); // 在 origin 之前 —— 见文件头

            return origin(player, std::move(changeRequest));
        }

        HookEventDef gDef{"PlayerChangeDimensionEvent", [] { PlayerChangeDimensionHook::hook(); }};
        HookEventDef& changeDimDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
