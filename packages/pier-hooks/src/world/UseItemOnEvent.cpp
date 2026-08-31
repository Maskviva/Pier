/** hooks/world/UseItemOnEvent.cpp —— 合成事件 "PlayerUseItemOnEvent"，可取消。
 *
 * 刷怪蛋、桶、打火石、末影珍珠以及物品驱动的放置方块都经 GameMode::useItemOn
 * 抵达世界，只盯 interact/place 事件的保护会把它们全部放行，且不留日志。
 * 维度规则层刻意也不堵：Spawner::spawnMob 允许非自然、非刷怪笼来源的生成，因为
 * 创造世界里故意放生物是正常玩法。那个判断对世界规则成立，作为权限检查不成立,
 * 权限需要知道这是谁的领地。所以这里只上报，由模组决定。
 *
 * 取消返回两位都清零的 InteractionResult：不放方块、不生成生物、不倒桶，物品也
 * 不消耗。载荷 {eventId, x, y, z, dim, face, item, isFirstEvent, _player:{…}}。
 * x/y/z 取整，因为 LL 的反射把 BlockPos 序列化成 JSON 数组。isFirstEvent 原样
 * 透传：按住右键时这个调用每秒重复很多次，订阅方靠它区分首次与重复。
 */
#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/gamemode/GameMode.h"
#include "mc/world/gamemode/InteractionResult.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/block/Block.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& useItemOnDef(); // 前向

        LL_TYPE_INSTANCE_HOOK(
            PlayerUseItemOnHook,
            ll::memory::HookPriority::Normal,
            GameMode,
            &GameMode::$useItemOn,
            ::InteractionResult,
            ::ItemStack& item,
            ::BlockPos const& at,
            uchar face,
            ::Vec3 const& hit,
            ::Block const* targetBlock,
            bool isFirstEvent)
        {
            auto& def = useItemOnDef();
            if (!def.live())
            {
                return origin(item, at, face, hit, targetBlock, isFirstEvent);
            }

            // mPlayer 是 TypedStorage<8, 8, Player&>，不是包装：TypedStorageType
            // 对引用和标量都有坍缩特化，只有类类型的值才保持包装。这里写 .get()
            // 是编译错误。完整规则表在 tools/typed-storage.py 的文件头，那是唯一
            // 出处，脚本会读引擎头逐个校验调用点。
            Player& p = this->mPlayer;

            std::string itemName = item.getTypeName();

            std::string snbt = "{\"eventId\":\"PlayerUseItemOnEvent\""
                ",\"x\":" + snbtNum(at.x)
                + ",\"y\":" + snbtNum(at.y)
                + ",\"z\":" + snbtNum(at.z)
                + ",\"dim\":" + snbtNum(static_cast<int>(p.getDimensionId()))
                + ",\"face\":" + snbtNum(static_cast<int>(face))
                + ",\"isFirstEvent\":" + (isFirstEvent ? "1" : "0")
                + ",\"item\":\"" + snbtEscape(itemName)
                + "\"," + playerRefSnbt(p) + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                // 两位都清零：这次使用没有发生，客户端也不播挥手动画。
                ::InteractionResult refused{};
                refused.mSuccess = false;
                refused.mSwing = false;
                return refused;
            }
            return origin(item, at, face, hit, targetBlock, isFirstEvent);
        }

        HookEventDef gDef{"PlayerUseItemOnEvent", [] { return PlayerUseItemOnHook::hook() == 0; }};
        HookEventDef& useItemOnDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
