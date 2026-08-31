/** hooks/world/DimensionEvents.cpp —— 合成事件 "PlayerChangeDimensionEvent"。
 *
 * 没有这个事件，模组只能轮询维度并比对，那会漏掉每一次不是自己发起的转移：传送
 * 门、别的模组的传送、/execute in。以「玩家换世界了吗」为条件的功能（分世界背
 * 包、分世界游戏模式）在那些情况下静默失效。
 *
 * Level::requestPlayerChangeDimension 是每次转移的唯一漏斗，ChangeDimensionRequest
 * 同时带源维度和目标维度，一个钩子覆盖全部。事件在 origin 之前派发，此时玩家还
 * 在旧维度里，「保存正要离开的那个世界的背包」才成立；origin 之后背包可能已被引
 * 擎换过。只观察，不取消转移。
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

            // 转发之前把请求读出来：origin() 按右值引用接手，可以随意掏空它。
            int const from = changeRequest.mFromDimensionId->value();
            int const to = changeRequest.mToDimensionId->value();

            std::string snbt = "{\"eventId\":\"PlayerChangeDimensionEvent\""
                ",\"from\":" + snbtNum(from)
                + ",\"to\":" + snbtNum(to)
                + "," + playerRefSnbt(player) + "}";
            dispatchHookEvent(def, snbt); // 在 origin 之前，见文件头

            return origin(player, std::move(changeRequest));
        }

        HookEventDef gDef{"PlayerChangeDimensionEvent", [] { return PlayerChangeDimensionHook::hook() == 0; }};
        HookEventDef& changeDimDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
