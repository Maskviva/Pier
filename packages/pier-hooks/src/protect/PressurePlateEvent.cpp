/**
 * hooks/protect/PressurePlateEvent.cpp —— "PlayerStepOnPressurePlateEvent"：
 * 站在压力板上或绊线里的玩家即将触发它，**并且可以取消**。
 *
 * # 这一个和这里其他钩子有什么不同
 *
 * 这个桥里其他每一项保护都挂在一次明确的玩家**动作**上：一次点击、一次挥
 * 击、一次丢弃。压力板没有动作 —— 走路才是动作，板子是作为副作用触发的。
 * 没有任何玩家事件可以订阅，因为从引擎的角度看，玩家什么都没做。这也是模
 * 组侧最初猜的那些名字（`PlayerTogglePressurePlateEvent` 之类）永远解析不
 * 到的原因。
 *
 * # 两个钩点，以及为什么只有第一个不够
 *
 * 这个文件的第一版只钩了 `shouldTriggerEntityInside`，照抄
 * LegacyScriptEngine 的 `onStepOnPressurePlate`。在这个 BDS 构建上，那个钩
 * 子装得干干净净、却什么也拦不住：刚被拒绝的玩家踩板子照样触发。这个教训值
 * 得写下来 —— **「一个公认好用的插件钩在这里」是这个符号存在的证据，不是它
 * 在你的构建里位于路径上的证据。**
 *
 * `entityInside` 才是路径。它是「有实体和这个方块重叠」的标准 `BlockType`
 * 回调，仙人掌扎人、蜘蛛网减速走的是同一个，而对这两种方块，它就是跑触发逻
 * 辑的那一个：
 *
 *   `BasePressurePlateBlock::entityInside` → `checkPressed`（所有板子类型：
 *       石头、木头、每种木料变体、两种称重板 —— `PressurePlateBlock` 和
 *       `WeightedPressurePlateBlock` 都没有再覆写它，所以钩基类能全覆盖）
 *   `TripWireBlock::entityInside`          → `_checkPressed` → 拉响绊线钩
 *
 * 它返回 void，所以取消就是不调 origin：板子从不按下、绊线从不拉响、没有任
 * 何红石信号发出。
 *
 * `shouldTriggerEntityInside` 作为便宜的提前退出保留 —— 在引擎确实会咨询它
 * 的构建上有用。两个都留的代价是多一次缓存查询，换来的是不再依赖「这个特定
 * 构建恰好调的是哪一个」。
 *
 * # 节流
 *
 * 两个虚函数对方块内的每个实体每 tick 都跑。为什么这个缓存是**必需**而不是
 * 优化、以及为什么按那样的键控，见 decision_throttle.h。
 *
 * # 载荷
 *
 * ```text
 * {eventId, x, y, z, dim, kind, _player:{name,xuid,uuid}}
 * ```
 *
 * `kind` 是 `"pressure_plate"` 或 `"tripwire"`。
 */
#include "pier/hooks/decision_throttle.h"
#include "pier/hooks/hook_events.h"
#include "pier/support/log.h"

#include <string>
#include <unordered_map>

#include "ll/api/memory/Hook.h"

