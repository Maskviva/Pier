/**
 * hooks/player/GameModeEvent.cpp —— 合成事件 "PlayerChangeGameModeEvent"，可取消。
 *
 * 只在进服和跨维度时套用模式覆盖不了入口之后的 /gamemode、命令方块、记分板触
 * 发器。挂点是虚函数 Player::$setPlayerGameType，所有改模式的路径都从这里过；
 * 内层非虚的 _setPlayerGameType 是实现细节，不挂。
 *
 * 订阅方在回调里回设模式不会自激：目标模式必在允许集合内，判定幂等。仍加一道
 * 重入闸，防止把目标模式判为不允许的订阅方无限递归到引擎里崩掉（栈上全是同一
 * 帧，日志无线索）。重入时直接放行。取消即不调 origin，服务端不发变更包，客户
 * 端一拍内自行对齐。
 *
 * 载荷 {eventId, x, y, z, dim, from, to, _player:{…}}。from/to 是 ::GameType 的
 * 整数值原样（-1 Undefined、0 Survival、1 Creative、2 Adventure、5 Default、
 * 6 Spectator），不折算成自有编号，折算表会和引擎分叉。
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

            // 没变就不问。每次重生、每次切维度引擎都会把当前模式再设一遍。
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
                // void 返回值，取消即不调 origin：模式没动，服务端不发变更包。
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
                return r == 0;
            }
        };
        HookEventDef& gameModeDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
