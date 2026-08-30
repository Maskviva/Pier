/** hooks/world/UseItemOnEvent.cpp —— "PlayerUseItemOnEvent"：玩家即将把手
 * 里的物品**用在某个方块上**，**并且可以取消**。
 *
 * # 为什么需要这个钩子
 *
 * `PlayerInteractBlockEvent` 并不是人们以为的那个漏斗。刷怪蛋、桶、打火
 * 石、末影珍珠，以及由物品驱动的放置方块，全都是经 `GameMode::useItemOn`
 * 抵达世界的，而一个只盯 interact/place 事件的领地保护插件会把它们全部放
 * 行。那是一个活着的洞：一个站在别人领地上的访客可以把一整组刷怪蛋倒在那
 * 里，而服务器日志里一个字都没有。
 *
 * 维度规则层也堵不住它，而且是**刻意**堵不住：`Spawner::spawnMob` 一律允
 * 许既非自然、也非刷怪笼来源的生成 —— 因为玩家在创造世界里故意放一只生物
 * 是正常玩法。那个判断对**世界**规则是对的，作为**权限**检查是错的 —— 权
 * 限需要知道这是谁的领地，而那只有另一侧知道。所以这个钩子只上报事件，让
 * 模组去决定。
 *
 * # 取消
 *
 * `useItemOn` 返回 `InteractionResult`，两个位标志。返回一个两位都清零的
 * 结果就拒绝了这次使用、而不碰任何别的东西：不放方块、不生成生物、不倒
 * 桶，物品也不被消耗。
 *
 * # 载荷
 *
 * ```text
 * {eventId, x, y, z, dim, face, item, isFirstEvent, _player:{name,xuid,uuid}}
 * ```
 *
 * `x/y/z` 是**平铺的整数**而不是嵌套结构，和本目录其他合成事件一致。这很
 * 要紧：LeviLamina 的反射把 `BlockPos` 序列化成 JSON **数组**，而只认
 * `{x,y,z}` 的消费方从里面静默读不到东西。
 *
 * `isFirstEvent` 原样透传，因为它在下游确实有用：在 Windows 上按住右键会
 * 让这个调用每秒重复很多次，而想「每次点击只动作一次」的订阅者需要把第一
 * 次和后续重复区分开。
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

            // mPlayer 是 `TypedStorage<8, 8, Player&>` —— 而那**不是**包装。
            // `TypedStorageType` 有一个偏特化
            //
            //     requires(is_reference_v<T> || (is_scalar_v<T> && …))
            //     using Type = T;
            //
            // 所以引用成员和标量一样坍缩成裸引用。这里写 `.get()` 是编译错误。
            //
            // 这条把此前得出的规则又精确了一层：不是「标量坍缩」，而是
            //「标量**和引用**都坍缩；只有类类型的值才保持包装」。
            //
            // 完整规则表（含 unique_ptr 那一格）在 `tools/typed-storage.py`
            // 的文件头 —— 那是**唯一出处**。这条规则一度散在四个文件的注释
            // 里、措辞还各不相同，而规则有四个出处就等于没有出处：谁也不知道
            // 哪一份是最新的。那个脚本会读引擎头逐个校验调用点。
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
                // 两个标志都清零：这次使用没有发生，并且告诉客户端不要播挥手
                // 动画。
                ::InteractionResult refused{};
                refused.mSuccess = false;
                refused.mSwing = false;
                return refused;
            }
            return origin(item, at, face, hit, targetBlock, isFirstEvent);
        }

        HookEventDef gDef{"PlayerUseItemOnEvent", [] { PlayerUseItemOnHook::hook(); }};
        HookEventDef& useItemOnDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