#include "mc/world/actor/Actor.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/BasePressurePlateBlock.h"
#include "mc/world/level/block/TripWireBlock.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& plateDef();      // 前向：玩家踩
        HookEventDef& actorPlateDef(); // 前向：非玩家实体踩

        std::unordered_map<std::string, ThrottledDecision>& plateCache()
        {
            static std::unordered_map<std::string, ThrottledDecision> c;
            return c;
        }

        /** 非玩家实体用**另一张**表：两类键（xuid / 实体 id）落在同一张表里会
         *  互相挤占那 512 条上限，而实体的数量级远大于玩家。 */
        std::unordered_map<std::string, ThrottledDecision>& actorPlateCache()
        {
            static std::unordered_map<std::string, ThrottledDecision> c;
            return c;
        }

        /**
         * 该拒绝这次触发时返回 true。每个（玩家, 方块位置）在
         * kDecisionTtlMs 内至多派发一次。
         */
        bool refuseTrigger(
            ::Actor& entity, ::BlockSource& region, ::BlockPos const& pos, char const* kind)
        {
            bool const isPlayer = entity.isPlayer();
            auto& def = isPlayer ? plateDef() : actorPlateDef();
            if (!def.live()) return false;

            Player* p = isPlayer ? static_cast<Player*>(&entity) : nullptr;

            // 节流键：玩家用 xuid（指针会被回收，见 decision_throttle.h），
            // 非玩家用「类型名 + 实体 id」—— id 同样不复用，比裸指针安全。
            std::string key;
            if (p)
            {
                key = p->getXuid();
                if (key.empty()) key = p->getRealName(); // 离线模式服务器
            }
            else
            {
                try
                {
                    key = std::to_string(entity.getOrCreateUniqueID().rawID);
                }
                catch (...)
                {
                    return false; // 连 id 都问不出来的实体不值得为它派发
                }
            }

            int const dim = static_cast<int>(region.getDimensionId());
            long long const now = throttleNowMs();

            bool cached = false;
            auto& cache = isPlayer ? plateCache() : actorPlateCache();
            if (throttleLookup(cache, key, pos.x, pos.y, pos.z, dim, now, cached))
            {
                return cached;
            }

            std::string snbt = isPlayer
                ? std::string{"{\"eventId\":\"PlayerStepOnPressurePlateEvent\""}
                : std::string{"{\"eventId\":\"ActorStepOnPressurePlateEvent\""};
            snbt += ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"dim\":" + snbtNum(dim)
                + ",\"kind\":\"" + kind + "\"";
            if (p)
            {
                snbt += "," + playerRefSnbt(*p);
            }
            else
            {
                std::string type;
                try
                {
                    type = entity.getTypeName();
                }
                catch (...)
                {
                    type.clear();
                }
                snbt += ",\"actor\":\"" + snbtEscape(type) + "\",\"actorId\":" + key + "L";
            }
            snbt += "}";

            bool const cancelled = dispatchHookEventCancellable(def, snbt);
            throttleStore(cache, key, pos.x, pos.y, pos.z, dim, now, cancelled);
            return cancelled;
        }

        // ── 真正跑触发逻辑的那条路 ──────────────────────────────────────

        LL_TYPE_INSTANCE_HOOK(
            PressurePlateInsideHook,
            ll::memory::HookPriority::Normal,
            BasePressurePlateBlock,
            &BasePressurePlateBlock::$entityInside,
            void,
            ::BlockSource& region,
            ::BlockPos const& pos,
            ::Actor& entity)
        {
            if (refuseTrigger(entity, region, pos, "pressure_plate")) return;
            origin(region, pos, entity);
        }

        LL_TYPE_INSTANCE_HOOK(
            TripWireInsideHook,
            ll::memory::HookPriority::Normal,
            TripWireBlock,
            &TripWireBlock::$entityInside,
            void,
            ::BlockSource& region,
            ::BlockPos const& pos,
            ::Actor& entity)
        {
            if (refuseTrigger(entity, region, pos, "tripwire")) return;
            origin(region, pos, entity);
        }

        // ── 便宜的提前退出（引擎会咨询它的构建上生效）──────────────────

        LL_TYPE_INSTANCE_HOOK(
            PressurePlateShouldTriggerHook,
            ll::memory::HookPriority::Normal,
            BasePressurePlateBlock,
            &BasePressurePlateBlock::$shouldTriggerEntityInside,
            bool,
            ::BlockSource& region,
            ::BlockPos const& pos,
            ::Actor& entity)
        {
            if (refuseTrigger(entity, region, pos, "pressure_plate")) return false;
            return origin(region, pos, entity);
        }

        LL_TYPE_INSTANCE_HOOK(
            TripWireShouldTriggerHook,
            ll::memory::HookPriority::Normal,
            TripWireBlock,
            &TripWireBlock::$shouldTriggerEntityInside,
            bool,
            ::BlockSource& region,
            ::BlockPos const& pos,
            ::Actor& entity)
        {
            if (refuseTrigger(entity, region, pos, "tripwire")) return false;
            return origin(region, pos, entity);
        }

        HookEventDef gDef{
            "PlayerStepOnPressurePlateEvent",
            []
            {
                int const r1 = PressurePlateInsideHook::hook();
                int const r2 = TripWireInsideHook::hook();
                int const r3 = PressurePlateShouldTriggerHook::hook();
                int const r4 = TripWireShouldTriggerHook::hook();
                if (r1 != 0 || r2 != 0 || r3 != 0 || r4 != 0)
                {
                    hostLogger().error(
                        "[PressurePlateEvent] 有 detour 未装上（codes: {} {} {} {}）—— 订阅被拒绝。",
                        r1, r2, r3, r4);
                }
                return r1 == 0 && r2 == 0 && r3 == 0 && r4 == 0;
            }
        };
        HookEventDef& plateDef() { return gDef; }

        // 同一组 detour 供两个事件 id 使用（同 RideEvent）。
        HookEventDef gActorDef{
            "ActorStepOnPressurePlateEvent",
            []
            {
                int const r1 = PressurePlateInsideHook::hook();
                int const r2 = TripWireInsideHook::hook();
                int const r3 = PressurePlateShouldTriggerHook::hook();
                int const r4 = TripWireShouldTriggerHook::hook();
                return r1 == 0 && r2 == 0 && r3 == 0 && r4 == 0;
            }
        };
        HookEventDef& actorPlateDef() { return gActorDef; }

        HookEventRegistrar gReg{gDef};
        HookEventRegistrar gActorReg{gActorDef};
    } // namespace
} // namespace pier::hooks
