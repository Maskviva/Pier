/** player/SleepEvent.cpp —— 玩家要睡床了。
 *
 * 拦得住的东西：别人家的床（跳过夜、设重生点都是「用了别人的地」）、
 * 起床战争这类不该有夜晚跳过的玩法、以及「床是爆炸陷阱」的下界/末地。
 *
 * 取消语义用引擎自己的 `BedSleepingResult::NotPossibleHere` —— 客户端会弹
 * 原版的「你不能在这里睡觉」，不需要模组自己发消息，也不会把玩家卡在
 * 半睡状态。
 */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/actor/player/BedSleepingResult.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& sleepDef(); // 前向

        LL_TYPE_INSTANCE_HOOK(
            PlayerStartSleepHook,
            ll::memory::HookPriority::Normal,
            Player,
            &Player::$startSleepInBed,
            ::BedSleepingResult,
            ::BlockPos const& bedBlockPos)
        {
            auto& def = sleepDef();
            if (!def.live()) return origin(bedBlockPos);

            std::string snbt = "{\"eventId\":\"PlayerSleepEvent\""
                ",\"x\":" + snbtNum(bedBlockPos.x)
                + ",\"y\":" + snbtNum(bedBlockPos.y)
                + ",\"z\":" + snbtNum(bedBlockPos.z)
                + ",\"dim\":" + snbtNum(static_cast<int>(this->getDimensionId()))
                + "," + playerRefSnbt(*this) + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return ::BedSleepingResult::NotPossibleHere;
            }
            return origin(bedBlockPos);
        }

        HookEventDef gDef{
            "PlayerSleepEvent",
            []
            {
                int const r = PlayerStartSleepHook::hook();
                if (r != 0)
                {
                    hostLogger().error(
                        "[PlayerSleepEvent] Player::$startSleepInBed 的 detour 安装失败（code={}）。", r);
                }
                return r == 0;
            }
        };
        HookEventDef& sleepDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
