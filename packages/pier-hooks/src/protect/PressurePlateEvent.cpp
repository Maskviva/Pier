/**
 * hooks/protect/PressurePlateEvent.cpp —— 合成事件
 * "PlayerStepOnPressurePlateEvent"，可取消。
 *
 * 压力板没有玩家动作可挂：走路才是动作，板子是副作用，从引擎角度看玩家什么都
 * 没做。真正跑触发逻辑的是 entityInside（BasePressurePlateBlock 覆盖全部板子
 * 类型，TripWireBlock 走 _checkPressed）；它返回 void，取消即不调 origin，板子
 * 不按下、绊线不拉响、不发红石信号。shouldTriggerEntityInside 作为便宜的提前退
 * 出一并保留，代价是多一次缓存查询，换来不依赖某个构建恰好调哪一个。
 *
 * 两个虚函数对方块内每个实体每 tick 都跑，节流缓存是必需而非优化，键的选法见
 * decision_throttle.h。载荷 {eventId, x, y, z, dim, kind, _player:{…}}，
 * kind 为 "pressure_plate" 或 "tripwire"。
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

        /** 非玩家实体用另一张表：两类键落在同一张表会互相挤占 512 条上限，
         *  而实体的数量级远大于玩家。 */
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

        // 真正跑触发逻辑的那条路。

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

        // 便宜的提前退出，在引擎确实会咨询它的构建上生效。

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
