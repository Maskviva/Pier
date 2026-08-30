/**
 * hooks/player/GameModeEvent.cpp —— "PlayerChangeGameModeEvent"：玩家的游戏
 * 模式即将被改变，**可以取消**。
 *
 * # 为什么进世界时套一次模式不够
 *
 * 模组已经在「进服 / 跨维度」那两条路上套用了世界允许的游戏模式。那只覆盖
 * 了**进入的那一瞬间**：玩家进了生存世界之后再打一句 `/gamemode creative`，
 * 或者别的插件、命令方块、一个记分板触发器把他改回去，都没有任何人再问一
 * 次。「强制」如果只在入口生效，那它不是强制，是一次建议。
 *
 * # 挂载点
 *
 * `Player::setPlayerGameType(GameType)` 是虚函数，`&Player::$setPlayerGameType`
 * 可挂。它是**所有**改模式的必经之路：`/gamemode`、`/defaultgametype` 的追
 * 平、玩家死亡后的恢复、以及模组自己调的那次，全都从这里过。
 *
 * 挂虚函数而不是挂 `_setPlayerGameType`（非虚的内层实现）是有意的：内层那个
 * 是实现细节，而外层这个是引擎自己的语义边界。
 *
 * # 不会自激
 *
 * 模组收到事件之后往往会**再设一次**模式（把玩家掰回允许的那一个）。那一次
 * 同样会走到这里，但它的目标模式一定在允许集合里，于是订阅方放行、递归到此
 * 为止。这不是靠一个「正在处理中」的标志位挡住的 —— 判定本身是幂等的，所以
 * 不需要。
 *
 * 尽管如此还是加了一层**重入保护**：订阅方在回调里直接改模式（而不是排到下
 * 一拍）时，`origin` 还没返回就又进来一次。放着不管的话，一个把目标模式当成
 * 「不允许」的订阅方会无限递归，而崩溃点在引擎里、栈上全是同一帧，日志里什
 * 么线索都没有。重入时**直接放行**：内层那次是订阅方自己发起的，它不需要再
 * 被问一遍。
 *
 * # 取消 = 不调 origin
 *
 * 返回类型是 void，所以「取消」就是什么都不做。玩家的模式保持原样，客户端会
 * 在一拍之内自己对齐回来（服务端从没发过变更）。这比「先改再改回去」干净得
 * 多 —— 后者玩家会看到界面闪一下，而且中间那一拍里他真的处在创造模式。
 *
 * # 载荷
 *
 * ```text
 * {eventId, x, y, z, dim, from, to, _player:{name,xuid,uuid}}
 * ```
 *
 * `from` / `to` 是 `::GameType` 的整数值**逐值原样**：
 * `-1 Undefined · 0 Survival · 1 Creative · 2 Adventure · 5 Default · 6 Spectator`。
 * 不折算成一套自己的编号 —— 折算表迟早会和引擎分叉，而分叉的表现是「关掉旁
 * 观模式之后玩家还是能进旁观」。
 */
#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/GameType.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& gameModeDef(); // 前向

        /** 见文件头「不会自激」。派发期间再次进来一律放行。 */
        bool gDispatching = false;

        std::string buildSnbt(Player& p, int from, int to)
        {
            auto const& pos = p.getPosition();
            return "{\"eventId\":\"PlayerChangeGameModeEvent\""
                ",\"x\":" + snbtNum(static_cast<int>(pos.x))
                + ",\"y\":" + snbtNum(static_cast<int>(pos.y))
                + ",\"z\":" + snbtNum(static_cast<int>(pos.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(p.getDimensionId()))
                + ",\"from\":" + snbtNum(from)
                + ",\"to\":" + snbtNum(to)
                + "," + playerRefSnbt(p) + "}";
        }

        LL_TYPE_INSTANCE_HOOK(
            PlayerChangeGameModeHook,
            ll::memory::HookPriority::Normal,
            Player,
            &Player::$setPlayerGameType,
            void,
            ::GameType gameType)
        {
            auto& def = gameModeDef();
            if (!def.live() || gDispatching)
            {
                return origin(gameType);
            }

            int const from = static_cast<int>(this->getPlayerGameType());
            int const to = static_cast<int>(gameType);

            // 没变就别问。玩家每次重生、每次切维度，引擎都会把当前模式再设一
            // 遍；不挡掉的话订阅方每分钟要被叫醒几十次去回答一个没有内容的问
            // 题。
            if (from == to)
            {
                return origin(gameType);
            }

            std::string snbt = buildSnbt(*this, from, to);

            bool cancelled = false;
            {
                gDispatching = true;
                struct Reset
                {
                    ~Reset() { gDispatching = false; }
                } reset;
                cancelled = dispatchHookEventCancellable(def, snbt);
            }
            if (cancelled)
            {
                // void 返回值，所以「取消」就是不调 origin：模式一格没动，
                // 服务端也就没发出任何变更包。
                return;
            }
            return origin(gameType);
        }

        HookEventDef gDef{
            "PlayerChangeGameModeEvent",
            []
            {
                int const r = PlayerChangeGameModeHook::hook();
                auto& log = hostLogger();
                log.debug(
                    "[GameModeEvent] 安装 detour：PlayerChangeGameModeHook={} (code={})",
                    r == 0 ? "成功" : "失败", r);
                if (r != 0)
                {
                    log.error(
                        "[GameModeEvent] 原生 detour 安装失败（非 0 状态码）。最常见原因是"
                        "本宿主链接的 BDS/LeviLamina 版本与服务器实际运行的版本不一致，"
                        "导致 Player::$setPlayerGameType 的符号地址解析错误。结果：**游戏模式"
                        "强制只在进入世界的那一刻生效**，玩家进去之后自己 /gamemode 就能绕过，"
                        "而且不会有任何拦截日志。请用服务器实际运行的版本重新编译本宿主。");
                }
            }
        };
        HookEventDef& gameModeDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
