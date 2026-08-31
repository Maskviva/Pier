/** hooks/world/DestroyEvents.cpp —— 合成事件 "PlayerStartDestroyBlockEvent"。
 *
 * 钩 GameMode::startDestroyBlock，早于 LeviLamina 内建的 PlayerDestroyBlockEvent
 * （那个在破坏完成时才触发）。事件在 origin 之前派发、回调同步执行，所以换快捷
 * 栏槽位的订阅者能赶在破坏逻辑读取手持工具之前换完，这正是自动切工具需要的时
 * 机。生命周期规矩见 hook_events.h。 */
#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/actor/player/Player.h"
#include "mc/world/gamemode/GameMode.h"
#include "mc/world/level/BlockPos.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& destroyDef(); // 前向

        LL_TYPE_INSTANCE_HOOK(
            StartDestroyBlockHook,
            ll::memory::HookPriority::Normal,
            GameMode,
            &GameMode::$startDestroyBlock,
            bool,
            ::BlockPos const& pos,
            uchar face,
            bool& hasDestroyedBlock)
        {
            auto& def = destroyDef();
            if (!def.live())
            {
                return origin(pos, face, hasDestroyedBlock); // 装着但空闲
            }

            // GameMode::mPlayer 是包着 Player& 的 TypedStorage，引用走坍缩特化，
            // 成员本身就是那个引用，不用 .get()。
            Player& p = this->mPlayer;

            std::string snbt = "{\"eventId\":\"PlayerStartDestroyBlockEvent\""
                ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"dim\":" + snbtNum(static_cast<int>(p.getDimensionId()))
                + ",\"face\":" + snbtNum(static_cast<int>(face))
                + "," + playerRefSnbt(p) + "}";
            dispatchHookEvent(def, snbt); // 在 origin 之前，见文件头

            return origin(pos, face, hasDestroyedBlock);
        }

        HookEventDef gDef{"PlayerStartDestroyBlockEvent", [] { return StartDestroyBlockHook::hook() == 0; }};
        HookEventDef& destroyDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
