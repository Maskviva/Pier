/**
 * hooks/protect/TakeEntityEvent.cpp —— "PlayerTakeEntityEvent"：玩家正要把
 * 一个**实体**收进物品栏，**可取消**。
 *
 * # 为什么需要它，而不是用 PlayerPickUpItemEvent
 *
 * LeviLamina 的 `PlayerPickUpItemEvent` 挂在 `Player::take` 上，但里面有一
 * 道门（`ll/api/event/player/PlayerPickUpItemEvent.cpp`）：
 *
 * ```cpp
 * if (itemActor.hasCategory(ActorCategory::Item)) {
 *     auto ev = PlayerPickUpItemEvent(...);
 *     EventBus::getInstance().publish(ev);
 *     if (ev.isCancelled()) return false;
 * }
 * return origin(itemActor, orgCount, favoredSlot);   // ← 非 Item 类走这里
 * ```
 *
 * **射出去的箭矢和三叉戟不是 `ActorCategory::Item`。** 它们是投射物实体
 * （`minecraft:arrow` / `minecraft:thrown_trident`），落地后仍然是投射物，
 * 于是那个事件对它们**根本不发布**。
 *
 * 这个失效方式特别难查：订阅是成功的（日志里不会有任何异常），保护对掉落物
 * 完全正常，只有箭矢和三叉戟悄悄穿过去。看起来像「保护偶尔失灵」，其实是这
 * 一类实体从来就没进过那条判定。
 *
 * # 钩点：各投射物自己的 playerTouch，**不是** Player::take
 *
 * 这里踩过一次坑，值得写死在文件头，因为它和上面那段的结论正好差一层：
 *
 * 上一版挂的是 `Player::take` —— 那是 `ItemActor::playerTouch` 内部调的函
 * 数，掉落物走它。但 `Arrow::playerTouch` 和 `ThrownTrident::playerTouch`
 * 是**各自独立的实现**，它们直接把物品塞进背包，根本不经过 `Player::take`。
 * 所以那个钩子对箭矢一次都没触发过 —— 装是装上了，只是挂错了地方，而
 * 「装上了」和「有效」在日志里长得一模一样。
 *
 * `playerTouch` 是 `Actor` 上的虚函数，每个子类各有一份实现，所以按具体类
 * 分别挂：目前是 `Arrow` 和 `ThrownTrident` 两类。将来发现别的漏网投射物，
 * 加一行 `PIER_PICKUP_HOOK(...)` 即可，不用碰这个文件之外的任何东西。
 *
 * # 和 PlayerPickUpItemEvent 的分工
 *
 * 两者钩的是不同函数，覆盖面互补、不重叠：
 *
 *   - 掉落物（`ItemActor`）→ `Player::take` → LeviLamina 的
 *     `PlayerPickUpItemEvent`。本文件**不**触发。
 *   - 箭矢 / 三叉戟 → 各自的 `playerTouch` → 本文件的
 *     `PlayerTakeEntityEvent`。
 *
 * 所以订阅了本事件的模组补上的正是原先漏掉的那一半，而已经订阅
 * `PlayerPickUpItemEvent` 的模组行为一个字都不变。想同时管住两类，两个事件
 * 都订阅。
 *
 * # 载荷
 *
 * ```text
 * {eventId, x, y, z, dim, entity, entityId, isItemActor, item, _player:{name,xuid,uuid}}
 * ```
 *
 * - `entity` —— 被捡实体的类型名（`minecraft:arrow` 等）
 * - `isItemActor` —— 是不是掉落物。按上面的分工，当前的钩点集合下它**恒为
 *   false**；字段保留是为了载荷形状稳定：将来若真的把 `ItemActor` 也纳进
 *   来，订阅方不用改解析代码就能分辨两类。
 * - `item` —— 掉落物时是里面的物品名；非掉落物时为空串
 *
 * `x/y/z` 是玩家位置的整数，和本目录其它合成事件一致（LL 的反射把 Vec3 序
 * 列化成 JSON **数组**，按 `{x,y,z}` 读的消费方什么都读不到）。
 */
#include "pier/hooks/hook_events.h"

