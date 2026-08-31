/** protect/ArmorStandEvent.cpp —— 玩家和盔甲架换装备。
 *
 * 盔甲架是保护里的一个洞：它不是容器（PlayerOpenContainerEvent 看不见它），
 * 也不是方块（方块保护管不着），而右键一下就能把上面的整套装备换到自己身上。
 * 在展示装备的服务器上，这等于「点一下就搬空展柜」。
 *
 * 取消 = 这次交换不发生（`_trySwapItem` 返回 false 是引擎自己的「换不了」
 * 路径，物品两边都不动）。
 */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/shared_types/legacy/EquipmentSlot.h"
#include "mc/world/actor/ArmorStand.h"
#include "mc/world/actor/Actor.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& armorStandDef(); // 前向

        LL_TYPE_INSTANCE_HOOK(
            ArmorStandSwapItemHook,
            ll::memory::HookPriority::Normal,
            ArmorStand,
            &ArmorStand::_trySwapItem,
            bool,
            ::Player& player,
            ::SharedTypes::Legacy::EquipmentSlot slot)
        {
            auto& def = armorStandDef();
            if (!def.live()) return origin(player, slot);

            auto const& pos = this->getPosition();
            std::string snbt = "{\"eventId\":\"ArmorStandSwapItemEvent\""
                ",\"x\":" + snbtDouble(pos.x)
                + ",\"y\":" + snbtDouble(pos.y)
                + ",\"z\":" + snbtDouble(pos.z)
                + ",\"dim\":" + snbtNum(static_cast<int>(this->getDimensionId()))
                + ",\"slot\":" + snbtNum(static_cast<int>(slot))
                + ",\"standId\":" + snbtNum(static_cast<int64_t>(this->getOrCreateUniqueID().rawID)) + "L"
                + "," + playerRefSnbt(player) + "}";

            if (dispatchHookEventCancellable(def, snbt)) return false;
            return origin(player, slot);
        }

        HookEventDef gDef{
            "ArmorStandSwapItemEvent",
            []
            {
                int const r = ArmorStandSwapItemHook::hook();
                if (r != 0)
                {
                    hostLogger().error(
                        "[ArmorStandSwapItemEvent] ArmorStand::_trySwapItem 的 detour 安装失败"
                        "（code={}）—— 盔甲架上的装备不受保护。", r);
                }
                return r == 0;
            }
        };
        HookEventDef& armorStandDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