#include <set>
#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorCategory.h"
#include "mc/world/actor/item/ItemActor.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/actor/projectile/Arrow.h"
#include "mc/world/actor/projectile/ThrownTrident.h"
#include "mc/world/item/ItemStack.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& takeDef(); // 前向 —— gDef 定义在本文件末尾

        /** 被捡实体的类型名。取不到就给空串而不是猜。 */
        std::string safeActorType(Actor const& a)
        {
            try
            {
                return std::string{a.getTypeName()};
            }
            catch (...)
            {
                return {};
            }
        }

        /** 掉落物里装的是什么。非掉落物返回空串。 */
        std::string carriedItemName(Actor const& a, bool isItem)
        {
            if (!isItem) return {};
            try
            {
                auto const& stack = static_cast<ItemActor const&>(a).item();
                return std::string{stack.getTypeName()};
            }
            catch (...)
            {
                return {};
            }
        }

        std::string buildSnbt(Player& p, Actor const& taken, bool isItem)
        {
            auto const pos = p.getPosition();
            return std::string{"{\"eventId\":\"PlayerTakeEntityEvent\",\"x\":"}
                + snbtNum(static_cast<int>(pos.x)) + ",\"y\":" + snbtNum(static_cast<int>(pos.y))
                + ",\"z\":" + snbtNum(static_cast<int>(pos.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(p.getDimensionId()))
                + ",\"entity\":\"" + snbtEscape(safeActorType(taken))
                + "\",\"entityId\":"
                + snbtNum(static_cast<int64_t>(taken.getOrCreateUniqueID().rawID))
                + "L,\"isItemActor\":" + (isItem ? "true" : "false")
                + ",\"item\":\"" + snbtEscape(carriedItemName(taken, isItem))
                + "\"," + playerRefSnbt(p) + "}";
        }

        /** 每种实体类型打一次到达证明。
         *
         *  这条日志的用途是把「箭矢到底有没有走到这里」变成可以确认的事实
         *  —— 「钩子没装上」「装错了函数」「装对了但判定放行」三种情况的现象
         *  完全一样，没有它只能靠猜，而上一版正是挂错了函数却看不出来。 */
        void logFirstTouch(Actor const& a)
        {
            static std::set<std::string> seen;
            std::string const key = safeActorType(a);
            if (seen.insert(key).second)
            {
                hostLogger().debug("[TakeEntityEvent] 首次触碰 '{}'", key);
            }
        }

        /**
         * 拦一类投射物的拾取。
         *
         * `playerTouch` 返回 void，没法「取消」—— 拦截方式是**不调用
         * origin**：不调用就等于这次触碰什么都没发生，实体留在原地，玩家什么
         * 也没拿到，下次走过去还能再试。
         */
#define PIER_PICKUP_HOOK(HookName, ActorClass)                                                  \
    LL_TYPE_INSTANCE_HOOK(                                                                      \
        /* 虚函数必须挂 $ 前缀那份 —— LeviLamina 用它绕开 vtable 派发；                        \
         * 直接取 &Cls::playerTouch 会被 static_assert 拦下。同 DropItemEvent。 */              \
        HookName, ll::memory::HookPriority::Normal, ActorClass, &ActorClass::$playerTouch, void, \
        ::Player& player)                                                                       \
    {                                                                                           \
        auto& def = takeDef();                                                                  \
        if (!def.live())                                                                        \
        {                                                                                       \
            origin(player);                                                                     \
            return;                                                                             \
        }                                                                                       \
        logFirstTouch(*this);                                                                   \
        if (dispatchHookEventCancellable(def, buildSnbt(player, *this, false)))                 \
        {                                                                                       \
            /* 不调 origin = 这次触碰什么都没发生。实体留在原地，可以再试。 */                  \
            return;                                                                             \
        }                                                                                       \
        origin(player);                                                                         \
    }

        PIER_PICKUP_HOOK(ArrowPickupHook, Arrow)
        PIER_PICKUP_HOOK(TridentPickupHook, ThrownTrident)

#undef PIER_PICKUP_HOOK

        HookEventDef gDef{
            "PlayerTakeEntityEvent",
            []
            {
                // 和 DropItemEvent 一样显式装、显式报状态：0 == 成功。
                // 装失败必须看得见 —— 一个没装上的保护和「装上了但从不拦」在
                // 行为上完全一样，而这正是箭矢那个 bug 拖了这么久的原因。
                int ra = ArrowPickupHook::hook();
                int rt = TridentPickupHook::hook();
                auto& log = hostLogger();
                log.debug(
                    "[TakeEntityEvent] 安装 detour：Arrow::$playerTouch={} (code={})，"
                    "ThrownTrident::$playerTouch={} (code={})",
                    ra == 0 ? "成功" : "失败", ra,
                    rt == 0 ? "成功" : "失败", rt);
                if (ra != 0 || rt != 0)
                {
                    log.error(
                        "[TakeEntityEvent] 原生 detour 安装失败（非 0 状态码）。"
                        "结果：拾取保护对**箭矢、三叉戟等投射物**完全不生效"
                        "（掉落物仍由 PlayerPickUpItemEvent 覆盖）。"
                        "最常见原因是本宿主链接的 BDS/LeviLamina 版本与服务器"
                        "实际运行的版本不一致，导致 $playerTouch 的符号地址解析错误。");
                }
            }
        };

        HookEventDef& takeDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
